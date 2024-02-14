// Copyright (C) 2022-2024  ilobilo

#include <drivers/fs/dev/tty/tty.hpp>
#include <drivers/fs/devtmpfs.hpp>
#include <drivers/proc.hpp>
#include <drivers/fd.hpp>
#include <lib/panic.hpp>

#include <lib/log.hpp>

// #include <drivers/term.hpp>

namespace tty
{
    tty_t *active_tty;

    struct cdev_t : vfs::cdev_t
    {
        tty_t *tty;

        cdev_t(tty_t *tty) : tty(tty) { }

        bool open(vfs::resource *res, vfs::fdhandle *fd, proc::process *proc);

        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count);
        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count);

        int ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp);
    };

    bool cdev_t::open(vfs::resource *res, vfs::fdhandle *fd, proc::process *proc)
    {
        // this works, right?
        if (this->tty->session.expired() && !(fd->flags & o_noctty))
        {
            this->tty->session = proc->session;
            proc->session->tty = this->tty;
        }
        return true;
    }

    ssize_t cdev_t::read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
    {
        auto charbuf = static_cast<char*>(buffer);

        if (this->tty->termios.c_lflag & icanon)
        {
            this->tty->canon_event.await();
            std::unique_lock guard(this->tty->input_lock);

            auto line = this->tty->canon_lines.pop_front_element();
            return line.copy(reinterpret_cast<char*>(buffer), count);
        }

        auto min = this->tty->termios.c_cc[vmin];
        auto time = this->tty->termios.c_cc[vtime];
        auto ms = time * 100;

        auto await = [&]
        {
            return ms == 0 ?
                this->tty->raw_event.await() :
                this->tty->raw_event.await_timeout(ms);
        };

        if (min != 0 && time != 0)
        {
            PANIC("TTY: TODO: min != 0 && time != 0");
        }
        else if (min == 0 && time != 0)
        {
            while (this->tty->raw_queue.empty())
                if (await().has_value() == false)
                    return 0;
        }
        else if (min != 0 && time == 0)
        {
            // auto max = std::max(count, size_t(min));
            while (this->tty->raw_queue.size() < size_t(min))
                await();
        }

        std::unique_lock guard(this->tty->input_lock);

        size_t ret = 0;
        while (this->tty->raw_queue.empty() == false && ret < count)
            charbuf[ret++] = this->tty->raw_queue.pop_back_element();

        return ret;
    }

    ssize_t cdev_t::write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
    {
        // TMP
        auto cbuf = static_cast<const char *>(buffer);
        auto str = std::string_view(cbuf, std::min(strlen(cbuf), count));
        if (str.starts_with("mlibc: sys_") && str.ends_with("() is a stub\n"))
        {
            fmt::print("{}", str);
            return count;
        }

        std::unique_lock guard(this->tty->output_lock);
        for (const auto ch : std::string_view(static_cast<const char *>(buffer), count))
            this->tty->print(ch);

        return count;
    }

    int cdev_t::ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp)
    {
        switch (request)
        {
            case tcgets:
            {
                std::unique_lock iguard(this->tty->input_lock);
                std::unique_lock oguard(this->tty->output_lock);
                *reinterpret_cast<termios*>(argp) = this->tty->termios;
                return 0;
            }
            case tcsets:
            case tcsetsw:
            case tcsetsf:
            {
                std::unique_lock iguard(this->tty->input_lock);
                std::unique_lock oguard(this->tty->output_lock);
                this->tty->termios = *reinterpret_cast<termios*>(argp);
                return 0;
            }
            default:
            {
                auto ret = this->tty->ioctl(res, fd, request, argp);
                if (ret < 0)
                    return_err(-1, ENOTTY);
                return ret;
            }
        }
        std::unreachable();
    }

    void tty_t::internal_add_to_buf(char c)
    {
        auto is_control = [](auto ch)
        {
	        return ch < ' ' || ch == 0x7F;
        };

        auto output = [&](auto ch)
        {
            if (ch == '\n' && this->termios.c_oflag & onlcr)
            {
                this->print('\n');
                this->print('\r');
                return;
            }
            if (ch == '\r' && this->termios.c_oflag & onlret)
                return;

            this->print(ch);
        };

        auto erase_one = [&](bool erase)
        {
            if (this->canon_queue.empty() == false)
            {
                auto count = 1;
                if (is_control(this->canon_queue.pop_back_element()))
                    count = 2;
                if (this->termios.c_lflag & echo)
                {
                    if (erase == true)
                    {
                        while (count--)
                        {
                            output('\b');
                            output(' ');
                            output('\b');
                        }
                    }
                }
            }
        };

        auto move_to_output = [&]
        {
            // TODO: Linux allows max 4095 chars, should we do that too?
            this->canon_lines.emplace_back(this->canon_queue.begin(), this->canon_queue.end());
            this->canon_queue.clear();
            this->canon_event.trigger();
        };

        if (this->next_is_verbatim == true)
        {
            this->next_is_verbatim = false;
            this->canon_queue.push_back(c);
            if (this->termios.c_lflag & echo)
            {
                if (is_control(c))
                {
                    output('^');
                    output(('@' + c) % 128);
                }
                else output(c);
            }
            return;
        }

        // TODO if c_lflag & isig

        if (this->termios.c_iflag & istrip)
            c &= 0x7F;

        if (this->termios.c_iflag & igncr && c == '\r')
            return;

        if (this->termios.c_iflag & inlcr && c == '\n')
            c = '\r';
        else if (this->termios.c_iflag & icrnl && c == '\r')
            c = '\n';

        if (this->termios.c_lflag & icanon)
        {
            if (c == this->termios.c_cc[vlnext] && this->termios.c_lflag & iexten)
            {
                this->next_is_verbatim = true;
                output('^');
                output('\b');
                return;
            }

            if (c == this->termios.c_cc[vkill])
            {
                while (this->canon_queue.empty() == false)
                    erase_one(this->termios.c_lflag & echok);
                if (this->termios.c_lflag & echo && !(this->termios.c_lflag & echok))
                {
                    output('^');
                    output(('@' + c) % 128);
                }
                return;
            }

            if (c == this->termios.c_cc[verase])
            {
                erase_one(this->termios.c_lflag & echoe);
                if (this->termios.c_lflag & echo && !(this->termios.c_lflag & echoe))
                {
                    output('^');
                    output(('@' + c) % 128);
                }
                return;
            }

            if (c == this->termios.c_cc[vwerase] && this->termios.c_lflag & iexten)
            {
                while (this->canon_queue.empty() == false && this->canon_queue.back() == ' ')
                    erase_one(this->termios.c_lflag & echoe);

                while (this->canon_queue.empty() == false && this->canon_queue.back() != ' ')
                    erase_one(this->termios.c_lflag & echoe);

                if (this->termios.c_lflag & echo && !(this->termios.c_lflag & echoe))
                {
                    output('^');
                    output(('@' + c) % 128);
                }
                return;
            }

            if (c == this->termios.c_cc[veof])
            {
                if (this->canon_queue.empty() == false)
                    move_to_output();
                return;
            }

            this->canon_queue.push_back(c);
            if (this->termios.c_lflag & echo)
            {
                if (is_control(c) && c != '\n')
                {
                    output('^');
                    output(('@' + c) % 128);
                }
                else output(c);
            }

            if (c == '\n' || (this->termios.c_cc[veol] && c == this->termios.c_cc[veol]))
            {
                if (!(this->termios.c_lflag & echo) && this->termios.c_lflag & echonl)
                    output(c);
                this->canon_queue.back() = c;
                move_to_output();
                return;
            }

            return;
        }
        else if (this->termios.c_lflag & echo)
            output(c);

        this->raw_queue.push_back(c);
        this->raw_event.trigger(true);
    }

    void register_tty(dev_t dev, tty_t *tty)
    {
        devtmpfs::register_dev(new cdev_t(tty), dev);

        if (tty::active_tty == nullptr)
        {
            tty::active_tty = tty;

            auto cdev = makedev(5, 1);
            devtmpfs::register_dev(new cdev_t(tty), cdev);
            devtmpfs::add_dev("console", cdev, 0620 | s_ifchr);
        }
    }

    struct self_cdev_t : vfs::cdev_t
    {
        bool open(vfs::resource *res, vfs::fdhandle *fd, proc::process *proc)
        {
            auto tty = this_thread()->parent->session->tty;
            if (tty == nullptr)
                return_err(false, ENODEV);
            return tty::cdev_t(tty).open(res, fd, proc);
        }
        ssize_t read(vfs::resource *res, vfs::fdhandle *fd, void *buffer, off_t offset, size_t count)
        {
            auto tty = this_thread()->parent->session->tty;
            if (tty == nullptr)
                return_err(false, ENODEV);
            return tty::cdev_t(tty).read(res, fd, buffer, offset, count);
        }
        ssize_t write(vfs::resource *res, vfs::fdhandle *fd, const void *buffer, off_t offset, size_t count)
        {
            auto tty = this_thread()->parent->session->tty;
            if (tty == nullptr)
                return_err(false, ENODEV);
            return tty::cdev_t(tty).write(res, fd, buffer, offset, count);
        }

        int ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp)
        {
            auto tty = this_thread()->parent->session->tty;
            if (tty == nullptr)
                return_err(false, ENODEV);
            return tty::cdev_t(tty).ioctl(res, fd, request, argp);
        }
    };

    void init()
    {
        auto dev = makedev(5, 0);
        devtmpfs::register_dev(new self_cdev_t, dev);
        devtmpfs::add_dev("tty", dev, 0666 | s_ifchr);
    }
} // namespace tty
// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <drivers/vfs.hpp>
#include <lib/event.hpp>
#include <deque>

namespace proc { struct session; }
namespace tty
{
    struct cdev_t;
    struct tty_t
    {
        private:
        void internal_add_to_buf(char c);

        public:
        termios termios;

        lock_t input_lock;
        lock_t output_lock;

        std::deque<char> raw_queue;
        event_t raw_event;

        std::deque<std::string> canon_lines;
        std::deque<char> canon_queue;
        event_t canon_event;

        bool next_is_verbatim;

        std::weak_ptr<proc::session> session;

        tty_t() : input_lock(), output_lock(), next_is_verbatim(false)
        {
            this->termios.c_iflag = icrnl | ixon;
            this->termios.c_oflag = opost | onlcr;
            this->termios.c_cflag = 15 /* b38400 */ | cs8 | cread | hupcl;
            this->termios.c_lflag = isig | icanon | echo | echoe | echok | echoctl | echoke | iexten;

            this->termios.c_cc[vintr] = 'C' - 0x40;
            this->termios.c_cc[vquit] = '\\' - 0x40;
            this->termios.c_cc[verase] = '\177';
            this->termios.c_cc[vkill] = 'U' - 0x40;
            this->termios.c_cc[veof] = 'D' - 0x40;
            this->termios.c_cc[vstart] = 'Q' - 0x40;
            this->termios.c_cc[vstop] = 'S' - 0x40;
            this->termios.c_cc[vsusp] = 'Z' - 0x40;
            this->termios.c_cc[vreprint] = 'R' - 0x40;
            this->termios.c_cc[vwerase] = 'W' - 0x40;
            this->termios.c_cc[vlnext] = 'V' - 0x40;
            // this->termios.c_cc[vdsusp] = 'Y' - 0x40;
            this->termios.c_cc[vmin] = 1;

            this->termios.ispeed = 38400;
            this->termios.ospeed = 38400;
        }

        virtual void print(char c) { }
        virtual int ioctl(vfs::resource *res, vfs::fdhandle *fd, size_t request, uintptr_t argp) { return -1; }

        void add_to_buf(char c)
        {
            lockit(this->input_lock);
            this->internal_add_to_buf(c);
        }
        void add_to_buf(auto str)
        {
            lockit(this->input_lock);
            for (const auto c : str)
                this->internal_add_to_buf(c);
        }
    };

    extern tty_t *active_tty;

    void register_tty(dev_t dev, tty_t *tty);
    void init();
} // namespace tty
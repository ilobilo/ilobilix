// Copyright (C) 2022  ilobilo

#include <drivers/proc.hpp>
#include <drivers/vfs.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

// This code is **inspired** by lyre-os

namespace vmm
{
    namespace mmap
    {
        struct global
        {
            std::vector<std::shared_ptr<mmap::local>> locals;
            std::unique_ptr<pagemap> shadow;

            vfs::resource *res;
            uintptr_t base;
            size_t length;
            off_t offset;

            bool mmap_page(uintptr_t vaddr, uintptr_t paddr, int prot);
        };

        struct local
        {
            std::shared_ptr<mmap::global> global;
            pagemap *pagemap;

            uintptr_t base;
            size_t length;
            off_t offset;

            int prot;
            int flags;
        };

        bool global::mmap_page(uintptr_t vaddr, uintptr_t paddr, int prot)
        {
            size_t flags = user;

            if (prot & PROT_READ)
                flags |= read;
            if (prot & PROT_WRITE)
                flags |= write;
            if (prot & PROT_EXEC)
                flags |= exec;

            if (!this->shadow->map(vaddr, paddr, flags))
                return false;

            for (const auto &local : this->locals)
            {
                if (vaddr < local->base || vaddr >= local->base + local->length)
                    continue;

                if (!local->pagemap->map(vaddr, paddr, flags))
                    return false;
            }

            return true;
        }
    } // namespace mmap

    std::optional<std::tuple<std::shared_ptr<mmap::local>, size_t, size_t>> pagemap::addr2range(uintptr_t addr)
    {
        lockit(this->lock);

        for (const auto &local : this->ranges)
        {
            if (addr < local->base || addr >= local->base + local->length)
                continue;

            size_t mpage = addr / pmm::page_size;
            size_t fpage = local->offset / pmm::page_size + (mpage - local->base / pmm::page_size);
            return std::tuple<std::shared_ptr<mmap::local>, size_t, size_t> { local, mpage, fpage };
        }
        return std::nullopt;
    }

    pagemap::~pagemap()
    {
        while (this->ranges.size() > 0)
        {
            auto local = this->ranges.at(0);
            this->munmap(local->base, local->length);
        }
        arch_destroy_pmap(this);
    }

    bool pagemap::mmap_range(uintptr_t vaddr, uintptr_t paddr, size_t length, int prot, int flags)
    {
        flags |= MAP_ANONYMOUS;

        auto aligned_vaddr = align_down(vaddr, this->page_size);
        auto aligned_length = align_up(length + (vaddr - aligned_vaddr), this->page_size);

        auto global = std::make_shared<mmap::global>();
        global->shadow = std::make_unique<pagemap>();
        global->base = aligned_vaddr;
        global->length = aligned_length;

        auto local = std::make_shared<mmap::local>();
        local->global = global;
        local->pagemap = this;
        local->base = aligned_vaddr;
        local->length = aligned_length;
        local->prot = prot;
        local->flags = flags;

        global->locals.push_back(local);

        this->lock.lock();
        this->ranges.push_back(local);
        this->lock.unlock();

        for (size_t i = 0; i < aligned_length; i += this->page_size)
        {
            if (!global->mmap_page(aligned_vaddr + i, paddr + i, prot))
                return false;
        }

        return true;
    }

    void *pagemap::mmap(uintptr_t addr, size_t length, int prot, int flags, vfs::resource *res, off_t offset)
    {
        if (length == 0)
        {
            errno = EINVAL;
            return MAP_FAILED;
        }
        length = align_up(length, this->page_size);

        if (!(flags & MAP_ANONYMOUS) && res && !res->can_mmap)
        {
            errno = ENODEV;
            return MAP_FAILED;
        }

        auto proc = this_thread()->parent;

        uintptr_t base = 0;
        if (flags & MAP_FIXED)
        {
            if (!this->munmap(addr, length))
                return MAP_FAILED;
            base = addr;
        }
        else
        {
            base = proc->mmap_anon_base;
            proc->mmap_anon_base += length + this->page_size;
        }

        auto global = std::make_shared<mmap::global>();
        global->shadow = std::make_unique<pagemap>();
        global->base = base;
        global->length = length;
        global->res = res;
        global->offset = offset;

        auto local = std::make_shared<mmap::local>();
        local->global = global;
        local->pagemap = this;
        local->base = base;
        local->length = length;
        local->prot = prot;
        local->flags = flags;
        local->offset = offset;

        global->locals.push_back(local);

        this->lock.lock();
        this->ranges.push_back(local);
        this->lock.unlock();

        if (res != nullptr)
            res->refcount++;

        return reinterpret_cast<void*>(base);
    }

    bool pagemap::munmap(uintptr_t addr, size_t length)
    {
        if (length == 0)
        {
            errno = EINVAL;
            return MAP_FAILED;
        }
        length = align_up(length, this->page_size);

        for (auto i = addr; i < addr + length; i += this->page_size)
        {
            auto a2r = this->addr2range(i);
            if (a2r.has_value() == false)
                continue;

            auto [local, mpage, fpage] = a2r.value();
            auto global = local->global;

            auto begin = i;
            while (i < local->base + local->length && i < addr + length)
                i += this->page_size;
            auto end = i;
            auto len = end - begin;

            this->lock.lock();
            if (begin > local->base && end < local->base + local->length)
            {
                auto split_local = std::make_shared<mmap::local>();
                split_local->pagemap = local->pagemap;
                split_local->global = global;
                split_local->base = end;
                split_local->length = (local->base + local->length) - end;
                split_local->offset = local->offset + off_t(end - local->base);
                split_local->prot = local->prot;
                split_local->flags = local->flags;

                this->ranges.push_back(split_local);
                local->length -= split_local->length;
            }

            this->lock.unlock();
            this->unmap_range(begin, len);

            if (len == local->length)
                this->ranges.erase(std::find(this->ranges.begin(), this->ranges.end(), local));

            if (len == local->length && global->locals.size() == 1)
            {
                if (local->flags & MAP_ANONYMOUS)
                {
                    for (auto j = global->base; j < global->base + global->length; j += this->page_size)
                    {
                        auto paddr = global->shadow->virt2phys(j);
                        if (paddr == invalid_addr)
                            continue;

                        if (!global->shadow->unmap(j))
                        {
                            errno = EINVAL;
                            return false;
                        }
                        pmm::free(paddr);
                    }
                }
                // else res->unmap();
            }
            else
            {
                if (begin == local->base)
                {
                    local->offset += len;
                    local->base += end;
                }
                local->length -= len;
            }
        }

        return true;
    }

    bool page_fault(uintptr_t addr)
    {
        auto thread = this_thread();
        auto proc = thread->parent;
        auto pagemap = proc->pagemap;

        auto a2r = pagemap->addr2range(addr);

        if (a2r.has_value() == false)
            return false;

        auto [local, mpage, fpage] = a2r.value();

        void *page = nullptr;
        if (local->flags & MAP_ANONYMOUS)
            page = pmm::alloc();
        else
        {
            auto res = local->global->res;
            page = res->mmap(fpage, local->flags);
        }

        if (page == nullptr)
            return false;

        return local->global->mmap_page(mpage * pmm::page_size + 1, reinterpret_cast<uintptr_t>(page), local->prot);
    }
} // namespace vmm
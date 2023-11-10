// Copyright (C) 2022-2023  ilobilo

#include <lib/containers.hpp>

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

            if (prot & prot_read)
                flags |= read;
            if (prot & prot_write)
                flags |= write;
            if (prot & prot_exec)
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
        std::unique_lock guard(this->lock);

        for (const auto &local : this->ranges)
        {
            if (addr < local->base || addr >= local->base + local->length)
                continue;

            size_t mpage = addr / this->page_size;
            size_t fpage = local->offset / this->page_size + (mpage - local->base / this->page_size);
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
        flags |= mmap::map_anonymous;

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
            return_err(mmap::map_failed, EINVAL);
        length = align_up(length, this->page_size);

        if (!(flags & (mmap::map_anonymous | mmap::map_shared | mmap::map_private)))
            return_err(mmap::map_failed, EINVAL);

        uintptr_t base = 0;
        if (flags & mmap::map_fixed)
        {
            if (!this->munmap(addr, length))
                return mmap::map_failed;
            base = addr;
        }
        else
        {
            base = this->mmap_bump_base;
            this->mmap_bump_base += length + this->page_size;
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
            res->ref();

        return reinterpret_cast<void*>(base);
    }

    bool pagemap::mprotect(uintptr_t addr, size_t length, int prot)
    {
        if (length == 0)
            return_err(false, EINVAL);
        length = align_up(length, this->page_size);

        for (auto i = addr; i < addr + length; i += this->page_size)
        {
            auto a2r = addr2range(i);
            if (a2r.has_value() == false)
                continue;

            auto local = std::get<0>(a2r.value());
            if (local->prot == prot)
                continue;

            auto begin = i;
            while (i < local->base + local->length && i < addr + length)
                i += this->page_size;
            auto end = i;
            auto len = end - begin;

            std::unique_lock guard(this->lock);

            if (begin > local->base && end < local->base + local->length)
            {
                auto split_local = std::make_shared<mmap::local>();
                split_local->pagemap = local->pagemap;
                split_local->global = local->global;
                split_local->base = end;
                split_local->length = (local->base + local->length) - end;
                split_local->offset = local->offset + off_t(end - local->base);
                split_local->prot = local->prot;
                split_local->flags = local->flags;

                this->ranges.push_back(split_local);
                local->length -= split_local->length;
            }

            for (auto j = begin; j < end; j += this->page_size)
            {
                size_t flags = user;
                if (prot & mmap::prot_read)
                    flags |= read;
                if (prot & mmap::prot_write)
                    flags |= write;
                if (prot & mmap::prot_exec)
                    flags |= exec;

                this->setflags_nolock(j, flags);
            }

            auto new_offset = local->offset + (begin - local->base);
            if (begin == local->base)
            {
                local->offset += len;
                local->base = end;
            }
            local->base = len;

            auto new_local = std::make_shared<mmap::local>();
            new_local->pagemap = local->pagemap;
            new_local->global = local->global;
            new_local->base = begin;
            new_local->length = len;
            new_local->offset = new_offset;
            new_local->prot = prot;
            new_local->flags = local->flags;

            this->ranges.push_back(new_local);
        }
        return true;
    }

    bool pagemap::munmap(uintptr_t addr, size_t length)
    {
        if (length == 0)
        {
            errno = EINVAL;
            return mmap::map_failed;
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

            for (auto j = begin; j < end; j += this->page_size)
                this->unmap_nolock(j);

            if (len == local->length)
                erase_from(this->ranges, local);

            this->lock.unlock();

            if (len == local->length && global->locals.size() == 1)
            {
                if (local->flags & mmap::map_anonymous)
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
                // else res->unmmap();
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

    pagemap::pagemap(pagemap *other) : pagemap()
    {
        assert(this->page_size == other->page_size);
        this->mmap_bump_base = other->mmap_bump_base;

        for (const auto &local : other->ranges)
        {
            auto global = local->global;
            auto new_local = std::make_shared<mmap::local>(*local);

            if (global->res != nullptr)
                global->res->ref();

            if (local->flags & mmap::map_shared)
            {
                global->locals.push_back(new_local);
                for (uintptr_t i = local->base; i < local->base + local->length; i += other->page_size)
                {
                    auto old_pte = other->virt2pte(i, false, other->page_size);
                    if (old_pte == nullptr)
                        continue;

                    this->virt2pte(i, true, this->page_size)->value = old_pte->value;
                }
            }
            else
            {
                auto new_global = std::make_shared<mmap::global>();
                new_global->shadow = std::make_unique<pagemap>();
                new_global->res = global->res;
                new_global->base = global->base;
                new_global->length = global->length;
                new_global->offset = global->offset;

                // TODO: ?
                new_local->global = new_global;

                new_global->locals.push_back(new_local);

                assert(local->flags & mmap::map_anonymous, "non anonymous pagemap fork");
                for (uintptr_t i = local->base; i < local->base + local->length; i += other->page_size)
                {
                    auto old_pte = other->virt2pte(i, false, other->page_size);
                    if (old_pte == nullptr || !old_pte->getflags(flags2arch(0) /* valid flags */))
                        continue;

                    auto new_pte = this->virt2pte(i, true, this->page_size);
                    auto new_spte = new_global->shadow->virt2pte(i, true, new_global->shadow->page_size);

                    auto old_page = old_pte->getaddr();
                    auto page = pmm::alloc<uintptr_t>(other->page_size / pmm::page_size);

                    memcpy(reinterpret_cast<void*>(tohh(page)), reinterpret_cast<void*>(tohh(old_page)), other->page_size);
                    new_pte->value = 0;
                    new_pte->setflags(old_pte->getflags(), true);
                    new_pte->setaddr(page);
                    new_spte->value = new_pte->value;
                }
            }

            this->ranges.push_back(new_local);
        }
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
        if (local->flags & mmap::map_anonymous)
            page = pmm::alloc(pagemap->page_size / pmm::page_size);
        else
        {
            auto res = local->global->res;
            page = res->mmap(fpage, local->flags);
        }

        if (page == nullptr)
            return false;

        return local->global->mmap_page(mpage * pagemap->page_size + 1, reinterpret_cast<uintptr_t>(page), local->prot);
    }
} // namespace vmm
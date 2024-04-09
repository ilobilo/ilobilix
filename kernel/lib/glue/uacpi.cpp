// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/proc.hpp>

#include <init/kernel.hpp>

#include <arch/arch.hpp>

#include <lib/interrupts.hpp>
#include <lib/event.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>

#include <lib/mmio.hpp>
#include <lib/io.hpp>

#include <mm/vmm.hpp>

#include <utility>
#include <deque>

#include <uacpi/kernel_api.h>

namespace uacpi
{
    namespace
    {
        using queue_type = std::deque<std::pair<uacpi_work_handler, uacpi_handle>>;
        queue_type notify_work_queue { };
        queue_type gpe_work_queue { };

        std::atomic_size_t work_num { 0 };

        event::event_t work_event { };
        std::mutex work_lock;
    } // namespace

    void worker_caller(queue_type *queue)
    {
        while (true)
        {
            while (queue->empty())
                proc::yield();

            work_lock.lock();
            auto [worker, handle] = queue->pop_front_element();
            work_lock.unlock();

            worker(handle);

            work_num.fetch_sub(1, std::memory_order_acq_rel);
            work_event.trigger(true);
        }
    }

    void init_workers()
    {
        proc::enqueue(new proc::thread(kernel_proc, worker_caller, &gpe_work_queue, 0));
        proc::enqueue(new proc::thread(kernel_proc, worker_caller, &notify_work_queue));
    }

    using event_type = event::simple::event_t;
} // namespace uacpi

extern "C"
{
#if CAN_LEGACY_IO
    uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value)
    {
        uint16_t port = address;
        switch (byte_width)
        {
            case 1:
                *out_value = io::in<uint8_t>(port);
                break;
            case 2:
                *out_value = io::in<uint16_t>(port);
                break;
            case 4:
                *out_value = io::in<uint32_t>(port);
                break;
            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
        }
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr address, uacpi_u8 byte_width, uacpi_u64 in_value)
    {
        uint16_t port = address;
        switch (byte_width)
        {
            case 1:
                io::out<uint8_t>(port, in_value);
                break;
            case 2:
                io::out<uint16_t>(port, in_value);
                break;
            case 4:
                io::out<uint32_t>(port, in_value);
                break;
            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
        }
        return UACPI_STATUS_OK;
    }
#else
    uacpi_status uacpi_kernel_raw_io_read(uacpi_io_addr, uacpi_u8, uacpi_u64 *)
    {
        return UACPI_STATUS_UNIMPLEMENTED;
    }

    uacpi_status uacpi_kernel_raw_io_write(uacpi_io_addr, uacpi_u8, uacpi_u64)
    {
        return UACPI_STATUS_UNIMPLEMENTED;
    }
#endif

    uacpi_status uacpi_kernel_raw_memory_read(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 *out_value)
    {
        auto *ptr = uacpi_kernel_map(address, byte_width);
        switch (byte_width)
        {
            case 1:
                *out_value = mmio::in<uint8_t>(ptr);
                break;
            case 2:
                *out_value = mmio::in<uint16_t>(ptr);
                break;
            case 4:
                *out_value = mmio::in<uint32_t>(ptr);
                break;
            case 8:
                *out_value = mmio::in<uint64_t>(ptr);
                break;
            default:
                uacpi_kernel_unmap(ptr, byte_width);
                return UACPI_STATUS_INVALID_ARGUMENT;
        }

        uacpi_kernel_unmap(ptr, byte_width);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_raw_memory_write(uacpi_phys_addr address, uacpi_u8 byte_width, uacpi_u64 in_value)
    {
        auto *ptr = uacpi_kernel_map(address, byte_width);
        switch (byte_width)
        {
            case 1:
                mmio::out<uint8_t>(ptr, in_value);
                break;
            case 2:
                mmio::out<uint16_t>(ptr, in_value);
                break;
            case 4:
                mmio::out<uint32_t>(ptr, in_value);
                break;
            case 8:
                mmio::out<uint64_t>(ptr, in_value);
                break;
            default:
                uacpi_kernel_unmap(ptr, byte_width);
                return UACPI_STATUS_INVALID_ARGUMENT;
        }

        uacpi_kernel_unmap(ptr, byte_width);
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_read(uacpi_pci_address *address, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 *value)
    {
        switch (byte_width)
        {
            case 1:
                *value = pci::read<uint8_t>(address->segment, address->bus, address->device, address->function, offset);
                break;
            case 2:
                *value = pci::read<uint16_t>(address->segment, address->bus, address->device, address->function, offset);
                break;
            case 4:
                *value = pci::read<uint32_t>(address->segment, address->bus, address->device, address->function, offset);
                break;
            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
        }
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_pci_write( uacpi_pci_address *address, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 value)
    {
        switch (byte_width)
        {
            case 1:
                pci::write<uint8_t>(address->segment, address->bus, address->device, address->function, offset, value);
                break;
            case 2:
                pci::write<uint16_t>(address->segment, address->bus, address->device, address->function, offset, value);
                break;
            case 4:
                pci::write<uint32_t>(address->segment, address->bus, address->device, address->function, offset, value);
                break;
            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
        }
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle)
    {
        *out_handle = reinterpret_cast<uacpi_handle>(base);
        return UACPI_STATUS_OK;
    }
    void uacpi_kernel_io_unmap(uacpi_handle) { }

    uacpi_status uacpi_kernel_io_read(uacpi_handle handle, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 *value)
    {
        auto addr = reinterpret_cast<uacpi_io_addr>(handle);
        return uacpi_kernel_raw_io_read(addr + offset, byte_width, value);
    }

    uacpi_status uacpi_kernel_io_write(uacpi_handle handle, uacpi_size offset, uacpi_u8 byte_width, uacpi_u64 value)
    {
        auto addr = reinterpret_cast<uacpi_io_addr>(handle);
        return uacpi_kernel_raw_io_write(addr + offset, byte_width, value);
    }

    void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
    {
        return reinterpret_cast<uint8_t *>(tohh(addr));

        // TODO: FIXME

        // auto &pmap = vmm::kernel_pagemap;
        // auto [psize, flags] = pmap->required_size(len);

        // auto paddr = align_down(addr, psize);
        // auto size = align_up((addr - paddr) + len, psize);

        // auto vaddr = vmm::alloc_vspace(vmm::vsptypes::uacpi, size, psize);
        // assert(pmap->map_range(vaddr, paddr, size, vmm::rw | flags));

        // return reinterpret_cast<uint8_t *>(vaddr) + (addr - paddr);
    }

    void uacpi_kernel_unmap(void *ptr, uacpi_size len)
    {
        // auto addr = reinterpret_cast<uintptr_t>(ptr);

        // auto &pmap = vmm::kernel_pagemap;
        // auto psize = pmap->get_psize();

        // auto vaddr = align_down(addr, psize);
        // auto size = align_up((addr - vaddr) + len, psize);

        // assert(pmap->unmap_range(vaddr, size));
    }

    void *uacpi_kernel_alloc(uacpi_size size)
    {
        return malloc(size);
    }

    void *uacpi_kernel_calloc(uacpi_size count, uacpi_size size)
    {
        return calloc(count, size);
    }

    void uacpi_kernel_free(void *mem)
    {
        free(mem);
    }

    void uacpi_kernel_log(enum uacpi_log_level lvl, const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);

        uacpi_kernel_vlog(lvl, fmt, va);

        va_end(va);
    }

    void uacpi_kernel_vlog(enum uacpi_log_level lvl, const char *fmt, uacpi_va_list va)
    {
        std::string_view prefix;
        switch (lvl)
        {
            case UACPI_LOG_TRACE:
                prefix = "[\033[90mTRACE\033[0m] ";
                break;
            case UACPI_LOG_INFO:
                prefix = log::info_prefix;
                break;
            case UACPI_LOG_WARN:
                prefix = log::warn_prefix;
                break;
            case UACPI_LOG_ERROR:
                prefix = log::error_prefix;
                break;
            default:
                prefix = "";
        }

        va_list vacp;
        va_copy(vacp, va);

        size_t size = vsnprintf(nullptr, 0, fmt, vacp);

        va_end(vacp);

        auto buf = new char[size + 1];
        vsnprintf(buf, size + 1, fmt, va);

        if (buf[size - 1] == '\n')
            size--;

        log::println("{}UACPI: {}", prefix, std::string_view { buf, size });

        delete[] buf;
    }

    uacpi_u64 uacpi_kernel_get_ticks()
    {
        return time::time_ns() / 100;
    }

    void uacpi_kernel_stall(uacpi_u8 usec)
    {
        time::nsleep(usec * 1000);
    }

    void uacpi_kernel_sleep(uacpi_u64 msec)
    {
        // TODO: proper sleep
        time::msleep(msec);
    }

    uacpi_handle uacpi_kernel_create_mutex()
    {
        return reinterpret_cast<uacpi_handle>(new std::mutex);
    }

    void uacpi_kernel_free_mutex(uacpi_handle handle)
    {
        delete reinterpret_cast<std::mutex *>(handle);
    }

    uacpi_bool uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 mtimeout)
    {
        auto mut = reinterpret_cast<std::mutex *>(handle);
        if (mtimeout == 0xFFFF)
        {
            mut->lock();
            return true;
        }

        return mut->try_lock_until(mtimeout * 1'000'000);
    }

    void uacpi_kernel_release_mutex(uacpi_handle handle)
    {
        reinterpret_cast<std::mutex *>(handle)->unlock();
    }

    uacpi_handle uacpi_kernel_create_spinlock()
    {
        return uacpi_kernel_create_mutex();
    }

    void uacpi_kernel_free_spinlock(uacpi_handle handle)
    {
        uacpi_kernel_free_mutex(handle);
    }

    uacpi_cpu_flags uacpi_kernel_spinlock_lock(uacpi_handle handle)
    {
        reinterpret_cast<std::mutex *>(handle)->lock();
        auto saved_ints = arch::int_status();
        arch::int_toggle(false);
        return static_cast<uacpi_cpu_flags>(saved_ints);
    }

    void uacpi_kernel_spinlock_unlock(uacpi_handle handle, uacpi_cpu_flags flags)
    {
        bool saved_ints = flags;
        arch::int_toggle(saved_ints);
        reinterpret_cast<std::mutex *>(handle)->unlock();
    }

    uacpi_handle uacpi_kernel_create_event()
    {
        return reinterpret_cast<void *>(new uacpi::event_type);
    }

    void uacpi_kernel_free_event(uacpi_handle handle)
    {
        delete reinterpret_cast<uacpi::event_type *>(handle);
    }

    uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 mtimeout)
    {
        auto event = reinterpret_cast<uacpi::event_type *>(handle);
        if (mtimeout == 0xFFFF)
            event->await();
        else
            return event->await_timeout(mtimeout);

        return true;
    }

    void uacpi_kernel_signal_event(uacpi_handle handle)
    {
        auto event = reinterpret_cast<uacpi::event_type *>(handle);
        event->trigger();
    }

    void uacpi_kernel_reset_event(uacpi_handle handle)
    {
        auto event = reinterpret_cast<uacpi::event_type *>(handle);
        if (event->drop())
        {
            while (event->has_awaiters())
                event->trigger();
        }
    }

    uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *request)
    {
        switch (request->type)
        {
            case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
                log::warnln("UACPI: Ignoring AML breakpoint");
                break;
            case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
                log::errorln("UACPI: Fatal firmware error: type: {}, code: {}, arg: {}", request->fatal.type, request->fatal.code, request->fatal.arg);
                break;
        }
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_install_interrupt_handler(uacpi_u32 irq, uacpi_interrupt_handler uhandler, uacpi_handle ctx, uacpi_handle *out_irq_handle)
    {
        auto [handler, vector] = interrupts::allocate_handler(irq);
        handler.set([uhandler, ctx](auto) { uhandler(ctx); });

        interrupts::unmask(vector);

        *reinterpret_cast<size_t*>(out_irq_handle) = vector;
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler, uacpi_handle irq_handle)
    {
        auto vector = *reinterpret_cast<size_t*>(irq_handle);
        interrupts::mask(vector);

        auto handler = interrupts::get_handler(vector);
        handler.reset();

        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx)
    {
        std::unique_lock lock(uacpi::work_lock);
        switch (type)
        {
            case UACPI_WORK_GPE_EXECUTION:
                uacpi::gpe_work_queue.emplace_back(handler, ctx);
                uacpi::work_num.fetch_add(1, std::memory_order_acq_rel);
                break;
            case UACPI_WORK_NOTIFICATION:
                uacpi::notify_work_queue.emplace_back(handler, ctx);
                uacpi::work_num.fetch_add(1, std::memory_order_acq_rel);
                break;
            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
        }
        return UACPI_STATUS_OK;
    }

    uacpi_status uacpi_kernel_wait_for_work_completion()
    {
        while (uacpi::work_num.load(std::memory_order_acquire))
            uacpi::work_event.await();

        return UACPI_STATUS_OK;
    }
} // extern "C"
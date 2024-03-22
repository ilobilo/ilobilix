// Copyright (C) 2022-2024  ilobilo

#include <drivers/pci/pci.hpp>
#include <drivers/acpi.hpp>

#include <lib/panic.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>
#include <lib/io.hpp>

#include <lai/host.h>

extern "C"
{
#if CAN_LEGACY_IO
    void laihost_outb(uint16_t port, uint8_t val)
    {
        io::out<uint8_t>(port, val);
    }

    void laihost_outw(uint16_t port, uint16_t val)
    {
        io::out<uint16_t>(port, val);
    }

    void laihost_outd(uint16_t port, uint32_t val)
    {
        io::out<uint32_t>(port, val);
    }

    uint8_t laihost_inb(uint16_t port)
    {
        return io::in<uint8_t>(port);
    }

    uint16_t laihost_inw(uint16_t port)
    {
        return io::in<uint16_t>(port);
    }

    uint32_t laihost_ind(uint16_t port)
    {
        return io::in<uint32_t>(port);
    }
#endif


    void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint8_t val)
    {
        pci::write<uint8_t>(seg, bus, slot, fun, offset, val);
    }

    void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint16_t val)
    {
        pci::write<uint16_t>(seg, bus, slot, fun, offset, val);
    }

    void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset, uint32_t val)
    {
        pci::write<uint32_t>(seg, bus, slot, fun, offset, val);
    }

    uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
    {
        return pci::read<uint8_t>(seg, bus, slot, fun, offset);
    }

    uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
    {
        return pci::read<uint16_t>(seg, bus, slot, fun, offset);
    }

    uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t fun, uint16_t offset)
    {
        return pci::read<uint32_t>(seg, bus, slot, fun, offset);
    }

    void *laihost_scan(const char *signature, size_t index)
    {
        return acpi::findtable(signature, index);
    }

    void laihost_sleep(uint64_t ms)
    {
        time::msleep(ms);
    }

    uint64_t laihost_timer()
    {
        return time::time_ns() / 100;
    }

    void *laihost_map(size_t address, size_t)
    {
        return reinterpret_cast<void*>(tohh(address));
    }

    void laihost_unmap(void *, size_t) { }

    void *laihost_malloc(size_t size)
    {
        return malloc(size);
    }

    void *laihost_realloc(void *ptr, size_t size, size_t)
    {
        return realloc(ptr, size);
    }

    void laihost_free(void *ptr, size_t)
    {
        free(ptr);
    }

    void laihost_log(int level, const char *msg)
    {
        switch (level)
        {
            case LAI_DEBUG_LOG:
                log::infoln("LAI: {}", msg);
                break;
            case LAI_WARN_LOG:
                log::warnln("LAI: {}", msg);
                break;
        }
    }

    __attribute__((noreturn))
    void laihost_panic(const char *msg)
    {
        panic(msg);
    }
} // extern "C"
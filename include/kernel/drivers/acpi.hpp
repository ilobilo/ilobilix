// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <cstdint>
#include <vector>

namespace acpi
{
    struct [[gnu::packed]] RSDP
    {
        uint8_t signature[8];
        uint8_t chksum;
        uint8_t oemid[6];
        uint8_t revision;
        uint32_t rsdtaddr;
        uint32_t length;
        uint64_t xsdtaddr;
        uint8_t extchksum;
        uint8_t reserved[3];
    };

    struct [[gnu::packed]] SDTHeader
    {
        uint8_t signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t chksum;
        uint8_t oemid[6];
        uint8_t oemtableid[8];
        uint32_t oemrevision;
        uint32_t creatid;
        uint32_t creatrevision;
    };

    struct [[gnu::packed]] RSDT
    {
        SDTHeader header;
        union
        {
            uint32_t tables[];
            uint64_t tablesx[];
        };
    };

    struct [[gnu::packed]] MCFGEntry
    {
        uint64_t baseaddr;
        uint16_t segment;
        uint8_t startbus;
        uint8_t endbus;
        uint32_t reserved;
    };

    struct [[gnu::packed]] MCFGHeader
    {
        SDTHeader header;
        uint64_t reserved;
        MCFGEntry entries[];
    };

    struct [[gnu::packed]] MADTHeader
    {
        SDTHeader sdt;
        uint32_t lapic_addr;
        uint32_t flags;
        char entries[];

        bool legacy_pic()
        {
            return this->flags & 0x01;
        }
    };

    struct [[gnu::packed]] MADT
    {
        uint8_t type;
        uint8_t length;
    };

    struct [[gnu::packed]] MADTLapic // 0
    {
        MADT madtHeader;
        uint8_t processor_id;
        uint8_t apic_id;
        uint32_t flags;
    };

    struct [[gnu::packed]] MADTIOApic // 1
    {
        MADT madtHeader;
        uint8_t apic_id;
        uint8_t reserved;
        uint32_t addr;
        uint32_t gsib;
    };

    struct [[gnu::packed]] MADTIso // 2
    {
        MADT madtHeader;
        uint8_t bus_source;
        uint8_t irq_source;
        uint32_t gsi;
        uint16_t flags;
    };

    struct [[gnu::packed]] MADTIONmi // 3
    {
        MADT madtHeader;
        uint8_t source;
        uint8_t reserved;
        uint16_t flags;
        uint32_t gsi;
    };

    struct [[gnu::packed]] MADTLNmi // 4
    {
        MADT madtHeader;
        uint8_t processor;
        uint16_t flags;
        uint8_t lint;
    };

    struct [[gnu::packed]] MADTLAPICAO // 5
    {
        MADT madtHeader;
        uint16_t reserved;
        uint64_t lapic_addr;
    };

    struct [[gnu::packed]] MADTLx2APIC // 9
    {
        MADT madtHeader;
        uint16_t reserved;
        uint32_t x2apicid;
        uint32_t flags;
        uint32_t acpi_id;
    };

    struct [[gnu::packed]] GenericAddressStructure
    {
        uint8_t AddressSpace;
        uint8_t BitWidth;
        uint8_t BitOffset;
        uint8_t AccessSize;
        uint64_t Address;
    };

    struct [[gnu::packed]] HPETHeader
    {
        SDTHeader header;
        uint8_t hardware_rev_id;
        uint8_t comparator_count : 5;
        uint8_t counter_size : 1;
        uint8_t reserved : 1;
        uint8_t legacy_replacement : 1;
        uint16_t pci_vendor_id;
        GenericAddressStructure address;
        uint8_t hpet_number;
        uint16_t minimum_tick;
        uint8_t page_protection;
    };

    struct [[gnu::packed]] FADTHeader
    {
        SDTHeader header;
        uint32_t FirmwareCtrl;
        uint32_t Dsdt;
        uint8_t Reserved;
        uint8_t PreferredPowerManagementProfile;
        uint16_t SCI_Interrupt;
        uint32_t SMI_CommandPort;
        uint8_t AcpiEnable;
        uint8_t AcpiDisable;
        uint8_t S4BIOS_REQ;
        uint8_t PSTATE_Control;
        uint32_t PM1aEventBlock;
        uint32_t PM1bEventBlock;
        uint32_t PM1aControlBlock;
        uint32_t PM1bControlBlock;
        uint32_t PM2ControlBlock;
        uint32_t PMTimerBlock;
        uint32_t GPE0Block;
        uint32_t GPE1Block;
        uint8_t PM1EventLength;
        uint8_t PM1ControlLength;
        uint8_t PM2ControlLength;
        uint8_t PMTimerLength;
        uint8_t GPE0Length;
        uint8_t GPE1Length;
        uint8_t GPE1Base;
        uint8_t CStateControl;
        uint16_t WorstC2Latency;
        uint16_t WorstC3Latency;
        uint16_t FlushSize;
        uint16_t FlushStride;
        uint8_t DutyOffset;
        uint8_t DutyWidth;
        uint8_t DayAlarm;
        uint8_t MonthAlarm;
        uint8_t Century;
        uint16_t BootArchitectureFlags;
        uint8_t Reserved2;
        uint32_t Flags;
        GenericAddressStructure ResetReg;
        uint8_t ResetValue;
        uint16_t ArmBootArchitectureFlags;
        uint8_t MinorVersion;
        uint64_t X_FirmwareControl;
        uint64_t X_Dsdt;
        GenericAddressStructure X_PM1aEventBlock;
        GenericAddressStructure X_PM1bEventBlock;
        GenericAddressStructure X_PM1aControlBlock;
        GenericAddressStructure X_PM1bControlBlock;
        GenericAddressStructure X_PM2ControlBlock;
        GenericAddressStructure X_PMTimerBlock;
        GenericAddressStructure X_GPE0Block;
        GenericAddressStructure X_GPE1Block;
    };

    struct [[gnu::packed]] DMARHeader
    {
        SDTHeader header;
        uint8_t host_address_width;
        uint8_t flags;
        uint8_t reserved[10];
        uint8_t remapping_structures[];
    };

    extern FADTHeader *fadthdr;
    extern MADTHeader *madthdr;

    extern std::vector<MADTLapic> lapics;
    extern std::vector<MADTIOApic> ioapics;
    extern std::vector<MADTIso> isos;
    extern std::vector<MADTIONmi> ionmis;
    extern std::vector<MADTLNmi> lnmis;
    extern std::vector<MADTLAPICAO> laddrovers;
    extern std::vector<MADTLx2APIC> x2apics;

    extern RSDP *rsdp;
    extern RSDT *rsdt;
    extern bool xsdt;

    void poweroff();
    void reboot();

    void enable();
    void init();

    void *findtable(const char *signature, size_t skip);

    template<typename Type>
    inline Type *findtable(const char *signature, size_t skip)
    {
        return reinterpret_cast<Type*>(findtable(signature, skip));
    }
} // namespace acpi
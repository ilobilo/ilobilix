// Copyright (C) 2022-2024  ilobilo

#include <drivers/acpi.hpp>
#include <init/kernel.hpp>

#include <arch/arch.hpp>

#include <lib/alloc.hpp>
#include <lib/panic.hpp>
#include <lib/time.hpp>
#include <lib/log.hpp>

#include <cpu/cpu.hpp>

#include <mm/pmm.hpp>
#include <mm/vmm.hpp>

#include <lib/glue/uacpi.hpp>

#include <uacpi/utilities.h>
#include <uacpi/notify.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>

#if defined(__x86_64__)
#  include <arch/x86_64/cpu/ioapic.hpp>
#  include <arch/x86_64/cpu/idt.hpp>
#  include <arch/x86_64/lib/io.hpp>
#endif

namespace acpi
{
    acpi_fadt *get_fadt()
    {
        static acpi_fadt *fadt = nullptr;
        static bool first = true;

        if (first)
        {
            if (uacpi_table_fadt(&fadt) == UACPI_STATUS_OK)
                first = false;
        }

        return fadt;
    }

    namespace madt
    {
        acpi_madt *hdr = nullptr;

        std::vector<acpi_madt_ioapic> ioapics;
        std::vector<acpi_madt_interrupt_source_override> isos;

        void init()
        {
            uacpi_table out_table;
            if (uacpi_table_find_by_signature("APIC", &out_table) != UACPI_STATUS_OK)
                return;

            hdr = reinterpret_cast<acpi_madt *>(out_table.virt_addr);

            auto start = reinterpret_cast<uintptr_t>(hdr->entries);
            auto end = reinterpret_cast<uintptr_t>(hdr) + hdr->hdr.length;

            auto madt = reinterpret_cast<acpi_entry_hdr *>(start);

            for (uintptr_t entry = start; entry < end; entry += madt->length, madt = reinterpret_cast<acpi_entry_hdr *>(entry))
            {
                switch (madt->type)
                {
                    case 1:
                        ioapics.push_back(*reinterpret_cast<acpi_madt_ioapic*>(entry));
                        break;
                    case 2:
                        isos.push_back(*reinterpret_cast<acpi_madt_interrupt_source_override*>(entry));
                        break;
                }
            }
        }
    } // namespace madt

    void ec_init()
    {
        // TODO
        // log::infoln("ACPI: Initialising ECs...");
    }

    void poweroff()
    {
        log::infoln("ACPI: Preparing for sleep state 5...");

        auto ret = uacpi_prepare_for_sleep_state(UACPI_SLEEP_STATE_S5);
        assert(ret == UACPI_STATUS_OK);

        log::infoln("ACPI: Disabling interrupts and entering sleep state 5...");

        arch::int_toggle(false);

        ret = uacpi_enter_sleep_state(UACPI_SLEEP_STATE_S5);
        assert(ret == UACPI_STATUS_OK);
    }

    void reboot()
    {
        auto ret = uacpi_reboot();
        if (uacpi_unlikely_error(ret))
            log::errorln("ACPI: Reset failed: {}", uacpi_status_to_string(ret));
    }

    // called from arch/*/drivers/pci/pci.cpp
    void enable()
    {
        log::infoln("ACPI: Enabling...");

        uacpi::init_workers();

        auto ret = uacpi_namespace_load();
        assert(ret == UACPI_STATUS_OK);

        ret = uacpi_set_interrupt_model(UACPI_INTERRUPT_MODEL_IOAPIC);
        assert(ret == UACPI_STATUS_OK);

        ec_init();

        ret = uacpi_namespace_initialize();
        assert(ret == UACPI_STATUS_OK);

        uacpi_finalize_gpe_initialization();

        uacpi_install_fixed_event_handler(
            UACPI_FIXED_EVENT_POWER_BUTTON,
            [](uacpi_handle) -> uacpi_interrupt_ret
            {
                uacpi_kernel_schedule_work(UACPI_WORK_GPE_EXECUTION, [](uacpi_handle) { arch::shutdown(); }, nullptr);
                return UACPI_INTERRUPT_HANDLED;
            }, nullptr
        );

        uacpi_find_devices("PNP0C0C", [](void *, uacpi_namespace_node *node)
        {
            uacpi_install_notify_handler(node, [](uacpi_handle, uacpi_namespace_node *, uacpi_u64 value) -> uacpi_status
                {
                    // 0x80: S0 Power Button Pressed
                    if (value != 0x80)
                        return UACPI_STATUS_OK;

                    arch::shutdown();

                    return UACPI_STATUS_OK;
                }, nullptr
            );
            return UACPI_NS_ITERATION_DECISION_CONTINUE;
        }, nullptr);
    }

    void init()
    {
        log::infoln("ACPI: Initialising...");

        uacpi_init_params params {
            .rsdp = reinterpret_cast<uacpi_phys_addr>(fromhh(rsdp_request.response->address)),
            .log_level = UACPI_LOG_INFO,
            .flags = 0
        };
        assert(uacpi_initialize(&params) == UACPI_STATUS_OK);

        madt::init();
    }
} // namespace acpi
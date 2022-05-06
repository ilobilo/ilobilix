// Copyright (C) 2024  ilobilo

module;

#include <limine.h>

export module boot;
import lib;
import std;

namespace
{
    [[gnu::used, gnu::section(".requests_start_marker")]]
    volatile LIMINE_REQUESTS_START_MARKER;

    [[gnu::used, gnu::section(".requests_end_marker")]]
    volatile LIMINE_REQUESTS_END_MARKER;
} // namespace

export namespace boot
{
    // constexpr std::uintptr_t kernel_stack_size = 0x10000; // 64 kib
    constexpr std::uintptr_t kernel_stack_size = 0x4000; // 16 kib
    constexpr std::uintptr_t user_stack_size = 0x200000; // 2 mib

    using limine_smp_info = ::limine_smp_info;

    enum class memmap : std::uint64_t
    {
        usable = LIMINE_MEMMAP_USABLE,
        reserved = LIMINE_MEMMAP_RESERVED,
        acpi_reclaimable = LIMINE_MEMMAP_ACPI_RECLAIMABLE,
        acpi_nvs = LIMINE_MEMMAP_ACPI_NVS,
        bad_memory = LIMINE_MEMMAP_BAD_MEMORY,
        bootloader_reclaimable = LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE,
        kernel_and_modules = LIMINE_MEMMAP_KERNEL_AND_MODULES,
        framebuffer = LIMINE_MEMMAP_FRAMEBUFFER
    };

    [[gnu::used, gnu::section(".requests")]]
    volatile LIMINE_BASE_REVISION(3)

    namespace requests
    {
#if defined(__aarch64__)
        [[gnu::used, gnu::section(".requests")]]
        volatile limine_dtb_request dtb
        {
            .id = LIMINE_DTB_REQUEST,
            .revision = 0,
            .response = nullptr
        };
#endif

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_framebuffer_request framebuffer
        {
            .id = LIMINE_FRAMEBUFFER_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_smp_request smp
        {
            .id = LIMINE_SMP_REQUEST,
            .revision = 0,
            .response = nullptr,
#if defined(__x86_64__)
            .flags = LIMINE_SMP_X2APIC
#else
            .flags = 0
#endif
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_paging_mode_request paging_mode
        {
            .id = LIMINE_PAGING_MODE_REQUEST,
            .revision = 0,
            .response = nullptr,
#if LVL5_PAGING
            .mode = LIMINE_PAGING_MODE_MAX,
#else
            .mode = LIMINE_PAGING_MODE_DEFAULT,
#endif
            .max_mode = LIMINE_PAGING_MODE_DEFAULT,
            .min_mode = LIMINE_PAGING_MODE_DEFAULT
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_memmap_request memmap
        {
            .id = LIMINE_MEMMAP_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_rsdp_request rsdp
        {
            .id = LIMINE_RSDP_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_module_request module_
        {
            .id = LIMINE_MODULE_REQUEST,
            .revision = 0,
            .response = nullptr,
            .internal_module_count = 0,
            .internal_modules = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_kernel_file_request kernel_file
        {
            .id = LIMINE_KERNEL_FILE_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_boot_time_request boot_time
        {
            .id = LIMINE_BOOT_TIME_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_hhdm_request hhdm
        {
            .id = LIMINE_HHDM_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_kernel_address_request kernel_address
        {
            .id = LIMINE_KERNEL_ADDRESS_REQUEST,
            .revision = 0,
            .response = nullptr
        };

        [[gnu::used, gnu::section(".requests")]]
        volatile limine_stack_size_request stack_size
        {
            .id = LIMINE_STACK_SIZE_REQUEST,
            .revision = 0,
            .response = nullptr,
            .stack_size = kernel_stack_size
        };
    } // namespace requests

    limine_file *find_module(std::string_view name)
    {
        auto mods = requests::module_.response;
        for (std::size_t i = 0; i < mods->module_count; i++)
        {
            if (mods->modules[i]->cmdline == name)
                return mods->modules[i];
        }
        return nullptr;
    }

    std::uintptr_t get_hhdm_offset()
    {
        static const auto cached = [] { return requests::hhdm.response->offset; } ();
        return cached;
    }


    std::int64_t time()
    {
        static const auto cached = [] { return requests::boot_time.response->boot_time; } ();
        return cached;
    }

    void check_requests()
    {
        if (!LIMINE_BASE_REVISION_SUPPORTED)
            lib::panic("Limine base revision not supported");

        if (requests::memmap.response == nullptr)
            lib::panic("Could not get a response to the memmap request");
        if (requests::paging_mode.response == nullptr)
            lib::panic("Could not get a response to the paging mode request");
        if (requests::hhdm.response == nullptr)
            lib::panic("Could not get a response to the hhdm request");
        if (requests::kernel_file.response == nullptr)
            lib::panic("Could not get a response to the kernel file request");
        if (requests::kernel_address.response == nullptr)
            lib::panic("Could not get a response to the kernel address request");
        if (requests::framebuffer.response == nullptr)
            lib::panic("Could not get a response to the framebuffer request");
        if (requests::smp.response == nullptr)
            lib::panic("Could not get a response to the smp request");
        if (requests::rsdp.response == nullptr)
            lib::panic("Could not get a response to the rsdp request");
        if (requests::module_.response == nullptr)
            lib::panic("Could not get a response to the module request");
        if (requests::boot_time.response == nullptr)
            lib::panic("Could not get a response to the boot time request");

// #if defined(__aarch64__)
//         if (requests::dtb.response == nullptr)
//             lib::panic("Could not get a response to the dtb request");
// #endif
    }
} // export namespace boot

// used in math.cppm
extern "C++" std::uintptr_t (*get_hhdm_offset)() = &boot::get_hhdm_offset;
// Copyright (C) 2022-2023  ilobilo

#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <drivers/smp.hpp>

#include <init/kernel.hpp>

#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <string.h>

const char *cmdline = nullptr;
uintptr_t hhdm_offset = 0;
uint64_t paging_mode = 0;
bool uefi = false;

LIMINE_BASE_REVISION(1)

// #if defined(__aarch64__)
// volatile limine_dtb_request dtb_request
// {
//     .id = LIMINE_DTB_REQUEST,
//     .revision = 0,
//     .response = nullptr
// };
// #endif

volatile limine_efi_system_table_request efi_system_table_request
{
    .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_framebuffer_request framebuffer_request
{
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_smp_request smp_request
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

volatile limine_paging_mode_request paging_mode_request
{
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = nullptr,
#if LVL5_PAGING
    .mode = LIMINE_PAGING_MODE_MAX,
#else
    .mode = LIMINE_PAGING_MODE_DEFAULT,
#endif
    .flags = 0
};

volatile limine_memmap_request memmap_request
{
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_rsdp_request rsdp_request
{
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_module_request module_request
{
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_kernel_file_request kernel_file_request
{
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_boot_time_request boot_time_request
{
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_hhdm_request hhdm_request
{
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_kernel_address_request kernel_address_request
{
    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0,
    .response = nullptr
};

volatile limine_stack_size_request stack_size_request
{
    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .stack_size = kernel_stack_size
};

limine_file *find_module(const char *name)
{
    for (size_t i = 0; i < module_request.response->module_count; i++)
    {
        if (!strcmp(module_request.response->modules[i]->cmdline, name))
            return module_request.response->modules[i];
    }
    return nullptr;
}

extern "C" void _start()
{
    serial::early_init();

// #if defined(__aarch64__)
//     assert(dtb_request.response, "Could not get dtb response!");
// #endif

    uefi = efi_system_table_request.response != nullptr;

    assert(LIMINE_BASE_REVISION_SUPPORTED, "Limine base revision not supported!");
    assert(framebuffer_request.response, "Could not get framebuffer response!");
    assert(smp_request.response, "Could not get smp response!");
    assert(paging_mode_request.response, "Could not get paging mode response!");
    assert(memmap_request.response, "Could not get memmap response!");
    assert(rsdp_request.response, "Could not get rsdp response!");
    assert(module_request.response, "Could not get module response!");
    assert(kernel_file_request.response, "Could not get kernel file response!");
    assert(boot_time_request.response, "Could not get boot time response!");
    assert(hhdm_request.response, "Could not get hhdm response!");
    assert(kernel_address_request.response, "Could not get kernel address response!");
    assert(stack_size_request.response, "Could not get stack size response!");

    cmdline = kernel_file_request.response->kernel_file->cmdline;
    hhdm_offset = hhdm_request.response->offset;
    paging_mode = paging_mode_request.mode;

    kmain();
    arch::halt();
}
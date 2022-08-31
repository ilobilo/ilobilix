// Copyright (C) 2022  ilobilo


#include <drivers/serial.hpp>
#include <drivers/term.hpp>
#include <drivers/smp.hpp>

#include <kernel/kernel.hpp>
#include <kernel/main.hpp>

#include <arch/arch.hpp>
#include <lib/panic.hpp>
#include <string.h>

const char *cmdline = nullptr;
uint64_t hhdm_offset = 0;
bool lvl5 = LVL5_PAGING != 0;
bool uefi = true;

#if LVL5_PAGING
volatile limine_5_level_paging_request _5_level_paging_request
{
    .id = LIMINE_5_LEVEL_PAGING_REQUEST,
    .revision = 0,
    .response = nullptr
};
#endif

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

volatile limine_terminal_request terminal_request
{
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0,
    .response = nullptr,
    .callback = nullptr
};

volatile limine_smp_request smp_request
{
    .id = LIMINE_SMP_REQUEST,
    .revision = 0,
    .response = nullptr,
    .flags = (1 << 0)
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
    .stack_size = default_stack_size
};

limine_file *find_module(const char *name)
{
    for (size_t i = 0; i < module_request.response->module_count; i++)
    {
        if (!strcmp(module_request.response->modules[i]->cmdline, name))
        {
            return module_request.response->modules[i];
        }
    }
    return nullptr;
}

extern "C" void _start()
{
    serial::early_init();
    term::early_init();

    uefi = efi_system_table_request.response != nullptr;
#if LVL5_PAGING
    lvl5 = _5_level_paging_request.response != nullptr;
#endif

    #if !defined(__x86_64__)
    assert(framebuffer_request.response, "Could not get framebuffer response!");
    #endif
    assert(smp_request.response, "Could not get smp response!");
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

    main();
    arch::halt();
}
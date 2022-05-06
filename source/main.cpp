// Copyright (C) 2022  ilobilo

#include <drivers/serial/serial.hpp>
#include <drivers/term/term.hpp>
#include <drivers/frm/frm.hpp>
#include <cpu/gdt/gdt.hpp>
#include <cpu/idt/idt.hpp>
#include <mm/pmm/pmm.hpp>
#include <mm/vmm/vmm.hpp>
#include <lib/string.hpp>
#include <lib/panic.hpp>
#include <main.hpp>
#include <cstddef>

uint8_t *kernel_stack;
const char *cmdline;
uint64_t hhdm_offset;

#if LVL5_PAGING
volatile limine_5_level_paging_request _5_level_paging_request
{
    .id = LIMINE_5_LEVEL_PAGING_REQUEST,
    .revision = 0,
    .response = nullptr
};
#endif

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
    .stack_size = STACK_SIZE
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

using constructor_t = void (*)();

extern "C" constructor_t __init_array_start[];
extern "C" constructor_t __init_array_end[];
void constructors_init()
{
    for (constructor_t *ctor = __init_array_start; ctor < __init_array_end; ctor++) (*ctor)();
}

extern "C" void _start()
{
    asm volatile ("movq %%rsp, %0" : "=r"(kernel_stack));

    assert(framebuffer_request.response, "Could not get framebuffer response!");
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

    serial::init();
    mm::pmm::init();
    mm::vmm::init();

    frm::init();
    term::init();

    constructors_init();

    cpu::gdt::init();
    cpu::idt::init();

    printf("Hello, World!");

    while (true) asm volatile ("hlt");
}
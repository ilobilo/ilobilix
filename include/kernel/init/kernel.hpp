// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <limine.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
struct limine_term_request : limine_terminal_request
{
    limine_term_request() : limine_terminal_request { LIMINE_TERMINAL_REQUEST, 0, nullptr, nullptr } { }
};
struct limine_term : limine_terminal { };
#pragma clang diagnostic pop

static constexpr uintptr_t kernel_stack_size = 0x10000; // 64 kib
static constexpr uintptr_t user_stack_size = 0x200000; // 2 mib

extern const char *cmdline;
extern uintptr_t hhdm_offset;
extern bool lvl5;
extern bool uefi;

#if LVL5_PAGING
extern volatile limine_5_level_paging_request _5_level_paging_request;
#endif

// #if defined(__aarch64__)
// extern volatile limine_dtb_request dtb_request;
// #endif

extern volatile limine_efi_system_table_request efi_system_table_request;
extern volatile limine_framebuffer_request framebuffer_request;
extern volatile limine_term_request terminal_request;
extern volatile limine_smp_request smp_request;
extern volatile limine_memmap_request memmap_request;
extern volatile limine_rsdp_request rsdp_request;
extern volatile limine_module_request module_request;
extern volatile limine_kernel_file_request kernel_file_request;
extern volatile limine_boot_time_request boot_time_request;
extern volatile limine_hhdm_request hhdm_request;
extern volatile limine_kernel_address_request kernel_address_request;
extern volatile limine_stack_size_request stack_size_request;

limine_file *find_module(const char *name);

namespace proc { struct process; }
extern proc::process *kernel_proc;
void kmain();
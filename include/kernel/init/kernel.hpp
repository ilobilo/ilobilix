// Copyright (C) 2022  ilobilo

#pragma once

#include <limine.h>

static constexpr uintptr_t default_stack_size = 0x10000; // 64Kib

extern const char *cmdline;
extern uintptr_t hhdm_offset;
extern bool uefi;
extern bool lvl5;

#if LVL5_PAGING
extern volatile limine_5_level_paging_request _5_level_paging_request;
#endif

// #if defined(__aarch64__)
// extern volatile limine_dtb_request dtb_request;
// #endif

extern volatile limine_efi_system_table_request efi_system_table_request;
extern volatile limine_framebuffer_request framebuffer_request;
extern volatile limine_terminal_request terminal_request;
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
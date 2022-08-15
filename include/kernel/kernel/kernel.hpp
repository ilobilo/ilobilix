// Copyright (C) 2022  ilobilo

#pragma once

#include <limine.h>

static constexpr uint64_t STACK_SIZE = 0x4000;
extern const char *cmdline;
extern uint64_t hhdm_offset;
extern bool uefi;
extern bool lvl5;

#if LVL5_PAGING
extern volatile limine_5_level_paging_request _5_level_paging_request;
#endif
extern volatile limine_terminal_request terminal_request;
extern volatile limine_framebuffer_request framebuffer_request;
extern volatile limine_smp_request smp_request;
extern volatile limine_memmap_request memmap_request;
extern volatile limine_rsdp_request rsdp_request;
extern volatile limine_module_request module_request;
extern volatile limine_kernel_file_request kernel_file_request;
extern volatile limine_boot_time_request boot_time_request;
extern volatile limine_hhdm_request hhdm_request;
extern volatile limine_kernel_address_request kernel_address_request;

limine_file *find_module(const char *name);
// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <limine.h>

inline constexpr uintptr_t kernel_stack_size = 0x10000; // 64 kib
inline constexpr uintptr_t user_stack_size = 0x200000; // 2 mib

extern const char *cmdline;
extern uintptr_t hhdm_offset;
extern uint64_t paging_mode;

#define if_max_pgmode(then) (paging_mode == LIMINE_PAGING_MODE_MAX) ? (then)

// #if defined(__aarch64__)
// extern volatile limine_dtb_request dtb_request;
// #endif

extern volatile limine_framebuffer_request framebuffer_request;
extern volatile limine_smp_request smp_request;
extern volatile limine_paging_mode_request paging_mode_request;
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
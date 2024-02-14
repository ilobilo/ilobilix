// Copyright (C) 2022-2024  ilobilo

#include <arch/x86_64/cpu/idt.hpp>

#include <lib/syscall.hpp>
#include <frozen/map.h>

#include <arch/x86_64/syscall/arch.hpp>
#include <syscall/proc.hpp>
#include <syscall/vfs.hpp>
#include <syscall/mm.hpp>

namespace syscall
{
    // Intellisense says conversion from function to voidptr is invalid in constant-expression evaluation
#ifdef __INTELLISENSE__
    const auto map = frozen::make_map<size_t, wrapper>
#else
    constexpr auto map = frozen::make_map<size_t, wrapper>
#endif
    ({
#define SYSCALL_ENTRY(num, name, sys, ...) { num, wrapper(name, sys __VA_OPT__(,) __VA_ARGS__) }
        SYSCALL_ENTRY(0, "read", vfs::sys_read),
        SYSCALL_ENTRY(1, "write", vfs::sys_write),
        SYSCALL_ENTRY(2, "open", vfs::sys_open),
        SYSCALL_ENTRY(3, "close", vfs::sys_close),
        SYSCALL_ENTRY(4, "stat", vfs::sys_stat),
        SYSCALL_ENTRY(5, "fstat", vfs::sys_fstat),
        SYSCALL_ENTRY(6, "lstat", vfs::sys_lstat),
        SYSCALL_ENTRY(8, "lseek", vfs::sys_lseek),
        SYSCALL_ENTRY(9, "mmap", vmm::sys_mmap),
        SYSCALL_ENTRY(10, "mprotect", vmm::sys_mprotect),
        SYSCALL_ENTRY(11, "munmap", vmm::sys_munmap),
        SYSCALL_ENTRY(16, "ioctl", vfs::sys_ioctl),
        SYSCALL_ENTRY(32, "dup", vfs::sys_dup),
        SYSCALL_ENTRY(33, "dup2", vfs::sys_dup2),
        SYSCALL_ENTRY(39, "getpid", proc::sys_getpid),
        SYSCALL_ENTRY(56, "clone", proc::sys_clone),
        SYSCALL_ENTRY(57, "fork", proc::sys_fork),
        SYSCALL_ENTRY(59, "execve", proc::sys_execve),
        SYSCALL_ENTRY(60, "exit", proc::sys_exit),
        SYSCALL_ENTRY(61, "wait4", proc::sys_wait4),
        SYSCALL_ENTRY(63, "uname", proc::sys_uname),
        SYSCALL_ENTRY(72, "fcntl", vfs::sys_fcntl),
        SYSCALL_ENTRY(78, "getdents", vfs::sys_getdents),
        SYSCALL_ENTRY(79, "getcwd", vfs::sys_getcwd, [](uintptr_t val)
            { return reinterpret_cast<char*>(val) == nullptr; }),
        SYSCALL_ENTRY(80, "chdir", vfs::sys_chdir),
        SYSCALL_ENTRY(81, "fchdir", vfs::sys_fchdir),
        SYSCALL_ENTRY(83, "mkdir", vfs::sys_mkdir),
        SYSCALL_ENTRY(85, "creat", vfs::sys_creat),
        SYSCALL_ENTRY(86, "link", vfs::sys_link),
        SYSCALL_ENTRY(87, "unlink", vfs::sys_unlink),
        SYSCALL_ENTRY(89, "readlink", vfs::sys_readlink),
        SYSCALL_ENTRY(91, "fchmod", vfs::sys_fchmod),
        SYSCALL_ENTRY(95, "umask", proc::sys_umask),
        SYSCALL_ENTRY(110, "getppid", proc::sys_getppid),
        SYSCALL_ENTRY(133, "mknod", vfs::sys_mknod),
        SYSCALL_ENTRY(158, "arch_prctl", arch::sys_arch_prctl),
        SYSCALL_ENTRY(217, "getdents64", vfs::sys_getdents64),
        SYSCALL_ENTRY(257, "openat", vfs::sys_openat),
        SYSCALL_ENTRY(258, "mkdirat", vfs::sys_mkdirat),
        SYSCALL_ENTRY(259, "mknodat", vfs::sys_mknodat),
        SYSCALL_ENTRY(262, "fstatat", vfs::sys_fstatat),
        SYSCALL_ENTRY(263, "unlinkat", vfs::sys_unlinkat),
        SYSCALL_ENTRY(265, "linkat", vfs::sys_linkat),
        SYSCALL_ENTRY(267, "readlinkat", vfs::sys_readlinkat),
        SYSCALL_ENTRY(268, "fchmodat", vfs::sys_fchmodat),
        SYSCALL_ENTRY(292, "dup3", vfs::sys_dup3)
#undef  SYSCALL_ENTRY
    });

    extern "C" void syscall_handler(cpu::registers_t *regs)
    {
        if (auto entry = map.find(regs->rax); entry != map.end())
        {
            this_thread()->saved_regs = *regs;
            regs->rax = cpu::as_user(
                [&entry](auto arg) { return entry->second.run(arg); },
                std::array { regs->rdi, regs->rsi, regs->rdx, regs->r10, regs->r8, regs->r9 }
            );
        }
        else
        {
            log::errorln("Unknown syscall {}!", regs->rax);
            regs->rax = -ENOSYS;
        }
    }

    void init()
    {
        // support both syscall and int 0x80
        idt::handlers[idt::INT_SYSCALL].set(syscall_handler);
    }
} // namespace syscall
# Copyright (C) 2022  ilobilo

.include "arch/x86_64/cpu/cpu.inc"

.extern syscall_handler
syscall_entry:
    swapgs

    mov gs:[16], rsp
    mov rsp, gs:[24]

    push 0x23
    push gs:[16]
    push r11
    push 0x1B
    push rcx
    push 0
    push 0

    pushall

    sti
    mov rdi, rsp
    call syscall_handler
    cli

    popall

    add rsp, 56
    mov rsp, gs:[16]

    swapgs
    sysretq
.global syscall_entry
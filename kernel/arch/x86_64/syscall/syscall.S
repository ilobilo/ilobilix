# Copyright (C) 2022  ilobilo

.include "arch/x86_64/cpu/cpu.inc"

.extern syscall_handler
syscall_entry:
    swapgs

    mov gs:[16], rsp
    mov rsp, gs:[24]

    push 0x1B # ss
    push gs:[16] # rsp
    push r11 # rflags
    push 0x23 # cs
    push rcx # rip
    push 0 # error_code
    push 0 # int_no

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
# Copyright (C) 2022  ilobilo

.include "arch/x86_64/cpu/cpu.inc"

.extern int_handler
int_common_stub:
    test qword ptr [rsp + 24], 0x03
    jz 1f
    swapgs
1:

    pushall
    mov rdi, rsp
    call int_handler
    popall
    add rsp, 16

    test qword ptr [rsp + 8], 0x03
    jz 1f
    swapgs
1:

    iretq

.macro isr number
    isr_\number:
.if !(\number == 8 || (\number >= 10 && \number <= 14) || \number == 17 || \number == 21 || \number == 29 || \number == 30)
    push 0
.endif
    push \number
    jmp int_common_stub
.endm

.altmacro
.macro isr_insert number
    .section .text
    isr \number

    .section .data
    .quad isr_\number
.endm

.section .data
int_table:
.set i, 0
.rept 256
    isr_insert %i
    .set i, i + 1
.endr
.global int_table
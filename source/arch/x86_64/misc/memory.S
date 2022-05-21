# Copyright (C) 2022  ilobilo

#if defined(__x86_64__)

memcpy:
    mov rcx, rdx
    mov rax, rdi
    rep movsb
    ret
.global memcpy

memset:
    push rdi
    mov rax, rsi
    mov rcx, rdx
    rep stosb
    pop rax
    ret
.global memset

memmove:
    mov rcx, rdx
    mov rax, rdi

    cmp rdi, rsi
    ja 1f

    rep movsb
    jmp 2f

1:
    lea rdi, [rdi + rcx - 1]
    lea rsi, [rsi + rcx - 1]
    std
    rep movsb
    cld

2:
    ret
.global memmove

memcmp:
    mov rcx, rdx
    repe cmpsb
    jecxz 1f

    mov al, byte ptr [rdi - 1]
    sub al, byte ptr [rsi - 1]
    movsx rax, al
    jmp 2f

1:
    xor eax, eax

2:
    ret
.global memcmp

#endif
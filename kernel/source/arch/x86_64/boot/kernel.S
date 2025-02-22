.text

.extern kernel_stack_top
.extern kmain

_start:
    mov rsp, kernel_stack_top
    call kmain
    ud2
.global _start

.extern arch_core_entry

cpu_entry:
    mov rax, [rdi + 24]
    mov rsp, [rax + 24]
    call arch_core_entry
    ud2
.global cpu_entry
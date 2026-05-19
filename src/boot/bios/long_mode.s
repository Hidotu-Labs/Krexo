[bits 32]

section .text
global switch_to_long_mode

switch_to_long_mode:
    ; Save initial parameters
    mov esi, [esp + 4]      ; PML4 address
    mov ebx, [esp + 8]      ; Kernel Entry point
    mov edx, [esp + 12]     ; Boot Info

    ; 1. Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 2. Enable SSE
    mov eax, cr0
    and ax, 0xFFFB
    or ax, 0x0022
    mov cr0, eax
    mov eax, cr4
    or ax, 3 << 9
    mov cr4, eax

    ; 3. Load CR3
    mov cr3, esi

    ; 4. Enable Long Mode in EFER
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 5. Load 64-bit GDT
    lgdt [gdt64_ptr]

    ; 6. Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 7. Far jump to 64-bit mode
    push 0x08
    push long_mode_entry
    jmp far [esp]

[bits 64]
long_mode_entry:
    ; Now in 64-bit mode
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov rsp, kernel_stack_top

    ; Pass boot_info and entry point
    mov edi, 0x1000
    mov ebx, ebx

    ; To the Kernel!
    jmp rbx

section .bss
align 4096
kernel_stack:
    resb 8192
kernel_stack_top:

section .text
align 16
gdt64:
    dq 0x0000000000000000 ; Null
    dq 0x00209a0000000000 ; 64-bit Code
    dq 0x0000920000000000 ; 64-bit Data
gdt64_end:

gdt64_ptr:
    dw gdt64_end - gdt64 - 1
    dd gdt64

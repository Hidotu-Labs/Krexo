[bits 16]
[org 0x4000]

trampoline_start:
    cli
    cld

    ; Initialize segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; 1. Load temporary 32-bit GDT
    lgdt [gdt32_ptr]

    ; 2. Enable protected mode
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 3. Jump to 32-bit code (offset 0x08 in GDT)
    jmp 0x08:trampoline32

[bits 32]
trampoline32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; --- SETUP LONG MODE ---
    ; 1. Load the PML4 address
    ; We expect the bootloader to put the CR3 value at 0x4500
    mov eax, [0x4500]
    mov cr3, eax

    ; 2. Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 3. Enable Long Mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 4. Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 5. Jump to 64-bit code (offset 0x18 in GDT)
    jmp 0x18:trampoline64

[bits 64]
trampoline64:
    ; We are in 64-bit mode!
    cli
    
    ; Simple spinlock for now
.park:
    hlt
    jmp .park

align 16
gdt32:
    dq 0 ; Null
    dq 0x00CF9A000000FFFF ; 32-bit Code
    dq 0x00CF92000000FFFF ; 32-bit Data
    dq 0x00AF9A000000FFFF ; 64-bit Code
gdt32_ptr:
    dw $ - gdt32 - 1
    dd gdt32

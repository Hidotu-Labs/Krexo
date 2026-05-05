[org 0x7c00]
[bits 16]

; Entry point
start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [BOOT_DRIVE], dl ; Save boot drive from DL

    mov si, msg_load
    call print_string

    ; Load Stage 2 from disk
    mov di, 5 ; Retry count
.retry:
    mov ah, 0x02    ; BIOS read sectors
    mov al, 9       ; Number of sectors (let's load 9 more)
    mov ch, 0       ; Cylinder 0
    mov dh, 0       ; Head 0
    mov cl, 2       ; Sector 2
    mov dl, [BOOT_DRIVE]
    mov bx, 0x8000  ; Destination
    int 0x13
    jnc .success
    
    dec di
    jnz .retry
    jmp disk_error

.success:

    ; Switch to Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebp, 0x90000    ; Set up stack
    mov esp, ebp

    call 0x8000        ; Jump to Stage 2 C code

    jmp $

[bits 16]
print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

disk_error:
    mov si, msg_error
    call print_string
    jmp $

msg_load  db "Loading Stage 2...", 0x0d, 0x0a, 0
msg_error db "Disk error!", 0

BOOT_DRIVE db 0

; GDT
gdt_start:
    dd 0x0          ; Null descriptor
    dd 0x0

gdt_code:           ; Code segment
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:           ; Data segment
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xaa55

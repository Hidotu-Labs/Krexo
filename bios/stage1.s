[org 0x7c00]
[bits 16]

; Entry point
start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [BOOT_DRIVE], dl ; Save boot drive

    ; Print Loading
    mov si, msg_load
    call print_string

    ; Check LBA
    mov ah, 0x41
    mov bx, 0x55aa
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc no_lba

    ; Load Stage 2 (LBA 1, 63 sectors to 0x8000)
    mov si, dap
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Setup Memory Map and VBE
    call get_mmap
    call setup_vbe

    ; GDT load and switch to Protected Mode
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:init_pm

[bits 32]
init_pm:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov ebp, 0x90000
    mov esp, ebp

    movzx eax, byte [BOOT_DRIVE]
    push eax
    call 0x8000
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
    mov si, msg_err
    call print_string
    jmp $

no_lba:
    mov si, msg_lba
    call print_string
    jmp $

get_mmap:
    mov di, 0x7010
    xor ebx, ebx
    mov byte [0x7000], 0
.loop:
    mov eax, 0xE820
    mov ecx, 24
    mov edx, 0x534D4150
    int 0x15
    jc .done
    add di, 24
    inc byte [0x7000]
    test ebx, ebx
    jnz .loop
.done:
    ret

setup_vbe:
    mov ax, 0x4f01
    mov cx, 0x411b
    mov di, 0x5000
    int 0x10
    cmp ax, 0x004f
    jne .vbe_done
    
    ; Copy mode info to 0x6000
    mov eax, [0x5028] ; LFB
    mov [0x6000], eax
    mov ax, [0x5012]   ; Width
    mov [0x6004], ax
    mov ax, [0x5014]   ; Height
    mov [0x6006], ax
    mov ax, [0x5010]   ; Pitch
    mov [0x6008], ax
    mov al, [0x5019]   ; BPP
    mov [0x600a], al
    mov al, [0x5020]   ; Red Field Position
    mov [0x600b], al
    mov al, [0x5022]   ; Green Field Position
    mov [0x600c], al
    mov al, [0x5024]   ; Blue Field Position
    mov [0x600d], al

    mov ax, 0x4f02
    mov bx, 0x411b
    int 0x10
.vbe_done:
    ret

msg_load db "Loading...", 13, 10, 0
msg_err  db "Disk error!", 0
msg_lba  db "LBA error!", 0

align 16
dap:
    db 16, 0
    dw 127
    dw 0x8000
    dw 0x0000
    dq 1

BOOT_DRIVE db 0

align 8
gdt_start:
    dq 0 ; Null
gdt_code:
    dw 0xffff, 0
    db 0, 10011010b, 11001111b, 0
gdt_data:
    dw 0xffff, 0
    db 0, 10010010b, 11001111b, 0
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510-($-$$) db 0
dw 0xaa55

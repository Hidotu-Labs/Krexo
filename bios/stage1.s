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

    ; Query memory map using INT 0x15 E820
    call get_memory_map

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

mmap_error:
    mov si, msg_mmap
    call print_string
    jmp $

; Get memory map using E820
; Stores entry count at 0x7000, entries at 0x7010
get_memory_map:
    mov di, 0x7010          ; Destination for entries
    xor ebx, ebx            ; Continuation value (must be 0 on first call)
    mov byte [MMAP_ENTRIES], 0

.next_entry:
    mov eax, 0xE820         ; Function number
    mov ecx, 24             ; Entry size (ACPI 3.0+)
    mov edx, 0x534D4150     ; 'SMAP' signature
    int 0x15
    jc .mmap_failed         ; Carry = error
    cmp eax, 0x534D4150     ; Check signature
    jne .mmap_failed

    ; Entry was written to DI, advance pointer
    add di, 24
    inc byte [MMAP_ENTRIES]

    ; Check if we've reached max entries (64)
    mov al, [MMAP_ENTRIES]
    cmp al, 64
    jge .done

    ; Check if this was the last entry (EBX=0)
    test ebx, ebx
    jz .done

    jmp .next_entry

.mmap_failed:
    ; Could be a non-fatal error, try to continue
    cmp byte [MMAP_ENTRIES], 0
    je mmap_error           ; No entries at all = fatal

.done:
    ; Store entry count at 0x7000
    mov al, [MMAP_ENTRIES]
    mov [0x7000], al
    ret

msg_load  db "Loading Stage 2...", 0x0d, 0x0a, 0
msg_error db "Disk error!", 0
msg_mmap  db "Memory map error!", 0

BOOT_DRIVE db 0
MMAP_ENTRIES db 0

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

[bits 32]
extern stage2_main
global _start

_start:
    ; At this point, the stack looks like:
    ; [esp + 4]: The boot drive pushed by Stage 1
    ; [esp + 0]: The return address from Stage 1's call (which we don't need)
    
    ; We need to push the boot drive again for stage2_main(uint32_t boot_drive)
    push dword [esp + 4]
    call stage2_main
    
    ; Should never return
    jmp $

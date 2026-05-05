ENTRY(stage2_main)
OUTPUT_FORMAT("binary")

SECTIONS
{
    . = 0x8000;
    .text : {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .rodata : {
        *(.rodata)
    }
    .bss : {
        *(COMMON)
        *(.bss)
    }
}

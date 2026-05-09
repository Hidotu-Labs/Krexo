ENTRY(kernel_main)

SECTIONS
{
    /* Kernel base address: 16MB */
    . = 0x1000000;

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .krexo_requests :
    {
        KEEP(*(.krexo_requests))
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(COMMON)
        *(.bss)
    }
}

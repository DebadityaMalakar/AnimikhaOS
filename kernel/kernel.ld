ENTRY(start)  /* Assuming 'start' is your entry point label in main.asm */

SECTIONS
{
    . = 0x100000;    /* Kernel loads at 1MB */

    .text :
    {
        *(.text)
    }

    .rodata :
    {
        *(.rodata)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }
}
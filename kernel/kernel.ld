ENTRY(start)  /* Assuming 'start' is your entry point label in main.asm */
SECTIONS
{
    . = 0x1000;    /* Kernel loaded at 0x1000 by bootloader */

    .text :
    {
        /* .text.startup section is first so it
           will be the first code executed */
        *(.text.startup)
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = ALIGN(4);
    }
    __bss_sizel = (__bss_end - __bss_start) / 4;
}

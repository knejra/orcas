
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(loader)

SECTIONS
{
    . = 0x100000;

    .text :
    {
        *(.multiboot)
        *(.text*)
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

    PROVIDE(kernheap = .);

    /DISCARD/ : 
    {
        *(.comment)
    }
}
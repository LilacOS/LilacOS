OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDRESS = 0x80200000;

SECTIONS
{
    /* "." 是当前地址 */
    . = BASE_ADDRESS;
    skernel = .;

    .text : {
        stext = .;
        *(.text.entry)
        *(.text .text.*)
        . = ALIGN(4K);
        etext = .;
    }

    .rodata : {
        srodata = .;
        *(.rodata .rodata.*)
        . = ALIGN(4K);
        erodata = .;
    }

    .data : {
        sdata = .;
        *(.data .data.*)
        . = ALIGN(4K);
        edata = .;
    }

    .bss : {
        sbss_with_stack = .;
        *(.bss.stack)
        sbss = .;
        *(.sbss .bss .bss.*)
        . = ALIGN(4K);
        ebss = .;
    }

    PROVIDE(ekernel = .);
}
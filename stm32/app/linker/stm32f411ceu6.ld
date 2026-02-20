ENTRY(reset_handler)

MEMORY
{
    rom (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_stack = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
    .vectors : {
        KEEP(*(.vectors))
    } > rom

    .text : {
        *(.text*)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
    } > rom

    .preinit_array : {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > rom

    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > rom

    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > rom

    .data : {
        _data = .;
        *(.data*)
        _edata = .;
    } > ram AT > rom
    _data_loadaddr = LOADADDR(.data);

    .bss : {
        _bss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > ram

    . = ALIGN(8);
    _end = .;
}
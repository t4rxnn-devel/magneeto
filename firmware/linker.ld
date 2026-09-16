ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x60000000, LENGTH = 2048K
    ITCM  (rwx) : ORIGIN = 0x00000000, LENGTH = 512K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20200000, LENGTH = 1024K
}

__stack_top = ORIGIN(DTCM) + LENGTH(DTCM);
__heap_size = 0x10000;
__stack_size = 0x08000;

SECTIONS
{
    .vectors :
    {
        . = ALIGN(4);
        KEEP(*(.vectors))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        *(.ramfunc)
        *(.ramfunc*)
        . = ALIGN(4);
        _edata = .;
    } > DTCM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > DTCM

    .heap :
    {
        . = ALIGN(8);
        __end__ = .;
        provide(end = .);
        _sheap = .;
        . = . + __heap_size;
        . = ALIGN(8);
        _eheap = .;
    } > RAM

    .stack :
    {
        . = ALIGN(8);
        . = . + __stack_size;
        . = ALIGN(8);
    } > DTCM

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}

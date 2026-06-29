MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08002000, LENGTH = 0xFE000
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x20000
}

__stack_size = 0x1000;
__heap_size  = 0x1000;

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > RAM

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
        _etext = .;
    } > RAM

    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM

    .bss :
    {
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    .stack :
    {
        . = ALIGN(8);
        . = . + __stack_size;
        . = ALIGN(8);
    } > RAM

    .heap :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + __heap_size;
        __heap_end__ = .;
        . = ALIGN(8);
    } > RAM

    .noinit :
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit.*)
        . = ALIGN(4);
    } > RAM

    _end = .;
    __end = _end;
}
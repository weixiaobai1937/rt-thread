/**
 * @file    acm32p4xx_flash.ld
 * @brief   GCC Linker Script for ACM32P4XX (Flash execution)
 * @note    Flash: 0x08002000, Size: 0xFE000 (~1016KB)
 *          SRAM:  0x20000000, Size: 0x20000 (128KB)
 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08002000, LENGTH = 0xFE000
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x20000
}

/* Stack and Heap sizes */
__stack_size = 0x1000;
__heap_size  = 0x1000;

SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code and read-only data */
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
    } > FLASH

    /* Initialized data */
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
    } > RAM AT > FLASH

    /* Uninitialized data */
    . = ALIGN(4);
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

    /* Stack */
    .stack :
    {
        . = ALIGN(8);
        . = . + __stack_size;
        . = ALIGN(8);
    } > RAM

    /* Heap */
    .heap :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . = . + __heap_size;
        __heap_end__ = .;
        . = ALIGN(8);
    } > RAM

    /* Noinit section */
    .noinit :
    {
        . = ALIGN(4);
        *(.noinit)
        *(.noinit.*)
        . = ALIGN(4);
    } > RAM

    /* End of RAM */
    _end = .;
    __end = _end;
}
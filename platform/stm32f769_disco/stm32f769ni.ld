/* Linker script for STM32F769NIH6 (2MB Flash, 512KB RAM, 16MB SDRAM) */

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    DTCRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    SRAM1  (rwx) : ORIGIN = 0x20020000, LENGTH = 368K
    SRAM2  (rwx) : ORIGIN = 0x2007C000, LENGTH = 16K
    SDRAM  (rwx) : ORIGIN = 0xC0000000, LENGTH = 16384K
}

_stack_size = 16K;

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.init)
        *(.fini)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .data : AT(_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > DTCRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM1

    ._stack :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + _stack_size;
        . = ALIGN(8);
        _estack = .;
    } > DTCRAM

    /* Framebuffer in SDRAM */
    .sdram (NOLOAD) :
    {
        . = ALIGN(4);
        _ssdram = .;
        *(.sdram)
        *(.sdram*)
        . = ALIGN(4);
        _esdram = .;
    } > SDRAM

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.attributes*)
    }
}

_end = .;
PROVIDE(end = .);

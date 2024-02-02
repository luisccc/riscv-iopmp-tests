OUTPUT_FORMAT(elf64-littleriscv)
ENTRY(_boot)
STACK_SIZE = 1M;
MEMORY {
    RAM (rwx) : ORIGIN = (0x80000000), LENGTH = (0x10000000)
}
SECTIONS {
 .boot : {
  *(.boot)
 }
 .text : {
  *(.text)
 }
    . = ALIGN(0x1000);
    .tohost : { *(.tohost) }
 .rodata : {
  *(.rodata)
 }
    .test_table : {
        _test_table = .;
  *(.test_table)
        _test_table_end = .;
 }
    _test_table_size = (_test_table_end - _test_table) / 8;
 .data : {
  *(.data)
 }
 .bss (NOLOAD) : {
  __bss_start = .;
  *(.bss)
  *(.sbss)
  *(COMMON)
  __bss_end = .;
 }
 . = (0x80000000) + 0x1ff000;
    . = ALIGN(16) + STACK_SIZE;
    __stack_top = .;
    _end = .;
}

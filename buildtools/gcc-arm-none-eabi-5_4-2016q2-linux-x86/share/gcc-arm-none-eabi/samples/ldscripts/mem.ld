/* Linker script to configure memory regions. 
 * Need modifying for a specific board. 
 *   FLASH.ORIGIN: starting address of flash
 *   FLASH.LENGTH: length of flash
 *   RAM.ORIGIN: starting address of RAM bank 0
 *   RAM.LENGTH: length of RAM bank 0
 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x0, LENGTH = 0x20000 /* 128K */
  RAM (rwx) : ORIGIN = 0x10000000, LENGTH = 0x2000 /* 8K */
}

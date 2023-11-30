#pragma once
#include <stdbool.h>
#include <inttypes.h>
/* CGA
#define VIDEORAM_START32 0xB8000ul
#define VIDEORAM_END32 0xC8000ul
*/
#define VIDEORAM_START32 0xA0000ul
#define VIDEORAM_END32 0xBFFFFul

#define VIDEORAM_SIZE (VIDEORAM_END32 - VIDEORAM_START32)

#define RAM_PAGE_SIZE_KB 4ul
#define RAM_PAGE_SIZE (RAM_PAGE_SIZE_KB * 1024)
#define RAM_IN_PAGE_ADDR_MASK (0x00000FFF)

#if PICO_ON_DEVICE
// CGA
//#define RAM_SIZE (4 * 40ul << 10) // 40 pages 4Kb = 160KB real pico RAM
#define RAM_SIZE (4 * 24ul << 10) // 40 pages 4Kb = 160KB real pico RAM
#else
#define RAM_SIZE (640ul << 10)
#endif

extern uint8_t RAM[RAM_SIZE];

#define RAM_BLOCKS (RAM_SIZE / RAM_PAGE_SIZE)
extern uint16_t RAM_PAGES[RAM_BLOCKS]; // lba (14-0); 15 - written

#define CONVENTIONAL_END 0xA0000ul

bool init_vram();

uint8_t ram_page_read(uint32_t addr32);
uint16_t ram_page_read16(uint32_t addr32);

void ram_page_write(uint32_t addr32, uint8_t value);
void ram_page_write16(uint32_t addr32, uint16_t value);

void read_vram_block(char* dst, uint32_t file_offset, uint32_t sz);
void flush_vram_block(const char* src, uint32_t file_offset, uint32_t sz);


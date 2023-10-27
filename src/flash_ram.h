//
// Created by xrip on 23.10.2023.
//
#pragma once
#ifndef TINY8086_FLASHRAM_H
#define TINY8086_FLASHRAM_H
//ESP8266 version of the SRAM handler
//Runs with a flashbased cached MMU swap system giving the emulator 1MB of RAM
//Raspberry Pi Pico version of the SRAM handler
//Runs with the internal flash memory swap system giving the emulator 1MB of RAM

#include <hardware/flash.h>
#include "memory.h"

uint8_t lowmemRAM[0x600];
uint8_t cacheRAM1[4096];
uint8_t cacheRAM2[4096];
uint32_t block1=0xffffffff;
uint32_t block2=0xffffffff;
uint8_t dirty1=0;
uint8_t dirty2=0;
uint8_t NextCacheUse=0;

//The flash offset where the SRAM data is stored
#define FLASH_OFFSET ((1024 + 512) * 1024)


uint8_t SRAM_read(uint32_t address) {
    if (address<0x600) return lowmemRAM[address]; //If RAM lower than 0x600 return direct RAM
    uint32_t block=(address&0xfffff000)+FLASH_OFFSET; //Else look into cached RAM
    if (block1==block) return cacheRAM1[address&0x00000fff]; //Cache hit in cache1
    if (block2==block) return cacheRAM2[address&0x00000fff]; //Cache hit in cache2

    //Cache miss, fetch it

    if (NextCacheUse==0) {
        if (dirty1) {           //Write back the cached block if it's dirty
            uint32_t interrupts = save_and_disable_interrupts();
            flash_range_erase(block1, FLASH_SECTOR_SIZE);
            flash_range_program(block1, cacheRAM1, FLASH_SECTOR_SIZE);
            restore_interrupts(interrupts);

            dirty1=0;
        }

        printf("miss1\r\n");
        memcpy(cacheRAM1, (const uint8_t *)(XIP_BASE+block), FLASH_SECTOR_SIZE);

        block1=(address&0xfffff000)+FLASH_OFFSET;
        NextCacheUse+=1;
        NextCacheUse&=1;
        return cacheRAM1[address&0x00000fff]; //Now found it in cache1
    }

    if (NextCacheUse==1) {
        if (dirty2) {           //Write back the cached block if it's dirty
            uint32_t interrupts = save_and_disable_interrupts();
            flash_range_erase(block2, FLASH_SECTOR_SIZE);
            flash_range_program(block2, cacheRAM2, FLASH_SECTOR_SIZE);
            restore_interrupts(interrupts);
            dirty2=0;
        }
        printf("miss2\r\n");
        memcpy(cacheRAM2, (const uint8_t *)(XIP_BASE+block), FLASH_SECTOR_SIZE);

        block2=(address&0xfffff000)+FLASH_OFFSET;
        NextCacheUse+=1;
        NextCacheUse&=1;
        return cacheRAM2[address&0x00000fff]; //Now found it in cache2
    }

}

void SRAM_write(uint32_t address, uint8_t value) {
    if (address<0x600) {
        lowmemRAM[address]=value; //If RAM lower than 0x600 write direct RAM
        return;
    }
    uint32_t block=(address&0xfffff000)+FLASH_OFFSET; //Else write into cached block

    if (block1==block) {
        cacheRAM1[address&0x00000fff]=value; //Write in cache1
        dirty1=1; //mark cache1 as dirty
        return;
    }

    if (block2==block) {
        cacheRAM2[address&0x00000fff]=value; //Write in cache2
        dirty2=1; //mark cache2 as dirty
        return;
    }

    //Cache miss, fetch it

    if (NextCacheUse==0) {
        if (dirty1) {           //Write back the cached block if it's dirty
            uint32_t interrupts = save_and_disable_interrupts();
            flash_range_erase(block1, FLASH_SECTOR_SIZE);
            flash_range_program(block1, cacheRAM1, FLASH_SECTOR_SIZE);
            restore_interrupts(interrupts);
        }

        memcpy(cacheRAM1, (const uint8_t *)(XIP_BASE+block), FLASH_SECTOR_SIZE);

        block1=(address&0xfffff000)+FLASH_OFFSET;
        NextCacheUse+=1;
        NextCacheUse&=1;
        cacheRAM1[address&0x00000fff]=value; //Now write it in cache1
        dirty1=1;
        return;
    }

    if (NextCacheUse==1) {
        if (dirty2) {           //Write back the cached block if it's dirty
            uint32_t interrupts = save_and_disable_interrupts();
            flash_range_erase(block2, FLASH_SECTOR_SIZE);
            flash_range_program(block2, cacheRAM2, FLASH_SECTOR_SIZE);
            restore_interrupts(interrupts);
        }

        memcpy(cacheRAM2, (const uint8_t *)(XIP_BASE+block), FLASH_SECTOR_SIZE);

        block2=(address&0xfffff000)+FLASH_OFFSET;
        NextCacheUse+=1;
        NextCacheUse&=1;
        cacheRAM2[address&0x00000fff]=value; //Now write it in cache2
        dirty2=1;
        return;
    }

}




#endif
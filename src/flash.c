/*
   Copyright (c) 2015 Arduino LLC.  All right reserved.
   Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.
   Copyright (C) 2017 Industruino <connect@industruino.com>  All right reserved.
   Copyright (c) 2018 Blokable, Inc All rights reserved

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
   */

#include <sam.h>
#include <string.h>

#include "flash.h"
#include "utils.h"
#include "log.h"

// Size of a flash page (in bytes)
static uint32_t PAGE_SIZE;

// Define some commonly used combos of PAGE_SIZE and PAGES
#define PAGES                 (NVMCTRL->PARAM.bit.NVMP)
#define PAGE_SIZE_IN_WORDS    (PAGE_SIZE >> 2)
#define ROW_SIZE              (PAGE_SIZE * 4)
#define ROW_SIZE_IN_WORDS     (ROW_SIZE >> 2)
#define MAX_FLASH             (PAGE_SIZE * PAGES)

#define APP_FLASH_MEMORY_START_PTR  ((uint32_t *) &__sketch_vectors_ptr)

static uint32_t *flashProgrammingPtr;
static uint32_t imageSize;

void flash_init() {
  //uint32_t pageSizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
  //PAGE_SIZE = pageSizes[NVMCTRL->PARAM.bit.PSZ];
  PAGE_SIZE = 1 << (NVMCTRL->PARAM.bit.PSZ + 3);

  flashProgrammingPtr = APP_FLASH_MEMORY_START_PTR;
  imageSize = 0;
}

// Erase a single flash row
static void flash_erase_row(uint32_t *dst_addr) {
  // Note: the flash memory is erased in ROWS, that is in block of 4 pages.
  //       Even if the starting address is the last byte of a ROW the entire
  //       ROW is erased anyway.

  // Execute "ER" Erase Row
  NVMCTRL->ADDR.reg = (uint32_t)dst_addr >> 1; // 16bit word address, so shift right
  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_ER;
  while (NVMCTRL->INTFLAG.bit.READY == 0);
}

static void flash_write(uint32_t *src_addr, uint32_t *dst_addr, uint32_t size) {
  // Set automatic page write
  NVMCTRL->CTRLB.bit.MANW = 0;

  // Do writes in pages
  while (size) {
    // Execute "PBC" Page Buffer Clear
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_PBC;
    while (NVMCTRL->INTFLAG.bit.READY == 0);

    // Fill page buffer
    uint32_t i;
    for ( i = 0; i < PAGE_SIZE_IN_WORDS && i < size; i++) {
      dst_addr[i] = src_addr[i];
    }

    // Execute "WP" Write Page
    NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMDEX_KEY | NVMCTRL_CTRLA_CMD_WP;
    while (NVMCTRL->INTFLAG.bit.READY == 0);

    // Advance to next page
    dst_addr += i;
    src_addr += i;
    size     -= i;
  }
}

static void flash_update_row(uint8_t* rowBuffer, uint32_t* flashPtr) {
  LOG_STR("Flash: ptr:");
  LOG_HEX(flashPtr);
  if (memcmp(rowBuffer, flashPtr, ROW_SIZE) != 0) {
    flash_erase_row(flashPtr);
    flash_write((uint32_t *) rowBuffer, flashPtr, ROW_SIZE_IN_WORDS);
    LOG(" PROG ");
  } else {
    LOG(" skip ");
  }
}

bool flash_tftp_buffer(uint8_t* buffer, uint32_t length) {
  // Flash full rows from the buffer
  while (length >= ROW_SIZE) {
    flash_update_row(buffer, flashProgrammingPtr);
    flashProgrammingPtr += ROW_SIZE_IN_WORDS;
    imageSize += ROW_SIZE;
    buffer += ROW_SIZE;
    length -= ROW_SIZE;
  }

  // Flash a partial row
  if (length > 0) {
    uint8_t rowBuffer[ROW_SIZE];

    memcpy(rowBuffer, buffer, length);

    // Fill remaining bytes with 0xFF
    for (uint16_t idx = length; idx < ROW_SIZE; idx++) {
      rowBuffer[idx] = 0xFF;
    }

    flash_update_row(rowBuffer, flashProgrammingPtr);
    flashProgrammingPtr += ROW_SIZE_IN_WORDS;
    imageSize += length;
  }

  LOG_STR("size: ");
  LOG_HEX(imageSize);
  LOG_STR("\r\n");

  if (flashProgrammingPtr - APP_FLASH_MEMORY_START_PTR >= MAX_FLASH) {
    LOG("flash overflow");
    return false;
  }

  return true;
}


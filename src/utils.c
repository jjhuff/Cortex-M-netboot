//  Utility functions
//  Copyright (C) 2017  Industruino <connect@industruino.com>
//  Copyright (c) 2018 Blokable, Inc All rights reserved
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
//  Developed by Claudio Indellicati <bitron.it@gmail.com>
//  (implementation of 'startApplication()' moved here from 'main.c')

#include "utils.h"
#include "board_definitions.h"
#include "log.h"

volatile uint32_t* pulSketch_Start_Address;

volatile uint64_t tickCount = 0ULL;

uint64_t millis (void) {
  return tickCount;
}

void delay (uint32_t delayInMilliseconds) {
  uint64_t endCount = tickCount + (delayInMilliseconds * (CPU_FREQUENCY / 1000000UL));

  while (tickCount < endCount);
}

void startApplication (void) {
#if defined(REBOOT_STATUS_ADDRESS)
  REBOOT_STATUS_VALUE = REBOOT_STATUS_START_APP;
  NVIC_SystemReset();
#else
  jumpToApplication();
#endif
}

void jumpToApplication (void) {
  uint32_t* pulSketch_Start_Address;

  // Load the sketch Reset Handler address
  // __sketch_vectors_ptr is exported from linker script and point on first 32b word of sketch vector table
  // First 32b word is sketch stack
  // Second 32b word is sketch entry point: Reset_Handler()
  pulSketch_Start_Address = &__sketch_vectors_ptr ;
  pulSketch_Start_Address++;

  // Rebase the Stack Pointer
  __set_MSP((uint32_t)(__sketch_vectors_ptr));

  // Rebase the vector table base address
  SCB->VTOR = ((uint32_t)(&__sketch_vectors_ptr) & SCB_VTOR_TBLOFF_Msk);

  // Jump to application Reset Handler in the application
  asm("bx %0"::"r"(*pulSketch_Start_Address));
}

// Populate words with the 128bit serial number
void getDeviceSerialNumber(uint32_t words[4]) {
  // These addresses are from the ATSAMD21 datasheet section 9.3.3
  volatile uint32_t *a = (volatile uint32_t *)0x0080A00C;
  volatile uint32_t *b = (volatile uint32_t *)0x0080A040;
  words[0] = a[0];
  words[1] = b[0];
  words[2] = b[1];
  words[3] = b[2];
}

// XOR the words of the full 128 bit serial number
uint32_t getDeviceSerialNumber32() {
  uint32_t words[4];
  getDeviceSerialNumber(words);

  return words[0] ^ words[1] ^ words[2] ^ words[3];
}


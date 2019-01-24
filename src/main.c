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

#include <stdio.h>
#include <string.h>
#include <sam.h>
#include "board_definitions.h"
#include "board_driver_led.h"
#include "board_driver_i2c.h"
#include "networking.h"
#include "tftp.h"
#include "utils.h"
#include "log.h"
#include "dhcp.h"

extern void board_init(void);

static volatile uint8_t led_pulse_rate = 1;

static bool exitBootloaderAfterTimeout = true;

/* Include the version in the flash 'footer' */
__attribute__ ((section(".bl_footer")))
const char *version = VERSION;

/**
 * \brief Check the application startup condition
 *
 */
static void check_start_application(void)
{
#if defined(REBOOT_STATUS_ADDRESS)
  if (REBOOT_STATUS_VALUE == REBOOT_STATUS_START_APP) {
    /* Requested to start the app */
    REBOOT_STATUS_VALUE = REBOOT_STATUS_UNDEFINED;
    jumpToApplication();
  } else if (REBOOT_STATUS_VALUE == REBOOT_STATUS_DOUBLE_TAP_MAGIC) {
    /* Second tap, stay in bootloader */
    REBOOT_STATUS_VALUE = REBOOT_STATUS_UNDEFINED;
    exitBootloaderAfterTimeout = false;
    return;
  }

  if (PM->RCAUSE.bit.POR || PM->RCAUSE.bit.BOD12 || PM->RCAUSE.bit.BOD33) {
    // Power on reset OR brown-out.
    REBOOT_STATUS_VALUE = REBOOT_STATUS_UNDEFINED;
  } else if (PM->RCAUSE.bit.EXT) {
    // External reset (button)
    REBOOT_STATUS_VALUE = REBOOT_STATUS_DOUBLE_TAP_MAGIC;

    /* Wait 1sec to see if the user tap reset again.
     * The loop value is based on SAMD21 default 1MHz clock @ reset.
     */
    for (uint32_t i=0; i<250000 ; i++) /* 1000ms */
      /* force compiler to not optimize this... */
      __asm__ __volatile__("");

    /* Timeout happened, continue boot... */
    REBOOT_STATUS_VALUE = REBOOT_STATUS_UNDEFINED;
  }
#endif

  /*
   * Test sketch stack pointer @ &__sketch_vectors_ptr
   * Stay in SAM-BA if value @ (&__sketch_vectors_ptr) == 0xFFFFFFFF (Erased flash cell value)
   */
  if (__sketch_vectors_ptr == 0xFFFFFFFF)
  {
    /* Stay in bootloader */
    exitBootloaderAfterTimeout = false;
    return;
  }

  /*
   * Test vector table address of sketch @ &__sketch_vectors_ptr
   * Stay in SAM-BA if this function is not aligned enough, ie not valid
   */
  if ( ((uint32_t)(&__sketch_vectors_ptr) & ~SCB_VTOR_TBLOFF_Msk) != 0x00)
  {
    /* Stay in bootloader */
    exitBootloaderAfterTimeout = false;
    return;
  }

  // Continue to boot
  exitBootloaderAfterTimeout = true;
}

/**
 *  \brief SAMD21 SAM-BA Main loop.
 *  \return Unused (ANSI-C compatibility).
 */
int main(void)
{
  /* Jump in application if condition is satisfied */
  check_start_application();

  /* We have determined we should stay in the monitor. */
  /* System initialization */
  board_init();
  __enable_irq();

  /* Initialize LEDs */
  LED_init();

  /* Start the sys tick (1/48th of a millisecond) */
  SysTick_Config(1000);

  // Init logging & wait for a USB connection (only debug mode)
  logInit();

  LOG_STR("Version: ");
  LOG_STR(version);
  LOG("");

  // Init I2C
  #ifdef I2C_SERCOM
  i2c_init(I2C_BITRATE);
  #endif

  // Init network
  LOG("init start");
  if (!netInit()) {
    LOG("netInit: failed")
  }

  // Send DHCP request
  dhcpInit();
  uint64_t bootloaderExitTime = millis() + BOOTLOADER_MAX_RUN_TIME;
  while(1) {
    if(dhcpRun()) {
      LOG("DHCP: got response");

      break;
    }

    if (exitBootloaderAfterTimeout && (millis() > bootloaderExitTime)) {
      LOG("DHCP: no response, booting");
      startApplication();
    }
  }
  dhcpEnd();

  led_pulse_rate = 2; // 2x second is after DHCP

  // Start TFTP
  tftpInit();
  LOG_STR("TFTP: Start ");
  LOG_HEX_BYTE(netConfig.tftpServer[0]);
  LOG_HEX_BYTE(netConfig.tftpServer[1]);
  LOG_HEX_BYTE(netConfig.tftpServer[2]);
  LOG_HEX_BYTE(netConfig.tftpServer[3]);
  LOG_STR(" '");
  LOG_STR(netConfig.tftpFile);
  LOG_STR("'\r\n");

  // Request the file
  tftpRequestFile(netConfig.tftpServer, netConfig.tftpFile);

  // Main loop
  bootloaderExitTime = millis() + BOOTLOADER_MAX_RUN_TIME;
  while (1) {
    if (tftpRun()) {
      led_pulse_rate = 4; // 4x second is active TFTP
      bootloaderExitTime = millis() + BOOTLOADER_MAX_RUN_TIME;
    }

    if (exitBootloaderAfterTimeout && (millis() > bootloaderExitTime)) {
      LOG("No TFTP response, booting");
      startApplication();
    }
  }
}

void SysTick_Handler(void)
{
  ++tickCount;

  for(uint8_t i=0;i<led_pulse_rate;i++) {
    LED_pulse();
  }
}

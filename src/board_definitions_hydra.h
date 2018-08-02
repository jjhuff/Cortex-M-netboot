/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.
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

#ifndef _BOARD_DEFINITIONS_H_
#define _BOARD_DEFINITIONS_H_

/*
 * USB device definitions
 */
#define STRING_MANUFACTURER "Blokable, Inc"
#define STRING_PRODUCT      "Hydra"

#define USB_VID_HIGH   0x2E
#define USB_VID_LOW    0x78
#define USB_PID_HIGH   0x00
#define USB_PID_LOW    0x4D

/*
 * REBOOT_STATUS_ADDRESS must point to a free SRAM cell that must not
 * be touched from the loaded application.
 */
#define REBOOT_STATUS_ADDRESS  (0x20007FFCul)
#define REBOOT_STATUS_VALUE    (*((volatile uint32_t *) REBOOT_STATUS_ADDRESS))

/*
 * REBOOT_STATUS_UNDEFINED
 *    No specific action defined for the next warm reset.  This status is
 *    assagned right after a cold reset or when no behaviour after the next warm
 *    reset has been defined yet
 *
 * REBOOT_STATUS_DOUBLE_TAP_MAGIC
 *    Quick two times tapping of the reset button detected: run the bootloader
 *    until a successful programming of the FLASH memory has been completed
 *
 * REBOOT_STATUS_START_APP
 *    Start the application right after the warm reset
 */
#define REBOOT_STATUS_UNDEFINED        (0)
#define REBOOT_STATUS_DOUBLE_TAP_MAGIC (0x07738135)
#define REBOOT_STATUS_START_APP        (0x63313669)


/* Master clock frequency */
#define CPU_FREQUENCY                     (48000000ul)
#define VARIANT_MCK                       CPU_FREQUENCY

/* Frequency of the board main oscillator */
#define VARIANT_MAINOSC                   (32768ul)

/* Calibration values for DFLL48 pll */
#define NVM_SW_CALIB_DFLL48M_COARSE_VAL   (58)
#define NVM_SW_CALIB_DFLL48M_FINE_VAL     (64)

/*
 * LEDs definitions
 */
#define BOARD_LED_PORT                    (1)
#define BOARD_LED_PIN                     (8)

/*
 * SPI definitions and configuration
 */

#define SPI_SERCOM                        SERCOM0
#define SPI_PM_APBCMASK                   PM_APBCMASK_SERCOM0
#define SPI_PER_CLOCK_ID                  GCLK_CLKCTRL_ID_SERCOM0_CORE_Val

// See page 493: https://cdn.sparkfun.com/datasheets/Dev/Arduino/Boards/Atmel-42181-SAM-D21_Datasheet.pdf
#define SPI_OUTPUT_PINOUT                 (3U)          // PAD0=MOSI, PAD3=SCK
#define SPI_INPUT_PINOUT                  (1U)          // PAD1=MISO

// These come from CMSIS-Atmel/CMSIS/CMSIS/Device/ATMEL/samd21/include/pio/samd21g18a.h
#define SPI_MISO                          PINMUX_PA09C_SERCOM0_PAD1
#define SPI_MOSI                          PINMUX_PA08C_SERCOM0_PAD0
#define SPI_SCK                           PINMUX_PA11C_SERCOM0_PAD3

#define SPI_BITRATE                       (4000000UL)     // 4 MHz
#define SPI_MAX_BITRATE                   (12000000UL)    // 12 MHz max, see 'SPI.h', SPI_MIN_CLOCK_DIVIDER

/*
 * Ethernet module/chip definitions
 */

// Wiznet Ethernet chip CS pin
#define W5X00_CS_PORT                     (0U)   // Port A
#define W5X00_CS_PIN                      (10U)   // PA10

#endif // _BOARD_DEFINITIONS_H_

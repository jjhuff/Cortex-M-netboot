//  Minimal SPI driver
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
//  (code adapted from Arduino SAMD core)

#include <stdbool.h>
#include <sam.h>
#include "spi.h"
#include "board_definitions.h"

#define SERCOM_FREQ_REF (48000000)   // See 'SERCOM.h'

static bool spiInitialized = false;

// This comes from http://ww1.microchip.com/downloads/en/AppNotes/00002465A.pdf
static void setPeripheralPinMux(uint32_t pinmux) {
  uint8_t port = (uint8_t)((pinmux >> 16)/32);
  PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
            16) & 0x01u)));
  PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
        0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
  PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
}

static void spiReset (void) {
  SPI_SERCOM->SPI.CTRLA.bit.SWRST = 1;
  while (SPI_SERCOM->SPI.CTRLA.bit.SWRST || SPI_SERCOM->SPI.SYNCBUSY.bit.SWRST);

  spiInitialized = false;
}

void spiInit (uint32_t bitrate) {
  if (spiInitialized)
    return;

  if (bitrate > SPI_MAX_BITRATE) {
    bitrate = SPI_MAX_BITRATE;
  }

  setPeripheralPinMux(SPI_MISO);
  setPeripheralPinMux(SPI_SCK);
  setPeripheralPinMux(SPI_MOSI);

  // Disable SPI
  SPI_SERCOM->SPI.CTRLA.bit.ENABLE = 0;
  while (SPI_SERCOM->SPI.SYNCBUSY.bit.ENABLE);

  spiReset();

  // Setting clock
  PM->APBCMASK.reg |= SPI_PM_APBCMASK;
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(SPI_PER_CLOCK_ID) |
    GCLK_CLKCTRL_GEN_GCLK0 |
    GCLK_CLKCTRL_CLKEN;
  while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

  // Peripheral configuration
  SPI_SERCOM->SPI.CTRLA.reg = SERCOM_SPI_CTRLA_MODE_SPI_MASTER |
    SERCOM_SPI_CTRLA_DOPO(SPI_OUTPUT_PINOUT) |
    SERCOM_SPI_CTRLA_DIPO(SPI_INPUT_PINOUT);
  while(SPI_SERCOM->SPI.SYNCBUSY.bit.CTRLB);

  SPI_SERCOM->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;
  while(SPI_SERCOM->SPI.SYNCBUSY.bit.CTRLB);

  SPI_SERCOM->SPI.BAUD.reg = SERCOM_FREQ_REF / (2 * bitrate) - 1;

  // Enable SPI
  SPI_SERCOM->SPI.CTRLA.bit.ENABLE = 1;
  while (SPI_SERCOM->SPI.SYNCBUSY.bit.ENABLE);

  // See 'Atmel-42181N-SAM-D21_Datasheet_Complete-10/2016', page 519, Bit 17 - RXEN
  while (SPI_SERCOM->SPI.SYNCBUSY.bit.CTRLB);
  while (!SPI_SERCOM->SPI.CTRLB.bit.RXEN);

  spiInitialized = true;
}

void spiEnd (void) {
  spiReset();
}

uint8_t spiTransfer (uint8_t data) {
  SPI_SERCOM->SPI.DATA.bit.DATA = data;

  while (SPI_SERCOM->SPI.INTFLAG.bit.RXC == 0);

  return SPI_SERCOM->SPI.DATA.bit.DATA;
}

void spiTransferBytes (uint8_t *data, uint16_t size) {
  while (size--)
  {
    SPI_SERCOM->SPI.DATA.bit.DATA = *(data++);

    while (SPI_SERCOM->SPI.INTFLAG.bit.RXC == 0);
  }
}

uint16_t spiTransfer16 (uint16_t data) {
  uint8_t msb = spiTransfer(data >> 8);
  uint8_t lsb = spiTransfer(data & 0x00FF);
  return msb << 8 | lsb;
}

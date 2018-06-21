//  Minimal Wiznet W5x00 driver
//  Copyright (C) 2017  Industruino <connect@industruino.com>
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
//
//  Code adapted from Ariadne bootloader (https://github.com/codebendercc/Ariadne-Bootloader)
//  Original copyright:
//     Author: .
//     Copyright: Arduino
//     License: GPL http://www.gnu.org/licenses/gpl-2.0.html
//     Project: eboot
//     Function: Network initialization
//     Version: 0.1 tftp / flashing functional
//
//  Wiznet W5500 differentiation code adapted from https://github.com/jbkim/Differentiate-WIznet-Chip

#include "w5x00.h"
#include "spi.h"

#define W5X00_READ  (0x0F)
#define W5X00_WRITE (0xF0)

#define W5X00_GW_ADDR_POS  1
#define W5X00_NETMASK_POS  5
#define W5X00_MAC_ADDR_POS 9
#define W5X00_IP_ADDR_POS  15

#define W5X00_REGISTER_BLOCK_SIZE 28
uint8_t registerBuffer[W5X00_REGISTER_BLOCK_SIZE] =
{
  0x80,   // MR Mode - reset device

  0, 0, 0, 0,         // GWR Gateway IP Address Register
  0, 0, 0, 0,         // SUBR Subnet Mask Register
  0, 0, 0, 0, 0, 0,   // SHAR Source Hardware Address Register
  0, 0, 0, 0,         // SIPR Source IP Address Register

  0, 0,         // Interrupt Low Level Timer (INTLEVEL0), (INTLEVEL1) (0x0013, 0x0014)
  0,            // IR Interrupt Register (0x0015)
  0,            // IMR Interrupt Mask Register (0x0016)
  0,            // Socket Interrupt (SIR) (0x0017)
  0,            // Socket Interrupt Mask (SIMR) (0x0018)
  0x07, 0xD0,   // RTR Retry Time-value Register ((RTR0),(RTR0)) (0x0019,0x001A)
  0x08          // RCR Retry Count Register (0x001B)
};

#define W5X00_INIT_CS     PORT->Group[W5X00_CS_PORT].DIRSET.reg=(1<<W5X00_CS_PIN)
#define W5X00_END_CS      PORT->Group[W5X00_CS_PORT].DIRCLR.reg=(1<<W5X00_CS_PIN)
#define W5X00_ASSERT_CS   PORT->Group[W5X00_CS_PORT].OUTCLR.reg=(1<<W5X00_CS_PIN)
#define W5X00_DEASSERT_CS PORT->Group[W5X00_CS_PORT].OUTSET.reg=(1<<W5X00_CS_PIN)


bool w5x00Init() {
  W5X00_INIT_CS;

  W5X00_ASSERT_CS;

  // Adapted form https://github.com/jbkim/Differentiate-WIznet-Chip
  spiTransfer(0x00);
  spiTransfer(0x39);
  spiTransfer(0x00);
  bool chipAvailable = (spiTransfer(0x00) == 0x04);

  W5X00_DEASSERT_CS;

  if (!chipAvailable) {
    return false;
  }

  // Soft reset
  w5x00WriteReg(REG_MR, GP_W_CB, REG_MR_RESET);

  // Wait for reset to complete
  while (w5x00ReadReg(REG_MR, GP_R_CB) && REG_MR_RESET);

  return true;
}

void w5x00End (void) {
  W5X00_END_CS;
}

uint8_t w5x00ReadReg (uint16_t address, uint8_t cb) {
  uint8_t receivedByte;

  W5X00_ASSERT_CS;

  spiTransfer16(address);

  spiTransfer(cb);

  receivedByte = spiTransfer(0U);

  W5X00_DEASSERT_CS;

  return receivedByte;
}

uint16_t w5x00ReadWord (uint16_t address, uint8_t cb) {
  return ((uint16_t)(w5x00ReadReg(address, cb)) << 8) | (uint16_t)(w5x00ReadReg(address + 1, cb));
}

void w5x00WriteReg (uint16_t address, uint8_t cb, uint8_t value) {
  W5X00_ASSERT_CS;

  spiTransfer16(address);

  spiTransfer(cb);

  spiTransfer(value);

  W5X00_DEASSERT_CS;
}

void w5x00WriteWord (uint16_t address, uint8_t cb, uint16_t value) {
  w5x00WriteReg(address, cb, (uint8_t)(value >> 8));
  w5x00WriteReg(++address, cb, (uint8_t)(value & 0xff));
}

void w5x00WriteBuffer(uint16_t address, uint8_t cb, const uint8_t* buf, uint8_t len) {
  for (uint8_t i=0; i<len; i++) {
    w5x00WriteReg(address + i, cb, buf[i]);
  }
}

void w5x00Dump(uint8_t cb) {
#if DEBUG
  for (uint8_t i = 0; i < 0x2F ; i++) {
    if (i % 16 == 0) {
      if (i > 0) {
        LOG_STR("\r\n");
      }

      LOG_HEX_BYTE(i);
      LOG_STR(": ");
    }
    uint8_t v = w5x00ReadReg(i, cb);
    LOG_HEX_BYTE(v);
    LOG_STR(" ");
  }
  LOG_STR("\r\n");
#endif
}

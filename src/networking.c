//  Basic networking functions
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
//
//  Code adapted from Ariadne bootloader (https://github.com/codebendercc/Ariadne-Bootloader)
//  Original copyright:
//     Author: .
//     Copyright: Arduino
//     License: GPL http://www.gnu.org/licenses/gpl-2.0.html
//     Project: eboot
//     Function: Network initialization
//     Version: 0.1 tftp / flashing functional

#include <string.h>
#include "networking.h"
#include "spi.h"
#include "w5x00.h"
#include "utils.h"
#include "log.h"
#include "board_driver_i2c.h"

// Declare the network settings
netConfig_t netConfig = {
  .macAddr = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02},
  .ipAddr = {0},
  .netMask = {0},
  .gwAddr = {0},
  .tftpServer = {0},
  .tftpFile = "firmware.bin",
};

#ifdef MAC_CHIP_ADDRESS
static void readMACAddress() {
  // Read the MAC (6 bytes) from address 0xFA
  // See page 14 of http://ww1.microchip.com/downloads/en/DeviceDoc/24AA02E48-24AA025E48-24AA02E64-24AA025E64-Data-Sheet-20002124H.pdf
  LOG_STR("Read MAC address...");

  i2c_beginTransmission(MAC_CHIP_ADDRESS);
  i2c_write(MAC_CHIP_READ_OFFSET);
  uint8_t status = i2c_endTransmission(false);

  if (!status) {
    i2c_read(MAC_CHIP_ADDRESS, 6, netConfig.macAddr, true);
    LOG("success");
  } else {
    LOG("failed");
  }
}
#endif

bool netInit() {
  spiInit(SPI_BITRATE);

  if (!w5x00Init()) {
    spiEnd();
    return false;
  }
#ifdef MAC_CHIP_ADDRESS
  readMACAddress();
#endif

#if DEBUG
  LOG_STR("MAC Address: ");
  for (uint8_t i=0;i<6;i++) {
    if (i>0) {
      LOG_STR(":");
    }
    LOG_HEX_BYTE(netConfig.macAddr[i]);
  }
  LOG_STR("\r\n")
#endif
  // Write MAC address
  w5x00WriteBuffer(REG_SHAR0, GP_W_CB, netConfig.macAddr, 6);

  // Assign 2KB RX and TX memory per socket
  for (uint8_t idx = 0; idx < 8; ++idx) {
    uint8_t controlByte = (0x0C + (idx << 5));
    w5x00WriteReg(0x1E, controlByte, 2);   //0x1E - Sn_RXBUF_SIZE
    w5x00WriteReg(0x1F, controlByte, 2);   //0x1F - Sn_TXBUF_SIZE
  }

  return true;
}

void netCommitConfig() {
  w5x00WriteBuffer(REG_SIPR0, GP_W_CB, netConfig.ipAddr, 4);
  w5x00WriteBuffer(REG_SUBR0, GP_W_CB, netConfig.netMask, 4);
  w5x00WriteBuffer(REG_GAR0,  GP_W_CB, netConfig.gwAddr, 4);
}

void netEnd (void) {
  w5x00End();

  spiEnd();
}

void netCloseSocket3 (void) {
  // Close the socket
  w5x00WriteReg(REG_S3_CR, S3_W_CB, CR_CLOSE);

  // Wait for command to complete
  while (w5x00ReadReg(REG_S3_CR, S3_R_CB));
}

void netOpenUdpSocket3 (uint16_t port) {
  netCloseSocket3();

  // Clear the socket interrupt register
  w5x00WriteReg(REG_S3_IR, S3_W_CB, 0xFF);
  // Set UDP mode
  w5x00WriteReg(REG_S3_MR, S3_W_CB, MR_UDP);
  // Set the socket port
  w5x00WriteWord(REG_S3_PORT0, S3_W_CB, port);

  // Open Socket
  w5x00WriteReg(REG_S3_CR, S3_W_CB, CR_OPEN);

  // Wait for socket to be opened
  while (w5x00ReadReg(REG_S3_SR, S3_R_CB) != SOCK_UDP);
}

static uint16_t netReceivedDataSizeSocket3(void) {
  // This is from https://github.com/sstaub/Ethernet3/blob/d2b7dc0efcddfd9d7c7bd07b8c131a240cd5f148/src/utility/w5500.cpp#L93
  uint16_t val=0,val1=0;
  do {
    val1 = w5x00ReadWord(REG_S3_RX_RSR0, S3_R_CB);
    if (val1 != 0)
      val = w5x00ReadWord(REG_S3_RX_RSR0, S3_R_CB);
  }
  while (val != val1);
  return val;
}

static void netReadBufferSocket3(uint16_t* readPointer, uint8_t* buffer, uint16_t len) {
  for(uint16_t i = 0; i < len; i++) {
    buffer[i] = w5x00ReadReg((*readPointer)++, S3_RXBUF_CB);
  }
}

uint16_t netReceivePacketSocket3(uint8_t* buffer, uint8_t* fromAddr, uint16_t* fromPort) {
  // Get packet size
  uint16_t packetSize = netReceivedDataSizeSocket3();
  if (packetSize == 0) {
    return 0;
  }

  // Get the current read pointer
  uint16_t readPointer = w5x00ReadWord(REG_S3_RX_RD0, S3_R_CB);

  // Read UDP header
  uint8_t head[8];
  netReadBufferSocket3(&readPointer, head, sizeof(head));

  if (fromAddr) {
    memcpy(fromAddr, head, 4);
  }

  if (fromPort) {
    *fromPort = (head[4] << 8) + head[5];
  }

  uint16_t dataSize = (head[6] << 8) + head[7];

  netReadBufferSocket3(&readPointer, buffer, dataSize);

  // Send the new pointer value back to the chip
  w5x00WriteWord(REG_S3_RX_RD0, S3_W_CB, readPointer);

  w5x00WriteReg(REG_S3_CR, S3_W_CB, CR_RECV);

  while (w5x00ReadReg(REG_S3_CR, S3_R_CB));

  return dataSize;
}

void netBeginPacketSocket3(const uint8_t address[4], uint16_t port) {
  for (uint8_t i = 0; i < 4; ++i) {
    w5x00WriteReg(REG_S3_DIPR0 + i, S3_W_CB, address[i]);
  }
  w5x00WriteWord(REG_S3_DPORT0, S3_W_CB, port);
}

void netWriteSocket3(const uint8_t *data, uint16_t size) {
  uint16_t writePointer = w5x00ReadWord(REG_S3_TX_WR0, S3_R_CB);

  while (size--)
  {
    w5x00WriteReg(writePointer++, S3_TXBUF_CB, *data++);
    // W5500 auto increments the readpointer by memory mapping a 16 bit address
    // Use uint16_t overflow from 0xFFFF to 0x10000 to follow W5500 internal pointer
  }
  w5x00WriteWord(REG_S3_TX_WR0, S3_W_CB, writePointer);
}

void netEndPacketSocket3() {
  // Transmit the data
  w5x00WriteReg(REG_S3_CR, S3_W_CB, CR_SEND);

  // Wait for transmission to complete
  while (w5x00ReadReg(REG_S3_CR, S3_R_CB));
}


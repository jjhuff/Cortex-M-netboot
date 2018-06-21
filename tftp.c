//  Partial TFTP implementation
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
//     Project: ethboot
//     Function: tftp implementation and flasher
//     Version: 0.2 tftp / flashing functional

#include <string.h>
#include <stddef.h>
#include "tftp.h"
#include "networking.h"
#include "utils.h"
#include "log.h"
#include "flash.h"

#define TFTP_PORT ((uint16_t) 69)
#define TFTP_PORT_LOCAL ((uint16_t) 69)

// TFTP Opcode values from RFC 1350
#define TFTP_OPCODE_RRQ   ((uint16_t) 1)
#define TFTP_OPCODE_WRQ   ((uint16_t) 2)
#define TFTP_OPCODE_DATA  ((uint16_t) 3)
#define TFTP_OPCODE_ACK   ((uint16_t) 4)
#define TFTP_OPCODE_ERROR ((uint16_t) 5)

#define TFTP_ERROR_DISK_FULL ((uint16_t) 3)

#define TFTP_MAX_PAYLOAD     (512)

static uint16_t nextBlockNumber;
static uint8_t tftpServer[4];

void tftpInit (void) {
  netOpenUdpSocket3(TFTP_PORT_LOCAL);
}

void tftpEnd (void) {
  netCloseSocket3();
}

static uint8_t* appendString(uint8_t* ptr, const char* str)  {
  size_t fileLen = strlen(str) + 1; // Include the null terminator
  memcpy(ptr, str, fileLen);
  ptr += fileLen;
  return ptr;
}

static uint8_t* appendUint16(uint8_t* ptr, uint16_t val)  {
  *(ptr++) = val >> 8;
  *(ptr++) = val & 0xff;
  return ptr;
}

void tftpRequestFile(const uint8_t destIP[4], const char* file) {
  uint8_t txBuffer[100];
  uint8_t* txPtr = txBuffer;

  memcpy(tftpServer, destIP, 4);

  // Start with opcode
  txPtr = appendUint16(txPtr, TFTP_OPCODE_RRQ);

  // File name
  txPtr = appendString(txPtr, file);

  // Mode
  txPtr = appendString(txPtr, "octet");

  // Reset nextBlockNumber
  nextBlockNumber = 1;

  // Reset flashing
  flash_init();

  netBeginPacketSocket3(destIP, TFTP_PORT);
  netWriteSocket3(txBuffer, txPtr - txBuffer);
  netEndPacketSocket3();
}

static void tftpSendACK(uint16_t blockNum) {
  uint8_t txBuffer[4];
  uint8_t* txPtr = txBuffer;

  // Start with opcode
  txPtr = appendUint16(txPtr, TFTP_OPCODE_ACK);

  // Block number to ACK
  txPtr = appendUint16(txPtr, blockNum);

  netWriteSocket3(txBuffer, txPtr - txBuffer);
}

static void tftpSendERROR(uint16_t code) {
  uint8_t txBuffer[5];
  uint8_t* txPtr = txBuffer;

  // Start with opcode
  txPtr = appendUint16(txPtr, TFTP_OPCODE_ERROR);

  // Error
  txPtr = appendUint16(txPtr, code);

  // No message string, but we still need the terminator
  *(txPtr++) = 0;

  netWriteSocket3(txBuffer, txPtr - txBuffer);
}

bool tftpRun (void) {
  uint8_t buffer[TFTP_MAX_PAYLOAD];
  uint8_t* bufferPtr = buffer;

  // Get the packet
  uint8_t fromAddr[4];
  uint16_t fromPort;
  uint16_t bufferLen = netReceivePacketSocket3(buffer, fromAddr, &fromPort);
  if (bufferLen == 0) {
    return false;
  }

  // Get the opcode
  uint16_t tftpOpcode = (bufferPtr[0] << 8) + bufferPtr[1];
  bufferPtr+=2;
  bufferLen-=2;

  switch (tftpOpcode)
  {
    case TFTP_OPCODE_DATA:
      {
        uint16_t tftpBlockNumber = (bufferPtr[0] << 8) + bufferPtr[1];
        bufferPtr += 2;
        bufferLen -= 2;

        if (tftpBlockNumber < nextBlockNumber) {
          // ACK the prior data block since this appears to be a retransmit
          netBeginPacketSocket3(tftpServer, fromPort);
          tftpSendACK(tftpBlockNumber);
          netEndPacketSocket3();
          break;
        } else if (tftpBlockNumber > nextBlockNumber) {
          // Ignore newer packets
          // TODO: should they be NACKed or ERRed?
          break;
        }
        nextBlockNumber = tftpBlockNumber + 1;

        LOG_STR("TFTP DATA: ");
        LOG_HEX(tftpBlockNumber);
        LOG_STR(" ");
        LOG_HEX(bufferLen);
        LOG_STR("\r\n");

        // Setup for sending to the tftp server
        netBeginPacketSocket3(tftpServer, fromPort);

        // Write to flash
        if (flash_tftp_buffer(bufferPtr, bufferLen)) {
          // ACK the data block
          tftpSendACK(tftpBlockNumber);
        } else {
          // Write failed, so let's call that 'disk full'
          tftpSendERROR(TFTP_ERROR_DISK_FULL);
        }

        netEndPacketSocket3();

        // A smaller than max payload means we're done with the transfer
        if (bufferLen < TFTP_MAX_PAYLOAD) {
          LOG("TFTP DONE");
          startApplication();
        }
        break;
      }

#if DEBUG
    case TFTP_OPCODE_ERROR:
      {
        uint16_t tftpErrorCode = (bufferPtr[0] << 8) + bufferPtr[1];
        bufferPtr += 2;
        bufferLen -= 2;
        LOG_STR("ERROR: ");
        LOG_HEX(tftpErrorCode);
        LOG_STR("\r\n");
        break;
      }
#endif
  }

  return true;
}


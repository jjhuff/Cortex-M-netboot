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

#ifndef __NETWORKING_H__
#define __NETWORKING_H__

#include <stdbool.h>
#include <sam.h>

bool netInit();
void netEnd();

void netCommitConfig();

void netOpenUdpSocket3(uint16_t port);
void netCloseSocket3(void);

// Receiving packets
uint16_t netReceivePacketSocket3(uint8_t* buffer, uint8_t* fromAddr, uint16_t* fromPort);

// Sending Packets
void netBeginPacketSocket3(const uint8_t address[4], uint16_t port);
void netWriteSocket3(const uint8_t *data, uint16_t size);
void netEndPacketSocket3();

typedef struct {
  uint8_t macAddr[6];
  uint8_t ipAddr[4];
  uint8_t netMask[4];
  uint8_t gwAddr[4];
  uint8_t tftpServer[4];
  char tftpFile[128];
} netConfig_t;

extern netConfig_t netConfig;

// Copy an IP address
#define COPY_IP(dest, src) for(uint8_t i = 0; i < 4; i++) { dest[i] = src[i]; }

// Byteorder swapping
#define htons(x) ( ((x)<< 8 & 0xFF00) | \
                   ((x)>> 8 & 0x00FF) )
#define ntohs(x) htons(x)

#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
#define ntohl(x) htonl(x)

#endif   // __NETWORKING_H__

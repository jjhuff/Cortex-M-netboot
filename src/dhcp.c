// Basic DHCP implementation
// Copyright (c) 2018 Blokable, Inc All rights reserved
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
// Uses/inspired by https://github.com/sstaub/Ethernet3

#include <string.h>

#include "networking.h"
#include "utils.h"
#include "log.h"
#include "dhcp.h"

#define DHCP_FLAGSBROADCAST  0x8000

/* UDP port numbers for DHCP */
#define DHCP_SERVER_PORT    67  /* from server to client */
#define DHCP_CLIENT_PORT    68  /* from client to server */

/* DHCP message OP code */
#define DHCP_BOOTREQUEST    1
#define DHCP_BOOTREPLY      2

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

#define DHCP_HTYPE10MB      1

#define DHCP_HLENETHERNET   6
#define DHCP_SECS           0

#define MAGIC_COOKIE        0x63825363

// DHCP Options
enum
{
  padOption             = 0,
  subnetMask            = 1,
  timerOffset           = 2,
  routersOnSubnet       = 3,
  dns                   = 6,
  hostName              = 12,
  domainName            = 15,
  broadcastAddr         = 28,
  dhcpRequestedIPaddr   = 50,
  dhcpIPaddrLeaseTime   = 51,
  dhcpMessageType       = 53,
  dhcpServerIdentifier  = 54,
  dhcpParamRequest      = 55,
  dhcpMaxMsgSize        = 57,
  dhcpT1value           = 58,
  dhcpT2value           = 59,
  dhcpClientIdentifier  = 61,
  nextServerName        = 66,
  bootFileName          = 67,
  endOption             = 255
};


// These names match the names in RFC 951
#pragma pack(1)
typedef struct {
  uint8_t op;         // packet opcode type
  uint8_t htype;      // hardware addr type
  uint8_t hlen;       // hardware addr length
  uint8_t hops;       // gateway hops
  uint32_t xid;       // transaction ID
  uint16_t secs;      // seconds since boot began
  uint16_t flags;     // bit 16 = Broadcast
  uint8_t ciaddr[4];  // client IP address (this is the IP that you may already have)
  uint8_t yiaddr[4];  // 'your' IP address (this is the IP that the server offers you)
  uint8_t siaddr[4];  // server IP address
  uint8_t giaddr[4];  // gateway IP address
  uint8_t chaddr[16]; // client hardware address
  char srname[64];    // server host name
  char file[128];     // boot file name
  uint32_t magic;     // Magic cookie to identify this as DHCP
  uint8_t opts[60];   // Options
} dhcpPacket;

static uint32_t _dhcpTransactionId;
static uint32_t _dhcpStartTime;
static uint8_t _dhcpServerIdentifier[4];

typedef enum {
  DHCP_STATE_START,
  DHCP_STATE_DISCOVER,
  DHCP_STATE_REQUEST,
  DHCP_STATE_LEASED,
} dhcpStateType;

static dhcpStateType _dhcpState;

static void dhcpSendMessage(uint8_t messageType) {
  const uint8_t destIP[] = {255, 255, 255, 255};
  netBeginPacketSocket3(destIP, DHCP_SERVER_PORT);

  const uint16_t secondsElapsed = (millis() - _dhcpStartTime) / 1000;

  // Build DHCP packet
  dhcpPacket packet = {
    .op = DHCP_BOOTREQUEST,
    .htype = DHCP_HTYPE10MB,
    .hlen = DHCP_HLENETHERNET,  //  IEEE 802 MAC length
    .hops = 0,                  // Number of gateway hops
    .xid = htonl(_dhcpTransactionId),
    .secs = htons(secondsElapsed),
    .flags = htons(DHCP_FLAGSBROADCAST),
    .magic = htonl(MAGIC_COOKIE),
  };

  // Set the client's MAC address
  memcpy(packet.chaddr, netConfig.macAddr, 6);

  uint8_t* opts = packet.opts;

  // OPT - message type
  *(opts++) = dhcpMessageType;
  *(opts++) = 0x01; // Length
  *(opts++) = messageType;

  // OPT - client identifier
  *(opts++) = dhcpClientIdentifier;
  *(opts++) = 0x07; // length
  *(opts++) = DHCP_HTYPE10MB;
  memcpy(opts, netConfig.macAddr, 6);
  opts += 6;

  if(messageType == DHCP_REQUEST)
  {
    *(opts++) = dhcpRequestedIPaddr;
    *(opts++) = 0x04; // Length
    COPY_IP(opts, netConfig.ipAddr);
    opts+=4;

    *(opts++) = dhcpServerIdentifier;
    *(opts++) = 0x04; // Length
    COPY_IP(opts, _dhcpServerIdentifier);
    opts+=4;
  }

  // DHCP Param request
  *(opts++) = dhcpParamRequest;
  *(opts++) = 0x02; // Length
  *(opts++) = subnetMask;
  *(opts++) = routersOnSubnet;

  // End marker
  *(opts++) = endOption;

  netWriteSocket3((uint8_t*)&packet, sizeof(packet));

  netEndPacketSocket3();
}

static uint8_t dhcpParsePacket() {
  dhcpPacket packet;
  uint8_t serverAddr[4];
  uint16_t bufferLen = netReceivePacketSocket3((uint8_t*)&packet, serverAddr, NULL);
  if (bufferLen == 0) {
    return 0;
  }

  if (packet.op != DHCP_BOOTREPLY) {
    LOG("DHCP: invalid packet op");
    return 0;
  }

  if (packet.magic !=  htonl(MAGIC_COOKIE)) {
    LOG("DHCP: invalid packet magic cookie");
    return 0;
  }

  if (_dhcpTransactionId != ntohl(packet.xid)) {
    LOG("DHCP: invalid transaction id. Expecting: ");
    LOG_HEX(_dhcpTransactionId);
    LOG_STR(" got: ");
    LOG_HEX(ntohl(packet.xid));
    LOG_STR("\r\n");
    return 0;
  }

  if (memcmp(packet.chaddr, netConfig.macAddr, 6) != 0) {
    LOG("DHCP: hardware address mismatch");
    return 0;
  }

  COPY_IP(netConfig.ipAddr, packet.yiaddr);

  // Only copy tftp info if the dhcp server returned a file
  if (strlen(packet.file) > 0) {
    COPY_IP(netConfig.tftpServer, packet.siaddr);
    memcpy(netConfig.tftpFile, packet.file, 128);
  } else {
    // Otherwise, use the DHCP server itself
    COPY_IP(netConfig.tftpServer, serverAddr);
  }

  uint8_t messageType = 0;

  uint8_t* opts = packet.opts;
  while (opts < (opts + sizeof(packet.opts))) {
    uint8_t optType = *(opts++);

    if (optType == endOption) {
      break;
    } else if (optType != padOption) {
      uint8_t optLen = *(opts++);
      switch(optType) {
        case dhcpMessageType:
          messageType = *opts;
          break;

        case subnetMask:
          COPY_IP(netConfig.netMask, opts);
          break;

        case routersOnSubnet:
          COPY_IP(netConfig.gwAddr, opts)
          break;

        case dhcpServerIdentifier:
          COPY_IP(_dhcpServerIdentifier, opts);
          break;

        case bootFileName:
          memcpy(netConfig.tftpFile, opts, optLen);
          break;
      }
      opts += optLen;
    }
  }

  return messageType;
}



void dhcpInit() {
  _dhcpState = DHCP_STATE_START;

  netOpenUdpSocket3(DHCP_CLIENT_PORT);

  // Use a 32bit value derived from the 128bit serial number as the transaction id
  _dhcpTransactionId = getDeviceSerialNumber32();
  _dhcpStartTime = millis();
}

void dhcpEnd(void) {
  netCloseSocket3();
}

bool dhcpRun() {
  switch(_dhcpState) {
    case DHCP_STATE_START:
      {
        LOG("DHCP: STATE_START");
        _dhcpTransactionId++;
        dhcpSendMessage(DHCP_DISCOVER);
        _dhcpState = DHCP_STATE_DISCOVER;
        break;
      }
    case DHCP_STATE_DISCOVER:
      {
        uint8_t messageType = dhcpParsePacket();
        if (messageType == DHCP_OFFER) {
          LOG("DHCP: STATE_DISCOVER got OFFER");
          _dhcpTransactionId++;
          dhcpSendMessage(DHCP_REQUEST);
          LOG("dhcpSendMessage done");
          _dhcpState = DHCP_STATE_REQUEST;
        }
        break;
      }
    case DHCP_STATE_REQUEST:
      {
        uint8_t messageType = dhcpParsePacket();
        if (messageType == DHCP_ACK) {
        LOG("DHCP: STATE_REQUEST got ACK");
          _dhcpState = DHCP_STATE_LEASED;
        }
        break;
      }
    case DHCP_STATE_LEASED:
      {
        LOG("DHCP: STATE_LEASED");
        netCommitConfig();
        return true;
        break;
      }
  }

  return false;
}


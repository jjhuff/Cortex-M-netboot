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

#ifndef __TFTP_H__
#define __TFTP_H__

#include <stdint.h>
#include <stdbool.h>

void tftpInit (void);
void tftpEnd (void);
bool tftpRun (void);

void tftpRequestFile(const uint8_t destIP[4], const char* file);

#endif   // __TFTP_H__

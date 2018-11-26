# Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.
# Copyright (c) 2015 Arduino LLC.  All right reserved.
# Copyright (c) 2018 Blokable, Inc All rights reserved
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# -----------------------------------------------------------------------------
# Paths
RM=rm

#ARM_GCC_PATH?=/usr/local/bin/gcc-arm-none-eabi-4_8-2014q1/bin/arm-none-eabi-
ARM_GCC_PATH?=/usr/local/bin/gcc-arm-none-eabi-5_4-2016q3/bin/arm-none-eabi-

BUILD_PATH=build

# -----------------------------------------------------------------------------
# Tools
CC=$(ARM_GCC_PATH)gcc
OBJCOPY=$(ARM_GCC_PATH)objcopy
NM=$(ARM_GCC_PATH)nm
SIZE=$(ARM_GCC_PATH)size

# -----------------------------------------------------------------------------
# Boards definitions
BOARD_ID?=feather_m0

TFTP_DEFAULT?=$(BOARD_ID).bin

# -----------------------------------------------------------------------------
# Compiler options
CFLAGS_EXTRA=-D__SAMD21G18A__ -DBOARD_ID_$(BOARD_ID) -DTFTP_DEFAULT=$(TFTP_DEFAULT)
CFLAGS=-mthumb -mcpu=cortex-m0plus -Wall -c -std=gnu99 -ffunction-sections -fdata-sections -nostdlib -nostartfiles --param max-inline-insns-single=500
ifdef DEBUG
	CFLAGS+=-g3 -O1 -DDEBUG=1
else
	CFLAGS+=-Os -DDEBUG=0
endif

NAME?=$(BOARD_ID)
ELF=$(BUILD_PATH)/$(NAME).elf
BIN=$(BUILD_PATH)/$(NAME).bin
HEX=$(BUILD_PATH)/$(NAME).hex


# Include CMSIS and CMSIS-Atmel from submodules
INCLUDES=-I"CMSIS/CMSIS/Include/" -I"CMSIS-Atmel/CMSIS/CMSIS/Device/ATMEL/"

# -----------------------------------------------------------------------------
# Linker options
LDFLAGS=-mthumb -mcpu=cortex-m0plus -Wall -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LDFLAGS+=--specs=nano.specs --specs=nosys.specs

# -----------------------------------------------------------------------------
# Source files and objects
SOURCES= \
				 src/board_driver_led.c \
				 src/board_driver_i2c.c \
				 src/board_driver_usb.c \
				 src/board_init.c \
				 src/board_startup.c \
				 src/main.c \
				 src/networking.c \
				 src/sam_ba_usb.c \
				 src/sam_ba_cdc.c \
				 src/flash.c \
				 src/log.c \
				 src/spi.c \
				 src/tftp.c \
				 src/dhcp.c \
				 src/utils.c \
				 src/w5x00.c

OBJECTS=$(addprefix $(BUILD_PATH)/, $(SOURCES:.c=.o))
DEPS=$(addprefix $(BUILD_PATH)/, $(SOURCES:.c=.d))

all: $(SOURCES) $(BIN) $(HEX)

$(ELF): Makefile $(BUILD_PATH) $(OBJECTS)
	"$(CC)" -L. -L$(BUILD_PATH) $(LDFLAGS) -Os -Wl,--gc-sections -save-temps -Tbootloader_samd21x18.ld -Wl,-Map,"$(BUILD_PATH)/$(NAME).map" -o "$@" -Wl,--start-group $(OBJECTS) -lm -Wl,--end-group
	"$(NM)" -n "$@" >"$(BUILD_PATH)/$(NAME)_symbols.txt"
	"$(SIZE)" --format=sysv -t -x $@
	"$(SIZE)" --format=sysv -t -d $@

$(BIN): $(ELF)
	"$(OBJCOPY)" -O binary $< $@

$(HEX): $(ELF)
	"$(OBJCOPY)" -O ihex $< $@

$(BUILD_PATH)/%.o: %.c
	"$(CC)" $(CFLAGS) $(CFLAGS_EXTRA) $(INCLUDES) $< -o $@

$(BUILD_PATH):
	-mkdir $(BUILD_PATH)
	-mkdir $(BUILD_PATH)/src

clean:
	-$(RM) -rf $(BUILD_PATH)

install: $(HEX)
	openocd -f openocd_jlink.cfg \
		-c "reset halt" \
		-c "at91samd bootloader 0" \
		-c "program $(HEX) verify" \
		-c "at91samd bootloader 16384" \
		-c "reset" \
		-c "exit"

.phony: clean install $(BUILD_PATH)

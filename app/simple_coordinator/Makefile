# Copyright (c) 2015, Dan Collins
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Project Name
PROJECT = simple_coordinator


# Sources
SRCS_C = \
	startup_gcc.c \
	simple_coordinator.c

# Include directories
INCLUDE = src


#
# WSN library
#
WSN_DIR := ../../lib
INCLUDE += $(WSN_DIR)/src


#
# Vendor libraries
#
ifndef TI_DIR
$(error "TI_DIR is not defined.")
endif

TI_INCLUDE = \
	driverlib/cc2538/inc \
	driverlib/cc2538/source \
	bsp/srf06eb_cc2538/drivers/source

TI_SRCS_C = \
	driverlib/cc2538/source/adc.c \
	driverlib/cc2538/source/aes.c \
	driverlib/cc2538/source/ccm.c \
	driverlib/cc2538/source/cpu.c \
	driverlib/cc2538/source/debug.c \
	driverlib/cc2538/source/flash.c \
	driverlib/cc2538/source/gpio.c \
	driverlib/cc2538/source/gptimer.c \
	driverlib/cc2538/source/i2c.c \
	driverlib/cc2538/source/interrupt.c \
	driverlib/cc2538/source/ioc.c \
	driverlib/cc2538/source/pka.c \
	driverlib/cc2538/source/sha256.c \
	driverlib/cc2538/source/sleepmode.c \
	driverlib/cc2538/source/ssi.c \
	driverlib/cc2538/source/sys_ctrl.c \
	driverlib/cc2538/source/systick.c \
	driverlib/cc2538/source/uart.c \
	driverlib/cc2538/source/udma.c \
	driverlib/cc2538/source/watchdog.c \
	bsp/srf06eb_cc2538/drivers/source/acc_srf06eb.c \
	bsp/srf06eb_cc2538/drivers/source/als_srf06eb.c \
	bsp/srf06eb_cc2538/drivers/source/bsp.c \
	bsp/srf06eb_cc2538/drivers/source/bsp_key.c \
	bsp/srf06eb_cc2538/drivers/source/bsp_led.c \
	bsp/srf06eb_cc2538/drivers/source/bsp_uart.c \
	bsp/srf06eb_cc2538/drivers/source/io_pin_int.c \
	bsp/srf06eb_cc2538/drivers/source/lcd_dogm128_6.c \
	bsp/srf06eb_cc2538/drivers/source/lcd_dogm128_6_alphabet.c \
	bsp/srf06eb_cc2538/drivers/source/lcd_srf06eb.c \
	bsp/srf06eb_cc2538/drivers/source/sdcard_srf06eb.c

TI_DEFS = BSP_UART_ISR BSP_UART_ALLOCATE_ISR

# Extra flags to get the TI libraries to compile...
TI_CFLAGS = -fno-strict-aliasing

INCLUDE += $(addprefix $(TI_DIR)/, $(TI_INCLUDE))



#
# Compiler Flags
#
ARCH_FLAGS = -mcpu=cortex-m3 -mfloat-abi=soft -mlittle-endian -mthumb

GC = -Wl,--gc-sections
MAP = -Wl,-Map=output/$(PROJECT).map

CFLAGS += $(ARCH_FLAGS)
CFLAGS += $(addprefix -D, $(TI_DEFS))
CFLAGS += -Wall -Werror -Wno-unused
CFLAGS += --std=gnu99 -Os -g3
CFLAGS += -ffunction-sections -fdata-sections -fno-builtin

CFLAGS += $(addprefix -I, $(INCLUDE))

LFLAGS += -T../cc2538dk.ld
LFLAGS += $(GC) $(MAP)

LFLAGS += -L$(WSN_DIR) -lwsn

# Sections for objcopy to keep
OFLAGS += -j .vectors -j .text -j .flashcca -j .data


#
# Objects to compile
#
OBJS = \
	$(addprefix build/, $(SRCS_C:.c=.o)) \
	$(addprefix build/vendor/, $(TI_SRCS_C:.c=.o))


#
# Build Rules
#
.PHONY: all clean

all: output/$(PROJECT).elf

build/%.o: src/%.c
	$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: src/%.S
	$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/vendor/%.o: $(TI_DIR)/%.c
	$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) $(TI_CFLAGS) -c -o $@ $<

output/$(PROJECT).elf: $(OBJS) $(LIBS)
	$(MKDIR) -p output
	$(CC) $(OBJS) $(CFLAGS) $(LFLAGS) -o $@
	$(OBJCOPY) -O binary $(OFLAGS) output/$(PROJECT).elf output/$(PROJECT).bin
	$(OBJDUMP) -St -marm output/$(PROJECT).elf > output/$(PROJECT).lst
	$(SIZE) output/$(PROJECT).elf

clean:
	rm -rf build
	rm -rf output

prog: all
	$(JLINK) -device cc2538sf53 -commanderscript flash.jlink


#
# TOOLCHAIN
#
CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
OBJDUMP=arm-none-eabi-objdump
SIZE=arm-none-eabi-size
MKDIR=mkdir
MAKE=make
JLINK=JLinkExe

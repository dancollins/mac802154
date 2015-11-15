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

#
# Library sources
#
PROJECT = libwsn

SRCS_C = \
	util/list.c \
	util/pktbuf.c \
	util/ringbuf.c \
	os/os.c \
	radio/cc2538/rfcore.c \
	radio/cc2538/mactimer.c \
	net/mac/mlme.c \
	net/mac/mlme_scan.c \
	net/mac/mlme_association.c \
	net/mac/mcps.c \
	net/mac/coordinator.c \
	net/mac/packet_scheduler.c \
	net/mac/frame.c \
	net/mac/device.c \
	net/mac/security_supplicant.c \
	crypto/cc2538/aes.c

INCLUDE = \
	src \
	src/util \
	src/os \
	src/radio \
	src/radio/mac \
	src/net \
	src/net/mac \
	src/crypto


OBJS = $(addprefix build/, $(SRCS_C:.c=.o))


#
# Vendor libraries
#
ifndef TI_DIR
$(error "TI_DIR not defined.")
endif
INCLUDE += \
	$(TI_DIR)/driverlib/cc2538/inc \
	$(TI_DIR)/driverlib/cc2538/source \
	$(TI_DIR)/bsp/srf06eb_cc2538/drivers/source

DEFINES += BSP_UART_ISR BSP_UART_ALLOCATE_ISR


#
# Compiler flags
#
ARCH_FLAGS = -mcpu=cortex-m3 -mfloat-abi=soft -mlittle-endian -mthumb

GC = -Wl,--gc-sections
MAP = -Wl,-Map=output/$(PROJECT).map

CFLAGS += $(ARCH_FLAGS)
CFLAGS += $(addprefix -D, $(DEFINES))
CFLAGS += -Wall -Werror -Wno-unused -g3 -std=gnu99
CFLAGS += -O0 -ffunction-sections -fdata-sections -fno-builtin

CFLAGS += $(addprefix -I, $(INCLUDE))


#
# Build rules
#
.PHONY: all clean

all: $(PROJECT).a

build/%.o: src/%.c
	@$(MKDIR) -p $(dir $@)
	@$(ECHO) "  CC $(<)"
	@$(CC) $< $(CFLAGS) -c -o $@

$(PROJECT).a: $(OBJS)
	@$(ECHO) "  AR $(@)"
	@$(AR) -cr $@ $^

clean:
	rm -rf build
	rm -rf $(PROJECT).a


#
# Toolchain
#
CC=arm-none-eabi-gcc
AR=arm-none-eabi-gcc-ar
MKDIR=mkdir
ECHO=echo

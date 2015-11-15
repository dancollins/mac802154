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

# Project name
PROJECT = tests

# Project sources
SRCS_C = \
	src/list_test.c \
	src/main.c

INCLUDE = src


#
# WSN library
# We need to compile natively, so we need the actual sources
#
WS_DIR = ../

WS_INCLUDE += src src/util

WS_SRCS_C += \
	src/util/list.c

INCLUDE += $(addprefix $(WS_DIR), $(WS_INCLUDE))


OBJS =  $(addprefix build/, $(SRCS_C:.c=.o)) \
	$(addprefix build/wsn/, $(WS_SRCS_C:.c=.o))

CFLAGS += -O0 -Wall -Werror
CFLAGS += $(addprefix -I, $(INCLUDE))


#
# Build rules
#
.PHONY: all run clean

all: $(PROJECT) run

build/%.o: %.c
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

build/wsn/%.o: $(WS_DIR)/%.c
	$(MKDIR) -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -o $@

$(PROJECT): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LFLAGS) -o $@

run: $(PROJECT)
	./$(PROJECT)

clean:
	rm -rf build
	rm -rf $(PROJECT)


#
# Toolchain
#
CC = gcc
MKDIR = mkdir

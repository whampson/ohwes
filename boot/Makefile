#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of AFROS - Another FRee Operating System.                  #
#                                                                              #
# AFROS is free software. You can redistribute it and/or modify it under the   #
# terms of the MIT License as written in the LICENSE file in the top-level     #
# directory of this codebase.                                                  #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#

AS				:= i686-elf-as
ASFLAGS			:=
LD 				:= i686-elf-ld
LDFLAGS			:=

STAGE1_SOURCES	:= fat.S
STAGE1_OBJECTS	= $(STAGE1_SOURCES:.S=.S.o)

STAGE2_SOURCES	:= init.S
STAGE2_OBJECTS	= $(STAGE2_SOURCES:.S=.S.o)

KERNEL_SOURCES	:= kernel.S
KERNEL_OBJECTS	= $(KERNEL_SOURCES:.S=.S.o)

.PHONY: all clean

all: bootsect init.sys kernel.sys

bootsect: $(STAGE1_OBJECTS)
	@printf 'LD  $^\n    ==> $@\n'
	@$(LD) --oformat binary -Ttext 0x7C00 -e entry -o $@ $^

init.sys: $(STAGE2_OBJECTS)
	@printf 'LD  $^\n    ==> $@\n'
	@$(LD) --oformat binary -Ttext 0x8000 -e stage2 -o $@ $^

kernel.sys: $(KERNEL_OBJECTS)
	@printf 'LD  $^\n    ==> $@\n'
	@$(LD) --oformat binary -Ttext 0x0000 -e kentry -o $@ $^

%.S.o: %.S
	@echo 'AS  $<'
	@$(AS) $(ASFLAGS) -c -o $@ $<

clean:
	@rm -f *.o
	@rm -f *.sys
	@rm -f bootsect

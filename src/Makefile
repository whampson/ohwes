include $(_OSROOT)/build/Common.mk

IMGFILE = $(BINDIR)/ohwes.img

.PHONY: all img boot kernel drivers lib tests

all: img

img: boot kernel
	@echo '> Creating floppy image...'
	@fatfs -i $(IMGFILE) create
	@fatfs -i $(IMGFILE) add $(BINDIR)/init.sys
	@fatfs -i $(IMGFILE) add $(BINDIR)/ohwes.sys
	@echo '> Writing boot sector...'
	@dd if=$(_BINROOT)/boot.bin of=$(IMGFILE) conv=notrunc status=none
	@echo 'OUT $(abspath $(IMGFILE))'

boot:
	@$(MAKE) -C boot

drivers:
	@$(MAKE) -C drivers

kernel: drivers lib tests
	@$(MAKE) -C kernel

lib:
	@$(MAKE) -C lib

tests:
	@$(MAKE) -C tests

include $(_OSROOT)/Makefile

.PHONY: all boot kernel drivers lib tests tools

all: boot kernel tools

# $(BINDIR)/ohwes.img: tools boot kernel
# 	@echo '> Creating floppy image...'
# 	@fatfs -i $(BIN) create
# 	@fatfs -i $(BIN) add $(BINDIR)/init.sys
# 	@fatfs -i $(BIN) add $(BINDIR)/ohwes.sys
# 	@echo '> Writing boot sector...'
# 	@dd if=$(BINDIR)/boot.bin of=$(BIN) conv=notrunc status=none
# 	@echo 'OUT $(abspath $(BIN))'


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

tools:
	@$(MAKE) -C tools

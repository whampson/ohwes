BIN = $(BINDIR)/boot.bin $(BINDIR)/init.sys

include $(_OSROOT)/Makefile

STAGE1_ENTRY = entry
STAGE1_BASE  = 0x7C00
$(BINDIR)/boot.elf: LDFLAGS := -Ttext $(STAGE1_BASE) -e $(STAGE1_ENTRY)

STAGE2_ENTRY = init
STAGE2_BASE  = 0x0000
$(BINDIR)/init.elf: LDFLAGS := -Ttext $(STAGE2_BASE) -e $(STAGE2_ENTRY)

STAGE1_ENTRY	:= entry
STAGE1_BASE	:= 0x7C00
STAGE1_SRC	= fat.S
STAGE1_OBJ	= $(addprefix $(OBJDIR)/,$(STAGE1_SRC:.S=.o))
STAGE1_BIN	= boot.bin

STAGE2_ENTRY	:= init
STAGE2_BASE	:= 0x0000
STAGE2_SRC	= init.S
STAGE2_OBJ	= $(addprefix $(OBJDIR)/,$(STAGE2_SRC:.S=.o))
STAGE2_BIN	= init.sys

BIN             = $(addprefix $(BINDIR)/,$(STAGE1_BIN) $(STAGE2_BIN))

include $(_OSROOT)/build/Common.mk

$(BINDIR)/boot.elf: LDFLAGS := -Ttext $(STAGE1_BASE) -e $(STAGE1_ENTRY)
$(BINDIR)/boot.elf: $(STAGE1_OBJ)

$(BINDIR)/init.elf: LDFLAGS := -Ttext $(STAGE2_BASE) -e $(STAGE2_ENTRY)
$(BINDIR)/init.elf: $(STAGE2_OBJ)

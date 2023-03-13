BIN = $(BINDIR)/fatfs

include $(_TOOLSRC)/Makefile

WARNINGS = no-deprecated-declarations

all:
	@echo "BIN = $(BIN)"
	@echo "OBJ = $(OBJ)"

$(BINDIR)/fatfs: $(OBJ)
	@echo 'LD      $(LDFLAGS) $^'
	@$(LD) $(LDFLAGS) -o $@ $^

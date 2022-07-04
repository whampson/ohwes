BIN = $(BINDIR)/ohwes.sys

include $(_OSROOT)/Makefile

DEFINES += __KERNEL__

OBJ := $(OBJ) \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/drivers/*') \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/lib/*') \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/tests/*')

$(BINDIR)/ohwes.elf: LDFLAGS := -T kernel.ld
$(BINDIR)/ohwes.elf: $(OBJ)

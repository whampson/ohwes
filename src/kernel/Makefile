BIN = $(BINDIR)/ohwes.sys

include $(_OSROOT)/build/Common.mk

CFLAGS += -D__KERNEL__

SRC_ASM = $(wildcard *.S)
SRC = $(wildcard *.c)

OBJ = $(addprefix $(OBJDIR)/,$(SRC_ASM:.S=.o) $(SRC:.c=.o)) \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/drivers/*') \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/lib/*') \
      $(shell find '$(_OBJROOT)/' -type f -name '*.o' -path '$(_OBJROOT)/tests/*')

$(BINDIR)/ohwes.elf: LDFLAGS := -T kernel.ld
$(BINDIR)/ohwes.elf: $(OBJ)

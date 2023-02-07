NO_ELF = 1
TARGET = boot/boot.bin
ENTRYPOINT = Entry
BASE = 0x7C00

SOURCES = \
	boot.S \
	stage2.S

include $(MAKEROOT)

all:: $(BOOTLDR) $(BOOTSCT)

$(BOOTLDR):
	tail -c +512 $(TARGET) > $(BOOTLDR)

$(BOOTSCT):
	head -c 512 $(TARGET) > $(BOOTSCT)

# TODO: update BINDIR and OBJDIR to match bin and obj for current target
# but keep TARGET relative to $(BINROOT)/$(ARCH)/

# TODO: SECONDARY_TARGETS?
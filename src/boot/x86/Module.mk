TARGET  = boot.elf
SOURCES = boot.S stage2.S

 # ORIGIN ENTRYPOINT
LDFLAGS = -Ttext 0x7C00 -e Entry

$(eval $(call make-exe, $(TARGET), $(SOURCES)))

# TODO: split boot.elf into bootsect.bin and boot.sys
# strip boot.elf; first 512b = bootsect.bin, rest = boot.sys

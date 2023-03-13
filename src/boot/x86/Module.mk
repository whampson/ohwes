TARGET  = boot.elf
SOURCES = boot.S stage2.S

 # ORIGIN ENTRYPOINT
LDFLAGS = -Ttext 0x7C00 -e Entry

$(eval $(call make-exe, $(TARGET), $(SOURCES)))

# split boot.elf into boot.bin, bootsect.bin and boot.sys
# boot.bin = stripped boot.elf
# bootsect.bin = bytes 0-511 of boot.bin
# boot.sys = bytes 512-end of boot.bin

_TARGETS += \
	$(BIN_ROOT)/boot.bin \
	$(BIN_ROOT)/boot.sys \
	$(BIN_ROOT)/bootsect.bin

# strip
$(BIN_ROOT)/boot.bin: $(BIN_ROOT)/boot.elf
	objcopy -Obinary $< $@

# extract boot sector
$(BIN_ROOT)/bootsect.bin: $(BIN_ROOT)/boot.bin
	head -c 512 $< > $@

# rest of boot loader
$(BIN_ROOT)/boot.sys: $(BIN_ROOT)/boot.bin
	tail -c +512 $< > $@

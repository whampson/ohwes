TARGET = boot/boot.elf

# NOTE: stage1.S must come before stage2.S,
#       or else entrypoint will end up in the wrong spot!
SOURCES = \
	stage1.S \
	stage2.S \

TGT_LDFLAGS := -Ttext 0x7C00
TGT_LDLIBS  := ${TARGET_DIR}/lib/libcrt.a

###
# split boot.elf into boot.bin, bootsect.bin and boot.sys
#     boot.bin = stripped boot.elf
# bootsect.bin = bytes 0-511 of boot.bin (boot sector)
#     boot.sys = bytes 512-end of boot.bin
###

# strip elf
${TARGET_DIR}/boot.bin: ${TARGET_DIR}/${TARGET}
	${OBJCOPY} -Obinary $< $@

# # extract boot sector (stage 1)
# $(BIN)/bootsect.bin: $(BIN)/$(TARGETNAME).bin
# 	head -c 512 $< > $@

# # extract rest of boot loader (stage 2)
# $(BIN)/$(TARGETNAME).sys: $(BIN)/$(TARGETNAME).bin
# 	tail -c +513 $< > $@



${TARGET_DIR}/boot/boot.bin: ${TARGET_DIR}/boot/boot.elf
	${OBJCOPY} -Obinary $< $@

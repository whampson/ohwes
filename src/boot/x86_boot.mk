TARGET = boot/boot.elf
TARGETBIN = boot/boot.bin
TARGET_STAGE1 = boot/bootsect.bin
TARGET_STAGE2 = sys/boot.sys

.SECONDARY: ${TARGET_STAGE1} ${TARGET_STAGE2}

# NOTE: stage1.S must come before stage2.S,
#       or else entrypoint will end up in the wrong spot!
SOURCES = \
	stage1.S \
	stage2.S \

TGT_LDFLAGS := -Ttext 0x7C00 -e Entry

###
# split boot.elf into boot.bin, bootsect.bin and boot.sys
#     boot.bin = stripped boot.elf
# bootsect.bin = bytes 0-511 of boot.bin (boot sector)
#     boot.sys = bytes 512-end of boot.bin
###

# make boot sector
#   1 - raw boot loader file
#   2 - output file
define make-stage1
	@mkdir -p $(dir $2)
	head -c 512 $1 > $2
endef

# make stage2 boot loader
#   1 - raw boot loader file
#   2 - output file
define make-stage2
	@mkdir -p $(dir $2)
	tail -c +513 $1 > $2
endef

define boot-postmake
  $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/${TARGETBIN})
  $(call make-stage1,${TARGET_DIR}/${TARGETBIN},${TARGET_DIR}/${TARGET_STAGE1})
  $(call make-stage2,${TARGET_DIR}/${TARGETBIN},${TARGET_DIR}/${TARGET_STAGE2})
endef

define boot-postclean
	${RM} ${TARGET_DIR}/${TARGETBIN}
	${RM} ${TARGET_DIR}/${TARGET_STAGE1}
	${RM} ${TARGET_DIR}/${TARGET_STAGE2}
endef

TGT_POSTMAKE  := $(call boot-postmake)
TGT_POSTCLEAN := $(call boot-postclean)

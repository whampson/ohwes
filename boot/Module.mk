TARGET = boot/boot.elf
TARGET_STAGE1 = boot/head.bin
TARGET_STAGE2 = boot/boot.sys

.SECONDARY: ${TARGET_STAGE1} ${TARGET_STAGE2}

SOURCES = \
	stage1.S \
	stage2.S

TGT_LDFLAGS := -T boot/boot.ld

STAGE1_ARGS = --only-section=.stage1
STAGE2_ARGS = --only-section=.stage2

define boot-postmake
  $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/${TARGET_STAGE1},${STAGE1_ARGS})
  $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/${TARGET_STAGE2},${STAGE2_ARGS})
endef

define boot-postclean
	${RM} ${TARGET_DIR}/${TARGET_STAGE1}
	${RM} ${TARGET_DIR}/${TARGET_STAGE2}
endef

TGT_POSTMAKE  := $(call boot-postmake)
TGT_POSTCLEAN := $(call boot-postclean)

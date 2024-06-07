# debug build toggle and params
DEBUG      := 1
DEBUGOPT   := 1
DEBUGFLAGS := -DDEBUG -g
TEST_BUILD := 0

ifeq "${TEST_BUILD}" "1"
  DEBUGFLAGS += -DTEST_BUILD
endif
ifeq "${DEBUGOPT}" "1"
  DEBUGFLAGS += -Og
endif

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

BOOTIMG := ${TARGET_DIR}/boot/bootsect.bin
BOOTSYS := ${TARGET_DIR}/sys/boot.sys
KERNSYS := ${TARGET_DIR}/sys/ohwes.sys
INITEXE := ${TARGET_DIR}/init/init.exe
DISKIMG := ${TARGET_DIR}/ohwes.img

SUBMAKEFILES := \
    ohwes.mk \

MAKEFLAGS := --no-print-directory

.PHONY: all ohwes tools
.PHONY: img floppy format-floppy
.PHONY: run run-bochs debug debug-boot
.PHONY: clean nuke relink

all:
ohwes: all

tools:
	@${MAKE} -C tools

img: tools ohwes
	@mkdir -p $(dir ${DISKIMG})
	fatfs create --force ${DISKIMG} 2880
	dd if=${BOOTIMG} of=${DISKIMG} bs=512 count=1 conv=notrunc
	fatfs add ${DISKIMG} ${BOOTSYS} BOOT.SYS
	fatfs attr -s ${DISKIMG} BOOT.SYS
	fatfs add ${DISKIMG} ${KERNSYS} OHWES.SYS
	fatfs attr -s ${DISKIMG} OHWES.SYS
	fatfs add ${DISKIMG} ${INITEXE} INIT.EXE
	fatfs list -Aa ${DISKIMG}

floppy: ohwes
	dd if=${BOOTIMG} of=/dev/fd0 bs=512 count=1 conv=notrunc
	cp ${BOOTSYS} /a/
	cp ${KERNSYS} /a/
	cp ${INITEXE} /a/

format-floppy:
	mkdosfs -s 1 -S 512 /dev/fd0

run: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG}

run-bochs: img
	${SCRIPT_DIR}/run.sh bochs bochsrc.bxrc

debug: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug

debug-boot: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug-boot

clean:
	${RM} ${DISKIMG}

nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

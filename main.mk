# debug build toggle and params
DEBUG           := 1
DEBUGOPT        := 1
TEST_BUILD      := 0
CFLAGS          := -Wall -Werror

ifeq "${TEST_BUILD}" "1"
  DEFINES += TEST_BUILD
endif
ifeq "${DEBUG}" "1"
  CFLAGS += -g
  DEFINES += DEBUG
endif
ifeq "${DEBUGOPT}" "1"
  CFLAGS += -Og
endif

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

BOOTSECT := ${TARGET_DIR}/boot/head.bin
DISKDRIVE := /a

FILES   := \
    ${TARGET_DIR}/boot/boot.sys \
    ${TARGET_DIR}/ohwes.sys

ifeq "${TEST_BUILD}" "1"
  FILES += ${TARGET_DIR}/test.exe
endif

DISKIMG := ${TARGET_DIR}/ohwes.img

SUBMAKEFILES := \
    ohwes.mk \

.PHONY: all ohwes tools test
.PHONY: img floppy format-floppy
.PHONY: run run-bochs debug debug-boot
.PHONY: clean clean-tools nuke

all:
ohwes: all

tools:
	@${MAKE} -C tools

img: tools ohwes
	@mkdir -p $(dir ${DISKIMG})
	fatfs create --force ${DISKIMG} 2880
	dd if=${BOOTSECT} of=${DISKIMG} bs=512 count=1 conv=notrunc
	$(foreach _file,${FILES},fatfs add ${DISKIMG} ${_file} $(notdir ${_file});\
	    fatfs attr -s ${DISKIMG} $(notdir ${_file});)
	fatfs list -Aa ${DISKIMG}

floppy: ohwes
	dd if=${BOOTSECT} of=/dev/fd0 bs=512 count=1 conv=notrunc
	$(foreach file,${FILES},cp ${file} ${DISKDRIVE}/;)
	ls -l ${DISKDRIVE}

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

clean-tools:
	${MAKE} -C tools clean

clean-all: clean clean-tools

nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}
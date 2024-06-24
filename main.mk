# debug build toggle and params
DEBUG           := 1
DEBUGOPT        := 1
TEST_BUILD      := 1
GLOBAL_FLAGS    := -Wall -Werror

ifeq "${TEST_BUILD}" "1"
  GLOBAL_FLAGS += -DTEST_BUILD
endif
ifeq "${DEBUG}" "1"
  GLOBAL_FLAGS += -DDEBUG -g
endif
ifeq "${DEBUGOPT}" "1"
  GLOBAL_FLAGS += -Og
endif

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

BOOTIMG := ${TARGET_DIR}/bootsect.bin

FILES   := \
    ${TARGET_DIR}/boot.sys \
    ${TARGET_DIR}/ohwes.sys \

ifeq "${TEST_BUILD}" "1"
  FILES += ${TARGET_DIR}/test.exe
endif

DISKIMG := ${TARGET_DIR}/ohwes.img

SUBMAKEFILES := \
    ohwes.mk \

MAKEFLAGS := --no-print-directory

.PHONY: all ohwes tools test
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
	$(foreach _file,${FILES},fatfs add ${DISKIMG} ${_file} $(notdir ${_file});\
	    fatfs attr -s ${DISKIMG} $(notdir ${_file});)
	fatfs list -Aa ${DISKIMG}

floppy: ohwes
	dd if=${BOOTIMG} of=/dev/fd0 bs=512 count=1 conv=notrunc
	$(foreach file,${FILES},cp ${file} /a/;)

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

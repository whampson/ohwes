# debug build toggle and params
DEBUG           := 1
DEBUGOPT        := 1
TEST_BUILD      := 0
CFLAGS          := -Wall -Werror

ifeq "${DEBUG}" "1"
  ASFLAGS += -g
  CFLAGS += -g
  DEFINES += DEBUG
endif
ifeq "${DEBUGOPT}" "1"
  CFLAGS += -Og
endif
ifeq "${TEST_BUILD}" "1"
  DEFINES += TEST_BUILD
endif

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

# source modules
SUBMAKEFILES := src/ohwes.mk

# floppy disk stuff
define FLOPPY_FORMAT_CMDS
	mkdosfs -s 1 -S 512 /dev/fd0
endef
define FLOPPY_COPY_CMDS
	dd if=${BOOTSECT} of=/dev/fd0 bs=512 count=1 conv=notrunc
	$(foreach file,${DISK_FILES},cp ${file} ${DISK_MOUNT}/;)
	ls -l ${DISK_MOUNT}
endef

BOOTSECT    := ${TARGET_DIR}/boot/boot.bin
DISK_IMAGE  := ${TARGET_DIR}/ohwes.img
DISK_MOUNT  := /a
DISK_DEVICE := /dev/fd0
DISK_FILES  := \
    ${TARGET_DIR}/boot/boot.sys \
    ${TARGET_DIR}/ohwes.sys

# -----------------------------------------------------------------------------

.PHONY: all ohwes tools
.PHONY: img floppy format-floppy
.PHONY: run run-bochs debug debug-boot debug-setup
.PHONY: clean clean-tools nuke

all:
ohwes: all

clean:
	${RM} ${DISK_IMAGE}

clean-tools:
	${MAKE} -C tools clean

clean-all: clean clean-tools

nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

tools:
	@${MAKE} -C tools

img: tools ohwes
	@mkdir -p $(dir ${DISK_IMAGE})
	fatfs create --force ${DISK_IMAGE} 2880
	dd if=${BOOTSECT} of=${DISK_IMAGE} bs=512 count=1 conv=notrunc
	$(foreach _file,${DISK_FILES},fatfs add ${DISK_IMAGE} ${_file} $(notdir ${_file});\
	    fatfs attr -s ${DISK_IMAGE} $(notdir ${_file});)
	fatfs list -Aa ${DISK_IMAGE}

run: img
	${SCRIPT_DIR}/run.sh qemu ${DISK_IMAGE}

run-bochs: img
	${SCRIPT_DIR}/run.sh bochs bochsrc.bxrc

debug: img
	${SCRIPT_DIR}/run.sh qemu ${DISK_IMAGE} debug

debug-boot: img
	${SCRIPT_DIR}/run.sh qemu ${DISK_IMAGE} debug-boot

debug-setup: img
	${SCRIPT_DIR}/run.sh qemu ${DISK_IMAGE} debug-setup

floppy: ohwes
	${FLOPPY_COPY_CMDS}

format-floppy: ohwes
	${FLOPPY_FORMAT_CMDS}
	${FLOPPY_COPY_CMDS}

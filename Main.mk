# debug build toggle and params
DEBUG      := 1
DEBUGFLAGS := -DDEBUG -g -Og

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

# boot image
DISKIMG := ${TARGET_DIR}/boot/bootsect.bin

SUBMAKEFILES := \
    src/ohwes.mk \

.PHONY: all tools nuke run run-debug

all:

tools:
	@${MAKE} -C tools $(filter-out tools,${MAKECMDGOALS})

# destroy everything!!!
nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

run: all
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG}

run-debug: all
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug

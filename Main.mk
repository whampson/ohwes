# debug build toggle and params
DEBUG      := 1
DEBUGFLAGS := -DDEBUG -g -Og

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

# boot image
DISKIMG := ${TARGET_DIR}/boot/bootsect.bin

# build the OS!
SUBMAKEFILES := src/ohwes.mk

# do it all!
.PHONY: all
all:

# destroy everything!!!
.PHONY: nuke
nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

# run away!!
.PHONY: run
run: all
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG}

# exterminate bugs!
.PHONY: run-debug
run-debug: all
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug

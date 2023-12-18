# debug build toggle and params
DEBUG      := 1
DEBUGFLAGS := -DDEBUG -g -Og

# important dirs
TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

BOOTIMG := ${TARGET_DIR}/boot/bootsect.bin
BOOTSYS := ${TARGET_DIR}/sys/boot.sys
KERNSYS := ${TARGET_DIR}/sys/ohwes.sys
DISKIMG := ${TARGET_DIR}/ohwes.img

SUBMAKEFILES := \
    src/ohwes.mk \

.PHONY: all img ohwes tools img run debug debug-boot nuke

all:

ohwes: all

tools:
	@${MAKE} -C tools

floppy: ohwes
# mkdosfs -s 1 -S 512 /dev/fd0
# TODO: this is Windows/MINGW only!
	dd if=${BOOTIMG} of=/dev/fd0 bs=512 count=1 conv=notrunc
	cp ${BOOTSYS} /a/
	cp ${KERNSYS} /a/

img: tools ohwes
	@mkdir -p $(dir ${DISKIMG})
	fatfs create --force ${DISKIMG} 2880
	dd if=${BOOTIMG} of=${DISKIMG} bs=512 count=1 conv=notrunc
	fatfs add ${DISKIMG} ${BOOTSYS} BOOT.SYS
	fatfs attr -s ${DISKIMG} BOOT.SYS
	fatfs add ${DISKIMG} ${KERNSYS} OHWES.SYS
	fatfs attr -s ${DISKIMG} OHWES.SYS
	fatfs list -Aa ${DISKIMG}

run: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG}

run-bochs: img
	${SCRIPT_DIR}/run.sh bochs bochsrc.bxrc

debug: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug

debug-boot: img
	${SCRIPT_DIR}/run.sh qemu ${DISKIMG} debug-boot

# destroy everything!!!
nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}

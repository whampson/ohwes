#!/bin/bash
#===============================================================================
#    File: ohwes.sh
# Created: Jan 5, 2023
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU or Bochs.
#===============================================================================

# Usage: run.sh bochs bochs_config
#        run.sh qemu disk_image [debug]

QEMU_PATH=qemu-system-i386
BOCHS_PATH=bochs

if [ "$#" -lt 2 ]; then
    echo "Usage: run.sh bochs bochs_config"
    echo "       run.sh qemu disk_image"
    exit 1
fi

if [ "$1" = "qemu" ]; then
    QEMU_FLAGS=""
    QEMU_FLAGS+=" -m 1M"
    QEMU_FLAGS+=" -boot a"
    QEMU_FLAGS+=" -fda $2"
    QEMU_FLAGS+=" -monitor stdio"
    QEMU_FLAGS+=" -d cpu_reset"
    # QEMU_FLAGS+=" -display gtk,zoom-to-fit=true"
    QEMU_FLAGS+=" -serial telnet:127.0.0.1:50000,server=on"
    # QEMU_FLAGS+=" -serial telnet:127.0.0.1:50001,server=on"
    # QEMU_FLAGS+=" -serial COM5"

    DEBUG_MODE=0
    if [ "$3" = "debug" ]; then
        DEBUG_MODE=1        # 1 = kernel debug
    fi
    if [ "$3" = "debug-boot" ]; then
        DEBUG_MODE=2        # 2 = boot debug
    fi
    if [ "$3" = "debug-setup" ]; then
        DEBUG_MODE=3        # 3 = kernel setup debug
    fi

    if [ $DEBUG_MODE -ne 0 ]; then
        QEMU_FLAGS+=" -S -s"
    fi

    echo "$QEMU_PATH" "$QEMU_FLAGS"

    if [ $DEBUG_MODE = 1 ]; then
        # kernel debug params
        "$QEMU_PATH" $QEMU_FLAGS &
        gdb \
            -ex 'target remote localhost:1234' \
            -ex 'add-symbol-file bin/kernel.elf' \
            -ex 'set confirm off' \
            -ex 'lay src' -ex 'lay reg' \
            -ex 'b start_kernel'
    elif [ $DEBUG_MODE = 2 ]; then
        # boot debug params
        "$QEMU_PATH" $QEMU_FLAGS &
        gdb \
            -ex 'target remote localhost:1234' \
            -ex 'add-symbol-file bin/boot/boot.elf' \
            -ex 'lay src' -ex 'lay reg' \
            -ex 'b stage1' \
            -ex 'b stage2'
    elif [ $DEBUG_MODE = 3 ]; then
        # kernel setup debug params
        "$QEMU_PATH" $QEMU_FLAGS &
        gdb \
            -ex 'target remote localhost:1234' \
            -ex 'add-symbol-file bin/kernel.elf' \
            -ex 'set confirm off' \
            -ex 'lay src' -ex 'lay reg' \
            -ex 'b setup_kernel'
    else
        "$QEMU_PATH" $QEMU_FLAGS
    fi

elif [ "$1" = "bochs" ]; then
    BOCHS_FLAGS=""
    BOCHS_FLAGS+=" -f $2"           # config file
    BOCHS_FLAGS+=" -q"              # no config menu

    echo $BOCHS_PATH $BOCHS_FLAGS
    "$BOCHS_PATH" $BOCHS_FLAGS
else
    echo "error: unknown emulator"
    exit 1
fi

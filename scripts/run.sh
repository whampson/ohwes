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

QEMU=qemu-system-i386
BOCHS=bochs
GDB=gdb

QEMU_FLAGS=""
BOCHS_FLAGS=""
GDB_FLAGS=()

if [ "$#" -lt 2 ]; then
    echo "Usage: run.sh bochs bochs_config"
    echo "       run.sh qemu disk_image"
    exit 1
fi


# TODO: consider...
#   while (($#)); do
#       # parse args
#       shift
#   done

if [ "$1" = "qemu" ]; then
    QEMU_FLAGS+=" -m 4M"
    QEMU_FLAGS+=" -boot a"
    QEMU_FLAGS+=" -fda $2"
    # QEMU_FLAGS+=" -hda $2"
    QEMU_FLAGS+=" -monitor stdio"
    QEMU_FLAGS+=" -d cpu_reset"
    QEMU_FLAGS+=" -serial telnet:127.0.0.1:50001,server=on,nowait" # com1
    QEMU_FLAGS+=" -serial telnet:127.0.0.1:50002,server=on,nowait" # com2

    ##
    ## TODO: need to do parameter list or something because this is ridiculous
    ##
    DEBUG_MODE=0
    NOBREAK=0
    if [ "$3" = "debug" ]; then
        DEBUG_MODE=1        # 1 = kernel entry
    fi
    if [ "$3" = "debug-nobreak" ]; then
        DEBUG_MODE=1
        NOBREAK=1
    fi
    if [ "$3" = "debug-boot" ]; then
        DEBUG_MODE=2        # 2 = boot entry
    fi
    if [ "$3" = "debug-setup" ]; then
        DEBUG_MODE=3        # 3 = kernel setup debug
    fi
    if [ "$3" = "debug-nogdb" ]; then
        DEBUG_MODE=4        # 4 = do not automatically connect GDB
    fi

    if [ $DEBUG_MODE -ne 0 ]; then
        QEMU_FLAGS+=" -S -s"
    fi

    echo "$QEMU" ${QEMU_FLAGS[*]}
    if [ $DEBUG_MODE -eq 0 ]; then
        "$QEMU" $QEMU_FLAGS
    else
        "$QEMU" $QEMU_FLAGS &
    fi

    # this is so bad lol
    if [ $DEBUG_MODE = 1 ]; then
        # kernel debug params
        if [ $NOBREAK -ne 1 ]; then
            gdb \
                -ex 'target remote localhost:1234' \
                -ex 'add-symbol-file bin/kernel.elf' \
                -ex 'set confirm off' \
                -ex 'lay src' -ex 'lay reg' \
                -ex 'b kmain' \

        else
            gdb \
                -ex 'target remote localhost:1234' \
                -ex 'add-symbol-file bin/kernel.elf' \
                -ex 'set confirm off' \
                -ex 'lay src' -ex 'lay reg' \

        fi

        # can't get this to work for some reason
        # GDB_FLAGS+=(-ex 'target remote localhost:1234')
        # GDB_FLAGS+=(-ex 'add-symbol-file bin/kernel.elf')
        # GDB_FLAGS+=(-ex 'set confirm off')
        # GDB_FLAGS+=(-ex 'lay src' -ex 'lay reg')
        # if [ $NOBREAK -ne 1 ]; then
        #     GDB_FLAGS+=(-ex 'b kmain')
        # fi
    elif [ $DEBUG_MODE = 2 ]; then
        # boot debug params
        gdb \
            -ex 'target remote localhost:1234' \
            -ex 'add-symbol-file bin/boot/boot.elf' \
            -ex 'lay src' -ex 'lay reg' \
            -ex 'b stage1' -ex 'b stage2' \

    elif [ $DEBUG_MODE = 3 ]; then
        # kernel setup debug params
        gdb \
            -ex 'target remote localhost:1234' \
            -ex 'add-symbol-file bin/kernel.elf' \
            -ex 'set confirm off' \
            -ex 'lay src' -ex 'lay reg' \
            -ex 'b ksetup' \

    else
        # no initial GDB
        DEBUG_MODE=0
    fi

    # if [ $? -ne 0 ]; then
    #     exit
    # fi

    # if [ $DEBUG_MODE -ne 0 ]; then
    #     echo "$GDB" ${GDB_FLAGS[*]}
    #     "$GDB" ${GDB_FLAGS[*]}
    # fi

elif [ "$1" = "bochs" ]; then
    BOCHS_FLAGS+=" -f $2"           # config file
    BOCHS_FLAGS+=" -q"              # no config menu

    echo $BOCHS $BOCHS_FLAGS
    "$BOCHS" $BOCHS_FLAGS
else
    echo "error: unknown emulator"
    exit 1
fi

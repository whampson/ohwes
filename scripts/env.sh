#!/bin/bash
#===============================================================================
#    File: scripts/env.sh
# Created: September 23, 2023
# Revised: November 1, 2026
#  Author: Wes Hampson
#
# Sets up the environment for building OH-WES.
#
# Usage: `source env.sh [clean]`
#===============================================================================

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    echo "error: env.sh must be sourced"
    echo "usage: source scripts/env.sh [clean]"
    exit 1
fi

# reset environment
if [ "$1" = clean ]; then
    PATH=${OHWES_PATH_BACKUP:-$PATH}
    unset OHWES_ENVIRONMENT_SET OHWES_PATH_BACKUP \
        HOST_CC HOST_CXX HOST_SYSTEM HOST_PREFIX \
        CROSS_PREFIX CROSS_CC CROSS_AR CROSS_AS CROSS_CXX CROSS_LD CROSS_OBJCOPY \
        PROJ_ROOT TOOLCHAIN ELF_TOOLS_ROOT \
        QEMU BOCHS GDB \
        BOCHS_DIR BOCHSRC

    unset -f path_append print_name_path env_main
    complete -r make run.sh env.sh
    echo "Environment cleaned."
    return 0
fi

env_main() {
    if [ -n "$OHWES_ENVIRONMENT_SET" ]; then
        echo "note: OH-WES environment already set; reloading..."
        echo ""
    fi

    # Remember the original PATH so `clean` can restore it later.
    OHWES_PATH_BACKUP=${OHWES_PATH_BACKUP:-$PATH}

    # Get project root path (relative to this script)
    export PROJ_ROOT=$(realpath $(dirname $BASH_SOURCE[0])/..)

    # Import the defaults so HOST_CC/HOST_CXX/etc. are available below.
    source "$PROJ_ROOT/scripts/def.sh"

    # Complete target names from all Makefile fragments in the project.
    # https://stackoverflow.com/a/38415982
    complete -W "\`find "$PROJ_ROOT" \( -name '*.mk' -o -name 'Makefile' \) \
        -not -path '*/build/*' -not -path '*/obj/*' -not -path '*/bin/*' \
        -exec grep -hoE '^[a-zA-Z0-9_.-]+:([^=]|$)' {} + 2>/dev/null \
        | sed -r 's/[^a-zA-Z0-9_.-]*$//' | sort -u\`" make

    # env.sh argument completion
    complete -W "clean" env.sh

    # run.sh flag completion
    complete -W "\
        --bochs -b \
        --img -i \
        --bochsrc -c \
        --debug -d \
        --port -p \
        --no-gdb -G \
        --no-break -B \
        --log -l \
        --dry-run -n \
        --help -h" \
        run.sh

    export HOST_SYSTEM=unknown              # platform tag

    # path_append DIR - Append DIR to PATH only if not already present
    path_append() {
        case ":$PATH:" in
            *":$1:"*) ;;
            *) PATH="$PATH:$1" ;;
        esac
    }

    # Platform-specific stuff.
    # Identify host system and ensure a consistent environment across systems.
    local UNAME_S=$(uname -s)
    case "$UNAME_S" in
        MINGW*)  # Windows / MSYS2
            HOST_SYSTEM=msys2
            QEMU_DIR="/c/Program Files/qemu"
            if [ -d "$QEMU_DIR" ]; then
                path_append "$QEMU_DIR"
            fi
            # Bochs dirs are versioned (Bochs-X.Y), so glob them and use the newest
            while IFS= read -r BOCHS_DIR; do
                path_append "$BOCHS_DIR"
                break
            done < <(ls -d "/c/Program Files/Bochs-"* 2>/dev/null | sort -Vr)
            ;;
        Linux*)                   # Linux: expect emulators on PATH
            HOST_SYSTEM=linux
            ;;
        Darwin*)                  # macOS: expect emulators on PATH
            HOST_SYSTEM=macos
            ;;
        *)
            HOST_SYSTEM=unknown
            echo "warning: unsupported platform '$UNAME_S'"
            ;;
    esac

    # Prebuilt cross-compiler, prefer `latest` (symlink to current release),
    # then the versioned release dirs, newest first (all platforms).
    export ELF_TOOLS_ROOT=${ELF_TOOLS_ROOT:-"$PROJ_ROOT/build/i686-elf-tools"}
    while IFS= read -r ELF_TOOLS_REL; do
        local ELF_TOOLS_BIN="$ELF_TOOLS_ROOT/$ELF_TOOLS_REL/bin"
        if [ -x "$ELF_TOOLS_BIN/${TOOLCHAIN}-gcc" ]; then
            path_append "$ELF_TOOLS_BIN"
            break
        fi
    done < <( {
            [ -e "$ELF_TOOLS_ROOT/latest" ] && echo latest
            [ -d "$ELF_TOOLS_ROOT" ] && ls -1 "$ELF_TOOLS_ROOT" | grep -vx latest | sort -Vr
        } )   # this is crazy lol

    # Locate Bochs config for current system type.
    local BOCHS_PLATFORM="$HOST_SYSTEM"
    [ "$HOST_SYSTEM" = msys2 ] && BOCHS_PLATFORM=win32
    export BOCHSRC="$PROJ_ROOT/build/bochs/$BOCHS_PLATFORM/bochsrc.bxrc"

    # Add native-built tools to PATH
    local TOOLS_PATH=$PROJ_ROOT/bin/tools
    path_append "$TOOLS_PATH"

    # print_name_path NAME PATH - Prints "NAME => PATH", with NAME
    #   right padded by NAME_WIDTH
    print_name_path() {
        local name=$1 path=$2
        printf '%-*s => %s\n' "$NAME_WIDTH" "$name" "$path"
    }

    echo "Host system is $HOST_SYSTEM"
    echo ""

    local NAME_WIDTH=10
    print_name_path "native-cc"   "$(command -v $HOST_CC)"
    print_name_path "native-cxx"  "$(command -v $HOST_CXX)"
    print_name_path "cross-cc"    "$(command -v $CROSS_CC)"
    print_name_path "gdb"         "$(command -v $GDB)"
    print_name_path "make"        "$(command -v make)"
    print_name_path "qemu"        "$(command -v $QEMU)"
    print_name_path "bochs"       "$(command -v $BOCHS)"
    print_name_path "bochsrc"     "$BOCHSRC"
    echo ""

    # Checks
    if [[ -z "$(command -v make)" ]]; then
        echo "error: missing make!"
        return 1
    fi
    if [[ -z "$(command -v $HOST_CC)" ]]; then
        echo "error: missing C compiler ($HOST_CC)!"
        return 1
    fi
    if [[ -z "$(command -v $HOST_CXX)" ]]; then
        echo "error: missing C++ compiler ($HOST_CXX)!"
        return 1
    fi
    if [[ -z "$(command -v $CROSS_CC)" ]]; then
        echo "error: missing C cross-compiler ($CROSS_CC)!"
        return 1
    fi
    if [[ -z "$(command -v $QEMU)" ]]; then
        echo "warning: qemu not found!"
    fi
    if [[ -z "$(command -v $BOCHS)" ]]; then
        echo "warning: bochs not found!"
    fi
    if [[ -z "$(command -v $GDB)" ]]; then
        echo "warning: gdb not found!"
    fi

    # All done!
    OHWES_ENVIRONMENT_SET=1
    echo "All set! Run \`make help\` to learn how to build OH-WES."
}

env_main

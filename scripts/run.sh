#!/bin/bash
#===============================================================================
#    File: scripts/run.sh
# Created: November 27, 2020
# Revised: November 4, 2026
#  Author: Wes Hampson
#
# Boots OH-WES in QEMU or Bochs.
#===============================================================================

set -euo pipefail

PROJ_ROOT=$(realpath "$(dirname "${BASH_SOURCE[0]}")/..")

# Emulator/debugger paths (QEMU, BOCHS, GDB) with their env-var overrides.
source "$PROJ_ROOT/scripts/def.sh"

# ------------------------------------------------------------------------------
# Usage & helpers
# ------------------------------------------------------------------------------

usage() {
    cat <<'EOF'
Usage: run.sh [options] [-- <extra qemu args>]

Run options:
  -b, --bochs          Use Bochs instead of QEMU (default).
  -i, --img PATH       Floppy disk image (default: bin/ohwes.img).
  -c, --bochsrc PATH   Bochs config file (default: $BOCHSRC, then
                       ./bochsrc.bxrc).

Debug options:
  -d, --debug MODE     Debug mode: kernel, boot, setup, or serial. kernel/
                       boot/setup start QEMU halted (-S) with a gdb stub (-s)
                       and attach gdb with mode-specific breakpoints. serial
                       attaches gdb to a real target over a serial COM port
                       (no QEMU; 115200 baud, no breakpoints).
  -p, --port DEVICE    Serial device for '--debug serial' (default: com1).
  -G, --no-gdb         Do not launch gdb in debug mode.
  -B, --no-break       Do not set mode-specific breakpoints in gdb.
  -l, --log FILE       QEMU output log file in debug mode (default: qemu.log).

General options:
  -n, --dry-run        Print the resolved commands without executing them.
      --               Everything after this is passed verbatim to QEMU
                       (e.g. run.sh --dry-run -- -monitor stdio).
  -h, --help           Show this help message and exit.

Long options also accept the --option=value form. The QEMU, BOCHS, and GDB
environment variables override the tool executables (see scripts/env.sh).
EOF
}

die() {
    echo "error: $*" >&2
    exit 1
}

# Print a shell-quoted command line that can be copy-pasted verbatim.
print_cmd() {
    printf '%q ' "$@"
    printf '\n'
}

# ------------------------------------------------------------------------------
# Argument parsing
# ------------------------------------------------------------------------------

EMU=qemu
IMG="$PROJ_ROOT/bin/ohwes.img"
BOCHSRC_FILE=""
DEBUG_MODE=""
USE_GDB=1
USE_BREAK=1
SERIAL_DEVICE=com1
SERIAL_DEVICE_SET=0
LOG_FILE=qemu.log
DRY_RUN=0
QEMU_EXTRA=()

while (($#)); do
    case "$1" in
        -b|--bochs)     EMU=bochs; shift ;;
        -i|--img)       [ $# -ge 2 ] || die "missing argument for '$1'"
                        IMG=$2; shift 2 ;;
        --img=*)        IMG=${1#*=}; shift ;;
        -c|--bochsrc)   [ $# -ge 2 ] || die "missing argument for '$1'"
                        BOCHSRC_FILE=$2; shift 2 ;;
        --bochsrc=*)    BOCHSRC_FILE=${1#*=}; shift ;;
        -d|--debug)     [ $# -ge 2 ] || die "missing argument for '$1'"
                        DEBUG_MODE=$2; shift 2 ;;
        --debug=*)      DEBUG_MODE=${1#*=}; shift ;;
        -p|--port)      [ $# -ge 2 ] || die "missing argument for '$1'"
                        SERIAL_DEVICE=$2; SERIAL_DEVICE_SET=1; shift 2 ;;
        --port=*)       SERIAL_DEVICE=${1#*=}; SERIAL_DEVICE_SET=1; shift ;;
        -G|--no-gdb)    USE_GDB=0; shift ;;
        -B|--no-break)  USE_BREAK=0; shift ;;
        -l|--log)       [ $# -ge 2 ] || die "missing argument for '$1'"
                        LOG_FILE=$2; shift 2 ;;
        --log=*)        LOG_FILE=${1#*=}; shift ;;
        -n|--dry-run)   DRY_RUN=1; shift ;;
        --)             shift
                        while (($#)); do QEMU_EXTRA+=("$1"); shift; done ;;
        -h|--help)      usage; exit 0 ;;
        *)              die "unknown option '$1' (see --help)" ;;
    esac
done

# ------------------------------------------------------------------------------
# Validation
# ------------------------------------------------------------------------------

case "$EMU" in
    qemu|bochs) ;;
    *)      die "unknown emulator '$EMU' (expected 'qemu' or 'bochs')" ;;
esac

case "$DEBUG_MODE" in
    ""|kernel|boot|setup|serial) ;;
    *)      die "unknown debug mode '$DEBUG_MODE' (expected 'kernel', 'boot', 'setup', or 'serial')" ;;
esac

if [ "$EMU" = bochs ] && [ -n "$DEBUG_MODE" ] && [ "$DEBUG_MODE" != serial ]; then
    echo "warning: --debug is only supported with qemu; ignoring" >&2
fi
if ((SERIAL_DEVICE_SET)) && [ "$DEBUG_MODE" != serial ]; then
    echo "warning: --port has no effect without '--debug serial'" >&2
fi
if [ -z "$DEBUG_MODE" ] && ((! USE_GDB)); then
    echo "warning: --no-gdb has no effect without --debug" >&2
fi
if [ -z "$DEBUG_MODE" ] && ((! USE_BREAK)); then
    echo "warning: --no-break has no effect without --debug" >&2
fi

# ------------------------------------------------------------------------------
# QEMU
# ------------------------------------------------------------------------------

run_qemu() {
    local -a args=(
        -boot a
        -drive if=floppy,format=raw,file="$IMG"
        -audiodev sdl,id=audio0 -machine pcspk-audiodev=audio0
        -chardev stdio,id=dbgcon0 -device isa-debugcon,chardev=dbgcon0
        -d cpu_reset    # dump CPU state on reset
        -serial telnet:127.0.0.1:50001,server=on,nowait   # com1
        -serial telnet:127.0.0.1:50002,server=on,nowait   # com2
        -serial telnet:127.0.0.1:50003,server=on,nowait   # com3
        -serial telnet:127.0.0.1:50004,server=on,nowait   # com4
    )
    if [ -n "$DEBUG_MODE" ]; then
        args+=(-S -s)
    fi
    if ((${#QEMU_EXTRA[@]})); then
        args+=("${QEMU_EXTRA[@]}")
    fi

    if ((DRY_RUN)); then
        print_cmd "$QEMU" "${args[@]}"
        return
    fi

    if [ ! -f "$IMG" ]; then
        die "disk image not found: $IMG (build it with 'make img')"
    fi
    echo "$QEMU" "${args[@]}"
    if [ -z "$DEBUG_MODE" ]; then
        "$QEMU" "${args[@]}"
    else
        # Background QEMU and redirect its output (E9 debug output, cpu_reset
        # dumps, etc.) to a log so it doesn't corrupt gdb's TUI. The PID is
        # captured so QEMU can be killed when the gdb session ends (see the
        # trap in the main flow); --no-gdb sessions leave QEMU running on
        # purpose so a debugger can be attached manually.
        "$QEMU" "${args[@]}" > "$LOG_FILE" 2>&1 &
        QEMU_PID=$!
    fi
}

# ------------------------------------------------------------------------------
# gdb (debug modes only)
# ------------------------------------------------------------------------------

run_gdb() {
    if [ -z "$DEBUG_MODE" ] || ((! USE_GDB)); then
        return 0
    fi

    local -a args=(
        -ex 'set confirm off'
        -ex 'set architecture i386'
        -ex 'lay src' -ex 'lay reg'
    )

    # Native Windows gdb can't resolve MSYS paths like /d/..., so convert the
    # symbol-file paths to Windows form when cygpath is available (MSYS2/Cygwin).
    local kernel_elf="$PROJ_ROOT/bin/kernel.elf"
    local boot_elf="$PROJ_ROOT/bin/boot/boot.elf"
    if command -v cygpath >/dev/null 2>&1; then
        kernel_elf=$(cygpath -m "$kernel_elf")
        boot_elf=$(cygpath -m "$boot_elf")
    fi

    case "$DEBUG_MODE" in
        kernel)
            args+=(-ex 'target remote localhost:1234')
            args+=(-ex "add-symbol-file $kernel_elf")
            if ((USE_BREAK)); then args+=(-ex 'b kmain'); fi
            ;;
        boot)
            args+=(-ex 'target remote localhost:1234')
            args+=(-ex "add-symbol-file $boot_elf")
            if ((USE_BREAK)); then args+=(-ex 'b entry' -ex 'b stage2'); fi
            ;;
        setup)
            args+=(-ex 'target remote localhost:1234')
            args+=(-ex "add-symbol-file $kernel_elf")
            if ((USE_BREAK)); then args+=(-ex 'b ksetup'); fi
            ;;
        serial)
            args+=(-ex 'set serial baud 115200')
            args+=(-ex "target remote $SERIAL_DEVICE")
            args+=(-ex "add-symbol-file $kernel_elf")
            ;;
    esac

    if ((DRY_RUN)); then
        print_cmd "$GDB" "${args[@]}"
    else
        echo "$GDB" "${args[@]}"
        "$GDB" "${args[@]}"
    fi
}

# ------------------------------------------------------------------------------
# Bochs
# ------------------------------------------------------------------------------

run_bochs() {
    # Config file: prefer the explicit --bochsrc argument, then fall back to
    # the platform-specific BOCHSRC (set by scripts/env.sh), then the default.
    local bochsrc=${BOCHSRC_FILE:-${BOCHSRC:-bochsrc.bxrc}}
    if [ ! -f "$bochsrc" ]; then
        die "Bochs config not found: $bochsrc"
    fi

    local -a bochs_args=(-f "$bochsrc" -q)   # config file, no config menu
    case "$(uname -s)" in
        MINGW*)
            # On Windows/MSYS2, launch via PowerShell so the bundled DLLs are
            # found. Resolve Bochs to a Windows path and convert the config
            # path too, since PowerShell can't resolve MSYS paths (the -f
            # existence check above used the MSYS one).
            if ! command -v "$BOCHS" >/dev/null 2>&1; then
                die "Bochs not found ($BOCHS)"
            fi
            local bochs_exe
            bochs_exe=$(cygpath -w "$(command -v "$BOCHS")")
            local bochsrc_win
            bochsrc_win=$(cygpath -w "$bochsrc")
            local cmd="powershell -command \"& { & '$bochs_exe' -f '$bochsrc_win' -q }\""
            echo "$cmd"
            if ((! DRY_RUN)); then
                eval "$cmd"
            fi
            ;;
        *)
            if ((DRY_RUN)); then
                print_cmd "$BOCHS" "${bochs_args[@]}"
            else
                echo "$BOCHS" "${bochs_args[@]}"
                "$BOCHS" "${bochs_args[@]}"
            fi
            ;;
    esac
}

# ------------------------------------------------------------------------------
# Go!
# ------------------------------------------------------------------------------

if [ "$DEBUG_MODE" = serial ]; then
    # Serial debugging attaches gdb to a real target over a COM port; no
    # emulator is launched.
    run_gdb
else
    case "$EMU" in
        qemu)
            run_qemu
            if [ -n "${QEMU_PID:-}" ]; then
                if ((USE_GDB)); then
                    # Kill QEMU when the gdb session (or this script) ends;
                    # otherwise it lingers holding the gdb/serial ports.
                    trap 'kill "$QEMU_PID" 2>/dev/null' EXIT
                else
                    echo "QEMU running in the background (pid $QEMU_PID); kill it when done."
                fi
            fi
            run_gdb
            ;;
        bochs)
            run_bochs
            ;;
    esac
fi

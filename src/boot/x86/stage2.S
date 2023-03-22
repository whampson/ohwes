.code16
.text

# TODO: glean as much information about the system as possible from the
# BIOS system calls before switching to protected mode
# VGA, RAM, installed hardware, CPU params?

.globl Stage2
Stage2:
    xorw        %ax, %ax
    leaw        s_HelloFromStage2, %si
    call        PrintLn

    // movb        $0x12, %al
    // call        VgaSetMode

    // movb        $1, %al     # blue
    // call        VgaSetColor

    # get VGA info
    leaw        s_VideoMode, %si
    call        Print
    call        VgaGetInfo
    movzbw      %al, %ax
    call        Num2Hex
    call        PrintLn

    # get hardware info
    leaw        s_EquipmentFlags, %si
    call        Print
    int         $0x11       # get partial installed hardware list
    movw        %ax, g_EquipmentFlags
    call        Num2Hex
    call        PrintLn

    # enable A20
EnableA20:
    call        TestA20
    jnc         _enable_a20_done
    call        _enable_a20_int15h
    call        TestA20
    jnc         _enable_a20_done
    call        _enable_a20_port92h
    call        TestA20
    jnc         _enable_a20_done
    call        _enable_a20_keyboard
    movw        $0xFFFF, %cx
    rep  nop
    call        TestA20
    jmp         _enable_a20_done

_enable_a20_int15h:
    leaw        s_A20_TryingInt15h, %si
    call        PrintLn
    movw        $0x2401, %ax        # enable A20 gate
    int         $0x15               # CF set on error
    ret

_enable_a20_port92h:
    leaw        s_A20_TryingPort92h, %si
    call        PrintLn
    inb         $0x92, %al
    orb         $0x02, %al
    andb        $0xFE, %al
    outb        %al, $0x92
    ret

_enable_a20_keyboard:
    leaw        s_A20_TryingKeyboard, %si
    call        PrintLn

    call        wait_8042_cmd
    movb        $0xAD, %al      # cmd: disable keyboard
    outb        %al, $0x64

    call        wait_8042_cmd
    movb        $0xD0, %al      # cmd: read controller output port
    outb        %al, $0x64

    call        wait_8042_data
    inb         $0x60, %al      # data: controller output port
    pushw       %ax

    call        wait_8042_cmd
    movb        $0xD1, %al      # cmd: write controller output port
    outb        %al, $0x64

    call        wait_8042_cmd
    popw        %ax             # data: prev output port
    orb         $0x02, %al      # bit 1 = A20 enable
    outb        %al, $0x60

    call        wait_8042_cmd
    movb        $0xAE, %al      # cmd: enable keyboard
    outb        %al, $0x64

    call        wait_8042_cmd   # empty 8042 input buffer
    ret

wait_8042_cmd:
    inb         $0x64, %al      # 8042 Status Register
    testb       $0x02, %al      # is input buffer full?
    jnz         wait_8042_cmd      # if yes, continue checking 'til empty
    ret

wait_8042_data:
    inb         $0x64, %al
    testb       $0x01, %al
    jz          wait_8042_data
    ret

_enable_a20_done:
    leaw        s_A20, %si
    call        Print
    call        TestA20
    jc          _a20_status_off
    leaw        s_On, %si
    call        PrintLn
    jmp         _ram
_a20_status_off:
    leaw        s_Off, %si
    call        PrintLn
    jmp         _ram

_ram:
    # get RAM capacity
    leaw        s_KbRam, %si
    call        Print
    int         $0x12       # get system RAM
    movw        %ax, g_RamKB
    call        Num2Hex
    call        PrintLn
    # TODO: int 15h ah=88h, int 15h ax=e801h, int 15h ax=e820h

    call        SetupGdt

    cli
    movl        %cr0, %eax
    orl         $0x01, %eax       # enable Protected Mode
    movl        %eax, %cr0
    jmpl        $0x08, $Entry32

.code32

Entry32:
    movw        $0x10, %ax
    movw        %ax, %ds
    movw        %ax, %es
    movw        %ax, %ss
    call        kmain

Idle32:
    jmp         Idle32

.code16

# -----------------------------------------------------------------------------
# TestA20
# -----------------------------------------------------------------------------
# Checks whether the A20 line is enabled.
#
# Inputs:
#       (none)
# Outputs:
#       CF is set if A20 is OFF
# Clobbers:
#       ax, dx, ds, fs
#
TestA20:
    // wbinvd       # invalidate cache (i486+)
    xorw        %ax, %ax
    movw        %ax, %ds            # ds = 0x0000
    decw        %ax
    movw        %ax, %fs            # fs = 0xFFFF
    rorw        $4, %ds:0x7DFE      # tweak the value
    movw        %ds:0x7DFE, %dx     # [0x00FDFE] -> dx
    movw        %fs:0x7E0E, %ax     # [0x10FDFE] -> ax
    cmpw        %ax, %dx
    je          _A20_OFF
    clc
    jmp         _A20_DONE
_A20_OFF:
    stc
_A20_DONE:
    ret

# -----------------------------------------------------------------------------
# SetupGdt
# -----------------------------------------------------------------------------
SetupGdt:
    movw        gdt_size, %dx
    decw        %dx
    movw        %dx, gdt_regptr
    movl        $gdt_base, gdt_regptr+2
    lgdtl       gdt_regptr
    ret

// TODO: expand-out fields for clarity
.align 16
gdt_base:
    .quad       0x0000000000000000      # (not used, required by CPU)
    .quad       0x00CF9A000000FFFF      # cs = 0x08
    .quad       0x00CF92000000FFFF      # ds = 0x10
gdt_end:

.align 8
gdt_regptr:
    .quad       0

gdt_size:
    .long       gdt_end - gdt_base

# -----------------------------------------------------------------------------
# Num2Hex
# -----------------------------------------------------------------------------
# Writes an unsigned number to a buffer as a zero-terminated hexadecimal
# string. The hexadecimal digits are suffixed with an 'h'.
#
# Inputs:
#         ax - number to stringify
# Outputs:
#      ds:si - string buffer address
# Clobbers:
#      ax, cx, dx
# TODO: start using C calling convention?
#
.globl Num2Hex
Num2Hex:
    movw        m_Num2HexBuf, %si
    movw        $4, %cx

_Num2Hex_Loop:
    xorl        %edx, %edx
    rolw        $4, %ax
    movw        %ax, %dx
    andw        $0x0F, %dx

    movb        m_Num2HexLut(%edx), %dl
    movb        %dl, (%si)
    incw        %si

_Num2Hex_LoopCheck:
    decw        %cx
    jnz         _Num2Hex_Loop

_Num2Hex_Done:
    movb        $0x68, 0(%si)   # 'h'
    movb        $0, 1(%si)
    movw        m_Num2HexBuf, %si
    ret

m_Num2HexLut:
    .ascii      "0123456789ABCDEF"

m_Num2HexBuf:
    .quad       0


# -----------------------------------------------------------------------------
# VgaGetInfo
# -----------------------------------------------------------------------------
# Gets the current VGA parameters.
#
# Inputs:
#      (none)
# Outputs:
#      al, g_VgaMode - video mode
#      ah, g_VgaNunCols - number of columns per line
#      bh, g_VgaPage - active display page number
# Clobbers:
#      possibly many due to interrupt
#
.globl VgaGetInfo
VgaGetInfo:
    movb        $0x0F, %ah
    int         $0x10
    movb        %al, g_VgaMode
    movb        %ah, g_VgaNumCols
    movb        %bh, g_VgaPage
    ret

# -----------------------------------------------------------------------------
# VgaSetMode
# -----------------------------------------------------------------------------
# Sets the current background/border color.
#
# Inputs:
#      al - mode ID
# Clobbers:
#      bx, possibly many others due to interrupt
#
.globl VgaSetMode
VgaSetMode:
    movb        $0x00, %ah
    int         $0x10
    call        VgaGetInfo
    ret

# -----------------------------------------------------------------------------
# VgaSetColor
# -----------------------------------------------------------------------------
# Sets the current background/border color.
#
# Inputs:
#      al - color ID
# Clobbers:
#      bx, possibly many others due to interrupt
#
.globl VgaSetColor
VgaSetColor:
    movb        %al, g_VgaColor
    movb        $0x0B, %ah
    movb        $0x00, %bh
    movb        g_VgaColor, %bl
    int         $0x10
    ret

.data

.globl g_VgaMode
g_VgaMode:
    .long       0

.globl g_VgaNumCols
g_VgaNumCols:
    .long       0

.globl g_VgaPage
g_VgaPage:
    .long       0

.globl g_VgaColor
g_VgaColor:
    .long       0

.globl g_EquipmentFlags
g_EquipmentFlags:
#       [0] - diskette drive installed
#       [1] - math coprocessor (x87) installed
#       [2] - PS/2 mouse installed
#       [3] - (unused)
#     [5:4] - initial video mode (00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono)
#     [7:6] - num diskette drives attached less 1
#       [8] - (unused) (previously DMA access installed, or DMA access NOT installed)
#    [11:9] - num serial ports installed
#      [12] - game port installed
#      [13] - (unused) (previously serial printer attached, or internal modem installed)
#   [15:14] - num parallel ports installed
    .long       0

.globl g_RamKB
g_RamKB:
    .long       0

s_HelloFromStage2:
    .asciz      "\r\nHello from stage 2!"
s_VideoMode:
    .asciz      "BIOS: Video Mode = "
s_EquipmentFlags:
    .asciz      "BIOS: Equipment Flags = "
s_KbRam:
    .asciz      "BIOS: KB RAM = "
s_A20:
    .asciz      "BIOS: A20 Status = "
s_On:
    .asciz      "ON"
s_Off:
    .asciz      "OFF"
s_A20_TryingInt15h:
    .asciz      "BIOS: A20: trying int 15h... "
s_A20_TryingPort92h:
    .asciz      "BIOS: A20: trying port 92h... "
s_A20_TryingKeyboard:
    .asciz      "BIOS: A20: trying keyboard... "

.code16

// TODO: glean as much information about the system as possible from the
// BIOS system calls before switching to protected mode
// VGA, RAM, installed hardware, CPU params?

.globl Stage2
Stage2:
    xorw        %ax, %ax
    leaw        s_HelloFromStage2, %si
    call        PrintLn

    leaw        s_VideoMode, %si
    call        Print
    call        VgaGetMode
    movzbw      %al, %ax
    call        Num2Hex
    call        PrintLn

    leaw        s_EquipmentFlags, %si
    call        Print
    int         $0x11       // get partial installed hardware list
    movw        %ax, m_EquipmentFlags
    call        Num2Hex
    call        PrintLn

    leaw        s_KbRam, %si
    call        Print
    int         $0x12       // get system RAM
    movw        %ax, m_KbContiguousRam
    call        Num2Hex
    call        Print
    // TODO: int 15h ah=88h, int 15h ax=e801h, int 15h ax=e820h

    call        VgaDemo
    jmp         Halt

VgaDemo:

KEY_ARROWDOWN   = 0x5000
KEY_ARROWLEFT   = 0x4B00
KEY_ARROWRIGHT  = 0x4D00
KEY_ARROWUP     = 0x4800

_VgaDemo_Loop:
    call        ReadKey
    cmpw        $KEY_ARROWUP, %ax
    jz          _NextMode
    cmpw        $KEY_ARROWDOWN, %ax
    jz          _PrevMode
    cmpw        $KEY_ARROWRIGHT, %ax
    jz          _NextColor
    cmpw        $KEY_ARROWLEFT, %ax
    jz          _PrevColor
    leaw        s_InvalidInput, %si
    call        PrintLn
    jmp         _VgaDemo_Loop

_PrevMode:
    decb        m_VgaMode
    jmp         _SetMode

_NextMode:
    incb        m_VgaMode
    jmp         _SetMode

_PrevColor:
    decb        m_VgaColor
    jmp         _SetColor

_NextColor:
    incb        m_VgaColor
    jmp         _SetColor

_SetColor:
    movb        $0x0B, %ah
    movb        $0x00, %bh
    movb        m_VgaColor, %bl
    int         $0x10
    jmp         _VgaDemo_Loop

_SetMode:
    movb        $0, %ah
    movb        m_VgaMode, %al
    int         $0x10
    leaw        s_VideoMode, %si
    call        Print
    movzbw      m_VgaMode, %ax
    call        Num2Hex
    call        Print
    jmp         _VgaDemo_Loop


// -----------------------------------------------------------------------------
// Num2Hex
// -----------------------------------------------------------------------------
// Writes an unsigned number to a buffer as a zero-terminated hexadecimal
// string. The hexadecimal digits are suffixed with an 'h'.
//
// Inputs:
//         ax - number to stringify
// Outputs:
//      ds:si - string buffer address
// Clobbers:
//      ax, cx, dx
// TODO: start using C calling convention?
//
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
    movb        $0x68, 0(%si)   // 'h'
    movb        $0, 1(%si)
    movw        m_Num2HexBuf, %si
    ret

m_Num2HexLut:
    .ascii      "0123456789ABCDEF"

m_Num2HexBuf:
    .quad       0


// -----------------------------------------------------------------------------
// VgaGetMode
// -----------------------------------------------------------------------------
// Gets the current VGA mode using BIOS interrupt int 10h.
//
// Inputs:
//      (none)
// Outputs:
//      al - video mode
//      ah - number of columns per line
//      bh - active display page number
// Clobbers:
//      ???
//
.globl VgaGetMode
VgaGetMode:
    movb        $0x0F, %ah
    int         $0x10
    movb        %al, m_VgaMode
    movb        %ah, m_VgaNumCols
    movb        %bh, m_VgaPage
    ret



m_VgaMode:
    .byte       0

m_VgaNumCols:
    .byte       0

m_VgaPage:
    .byte       0

m_VgaColor:
    .byte       0


m_EquipmentFlags:
    //    [0] - diskette drive installed
    //    [1] - math coprocessor (x87) installed
    //    [2] - PS/2 mouse installed
    //    [3] - (unused)
    //  [5:4] - initial video mode (00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono)
    //  [7:6] - num diskette drives attached less 1
    //    [8] - (unused) (previously DMA access installed, or DMA access NOT installed)
    // [11:9] - num serial ports installed
    //   [12] - game port installed
    //   [13] - (unused) (previously serial printer attached, or internal modem installed)
    //[15:14] - num parallel ports installed
    .word       0

m_KbContiguousRam:
    .word       0

s_HelloFromStage2:
    .asciz      "\r\nHello from stage 2!"
s_InvalidInput:
    .asciz      "Invalid input!"
s_VideoMode:
    .asciz      "Video Mode: "
s_EquipmentFlags:
    .asciz      "Equipment Flags: "
s_KbRam:
    .asciz      "Contiguous RAM: "
.code16

// TODO: glean as much information about the system as possible from the
// BIOS system calls before switching to protected mode

.globl Stage2
Stage2:
    xorw        %ax, %ax
    leaw        s_Hello2, %si
    call        PrintLn
    call        ReadKey
    movb        $0x0B, %ah
    movb        $0x00, %bh
    movb        m_Color, %bl
    int         $0x10
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
    leaw        s_Error, %si
    call        PrintLn
    jmp         _VgaDemo_Loop

_PrevMode:
    decw        m_VideoMode
    jmp         _SetMode

_NextMode:
    incw        m_VideoMode
    jmp         _SetMode

_PrevColor:
    decw        m_Color
    jmp         _SetColor

_NextColor:
    incw        m_Color
    jmp         _SetColor

_SetColor:
    movb        $0x0B, %ah
    movb        $0x00, %bh
    movb        m_Color, %bl
    int         $0x10
    jmp         _VgaDemo_Loop

_SetMode:
    movb        $0, %ah
    movb        m_VideoMode, %al
    int         $0x10
    leaw        s_NewMode, %si
    call        Print
    movw        m_VideoMode, %ax
    subw        $8, %sp
    movw        %sp, %di
    call        Num2Hex
    movw        %di, %si
    call        Print
    addw        $8, %sp
    jmp         _VgaDemo_Loop

.globl Num2Hex
Num2Hex:
    pushw       %di
    movw        $4, %cx

_Num2Hex_Loop:
    xorl        %edx, %edx
    rolw        $4, %ax
    movw        %ax, %dx
    andw        $0x0F, %dx

    movb        s_Num2Hex(%edx), %dl
    // jz          _Num2Hex_LoopCheck
    movb        %dl, (%di)
    incw        %di

_Num2Hex_LoopCheck:
    decw        %cx
    jnz         _Num2Hex_Loop

_Num2Hex_Done:
    movb        $0x68, 0(%di)
    movb        $0, 1(%di)
    popw        %di
    ret

s_Hello2:
    .asciz      "Got to stage 2!"
s_Error:
    .asciz      "Invalid entry!"
s_NewMode:
    .asciz      "\rVideo Mode: "

s_Num2Hex:
    .ascii      "0123456789ABCDEF"

m_VideoMode:
    .word       0

m_Color:
    .word       1
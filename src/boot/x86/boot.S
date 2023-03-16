.code16

//
// Global entry point.
// Do NOT move or add to this.
//
.globl Entry
Entry:
        jmp             Boot
        nop

//
// BIOS Parameter Block for a FAT12-formatted 3.5" 1.44MB floppy disk.
// Do NOT move this. Tweak with care.
//
OemName:                .ascii      "WHAMPSON"
SectorSize:             .short      512
SectorsPerCluster:      .byte       1
ReservedSectorCount:    .short      1
TableCount:             .byte       2
RootDirCapacity:        .short      224
SectorCount:            .short      2880
MediaType:              .byte       0xF0
SectorsPerTable:        .short      9
SectorsPerTrack:        .short      18
HeadCount:              .short      2
HiddenSectorCount:      .int        0
SectorCountLarge:       .int        0
DriveNumber:            .byte       0
_Reserved:              .byte       0
Signature:              .byte       0x29
VolumeId:               .int        0xCA55E77E
Label:                  .ascii      "OH-WES     "
FsType:                 .ascii      "FAT12   "

//
// Initial bootloader code.
// Gotta be tight; file must assemble to exactly 512 bytes!
// TODO: assert label address
Boot:
        cli
        cld
        xorw            %ax, %ax
        movw            %ax, %ds
        movw            %ax, %es
        leaw            s_Loading, %si
        call            Print

        // Read stage 2 loader
        // TODO: load from FAT
        // TOOD: chs to lba
        movw            $0x7E00, %bx    // dstBuf
        movb            $0, %dh         // head
        movb            $0, %ch         // cylinder
        movb            $2, %cl         // sector
        movb            $16, %al         // count
        call            ReadSectors

        jmp             Stage2


//
// ~~~ Subroutines
//

.global ReadSectors
ReadSectors:
        movb            DriveNumber, %dl
        movb            $0x02, %ah      // command: read sectors from drive
        int             $0x13
        jc              _DiskError
        ret
_DiskError:
        leaw            s_DiskError, %si
        call            Print
        jmp             Halt

// -----------------------------------------------------------------------------
// Print
// -----------------------------------------------------------------------------
// Prints a zero-terminated string at the current curusor location and
// advances the cursor.
//
// Inputs:
//      ds:si - address of string to print
// Clobbers:
//      ax, bx
//
.globl Print
Print:
        xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              _PrintDone
        int             $0x10
        jmp             Print
_PrintDone:
        ret

.globl PrintLn
PrintLn:
        call            Print
        leaw            s_Newline, %si
        call            Print
        ret

// -----------------------------------------------------------------------------
// ReadKey
// -----------------------------------------------------------------------------
// Waits for keyboard input.
//
// Returns:
//      al - key pressed
// Clobbers:
//      ax
//
.globl ReadKey
ReadKey:
        xorb            %ah, %ah
        int             $0x16
        ret

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
// Performs a warm reboot.
//
.globl Reset
Reset:

RESET_STATE_ADDR        = 0x0472
RESET_STATE_NOMEMTEST   = 0x1234  # like Ctrl+Alt+Del, use 0x0 for "cold" boot
RESET_VECTOR_SEG        = 0xF000
RESET_VECTOR_ADDR       = 0xFFF0

        xorl            %eax, %eax
        movw            %ax, %es
        movw            $RESET_STATE_ADDR, %ax
        movw            $RESET_STATE_NOMEMTEST, %es:(%eax)
        ljmp            $RESET_VECTOR_SEG, $RESET_VECTOR_ADDR

// -----------------------------------------------------------------------------
// ReadKeyReset
// -----------------------------------------------------------------------------
// Waits for keyboard input, then resets the computer.
//
.globl ReadKeyReset
ReadKeyReset:
        call            ReadKey
        jmp             Reset

// -----------------------------------------------------------------------------
// Halt
// -----------------------------------------------------------------------------
// Idle the CPU.
//
.globl Halt
Halt:
        hlt
        jmp             Halt

s_Newline:
        .asciz          "\r\n"
s_Loading:
        .asciz          "\r\nLoading..."
s_DiskError:
        .asciz          "\r\nDiskette read error!"

// TODO: ensure file does not grow beyond 512B

_Pad:                   .space      0x1FE - (. - Entry)
Sig:                    .short      0xAA55

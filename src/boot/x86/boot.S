.code16         // Oh yeah, it's real.

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

#TODO: load a file from disk

# Idea: When the image is built, OS.SYS will be placed at a known sector
# location on disk. We load the DirEntry from a known location (root dir slot)
# and read N sectors based on size into memory at a known address.
#
# Might need to load a STAGE2 file (BOOT.SYS) containing additional 16-bit code
# required for system initialization.
#
# Switched to Protected Mode and call 32-bit kernel (kmain).


        // read first root dir sector
        // TOOD: chs to lba
        movw            %sp, %bx        // dstBuf
        movb            $1, %dh         // head
        movb            $0, %ch         // cylinder
        movb            $2, %cl         // sector
        movb            $1, %al         // count
        call            ReadSector

        // TODO: handle error

        movw            %sp, %bx
        movw            26(%bx), %cx    // firstCluster
        // movl            28(%bx), %eax   // fileSize


        movw            %sp, %bx        // dstBuf
        movb            $1, %dh         // head
        movb            $0, %ch         // cylinder
        movb            $2, %cl         // sector
        movb            $1, %al         // count

        call            KeyPress
        jmp             Reset



        jmp             Halt


//
// ~~~ Subroutines
//

.global ReadSector
ReadSector:
        movb            (DriveNumber), %dl
        movb            $0x02, %ah      // command: read sectors from drive
        int             $0x13
        jc              DiskError
        leaw            s_Dot, %si
        call            Print
        ret
.globl DiskError
DiskError:
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
        // xorw            %ax, %ax
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              _Print_Done
        int             $0x10
        jmp             Print
_Print_Done:
        ret

// -----------------------------------------------------------------------------
// Keypress
// -----------------------------------------------------------------------------
// Waits for keyboard input.
//
// Returns:
//      al - key pressed
// Clobbers:
//      ax
//
.globl KeyPress
KeyPress:
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
        leaw            S_Reset, %si
        call            Print
        xorl            %eax, %eax
        movw            %ax, %es
        movw            $0x0472, %ax            # Reset State Flag
        movw            $0x1234, %es:(%eax)     # 0x1234 = Skip Memory Test
        ljmp            $0xF000, $0xFFF0        # Reset Vector

// -----------------------------------------------------------------------------
// Halt
// -----------------------------------------------------------------------------
// Idle the CPU.
//
.globl Halt
Halt:
        hlt
        jmp             Halt

s_Dot:
        .asciz          "."
S_Reset:
        .asciz          "Resetting..."
s_DiskError:
        .asciz          "\r\nDisk read error."

// TODO: ensure file does not grow beyond 512B

_Pad:                   .space      0x1FE - (. - Entry)
Sig:                    .short      0xAA55

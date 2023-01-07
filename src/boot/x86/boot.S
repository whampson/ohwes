.code16

.globl Entry
Entry:
        jmp             BootCode
        nop

// -----------------------------------------------------------------------------
// FAT12 BIOS Parameter Block
// -----------------------------------------------------------------------------

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
Label:                  .ascii      "OH-WES 0.1 "
FsType:                 .ascii      "FAT12   "

// -----------------------------------------------------------------------------
// Boot Loader
// -----------------------------------------------------------------------------

BootCode:
        cld
        xorw            %ax, %ax
        movw            %ax, %ds
        leaw            Message, %si

PrintLoop:
        movb            $0x0E, %ah
        movw            $0x07, %bx
        lodsb
        andb            %al, %al
        jz              KeyPress
        int             $0x10
        jmp             PrintLoop

KeyPress:
        xorb            %ah, %ah
        int             $0x16
        int             $0x19

Halt:
        jmp             Halt

Message:
        .ascii          "\r\nThis disk is not bootable!"
        .ascii          "\r\nPlease insert a bootable disk, "
        .asciz          "then press any key to try again...\r\n"

// -----------------------------------------------------------------------------
// Padding/Signature
// -----------------------------------------------------------------------------

_Pad:                   .space      0x1FE - (. - Entry)
BootSignature:          .short      0xAA55

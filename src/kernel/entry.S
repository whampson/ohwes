.text
.code32

.globl KeEntry
KeEntry:
        cli
        movw    $0xd121, 0xB8000
        // TODO: set up stack at EBDA base
        // call    KeMain

.globl Halt
Halt:
        hlt
        jmp     Halt

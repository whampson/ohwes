.text
.code32

.globl KeEntry
KeEntry:
        movw    $0xa421, 0xB8000
        // call    OhWes

.globl Halt
Halt:
        hlt
        jmp     Halt

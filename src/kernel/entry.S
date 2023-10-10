.text
.code32

.globl KeEntry
KeEntry:
        cli
        // movw    $0xd121, 0xB8000

        # params: ecx = boot params ptr
        call    KeMain

.globl Halt
Halt:
        hlt
        jmp     Halt

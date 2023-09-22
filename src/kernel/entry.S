# TODO: kernel entry point.

.globl _start
_start:
        call    OhWes

.globl Halt
Halt:
        hlt
        jmp     Halt

.text
.code32

# TODO: ensure this starts at the very top of the kernel image.

.globl KeEntry
KeEntry:
        cli
        # params: ecx = boot params ptr
        call    KeMain

.globl Halt
Halt:
        hlt
        jmp     Halt

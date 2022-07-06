.code16

.global _x86_init
_x86_init:
    pushw   %cs
    popw    %ds
    jmp     halt

halt:
    jmp halt
# Dummy kernel, will be removed soon...

.code16

.global kmain
kmain:
    pushw   %cs
    popw    %ax
    addw    $0x1000, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    $0, %si
    call    print

retry_boot:
    xorb    %ah, %ah
    int     $0x16
    int     $0x19

halt:
    hlt
    jmp     halt


print:
    movb    $0x0E, %ah
    movw    $0x07, %bx
_print_loop:
    lodsb
    andb    %al, %al
    jz      _print_done
    int     $0x10
    jmp     _print_loop
_print_done:
    ret

.org 0x10000, 0xA5
msg:        .asciz          "You've entered the dummy kernel!\r\n"

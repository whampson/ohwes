# Dummy kernel, will be removed soon...

.code16

.global kmain
kmain:
    pushw   %cs
    popw    %ds
    leaw    msg, %si
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

msg:        .asciz          "You've entered the dummy kernel!\r\n"

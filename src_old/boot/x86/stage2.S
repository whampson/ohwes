.code16

Stage2:
    xorw        %ax, %ax
    leaw        Hello2, %si
    call        Print
    call        Halt

Hello2:
    .asciz      "\r\nGot to stage 2!!!\r\n"

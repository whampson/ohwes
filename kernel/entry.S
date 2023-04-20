.text
.code32

.globl kentry
kentry:

        call            init            # initialize devices and load kernel

idle_loop:
        jmp             idle_loop

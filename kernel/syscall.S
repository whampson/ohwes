#==============================================================================#
# Copyright (C) 2020-2021 Wes Hampson. All Rights Reserved.                    #
#                                                                              #
# This file is part of the OHWES Operating System.                             #
# OHWES is free software; you may redistribute it and/or modify it under the   #
# terms of the license agreement provided with this software.                  #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: kernel/syscall.S                                                    #
# Created: December 22, 2020                                                   #
#  Author: Wes Hampson                                                         #
#==============================================================================#

#include <errno.h>

.globl system_call
system_call:
    cmpl    $(sys_jmptbl_end-sys_jmptbl)>>2, %eax
    jge     nosys
    jmp     *sys_jmptbl(,%eax,4)

nosys:
    movl    $-ENOSYS, %eax
    ret

sys_jmptbl:
    .long   sys_test0
    .long   sys_test1
    .long   sys_test2
    .long   sys_test3
    .long   sys_test4
    .long   sys_test5
sys_jmptbl_end:

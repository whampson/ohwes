#==============================================================================#
# Copyright (C) 2020 Wes Hampson. All Rights Reserved.                         #
#                                                                              #
# This file is part of the Niobium Operating System.                           #
# Niobium is free software; you may redistribute it and/or modify it under     #
# the terms of the license agreement provided with this software.              #
#                                                                              #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL      #
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER   #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER          #
# DEALINGS IN THE SOFTWARE.                                                    #
#==============================================================================#
#    File: kernel/entry.S                                                      #
# Created: December 9, 2020                                                    #
#  Author: Wes Hampson                                                         #
#                                                                              #
# This is the kernel entry point. Stage 2 of the bootloader jumps here after   #
# enabling Protected Mode.                                                     #
#==============================================================================#

.section .entry
#include <nb/boot.h>
#include <nb/init.h>
#include <nb/memory.h>
#include <x86/paging.h>
#include <x86/cntrl.h>

.global kentry
kentry:
    movl    $KERN_STACK, %esp
    movl    %esp, %ebp
    call    enable_paging
    call    kmain

die:
    hlt
    jmp     die

enable_paging:
    movl    $PAGE_SIZE>>2, %ecx
    movl    $PGDIR, %edi
    xorl    %eax, %eax
    cld
    rep     stosl
    movl    $PGTBL0, %eax
    orl     $PDE_G, %eax
    orl     $PDE_RW, %eax
    orl     $PDE_P, %eax
    movl    %eax, PGDIR
    movl    $PGTBL0, %edx
    xorl    %ebx, %ebx
    xorl    %ecx, %ecx
_load_pgtbl0:
    cmpl    $PAGE_SIZE>>2, %ecx
    je      _load_cr
    movl    %ebx, %eax
    orl     $PTE_G, %eax
    orl     $PTE_RW, %eax
    orl     $PTE_P, %eax
    movl    %eax, (%edx, %ecx, 4)
    addl    $PAGE_SIZE, %ebx
    incl    %ecx
    jmp     _load_pgtbl0
_load_cr:
    movl    $PGDIR, %eax
    movl    %eax, %cr3
    movl    %cr0, %eax
    orl     $CR0_PG, %eax
    movl    %eax, %cr0
    movl    %cr4, %eax
    orl     $CR4_PSE, %eax
    orl     $CR4_PGE, %eax
    andl    $~CR4_PAE, %eax
    movl    %eax, %cr4
    ret

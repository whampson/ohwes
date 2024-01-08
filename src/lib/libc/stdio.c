/* =============================================================================
 * Copyright (C) 2020-2024 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: lib/libc/stdio.c
 *      Created: January 4, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <stdio.h>
#include <console.h>

void stdout_fn(char c)
{
    console_write(c);
}

int putchar(int c)
{
    // TODO: write to stdout, return EOF on error
    stdout_fn(c);
    return c;
}

int puts(const char *str)
{
    // TODO: write to stdout, return EOF on error
    while (*str) {
        stdout_fn(*str++);
    }
    stdout_fn('\n');
    return 1;
}

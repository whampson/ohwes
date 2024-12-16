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
 *         File: lib/stdio.c
 *      Created: January 4, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int putchar(int c)
{
    unsigned char ch;

    ch = (unsigned char) c;
    if (write(STDOUT_FILENO, &ch, 1) < 0) {
        return -1;  // == EOF
    }

    return ch;
}

int puts(const char *str)
{
    // TODO: write string to a buffer then write buffer in chunks

    int nwritten, ret;

    nwritten = 0;
    ret = write(STDOUT_FILENO, str, strlen(str));
    if (ret < 0) {
        return -1;  // == EOF
    }
    nwritten += ret;

    char nl ='\n';
    ret = write(STDOUT_FILENO, &nl, 1);
    if (ret < 0) {
        return -1;
    }
    nwritten += ret;

    return nwritten;
}

void perror(const char *s)
{
    printf("%s: ", s);
    puts(strerror(errno));
}

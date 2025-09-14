/* =============================================================================
 * Copyright (C) 2020-2025 Wes Hampson. All Rights Reserved.
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
 *         File: usr/shell.c
 *      Created: September 8, 2025
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/termios.h>     // TODO: just <termios.h>?
#include <kernel/ioctls.h>      // TODO: some user-mode location for this

#define LINE_LENGTH     64
#define PROMPT_CHAR     '#'

static int cat(int argc, char *argv[]);

#define SYS_RIF(x) \
do { \
    int __ret; \
    if ((__ret = (x)) < 0) { \
        perror("\e[1;31merror\e[0m: " #x); \
        return -__ret; \
    } \
} while (0)

// basic shell for testing kernel interfaces
// TODO: make this a real user-mode program on disk!

static int parse_command(char *line, size_t len);

int shell(void)
{
    struct termios termios, orig_termios;

    SYS_RIF(ioctl(STDIN_FILENO, TCGETS, &termios));
    orig_termios = termios;
    termios.c_iflag = (ICRNL);              // in  CR->NL
    termios.c_oflag = (OPOST | ONLCR);      // out NL->CRNL
    termios.c_lflag = ~(ECHO | ECHOCTL);    // disable echo
    SYS_RIF(ioctl(STDIN_FILENO, TCSETS, &termios));

    size_t len = 0;
    char line[LINE_LENGTH];
    char c;
    int ret;

    do {
        putchar(PROMPT_CHAR);

        do {
            ret = read(STDIN_FILENO, &c, 1);
            if (ret < 0) {
                break;
            }
            switch (c) {
            case '\b': case 0x7F:               // TODO: ^H vs ^? distinction
                if (len > 0) {                  // I'm surprised there isn't a termios flag for this...
                    if (iscntrl(line[len-1])) {
                        printf("\b\b  \b\b");
                    }
                    else {
                        printf("\b \b");
                    }
                    line[--len] = '\0';
                }
                continue;
            case '\t':
                continue;
            default:
                break;
            }

            if (c == '\n' || len <= LINE_LENGTH - 2) {
                line[len++] = c;
                if (c != '\n' && iscntrl(c)) {
                    printf("^%c", c ^ 0x40);
                }
                else {
                    putchar(c);
                }
                if (c == 3) {
                    goto quit;
                }
            }
        } while (c != '\n');
        assert(line > 0);

        line[--len] = '\0'; // chop off that newline
        if (len > 0) {
            ret = parse_command(line, len);
            if (ret < 0) {
                goto quit;
            }
        }
        len = 0;
    } while (true);

quit:
    putchar('\n');
    SYS_RIF(ioctl(STDIN_FILENO, TCSETS, &orig_termios));    // TODO: atexit
    return 0;
}

static int parse_command(char *line, size_t len)
{
    int argc = 0;
    char *argv[64];

    for (char *c = line; argc < sizeof(argv); c = NULL, argc++) {
        argv[argc] = strtok(c, " ");
        if (argv[argc] == NULL) {
            break;
        }
    }

    int ret = 0;
    if (strcmp("exit", argv[0]) == 0) {
        ret = -1;
    }
    else if (strcmp("cat", argv[0]) == 0) {
        ret = cat(argc, argv);
    }
    else {
        printf("error: unknown command '%.*s'\n", len, argv[0]);
        ret = 1;
    }

    return ret;
}

#define ERR(...) \
    printf("%s: error: ", __FUNCTION__); \
    printf(__VA_ARGS__)

#define BUFSIZ 512

static int cat(int argc, char *argv[])
{
    int fd;
    ssize_t count;
    char buf[BUFSIZ];

    assert(strcmp("cat", argv[0]) == 0);

    if (argc < 2) {
        ERR("missing argument\n");
        return 1;
    }
    argv++;

    count = 0;
    do {
        fd = open(*argv, O_RDWR);
        if (fd < 0) {
            ERR("could not open file '%s'\n", **argv);
            return 2;
        }
        while ((count = read(fd, buf, BUFSIZ)) > 0) {
            write(STDOUT_FILENO, buf, count);
        }
        close(fd);
        argc--; argv++;
    } while (argc > 1);

    return 0;
}

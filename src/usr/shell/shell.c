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
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/termios.h>     // TODO: just <termios.h>?
#include <kernel/ioctls.h>      // TODO: some user-mode location for this

#define NR_ARGV         64
#define LINE_LENGTH     64
#define ARG_LENGTH      LINE_LENGTH
#define HISTORY_SIZE    32
#define PROMPT_CHAR     '#'
#define BUFSIZ          512

// syscall return if failed
#define SYS_CHECK(x) \
do { \
    int __ret; \
    if ((__ret = (x)) < 0) { \
        perror("\e[1;31merror\e[0m: " #x); \
        return -__ret; \
    } \
} while (0)

// basic shell for testing kernel interfaces
// TODO: make this a real user-mode program on disk!

// this is kind of like a process context...
struct shell_context {
    int argc;
    char *argv[NR_ARGV];
    char *stdout_path;
    char history[LINE_LENGTH][HISTORY_SIZE];
    int history_index;
    int history_ptr;
};

static int cat(int argc, char *argv[]);
static int echo(struct shell_context *ctx);
static int cereal(int argc, char *argv[]);

static int parse_command(struct shell_context *ctx, char *line, size_t len);

int shell(void)
{
    struct termios termios, orig_termios;

    SYS_CHECK(ioctl(STDIN_FILENO, TCGETS, &termios));
    orig_termios = termios;
    termios.c_iflag = (ICRNL);              // in  CR->NL
    termios.c_oflag = (OPOST | ONLCR);      // out NL->CRNL
    termios.c_lflag = ~(ECHO | ECHOCTL);    // disable echo
    SYS_CHECK(ioctl(STDIN_FILENO, TCSETS, &termios));

    size_t len = 0;
    char line[LINE_LENGTH];
    char c;
    int ret;
    bool esc, csi;

    struct shell_context _ctx = { };
    struct shell_context *ctx = &_ctx;
    esc = false; csi = false;

    do {
        putchar(PROMPT_CHAR);
        do {
        next_char:
            SYS_CHECK(read(STDIN_FILENO, &c, 1) < 0);
            switch (c) {
            case '\e':
                esc = true;
                goto next_char;
            case '\b': case 0x7F:   // TODO: ^H vs ^? distinction
                if (len > 0) {      // I'm surprised there isn't a standard termios flag for this...
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

            if (esc) {
                if (!csi && c == '[') {
                    csi = true;
                    goto next_char;
                }
                if (csi && (c == 'A' || c == 'B')) {
                    if (c == 'A') {
                        ctx->history_ptr -= 1;
                        if (ctx->history_ptr < 0) {
                            ctx->history_ptr = HISTORY_SIZE - 1;
                        }
                    }
                    else {
                        ctx->history_ptr += 1;
                        if (ctx->history_ptr >= HISTORY_SIZE) {
                            ctx->history_ptr = 0;
                        }
                    }
                    strncpy(line, ctx->history[ctx->history_ptr], LINE_LENGTH);
                    len = strnlen(line, LINE_LENGTH);
                    printf("\r\e[2K%c%.*s", PROMPT_CHAR, len, line);
                }
                else {
                    printf("^[");
                }
            // esc_done:
                esc = false;
                csi = false;
                continue;
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
            ret = parse_command(ctx, line, len);
            if (ret < 0) {
                goto quit;
            }
        }
        len = 0;
    } while (true);

quit:
    putchar('\n');
    SYS_CHECK(ioctl(STDIN_FILENO, TCSETS, &orig_termios));    // TODO: atexit
    return 0;
}

static int parse_command(struct shell_context *ctx, char *line, size_t len)
{
    ctx->argc = 0;
    ctx->stdout_path = NULL;
    bool redir_stdout = false;

    strncpy(ctx->history[ctx->history_index++], line, len);
    if (ctx->history_index >= HISTORY_SIZE) {
        ctx->history_index = 0;
    }

    for (char *c = line; ctx->argc < sizeof(ctx->argv); c = NULL) {
        char *tok = strtok(c, " ");
        if (tok == NULL) {
            ctx->argv[ctx->argc] = NULL;
            break;
        }
        if (strcmp(tok, ">") == 0) {
            redir_stdout = true;
            continue;
        }
        if (redir_stdout) {
            ctx->stdout_path = tok;
            break;
        }
        ctx->argv[ctx->argc] = tok;
        ctx->argc++;
    }

    char *cmd = ctx->argv[0];
    if (strcmp("exit", cmd) == 0) {
        return -1;
    }

    int ret = 0;
    // TODO: spawn program using exec()
    //   pass stdout_path to new process if set
    if (strcmp("echo", cmd) == 0) {
        echo(ctx);
    }
    else if (strcmp("cat", cmd) == 0) {
        cat(ctx->argc, ctx->argv);
    }
    else if (strcmp("cereal", cmd) == 0) {
        cereal(ctx->argc, ctx->argv);
    }
    else {
        printf("error: unknown command '%.*s'\n", len, cmd);
        ret = -1;
    }

    if (ret) {
        ctx->history_index--;
        if (ctx->history_index < 0) {
            ctx->history_index = 0;
        }
    }
    ctx->history_ptr = ctx->history_index;

    return 0;   // "don't exit shell"
}

//
// -------------------------------------------------------------------------
//

// print message with function prefix
#define CMD_PRINT(...) \
    printf("%s: ", __FUNCTION__); \
    printf(__VA_ARGS__)

#define ERROR_ARG   1
#define ERROR_IO    2

static int cat(int argc, char *argv[])  // TODO: make this a standalone executable
{
    int fd;
    ssize_t count;
    char buf[BUFSIZ];
    int ret;

    assert(strcmp("cat", argv[0]) == 0);

    if (argc < 2) {
        CMD_PRINT("missing argument\n");
        return ERROR_ARG;
    }
    argv++;

    ret = 0;
    count = 0;
    do {
        fd = open(*argv, O_RDWR);
        if (fd < 0) {
            CMD_PRINT("%s: %s\n", *argv, strerror(errno));
            return ERROR_IO;
        }
        while ((count = read(fd, buf, BUFSIZ)) > 0) {
            if (count == 1 && *buf == 3) {
                break;  // Ctrl+C terminate...
            }
            if (write(STDOUT_FILENO, buf, count) < 0) {
                CMD_PRINT("%s\n", strerror(errno));
                ret = ERROR_IO;
                break;
            }
        }
        if (count < 0) {
            CMD_PRINT("%s\n", strerror(errno));
            ret = ERROR_IO;
        }
        (void) close(fd);
        argc--; argv++;
    } while (argc > 1 && !ret);

    return ret;
}

static int echo(struct shell_context *ctx)
{
    int ret;
    int fd;

    fd = STDOUT_FILENO;
    if (ctx->stdout_path) {
        fd = open(ctx->stdout_path, O_WRONLY);
        if (fd < 0) {
            CMD_PRINT("%s: %s\n", ctx->stdout_path, strerror(errno));
            return ERROR_IO;
        }
    }

    ret = 0;
    for (int i = 1; i < ctx->argc; i++) {
        size_t count = strnlen(ctx->argv[i], ARG_LENGTH);
        if (write(fd, ctx->argv[i], count) < 0) {
            CMD_PRINT("%s\n", strerror(errno));
            ret = ERROR_IO;
            break;
        }
        if (i < ctx->argc - 1) {
            if (write(fd, " ", 1) < 0) {
                CMD_PRINT("%s\n", strerror(errno));
                ret = ERROR_IO;
                break;
            }
        }
    }
    if (!ret) {
        if (write(fd, "\n", 1) < 0) {
            CMD_PRINT("%s\n", strerror(errno));
            ret = ERROR_IO;
        }
    }

    if (ctx->stdout_path) {
        if (close(fd) < 0) {
            CMD_PRINT("%s: %s\n", ctx->stdout_path, strerror(errno));
            ret = ERROR_IO;
        }
    }

    return ret;
}

static int cereal(int argc, char *argv[])
{
    int ret;
    int fd;
    int status;
    char buf[BUFSIZ];

    assert(strcmp("cereal", argv[0]) == 0);

    if (argc < 2) {
        CMD_PRINT("missing argument\n");
        return ERROR_ARG;
    }
    argv++;

    fd = open(*argv, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        CMD_PRINT("%s: %s\n", *argv, strerror(errno));
        return ERROR_IO;
    }

    ret = ioctl(fd, TIOCMGET, &status);
    if (ret || !(status & TIOCM_CAR)) {
        CMD_PRINT("%s: No device attached\n", *argv);
        ret = ERROR_IO;
        goto close_out;
    }

    ret = 0;
    do {
        // read serial TTY, nonblocking
        ret = read(fd, buf, sizeof(buf));
        if (ret < 0 && errno != EAGAIN) {
            CMD_PRINT("%s: read(TTY): %s\n", *argv, strerror(errno));
            ret = ERROR_IO;
            break;
        }

        // write received chars from serial TTY to stdout
        if (ret > 0) {
            write(STDOUT_FILENO, buf, ret);
        }

        // read stdin, nonblocking
        ret = read(STDIN_FILENO, buf, sizeof(buf));
        if (ret < 0 && errno != EAGAIN) {
            CMD_PRINT("%s: read(0): %s\n", *argv, strerror(errno));
            ret = ERROR_IO;
            break;
        }

        // write received chars from stdin to serial TTY
        if (ret > 0) {
            write(fd, buf, ret);
        }
    } while (*buf != 3);   // quit if CTRL+C pressed        TODO: handle SIGINT/SIGHUP

close_out:
    close(fd);
    return ret;
}

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
#include <kernel/console.h>
#include <kernel/terminal.h>
#include <kernel/termios.h>     // TODO: just <termios.h>?
#include <kernel/ioctls.h>      // TODO: some user-mode location for this
#include <kernel/rtc.h>
#include <sys/ohwes.h>

#define NR_ARGV         64
#define LINE_LENGTH     128
#define HISTORY_SIZE    32
#define PROMPT_CHAR     '#'

// syscall return if failed
#define SYS_CHECK(x) \
({ \
    int __ret; \
    if ((__ret = (x)) < 0) { \
        printf("%s: error: " #x ": %s\n", __FUNCTION__, strerror(errno)); \
        return errno; \
    } \
    __ret; \
})

#define SYS_CHECK_GOTO(x, label) \
({ \
    int __ret; \
    if ((__ret = (x)) < 0) { \
        printf("%s: error: " #x ": %s\n", __FUNCTION__, strerror(errno)); \
        goto label; \
    } \
    __ret; \
})

// basic shell for testing kernel interfaces
// TODO: make this a real user-mode program on disk!

// this is kind of like a process context...
struct shell_context {
    int argc;
    char *argv[NR_ARGV];
    char *stdout_path;
};

static unsigned s_history_head = 0;
static int s_history_count = 0;
static char s_history[HISTORY_SIZE][LINE_LENGTH];
static_assert((HISTORY_SIZE & (HISTORY_SIZE - 1)) == 0, "HISTORY_SIZE must be a power of 2");

static struct winsize s_winsz;

static int cat(int argc, char *argv[]);
static int echo(struct shell_context *ctx);
static int cereal(int argc, char *argv[]);
static int counter(int argc, char *argv[]);
static int raw(int argc, char *argv[]);
static int rtc(int argc, char *argv[]);
static int termtest(int argc, char *argv[]);

static int parse_command(struct shell_context *ctx, char *line, size_t len);

static int history_slot(int n)
{
    return (s_history_head - 1 - n + HISTORY_SIZE) % HISTORY_SIZE;
}

static void history_insert(const char *line)
{
    if (!line || *line == '\0') {
        return;
    }

    if (s_history_count > 0 &&
        strncmp(s_history[history_slot(0)], line, LINE_LENGTH) == 0) {
        return;     // don't store consecutive duplicates
    }

    strcpy(s_history[s_history_head], line);
    s_history_head = (s_history_head + 1) % HISTORY_SIZE;
    if (s_history_count < HISTORY_SIZE) {
        s_history_count++;
    }
}

static void echo_char(char c)
{
    if (c != '\n' && iscntrl(c)) {
        printf("^%c", c ^ 0x40);
    }
    else {
        putchar(c);
    }
}

static void cursor_offset(size_t len, int *row, int *col)
{
    int n = (int) len + 1;
    int cols = s_winsz.ws_col;
    int pos = (n % cols == 0) ? (n - 1) : n;
    *row = pos / cols;
    *col = pos % cols;
}

static void redraw_line(const char *text, size_t len, size_t old_len)
{
    int drow, dcol;
    cursor_offset(old_len, &drow, &dcol);

    if (drow > 0) printf("\e[%dA", drow);      // up to the prompt's row
    if (dcol > 1) printf("\e[%dD", dcol - 1);  // left to the input's first column

    printf("\e[K");                            // erase prompt row, from col 1
    for (int i = 0; i < drow; i++) {
        printf("\e[B\e[D\e[K");                // erase each wrapped row, from col 0
    }
    if (drow > 0) {                            // back to input start
        printf("\e[%dA", drow);
        printf("\e[C");
    }

    for (size_t i = 0; i < len; i++) {
        echo_char(text[i]);
    }
}

int shell(void)
{
    struct termios termios, orig_termios;

    SYS_CHECK(ioctl(STDOUT_FILENO, TIOCGWINSZ, &s_winsz));
    if (s_winsz.ws_col == 0) {
        s_winsz.ws_col = 80;
    }

    SYS_CHECK(ioctl(STDIN_FILENO, TCGETS, &termios));
    orig_termios = termios;
    termios.c_iflag = (ICRNL | IXON | IXOFF);   // in  CR->NL, input/output flow control
    termios.c_oflag = (OPOST | ONLCR);          // out NL->CRNL
    termios.c_lflag = ~(ECHO | ECHOCTL);        // disable echo
    SYS_CHECK(ioctl(STDIN_FILENO, TCSETS, &termios));

    size_t len = 0;
    char line[LINE_LENGTH];
    char saved[LINE_LENGTH];
    bool has_saved = false;
    int history_pos = 0;
    char c;
    int ret;

    struct shell_context _ctx = { };
    struct shell_context *ctx = &_ctx;

    do {
    new_prompt:
        putchar(PROMPT_CHAR);
        printf("\e[s"); // save cursor position
        do {
        next_char:
            SYS_CHECK(read(STDIN_FILENO, &c, 1) < 0);
            switch (c) {
            case '\b': case 0x7F:   // TODO: ^H vs ^? distinction
                if (len > 0) {      // I'm surprised there isn't a standard termios flag for this...
                    if ((len % s_winsz.ws_col) == s_winsz.ws_col - 1) {
                        printf("\eM");
                        line[--len] = '\0';
                    }
                    else {
                        printf("\b \b");
                        line[--len] = '\0';
                    }
                }
                goto next_char;

            case 0x0C:  // Ctrl+L, clear screen
                printf("\e[2J\e[H\e[K");
                goto new_prompt;
            case '\t':
                goto next_char;

            case '\e': {
                char c1, c2;
                size_t old_len = len;
                SYS_CHECK(read(STDIN_FILENO, &c1, 1) < 0);
                SYS_CHECK(read(STDIN_FILENO, &c2, 1) < 0);

                if (c1 == '[' && c2 == 'A') {
                    // Up
                    if (history_pos == 0) {
                        line[len] = 0;
                        strcpy(saved, line);    // remember live edit
                        has_saved = true;
                    }
                    if (history_pos < s_history_count) {
                        history_pos++;
                        strcpy(line, s_history[history_slot(history_pos - 1)]);
                        len = strlen(line);
                        redraw_line(line, len, old_len);
                    }
                }
                else if (c1 == '[' && c2 == 'B') {
                    // Down
                    if (history_pos > 0) {
                        history_pos--;
                        if (history_pos == 0) {
                            if (has_saved) {    // back at live edit, restore saved
                                strcpy(line, saved);
                            }
                            else {
                                line[0] = '\0';
                            }
                        }
                        else {
                            strcpy(line, s_history[history_slot(history_pos - 1)]);
                        }
                        len = strlen(line);
                        redraw_line(line, len, old_len);
                    }
                }
                continue;
            }
            default:
                break;
            }

            if (c == '\n' || len <= LINE_LENGTH - 2) {
                line[len++] = c;
                if (c == '\n') {
                    putchar('\n');
                }
                else {
                    line[len] = '\0';
                    echo_char(c);
                }
                if (c == 3) {
                    goto quit;
                }
            }
        } while (c != '\n');

        assert(len > 0);
        line[--len] = '\0'; // chop off that newline

        if (len > 0) {
            history_insert(line);
            history_pos = 0;
            has_saved = false;
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
    errno = 0;  // HACK! C runtime should reset errno when launching program
    if (strcmp("help", cmd) == 0) {
        printf("help\n");
        printf("cat\n");
        printf("cereal\n");
        printf("counter\n");
        printf("echo\n");
        printf("exit\n");
        printf("hello\n");
        printf("raw\n");
        printf("rtc\n");
        printf("termtest\n");
        ret = 0;
    }
    else if (strcmp("echo", cmd) == 0) {
        ret = echo(ctx);
    }
    else if (strcmp("hello", cmd) == 0) {
        printf("Hello, world!\n");
    }
    else if (strcmp("cat", cmd) == 0) {
        ret = cat(ctx->argc, ctx->argv);
    }
    else if (strcmp("cereal", cmd) == 0) {
        ret = cereal(ctx->argc, ctx->argv);
    }
    else if (strcmp("counter", cmd) == 0) {
        ret = counter(ctx->argc, ctx->argv);
    }
    else if (strcmp("raw", cmd) == 0) {
        ret = raw(ctx->argc, ctx->argv);
    }
    else if (strcmp("rtc", cmd) == 0) {
        ret = rtc(ctx->argc, ctx->argv);
    }
    else if (strcmp("termtest", cmd) == 0) {
        ret = termtest(ctx->argc, ctx->argv);
    }
    else {
        printf("error: unknown command '%.*s'\n", (int) len, cmd);
    }

    // TODO: set return value to some equivalent of ERRORLEVEL/$?
    (void) ret;
    // if (ret) {
    //     printf("%s: returned %d\n", cmd, ret);
    // }

    return 0;   // 0 = "don't exit shell"
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
        size_t count = strnlen(ctx->argv[i], LINE_LENGTH);
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

static int counter(int argc, char *argv[])
{
    int stdin_orig_flags;
    int status;
    int count = 0;
    char c = 0;

    // we don't have signals yet, so we need poll the keyboard
    // for CTRL+C and the like so the user can exit :-)
    // to do that, we need to make sure STDIN is nonblocking

    // first, back up the old fd flags
    stdin_orig_flags = SYS_CHECK(fcntl(STDIN_FILENO, F_GETFL));

    // now, make stdin nonblocking
    SYS_CHECK_GOTO(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK), cleanup);

    // now, count until the user tells us to quit!
    while (true) {
        printf("%d\n", count++);

        // poll stdin for CTRL+C
        if ((status = read(STDIN_FILENO, &c, 1)) < 0) {
            if (errno != EAGAIN) {
                CMD_PRINT("error: read(stdin): %s\n", strerror(errno));
                break;
            }
        }
        else if (c == 0x3) {
            errno = 0;
            printf("CTRL+C pressed, exiting...\n");
            break;
        }
    }

cleanup:
    // set original stdin fd flags
    SYS_CHECK(fcntl(STDIN_FILENO, F_SETFL, stdin_orig_flags));
    return errno;
}

static void cfmakeraw(struct termios *termios_p)
{
    termios_p->c_iflag &= ~(INLCR|IGNCR|ICRNL|IXON|IGNBRK|PARMRK/*|BRKINT|ISTRIP*/);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO/*|ECHONL|ICANON|ISIG|IEXTEN*/);
    // termios_p->c_cflag &= ~(CSIZE|PARENB);
    // termios_p->c_cflag |= CS8;
}

static int raw(int argc, char *argv[])
{
    char c;
    int orig_flags, status;
    struct termios termios, orig_termios;

    orig_flags = SYS_CHECK(fcntl(STDIN_FILENO, F_GETFL));
    SYS_CHECK_GOTO(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK), quit);

    SYS_CHECK(ioctl(STDIN_FILENO, TCGETS, &termios));
    orig_termios = termios;
    cfmakeraw(&termios);
    SYS_CHECK_GOTO(ioctl(STDIN_FILENO, TCSETS, &termios), quit);

    while (true) {
        if ((status = read(STDIN_FILENO, &c, 1)) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            CMD_PRINT("read(stdin): %s\n", strerror(errno));
            break;
        }
        else if (c == 0x3) {
            break;
        }
        if (write(STDOUT_FILENO, &c, 1) < 0) {
            CMD_PRINT("write(stdout): %s\n", strerror(errno));
            break;
        }
    }

quit:
    putchar('\n');
    SYS_CHECK(fcntl(STDIN_FILENO, F_SETFL, orig_flags));
    SYS_CHECK(ioctl(STDIN_FILENO, TCSETS, &orig_termios));    // TODO: atexit
    return 0;
}

static void add_seconds(struct rtc_time *time, int seconds)
{
    time->tm_sec += seconds;
    if (time->tm_sec >= 60) {
        time->tm_min += time->tm_sec / 60;
        time->tm_sec %= 60;

    }
    if (time->tm_min >= 60) {
        time->tm_hour += time->tm_min / 60;
        time->tm_min %= 60;
    }
    if (time->tm_hour >= 24) {
        time->tm_hour %= 24;
    }
    // not doing the date lol sorry
}

static void print_datetime(struct rtc_time *dt)
{
    printf("%02d/%02d/%04d %02d:%02d:%02d", // american format >:)
        dt->tm_mon + 1, dt->tm_mday, dt->tm_year + 1900,
        dt->tm_hour, dt->tm_min, dt->tm_sec);
}

static void print_time(struct rtc_time *tm)
{
    printf("%02d:%02d:%02d",
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}

static int rtc(int argc, char *argv[])
{
    const int WaitTimeSec = 5;

    uint32_t rtc_data[1];
    struct rtc_time time;
    struct rtc_time orig_time;
    int fd;
    int rate;
    int ret;

    printf("rtc_test:\n");
    errno = 0;

    //
    // open RTC
    //
    fd = SYS_CHECK(open("/dev/rtc", 0));

    //
    // disable all interrupt types
    //
    SYS_CHECK(ioctl(fd, RTC_UPDATE_DISABLE, NULL));
    SYS_CHECK(ioctl(fd, RTC_IRQP_DISABLE, NULL));
    SYS_CHECK(ioctl(fd, RTC_ALARM_DISABLE, NULL));

    //
    // enable update interrupts (1 per sec)
    //
    SYS_CHECK(ioctl(fd, RTC_UPDATE_ENABLE, NULL));

    //
    // wait for n seconds, 1 interrupt per second
    //
    printf("waiting %d seconds using clock update interrupts...\n", WaitTimeSec);
    for (int i = 0; i < WaitTimeSec; i++) {
        ret = SYS_CHECK(read(fd, rtc_data, sizeof(rtc_data)));   // should block for 1s...
        assert(ret == sizeof(uint32_t));
        printf("!");
    }
    printf("\n");

    //
    // disable update interrupts
    //
    SYS_CHECK(ioctl(fd, RTC_UPDATE_DISABLE, NULL));

    //
    // set periodic rate and enable periodic interrupts
    //
    rate = RTC_RATE_4Hz;
    SYS_CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    SYS_CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_4Hz);
    SYS_CHECK(ioctl(fd, RTC_IRQP_ENABLE, NULL));

    //
    // wait approx n seconds, 'rate' interrupts per second
    //
    printf("waiting %d seconds using periodic interrupts at %d Hz...\n", WaitTimeSec, rate2hz(rate));
    for (int i = 0; i < WaitTimeSec * rate2hz(rate); i++) {
        ret = SYS_CHECK(read(fd, rtc_data, sizeof(rtc_data)));  // TODO: what should the data be...?
        assert(ret == sizeof(uint32_t));
        printf("%d ", i + 1);
    }
    printf("\n");

    //
    // set rate to highest possible and disable periodic interrupts
    //
    rate = RTC_RATE_8192Hz;
    SYS_CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    SYS_CHECK(ioctl(fd, RTC_IRQP_GET, &rate));
    assert(rate == RTC_RATE_8192Hz);
    SYS_CHECK(ioctl(fd, RTC_IRQP_DISABLE, NULL));

    //
    // get current time
    //
    SYS_CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("current date and time is ");
    print_datetime(&time);
    printf("\n");

    //
    // set alarm to n seconds ahead of current time, handle rollover
    //
    add_seconds(&time, WaitTimeSec);
    SYS_CHECK(ioctl(fd, RTC_ALARM_SET, &time));
    SYS_CHECK(ioctl(fd, RTC_ALARM_GET, &time));
    printf("alarm set to ring %d seconds from now at ", WaitTimeSec);
    print_time(&time);
    printf("\n");

    //
    // enable alarm interrupts
    //
    SYS_CHECK(ioctl(fd, RTC_ALARM_ENABLE, NULL));

    //
    // wait for alarm
    //
    printf("waiting for alarm to ring...\n");
    SYS_CHECK(read(fd, rtc_data, sizeof(rtc_data)));

    //
    // disable alarm interrupts
    //
    SYS_CHECK(ioctl(fd, RTC_ALARM_DISABLE, NULL));

    //
    // get time after alarm
    //
    SYS_CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("alarm rang at "); print_time(&time); printf("\n");

    //
    // set the time and read it back
    //
    orig_time = time;
    add_seconds(&time, 31337);
    SYS_CHECK(ioctl(fd, RTC_TIME_SET, &time));
    SYS_CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("time temporarily set to "); print_time(&time); printf("\n");

    //
    // wait for a bit 'cause why not? let's add a cool spinner!
    //
    rate = RTC_RATE_4Hz;
    SYS_CHECK(ioctl(fd, RTC_IRQP_SET, &rate));
    SYS_CHECK(ioctl(fd, RTC_IRQP_ENABLE, NULL));
    for (int i = 0; i < WaitTimeSec * rate2hz(rate); i++) {
        ret = SYS_CHECK(read(fd, rtc_data, sizeof(rtc_data)));
        assert(ret == sizeof(uint32_t));

        char *spinnybits = "-\\|/";
        printf("\r%c", spinnybits[i % 4]);
    }
    printf("\r");

    //
    // restore time, set periodic rate
    //
    add_seconds(&orig_time, WaitTimeSec);
    SYS_CHECK(ioctl(fd, RTC_TIME_SET, &orig_time));
    SYS_CHECK(ioctl(fd, RTC_TIME_GET, &time));
    printf("time restored to "); print_datetime(&time); printf("\n");

    //
    // close RTC
    //
    SYS_CHECK(ioctl(fd, RTC_IRQP_DISABLE, NULL));
    SYS_CHECK(ioctl(fd, RTC_ALARM_DISABLE, NULL));
    SYS_CHECK(ioctl(fd, RTC_UPDATE_ENABLE, NULL));
    ret = close(fd);
    assert(ret == 0);

    return ret;
}

static int getc(void)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) < 0) {
        if (errno == EAGAIN) {
            continue;
        }
        return 0;
    }
    return c;
}

static bool s_blink = false;

static void pause(void) { (void) getc(); }
static void clear_screen(void)  { printf("\e[2J\e[H"); }
static void disable_blink(void) { printf("\e[=6~"); }
static void enable_blink(void)  { printf("\e[=5~"); }
static void hide_cursor(void)   { printf("\e[?25l"); }
static void show_cursor(void)   { printf("\e[?25h"); }
static void reset_attr(void)    { printf("\e[0m"); }

static void draw_menubar(void)
{
    printf("\e[44;93m");
    printf("\e[99H\e[K");
    for (int i = 0; i < s_winsz.ws_col; i++) {
        printf(" ");
    }
    printf("\r");
    printf("  (b) Toggle Blink  \xB3");
    printf("  (c) Show Color Bars  \xB3");
    printf("  (Ctrl+C) Exit");
    printf("\e[0m");
}

static void draw_box(int top, int left, int height, int length)
{
    printf("\e[%d;%dH", top, left);
    for (int x = 0; x < (length+1) * 2; x++) {
        char c = '\xCD';    // TODO: box style select
        if ((x / 2) == 0) {
            c = (x % 2) ? '\xC8' : '\xC9';
        }
        else if ((x / 2) == length) {
            c = (x % 2) ? '\xBC' : '\xBB';
        }
        printf("%c%c", c, (x % 2) ? '\0' : '\b');
        printf("\e[%d%c", height, (x % 2) ? 'A' : 'B');
    }

    printf("\e[%d;%dH", top+1, left);
    for (int y = 0; y < (height-1) * 2; y++) {
        printf("\xBA");
        printf("\e[%d%c\e[%c", length - ((y % 2) ? -1 : 0),
            (y % 2) ? 'D' : 'C',
            (y % 2) ? 'B' : 'D');
    }
}

struct rect {
    int top, left;
    int height, length;
};


static void draw_box_r(const struct rect *r)
{
    draw_box(r->top, r->left, r->height, r->length);
}

static void show_terminal_test(void)
{
    hide_cursor();
    clear_screen();
    draw_menubar();

    const struct rect ColorBoxRect = { 2, 3, 17, 33 };

    draw_box_r(&ColorBoxRect);
    printf("\e[%d;%dH", ColorBoxRect.top+1, ColorBoxRect.left+1);

    for (int bg = 0; bg < 16; bg++) {
        printf("\e[0m\e[%d;%dH", ColorBoxRect.top+bg+1, ColorBoxRect.left+1);
        for (int fg = 0; fg < 16; fg++) {
            printf("\e[%d;%dm",
                (bg < 8) ? (40 + bg) : (100 + (bg - 8)),
                (fg < 8) ? (30 + fg) : (90 + (fg - 8)));
            printf("aa");
        }
    }

    struct style {
        int on, off;
        const char *name, *extra;
    };

    const struct style Styles[] = {
        {0, 0,  "Normal"},
        {1, 22, "Bold"},
        {2, 22, "Faint"},
        {3, 23, "Italic"},
        {4, 24, "Underline"},
        // {21,24, "Double Underline"},
        {5, 25, "Blinking"},
        // {6, 25, "Fast Blinking"},
        {7, 27, "Inverted"},
        {9, 29, "Strike"},
        {8, 28, "Hidden", "(hidden)"},
    };

    printf("\e[%d;%dH\e[0m\e[s", ColorBoxRect.top+1, ColorBoxRect.left+ColorBoxRect.length+4);

    for (int i = 0; i < countof(Styles); i++) {
        const struct style s = Styles[i];
        printf("An example of \e[%dm%s\e[%dm text. %s",
            s.on, s.name, s.off, (s.extra) ? s.extra : "");
        printf("\e[u\e[%dB", (i+1)*2);
    }
}

static void show_ansi_colorcard(void)
{
    const int CgaToAnsi[] = {
        0, // black
        4, // blue
        2, // green
        6, // cyan
        1, // red
        5, // magenta
        3, // yellow
        7  // white
    };

    disable_blink();
    clear_screen();
    for (int y = 0; y < s_winsz.ws_row; y++) {
        for (int x = 0; x < s_winsz.ws_col; x++) {
            printf("\e[%d%dm ", (y > s_winsz.ws_row / 2) ? 10 : 4,
                   CgaToAnsi[(x / 10) % countof(CgaToAnsi)]);
        }
        if (y < s_winsz.ws_row - 1) {
            printf("\r\n");
        }
    }
    reset_attr();
}

static int termtest(int argc, char *argv[])
{
    (void) argc; (void) argv;

    char c;
    int orig_flags, status;
    struct termios termios, orig_termios;

    orig_flags = SYS_CHECK(fcntl(STDIN_FILENO, F_GETFL));
    SYS_CHECK_GOTO(fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK), quit);

    SYS_CHECK(ioctl(STDIN_FILENO, TCGETS, &termios));
    orig_termios = termios;
    cfmakeraw(&termios);
    SYS_CHECK_GOTO(ioctl(STDIN_FILENO, TCSETS, &termios), quit);

    SYS_CHECK(ioctl(STDOUT_FILENO, TIOCGWINSZ, &s_winsz));

    s_blink = false;
    disable_blink();
    show_terminal_test();

    while (true) {
        if ((status = read(STDIN_FILENO, &c, 1)) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            CMD_PRINT("read(stdin): %s\n", strerror(errno));
            break;
        }
        // TODO: read keypresses and only respond to KP_ESCAPE,
        // otherwise random character escape sequences will exit the program
        else if (/*c == ASCII_ESC || */c == 0x3) {
            break;
        }

        switch (c) {
            case 'c':
                show_ansi_colorcard();
                pause();
                if (s_blink) {
                    enable_blink();
                }
                show_terminal_test();
                break;
            case 'b':
                if (s_blink) {
                    disable_blink();
                }
                else {
                    enable_blink();
                }
                s_blink = !s_blink;
                draw_menubar();
                break;
        }
    }

quit:
    SYS_CHECK(fcntl(STDIN_FILENO, F_SETFL, orig_flags));
    SYS_CHECK(ioctl(STDIN_FILENO, TCSETS, &orig_termios));    // TODO: atexit
    clear_screen();
    show_cursor();
    return 0;
}

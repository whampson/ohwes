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
 *         File: kernel/sys/open.c
 *      Created: April 2, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#include <ohwes.h>
#include <errno.h>
#include <syscall.h>
#include <task.h>

typedef int (*open_fn)(struct file **,int);

extern int rtc_open(struct file **,int);

struct char_dev {
    int id;
    const char *name;
};

#define MAX_PATH 256

#define DEV_RTC     1
#define DEV_PCSPK   2
#define DEV_KBD     3
#define DEV_CONSOLE 4

struct char_dev cdevs[] =
{
    { DEV_RTC, "/dev/rtc", },
    { DEV_PCSPK, "/dev/pcspk", },
    { DEV_KBD, "/dev/kbd", },
    { DEV_CONSOLE, "/dev/console", },
};
// TODO: well-defined device IDs;
// register them somehow during device init;
// major/minor IDs? to distinguish block and char devices

struct file _rtc;   // TODO: MOVE!!

int sys_open(const char *name, int flags)
{
    uint32_t cli_flags;
    int devid;
    int fd;
    int ret;
    open_fn open;

    assert(getpl() == KERNEL_PL);
    kprint("sys: open(%s, %d)\n", name, flags);
    cli_save(cli_flags); // prevent task switch

    // TODO: user-mode buffer validation!!

    // search device list for matching file name
    devid = -1;
    for (int i = 0; i < countof(cdevs); i++) {
        if (strncmp(name, cdevs[i].name, MAX_PATH) == 0) {
            devid = cdevs[i].id;
            break;
        }
    }
    // TOOD: search file system for matching file name

    if (devid < 0) {
        ret = -EINVAL;     // TODO: error for file not found?
        goto done;
    }

    // find next available file descriptor slot in current task struct
    fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (g_task->open_files[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        ret = -EBADF;    // TODO: error for no remaining file descriptors?
        goto done;
    }

    switch (devid) {
        case DEV_RTC:
            open = rtc_open;
            break;
        default:
            open = NULL;
            break;
    }

    if (!open) {
        ret = -ENOSYS;
        goto done;
    }

    ret = open(&g_task->open_files[fd], flags);
    if (ret < 0) {
        goto done;
    }
    ret = fd;

done:
    restore_flags(cli_flags);
    return ret;
}

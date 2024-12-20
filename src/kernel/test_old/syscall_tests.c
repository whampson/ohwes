#define __USER_MODE__ 1

#include "test.h"
#include <syscall.h>
#include <fs.h>
#include <errno.h>

void test_syscalls(void)
{
    char buf[8];
    assert(read(stdin_fd, NULL, 0) == 0);
    assert(read(stdin_fd, NULL, 1) == -1 && errno == EINVAL);
    assert(read(stdout_fd, buf, 1) == -1 && errno == ENOSYS);
    assert(read(2, buf, 1) == -1 && errno == EBADF);
    assert(write(stdout_fd, NULL, 0) == 0);
    assert(write(stdout_fd, NULL, 1) == -1 && errno == EINVAL);
    assert(write(stdin_fd, buf, 1) == -1 && errno == ENOSYS);
    assert(write(2, buf, 1) == -1 && errno == EBADF);
    assert(open("dummy", 0) == -1 && errno == EINVAL);
    assert(close(2) == -1 && errno == ENOSYS);
    assert(ioctl(2, 0, 0) == -1 && errno == EBADF);  // TODO: test w/ valid ioctl fn
    errno = 0;

    int retval;
    __asm__ volatile ("int $0x80" : "=a"(retval) : "a"(69));
    assert(retval == -ENOSYS);
}

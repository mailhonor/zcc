/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-07-10
 * ================================
 */

#include "zcc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void *file_do(void *arg)
{
    char *fn = (char *)arg;
    int fd = open(fn, O_RDWR|O_CREAT|O_TRUNC, 0777);
    if (fd == -1) {
        printf("can not open %s(%m)\n", fn);
        return 0;
    }
    for (int i=0;i < 10; i++) {
        if (write(fd, "0123456789", 8) != 8) {
            printf("write error\n");
        }
        if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
            printf("lseek error (%m)");
        }
        zcc::msleep(100);
    }
    close(fd);
    zcc::var_proc_stop = true;
    return 0;
}

int main(int argc, char **argv)
{
    zcc::coroutine_base_init();
    zcc::coroutine_go(file_do, (void *)"a.txt");
    zcc::coroutine_go(file_do, (void *)"b.txt");
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    sleep(3);
    return 0;
}

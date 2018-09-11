/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-07-02
 * ================================
 */

#include "zcc.h"
 
static void *test_sleep1(void *context)
{
    while(1){
        zcc::coroutine_msleep(1000);
        printf("sleep coroutine_msleep, 1 * 1000(ms)\n");
    }
    return context;
}

static void *test_sleep2(void *context)
{
    while(1){
        sleep(10);
        printf("sleep system sleep, 10 * 1000(ms)\n");
    }
    return context;
}

int main(int argc, char **argv)
{
    zcc::coroutine_base_init();
    zcc::coroutine_go(test_sleep1, 0);
    zcc::coroutine_go(test_sleep2, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}

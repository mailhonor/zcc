/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-02
 * ================================
 */

#include "zcc.h"
 
zcc::coroutine_cond_t *cond;
zcc::coroutine_mutex_t *mutex;
static void *wait_cond_then_do_something(void *context)
{
    for (int i = 0; i < (int)(long)(context); i++) {
        /* zcc::coroutine_mutex_lock(mutex); */
        zcc::coroutine_cond_wait(cond, mutex);
        printf("get signal, id=%d\n", (int)(long)context);
        /* zcc::coroutine_mutex_unlock(mutex); */
    }
    /* should execute unlock, here */
    /* auto release all mutexs when a coroutine exit */
    return context;
}

static void *begin_my_test(void *context)
{
    cond  = zcc::coroutine_cond_create();
    mutex  = zcc::coroutine_mutex_create();
    printf("start 10 coroutine to wait cond.\n");
    for (int i = 0; i < 10; i++) {
        zcc::coroutine_go(wait_cond_then_do_something, (void *)(long)i);
    }
    printf("waiting 1 second ...\n");

    printf("\n\ncoroutine_cond_signal, then sleep 1s  ... \n");
    zcc::coroutine_cond_signal(cond);
    sleep(1);

    printf("\n\ncoroutine_cond_broadcast ... \n");
    zcc::coroutine_cond_broadcast(cond);

    printf("\n\ncoroutine_cond_broadcast, then sleep 1s ... \n");
    zcc::coroutine_cond_broadcast(cond);
    sleep(1);

    printf("\n\ncoroutine_cond_broadcast ... \n");
    zcc::coroutine_cond_broadcast(cond);

    zcc::coroutine_cond_free(cond);
    zcc::coroutine_mutex_free(mutex);
    //exit(0);
    return 0;
}

int main(int argc, char **argv)
{
    zcc::coroutine_base_init();
    zcc::coroutine_go(begin_my_test, 0);
    zcc::coroutine_base_loop();
    zcc::coroutine_base_fini();
    return 0;
}

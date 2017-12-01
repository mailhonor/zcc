/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-16
 * ================================
 */

#include "zcc.h"

static zcc::tjob job;

static void *pworker(void *arg)
{
    void (*callback)(void *);
    void *context;

    while(1){
        if(!job.get(&callback, &context, 1000)) {
            continue;
        }
        callback(context);
    }

    return 0;
}

void foo(void * arg)
{
    printf("AAA, pthread_id: %ld\n", pthread_self());
}

int main()
{
    int i;
    for (i = 0; i< 3;i ++) {
        pthread_t pth;
        pthread_create(&pth, 0, pworker, 0);
    }

    while(1) {
        zcc::msleep(300);
        printf("job left:%zd\n", job.size());
        job.enter(foo, 0);
    }

    return 0;
}


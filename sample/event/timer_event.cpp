/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "zcc.h"
#include <time.h>

void timer_cb(zcc::event_timer &tm)
{
    time_t t = time(0);
    printf("go: %s\n", ctime(&t));
    tm.start(timer_cb, 1 * 1000);
}

int main(int argc, char **argv)
{
    zcc::event_base eb;
    zcc::event_timer *tm = new zcc::event_timer();

    tm->bind(eb);
    tm->start(timer_cb, 1 * 1000);

    while (1) {
        eb.dispatch();
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-16
 * ================================
 */

#include "zcc.h"

namespace zcc
{

tjob::tjob()
{
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&cond, 0);
}

tjob::~tjob()
{
}

void tjob::enter(void(*callback)(void *), void *context)
{
    item im;
    im.callback = callback;
    im.context = context;
    zcc_pthread_lock(&mutex);
    list.push_back(im);
    zcc_pthread_unlock(&mutex);
    pthread_cond_signal(&cond);
}

bool tjob::get(void(**callback)(void *), void **context, long timeout)
{
    struct timespec ts;
    zcc_pthread_lock(&mutex);

    if (timeout > 0) {
        ts.tv_sec = time(0) + timeout/1000;
        ts.tv_nsec = timeout%1000 * 1000 * 1000;
        pthread_cond_timedwait(&cond, &mutex, &ts);
    } else {
        pthread_cond_wait(&cond, &mutex);
    }

    if (list.empty()) {
        zcc_pthread_unlock(&mutex);
        return false;
    }

    tjob::item im = list.front();
    list.pop_front();
    zcc_pthread_unlock(&mutex);

    *callback = im.callback;
    *context = im.context;

    return true;
}

ssize_t tjob::size()
{
    ssize_t r;
    zcc_pthread_lock(&mutex);
    r = list.size();
    zcc_pthread_unlock(&mutex);
    return r;
}

bool tjob::empty()
{
    bool r;
    zcc_pthread_lock(&mutex);
    r = list.empty();
    zcc_pthread_unlock(&mutex);
    return r;
}

}

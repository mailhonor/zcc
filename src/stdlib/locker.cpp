/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-22
 * ================================
 */

#include "zcc.h"
#include <pthread.h>

namespace zcc
{

class pthread_locker_0411: public locker
{
public:
    pthread_locker_0411();
    ~pthread_locker_0411();
    void rlock();
    void wlock();
    void unlock();
private:
    pthread_mutex_t mutex;
};

pthread_locker_0411::pthread_locker_0411()
{
    pthread_mutex_init(&mutex, 0);
}

pthread_locker_0411::~pthread_locker_0411()
{
    pthread_mutex_destroy(&mutex);
}

void pthread_locker_0411::rlock()
{
    if(pthread_mutex_lock(&mutex)) {
        zcc_fatal("mutex:%m");
    }
}

void pthread_locker_0411::wlock()
{
    if(pthread_mutex_lock(&mutex)) {
        zcc_fatal("mutex:%m");
    }
}

void pthread_locker_0411::unlock()
{
    if(pthread_mutex_unlock(&mutex)) {
        zcc_fatal("mutex:%m");
    }
}

locker *pthread_locker_create()
{
    return new pthread_locker_0411();
}

void pthread_locker_free(locker *lock)
{
    delete lock;
}

}

/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-06-29
 * ================================
 */

#include "zcc.h"
#include "pthread.h"

namespace zcc
{

static pthread_mutex_t var_general_pthread_mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *var_general_pthread_mutex = &var_general_pthread_mutex_buffer;

}

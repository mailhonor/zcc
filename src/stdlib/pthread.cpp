/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
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

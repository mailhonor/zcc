/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-30
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class memkvd: public master_event_server
{
public:
    memkvd();
    ~memkvd();
    void before_service();
    void simple_service(int fd);
};

class memkv
{
public:
    memkv(const char *_destination);
    ~memkv();
    int set(const char *partition, const char *key, const char *val, ssize_t vlen = -1);
    int set(const char *partition, const char *key, long val);
    int del(const char *partition, const char *key);
    int inc(const char *partition, const char *key, long num, long *result = 0);
    int clear(const char *partition = 0);
    int exists(const char *partition, const char *key);
    int get(const char *partition, const char *key, std::string &result);
    int get(const char *partition, const char *key, long *result);
private:
    int require(char op, const char *partition, const char *key, const char *val, ssize_t vlen, std::string *result);
    char *destination;
    stream *fp;
};

}


namespace zcc
{

#define memkv_op_type_set           's'
#define memkv_op_type_set_int       'S'
#define memkv_op_type_del           'd'
#define memkv_op_type_get           'g'
#define memkv_op_type_exists        'e'
#define memkv_op_type_inc           'i'
#define memkv_op_type_clear         'c'

#define memkv_op_result_error       'e'
#define memkv_op_result_want        '1'
#define memkv_op_result_unwant      '0'

}

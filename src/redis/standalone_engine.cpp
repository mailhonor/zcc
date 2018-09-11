/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

redis_client_standalone_engine::redis_client_standalone_engine()
{
    r_fd = -1;
}

redis_client_standalone_engine::redis_client_standalone_engine(const char *destination, const char *password)
{
    r_fd = -1;
    open(destination, password);
}

redis_client_standalone_engine::~redis_client_standalone_engine()
{
    close();
}

int redis_client_standalone_engine::query_protocol(long *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, std::list<std::string> &tokens, long timeout, std::string &r_msg)
{
    if (r_fd == -1) {
        open();
    }
    if (r_fd == -1) {
        r_msg = "can not open ";
        r_msg += r_destination;
        return -2;
    }
    stream fp(r_fd);
    int ret = query_protocol_io(number_ret, string_ret, list_ret, json_ret, tokens, timeout, r_msg, fp);
    fp.close();
    if (ret == -2) {
        close();
    }
    return ret;
}

void redis_client_standalone_engine::open(const char *destination, const char *password)
{
    close();
    r_destination = (destination?destination:"");
    r_password = (password?password:"");
}

void redis_client_standalone_engine::open()
{
    close();
    r_fd = connect(r_destination.c_str());
}

void redis_client_standalone_engine::close()
{
    if (r_fd > -1) {
        ::close(r_fd);
        r_fd = -1;
    }
}

}

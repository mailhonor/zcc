/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

redis_standalone_client::redis_standalone_client()
{
    r_fd = -1;
}

redis_standalone_client::redis_standalone_client(const char *destination, const char *password)
{
    r_fd = -1;
    open(destination, password);
}

redis_standalone_client::~redis_standalone_client()
{
    close();
}

int redis_standalone_client::query_protocol(std::list<std::string> &ptokens)
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
    int ret = query_protocol_io(ptokens, fp);
    fp.close();
    if (ret == -2) {
        close();
    }
    return ret;
}

void redis_standalone_client::open(const char *destination, const char *password)
{
    close();
    r_destination = (destination?destination:"");
    r_password = (password?password:"");
}

void redis_standalone_client::open()
{
    close();
    r_fd = connect(r_destination.c_str());
}

void redis_standalone_client::close()
{
    if (r_fd > -1) {
        ::close(r_fd);
        r_fd = -1;
    }
}

}

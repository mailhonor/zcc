/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2018-04-12
 * ================================
 */

#include "zcc.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s 192.168.88.12/26\n", argv[0]);
        return 0;
    }

    std::string ipbuf = argv[1];
    char *ip = const_cast<char *>(ipbuf.c_str());
    char *p = strchr(ip, '/');
    if (!p) {
        printf("USAGE: %s 192.168.88.12/26\n", argv[0]);
        return 0;
    }
    *p++ = 0;
    int masklen = atoi(p);
    int ipint_input, ipint;
    std::string host;

    ipint_input = zcc::get_ipint(ip);

    host = "";
    ipint = zcc::get_network(ipint_input, masklen);
    zcc::get_ipstring(ipint, host);
    printf("Network address: %s\n", host.c_str());

    host = "";
    ipint = zcc::get_netmask(masklen);
    zcc::get_ipstring(ipint, host);
    printf("Netmask: %s\n", host.c_str());

    host = "";
    ipint = zcc::get_broadcast(ipint_input, masklen);
    zcc::get_ipstring(ipint, host);
    printf("Broadcast: %s\n", host.c_str());


    host = "";
    ipint = zcc::get_ip_min(ipint_input, masklen);
    zcc::get_ipstring(ipint, host);
    printf("ip_min: %s\n", host.c_str());

    host = "";
    ipint = zcc::get_ip_max(ipint_input, masklen);
    zcc::get_ipstring(ipint, host);
    printf("ip_max: %s\n", host.c_str());

    return 0;
}

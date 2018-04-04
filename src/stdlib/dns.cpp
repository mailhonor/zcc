/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-01
 * ================================
 */

#include "zcc.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>

namespace zcc
{

ssize_t get_localaddr(std::list<std::string> &addr_list, size_t max_count)
{
    struct ifaddrs *ifaddr, *ifa;
    struct sockaddr_in *scin;
    char ipbuf[32];
    size_t ret_count = 0;

    if (getifaddrs(&ifaddr) == -1) {
        return 0;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        scin = (struct sockaddr_in *)(ifa->ifa_addr);
        inet_ntop(AF_INET, &(scin->sin_addr), ipbuf, 16);
        addr_list.push_back(ipbuf);
        ret_count++;
        if (ret_count == max_count) {
            break;
        }
    }

    freeifaddrs(ifaddr);

    return ret_count;
}

ssize_t get_hostaddr(const char *host, std::list<std::string> &hosts, size_t max_count)
{
    struct in_addr **addr_list_tmp;
    struct hostent htt, *htr = 0;
    char hbuf[4096], *tmpbuf, ipbuf[32];
    int tmpbuflen = 4096, hterror;
    int alloc_flag = 0;
    size_t ret_count = 0;

    if (max_count == 0) {
        max_count = 1024;
    }
    if (empty(host)) {
        return get_localaddr(hosts, max_count);
    }

    tmpbuf = hbuf;
    while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror)) {
        if (hterror == NETDB_INTERNAL && errno == ERANGE) {
            tmpbuflen *= 2;
            if (alloc_flag) {
                tmpbuf = (char *)zcc::realloc(tmpbuf, tmpbuflen);
            } else {
                tmpbuf = (char *)zcc::malloc(tmpbuflen);
                alloc_flag = 1;
            }
        } else {
            break;
        }
    }
    if (htr) {
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        for (size_t i = 0; addr_list_tmp[i] != 0 && i < max_count; i++) {
            inet_ntop(AF_INET, addr_list_tmp[i], ipbuf, 16);
            hosts.push_back(ipbuf);
            ret_count++;
            if (ret_count == max_count) {
                break;
            }
        }
    }
    if (alloc_flag) {
        zcc::free(tmpbuf);
    }

    return ret_count;
}

bool get_peername(int sockfd, int *host, int *port)
{
    struct sockaddr_in sa;
    socklen_t sa_length = sizeof(struct sockaddr);

    if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0) {
        return false;
    }

    if (host) {
        *host = *((int *)&(sa.sin_addr));
    }

    if (port) {
        *port = ntohs(sa.sin_port);
    }

    return true;
}

bool get_peername(int sockfd, std::string &host, int *port)
{
    int ihost;
    host.clear();
    if (!get_peername(sockfd, &ihost, port)) {
        return false;
    }
    get_ipstring(ihost, host);
    return true;
}

bool get_ipstring(int ip, std::string &host)
{
    char str[32];
    host.clear();
    bool ret = (inet_ntop(AF_INET, &ip, str, 16) != 0);
    if (ret) {
        host.append(str);
    }
    return ret;
}

int get_ipint(const char *ipstr)
{
    return inet_addr(ipstr);
}

}

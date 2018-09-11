/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
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
    int ip = inet_addr(ipstr);
    if ((unsigned int)ip == INADDR_NONE) {
        return 0;
    }
    return ip;
}

static int ___ip_switch(int ip)
{
    int ip_switch;
    char *p1 = (char *)&ip;
    char *p2 = (char *)&ip_switch;
    p2[0] = p1[3];
    p2[1] = p1[2];
    p2[2] = p1[1];
    p2[3] = p1[0];
    return ip_switch;
}

static int ___get_netmask(int masklen)
{
    if ((masklen < 1) || (masklen > 32)) {
        return 0;
    }
    int mask = 0;
    for (int mi = masklen; mi < 32; mi ++) {
        mask = mask << 1;
        mask += 1;
    }
    mask = ~mask;
    return mask;
}

int get_network(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip & mask);
}

int get_netmask(int masklen)
{
    return ___ip_switch(___get_netmask(masklen));
}

int get_broadcast(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip |(~mask));
}

int get_ip_min(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip & mask) + 1);
}

int get_ip_max(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip |(~mask)) -1);
}

bool ip_is_intranet(const char *ip)
{
    if (!ip) {
        return false;
    }
    if ((!strncmp(ip, "127.", 4))||(!strncmp(ip, "10.", 3))||(!strncmp(ip, "192.168.", 8))) {
        return true;
    }
    if ((!strncmp(ip, "172.",4)) && ip[4] && ip[5] && (ip[6]=='.')) {
        int a = (ip[4] - '0') * 10 + (ip[5] - '0');
        if ((a>15) && (a<32)){
            return true;
        }
    }
    return false;
}

bool ip_is_intranet(int ip)
{
    int a = ((unsigned char *)&ip)[0];
    int b = ((unsigned char *)&ip)[1];

    if ((a==127)||(a==10)) {
        return true;
    }
    if ((a==192) && (b==168)) {
        return true;
    }
    if ((a==172) && (15<b) && (b<32)) {
        return true;
    }

    return false;
}

}

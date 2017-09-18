/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-09
 * ================================
 */

#include "zcc.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

namespace zcc
{

typedef struct {
    char val[20];
} __MAC;

static int get_mac_list(__MAC * mac_list)
{
    int sock_fd;
    struct ifreq *buf;
    struct ifconf ifc;
    int ret_num = 0, interface_num;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        return -1;
    }
    ifc.ifc_len = sizeof(struct ifreq) * 128;
    buf = (struct ifreq *)malloc(sizeof(struct ifreq) * 128);
    ifc.ifc_req = buf;
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0) {
        free(buf);
        return -1;
    }
    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    for (int ii = 0; ii < interface_num; ii++) {
        if (ioctl(sock_fd, SIOCGIFHWADDR, buf+ii) < 0) {
            continue;
        }
        unsigned char *ar = (unsigned char *)(buf[ii].ifr_hwaddr.sa_data);
        sprintf(mac_list[ret_num].val, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
        ret_num++;
    }

    close(sock_fd);

    free(buf);
    return ret_num;
}

bool license_mac_check(const char *salt, const char *license)
{
    char *mac;
    char license_c[20];
    __MAC *mac_list;
    int mac_num, i;

    if (empty(salt) || empty(license)) {
        return false;
    }
    mac_list = (__MAC *)malloc(sizeof(__MAC) * 128);
    mac_num = get_mac_list(mac_list);
    for (i = 0; i < mac_num; i++) {
        mac = mac_list[i].val;
        license_mac_build(salt, mac, license_c);
        if (!strncasecmp(license_c, license, 16)) {
            free(mac_list);
            return true;
        }
    }

    free(mac_list);
    return false;
}

void license_mac_build(const char *salt, const char *_mac, char *rbuf)
{
    char mac[128];
    char buf[512];
    unsigned char *p;
    int i;
    unsigned long crc;

    snprintf(mac, 127, "%s", _mac);
    toupper(mac);
    snprintf(buf, 511, "%s,%s", salt, mac);
    crc = get_crc64_result(buf, strlen(buf), 0);

    p = (unsigned char *)(&crc);
    for (i = 0; i < 8; i++) {
        sprintf(rbuf + i * 2, "%02X", *p++);
    }
    rbuf[16] = 0;
}

bool license_mac_check_by_config(const char *salt, const char *config_file, const char *key)
{
    zcc::config  cf;
    char *license;

    cf.load_by_filename(config_file);
    license = cf.get_str(key, "");

    if (empty(license)) {
        return false;
    }
    if (!zcc::license_mac_check(salt, license)) {
        return false;
    }
    return true;
}

}

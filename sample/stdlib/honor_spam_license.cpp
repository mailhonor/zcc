/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-09-15
 * ================================
 */

#include "zcc.h"

static void ___usage(char *arg = 0)
{
    printf("%s -mac mac_address      #to generate license\n", zcc::var_progname);
    printf("%s -license license      #to check lincese\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *salt = const_cast<char *>("spam.mailhonor.com");
    char *mac = 0;
    char *license = 0;

    zcc_main_parameter_begin() {
        if (!optval) {
            ___usage();
        }
        if (!strcmp(optname, "-salt")) {
            salt = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-mac")) {
            mac = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-license")) {
            license = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(salt)) {
        ___usage();
    }

    if (zcc::empty(mac) && zcc::empty(license)) {
        ___usage();
    }

    if (mac) {
        char nlicense[32];
        zcc::license_mac_build(salt, mac, nlicense);
        printf("%s\n", nlicense);
    } else {
        if (zcc::license_mac_check(salt, license)) {
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
    }

    return 0;
}

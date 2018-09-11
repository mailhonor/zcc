/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-05-19
 * ================================
 */

#include "zcc.h"

static void ___usage(char *arg = 0)
{
    printf("%s -salt salt_string -mac mac_address      #to generate license\n", zcc::var_progname);
    printf("%s -salt salt_string -license license      #to check lincese\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *salt = 0;
    char *mac = 0;
    char *license = 0;

    zcc::main_parameter_run(argc, argv);
    salt = zcc::default_config.get_str("salt", 0);
    mac = zcc::default_config.get_str("mac", 0);
    license = zcc::default_config.get_str("license", 0);


    if (zcc::empty(salt)) {
        ___usage();
    }

    if (zcc::empty(mac) && zcc::empty(license)) {
        ___usage();
    }

    if (mac) {
        std::string nlicense;
        zcc::license_mac_build(salt, mac, nlicense);
        printf("%s\n", nlicense.c_str());
    } else {
        if (zcc::license_mac_check(salt, license)) {
            printf("OK\n");
        } else {
            printf("ERR\n");
        }
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-11-14
 * ================================
 */

#include "zcc.h"

char *progname;

void usage()
{
    printf("USAGE: %s config_filename\n", progname);
    exit(0);
}

int main(int argc, char **argv)
{
    progname = argv[0];

    zcc::config config;

    if (argc < 2) {
        usage();
        
    }
    if (config.load_by_filename(argv[1]) == false) {
        printf("ERR open %s\n", argv[1]);
        exit(1);
    }

    config["abc"] = "SDFfff";
    config["abc"] = "SDFfff";

    config.debug_show();

    return 0;
}

/*
anvil_rate_time_unit = 60000s
anvil_status_update_time = 600000s
append_dot_mydomain = no
append_dot_mydomain + ssssss, 
append_dot_mydomain + ooooooo
biff + no
inet_interfaces = all
mailbox_command = procmail -a "$EXTENSION"
mailbox_size_limit = 0
mydestination = zytest.localdomain, localhost.localdomain, , localhost
*/

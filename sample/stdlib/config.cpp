/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
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
    if (config.load_from_filename(argv[1]) == false) {
        printf("ERR open %s\n", argv[1]);
        exit(1);
    }

    config.update("abc", "SDFfff");
    config.update("abc", "SDFfff");

    config.debug_show();

    return 0;
}

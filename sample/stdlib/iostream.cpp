/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-01
 * ================================
 */

#include "zcc.h"

static char *server = 0;
static void ___usage(char *arg = 0)
{
    printf("%s -server smtp_server:port\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zcc_main_parameter_begin() {
        if (!optval) {
            ___usage();
        }
        if (!strcmp(optname, "-server")) {
            server = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(server)) {
        ___usage();
    }

    int fd;
    fd = zcc::connect(server);
    if (fd < 0) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }
    zcc::nonblocking(fd);

    zcc::iostream fp(fd);

    std::string str;

    fp.gets(str);
    printf("S: %s", str.c_str());

    fp.puts("quit\r\n");
    printf("C: %s", "quit\r\n");

    fp.gets(str);
    printf("S: %s", str.c_str());

    return 0;
}

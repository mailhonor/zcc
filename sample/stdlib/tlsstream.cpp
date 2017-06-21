/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-02
 * ================================
 */

#include "zcc.h"

#include "zcc.h"

static char *server = 0;
static void ___usage(char *arg = 0)
{
    printf("%s -server smtp_server:port\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zcc::var_progname = argv[0];
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

    zcc::tlsstream fp(fd);

    std::string str;
    if (fp.gets(str) < 1) {
        printf("=== get line from server error\n");
        return 1;
    }
    printf("S: %s", str.c_str());

    fp.puts("STARTTLS\r\n");
    printf("C: STARTTLS\n");

    if (fp.gets(str) < 1) {
        printf("=== get line from server error\n");
        return 1;
    }
    printf("S: %s", str.c_str());

    zcc::openssl_init();
    SSL_CTX *ssl_ctx = zcc::openssl_create_SSL_CTX_client();
    fp.tls_connect(ssl_ctx);

    fp.puts("quit\r\n");
    printf("C: %s", "quit\n");

    if (fp.gets(str) < 1) {
        printf("=== get line from server error\n");
        return 1;
    }
    printf("S: %s", str.c_str());

    return 0;
}

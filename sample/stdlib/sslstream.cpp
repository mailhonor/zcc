/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-02
 * ================================
 */

#include "zcc.h"

static char *server = 0;
static void ___usage(char *arg = 0)
{
    printf("%s -server smtp_ssl_server:port\n", zcc::var_progname);
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

    zcc::openssl_init();
    SSL_CTX *ssl_ctx = zcc::openssl_create_SSL_CTX_client();
    SSL *ssl = zcc::openssl_create_SSL(ssl_ctx, fd);


    if (!zcc::openssl_timed_connect(ssl, 10 * 1000)) {
        printf("ERR ssl initialization error (%m)\n");
        exit(1);
    }
    zcc::sslstream fp(ssl);

    zcc::string str;

    fp.gets(str);
    printf("S: %s", str.c_str());

    fp.puts("quit\r\n");
    printf("C: %s", "quit\n");

    fp.gets(str);
    printf("S: %s", str.c_str());

    return 0;
}

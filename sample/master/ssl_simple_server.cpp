/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "zcc.h"
#include <time.h>
#include <openssl/ssl.h>

static SSL_CTX *var_openssl_server_ctx;
int main(int argc, char **argv)
{

    zcc::openssl_init();
    var_openssl_server_ctx = zcc::openssl_create_SSL_CTX_server();
    const char *fn_cert = "/home/xxx/mycode/honor-grid/etc/ssl/grid.crt";
    const char *fn_key = "/home/xxx/mycode/honor-grid/etc/ssl/grid.key";
    if (!zcc::openssl_SSL_CTX_set_cert(var_openssl_server_ctx, fn_cert, fn_key)) {
        zcc_fatal("can not load ssl cert(%s) or key file(%s)", fn_cert, fn_key);
    }   
    int type;
    int fd = zcc::listen("0:8899", &type, 5);
    while(1) {
        int sock = zcc::accept(fd, type);
        if (sock < 0) {
            continue;
        }
        zcc::stream *fp = new zcc::stream(sock);
        fp->tls_accept(var_openssl_server_ctx);
        fp->puts("220 00\n");
        fp->flush();
    }
    return 0;
}

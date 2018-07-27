/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-01
 * ================================
 */

#include "zcc.h"

static char *server = 0;
static bool ssl_mode = false;
static bool tls_mode = false;
static void ___usage(char *arg = 0)
{
    printf("%s -server smtp_server:port [--ssl ] [ --tls]\n", zcc::var_progname);
    exit(1);
}

static void  write_line_read_line(zcc::stream &fp, std::string &tmpline, const char *query)
{
    fp.puts(query).puts("\r\n");
    printf("C: %s\r\n", query);
    fp.gets(tmpline);
    printf("S: %s", tmpline.c_str());
}

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    server = zcc::default_config.get_str("server");
    ssl_mode = zcc::default_config.get_bool("ssl", false);
    tls_mode = zcc::default_config.get_bool("tls", false);
    if (zcc::empty(server)) {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    if (tls_mode || ssl_mode) {
        zcc::openssl_init();
        ssl_ctx = zcc::openssl_create_SSL_CTX_client();
    }

    zcc::stream fp;
    for (int iii = 0; iii < 2; iii ++) {
        printf("##############################\n\n\n");
        int fd;
        fd = zcc::connect(server);
        if (fd < 0) {
            printf("ERR open %s error, (%m)\n", server);
            exit(1);
        }
        zcc::nonblocking(fd);

        if (ssl_mode) {
            SSL *ssl = zcc::openssl_create_SSL(ssl_ctx, fd);
            if (!zcc::openssl_timed_connect(ssl, 10 * 1000)) {
                printf("ERR ssl initialization error (%m)\n");
                exit(1);
            }
            fp.open(ssl);
        } else {
            fp.open(fd);
        }
        fp.set_auto_close_fd();

        std::string str;
        fp.gets(str);
        printf("S: %s", str.c_str());

        write_line_read_line(fp, str, "helo goodtest");
        if (tls_mode && !ssl_mode) {
            write_line_read_line(fp, str, "STARTTLS");
            fp.tls_connect(ssl_ctx);
        }
        write_line_read_line(fp, str, "mail from: <xxx@163.com>");
        write_line_read_line(fp, str, "quit");

        fp.close();
    }
    if (tls_mode || ssl_mode) {
        zcc::openssl_SSL_CTX_free(ssl_ctx);
        zcc::openssl_fini();
    }


    return 0;
}

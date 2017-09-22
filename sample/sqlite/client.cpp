/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-02
 * ================================
 */

#include "zcc.h"

static void usage()
{
    printf("%s -server -query/exec/log sql_sentense\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *server = 0;
    char *sentense = 0;
    int op = 0;
    zcc_main_parameter_begin() {
        if (optval == 0) {
            usage();
        }
        if (!strcmp(optname, "-server")) {
            server = optval;
            opti += 2;
            continue;
        }
        if ((!strcmp(optname, "-query")) || (!strcmp(optname, "-exec")) || (!strcmp(optname, "-log"))) {
            sentense = optval;
            opti += 2;
            op = optname[1];
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(server) || zcc::empty(sentense)) {
        usage();
    }

    zcc::sqlite3_proxy pr(server);
    if (op == 'q') {
        if (pr.query(sentense, strlen(sentense), 0) == false) {
            printf("error: %s\n", pr.get_errmsg());
            exit(1);
        }
        zcc::size_data_t *sdvec;

        while (1) {
            int r = pr.get_row(&sdvec);
            if (r < 0) {
                printf("error: %s\n", pr.get_errmsg());
                exit(1);
            }
            if (r == 0) {
                break;
            }
            for (int idx = 0; idx < (int)pr.get_column(); idx++) {
                printf("%s ",  sdvec[idx].data);
            }
            printf("\n");
        }
    } else if (op == 'e') {
        if (pr.exec(sentense, strlen(sentense), 0) == false) {
            printf("error: %s\n", pr.get_errmsg());
            exit(1);
        }
        printf("ok\n");
    } else if (op == 'l') {
        if (pr.log(sentense, strlen(sentense), 0) == false) {
            printf("error: %s\n", pr.get_errmsg());
            exit(1);
        }
        printf("ok\n");
    }
}

/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-08-02
 * ================================
 */

#include "zcc.h"

static void usage()
{
    printf("ERR USAGE %s -server server -query/exec/log sql_sentense\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *server = 0;
    char *sentense = 0;
    int op = 0;
    zcc::main_parameter_run(argc, argv);
    server = zcc::default_config.get_str("server", 0);
    sentense = zcc::default_config.get_str("query", 0);
    op = 'q';
    if (!sentense) {
        sentense = zcc::default_config.get_str("exec", 0);
        op = 'e';
    }
    if (!sentense) {
        sentense = zcc::default_config.get_str("log", 0);
        op = 'l';
    }
    if (zcc::empty(server) || zcc::empty(sentense)) {
        usage();
    }

    zcc::sqlite3_proxy pr(server);
    if (op == 'q') {
        if (pr.query(sentense, strlen(sentense), 0) == false) {
            printf("error: %s\n", pr.get_errmsg());
            exit(1);
        }
        std::string *sdvec;

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
                printf("%s ",  sdvec[idx].c_str());
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

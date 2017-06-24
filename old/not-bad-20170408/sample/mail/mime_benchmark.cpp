/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "zcc.h"
#include <ctype.h>

static void ___usage(char *parameter)
{
    printf("USAGE: %s -f eml_filename [ -loop 1000 ] [ -mpool ] \n", zcc::var_progname);
    exit(1);
}

static char hunman_buf[100];
static char *hunman_size2(long a)
{
    char buf[300], *p = buf, ch;
    int len, m, i;
    int tl = 0;

    hunman_buf[0] = 0;
    sprintf(buf, "%ld", a);
    len = strlen(buf);
    m = len % 3;

    while (1) {
        for (i = 0; i < m; i++) {
            ch = *p++;
            if (ch == '\0') {
                goto over;
            }
            hunman_buf[tl++] = ch;
        }
        hunman_buf[tl++] = ',';
        m = 3;
    }

  over:
    hunman_buf[tl] = 0;
    len = strlen(hunman_buf);
    if (len > 0) {
        if (hunman_buf[len - 1] == ',') {
            hunman_buf[len - 1] = 0;
        }
    }
    if (hunman_buf[0] == ',') {
        return hunman_buf + 1;
    }

    return hunman_buf;
}

int main(int argc, char **argv)
{
    zcc::mem_pool *mpool;
    zcc::greedy_mem_pool greedy_mpool;
    char *eml_fn = 0, *eml_data;
    int times = 1000, i, eml_size;
    zcc::string eml_data_buf;
    long t;

    int enable_mpool = 0;
    ZCC_PARAMETER_BEGIN() {
        if (!strcmp(optname, "-mpool")) {
            enable_mpool = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-loop")) {
            times = atoi(optval);
            opti+=2;
            continue;
        }
        if (!strcmp(optname, "-f")) {
            eml_fn = optval;
            opti+=2;
            continue;
        }
    } ZCC_PARAMETER_END;

    if (!eml_fn) {
        ___usage(0);
    }

    if (zcc::file_get_contents_sample(eml_fn, eml_data_buf) < 0) {
        printf("ERR: open %s(%m)\n", eml_fn);
        exit(1);
    }
    eml_data = eml_data_buf.c_str();
    eml_size = eml_data_buf.size();

    printf("eml     : %s\n", eml_fn);
    printf("size    : %d(bytes)\n", eml_size);
    printf("loop    : %d\n", times);
    printf("total   : %s(bytes)\n", hunman_size2((long)eml_size * times));

    if (enable_mpool) {
        mpool = &greedy_mpool;
    } else {
        mpool = &zcc::system_mem_pool_instance;
    }
    t = zcc::timeout_set(0);
    for (i = 0; i < times; i++) {
        zcc::mail_parser mp(*mpool);
        mp.parse(eml_data, eml_size);
    }
    t = zcc::timeout_set(0) - t;

    printf("elapse  : %ld.%03ld(second)\n", t / 1000, t % 1000);
    printf("%%second : %s(bytes)\n", hunman_size2((long)(((long)eml_size * times) / ((1.0 * t) / 1000))));

    return 0;
}
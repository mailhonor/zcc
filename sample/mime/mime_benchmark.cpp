/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "zcc.h"
#include <ctype.h>

static void ___usage(char *parameter)
{
    printf("USAGE: %s -f eml_filename [ -loop 1000 ]\n", zcc::var_progname);
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
    zcc::main_parameter_run(argc, argv);
    char *eml_fn = 0, *eml_data;
    int times = 1000, i, eml_size;
    std::string eml_data_buf;
    long t;

    times = zcc::default_config.get_int("loop", 1000, 1, 100000);
    eml_fn = zcc::default_config.get_str("f");

    if (zcc::empty(eml_fn)) {
        ___usage(0);
    }

    if (zcc::file_get_contents_sample(eml_fn, eml_data_buf) < 0) {
        printf("ERR open %s(%m)\n", eml_fn);
        exit(1);
    }
    eml_data = const_cast<char *>(eml_data_buf.c_str());
    eml_size = eml_data_buf.size();

    printf("eml     : %s\n", eml_fn);
    printf("size    : %d(bytes)\n", eml_size);
    printf("loop    : %d\n", times);
    printf("total   : %s(bytes)\n", hunman_size2((long)eml_size * times));

    t = zcc::timeout_set(0);
    for (i = 0; i < times; i++) {
        zcc::mail_parser mp;
        mp.parse(eml_data, eml_size);
    }
    t = zcc::timeout_set(0) - t;

    printf("elapse  : %ld.%03ld(second)\n", t / 1000, t % 1000);
    printf("%%second : %s(bytes)\n", hunman_size2((long)(((long)eml_size * times) / ((1.0 * t) / 1000))));

    return 0;
}

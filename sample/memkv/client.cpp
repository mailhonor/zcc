/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-01
 * ================================
 */

#include "zcc.h"

void usage()
{
    printf("%s -server\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *server = 0;
    zcc_main_parameter_begin() {
        if (optval == 0) {
            usage();
        }
        if (!strcmp(optname, "-server")) {
            server = optval;
            opti +=2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(server)) {
        usage();
    }
    
    zcc::memkv mkv(server);

    zcc::string string_result;
    long int_result;
#define ftest(sentence) { \
    printf("%-50s", #sentence);\
    int s_ret = (sentence); \
    if (s_ret < 0) {\
        printf("error\n"); \
        exit(1); \
    } else if (s_ret == 0) { \
        printf("no\n"); \
    } else { \
        printf("ok"); \
        if (strstr(#sentence, "string_result)")) { \
            printf(": %s", string_result.c_str()); \
        } else if (strstr(#sentence, "int_result)")) { \
            printf(": %ld", int_result); \
        } \
        printf("\n"); \
    }\
}
    ftest(mkv.set("country", "china", "beijing"));
    ftest(mkv.set("country", "china", "bbbeijing"));
    ftest(mkv.get("country", "china", string_result));

    ftest(mkv.del("country", "china"));
    ftest(mkv.get("country", "china", string_result));

    ftest(mkv.set("country", "beijing", 123));
    ftest(mkv.inc("country", "beijing", 1000, &int_result));
    ftest(mkv.inc("country", "hebei", 1000, &int_result));

    ftest(mkv.clear());

    return 0;
}

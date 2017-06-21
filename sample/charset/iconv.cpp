/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-09
 * ================================
 */

#include "zcc.h"
#include <ctype.h>

static void ___usage(char *arg)
{
    printf("USAGE: %s -f from_charset -t to_charset [ -c [ignore_bytes_count] ] < input \n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *from_charset = 0;
    const char *to_charset = 0;
    ssize_t ignore_bytes = 0;
    size_t converted_len = 0;
    zcc::var_progname = argv[0];

    zcc_main_parameter_begin() {
        if (!strcmp(optname, "-c")) {
            ignore_bytes = -1;
            if (optval && isdigit(optval[0])) {
                ignore_bytes = atoi(optval);
                opti +=2;
            } else {
                opti +=1;
            }
            continue;
        }
        if (optval==0) {
            ___usage(0);
        }
        if (!strcmp(optname, "-f")) {
            from_charset = optval;
            opti+= 2;
            continue;
        }
        if (!strcmp(optname, "-t")) {
            to_charset = optval;
            opti+= 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(from_charset) || zcc::empty(to_charset)) {
        ___usage(0);
    }

    std::string content;
    std::string result;
    zcc::stdin_get_contents(content);

    if ((zcc::charset_iconv(from_charset, content.c_str(), (size_t)(content.size())
                    ,to_charset , result
                    ,&converted_len 
                    ,ignore_bytes, 0)) < 0){
        printf("ERR can not convert\n");
    } else if (converted_len < content.size()) {
        if (ignore_bytes == 0) {
            printf("ERR illegal char at %zd\n", converted_len+1);
        } else if (ignore_bytes == -1) {
            printf("ERR unknown\n");
        } else {
            printf("ERR illegal char too much > %zd\n", ignore_bytes);
        }
    } else {
        printf("%s\n", result.c_str());
    }

    return 0;
}

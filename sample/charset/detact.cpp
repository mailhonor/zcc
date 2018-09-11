/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-01-06
 * ================================
 */

#include "zcc.h"

void ___usage(char *arg)
{
    printf("USAGE: %s filename1 [filename2 ...]\n", zcc::var_progname);
    exit(1);
}

void dorun(const char *fn)
{
    std::string charset;
    std::string content;

    zcc::file_get_contents_sample(fn, content);

    if (zcc::charset_detect_cjk(content.c_str(), content.size(), charset) < 0) {
        printf("%-30s: not found, maybe ASCII\n", fn);
    } else {
        printf("%-30s: %s\n", fn, charset.c_str());
    }
}

int main(int argc, char **argv)
{
    zcc::var_charset_debug = 1;
    zcc::var_progname = argv[0];

    if (argc < 2) {
        ___usage(0);
    }
    zcc::main_parameter_run(argc, argv);

    for (int i = 0; i < zcc::main_parameter_argc; i++) {
        dorun(zcc::main_parameter_argv[i]);
    }

    return 0;
}

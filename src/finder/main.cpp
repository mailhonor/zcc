/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-09-14
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static char *___finder_query = 0;
static char *___finder_url = 0;

static void ___usage(const char *arg = 0)
{
    printf("USAGE: %s -url url -q query\n", var_progname);
    exit(1);
}

static void do_search(void)
{
    std::string result;
    int ret;

    if (___finder_url == NULL || ___finder_query == NULL) {
        ___usage();
    }

    finder fder;
    if (!fder.open(___finder_url)) {
        printf("ERR create finder error, %s\n", ___finder_url);
        exit(1);
    }

    ret = fder.find(___finder_query, result, 10 * 1000);
    if (ret < 0) {
        printf("ERR %s\n", result.c_str());
    } else if (ret == 0) {
        printf("NO not found\n");
    } else {
        printf("OK %s\n", result.c_str());
    }
}

int finder_main(int argc, char **argv)
{
    main_parameter_run(argc, argv);
    ___finder_query = default_config.get_str("q", 0);
    ___finder_url = default_config.get_str("url", 0);
    do_search();
    return 0;
}

}

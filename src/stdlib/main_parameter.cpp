/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-12-03
 * ================================
 */

#include "zcc.h"
#include <signal.h>

namespace zcc
{

/* log */
bool var_log_fatal_catch = false;
bool var_log_debug_enable = false;

/* openssl */
bool var_openssl_debug = false;

/* charset */
bool var_charset_debug = false;

}

static void ___timeout_do(int pid)
{ 
    exit(1);
}

namespace zcc
{

char *var_progname = 0;
bool var_proc_stop = false;

char ** var_main_parameter_argv = 0;
int var_main_parameter_argc = 0;

void main_parameter_run(int argc, char **argv)
{
    int i;
    char *optname, *optval;
    config cmd_cf;
    zcc::var_progname = argv[0];
    for (i = 1; i < argc; i++) {
        optname = argv[i];

        /* abc */
        if (optname[0] != '-') {
            var_main_parameter_argv = argv + i;
            var_main_parameter_argc = argc - i;
            break;
        }

        /* --abc */
        if (optname[1] == '-') {
            if (!strncmp(optname, "--debug-", 8)) {
            } else if (!strcmp(optname, "--fatal-catch")) {
                var_log_fatal_catch = true;
            } else {
                cmd_cf[optname+2] = "yes";
            }
            continue;
        }

        /* -abc */
        if (i+1 >= argc) {
            printf("ERR parameter %s need value\n", optname);
            exit(1);
        }
        i++;
        optval = argv[i];
        if (!strcmp(optname, "-config")) {
            if (default_config.load_by_filename(optval) == false) {
                zcc_info("ERR load config error from %s", optval);
                exit(1);
            }
        } else if (!strcmp(optname, "-exit-after")) {
            alarm(0);
            signal(SIGALRM, ___timeout_do);
            alarm(to_second(optval, 0) + 3);
        }else {
            cmd_cf[optname+1] = optval;
        }
    }

    default_config.load_another(cmd_cf);
    if(!var_log_debug_enable) {
        if (default_config.get_bool("debug", false)) {
            var_log_debug_enable = true;
        }
    }
}

}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-03
 * ================================
 */

#include "zcc.h"

namespace zcc
{

char *var_progname = 0;
char *var_listen_address = 0;
char *var_module_name = 0;
bool var_test_mode = false;

static char *___chroot = 0;
static char *___chuser = 0;

int parameter_run(int argc, char **argv)
{
    char *optname, *optval;
    char buf[10240 + 1], *p;

    optname = argv[0];
    if (!strcmp(optname, "-h")) {
        return -1;

    }

    if ((optname[0] != '-') || (optname[1] != '-')) {
        return 0;
    }
    optname += 2;

    if (!strcmp(optname, "h")) {
        return -1;
    }

    if (!strcmp(optname, "t")) {
        var_test_mode = true;
        return 1;
    }

    if (!strcmp(optname, "D")) {
        var_log_debug_inner_enable = true;
        return 1;
    }

    if (!strcmp(optname, "d")) {
        var_log_debug_enable = true;
        return 1;
    }

    if (!strcmp(optname, "fatal-catch")) {
        var_log_fatal_catch = true;
        return 1;
    }

    optval = argv[1];
    if (!strcmp(optname, "c")) {
        if (default_config.load_from_filename(optval) == false) {
            log_info("ERR: load config error from %s", optval);
            exit(1);
        }
        return 2;
    }

    if (!strcmp(optname, "o")) {
        snprintf(buf, 10240, "%s", optval);
        p = strchr(buf, '=');
        if (p) {
            *p++ = 0;
            default_config.update(buf, p);
        } else {
            default_config.update(buf, "");
        }
        return 2;
    }

    if (!strcmp(optname, "chdir")) {
        if (chdir(optval) == -1) {
            log_info("ERR: chdir %s (%m)", optval);
            exit(1);
        }
        return 2;
    }

    if (!strcmp(optname, "chroot")) {
        ___chroot = optval;
        return 2;
    }

    if (!strcmp(optname, "chuser")) {
        ___chuser = optval;
        return 2;
    }

    if (!strcmp(optname, "l")) {
        var_listen_address = optval;
        return 2;
    }

    if (!strcmp(optname, "module")) {
        var_module_name = optval;
        return 2;
    }
    return 0;
}

void parameter_run_2()
{
    if (___chuser == 0) {
        ___chuser = default_config.get_str("zrun_user", 0);
    }
    if (___chroot || ___chuser) {
        if (!chroot_user(___chroot, ___chuser)) {
            log_info("ERR\n");
            exit(1);
        }
    }

    if(!var_log_debug_enable) {
        if (default_config.get_bool("zdebug", false)) {
            var_log_debug_enable = true;
        }
    }
}

}

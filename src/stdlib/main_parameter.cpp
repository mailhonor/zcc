/*
 * ================================
 * eli960@163.com
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

static void ___usage__h(void)
{
    printf("\t--h   #extra help\n");
    printf("\t--d   # show debug info\n");
    printf("\t--c config_filename         # load config from filename\n");
    printf("\t--o key=value               # update default config with key=value\n");
    printf("\t--chdir dir_name            # cd dir\n");
    printf("\t--chroot new_root_dir       # change system root\n");
    printf("\t--chuser new_user_name      # change to new user\n");
    printf("\t--T n                       # exit after n sec\n");
}

static void ___timeout_do(int pid)
{ 
    zcc::var_proc_stop = true;
    if (zcc::var_proc_stop_handler == 0) {
        exit(1);
    }
}

namespace zcc
{

int var_main_argc_start = 1;
char *var_progname = 0;
char *var_listen_address = 0;
char *var_module_name = 0;
bool var_test_mode = false;
long var_proc_timeout = 0;
bool var_proc_stop = false;
bool var_proc_stop_handler = false;

static char *___chroot = 0;
static char *___chuser = 0;

void (*show_usage) (const char *ctx) = 0;

int main_parameter_run(int argc, char **argv)
{
    char *optname, *optval;
    char buf[10240 + 1], *p;

    optname = argv[0];
    if (optname[0] != '-') {
        main_parameter_fatal(optname);
    }

    if (optname[1] != '-') {
        return 0;
    }
    optname += 2;

    if (!strcmp(optname, "h")) {
        if (show_usage) {
            show_usage(0);
        }
        ___usage__h();
    }

    if (!strcmp(optname, "t")) {
        var_test_mode = true;
        return 1;
    }

    if (!strncmp(optname, "d-", 2)) {
        /* inner debug */
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
            zcc_info("ERR load config error from %s", optval);
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
            zcc_info("ERR chdir %s (%m)", optval);
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

    if (!strcmp(optname, "T")) {
        var_proc_timeout = timeout_set(atoi(optval) * 1000);
        alarm(0);
        signal(SIGALRM, ___timeout_do);
        alarm(atoi(optval) + 3);
        return 2;
    }
    return 0;
}

void main_parameter_run_over()
{
    if (___chuser == 0) {
        ___chuser = default_config.get_str("zrun_user", 0);
    }
    if (___chroot || ___chuser) {
        if (!chroot_user(___chroot, ___chuser)) {
            zcc_info("ERR\n");
            exit(1);
        }
    }

    if(!var_log_debug_enable) {
        if (default_config.get_bool("zdebug", false)) {
            var_log_debug_enable = true;
        }
    }
}

void main_parameter_fatal(char *arg)
{
    zcc_info("ERR unknown parameter %s\n", arg);
    exit(1);
}

}

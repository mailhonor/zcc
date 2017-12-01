/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-16
 * ================================
 */

#include "zcc.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

static void load_global_config_by_dir(zcc::config &cf, const char *config_path)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];

    dir = opendir(config_path);
    if (!dir) {
        return;
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list)) {
        fn = ent.d_name;
        if (fn[0] == '.') {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "gcf"))) {
            continue;
        }
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        cf.load_by_filename(pn);
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    char * attr = zcc::default_config.get_str("server-config-path", "");
    if (!zcc::empty(attr)) {
        zcc::config cf;
        load_global_config_by_dir(cf, attr);
        cf.load_another(zcc::default_config);
        zcc::default_config.load_another(cf);
    }
    zcc::default_config.debug_show();
    return 0;
}

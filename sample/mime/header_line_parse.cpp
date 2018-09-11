/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-09-07
 * ================================
 */

#include "zcc.h"

void usage()
{
    fprintf(stderr, "USAGE: %s -f header_file [ -default-charset gb18030 ]\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    char *fn = zcc::default_config.get_str("f");
    char *default_charset = zcc::default_config.get_str("default-charset", "gb18030");
    if (zcc::empty(fn)) {
        usage();
    }

    FILE *fp = fopen(fn, "r");
    if (!fp) {
        fprintf(stderr, "ERR open %s(%m)\n", fn);
        exit(1);
    }
    char buf[102400+10];
    std::string line, result;

    while(1) {
        if (ferror(fp) || feof(fp)) {
            break;
        }
        line.clear();
        bool have_data = false;
        while(1){
            long last_seek = ftell(fp);
            if (!fgets(buf, 102400, fp)) {
                break;
            }
            if (buf[0] == ' ' || buf[0] == '\t') {
                line.append(buf+1);
                continue;
            }
            if (have_data) {
                fseek(fp, last_seek, SEEK_SET);
                break;
            } else {
                line.append(buf);
                have_data = true;
            }
        }
        if (line.size()) {
            result.clear();
            zcc::mime_header_line_get_utf8(default_charset, line.c_str(), line.size(), result);
            fputs(result.c_str(), stdout);
        }
    }
    fclose(fp);

    return 0;
}

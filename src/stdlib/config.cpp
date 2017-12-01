/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-14
 * ================================
 */

#include "zcc.h"

namespace zcc
{

config default_config;

bool config::load_by_filename(const char *filename)
{
    FILE *fp;
    char buf[10240 + 1];
    char *key, *val;

    fp = fopen(filename, "r");
    if (!fp) {
        return false;
    }

    while(fgets(buf, 10240, fp)) {
        key = trim_left(buf);
        if ((!*key) || (*key == '#')) {
            continue;
        }
        val = strchr(key, '=');
        if (val) {
            *val++ = 0;
        }
        key = trim_right(key);
        if (val) {
            val = trim(val);
        }
        (*this)[key] = val?val:"";
    }

    fclose(fp);

    return true;
}

void config::load_another(config &cf)
{
    for (config::iterator it = cf.begin(); it != cf.end(); it++) {
        (*this)[it->first] = it->second;
    }
}

void config::get_str_table(config_str_table_t * table)
{
    while (table->name) {
        *(table->target) = get_str(table->name, table->defval);
        table++;
    }
}

void config::get_bool_table(config_bool_table_t * table)
{
    while (table->name) {
        *(table->target) = get_bool(table->name, table->defval);
        table++;
    }
}

#define ___ZCC_CONFIG_GET_TABLE(ttype) \
    void config::get_## ttype ## _table(config_ ## ttype ## _table_t * table) \
    { \
        while (table && table->name) \
        { \
            *(table->target) = get_ ## ttype(table->name, table->defval, table->min, table->max); \
            table++; \
        } \
    }
___ZCC_CONFIG_GET_TABLE(int);
___ZCC_CONFIG_GET_TABLE(long);
___ZCC_CONFIG_GET_TABLE(size);
___ZCC_CONFIG_GET_TABLE(second);


}

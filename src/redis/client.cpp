/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static const char engine_mode_none = ' ';
static const char engine_mode_standalone = 's';
static const char engine_mode_cluster = 'c';
static const char engine_mode_other = 'o';

redis_client::redis_client()
{
    r_timeout = -1;
    r_engine = 0;
    r_engine_mode = engine_mode_none;
}

redis_client::redis_client(const redis_client_basic_engine *engine)
{
    r_timeout = -1;
    r_engine = 0;
    r_engine_mode = engine_mode_none;
    open(engine);
}

redis_client::redis_client(const char *destination, const char *password)
{
    r_timeout = -1;
    r_engine = 0;
    r_engine_mode = engine_mode_none;
    open(destination, password);
}

redis_client::~redis_client()
{
    close();
}

void redis_client::open(const redis_client_basic_engine *engine)
{
    close();
    r_engine = const_cast<redis_client_basic_engine *>(engine);
    r_engine_mode = engine_mode_other;
}

void redis_client::open(const char *destination, const char *password)
{
    close();
    r_engine = new redis_client_standalone_engine(destination, password);
    r_engine_mode = engine_mode_standalone;
}

void redis_client::cluster_open(const char *destinations, const char *password)
{
    close();
    r_engine = new redis_client_cluster_engine(destinations, password);
    r_engine_mode = engine_mode_cluster;
}

void redis_client::close()
{
    if (r_engine) {
        if ((r_engine_mode != engine_mode_none) && (r_engine_mode != engine_mode_other)) {
            delete r_engine;
        }
        r_engine = 0;
    }
    r_engine_mode = engine_mode_none;
}


void redis_client::set_timeout(long timeout)
{
    if (timeout> var_max_timeout) {
        timeout = 10 * 1000;
    } else if (timeout < 0) {
        timeout = 10 * 1000;
    }
    r_timeout = timeout;
}

const std::string &redis_client::get_msg()
{
    return r_msg;
}

static void ___exec_command_prepare(std::list<std::string> &ptokens, const char *redis_fmt, va_list ap)
{
    if (redis_fmt == 0) {
        return;
    }
    char nbuf[1024];
    int ch;
    const char *fmt = redis_fmt;
    while((ch = *fmt++)) {
        if (ch == 's') {
            const char *sv = va_arg(ap, const char *);
            if (sv != (const char *) -1) {
                ptokens.push_back(sv?sv:"");
            }
        } else if (ch == 'S') {
            const std::string *Sv = va_arg(ap, const std::string *);
            if (Sv != (const std::string *)-1) {
                if (Sv) {
                    ptokens.push_back(*Sv);
                } else {
                    ptokens.push_back("");
                }
            }
        } else if (ch == 'd') {
            long dv = va_arg(ap, long);
            sprintf(nbuf, "%ld", dv);
            ptokens.push_back(nbuf);
        } else if (ch == 'f') {
            double fv = va_arg(ap, double);
            sprintf(nbuf, "%lf", fv);
            ptokens.push_back(nbuf);
        } else if (ch == 'L') {
            const std::list<std::string> *lv = va_arg(ap, const std::list<std::string> *);
            if (!lv) {
                ptokens.push_back("");
            } else if (lv != (const std::list<std::string> *)-1) {
                for (std::list<std::string>::const_iterator it = lv->begin(); it != lv->end(); it++) {
                    ptokens.push_back(*it);
                }
            }
        } else if (ch == 'A') {
            argv *ts = va_arg(ap, argv *);
            if (!ts) {
                ptokens.push_back("");
            } else if (ts != (argv *) -1) {
                zcc_argv_walk_begin(*ts, p) {
                    ptokens.push_back(p);
                } zcc_argv_walk_end;
            }
        } else if (ch == 'P') {
            const char **pp = va_arg(ap, const char **);
            if (!pp) {
                ptokens.push_back("");
            } else if (pp != (const char **)-1) {
                while(*pp) {
                    ptokens.push_back(*pp++);
                }
            }
        }
    }
}

int redis_client::exec_command(long *number_ret, std::string *string_ret, std::list<std::string> *list_ret,
        json *json_ret, const char *redis_fmt, va_list ap)
{
    if (r_engine == 0) {
        return -1;
    }
    std::list<std::string> ptokens;
    ___exec_command_prepare(ptokens, redis_fmt, ap);
    r_msg.clear();
    return r_engine->query_protocol(number_ret, string_ret, list_ret, json_ret, ptokens, r_timeout, r_msg);
}

int redis_client::exec_command(const char *redis_fmt, ...)
{
#define exec_command_macro(n, s, l, j) \
    int ret;\
    va_list ap; \
    va_start(ap, redis_fmt); \
    ret = exec_command(n, s, l, j, redis_fmt, ap); \
    va_end(ap); \
    return ret; 
    exec_command_macro(0, 0, 0, 0);
}

int redis_client::exec_command(long &number_ret, const char *redis_fmt, ...)
{
    exec_command_macro(&number_ret, 0, 0, 0);
}

int redis_client::exec_command(std::string &string_ret, const char *redis_fmt, ...)
{
    exec_command_macro(0, &string_ret, 0, 0);
}

int redis_client::exec_command(std::list<std::string> &list_ret, const char *redis_fmt, ...)
{
    exec_command_macro(0, 0, &list_ret, 0);
}

int redis_client::exec_command(json &json_ret, const char *redis_fmt, ...)
{
    exec_command_macro(0, 0, 0, &json_ret);
#undef exec_command_macro
}

int redis_client::fetch_channel_message(std::list<std::string> &list_ret)
{
    return exec_command(list_ret, 0);
}

int redis_client::scan(std::list<std::string> &list_ret, long &cursor_ret, const char *redis_fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, redis_fmt);
    ret = exec_command(0, 0, &list_ret, 0, redis_fmt, ap);
    va_end(ap);
    if (ret > 0) {
        if (list_ret.empty()) {
            return -1;
        }
        std::string &cursor_str = list_ret.front();
        cursor_str = atol(cursor_str.c_str());
        list_ret.pop_front();
    }
    return ret;
}

int redis_client::info(std::map<std::string, std::string> &name_value_dict, std::string &string_ret)
{
    name_value_dict.clear();
    string_ret.clear();
    int ret =  exec_command(string_ret, "s", "INFO");
    if (ret < 1) {
        return ret;
    }
    const char *ps, *p = string_ret.c_str();
    std::string tmpn, tmpv;
    while(*p) {
        if ((*p == '#') || (*p == ' ') || (*p == '\r')) {
            p++;
            for (;*p;p++) {
                if (*p == '\n') {
                    break;
                }
            }
            if (*p == '\n') {
                p++;
            }
            continue;
        }
        ps = p;
        for (;*p;p++) {
            if (*p == ':') {
                break;
            }
        }
        if (*p == 0) {
            break;
        }
        tmpn.clear();
        if (p > ps) {
            tmpn.append(ps, p - ps);
        }
        p++;
        ps = p;

        for (;*p;p++) {
            if (*p == '\n') {
                break;
            }
        }
        if (*p == 0) {
            break;
        }
        tmpv.clear();
        if (p - ps > 1) {
            tmpv.append(ps, p - ps - 1);
        }
        name_value_dict[tmpn] = tmpv;
        p++;
    }
    return ret;
}

}

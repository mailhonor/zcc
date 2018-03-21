/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"
#include "./args.hpp"
#include "./engine.hpp"

namespace zcc
{

class redis_client::client_info
{
public:
    inline client_info() { }
    inline ~client_info() { }
    std::string msg_info;
    bool flag_inner_engine;
};

#define _nine_token_s const char *n2, const char *n3, const char *n4, const char *n5, const char *n6, const char *n7, const char *n8, const char *n9, const char *n10
#define _nine_token_p n2, n3, n4, n5, n6, n7, n8, n9, n10
#define _nine_token2_s const char *n2, const char *v2, const char *n3, const char *v3, const char *n4, const char *v4, const char *n5, const char *v5,const char *n6, const char *v6, const char *n7, const char *v7, const char *n8, const char *v8, const char *n9, const char *v9, const char *n10, const char *v10
#define _nine_token2_p n2, v2, n3, v3, n4, v4, n5, v5, n6, v6, n7, v7, n8, v8, n9, v9
#define _nine_token_S const std::string &n2, const std::string &n3, const std::string &n4, const std::string &n5, const std::string &n6, const std::string &n7, const std::string &n8, const std::string &n9, const std::string &n10
#define _nine_token_P  &n2, &n3, &n4, &n5, &n6, &n7, &n8, &n9, &n10
#define _nine_token2_S const std::string &n2, const std::string &v2, const std::string &n3, const std::string &v3, const std::string &n4, const std::string &v4, const std::string &n5, const std::string &v5,const std::string &n6, const std::string &v6, const std::string &n7, const std::string &v7, const std::string &n8, const std::string &v8, const std::string &n9, const std::string &v9, const std::string &n10, const std::string &v10
#define _nine_token2_P &n2, &v2, &n3, &v3, &n4, &v4, &n5, &v5, &n6, &v6, &n7, &v7, &n8, &v8, &n9, &v9, &n10, &v10

#define _nine_fmt_s "sssssssss"
#define _nine_fmt_S "SSSSSSSSS"

redis_client::redis_client(const char *destination, const char *password)
{
    r_info = new client_info();
    r_info->flag_inner_engine = true;
    r_engine = new redis_client_standalone_engine(destination, password);
}

redis_client::redis_client(redis_client_basic_engine &engine)
{
    r_info = new client_info();
    r_info->flag_inner_engine = false;
    r_engine = &engine;
}

redis_client::~redis_client()
{
    if (r_info->flag_inner_engine) {
        delete r_engine;
    }
}

const std::string &redis_client::get_msg()
{
    return r_info->msg_info;
}

static int ___qstr_build_append(std::string &qstr, double f)
{
    char numberbuf[1000];
    int nlen = sprintf(numberbuf, "%lf", f);
    char lenbuf[64];
    sprintf(lenbuf, "%d", nlen);
    qstr.append("$").append(lenbuf).append("\r\n").append(numberbuf).append("\r\n");
    return 1;
}

static int ___qstr_build_append(std::string &qstr, long l)
{
    char numberbuf[1000];
    int nlen = sprintf(numberbuf, "%ld", l);
    char lenbuf[64];
    sprintf(lenbuf, "%d", nlen);
    qstr.append("$").append(lenbuf).append("\r\n").append(numberbuf).append("\r\n");
    return 1;
}

static int ___qstr_build_append(std::string &qstr, const char *s, size_t slen = 0)
{
    char lenbuf[64];
    size_t len;
    if (slen > 0) {
        len = slen;
    } else {
        if (s == 0) {
            len = 0;
            s = "";
        } else {
            len = ::strlen(s);
        }
    }
    sprintf(lenbuf, "%zd", len);
    qstr.append("$").append(lenbuf).append("\r\n");
    if (len > 0) {
        qstr.append(s, len);
    }
    qstr.append("\r\n");
    return 1;
}

static int ___qstr_build_append(std::string &qstr, const char *redis_fmt, va_list ap)
{
    int a_count = 0;
    int ch;
    long dv;
    double fv;
    const char *sv;
    const std::string *Sv;

    if (redis_fmt == 0) {
        return 0;
    }
    const char *fmt = redis_fmt;
    while((ch = *fmt++)) {
        if (ch == 's') {
            sv = va_arg(ap, const char *);
            if (sv != ((const char *)-9)) {
                if (sv) {
                    a_count += ___qstr_build_append(qstr, sv);
                } else {
                    a_count += ___qstr_build_append(qstr, "");
                }
            } else {
            }
        } else if (ch == 'S') {
            Sv = va_arg(ap, const std::string *);
            if (Sv && (!is_std_string_ignore(*Sv))) {
                a_count += ___qstr_build_append(qstr, Sv->c_str(), Sv->size());
            }
        } else if (ch == 'd') {
            dv = va_arg(ap, long);
            a_count += ___qstr_build_append(qstr, dv);
        } else if (ch == 'f') {
            fv = va_arg(ap, double);
            a_count += ___qstr_build_append(qstr, fv);
        } else if (ch == 'L') {
            const std::list<std::string> *lv = va_arg(ap, const std::list<std::string> *);
            if (lv) {
                for (std::list<std::string>::const_iterator it = lv->begin(); it != lv->end(); it++) {
                    a_count += ___qstr_build_append(qstr, it->c_str(), it->size());
                }
            }
        }
    }
    return a_count;
}

class redis_string_list:public std::list<std::string>
{
public:
    inline redis_string_list() {}
    inline ~redis_string_list() {}
    inline redis_string_list &append(const std::string &s) { push_back(s); return *this; }
    redis_string_list &append(const char *s) { push_back(s?s:""); return *this; }
    redis_string_list &append(const char *s, size_t size);
    redis_string_list &append(long d);
    redis_string_list &append(int d) { return append((long)d); }
    redis_string_list &append(double f);
};
redis_string_list & redis_string_list::append(const char *s, size_t size)
{
    if (size == 0) {
        if (s == 0) {
            push_back("");
            return *this;
        } else {
            size = strlen(s);
        }
    }
    std::string tmp(s, size);
    push_back(tmp);
    return *this;
}

redis_string_list & redis_string_list::append(long d)
{
    char buf[64];
    sprintf(buf, "%ld", d);
    push_back(buf);
    return *this;
}

redis_string_list &redis_string_list::append(double f)
{
    char buf[512];
    sprintf(buf, "%lf", f);
    push_back(buf);
    return *this;
}

int redis_client::query_command(redis_client_query &query, const char *redis_fmt, ...)
{
    std::string qstr(12, '\0');
    int ret = ___qstr_build_append(qstr, query.cmd.c_str(), query.cmd.size());
    va_list ap;
    va_start(ap, redis_fmt);
    ret += ___qstr_build_append(qstr, redis_fmt, ap);
    va_end(ap);
    char count_buf[12];
    ret = sprintf(count_buf, "*%d\r\n", ret);
    memcpy((char *)qstr.c_str() + (12 - ret), count_buf, ret);
    query.req_data = qstr.c_str() + (12 - ret);
    query.req_len = qstr.size() - (12 - ret);
    long nval = -2;
    if (query.number_ret == 0) {
        query.number_ret = &nval;
    }
    ret = r_engine->query_protocol(query);
    if (ret > 0) {
        if (nval > -1) {
            ret = (int)nval;
        }
    }
    return ret;
}

#define query_command_macro_0(_cmd, _key, _flag_write, _nv, _sv, _lv, _jv) \
    redis_client_query query(_cmd); \
    query.key = _key; \
    query.flag_write = _flag_write; \
    query.number_ret = _nv; \
    query.string_ret = _sv; \
    query.list_ret = _lv; \
    query.json_ret = _jv;
    
#define query_command_macro_1(_cmd, _key, _flag_write, _nv, _sv, _lv, _jv, _rfmt, _args...) \
    query_command_macro_0(_cmd, _key, _flag_write, _nv, _sv, _lv, _jv);  \
    return query_command(query, _rfmt, ##_args);
    
#define query_command_macro_2(_cmd, _key, _flag_write, _nv, _sv, _lv, _jv, _rfmt, _args...) \
    query_command_macro_0(_cmd, _key, _flag_write, _nv, _sv, _lv, _jv);  \
    int ret = query_command(query, _rfmt, ##_args);
/* CONNECTION */
int redis_client::auth(const char *password)
{
    query_command_macro_1("AUTH", "", false, 0, 0, 0, 0, "s", password);
}

int redis_client::auth(const std::string &password)
{
    query_command_macro_1("AUTH", "", false, 0, 0, 0, 0, "S", &password);
}

int redis_client::echo(const char *str)
{
    query_command_macro_1("ECHO", "", false, 0, 0, 0, 0, "s", str);
}

int redis_client::echo(const std::string &str)
{
    query_command_macro_1("ECHO", "", false, 0, 0, 0, 0, "S", &str);
}

int redis_client::select(long idx)
{
    query_command_macro_1("SELECT", "", true, 0, 0, 0, 0, "d", idx);
}

int redis_client::ping()
{
    query_command_macro_1("PING", "", false, 0, 0, 0, 0, 0);
}

int redis_client::discard()
{
    query_command_macro_1("DISCARD", "", false, 0, 0, 0, 0, 0);
}

int redis_client::exec()
{
    /* FIXME */
    return -1;
}

int redis_client::multi()
{
    query_command_macro_1("MULTI", "", false, 0, 0, 0, 0, 0);
}

int redis_client::unwatch()
{
    query_command_macro_1("UNWATCH", "", false, 0, 0, 0, 0, 0);
}

int redis_client::watch(const char *key1, _nine_token_s)
{
    query_command_macro_1("WATCH", key1, false, 0, 0, 0, 0, "s" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::watch(const std::string &key1, _nine_token_S)
{
    query_command_macro_1("WATCH", key1, false, 0, 0, 0, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::bgrewriteaof()
{
    query_command_macro_1("BGREWRITEAOF", "", false, 0, 0, 0, 0, 0);
}

int redis_client::bgsave()
{
    query_command_macro_1("BGSAVE", "", true, 0, 0, 0, 0, 0);
}

int redis_client::client_getname(std::string &result)
{
    query_command_macro_1("CLIENT", "", false, 0, &result, 0, 0, "s", "GETNAME");
}

int redis_client::client_kill(const char *ip_port)
{
    query_command_macro_1("CLIENT", "", true, 0, 0, 0, 0, "ss", "KILL", ip_port);
}

int redis_client::client_kill(const std::string &ip_port)
{
    query_command_macro_1("CLIENT", "", true, 0, 0, 0, 0, "sS", "KILL", &ip_port);
}

int redis_client::client_list(std::string &result)
{
    query_command_macro_1("CLIENT", "", true, 0, &result, 0, 0, "s", "LIST");
}

int redis_client::client_setname(const char *name)
{
    query_command_macro_1("CLIENT", "", true, 0, 0, 0, 0, "ss", "SETNAME", name);
}

int redis_client::client_setname(const std::string &name)
{
    query_command_macro_1("CLIENT", "", true, 0, 0, 0, 0, "sS", "SETNAME", &name);
}

int redis_client::config_get(std::list<std::string> &result, const char *parameter_pattern)
{
    query_command_macro_1("CONFIG", "", false, 0, 0, &result, 0, "ss", "GET", parameter_pattern);
}

int redis_client::config_get(std::list<std::string> &result, const std::string &parameter_pattern)
{
    query_command_macro_1("CONFIG", "", false, 0, 0, &result, 0, "sS", "GET", &parameter_pattern);
}

int redis_client::config_resetstat()
{
    query_command_macro_1("CONFIG", "", true, 0, 0, 0, 0, "s", "RESETSTAT");
}

int redis_client::config_rewrite()
{
    query_command_macro_1("CONFIG", "", true, 0, 0, 0, 0, "s", "REWRITE");
}

int redis_client::config_set(const char *parameter, const char *value)
{
    query_command_macro_1("CONFIG", "", true, 0, 0, 0, 0, "sss", "SET", parameter, value);
}

int redis_client::config_set(const std::string &parameter, const std::string &value)
{
    query_command_macro_1("CONFIG", "", true, 0, 0, 0, 0, "sSS", "SET", &parameter, &value);
}

int redis_client::db_size()
{
    query_command_macro_1("DBSIZE", "", false, 0, 0, 0, 0, 0);
}

int redis_client::FLUSHALL()
{
    query_command_macro_1("FLUSHALL", "", true, 0, 0, 0, 0, 0);
}

int redis_client::FLUSHDB()
{
    query_command_macro_1("FLUSHDB", "", true, 0, 0, 0, 0, 0);
}

int redis_client::info(std::string &result)
{
    query_command_macro_1("INFO", "", false, 0, 0, 0, 0, 0);
}

int redis_client::info(std::map<std::string, std::string> &name_value_dict, std::string &result)
{
    name_value_dict.clear();
    result.clear();
    int ret =  info(result);
    if (ret < 1) {
        return ret;
    }
    const char *ps, *p = result.c_str();
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

int redis_client::lastsave(long &result)
{
    query_command_macro_1("LASTSAVE", "", false, 0, 0, 0, 0, 0);
}

int redis_client::psync()
{
    /* FIXME */
    return -1;
}

int redis_client::save()
{
    query_command_macro_1("SAVE", "", true, 0, 0, 0, 0, 0);
}

int redis_client::SHUTDOWN()
{
    query_command_macro_1("SHUTDOWN", "", true, 0, 0, 0, 0, 0);
}

int redis_client::SHUTDOWN_SAVE()
{
    query_command_macro_1("SHUTDOWN", "", true, 0, 0, 0, 0, "s", "SAVE");
}

int redis_client::SHUTDOWN_NOSAVE()
{
    query_command_macro_1("SHUTDOWN", "", true, 0, 0, 0, 0, "s", "NOSAVE");
}

int redis_client::slaveof(const char *host, long port)
{
    if (!port) {
        std::string myhost;
        char *p = (char *)strchr(host, ':');
        if (!p) {
            return -1;
        }
        myhost.append(host, p - host);
        p++;
        query_command_macro_1("SLAVEOF", "", true, 0, 0, 0, 0, "Ss", &myhost, p);
    } else {
        query_command_macro_1("SLAVEOF", "", true, 0, 0, 0, 0, "sd", host, port);
    }
}

int redis_client::slaveof(const std::string &host, long port)
{
    return slaveof(host.c_str(), port);
}

int redis_client::slaveof_no_one()
{
    query_command_macro_1("SLAVEOF", "", true, 0, 0, 0, 0, "ss", "NO", "ONE")
}

int redis_client::slowlog()
{
    /* FIXME */
    return -1;
}

int redis_client::sync()
{
    query_command_macro_1("SYNC", "", true, 0, 0, 0, 0, 0);
}

int redis_client::time(long &second, long &microsecond)
{
    std::list<std::string> res;
    query_command_macro_2("TIME", "", false, 0, 0, &res, 0, 0);
    if (ret > 0) {
        if (res.size() != 2) {
            return 0;
        }
        std::list<std::string>::iterator it = res.begin();
        second = atol(it->c_str());
        it ++;
        microsecond = atol(it->c_str());
    }
    return ret;
}

/* KEY */
int redis_client::del(const char *key1, _nine_token_s)
{
    query_command_macro_1("DEL", key1, true, 0, 0, 0, 0, "s" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::del(const std::string &key1, _nine_token_S)
{
    query_command_macro_1("DEL", key1, true, 0, 0, 0, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::del(const std::list<std::string> &keys)
{
    if (keys.empty()) {
        return 0;
    }
    query_command_macro_1("DEL", keys.front(), true, 0, 0, 0, 0, "L", &keys);
}

int redis_client::dump(std::string &dump_result, const char *key)
{
    query_command_macro_1("DUMP", key, false, 0, &dump_result, 0, 0, "s", key);
}

int redis_client::dump(std::string &dump_result, const std::string &key)
{
    query_command_macro_1("DUMP", key, false, 0, &dump_result, 0, 0, "S", &key);
}

int redis_client::exists(const char *key)
{
    query_command_macro_1("EXISTS", key, false, 0, 0, 0, 0, "s", key);
}

int redis_client::exists(const std::string &key)
{
    query_command_macro_1("EXISTS", key, false, 0, 0, 0, 0, "S", &key);
}

int redis_client::expire(const char *key, long timeout_second)
{
    query_command_macro_1("EXPIRE", key, true, 0, 0, 0, 0, "sd", key, timeout_second);
}

int redis_client::expire(const std::string &key, long timeout_second)
{
    query_command_macro_1("EXPIRE", key, true, 0, 0, 0, 0, "Sd", &key, timeout_second);
}

int redis_client::expireat(const char *key, long timeout_at_second)
{
    query_command_macro_1("EXPIREAT", key, true, 0, 0, 0, 0, "sd", key, timeout_at_second);
}

int redis_client::expireat(const std::string &key, long timeout_at_second)
{
    query_command_macro_1("EXPIREAT", key, true, 0, 0, 0, 0, "Sd", &key, timeout_at_second);
}

int redis_client::keys(std::list<std::string> &val, const char *pattern)
{
    query_command_macro_1("KEYS", "", false, 0, 0, &val, 0, "s", pattern);
}

int redis_client::keys(std::list<std::string> &val, const std::string &pattern)
{
    query_command_macro_1("KEYS", "", false, 0, 0, &val, 0, "S", &pattern);
}

int redis_client::migrate(const char *host, long port, const char *key, long dest_db, long timeout_ms, bool copy, bool replace)
{
    query_command_macro_1("MIGRATE", key, true, 0, 0, 0, 0, "sdsddss", host, port, key, dest_db, timeout_ms, copy?"COPY":(const char *)-9, replace?"REPLACE":(const char *)-9);
}

int redis_client::migrate(const std::string &host, long port, const std::string &key, long dest_db, long timeout_ms, bool copy, bool replace)
{
    query_command_macro_1("MIGRATE", key, true, 0, 0, 0, 0, "SdSddss", &host, port, &key, dest_db, timeout_ms, copy?"COPY":(const char *)-9, replace?"REPLACE":(const char *)-9);
}

int redis_client::move(const char *key, long dest_db_id)
{
    query_command_macro_1("MOVE", key, true, 0, 0, 0, 0, "sd", key, dest_db_id);
}

int redis_client::move(const std::string &key, long dest_db_id)
{
    query_command_macro_1("MOVE", key, true, 0, 0, 0, 0, "Sd", &key, dest_db_id);
}

int redis_client::object_refcount(long &refcount, const char *key)
{
    query_command_macro_1("OBJECT", key, false, &refcount, 0, 0, 0, "ss", "REFCOUNT", key);
}

int redis_client::object_refcount(long &refcount, const std::string &key)
{
    query_command_macro_1("OBJECT", key, false, &refcount, 0, 0, 0, "sS", "REFCOUNT", &key);
}

int redis_client::object_idletime(long &idletime, const char *key)
{
    query_command_macro_1("OBJECT", key, false, &idletime, 0, 0, 0, "ss", "IDLETIME", key);
}

int redis_client::object_idletime(long &idletime, const std::string &key)
{
    query_command_macro_1("OBJECT", key, false, &idletime, 0, 0, 0, "sS", "IDLETIME", &key);
}

int redis_client::object_encoding(std::string &encoding, const char *key)
{
    query_command_macro_1("OBJECT", key, false, 0, &encoding, 0, 0, "ss", "ENCODING", key);
}

int redis_client::object_encoding(std::string &encoding, const std::string &key)
{
    query_command_macro_1("OBJECT", key, false, 0, &encoding, 0, 0, "sS", "ENCODING", &key);
}

int redis_client::persist(const char *key)
{
    query_command_macro_1("PERSIST", key, true, 0, 0, 0, 0, "s", key);
}

int redis_client::persist(const std::string &key)
{
    query_command_macro_1("PERSIST", key, true, 0, 0, 0, 0, "S", &key);
}

int redis_client::pexpire(const char *key, long timeout_ms)
{
    query_command_macro_1("PEXPIRE", key, true, 0, 0, 0, 0, "sd", key, timeout_ms);
}

int redis_client::pexpire(const std::string &key, long timeout_ms)
{
    query_command_macro_1("PEXPIRE", key, true, 0, 0, 0, 0, "Sd", &key, timeout_ms);
}

int redis_client::pexpireat(const char *key, long timeout_at_ms)
{
    query_command_macro_1("PEXPIREAT", key, true, 0, 0, 0, 0, "sd", key, timeout_at_ms);
}

int redis_client::pexpireat(const std::string &key, long timeout_at_ms)
{
    query_command_macro_1("PEXPIREAT", key, true, 0, 0, 0, 0, "Sd", &key, timeout_at_ms);
}

int redis_client::pttl(long &left_ms, const char *key)
{
    query_command_macro_1("PTTL", key, false, &left_ms, 0, 0, 0, "s", key);
}

int redis_client::pttl(long &left_ms, const std::string &key)
{
    query_command_macro_1("PTTL", key, false, &left_ms, 0, 0, 0, "S", &key);
}

int redis_client::randomkey(std::string &val)
{
    query_command_macro_1("RANDOMKEY", "", false, 0, &val, 0, 0, 0);
}

int redis_client::rename(const char *key, const char *newkey)
{
    query_command_macro_1("RENAME", key, true, 0, 0, 0, 0, "ss", key, newkey);
}

int redis_client::rename(const std::string &key, const std::string &newkey)
{
    query_command_macro_1("RENAME", key, true, 0, 0, 0, 0, "SS", &key, &newkey);
}

int redis_client::renamenx(const char *key, const char *newkey)
{
    query_command_macro_1("RENAMENX", key, true, 0, 0, 0, 0, "ss", key, newkey);
}

int redis_client::renamenx(const std::string &key, const std::string &newkey)
{
    query_command_macro_1("RENAMENX", key, true, 0, 0, 0, 0, "SS", &key, &newkey);
}

int redis_client::restore(const char *key, long timeout_ms, const char *data)
{
    query_command_macro_1("RESTORE", key, true, 0, 0, 0, 0, "sds", key, timeout_ms<0?0:timeout_ms, data);
}

int redis_client::restore(const std::string &key, long timeout_ms, const std::string &data)
{
    query_command_macro_1("RESTORE", key, true, 0, 0, 0, 0, "SdS", &key, timeout_ms<0?0:timeout_ms, &data);
}

static inline char *___buf_printf_long(char *buf, long d)
{
    sprintf(buf, "%ld", d);
    return buf;
}

static inline char *___buf_printf_double(char *buf, double f)
{
    sprintf(buf, "%lf", f);
    return buf;
}

int redis_client::sort(std::list<std::string> &sort_result, const char *key, const char *by, long offset, long count, const std::list<std::string> *get, bool desc, bool alpha)
{
#define ___sort_1231123 \
    if ((offset < 0) || (count < 0)) { return 0; } \
    redis_string_list qlist; qlist.append(key); \
    if (!empty(by)) { qlist.append("BY"); qlist.append(by); } \
    if (count > 0) { qlist.append("LIMIT"); qlist.append(offset<0?0:offset); qlist.append(count); } \
    if (get) { for (std::list<std::string>::const_iterator it = get->begin(); it != get->end(); it++) { \
            qlist.append("GET"); qlist.append(*it); } } \
    if (desc) { qlist.append("DESC"); } \
    if (alpha) { qlist.append("ALPHA"); }
    ___sort_1231123;
    query_command_macro_1("SORT", key, false, 0, 0, &sort_result, 0, "sL", key, &qlist);
}

int redis_client::sort(long &saved_count, const char *key, const char *by, long offset, long count, const std::list<std::string> *get, bool desc, bool alpha, const char *store)
{
    ___sort_1231123;
    if (empty(store)) {
        return 0;
    }
    qlist.push_back("STORE");
    qlist.push_back(store);
    query_command_macro_1("SORT", key, true, 0, 0, 0, 0, "sL", key, &qlist);
#undef ___sort_1231123
}

int redis_client::ttl(long &left_second, const char *key)
{
    query_command_macro_1("TTL", key, false, &left_second, 0, 0, 0, "s", key);
}

int redis_client::ttl(long &left_second, const std::string &key)
{
    query_command_macro_1("TTL", key, false, &left_second, 0, 0, 0, "S", &key);
}

int redis_client::type(std::string &key_type, const char *key)
{
    query_command_macro_1("TYPE", key, false, 0, &key_type, 0, 0, "s", key);
}

int redis_client::type(std::string &key_type, const std::string &key)
{
    query_command_macro_1("TYPE", key, false, 0, &key_type, 0, 0, "S", &key);
}

int redis_client::_scan(const char *cmd, std::list<std::string> &result, const char *key, size_t klen, long &cursor, const char *pattern, size_t plen, long count)
{
    redis_string_list qlist;
    qlist.append(cmd);
    if (klen) {
        qlist.append(key, klen);
    }
    qlist.append(cursor);
    if (plen) {
        qlist.append("MATCH").append(pattern, plen);
    }
    if (count > 0) {
        qlist.append("COUNT").append(count);
    }

    std::string tmpkey;
    if (klen) {
        tmpkey.append(key, klen);
    } else {
        tmpkey = key?key:"";
    }
    query_command_macro_2(cmd, tmpkey, false, 0, 0, &result, 0, "L", &qlist);
    if ((ret == 1)) {
        if (result.size()) {
            std::list<std::string>::iterator it = result.begin();
            cursor = atoi(it->c_str());
            result.erase(it);
        } else {
            ret = -1;
        }
    }
    return ret;
}

int redis_client::scan(std::list<std::string> &result, long &cursor, const char *pattern, long count)
{
    return _scan("SCAN", result, 0, 0, cursor, pattern, pattern?::strlen(pattern):0, count);
}

int redis_client::scan(std::list<std::string> &result, long &cursor, const std::string &pattern, long count)
{
    return _scan("SCAN", result, 0, 0, cursor, pattern.c_str(), pattern.size(), count);
}

/* STRING */
int redis_client::append(const char *key, const char *val)
{
    query_command_macro_1("APPEND", key, true, 0, 0, 0, 0, "ss", key, val);
}

int redis_client::append(const std::string &key, const std::string &val)
{
    query_command_macro_1("APPEND", key, true, 0, 0, 0, 0, "SS", &key, &val);
}

int redis_client::bitcount(const char *key, long start, long end)
{
    if (start == 999999999L) {
        query_command_macro_1("BITCOUNT", key, false, 0, 0, 0, 0, "s", key);
    } else {
        query_command_macro_1("BITCOUNT", key, false, 0, 0, 0, 0, "sdd", key, start, end);
    }
}

int redis_client::bitcount(const std::string &key, long start, long end)
{
    if (start == 999999999L) {
        query_command_macro_1("BITCOUNT", key, false, 0, 0, 0, 0, "S", &key);
    } else {
        query_command_macro_1("BITCOUNT", key, false, 0, 0, 0, 0, "Sdd", &key, start, end);
    }
}

int redis_client::bitop(const std::string &op, const std::string &destkey, const std::list<std::string> &keys)
{
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "SSL", &op, &destkey, &keys);
}

int redis_client::bitop(const char *op, const char *destkey, const char *key1, _nine_token_s)
{
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "sss" _nine_fmt_s, op, destkey, key1, _nine_token_p);
}

int redis_client::bitop(const std::string &op, const std::string &destkey, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "SSS" _nine_fmt_S, &op, &destkey, &key1, _nine_token_P);
}

#define redis_bitop_op(func, op) \
int redis_client::func(const std::string &destkey, const std::list<std::string> &keys)\
{  \
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "sSL", op, &destkey, &keys); \
} \
int redis_client::func(const char *destkey, const char *key1, _nine_token_s) \
{ \
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "sss" _nine_fmt_s, op, destkey, key1, _nine_token_p); \
} \
int redis_client::func(const std::string &destkey, const std::string &key1, _nine_token_S) \
{ \
    query_command_macro_1("BITOP",destkey,true, 0, 0, 0, 0, "sSS"_nine_fmt_S, op, &destkey, &key1, _nine_token_P); \
}

redis_bitop_op(bitop_and, "AND");
redis_bitop_op(bitop_or, "OR");
redis_bitop_op(bitop_xor, "XOR");
#undef redis_bitop_op

int redis_client::bitop_not(const char *destkey, const char *key)
{ 
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "sss", "NOT", destkey, key);
}

int redis_client::bitop_not(const std::string &destkey, const std::string &key)
{ 
    query_command_macro_1("BITOP", destkey, true, 0, 0, 0, 0, "sSS", "NOT", &destkey, &key);
}

int redis_client::decr(long &val, const char *key)
{
    query_command_macro_1("DECR", key, true, &val, 0, 0, 0, "s", key);
}

int redis_client::decr(long &val, const std::string &key)
{
    query_command_macro_1("DECR", key, true, &val, 0, 0, 0, "S", &key);
}

int redis_client::decrby(long &val, const char *key, long decrement)
{
    query_command_macro_1("DECRBY", key, true, &val, 0, 0, 0, "sd", key, decrement);
}

int redis_client::decrby(long &val, const std::string &key, long decrement)
{
    query_command_macro_1("DECRBY", key, true, &val, 0, 0, 0, "Sd", &key, decrement);
}

int redis_client::get(std::string &val_result, const char *key)
{
    query_command_macro_1("GET", key, false, 0, &val_result, 0, 0, "s", key);
}

int redis_client::get(std::string &val_result, const std::string &key)
{
    query_command_macro_1("GET", key, false, 0, &val_result, 0, 0, "S", &key);
}

int redis_client::getbit(long &val, const char *key, long offset)
{
    query_command_macro_1("GETBIT", key, false, &val, 0, 0, 0, "sd", key, offset);
}

int redis_client::getbit(long &val, const std::string &key, long offset)
{
    query_command_macro_1("GETBIT", key, false, &val, 0, 0, 0, "Sd", &key, offset);
}

int redis_client::getrange(std::string &val, const char *key, long start, long end)
{
    query_command_macro_1("GETRANGE", key, false, 0, &val, 0, 0, "sdd", key, start, end);
}

int redis_client::getrange(std::string &val, const std::string &key, long start, long end)
{
    query_command_macro_1("GETRANGE", key, false, 0, &val, 0, 0, "Sdd", &key, start, end);
}

int redis_client::getset(std::string &oldval, const char *key, const char *val)
{
    query_command_macro_1("GETSET", key, true, 0, &oldval, 0, 0, "ss", key, val);
}

int redis_client::getset(std::string &oldval, const std::string &key, const std::string &val)
{
    query_command_macro_1("GETSET", key, true, 0, &oldval, 0, 0, "SS", &key, &val);
}

int redis_client::incr(long &val, const char *key)
{
    query_command_macro_1("INCR", key, true, &val, 0, 0, 0, "s", key);
}

int redis_client::incr(long &val, const std::string &key)
{
    query_command_macro_1("INCR", key, true, &val, 0, 0, 0, "S", &key);
}

int redis_client::incrby(long &val, const char *key, long increment)
{
    query_command_macro_1("INCRBY", key, true, &val, 0, 0, 0, "sd", key, increment);
}

int redis_client::incrby(long &val, const std::string &key, long increment)
{
    query_command_macro_1("INCRBY", key, true, &val, 0, 0, 0, "Sd", &key, increment);
}

int redis_client::incrbyfloat(double &val, const char *key, double increment)
{
    std::string sval;
    query_command_macro_2("INCRBYFLOAT", key, true, 0, &sval, 0, 0, "sf", key, increment);
    if (ret == 1) {
        val = atof(sval.c_str());
    }
    return ret;
}

int redis_client::incrbyfloat(double &val, const std::string &key, double increment)
{
    std::string sval;
    query_command_macro_2("INCRBYFLOAT", key, true, 0, &sval, 0, 0, "Sf", &key, increment);
    if (ret == 1) {
        val = atof(sval.c_str());
    }
    return ret;
}

int redis_client::incrbyfloat(std::string &val, const std::string &key, const std::string &increment)
{
    query_command_macro_1("INCRBYFLOAT", key, true, 0, &val, 0, 0, "SS", &key, &increment);
}

int redis_client::mget(std::list<std::string> &val, const std::list<std::string> &keys)
{
    if (keys.empty()) {
        return 0;
    }
    query_command_macro_1("MGET", keys.front(), false, 0, 0, &val, 0, "L", &keys);
}

int redis_client::mget(std::list<std::string> &val, const char *key1, _nine_token_s)
{
    query_command_macro_1("MGET", key1, false, 0, 0, &val, 0, "s" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::mget(std::list<std::string> &val, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("MGET", key1, false, 0, 0, &val, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::mset(const std::list<std::string> &key_val_list)
{
    if (key_val_list.empty()){
        return 0;
    }
    query_command_macro_1("MSET", key_val_list.front(), true, 0, 0, 0, 0, "L", &key_val_list);
}

int redis_client::mset(const char *key1, const char *val1, _nine_token2_s)
{
    query_command_macro_1("MSET", key1, true, 0, 0, 0, 0, "ss" _nine_fmt_s _nine_fmt_s, key1, val1, _nine_token2_p);
}

int redis_client::mset(const std::string &key1, const std::string &val1, _nine_token2_S)
{
    query_command_macro_1("MSET", key1, true, 0, 0, 0, 0, "SS" _nine_fmt_S _nine_fmt_S, &key1, &val1, _nine_token2_P);
}

int redis_client::msetnx(const std::list<std::string> &key_val_list)
{
    if (key_val_list.empty()){
        return 0;
    }
    query_command_macro_1("MSETNX", key_val_list.front(), true, 0, 0, 0, 0, "L", &key_val_list);
}

int redis_client::msetnx(const char *key1, const char *val1, _nine_token2_s)
{
    query_command_macro_1("MSETNX", key1, true, 0, 0, 0, 0, "ss" _nine_fmt_s _nine_fmt_s, key1, val1, _nine_token2_p);
}

int redis_client::msetnx(const std::string &key1, const std::string &val1, _nine_token2_S)
{
    query_command_macro_1("MSETNX", key1, true, 0, 0, 0, 0, "SS" _nine_fmt_S _nine_fmt_S, &key1, &val1, _nine_token2_P);
}

int redis_client::psetex(const char *key, long milliseconds, const char *val)
{
    query_command_macro_1("PSETEX", key, true, 0, 0, 0, 0, "sds", key, milliseconds<0?0:milliseconds, val);
}

int redis_client::psetex(const std::string &key, long milliseconds, const std::string &val)
{
    query_command_macro_1("PSETEX", key, true, 0, 0, 0, 0, "SdS", &key, milliseconds<0?0:milliseconds, &val);
}

int redis_client::set(const char *key, const char *val, long EX, long PX, bool NX, bool XX)
{
    if (empty(key)) {
        return 0;
    }
    std::string tmpkey(key), tmpval(val?val:"");
    return set(tmpkey, tmpval, EX, PX, NX, XX);
}

int redis_client::set(const std::string &key, const std::string &val, long EX, long PX, bool NX, bool XX)
{
    redis_string_list qlist;
    qlist.append("SET").append(key).append(val);
    if (EX > 0) {
        qlist.append("EX").append(EX);
    } else if (PX > 0) {
        qlist.append("PX").append(PX);
    }
    if (NX) {
        qlist.append("NX");
    } else if (XX) {
        qlist.append("XX");
    }
    query_command_macro_1("SET", key, true, 0, 0, 0, 0, "L", &qlist);
}

int redis_client::setbit(const char *key, long offset, long value)
{
    query_command_macro_1("SETBIT", key, true, 0, 0, 0, 0, "sdd", key, offset, value);
}

int redis_client::setbit(const std::string &key, long offset, long value)
{
    query_command_macro_1("SETBIT", key, true, 0, 0, 0, 0, "Sdd", &key, offset, value);
}

int redis_client::setex(const char *key, long seconds, const char *val)
{
    query_command_macro_1("SETEX", key, true, 0, 0, 0, 0, "sds", key, seconds<0?0:seconds, val);
}

int redis_client::setex(const std::string &key, long seconds, const std::string &val)
{
    query_command_macro_1("SETEX", key, true, 0, 0, 0, 0, "SdS", &key, seconds<0?0:seconds, &val);
}

int redis_client::setnx(const char *key, const char *val)
{
    query_command_macro_1("SETNX", key, true, 0, 0, 0, 0, "ss", key, val);
}

int redis_client::setnx(const std::string &key, const std::string &val)
{
    query_command_macro_1("SETNX", key, true, 0, 0, 0, 0, "SS", &key, &val);
}

int redis_client::setrange(const char *key, long offset, const char *val)
{
    query_command_macro_1("SETRANGE", key, true, 0, 0, 0, 0, "sds", key, offset, val);
}

int redis_client::setrange(const std::string &key, long offset, const std::string &val)
{
    query_command_macro_1("SETRANGE", key, true, 0, 0, 0, 0, "SdS", &key, offset, &val);
}

int redis_client::strlen(const char *key)
{
    query_command_macro_1("STRLEN", key, false, 0, 0, 0, 0, "s", key);
}

int redis_client::strlen(const std::string &key)
{
    query_command_macro_1("STRLEN", key, false, 0, 0, 0, 0, "S", &key);
}

int redis_client::hdel(const char *key, const char *field1, _nine_token_s)
{
    query_command_macro_1("HDEL", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, field1, _nine_token_p);
}

int redis_client::hdel(const std::string &key, const std::string &field1, _nine_token_S)
{
    query_command_macro_1("HDEL", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &field1, _nine_token_P);
}

int redis_client::hdel(const std::string &key, const std::list<std::string> &field_list)
{
    query_command_macro_1("HDEL", key, true, 0, 0, 0, 0, "SL", &key, &field_list);
}

int redis_client::hexists(const char *key, const char *field)
{
    query_command_macro_1("HEXISTS", key, false, 0, 0, 0, 0, "ss", key, field);
}

int redis_client::hexists(const std::string &key, const std::string &field)
{
    query_command_macro_1("HEXISTS", key, false, 0, 0, 0, 0, "SS", &key, &field);
}

int redis_client::hget(std::string &val, const char *key, const char *field)
{
    query_command_macro_1("HGET", key, false, 0, &val, 0, 0, "ss", key, field);
}

int redis_client::hget(std::string &val, const std::string &key, const std::string &field)
{
    query_command_macro_1("HGET", key, false, 0, &val, 0, 0, "SS", &key, &field);
}

int redis_client::hgetall(std::list<std::string> &field_value_list, const char *key)
{
    query_command_macro_1("HGETALL", key, false, 0, 0, &field_value_list, 0, "s", key);
}

int redis_client::hgetall(std::list<std::string> &field_value_list, const std::string &key)
{
    query_command_macro_1("HGETALL", key, false, 0, 0, &field_value_list, 0, "S", &key);
}

int redis_client::hincrby(long &val, const char *key, const char *field, long increment)
{
    query_command_macro_1("HINCRBY", key, true, &val, 0, 0, 0, "ssd", key, field, increment);
}

int redis_client::hincrby(long &result, const std::string &key, const std::string &field, long increment)
{
    query_command_macro_1("HINCRBY", key, true, &result, 0, 0, 0, "SSd", &key, &field, increment);
}

int redis_client::hincrbyfloat(double &result, const char *key, const char *field, double increment)
{
    std::string sval;
    query_command_macro_2("HINCRBYFLOAT", key, true, 0, &sval, 0, 0, "ssf", key, field,  increment);
    if (ret == 1) {
        result = atof(sval.c_str());
    }
    return ret;
}

int redis_client::hincrbyfloat(double &result, std::string &key, std::string &field, double increment)
{
    std::string sval;
    query_command_macro_2("HINCRBYFLOAT", key, true, 0, &sval, 0, 0, "SSf", &key, &field,  increment);
    if (ret == 1) {
        result = atof(sval.c_str());
    }
    return ret;
}

int redis_client::hincrbyfloat(std::string &result, const std::string &key, const std::string &field, const std::string &increment)
{
    query_command_macro_1("HINCRBYFLOAT", key, true, 0, &result, 0, 0, "SSS", &key, &field, &increment);
}

int redis_client::hkeys(std::list<std::string> &field_list, const char *key)
{
    query_command_macro_1("HKEYS", key, false, 0, 0, &field_list, 0, "s", key);
}

int redis_client::hkeys(std::list<std::string> &field_list, const std::string &key)
{
    query_command_macro_1("HKEYS", key, false, 0, 0, &field_list, 0, "S", &key);
}

int redis_client::hlen(const char *key)
{
    query_command_macro_1("HLEN", key, false, 0, 0, 0, 0, "s", key);
}

int redis_client::hlen(const std::string &key)
{
    query_command_macro_1("HLEN", key, false, 0, 0, 0, 0, "S", &key);
}

int redis_client::hmget(std::list<std::string> &val_list, const std::string &key, const std::list<std::string> &field_list)
{
    query_command_macro_1("HMGET", key, false, 0, 0, &val_list, 0, "SL", &key, &field_list);
}

int redis_client::hmget(std::list<std::string> &val_list, const char *key, const char *field1, _nine_token_s)
{
    query_command_macro_1("HMGET", key, false, 0, 0, &val_list, 0, "ss" _nine_fmt_s, key, field1, _nine_token_p);
}

int redis_client::hmget(std::list<std::string> &val_list, const std::string &key, const std::string &field1, _nine_token_S)
{
    query_command_macro_1("HMGET", key, false, 0, 0, &val_list, 0, "SS" _nine_fmt_S, &key, &field1, _nine_token_P);
}

int redis_client::hmset(const std::string &key, const std::list<std::string> &field_val_list)
{
    query_command_macro_1("HMSET", key, false, 0, 0, 0, 0, "SL", &key, &field_val_list);
}

int redis_client::hmset(const char *key, const char *field1, const char *val1, _nine_token2_s)
{
    query_command_macro_1("HMSET", key, false, 0, 0, 0, 0, "sss" _nine_fmt_s _nine_fmt_s, key, field1, val1, _nine_token2_p);
}

int redis_client::hmset(const std::string &key, const std::string &field1, const std::string &val1, _nine_token2_S)
{
    query_command_macro_1("HMSET", key, false, 0, 0, 0, 0, "SSS" _nine_fmt_S _nine_fmt_S, &key, &field1, &val1, _nine_token2_P);
}

int redis_client::hmsetnx(const std::string &key, const std::list<std::string> &field_val_list)
{
    query_command_macro_1("HMSETNX", key, false, 0, 0, 0, 0, "SL", &key, &field_val_list);
}

int redis_client::hmsetnx(const char *key, const char *field1, const char *val1, _nine_token2_s)
{
    query_command_macro_1("HMSETNX", key, false, 0, 0, 0, 0, "sss" _nine_fmt_s _nine_fmt_s, key, field1, val1, _nine_token2_p);
}

int redis_client::hmsetnx(const std::string &key, const std::string &field1, const std::string &val1, _nine_token2_S)
{
    query_command_macro_1("HMSETNX", key, false, 0, 0, 0, 0, "SSS" _nine_fmt_S _nine_fmt_S, &key, &field1, &val1, _nine_token2_P);
}

int redis_client::hvals(std::list<std::string> &val_list, const char *key)
{
    query_command_macro_1("HVALS", key, false, 0, 0, &val_list, 0, "s", key);
}

int redis_client::hvals(std::list<std::string> &val_list, const std::string &key)
{
    query_command_macro_1("HVALS", key, false, 0, 0, &val_list, 0, "S", &key);
}

int redis_client::hscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern, long count)
{
    return _scan("HSCAN", result, key, key?::strlen(key):0, cursor, pattern, pattern?::strlen(pattern):0, count);
}

int redis_client::hscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern, long count)
{
    return _scan("HSCAN", result, key.c_str(), key.size(), cursor, pattern.c_str(), pattern.size(), count);
}

int redis_client::blpop(std::string &key_pop, std::string &val_pop, long timeout, const char *key1, _nine_token_s)
{
    std::list<std::string> result;
    query_command_macro_2("BLPOP", key1, true, 0, 0, &result, 0, "ds" _nine_fmt_s, timeout, key1, _nine_token_p);
#define ___blpop_result  \
    if (ret == 1)  { if (result.size() == 2) { std::list<std::string>::iterator it = result.begin(); \
            key_pop = *it; it++; val_pop = *it; } else { ret = 0; } } return ret;
    ___blpop_result;
}

int redis_client::blpop(std::string &key_pop, std::string &val_pop, long timeout, const std::string &key1, _nine_token_S)
{
    std::list<std::string> result;
    query_command_macro_2("BLPOP", key1, true, 0, 0, &result, 0, "dS" _nine_fmt_S, timeout, &key1, _nine_token_P);
    ___blpop_result;
}

int redis_client::blpop(std::string &key_pop, std::string &val_pop, long timeout, const std::list<std::string> &keys)
{
    if (keys.empty()) {
        return 0;
    }
    std::list<std::string> result;
    query_command_macro_2("BLPOP", keys.front(), true, 0, 0, &result, 0, "dL", timeout, &keys);
    ___blpop_result;
}

int redis_client::brpop(std::string &key_pop, std::string &val_pop, long timeout, const char *key1, _nine_token_s)
{
    std::list<std::string> result;
    query_command_macro_2("BRPOP", key1, true, 0, 0, &result, 0, "ds" _nine_fmt_s, timeout, key1, _nine_token_p);
    ___blpop_result ;
}

int redis_client::brpop(std::string &key_pop, std::string &val_pop, long timeout, const std::string &key1, _nine_token_S)
{
    std::list<std::string> result;
    query_command_macro_2("BRPOP", key1, true, 0, 0, &result, 0, "dS" _nine_fmt_S, timeout, &key1, _nine_token_P);
    ___blpop_result;
}

int redis_client::brpop(std::string &key_pop, std::string &val_pop, long timeout, const std::list<std::string> &keys)
{
    if (keys.empty()) {
        return 0;
    }
    std::list<std::string> result;
    query_command_macro_2("BRPOP", keys.front(), true, 0, 0, &result, 0, "dL", timeout, &keys);
    ___blpop_result;
}

int redis_client::brpoplpush(std::string &val, const char *source_key, const char *dest_key, long timeout)
{
    query_command_macro_1("BRPOPLPUSH", source_key, true, 0, &val, 0, 0, "ssd", source_key, dest_key, timeout);
}

int redis_client::brpoplpush(std::string &val, const std::string &source_key, const std::string &dest_key, long timeout)
{
    query_command_macro_1("BRPOPLPUSH", source_key, true, 0, &val, 0, 0, "SSd", &source_key, &dest_key, timeout);
}

int redis_client::lindex(std::string &val, const char *key, long idx)
{

    query_command_macro_1("LINDEX", key, false, 0, &val, 0, 0, "sd", key, idx);
}

int redis_client::lindex(std::string &val, const std::string &key, long idx)
{
    query_command_macro_1("LINDEX", key, false, 0, &val, 0, 0,"Sd", &key, idx);
}

int redis_client::linsert(long &result, const char *key, bool before, const char *pivot, const char *value)
{
    query_command_macro_1("LINDEX", key, true, &result, 0, 0, 0,"ssss", key, before?"BEFORE":"AFTER", pivot, value);
}

int redis_client::linsert(long &result, const std::string &key, bool before, const std::string &pivot, const std::string &value)
{
    query_command_macro_1("LINDEX", key, true, &result,0,0,0,"SsSS", &key, before?"BEFORE":"AFTER", &pivot,& value);
}

int redis_client::llen(const char *key)
{
    query_command_macro_1("LLEN", key, false, 0, 0, 0, 0,"s", key);
}

int redis_client::llen(const std::string &key)
{
    query_command_macro_1("LLEN", key, false, 0, 0, 0, 0,"S", &key);
}

int redis_client::lpop(std::string &val_pop, const char *key)
{
    query_command_macro_1("LPOP", key, true, 0, &val_pop, 0, 0, "s", key);
}

int redis_client::lpop(std::string &val_pop, const std::string &key)
{
    query_command_macro_1("LPOP", key, true, 0, &val_pop, 0, 0, "S", &key);
}

int redis_client::lpush(const char *key, const char *value1, _nine_token_s)
{
    query_command_macro_1("LPUSH", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, value1, _nine_token_p);
}

int redis_client::lpush(const std::string &key, const std::string &value1, _nine_token_S)
{
    query_command_macro_1("LPUSH", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &value1, _nine_token_P);
}

int redis_client::lpush(const std::string &key, const std::list<std::string> &val_list)
{
    query_command_macro_1("LPUSH", key, true, 0, 0, 0, 0, "SL", &key, &val_list);
}

int redis_client::lpushx(const char *key, const char *value)
{
    query_command_macro_1("LPUSHX", key, true, 0, 0, 0, 0, "ss", key, value);
}

int redis_client::lpushx(const std::string &key, const std::string &value)
{
    query_command_macro_1("LPUSHX", key, true, 0, 0, 0, 0, "SS", &key, &value);
}

int redis_client::lrange(std::list<std::string> &val_list, const char *key, long start, long end)
{
    query_command_macro_1("LRANGE", key, false, 0, 0, &val_list, 0, "sdd", key, start, end);
}

int redis_client::lrange(std::list<std::string> &val_list, const std::string &key, long start, long end)
{
    query_command_macro_1("LRANGE", key, false, 0, 0, &val_list, 0, "Sdd", &key, start, end);
}

int redis_client::lrem(const char *key, long count, const char *value)
{
    query_command_macro_1("LREM", key, true, 0, 0, 0, 0, "sds", key, count, value);
}

int redis_client::lrem(const std::string &key, long count, const std::string &value)
{
    query_command_macro_1("LREM", key, true, 0, 0, 0, 0, "SdS", &key, count, &value);
}

int redis_client::lrem_all(const char *key, const char *value)
{
    query_command_macro_1("LREM", key, true, 0, 0, 0, 0, "sds", key, 0, value);
}

int redis_client::lrem_all(const std::string &key, const std::string &value)
{
    query_command_macro_1("LREM", key, true, 0, 0, 0, 0, "SdS", &key, 0, &value);
}

int redis_client::lset(const char *key, long index, const char *value)
{
    query_command_macro_1("LSET", key, true, 0, 0, 0, 0, "sds", key, index, value);
}

int redis_client::lset(const std::string &key, long index, const std::string &value)
{
    query_command_macro_1("LSET", key, true, 0, 0, 0, 0, "SdS", &key, index, &value);
}

int redis_client::ltrim(const char *key, long start, long end)
{
    query_command_macro_1("LTRIM", key, true, 0, 0, 0, 0, "sdd", key, start, end);
}

int redis_client::ltrim(const std::string &key, long start, long end)
{
    query_command_macro_1("LTRIM", key, true, 0, 0, 0, 0, "Sdd", &key, start, end);
}

int redis_client::rpop(std::string &val_pop, const char *key)
{
    query_command_macro_1("RPOP", key, true, 0, &val_pop, 0, 0, "s", key);
}

int redis_client::rpop(std::string &val_pop, const std::string &key)
{
    query_command_macro_1("RPOP", key, true, 0, &val_pop, 0, 0, "S", &key);
}

int redis_client::rpoplpush(std::string &val, const char *source_key, const char *dest_key)
{
    query_command_macro_1("RPOPLPUSH", source_key, true, 0, &val, 0, 0, "ss", source_key, dest_key);
}

int redis_client::rpoplpush(std::string &val, const std::string &source_key, const std::string &dest_key)
{
    query_command_macro_1("RPOPLPUSH", source_key, true, 0, &val, 0, 0, "SS", &source_key, &dest_key);
}

int redis_client::rpush(const char *key, const char *value1, _nine_token_s)
{
    query_command_macro_1("RPUSH", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, value1, _nine_token_p);
}

int redis_client::rpush(const std::string &key, const std::string &value1, _nine_token_S)
{
    query_command_macro_1("RPUSH", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &value1, _nine_token_P);
}

int redis_client::rpush(const std::string &key, const std::list<std::string> &val_list)
{
    query_command_macro_1("RPUSH", key, true, 0, 0, 0, 0, "SL", &key, &val_list);
}

int redis_client::rpushx(const char *key, const char *value)
{
    query_command_macro_1("LPUSHX", key, true, 0, 0, 0, 0, "ss", key, value);
}

int redis_client::rpushx(const std::string &key, const std::string &value)
{
    query_command_macro_1("LPUSHX", key, true, 0, 0, 0, 0, "SS", &key, &value);
}

int redis_client::sadd(const char *key, const char *member1, _nine_token_s)
{
    query_command_macro_1("SADD", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, member1, _nine_token_p);
}

int redis_client::sadd(const std::string &key, const std::string &member1, _nine_token_S)
{
    query_command_macro_1("SADD", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &member1, _nine_token_P);
}

int redis_client::sadd(const std::string &key, const std::list<std::string> &val_list)
{
    query_command_macro_1("SADD", key, true, 0, 0, 0, 0, "SL", &key, &val_list);
}

int redis_client::scard(const char *key)
{
    query_command_macro_1("SCARD", key, false, 0, 0, 0, 0, "s", key);
}

int redis_client::scard(const std::string &key)
{
    query_command_macro_1("SCARD", key, false, 0, 0, 0, 0, "S", &key);
}

int redis_client::sdiff(std::list<std::string> &members_diff, const char *key1, _nine_token_s)
{
    query_command_macro_1("SDIFF", key1, false, 0, 0, 0, 0, "ss" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::sdiff(std::list<std::string> &members_diff, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("SDIFF", key1, false, 0, 0, 0, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::sdiff(std::list<std::string> &members_diff, const std::list<std::string> &key_list)
{
    if (key_list.empty()) {
        return 0;
    }
    query_command_macro_1("SDIFF", key_list.front(), false, 0, 0, &members_diff, 0, "L",  &key_list);
}

int redis_client::sdiffstore(const char *dest_key, const char *key1, _nine_token_s)
{
    query_command_macro_1("SDIFFSTORE", dest_key, true, 0,0,0,0,"ss" _nine_fmt_s, dest_key, key1, _nine_token_p);
}

int redis_client::sdiffstore(const std::string &dest_key, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("SDIFFSTORE", dest_key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &dest_key, &key1, _nine_token_P);
}

int redis_client::sdiffstore(const std::string &dest_key, const std::list<std::string> &key_list)
{
    query_command_macro_1("SDIFFSTORE", dest_key, true, 0, 0, 0, 0, "SL",  &dest_key, &key_list);
}

int redis_client::sinter(std::list<std::string> &members_inter, const char *key1, _nine_token_s)
{
    query_command_macro_1("SINTER", key1, false, 0, 0, 0, 0, "s" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::sinter(std::list<std::string> &members_inter, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("SINTER", key1, false, 0, 0, 0, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::sinter(std::list<std::string> &members_inter, const std::list<std::string> &key_list)
{
    query_command_macro_1("SINTER", key_list.front(), false, 0, 0, &members_inter, 0, "L",  &key_list);
}

int redis_client::sinterstore(const char *dest_key, const char *key1, _nine_token_s)
{
    query_command_macro_1("SINTERSTORE", dest_key, true,0, 0, 0, 0, "ss" _nine_fmt_s, dest_key, key1, _nine_token_p);
}

int redis_client::sinterstore(const std::string &dest_key, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("SINTERSTORE", dest_key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &dest_key, &key1, _nine_token_P);
}

int redis_client::sinterstore(const std::string &dest_key, const std::list<std::string> &key_list)
{
    query_command_macro_1("SINTERSTORE", dest_key, true, 0, 0, 0, 0, "SL",  &dest_key, &key_list);
}

int redis_client::sismember(const char *key, const char *value)
{
    query_command_macro_1("SISMEMBER", key, false, 0, 0, 0, 0, "ss", key, value);
}

int redis_client::sismember(const std::string &key, const std::string &value)
{
    query_command_macro_1("SISMEMBER", key, false, 0, 0, 0, 0, "SS", &key, &value);
}


int redis_client::smove(const char *source_key, const char *dest_key, const char *value)
{
    query_command_macro_1("SMOVE", source_key, true, 0, 0, 0, 0, "sss", source_key, dest_key, value);
}

int redis_client::smove(const std::string &source_key, const std::string &dest_key, const std::string &value)
{
    query_command_macro_1("SMOVE", source_key, true, 0, 0, 0, 0, "SSS", &source_key, &dest_key, &value);
}

int redis_client::spop(std::string &val_pop, const char *key)
{
    query_command_macro_1("SPOP", key, true, 0, &val_pop, 0, 0, "s", key);
}

int redis_client::spop(std::string &val_pop, const std::string &key)
{
    query_command_macro_1("SPOP", key, true, 0, &val_pop, 0, 0, "S", &key);
}

int redis_client::srandmember(std::string &result, const char *key)
{
    query_command_macro_1("SRANDMEMBER", key, false, 0, &result, 0, 0, "s", key);
}

int redis_client::srandmember(std::string &result, const std::string &key)
{
    query_command_macro_1("SRANDMEMBER", key, false, 0, &result, 0, 0, "S", &key);
}

int redis_client::srandmember(std::list<std::string> &result, const char *key, long count)
{
    query_command_macro_1("SRANDMEMBER", key, false, 0, 0, &result, 0, "sd", key, count);
}

int redis_client::srandmember(std::list<std::string> &result, const std::string &key, long count)
{
    query_command_macro_1("SRANDMEMBER", key, false, 0, 0, &result, 0, "Sd", &key, count);
}

int redis_client::srem(const char *key, const char *member1, _nine_token_s)
{
    query_command_macro_1("SREM", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, member1, _nine_token_p);
}

int redis_client::srem(const std::string &key, const std::string &member1, _nine_token_S)
{
    query_command_macro_1("SREM", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &member1, _nine_token_P);
}

int redis_client::srem(const std::string &key, const std::list<std::string> &val_list)
{
    query_command_macro_1("SREM", key, true, 0, 0, 0, 0, "SL", &key, &val_list);
}

int redis_client::sunion(std::list<std::string> &val, const std::list<std::string> &keys)
{
    if (keys.empty()) {
        return 0;
    }
    query_command_macro_1("SUNION", keys.front(), false, 0, 0, &val, 0, "L", &keys);
}

int redis_client::sunion(std::list<std::string> &val, const char *key1, _nine_token_s)
{
    query_command_macro_1("SUNION", key1, false, 0, 0, &val, 0, "s" _nine_fmt_s, key1, _nine_token_p);
}

int redis_client::sunion(std::list<std::string> &val, const std::string &key1, _nine_token_S)
{
    query_command_macro_1("SUNION", key1, false, 0, 0, &val, 0, "S" _nine_fmt_S, &key1, _nine_token_P);
}

int redis_client::sscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern, long count)
{
    return _scan("SSCAN", result, key, key?::strlen(key):0, cursor, pattern, pattern?::strlen(pattern):0, count);
}

int redis_client::sscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern, long count)
{
    return _scan("SSCAN", result, key.c_str(), key.size(), cursor, pattern.c_str(), pattern.size(), count);
}


int redis_client::zadd(const std::string &key, const std::list<std::string> &score_member_list)
{
    query_command_macro_1("ZADD", key, true, 0, 0, 0, 0, "SL", &key, &score_member_list);
}

int redis_client::zadd(const char *key, double score, const char *member)
{
    query_command_macro_1("ZADD", key, true, 0, 0, 0, 0, "sfs", key, score, member);
}

int redis_client::zadd(const std::string &key, double score, const std::string &member)
{
    query_command_macro_1("ZADD", key, true, 0, 0, 0, 0, "SfS", &key, score, &member);
}

int redis_client::zadd(const std::string &key, const std::string &score, const std::string &member)
{
    query_command_macro_1("ZADD", key, true, 0, 0, 0, 0, "SfS", &key, &score, &member);
}

int redis_client::zcard(const char *key)
{
    query_command_macro_1("ZCARD", key, false, 0, 0, 0, 0, "s", key);
}

int redis_client::zcard(const std::string &key)
{
    query_command_macro_1("ZCARD", key, false, 0, 0, 0, 0, "S", &key);
}

int redis_client::zcount(const char *key, double min, double max)
{
    query_command_macro_1("ZCOUNT", key, false, 0, 0, 0, 0, "sff", key, min, max);
}

int redis_client::zcount(const std::string &key, double min, double max)
{
    query_command_macro_1("ZCOUNT", key, false, 0, 0, 0, 0, "Sff", &key, min, max);
}

int redis_client::zcount(const std::string &key, std::string &min, std::string &max)
{
    query_command_macro_1("ZCOUNT", key, false, 0, 0, 0, 0, "SSS", &key, &min, &max);
}

int redis_client::zincrby(double &result, const char *key, double increment, const char *member)
{
    std::string sval;
    query_command_macro_2("ZINCRBY", key, true, 0, &sval, 0, 0, "sfs", key, increment, member);
    if (ret == 1) {
        result = atof(sval.c_str());
    }
    return ret;
}

int redis_client::zincrby(double &result, std::string &key, double increment, std::string &member)
{
    std::string sval;
    query_command_macro_2("ZINCRBY", key, true, 0, &sval, 0, 0, "SfS", &key, increment, &member);
    if (ret == 1) {
        result = atof(sval.c_str());
    }
    return ret;
}

int redis_client::zincrby(std::string &result, const std::string &key, const std::string &increment, const std::string &member)
{
    query_command_macro_1("ZINCRBY", key, true, 0, &result, 0, 0, "SSS", &key, &increment, &member);
}

int redis_client::zrange(std::list<std::string> &result, const char *key, long start, long end, bool withscores)
{
    query_command_macro_1("ZRANGE", key, false, 0, 0, &result, 0, (withscores?"sdds":"sdd"), key, start, end, "WITHSCORES");
}

int redis_client::zrange(std::list<std::string> &result, const std::string &key, long start, long end, bool withscores)
{
    query_command_macro_1("ZRANGE", key, true, 0, 0, &result, 0, (withscores?"Sdds":"Sdd"), &key, start, end, "WITHSCORES");
}

int redis_client::zrangebyscore(std::list<std::string> &result, const char *key, double min, double max, bool withscores, long offset, long count)
{
#define _zrangebyscore_by_macro(_cmd) \
    redis_string_list qlist; qlist.append(_cmd).append(key).append(min).append(max); \
    if (withscores) { qlist.append("WITHSCORES"); } \
    if (count) { qlist.append("LIMIT").append(offset).append(count); } \
    query_command_macro_1(_cmd, key, false, 0, 0, &result, 0, "L", &qlist);
    _zrangebyscore_by_macro("ZRANGEBYSCORE");
}

int redis_client::zrangebyscore(std::list<std::string> &result, const std::string &key, double min, double max, bool withscores, long offset, long count)
{
    _zrangebyscore_by_macro("ZRANGEBYSCORE");
}

int redis_client::zrangebyscore(std::list<std::string> &result, const std::string &key, const std::string &min, const std::string &max, bool withscores, long offset, long count)
{
    _zrangebyscore_by_macro("ZRANGEBYSCORE");
}

int redis_client::zrank(const char *key, const char *member)
{
    query_command_macro_1("ZRANK", key, false, 0, 0, 0, 0, "ss", key, member);
}

int redis_client::zrank(std::string &key, std::string &member)
{
    query_command_macro_1("ZRANK", key, false, 0, 0, 0, 0, "SS", &key, &member);
}

int redis_client::zrem(const char *key, const char *member1, _nine_token_s)
{
    query_command_macro_1("ZREM", key, true, 0, 0, 0, 0, "ss" _nine_fmt_s, key, member1, _nine_token_p);
}

int redis_client::zrem(const std::string &key, const std::string &member1, _nine_token_S)
{
    query_command_macro_1("ZREM", key, true, 0, 0, 0, 0, "SS" _nine_fmt_S, &key, &member1, _nine_token_P);
}

int redis_client::zrem(const std::string &key, const std::list<std::string> &val_list)
{
    query_command_macro_1("ZREM", key, true, 0, 0, 0, 0, "SL", &key, &val_list);
}

int redis_client::zremrangebyrank(const char *key, long start, long end)
{
    query_command_macro_1("ZREMRANGEBYRANK", key, true, 0, 0, 0, 0, "sdd", key, start, end);
}

int redis_client::zremrangebyrank(const std::string &key, long start, long end)
{
    query_command_macro_1("ZREMRANGEBYRANK", key, true, 0, 0, 0, 0, "Sdd", &key, start, end);
}

int redis_client::zremrangebyscore(const char *key, double min, double max)
{
    query_command_macro_1("ZREMRANGEBYSCORE", key, true, 0, 0, 0, 0, "sff", min, max);
}

int redis_client::zremrangebyscore(const std::string &key, double min, double max)
{
    query_command_macro_1("ZREMRANGEBYSCORE", key, true, 0, 0, 0, 0, "Sff", &key, min, max);
}

int redis_client::zremrangebyscore(const std::string &key, const std::string &min, const std::string &max)
{
    query_command_macro_1("ZREMRANGEBYSCORE", key, true, 0, 0, 0, 0, "SSS", &key, &min, &max);
}

int redis_client::zrevrange(std::list<std::string> &result, const char *key, long start, long end, bool withscores)
{
    query_command_macro_1("ZREVRANGE", key, false, 0, 0, &result, 0, (withscores?"sdds":"sdd"), key, start, end, "WITHSCORES");
}

int redis_client::zrevrange(std::list<std::string> &result, const std::string &key, long start, long end, bool withscores)
{
    query_command_macro_1("ZREVRANGE", key, false, 0, 0, &result, 0, (withscores?"Sdds":"Sdd"), &key, start, end, "WITHSCORES");
}

int redis_client::zrevrangebyscore(std::list<std::string> &result, const char *key, double min, double max, bool withscores, long offset, long count)
{
    _zrangebyscore_by_macro("ZREMRANGEBYSCORE");
}

int redis_client::zrevrangebyscore(std::list<std::string> &result, const std::string &key, double min, double max, bool withscores, long offset, long count)
{
    _zrangebyscore_by_macro("ZREMRANGEBYSCORE");
}

int redis_client::zrevrangebyscore(std::list<std::string> &result, const std::string &key, const std::string &min, const std::string &max, bool withscores, long offset, long count)
{
    _zrangebyscore_by_macro("ZREMRANGEBYSCORE");
#undef _zrangebyscore_by_macro
}

int redis_client::zrevrank(const char *key, const char *member)
{
    query_command_macro_1("ZREVRANK", key, false, 0, 0, 0, 0, "ss", key, member);
}

int redis_client::zrevrank(std::string &key, std::string &member)
{
    query_command_macro_1("ZREVRANK", key, false, 0, 0, 0, 0, "SS", &key, &member);
}

int redis_client::zscore(const char *key, const char *member)
{
    query_command_macro_1("ZSCORE", key, false, 0, 0, 0, 0, "ss", key, member);
}

int redis_client::zscore(std::string &key, std::string &member)
{
    query_command_macro_1("ZSCORE", key, false, 0, 0, 0, 0, "SS", &key, &member);
}

int redis_client::zunionstore(const char *destination, const char *key1, _nine_token_s)
{
    if (empty(destination)||empty(key1)) {
        return 0;
    }
#define __push_k(nnn) if (1 /* XXX */) { keys.push_back(nnn); }
#define __push_kkkkk std::list<std::string> keys; \
    __push_k(key1); __push_k(n2); __push_k(n3); __push_k(n4); __push_k(n5); \
    __push_k(n6); __push_k(n7); __push_k(n8); __push_k(n9);

    __push_kkkkk;
    std::string d = destination;
    return zunionstore(d, keys);
}

int redis_client::zunionstore(const std::string &destination, const std::string &key1, _nine_token_S)
{
    __push_kkkkk;
    return zunionstore(destination, keys);
}

int redis_client::zunionstore(const std::string &destination, const std::list<std::string> &keys, const std::list<std::string> *weights, const char *aggregate)
{
    query_command_macro_1("ZUNIONSTORE", keys.front(), true, 0, 0, 0, 0, "SdLLs", &destination, keys.size(), &keys,  weights, aggregate?aggregate:(char *)-9);
}

int redis_client::zinterstore(const char *destination, const char *key1, _nine_token_s)
{
    if (empty(destination)||empty(key1)) {
        return 0;
    }
    std::string d = destination;
    __push_kkkkk;
    return zinterstore(d, keys);
}

int redis_client::zinterstore(const std::string &destination, const std::string &key1, _nine_token_S)
{
    __push_kkkkk;
    return zinterstore(destination, keys);
#undef __push_kkkkk
#undef __push_k
}

int redis_client::zinterstore(const std::string &destination, const std::list<std::string> &keys, const std::list<std::string> *weights, const char *aggregate)
{
    query_command_macro_1("ZINTERSTORE", keys.front(), true, 0, 0, 0, 0, "SdLLs", &destination, keys.size(), &keys,  weights, aggregate?aggregate:(char *)-9);
}

int redis_client::zscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern, long count)
{
    return _scan("ZSCAN", result, key, key?::strlen(key):0, cursor, pattern, pattern?::strlen(pattern):0, count);
}

int redis_client::zscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern, long count)
{
    return _scan("ZSCAN", result, key.c_str(), key.size(), cursor, pattern.c_str(), pattern.size(), count);
}

int redis_client::subscribe_get_message(std::string &channel, std::string &message, long timeout_ms)
{
    /* FIXME */
    redis_cmd_args args("@SUBSCRIBE_GET_MESSAGE");

    return -1;
}

int redis_client::psubscribe(const char *pattern, _nine_token_s)
{
    query_command_macro_1("PSUBSCRIBE", "", true, 0, 0, 0, 0, "s" _nine_fmt_s, pattern, _nine_token_p);
}

int redis_client::psubscribe(const std::string &pattern, _nine_token_S)
{
    query_command_macro_1("PSUBSCRIBE", "", true, 0, 0, 0, 0, "S" _nine_fmt_S, &pattern, _nine_token_P);
}

int redis_client::psubscribe(const std::list<std::string> &pattern_list)
{
    query_command_macro_1("PSUBSCRIBE", "", true, 0, 0, 0, 0, "L", &pattern_list);
}

int redis_client::publish(const char *channel, const char *message)
{
    query_command_macro_1("PUBLISH", "", true, 0, 0, 0, 0, "ss", channel, message);
}

int redis_client::publish(const std::string &channel, const std::string &message)
{
    query_command_macro_1("PUBLISH", "", true, 0, 0, 0, 0, "SS", &channel, &message);
}

int redis_client::pubsub_channels(std::list<std::string> &result, const char *pattern)
{
    query_command_macro_1("PUBSUB", "", false, 0, 0, &result, 0, "ss", "CHANNELS", pattern);
}

int redis_client::pubsub_channels(std::list<std::string> &result, std::string &pattern)
{
    query_command_macro_1("PUBSUB", "", false, 0, 0, &result, 0, "sS", "CHANNELS", &pattern);
}

int redis_client::pubsub_numsub(std::list<std::string> &result, const char *pattern, _nine_token_s)
{
    query_command_macro_1("PUBSUB", "", false, 0,0,&result,0, "s" _nine_fmt_s, "NUMSUB", pattern, _nine_token_p);
}

int redis_client::pubsub_numsub(std::list<std::string> &result, const std::string &pattern, _nine_token_S)
{
    query_command_macro_1("PUBSUB", "", false, 0,0,&result,0, "sS" _nine_fmt_S, "NUMSUB", &pattern, _nine_token_P);
}

int redis_client::pubsub_numsub(std::list<std::string> &result, const std::list<std::string> &pattern_list)
{
    query_command_macro_1("PUBSUB", "", false, 0, 0, &result, 0, "sL", "NUMSUB", &pattern_list);
}

int redis_client::pubsub_numpat()
{
    query_command_macro_1("PUBSUB", "", false, 0, 0, 0, 0, "s", "NUMPAT");
}

int redis_client::punsubscribe(const char *pattern, _nine_token_s)
{
    query_command_macro_1("PUNSUBSCRIBE", "", true, 0, 0, 0, 0, "s" _nine_fmt_s, pattern, _nine_token_p);
}

int redis_client::punsubscribe(const std::string &pattern, _nine_token_S)
{
    query_command_macro_1("PUNSUBSCRIBE", "", true, 0, 0, 0, 0, "S" _nine_fmt_S, &pattern, _nine_token_P);
}

int redis_client::punsubscribe(const std::list<std::string> &pattern_list)
{
    query_command_macro_1("PUNSUBSCRIBE", "", true, 0, 0, 0, 0, "L", &pattern_list);
}

int redis_client::subscribe(const char *channel, _nine_token_s)
{
    query_command_macro_1("SUBSCRIBE", "", true, 0, 0, 0, 0, "s" _nine_fmt_s, channel, _nine_token_p);
}

int redis_client::subscribe(const std::string &channel, _nine_token_S)
{
    query_command_macro_1("SUBSCRIBE", "", true, 0, 0, 0, 0, "S" _nine_fmt_S, &channel, _nine_token_P);
}

int redis_client::subscribe(const std::list<std::string> &channel_list)
{
    query_command_macro_1("SUBSCRIBE", "", true, 0, 0, 0, 0, "L", &channel_list);
}

int redis_client::unsubscribe(const char *channel, _nine_token_s)
{
    query_command_macro_1("UNSUBSCRIBE", "", true, 0, 0, 0, 0, "s" _nine_fmt_s, channel, _nine_token_p);
}

int redis_client::unsubscribe(const std::string &channel, _nine_token_S)
{
    query_command_macro_1("UNSUBSCRIBE", "", true, 0, 0, 0, 0, "S" _nine_fmt_S, &channel, _nine_token_P);
}

int redis_client::unsubscribe(const std::list<std::string> &channel_list)
{
    query_command_macro_1("UNSUBSCRIBE", "", true, 0, 0, 0, 0, "L", &channel_list);
}

}

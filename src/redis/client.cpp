/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class redis_client_standalone_engine: public redis_client_basic_engine
{
public:
    redis_client_standalone_engine(const char *_destination, const char *_password);
    ~redis_client_standalone_engine();
    int query_protocol(redis_client_query &query);
    int timed_wait_readable(long timeout);
    int timed_wait_writeable(long timeout);
private:
    void open();
    void close();
    std::string destination;
    std::string password;
    int fd;
    stream fp;
};

redis_client_standalone_engine::redis_client_standalone_engine(const char *_destination, const char *_password)
{
    destination = (_destination?_destination:"");
    password = (_password?_password:"");
    fd = -1;
}

redis_client_standalone_engine::~redis_client_standalone_engine()
{
    close();
}

int redis_client_standalone_engine::query_protocol(redis_client_query &query)
{
    if (fd == -1) {
        open();
    }
    if (fd == -1) {
        if (query.msg_info) {
            *query.msg_info = "can not open ";
            *query.msg_info += destination;
        }
        return -2;
    }
    int ret = query_protocol_io(query, fp);
    if (ret == -2) {
        close();
    }
    return ret;
}

void redis_client_standalone_engine::open()
{
    close();
    std::list<std::string> netpath_list;
    netpaths_expand(destination.c_str(), netpath_list);
    std_list_walk_begin(netpath_list, np) {
        while (fd == -1) {
            char *npp = (char *)(np.c_str());
            char *p = strchr(npp, ':');
            if (p) {
                *p++ = 0;
                fd = inet_connect(npp, atoi(p));
            } else {
                fd = unix_connect(npp);
            }
            if (fd == -1) {
                break;
            }
            break;
        }
    } std_list_walk_end;
    if (fd != -1) {
        fp.open(fd);
    }
}

void redis_client_standalone_engine::close()
{
    fp.close();
    if (fd > -1) {
        ::close(fd);
        fd = -1;
    }
}

int redis_client_standalone_engine::timed_wait_readable(long timeout_millisecond)
{
    return zcc::timed_wait_readable(fd, timeout_millisecond);
}

int redis_client_standalone_engine::timed_wait_writeable(long timeout_millisecond)
{
    return zcc::timed_wait_writeable(fd, timeout_millisecond);
}

#pragma pack(push,1)
typedef struct {
    unsigned char count;
    const char *vector;
    const char *info;
} redis_cmd_info;
#pragma pack(pop)

static redis_cmd_info redis_cmd_info_vector[] = 
{
    {
        .count = 4,
        .vector = "DEL" "GET" "SET" "TTL",
        .info = "bbbb"
    },
    {
        .count = 35,
        .vector = "AUTH" "DECR" "DUMP" "ECHO" "EVAL"
            "EXEC" "HDEL" "HGET" "HLEN" "HSET"
            "INCR" "INFO" "KEYS" "LLEN" "LPOP"
            "LREM" "LSET" "MGET" "MOVE" "MSET"
            "PING" "PTTL" "QUIT" "RPOP" "SADD"
            "SAVE" "SCAN" "SORT" "SPOP" "SREM"
            "SYNC" "TIME" "TYPE" "ZADD" "ZREM",
        .info = "AbbAA" "Abbbb" "bAAbb" "bbbbb" "AbAbb" "AAbbb" "AAbbb"
    },
    {
        .count = 24,
        .vector =  "BITOP" "BLPOP" "BRPOP" "DEBUG" "HKEYS"
            "HMGET" "HMSET" "HSCAN" "HVALS" "LPUSH"
            "LTRIM" "MULTI" "PSYNC" "RPUSH" "SCARD"
            "SDIFF" "SETEX" "SETNX" "SMOVE" "SSCAN"
            "WATCH" "ZCARD" "ZRANK" "ZSCAN",
        .info = "cbbAb" "bbbbb" "bAAbb" "bbbbb" "bbbb"
    },
    {
        .count = 31,
        .vector = "APPEND" "BGSAVE" "CLIENT" "CONFIG" "DBSIZE"
            "DECRBY" "EXISTS" "EXPIRE" "GETBIT" "GETSET"
            "HSETNX" "INCRBY" "LINDEX" "LPUSHX" "LRANGE"
            "MSETNX" "OBJECT" "PSETEX" "PUBSUB" "RENAME"
            "RPUSHX" "SCRIPT" "SELECT" "SETBIT" "SINTER"
            "STRLEN" "SUNION" "ZCOUNT" "ZRANGE" "ZSCORE"
            "DISCARD",

        .info = "bAAAA" "bbbbb"  "bbbbb" "bAbAb" "bABbb" "bbbbb" "B"
    },
    {
        .count = 16,
        .vector = "EVALSHA" "FLUSHDB" "HEXISTS" "HGETALL" "HINCRBY"
            "LINSERT" "MIGRATE" "MONITOR" "PERSIST" "PEXPIRE"
            "PUBLISH" "RESTORE" "SLAVEOF" "SLOWLOG" "UNWATCH"
            "ZINCRBY",
        .info = "AAbbb" "bdbbb" "AbAAB" "b"
    },
    {
        .count = 10,
        .vector = "BITCOUNT" "EXPIREAT" "FLUSHALL" "GETRANGE" "LASTSAVE"
            "RENAMENX" "SETRANGE" "SHUTDOWN" "SMEMBERS" "ZREVRANK",
        .info = "bbAbA" "bbAbb"
    },
    {
        .count = 6,
        .vector = "PEXPIREAT" "RANDOMKEY" "RPOPLPUSH" "SISMEMBER" "SUBSCRIBE"
            "ZREVRANGE",
        .info = "bAbbA" "b"
    },
    {
        .count = 3,
        .vector = "BRPOPLPUSH" "PSUBSCRIBE" "SDIFFSTORE",
        .info = "bAb"
    },
    {
        .count = 7,
        .vector = "INCRBYFLOAT" "SINTERSTORE" "SRANDMEMBER" "SUNIONSTORE" "UNSUBSCRIBE"
            "ZINTERSTORE" "ZUNIONSTORE",
        .info = "bbbbA" "bb"
    },
    {
        .count = 3,
        .vector = "BGREWRITEAOF" "HINCRBYFLOAT" "PUNSUBSCRIBE",
        .info = "AbA"
    },
    {
        .count = 1,
        .vector = "ZRANGEBYSCORE",
        .info = "b"
    },
    {
        .count = 0,
        .vector = 0,
        .info = 0
    },
    {
        .count = 1,
        .vector = "ZREMRANGEBYRANK",
        .info = "b"
    },
    {
        .count = 1,
        .vector = "ZREMRANGEBYSCORE" "ZREVRANGEBYSCORE",
        .info = "bb"
    }
};

int redis_cmd_key_station(std::string &_cmd)
{
    std::string cmd = _cmd;
    toupper(cmd);
    int clen = (int)cmd.size();
    if ((clen >16) || (clen < 3)) {
        return -1;
    }
    char *cmd_p = (char *)cmd.c_str();
    redis_cmd_info info = redis_cmd_info_vector[clen - 3];
    if (info.count == 0) {
        return -1;
    }
    int cmd_count = info.count;
    const char *cmd_vector = info.vector;
    int left = 0, right = cmd_count -1, middle;
    bool found = false;
    while (left < right) {
        middle = (left + right)/2;
        int rcmp = strncmp(cmd_p, cmd_vector + clen * middle, clen);
        if (rcmp < 0) {
            right = middle - 1;
        } else  if (rcmp > 0) {
            left = middle + 1;
        } else {
            found = true;
            break;
        }
    }
    if (found) {
        char r =info.info[middle];
        if (r =='A') {
            return -1;
        }
        if (r == 'B') {
            return -2;
        }
        return (r - 'a');
    }
    return -1;
}

}

namespace zcc
{

class redis_client::client_info
{
public:
    inline client_info() { }
    inline ~client_info() { }
    std::string msg_info;
    long timeout_milliseconds;
    bool flag_inner_engine;
};

redis_client::redis_client(const char *destination, const char *password)
{
    r_info = new client_info();
    r_info->flag_inner_engine = true;
    r_info->timeout_milliseconds = 10 * 1000;
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
    delete r_info;
}

void redis_client::set_timeout(long timeout_milliseconds)
{
    if (timeout_milliseconds > var_max_timeout) {
        timeout_milliseconds = 10 * 1000;
    } else if (timeout_milliseconds < 0) {
        timeout_milliseconds = 10 * 1000;
    }
    r_info->timeout_milliseconds = timeout_milliseconds;
}

const std::string &redis_client::get_msg()
{
    return r_info->msg_info;
}

static void ___exec_command_prepare(redis_client_query &query, const char *redis_fmt, va_list ap)
{
    char nbuf[1024];
    if (redis_fmt == 0) {
        return;
    }
    std::list<std::string> &ptokens = *query.protocol_tokens;
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
    redis_client_query query;
    std::list<std::string> protocol_tokens;
    query.protocol_tokens = &protocol_tokens;
    query.timeout_milliseconds = r_info->timeout_milliseconds;
    query.msg_info = &(r_info->msg_info);
    query.number_ret = number_ret;
    query.string_ret = string_ret;
    query.list_ret = list_ret;
    query.json_ret = json_ret;
    ___exec_command_prepare(query, redis_fmt, ap);
    return r_engine->query_protocol(query); 
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

int redis_client::scan_special(std::list<std::string> &list_ret, long &cursor_ret, const char *redis_fmt, ...)
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

int redis_client::info_special(std::map<std::string, std::string> &name_value_dict, std::string &string_ret)
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

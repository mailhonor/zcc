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

const short int var_slot_count = 16384;

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
        4,
        "DEL" "GET" "SET" "TTL",
        "bbbb"
    },
    {
        35,
        "AUTH" "DECR" "DUMP" "ECHO" "EVAL"
            "EXEC" "HDEL" "HGET" "HLEN" "HSET"
            "INCR" "INFO" "KEYS" "LLEN" "LPOP"
            "LREM" "LSET" "MGET" "MOVE" "MSET"
            "PING" "PTTL" "QUIT" "RPOP" "SADD"
            "SAVE" "SCAN" "SORT" "SPOP" "SREM"
            "SYNC" "TIME" "TYPE" "ZADD" "ZREM",
        "AbbAA" "Abbbb" "bAAbb" "bbbbb" "AbAbb" "AAbbb" "AAbbb"
    },
    {
        24,
         "BITOP" "BLPOP" "BRPOP" "DEBUG" "HKEYS"
            "HMGET" "HMSET" "HSCAN" "HVALS" "LPUSH"
            "LTRIM" "MULTI" "PSYNC" "RPUSH" "SCARD"
            "SDIFF" "SETEX" "SETNX" "SMOVE" "SSCAN"
            "WATCH" "ZCARD" "ZRANK" "ZSCAN",
        "cbbAb" "bbbbb" "bAAbb" "bbbbb" "bbbb"
    },
    {
        31,
        "APPEND" "BGSAVE" "CLIENT" "CONFIG" "DBSIZE"
            "DECRBY" "EXISTS" "EXPIRE" "GETBIT" "GETSET"
            "HSETNX" "INCRBY" "LINDEX" "LPUSHX" "LRANGE"
            "MSETNX" "OBJECT" "PSETEX" "PUBSUB" "RENAME"
            "RPUSHX" "SCRIPT" "SELECT" "SETBIT" "SINTER"
            "STRLEN" "SUNION" "ZCOUNT" "ZRANGE" "ZSCORE"
            "DISCARD",

        "bAAAA" "bbbbb"  "bbbbb" "bAbAb" "bABbb" "bbbbb" "B"
    },
    {
        16,
        "EVALSHA" "FLUSHDB" "HEXISTS" "HGETALL" "HINCRBY"
            "LINSERT" "MIGRATE" "MONITOR" "PERSIST" "PEXPIRE"
            "PUBLISH" "RESTORE" "SLAVEOF" "SLOWLOG" "UNWATCH"
            "ZINCRBY",
        "AAbbb" "bdbbb" "AbAAB" "b"
    },
    {
        10,
        "BITCOUNT" "EXPIREAT" "FLUSHALL" "GETRANGE" "LASTSAVE"
            "RENAMENX" "SETRANGE" "SHUTDOWN" "SMEMBERS" "ZREVRANK",
        "bbAbA" "bbAbb"
    },
    {
        6,
        "PEXPIREAT" "RANDOMKEY" "RPOPLPUSH" "SISMEMBER" "SUBSCRIBE"
            "ZREVRANGE",
        "bAbbA" "b"
    },
    {
        3,
        "BRPOPLPUSH" "PSUBSCRIBE" "SDIFFSTORE",
        "bAb"
    },
    {
        7,
        "INCRBYFLOAT" "SINTERSTORE" "SRANDMEMBER" "SUNIONSTORE" "UNSUBSCRIBE"
            "ZINTERSTORE" "ZUNIONSTORE",
        "bbbbA" "bb"
    },
    {
        3,
        "BGREWRITEAOF" "HINCRBYFLOAT" "PUNSUBSCRIBE",
        "AbA"
    },
    {
        1,
        "ZRANGEBYSCORE",
        "b"
    },
    {
        0,
        0,
        0
    },
    {
        1,
        "ZREMRANGEBYRANK",
        "b"
    },
    {
        1,
        "ZREMRANGEBYSCORE" "ZREVRANGEBYSCORE",
        "bb"
    }
};

static int redis_cmd_key_station(std::string &_cmd)
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
    while (left <= right) {
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
            return -2;
        }
        if (r == 'B') {
            return -3;
        }
        return (r - 'a');
    }
    return -1;
}

typedef struct cluster_node_t cluster_node_t;
struct cluster_node_t {
    short int used;
    int fd;
    int ip;
    int port;
};

redis_client_cluster_engine::redis_client_cluster_engine()
{
    r_slot_node = 0;
    r_slot_node_size = 0;
    r_slot_node_used = 0;
    r_slot_pair = 0;
    r_fd = -1;
}

redis_client_cluster_engine::redis_client_cluster_engine(const char *destination, const char *password)
{
    r_slot_node = 0;
    r_slot_node_size = 0;
    r_slot_node_used = 0;
    r_slot_pair = 0;
    r_fd = -1;
    open(destination, password);
}

redis_client_cluster_engine::~redis_client_cluster_engine()
{
    close();
    if (r_slot_node) {
        free(r_slot_node);
        free(r_slot_pair);
    }
}

int redis_client_cluster_engine::query_protocol_try(long *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, std::list<std::string> &tokens, long timeout, std::string &info_msg, int slot, char *ipbuf, int port)
{
    cluster_node_t *cnode = 0;
    int fd = -1;
    if ((slot < 0) || (slot > var_slot_count)) {
        return -2;
    }
    if (r_slot_pair[slot] == -1) {
        if (port == -1) {
            if (r_slot_node_used > 0) {
                cnode = r_slot_node + (r_slot_node_last ++)%(r_slot_node_used);
            }
        } else {
            bool found = false;
            int ip = get_ipint(ipbuf);
            for (int i = 0; i < r_slot_node_used; i++) {
                cnode = r_slot_node + i;
                if (cnode->ip == ip && cnode->port == port) {
                    found = true;
                    break;
                }
            }
            if (found) {
                r_slot_pair[slot] = cnode - r_slot_node;
                cnode->used++;
            } else {
                r_slot_pair[slot] = r_slot_node_used;
                if (r_slot_node_used == r_slot_node_size) {
                    r_slot_node_size *= 2;
                    r_slot_node = (cluster_node_t *)realloc(r_slot_node, sizeof(cluster_node_t) * r_slot_node_size);
                    for (int i = r_slot_node_used; i < r_slot_node_size; i++) {
                        r_slot_node[i].used  = 0;
                        r_slot_node[i].fd  = -1;
                    }
                }
                r_slot_node[r_slot_node_used].used = 1;
                r_slot_node[r_slot_node_used].ip = ip;
                r_slot_node[r_slot_node_used].port = port;
                r_slot_node_used ++;
            }
        }
    } else {
        cnode = r_slot_node + r_slot_pair[slot];
        if (port != -1) {
            int ip = get_ipint(ipbuf);
            if ((cnode->ip != ip) || (cnode->port != port)) {
                return -2;
            }
        }
    }
    if (cnode) {
        if (cnode->fd == -1) {
            std::string host;
            get_ipstring(cnode->ip, host);
            cnode->fd = inet_connect(host.c_str(), cnode->port);
        }
        fd = cnode->fd;
    } else {
        if (r_fd == -1) {
            open();
        }
        fd = r_fd;
    }
    if (fd == -1) {
        return -2;
    }
    stream fp(fd);
    return  query_protocol_io(number_ret, string_ret, list_ret, json_ret, tokens, timeout, info_msg, fp);
}

int redis_client_cluster_engine::query_protocol(long *r_number_ret, std::string *r_string_ret, std::list<std::string> *r_list_ret, json *r_json_ret, std::list<std::string> &ptokens, long timeout, std::string &r_msg)
{
    r_msg.clear();
    if (!r_slot_node) {
        r_msg = "need to do redis_client.open()";
        return -1;
    }
    std::string cmd = ptokens.front();
    int kstation = redis_cmd_key_station(cmd);
    if (kstation == -1) {
        r_msg = "unknown command '";
        r_msg += cmd +"'";
        return -1;
    }
    if (kstation < 0) {
        r_msg = "the cluster client deny to execute the cmd";
        return -1;
    }
    if ((int)ptokens.size() > kstation + 1) {
        r_msg = "-ERR wrong number of arguments for '";
        r_msg.append(cmd).append("'");
        return -1;
    }

    auto pit = ptokens.begin();
    for (int i=0;i<kstation;i++) {
        pit++;
    }
    std::string &mainkey = *pit;
    int slot_val = get_crc16_result(mainkey.c_str(), mainkey.size())%var_slot_count;
    char ipbuf[18];
    int port;
    port = -1;
    for (int retry = 0; retry < 10; retry++) {
        int ret = query_protocol_try(r_number_ret, r_string_ret, r_list_ret, r_json_ret, ptokens, timeout, r_msg, slot_val, ipbuf, port);
        if (ret < -1) {
            close();
            return -1;
        } else if (ret > -1) {
            if (r_slot_node_used > 1) {
                if (r_fd != -1) {
                    ::close(r_fd);
                    r_fd = -1;
                }
            }
            return ret;
        }
        char *p, *ps = const_cast<char *>(r_msg.c_str());
        if (ps[0] != '-') {
            return -1;
        }
        ps++;
        if ((*ps == 'M') &&(!strncmp(ps, "MOVED ", 6))) {
            ps += 6;
            p = strchr(ps, ' ');
            if (!p) {
                return -1;
            }
            *p = 0;
            slot_val = atoi(ps);
            *p = ' ';
            ps = p + 1;
            p = strchr(ps, ':');
            if (!p) {
                return -1;
            }
            if ((p-ps < 7) || (p-ps > 16)) {
                return -1;
            }
            memcpy(ipbuf, ps, p-ps);
            ipbuf[p-ps] = 0;
            port = atoi(p+1);
            continue;
        } else if ((*ps == 'C') &&(!strncmp(ps, "CLUSTERDOWN ", 12))) {
            close();
            return -1;
        } else if ((*ps == 'A') &&(!strncmp(ps, "ASK ", 4))) {
            close();
            return -1;
        }
        return -1;
    }

    return -1;
}

void redis_client_cluster_engine::open(const char *destination, const char *password)
{
    close();
    r_destination = (destination?destination:"");
    r_password = (password?password:"");
    if (!r_slot_node) {
        r_slot_node_size = 32;
        r_slot_node_used = 0;
        r_slot_node = (cluster_node_t *)malloc(sizeof(cluster_node_t) * r_slot_node_size);
        r_slot_pair = (short int *)malloc((sizeof(short int) * var_slot_count));
        for (int i=0; i < var_slot_count; i++) {
            r_slot_pair[i] = -1;
        }
        for (int i=0; i < r_slot_node_size; i++) {
            r_slot_node[i].used  = 0;
            r_slot_node[i].fd  = -1;
        }
    }
}

void redis_client_cluster_engine::open()
{
    std::list<std::string> netpath_list;
    netpaths_expand(r_destination.c_str(), netpath_list);
    std_list_walk_begin(netpath_list, np) {
        while (r_fd == -1) {
            char *npp = (char *)(np.c_str());
            char *p = strchr(npp, ':');
            if (p) {
                *p++ = 0;
                r_fd = inet_connect(npp, atoi(p));
            } else {
                r_fd = unix_connect(npp);
            }
            if (r_fd == -1) {
                break;
            }
            break;
        }
    } std_list_walk_end;
}

void redis_client_cluster_engine::close()
{
    if (r_slot_node) {
        for (int i=0; i < var_slot_count; i++) {
            r_slot_pair[i] = -1;
        }
        for (int i=0; i < r_slot_node_size; i++) {
            if ((r_slot_node[i].used) && (r_slot_node[i].fd!=-1)) {
                ::close(r_slot_node[i].fd);
            }
            r_slot_node[i].used  = 0;
            r_slot_node[i].fd  = -1;
        }
    }
    r_slot_node_used = 0;
    if (r_fd != -1) {
        ::close(r_fd);
        r_fd = -1;
    }
}

}

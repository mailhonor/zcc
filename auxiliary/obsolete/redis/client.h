/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-13
 * ================================
 */


/* redis ##################################################### */
#define _nine_token_s const char *n2 = ((char *)(-9)), const char *n3 = ((char *)(-9)), const char *n4 = ((char *)(-9)), const char *n5 = ((char *)(-9)), const char *n6 = ((char *)(-9)), const char *n7 = ((char *)(-9)), const char *n8 = ((char *)(-9)), const char *n9 = ((char *)(-9)), const char *n10 = ((char *)(-9))
#define _nine_token2_s const char *n2 = ((char *)(-9)), const char *v2 = ((char *)(-9)), const char *n3 = ((char *)(-9)), const char *v3 = ((char *)(-9)), const char *n4 = ((char *)(-9)), const char *v4 = ((char *)(-9)), const char *n5 = ((char *)(-9)), const char *v5 = ((char *)(-9)),const char *n6 = ((char *)(-9)), const char *v6 = ((char *)(-9)), const char *n7 = ((char *)(-9)), const char *v7 = ((char *)(-9)), const char *n8 = ((char *)(-9)), const char *v8 = ((char *)(-9)), const char *n9 = ((char *)(-9)), const char *v9 = ((char *)(-9)), const char *n10 = ((char *)(-9)), const char *v10 = ((char *)(-9))
#define _nine_token_S const std::string &n2 = var_std_string_ignore, const std::string &n3 = var_std_string_ignore, const std::string &n4 = var_std_string_ignore, const std::string &n5 = var_std_string_ignore, const std::string &n6 = var_std_string_ignore, const std::string &n7 = var_std_string_ignore, const std::string &n8 = var_std_string_ignore, const std::string &n9 = var_std_string_ignore, const std::string &n10 = var_std_string_ignore
#define _nine_token2_S const std::string &n2 = var_std_string_ignore, const std::string &v2 = var_std_string_ignore, const std::string &n3 = var_std_string_ignore, const std::string &v3 = var_std_string_ignore, const std::string &n4 = var_std_string_ignore, const std::string &v4 = var_std_string_ignore, const std::string &n5 = var_std_string_ignore, const std::string &v5 = var_std_string_ignore,const std::string &n6 = var_std_string_ignore, const std::string &v6 = var_std_string_ignore, const std::string &n7 = var_std_string_ignore, const std::string &v7 = var_std_string_ignore, const std::string &n8 = var_std_string_ignore, const std::string &v8 = var_std_string_ignore, const std::string &n9 = var_std_string_ignore, const std::string &v9 = var_std_string_ignore, const std::string &n10 = var_std_string_ignore, const std::string &v10 = var_std_string_ignore

class redis_client_query
{
public:
    redis_client_query(const char *_cmd);
    ~redis_client_query();
    long *number_ret;
    std::string *string_ret;
    std::list<std::string> *list_ret;
    json *json_ret;
    long timeout_millisecond;
    const char *req_data;
    unsigned int req_len;
    std::string *msg_info;
    std::string cmd; 
    std::string key; 
    bool flag_write;
};

class redis_client_basic_engine
{
public:
    inline redis_client_basic_engine() {}
    virtual inline ~redis_client_basic_engine() {}
    int query_protocol_io(redis_client_query &query, stream &tmp_stream);
    virtual int query_protocol(redis_client_query &query);
    virtual int timed_wait_readable(long timeout);
};

class redis_client
{
public:
    class client_info;
    redis_client();
    redis_client(const char *destination, const char *password = 0);
    redis_client(redis_client_basic_engine &engine);
    ~redis_client();
    const std::string &get_msg();

    /* CONNECTION */
    int auth(const char *password);
    int auth(const std::string &password);
    int echo(const char *str);
    int echo(const std::string &str);
    int ping();
    int select(long idx);

    /* SCRIPT */
    int eval();
    
    /* TRANSACTION */
    int discard();
    int exec();
    int multi();
    int unwatch();
    int watch(const char *key1, _nine_token_s);
    int watch(const std::string &key1, _nine_token_S);

    /* SERVER */
    int bgrewriteaof();
    int dbsize();
    int bgsave();
    int client_getname(std::string &result);
    int client_kill(const char *ip_port);
    int client_kill(const std::string &ip_port);
    int client_list(std::string &result);
    int client_setname(const char *name);
    int client_setname(const std::string &name);
    int config_get(std::list<std::string> &result, const char *parameter_pattern);
    int config_get(std::list<std::string> &result, const std::string &parameter_pattern);
    int config_resetstat();
    int config_rewrite();
    int config_set(const char *parameter, const char *value);
    int config_set(const std::string &parameter, const std::string &value);
    int db_size();
    int FLUSHALL();
    int FLUSHDB();
    int info(std::string &result);
    int info(std::map<std::string, std::string> &name_value_dict, std::string &result);
    int lastsave(long &result);
    int monitor();
    int psync();
    int save();
    int SHUTDOWN();
    int SHUTDOWN_SAVE();
    int SHUTDOWN_NOSAVE();
    int slaveof(const char *host, long port = 0);
    int slaveof(const std::string &host, long port = 0);
    int slaveof_no_one();
    int slowlog();
    int sync();
    int time(long &second, long &microsecond);

    /* KEY */
    int del(const char *key, _nine_token_s);
    int del(const std::string &key1, _nine_token_S);
    int del(const std::list<std::string> &keys);
    int dump(std::string &result, const char *key);
    int dump(std::string &result, const std::string &key);
    int exists(const char *key);
    int exists(const std::string &key);
    int expire(const char *key, long timeout_second);
    int expire(const std::string &key, long timeout_second);
    int expireat(const char *key, long timeout_at_second);
    int expireat(const std::string &key, long timeout_at_second);
    int keys(std::list<std::string> &val, const char *pattern);
    int keys(std::list<std::string> &val, const std::string &pattern);
    int migrate(const char *host, long port, const char *key, long dest_db, long timeout_ms, bool copy = false, bool replace = false);
    int migrate(const std::string &host, long port, const std::string &key, long dest_db, long timeout_ms, bool copy = false, bool replace = false);

    int move(const char *key, long dest_db_id);
    int move(const std::string &key, long dest_db_id);
    int object_refcount(long &refcount, const char *key);
    int object_refcount(long &refcount, const std::string &key);
    int object_idletime(long &idletime, const char *key);
    int object_idletime(long &idletime, const std::string &key);
    int object_encoding(std::string &encoding, const char *key);
    int object_encoding(std::string &encoding, const std::string &key);
    int persist(const char *key);
    int persist(const std::string &key);
    int pexpire(const char *key, long timeout_ms);
    int pexpire(const std::string &key, long timeout_ms);
    int pexpireat(const char *key, long timeout_at_ms);
    int pexpireat(const std::string &key, long timeout_at_ms);
    /* @pptl, 返回值 1
     * left_ms,毫秒,{ -2:key不存在, -1:没设置超时, >0: 剩余时间 }
     **/
    int pttl(long &left_ms, const char *key);
    int pttl(long &left_ms, const std::string &key);
    int randomkey(std::string &val);
    int rename(const char *key, const char *newkey);
    int rename(const std::string &key, const std::string &newkey);
    /* @renamenx, 返回值 1: 成功, 0: 失败, newkey已经存在 */
    int renamenx(const char *key, const char *newkey);
    int renamenx(const std::string &key, const std::string &newkey);
    int restore(const char *key, long timeout_ms, const char *data);
    int restore(const std::string &key, long timeout_ms, const std::string &data);
    int sort(std::list<std::string> &val, const char *key, const char *by, long offset, long count, const std::list<std::string> *get, bool desc, bool alpha);
    int sort(long &saved_count, const char *key, const char *by, long offset, long count, const std::list<std::string> *get, bool desc, bool alpha, const char *store);
    /* @ttl, 参考 pttl, left_second 单位:秒 */
    int ttl(long &left_second, const char *key);
    int ttl(long &left_second, const std::string &key);
    int type(std::string &key_type, const char *key);
    int type(std::string &key_type, const std::string &key);
    int scan(std::list<std::string> &result, long &cursor, const char *pattern = 0, long count = -1);
    int scan(std::list<std::string> &result, long &cursor, const std::string &pattern = var_std_string_ignore, long count = -1);

    /* STRING */
    int append(const char *key, const char *val);
    int append(const std::string &key, const std::string &val);
    int bitcount(const char *key, long start = 999999999L, long end = 999999999L);
    int bitcount(const std::string &key, long start = 999999999L, long end = 999999999L);
    int bitop(const std::string &op, const std::string &destkey, const std::list<std::string> &keys);
    int bitop(const char *op, const char *destkey, const char *key1, _nine_token_s);
    int bitop(const std::string &op, const std::string &destkey, const std::string &key1, _nine_token_S);
    int bitop_and(const std::string &destkey, const std::list<std::string> &keys);
    int bitop_and(const char *destkey, const char *key1, _nine_token_s);
    int bitop_and(const std::string &destkey, const std::string &key1, _nine_token_S);
    int bitop_or (const std::string &destkey, const std::list<std::string> &keys);
    int bitop_or (const char *destkey, const char *key1, _nine_token_s);
    int bitop_or (const std::string &destkey, const std::string &key1, _nine_token_S);
    int bitop_xor(const std::string &destkey, const std::list<std::string> &keys);
    int bitop_xor(const char *destkey, const char *key1, _nine_token_s);
    int bitop_xor(const std::string &destkey, const std::string &key1, _nine_token_S);
    int bitop_not(const char *destkey, const char *key);
    int bitop_not(const std::string &destkey, const std::string &key);
    int decr(long &result, const char *key);
    int decr(long &result, const std::string &key);
    int decrby(long &result, const char *key, long decrement);
    int decrby(long &result, const std::string &key, long decrement);
    int get(std::string &val_result, const char *key);
    int get(std::string &val_result, const std::string &key);
    int getbit(long &result, const char *key, long offset);
    int getbit(long &result, const std::string &key, long offset);
    int getrange(std::string &val, const char *key, long start, long end);
    int getrange(std::string &val, const std::string &key, long start, long end);
    int getset(std::string &oldval, const char *key, const char *val);
    int getset(std::string &oldval, const std::string &key, const std::string &val);
    int incr(long &val, const char *key);
    int incr(long &val, const std::string &key);
    int incrby(long &val, const char *key, long increment);
    int incrby(long &val, const std::string &key, long increment);
    int incrbyfloat(double &val, const char *key, double increment);
    int incrbyfloat(double &val, const std::string &key, double increment);
    int incrbyfloat(std::string &val, const std::string &key, const std::string &increment);
    int mget(std::list<std::string> &val, const std::list<std::string> &keys);
    int mget(std::list<std::string> &val, const char *key1, _nine_token_s);
    int mget(std::list<std::string> &val, const std::string &key1, _nine_token_S);
    int mset(const std::list<std::string> &key_val_list);
    int mset(const char *key1, const char *value1, _nine_token2_s);
    int mset(const std::string &key1, const std::string &value1, _nine_token2_S);
    int msetnx(const std::list<std::string> &key_val_list);
    int msetnx(const char *key1, const char *value1, _nine_token2_s);
    int msetnx(const std::string &key1, const std::string &value1, _nine_token2_S);
    int psetex(const char *key, long milliseconds, const char *val);
    int psetex(const std::string &key, long milliseconds, const std::string &val);
    int set(const char *key, const char *val, long EX = 0, long PX = 0, bool NX = false, bool XX = false);
    int set(const std::string &key, const std::string &val, long EX = 0, long PX = 0, bool NX = false, bool XX = false);
    int setbit(const char *key, long offset, long value);
    int setbit(const std::string &key, long offset, long value);
    int setex(const char *key, long seconds, const char *val);
    int setex(const std::string &key, long seconds, const std::string &val);
    int setnx(const char *key, const char *val);
    int setnx(const std::string &key, const std::string &val); 
    int setrange(const char *key, long offset, const char *val);
    int setrange(const std::string &key, long offset, const std::string &val);
    int strlen(const char *key);
    int strlen(const std::string &key);

    /* HASH */
    int hdel(const char *key, const char *field1, _nine_token_s);
    int hdel(const std::string &key, const std::string &field1, _nine_token_S);
    int hdel(const std::string &key, const std::list<std::string> &field_list);
    int hexists(const char *key, const char *field);
    int hexists(const std::string &key, const std::string &field);
    int hget(std::string &val, const char *key, const char *field);
    int hget(std::string &val, const std::string &key, const std::string &field);
    int hgetall(std::list<std::string> &field_value_list, const char *key);
    int hgetall(std::list<std::string> &field_value_list, const std::string &key);
    int hincrby(long &result, const char *key, const char *field, long increment);
    int hincrby(long &result, const std::string &key, const std::string &field, long increment);
    int hincrbyfloat(double &val, const char *key, const char *field, double increment);
    int hincrbyfloat(double &val, std::string &key, std::string &field, double increment);
    int hincrbyfloat(std::string &val, const std::string &key, const std::string &field, const std::string &increment);
    int hkeys(std::list<std::string> &field_list, const char *key);
    int hkeys(std::list<std::string> &field_list, const std::string &key);
    int hlen(const char *key);
    int hlen(const std::string &key);
    int hmget(std::list<std::string> &val_list, const std::string &key, const std::list<std::string> &field_list);
    int hmget(std::list<std::string> &vallist, const char *key, const char *field1, _nine_token_s);
    int hmget(std::list<std::string> &val_list, const std::string &key, const std::string &field1, _nine_token_S);
    int hmset(const std::string &key, const std::list<std::string> &field_val_list);
    int hmset(const char *key, const char *field1, const char *val1, _nine_token2_s);
    int hmset(const std::string &key, const std::string &field1, const std::string &val1, _nine_token2_S);
    int hmsetnx(const std::string &key, const std::list<std::string> &field_val_list);
    int hmsetnx(const char *key, const char *field1, const char *val1, _nine_token2_s);
    int hmsetnx(const std::string &key, const std::string &field1, const std::string &val1, _nine_token2_S);
    int hvals(std::list<std::string> &val_list, const char *key);
    int hvals(std::list<std::string> &val_list, const std::string &key);
    int hscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern = 0, long count = -1);
    int hscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern = var_std_string_ignore, long count = -1);

    /* LIST */
    int blpop(std::string &key_pop, std::string &val_pop, long timeout, const char *key1, _nine_token_s);
    int blpop(std::string &key_pop, std::string &val_pop, long timeout, const std::string &key1, _nine_token_S);
    int blpop(std::string &key_pop, std::string &val_pop, long timeout, const std::list<std::string> &keys);
    int brpop(std::string &key_pop, std::string &val_pop, long timeout, const char *key1, _nine_token_s);
    int brpop(std::string &key_pop, std::string &val_pop, long timeout, const std::string &key1, _nine_token_S);
    int brpop(std::string &key_pop, std::string &val_pop, long timeout, const std::list<std::string> &keys);
    int brpoplpush(std::string &val_pop, const char *source_key, const char *dest_key, long timeout);
    int brpoplpush(std::string &val_pop, const std::string &source_key, const std::string &dest_key, long timeout);
    int lindex(std::string &result, const char *key, long idx);
    int lindex(std::string &result, const std::string &key, long idx);
    int linsert(long &result, const char *key, bool before, const char *pivot, const char *value);
    int linsert(long &result, const std::string &key, bool before, const std::string &pivot, const std::string &value);
    int llen(const char *key);
    int llen(const std::string &key);
    int lpop(std::string &val_pop, const char *key);
    int lpop(std::string &val_pop, const std::string &key);
    int lpush(const char *key, const char *value1, _nine_token_s);
    int lpush(const std::string &key, const std::string &value1, _nine_token_S);
    int lpush(const std::string &key, const std::list<std::string> &val_list);
    int lpushx(const char *key, const char *value);
    int lpushx(const std::string &key, const std::string &value);
    int lrange(std::list<std::string> &val_list, const char *key, long start, long end);
    int lrange(std::list<std::string> &val_list, const std::string &key, long start, long end);
    int lrem(const char *key, long count, const char *value);
    int lrem(const std::string &key, long count, const std::string &value);
    int lrem_all(const char *key, const char *value);
    int lrem_all(const std::string &key, const std::string &value);
    int lset(const char *key, long index, const char *value);
    int lset(const std::string &key, long index, const std::string &value);
    int ltrim(const char *key, long start, long end);
    int ltrim(const std::string &key, long start, long end);
    int rpop(std::string &val_pop, const char *key);
    int rpop(std::string &val_pop, const std::string &key);
    int rpoplpush(std::string &val_pop, const char *source_key, const char *dest_key);
    int rpoplpush(std::string &val_pop, const std::string &source_key, const std::string &dest_key);
    int rpush(const char *key, const char *value1, _nine_token_s);
    int rpush(const std::string &key, const std::string &value1, _nine_token_S);
    int rpush(const std::string &key, const std::list<std::string> &val_list);
    int rpushx(const char *key, const char *value);
    int rpushx(const std::string &key, const std::string &value);

    /* SET */
    int sadd(const char *key, const char *member1, _nine_token_s);
    int sadd(const std::string &key, const std::string &member1, _nine_token_S);
    int sadd(const std::string &key, const std::list<std::string> &val_list);
    int scard(const char *key);
    int scard(const std::string &key);
    int sdiff(std::list<std::string> &members_diff, const char *key1, _nine_token_s);
    int sdiff(std::list<std::string> &members_diff, const std::string &key1, _nine_token_S);
    int sdiff(std::list<std::string> &members_diff, const std::list<std::string> &key_list);
    int sdiffstore(const char *dest_key, const char *key1, _nine_token_s);
    int sdiffstore(const std::string &dest_key, const std::string &key1, _nine_token_S);
    int sdiffstore(const std::string &dest_key, const std::list<std::string> &key_list);
    int sinter(std::list<std::string> &members_inter, const char *key1, _nine_token_s);
    int sinter(std::list<std::string> &members_inter, const std::string &key1, _nine_token_S);
    int sinter(std::list<std::string> &members_inter, const std::list<std::string> &key_list);
    int sinterstore(const char *dest_key, const char *key1, _nine_token_s);
    int sinterstore(const std::string &dest_key, const std::string &key1, _nine_token_S);
    int sinterstore(const std::string &dest_key, const std::list<std::string> &key_list);
    int sismember(const char *key, const char *value);
    int sismember(const std::string &key, const std::string &value);
    int smove(const char *source_key, const char *dest_key, const char *value);
    int smove(const std::string &source_key, const std::string &dest_key, const std::string &value);
    int spop(std::string &val_pop, const char *key);
    int spop(std::string &val_pop, const std::string &key);
    int srandmember(std::string &result, const char *key);
    int srandmember(std::string &result, const std::string &key);
    int srandmember(std::list<std::string> &result, const char *key, long count);
    int srandmember(std::list<std::string> &result, const std::string &key, long count);
    int srem(const char *key, const char *member1, _nine_token_s);
    int srem(const std::string &key, const std::string &member1, _nine_token_S);
    int srem(const std::string &key, const std::list<std::string> &val_list);
    int sunion(std::list<std::string> &val, const std::list<std::string> &keys);
    int sunion(std::list<std::string> &val, const char *key1, _nine_token_s);
    int sunion(std::list<std::string> &val, const std::string &key1, _nine_token_S);
    int sscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern = 0, long count = -1);
    int sscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern = var_std_string_ignore, long count = -1);

    /* ZSET */
    int zadd(const std::string &key, const std::list<std::string> &score_member_list);
    int zadd(const char *key, double score, const char *member);
    int zadd(const std::string &key, double score, const std::string &member);
    int zadd(const std::string &key, const std::string &score, const std::string &member);
    int zcard(const char *key);
    int zcard(const std::string &key);
    int zcount(const char *key, double min, double max);
    int zcount(const std::string &key, double min, double max);
    int zcount(const std::string &key, std::string &min, std::string &max);
    int zincrby(double &val, const char *key, double increment, const char *member);
    int zincrby(double &val, std::string &key, double increment, std::string &member);
    int zincrby(std::string &val, const std::string &key, const std::string &increment, const std::string &field);
    int zrange(std::list<std::string> &result, const char *key, long start, long end, bool withscores = false);
    int zrange(std::list<std::string> &result, const std::string &key, long start, long end, bool withscores = false);

    int zrangebyscore(std::list<std::string> &result, const char *key, double min, double max, bool withscores = false, long offset = 0, long count = 0);
    int zrangebyscore(std::list<std::string> &result, const std::string &key, double min, double max, bool withscores = false, long offset = 0, long count = 0);
    int zrangebyscore(std::list<std::string> &result, const std::string &key, const std::string &min, const std::string &max, bool withscores = false, long offset = 0, long count = 0);
    int zrank(const char *key, const char *member);
    int zrank(std::string &key, std::string &member);
    int zrem(const char *key, const char *member1, _nine_token_s);
    int zrem(const std::string &key, const std::string &member1, _nine_token_S);
    int zrem(const std::string &key, const std::list<std::string> &val_list);
    int zremrangebyrank(const char *key, long start, long end);
    int zremrangebyrank(const std::string &key, long start, long end);
    int zremrangebyscore(const char *key, double min, double max);
    int zremrangebyscore(const std::string &key, double min, double max);
    int zremrangebyscore(const std::string &key, const std::string &min, const std::string &max);
    int zrevrange(std::list<std::string> &result, const char *key, long start, long end, bool withscores = false);
    int zrevrange(std::list<std::string> &result, const std::string &key, long start, long end, bool withscores = false);
    int zrevrangebyscore(std::list<std::string> &result, const char *key, double min, double max, bool withscores = false, long offset = 0, long count = 0);
    int zrevrangebyscore(std::list<std::string> &result, const std::string &key, double min, double max, bool withscores = false, long offset = 0, long count = 0);
    int zrevrangebyscore(std::list<std::string> &result, const std::string &key, const std::string &min, const std::string &max, bool withscores = false, long offset = 0, long count = 0);
    int zrevrank(const char *key, const char *member);
    int zrevrank(std::string &key, std::string &member);
    int zscore(const char *key, const char *member);
    int zscore(std::string &key, std::string &member);
    int zunionstore(const char *destination, const char *key1, _nine_token_s);
    int zunionstore(const std::string &destination, const std::string &key1, _nine_token_S);
    int zunionstore(const std::string &destination, const std::list<std::string> &keys, const std::list<std::string> *weights = 0, const char *aggregate = 0);
    int zinterstore(const char *destination, const char *key1, _nine_token_s);
    int zinterstore(const std::string &destination, const std::string &key1, _nine_token_S);
    int zinterstore(const std::string &destination, const std::list<std::string> &keys, const std::list<std::string> *weights = 0, const char *aggregate = 0);
    int zscan(std::list<std::string> &result, const char *key, long &cursor, const char *pattern = 0, long count = -1);
    int zscan(std::list<std::string> &result, const std::string &key, long &cursor, const std::string &pattern = var_std_string_ignore, long count = -1);

    /* PUB/SUB */
    int subscribe_get_message(std::string &channel, std::string &message, long timeout_ms);
    int psubscribe(const char *pattern, _nine_token_s);
    int psubscribe(const std::string &pattern, _nine_token_S);
    int psubscribe(const std::list<std::string> &pattern_list);
    int publish(const char *channel, const char *message);
    int publish(const std::string &channel, const std::string &message);
    int pubsub_channels(std::list<std::string> &result, const char *pattern = 0);
    int pubsub_channels(std::list<std::string> &result, std::string &pattern = var_std_string_ignore);
    int pubsub_numsub(std::list<std::string> &result, const char *pattern, _nine_token_s);
    int pubsub_numsub(std::list<std::string> &result, const std::string &pattern, _nine_token_S);
    int pubsub_numsub(std::list<std::string> &result, const std::list<std::string> &pattern_list);
    int pubsub_numpat();
    int punsubscribe(const char *pattern, _nine_token_s);
    int punsubscribe(const std::string &pattern, _nine_token_S);
    int punsubscribe(const std::list<std::string> &pattern_list);
    int subscribe(const char *channel, _nine_token_s);
    int subscribe(const std::string &channel, _nine_token_S);
    int subscribe(const std::list<std::string> &channel_list);
    int unsubscribe(const char *channel, _nine_token_s);
    int unsubscribe(const std::string &channel, _nine_token_S);
    int unsubscribe(const std::list<std::string> &channel_list);
private:
    int query_command(redis_client_query &query, const char *redis_fmt, ...);
    int _scan(const char *cmd, std::list<std::string> &result, const char *key, size_t klen, long &cursor, const char *pattern, size_t plen, long count);
    int _blpop(std::string &key_pop, std::string &val_pop, const char *cmd, long timeout, const char *redis_fmt, ...);
    redis_client_basic_engine *r_engine;
    client_info *r_info;
};

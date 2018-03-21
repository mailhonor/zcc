/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

static long nval;
static std::string sval, sval2;
static std::list<std::string> lval;
static zcc::redis_client rc;

static void _test_test(zcc::redis_client &rc, const char *cmd, int cmd_ret, size_t line, int test_type)
{
    printf("%-36s: ", " ");
    if (cmd_ret == -1) {
        printf("%s [%zd]", rc.get_msg().c_str(), line);
    } else if (test_type == 'r') {
        if (cmd_ret == 0) {
            printf("none/no/not");
        } else {
            printf("exists/yes/ok/count");
        }
    } else if(test_type == 'n') {
        printf("count/number");
    } else if(test_type == 's') {
        if (cmd_ret == 0) {
            printf("none");
        } else {
            printf("%s", sval.c_str());
        }
    } else if(test_type == 'l') {
        if (cmd_ret == 0){
            printf("none");
        } else {
            std_list_walk_begin(lval, v) {
                printf("%s, ", v.c_str());
            } std_list_walk_end;
        }
    }
    printf("\r%-30s %5d: \n", cmd, cmd_ret); fflush(stdout);
    nval = -1000;
    sval.clear();
    sval2.clear();
    lval.clear();
}

#define _test_return(cmd, command)  _test_test(rc, cmd, command, __LINE__, 'r')
#define _test_number(cmd, command)  _test_test(rc, cmd, command, __LINE__, 'n')
#define _test_string(cmd, command)  _test_test(rc, cmd, command, __LINE__, 's')
#define _test___list(cmd, command)  _test_test(rc, cmd, command, __LINE__, 'l')

static void test_connection()
{
    _test_return("PING", rc.ping());
    _test_return("ECHO somestring", rc.echo("somestring"));
}

static void test_key()
{
    _test_string("GET k1", rc.get(sval, "k1"));
    _test_string("DUMP k1", rc.dump(sval, "k1"));
    _test_return("EXISTS k1", rc.exists("k1"));
    _test_return("EXPIRE abc 10", rc.expire("abc", 10));
    _test_return("EXPIREAT aaaabc ***", rc.expireat("aaaabc", time(0) + 100));
    _test___list("KEYS a*", rc.keys(lval, "a*"));

    /* migrate missing */
    _test_return("MOVE abc", rc.move("abc", 1));
    _test_string("OBJECT ENCODING abc", rc.object_encoding(sval, "abc"));
    _test_number("PTTL abc", rc.pttl(nval, "abc"));
    _test_string("RANDOMKEY", rc.randomkey(sval));
    _test_return("RENAME abc newabc", rc.rename("abc", "newabc"));
    _test_return("RENAMENX abc newabc", rc.renamenx("abc", "newabc"));
    _test___list("SORT set", rc.sort(lval, "set", 0, 0, 0, 0, false, true));
    _test_string("TYPE abc", rc.type(sval, "abc"));

    _test_return("APPEND abc", rc.append("abc", "abcdefghijklmnopqrstuvwxyz "));
    _test_return("DEL ___abc", rc.del("___abc"));
    _test_return("BITCOUNT abc", rc.bitcount("abc"));
    _test_return("BITOP OR aaa abc abd", rc.bitop_or("ppp","sss", "aaa", "abc", "abd"));
    _test_number("DECR abc", rc.decr(nval, "abc"));
    _test_number("DECRBY abcfffs 123", rc.decrby(nval, "abcfffs", 123));
    _test_number("GETBIT abcfffs 12", rc.getbit(nval, "abcfffs", 12));
    _test_number("GETRANGE abcfffs 1 8", rc.getrange(sval, "abcfffs", 1, 8));
    _test_number("GETSET abcfffs FFFF", rc.getset(sval, "abcfffs", "FFFF"));
    _test___list("MGET abc abcfffs ff", rc.mget(lval, "sss", "abc", "abcfffs", "ff"));
    _test_return("MSET abc def", rc.mset("ss", "abc", 0, "ss2", "fff", 0, 0));
    _test_return("PSETEX abc 1000 ccc", rc.psetex("abc", 1000, "ccc"));
    _test_return("SET abc ccc", rc.set("abc","ccc"));
    _test_return("SET abc ccc PX 12000 XX", rc.set("abc","ccc", 0, 0, 12000, false, true));
    _test_return("SETBIT abc 122 1", rc.setbit("abc", 122, 1));
    _test_return("SETBIT abc 122 1", rc.setbit("abc", 122, 1));
    _test_return("SETEX abc 12300 xxx", rc.setex("abc", 12300, "xxx"));
    _test_return("SETNX abc yyy", rc.setnx("abc", "yyy"));
    _test_return("SET abc ccc", rc.set("abc","ccc"));
    _test_return("SETRANGE abc 2 xxx", rc.setrange("abc", 2, "xxx"));
    _test_return("STRLEN abc", rc.strlen("abc"));
}

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    rc.open(zcc::default_config.get_str("server", "127.0.0.1:6379"));
    if (0) {
        std::map<std::string, std::string> infos;
        rc.info(infos, sval);
        zcc::dict_debug(infos);
    }

    test_connection();
    test_key();

    
    /* HASH */
    _test_return("HDEL hashkey aaa", rc.hdel("hashkey", "aaa"));
    _test_return("HDEL hashkey 1 aaa bbb", rc.hdel("hashkey", "1", "ss", "aaa", "bbb"));
    _test_return("HEXISTS hashkey aaa", rc.hexists("hashkey", "aaa"));
    _test_string("HGET hashkey aaa", rc.hget(sval, "hashkey", "aaa"));
    _test___list("HGETALL hashkey", rc.hgetall(lval, "hashkey"));
    _test_number("HINCRBY hashkey c1 128", rc.hincrby(nval, "hashkey", "c1", 128));
    _test_string("HINCRBYFLOAT hashkey c1 12.13", rc.hincrbyfloat(sval, "hashkey", "c1", "12.11"));
    _test_return("HLEN hashkey", rc.hlen("hashkey"));
    _test___list("HMGET hashkey abc c1", rc.hmget(lval, "hashkey", "ss", "abc", "c1"));
    _test_return("HMSET hashkey aaa v1 bbb v2", rc.hmset("hashkey", "aaa", "v1", "bbb", "v2"));
    _test_return("HMSETNX hashkey aaa v1 bbb v2", rc.hmsetnx("hashkey", "aaa", "v1", "bbb", "v2"));
    _test_return("HLEN hashkey", rc.hlen("hashkey"));
    _test___list("HVALS hashkey", rc.hvals(lval, "hashkey"));

    /* LIST */
    _test_return("BLPOP listkey 2", rc.blpop(sval, sval2, 1, "listkey", "listkey2"));
    _test_return("BRPOP listkey 1", rc.brpop(sval, sval2, 1, "listkey"));
    _test_return("BRPOPLPUSH listkey listkey2 1", rc.brpoplpush(sval, "listkey", "listkey2", 1));
    _test_string("LINDEX listkey2 1", rc.lindex(sval, "listkey2", 1));
    _test_number("LINSERT listkey BEFORE abc val", rc.linsert(nval, "listkey", true, "abc", "val"));
    _test_return("LLEN listkey", rc.llen("listkey"));
    _test_string("LPOP listkey", rc.lpop(sval, "listkey"));
    _test_return("LPUSH listkey abcdef", rc.lpush("listkey", "abcdef"));
    _test_return("LPUSH listkey a b c d", rc.lpush("listkey", "a", "b", "c", "d"));
    _test_return("LPUSHX listkey a", rc.lpushx("listkey", "a"));
    _test___list("LRANGE listkey 1 2", rc.lrange(lval, "listkey", 1, 2));
    _test_return("LREM listkey -3 a", rc.lrem("listkey", -3, "a"));
    _test_return("LSET listkey 100 a", rc.lset("listkey", 100, "a"));
    _test_return("LSET listkey 1 a", rc.lset("listkey", 1, "a"));
    _test_return("LTRIM listkey 1 -1", rc.ltrim("listkey", 1, -1));
    _test_string("RPOP listkey", rc.rpop(sval, "listkey"));
    _test_string("RPOPLPUSH listkey listkey2", rc.rpoplpush(sval, "listkey", "listkey2"));

    /* SET */
    _test_number("SADD set s e t", rc.sadd("set", "s", "e", "t"));
    _test_number("SCARD set", rc.scard("set"));
    _test___list("SDIFF set set2", rc.sdiff(lval,"set", "set2"));
    _test_return("SDIFFSTORE set3 set set2", rc.sdiffstore("set3", "set", "set2"));
    nval = 0;
    _test___list("SSCAN set 0", rc.sscan(lval,"set", nval));


    /* */

    return 0;
}

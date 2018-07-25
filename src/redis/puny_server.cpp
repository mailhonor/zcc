/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2018-01-22
 * ================================
 */

#include "zcc.h"

namespace zcc
{

/* {{{ structure */
typedef struct main_db_t main_db_t;
typedef struct main_node_t main_node_t;
typedef struct hash_node_t hash_node_t;
#pragma pack(push, 4)
enum node_type_t {
    node_type_init = 0,
    node_type_string,
    node_type_integer,
    node_type_hash,
    node_type_set,
};
typedef enum  node_type_t node_type_t;

struct main_db_t {
    rbtree_t key_tree;
    rbtree_t timeout_tree;
    int count;
};

struct main_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    union {
        char *ptr;
        char str[sizeof(char *)];
        long num;
        rbtree_t *hash_tree;
        rbtree_t *set_tree;
    } val;
    unsigned int val_len;
    rbtree_node_t rbkey;
    long timeout;
    rbtree_node_t rbtimeout;
};

static int main_node_cmp_key(rbtree_node_t * n1, rbtree_node_t * n2)
{
    main_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = zcc_container_of(n1, main_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = zcc_container_of(n2, main_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }
    std::string s1, s2;
    s1.append(key1, t1->key_len);
    s2.append(key2, t2->key_len);
    return s1.compare(s2);

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

static int main_node_cmp_timeout(rbtree_node_t * n1, rbtree_node_t * n2)
{
    main_node_t *t1, *t2;
    long r;
    t1 = zcc_container_of(n1, main_node_t, rbkey);
    t2 = zcc_container_of(n2, main_node_t, rbkey);
    r = t1->timeout - t2->timeout;
    if (r==0) {
        r = (int)(n1-n2);
    }
    return r;
}

struct hash_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    union {
        char *ptr;
        char str[sizeof(char *)];
        long num;
    } val;
    unsigned int val_len;
    rbtree_node_t rbkey;
};

static int hash_node_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    hash_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = zcc_container_of(n1, hash_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = zcc_container_of(n2, hash_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }
    std::string s1, s2;
    s1.append(key1, t1->key_len);
    s2.append(key2, t2->key_len);
    return s1.compare(s2);

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

struct set_node_t {
    union {
        char *ptr;
        char str[sizeof(char *)];
    } key;
    short int key_len:16;
    char type;
    rbtree_node_t rbkey;
};

static int set_node_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    set_node_t *t1, *t2;
    char *key1, *key2;
    int len, i, cmp;

    t1 = zcc_container_of(n1, set_node_t, rbkey);
    key1 = t1->key.ptr;
    if (t1->key_len <= (int)sizeof(char *)) {
        key1 = t1->key.str;
    }

    t2 = zcc_container_of(n2, set_node_t, rbkey);
    key2 = t2->key.ptr;
    if (t2->key_len <= (int)sizeof(char *)) {
        key2 = t2->key.str;
    }
    std::string s1, s2;
    s1.append(key1, t1->key_len);
    s2.append(key2, t2->key_len);
    return s1.compare(s2);

    len = t1->key_len;
    if (t2->key_len < len) {
        len = t2->key_len;
    }
    for(i=0;i<len;i++) {
        cmp = (int)(key1[i]) - (int)(key2[i]);
        if (cmp) {
            return cmp;
        }
    }
    if (t1->key_len <t2->key_len) {
        return -1;
    } else if (t1->key_len > t2->key_len) {
        return 1;
    }

    return 0;
}

class connection_context
{
public:
    inline connection_context() { quit = false;}
    inline ~connection_context() {}
    int fd;
    stream fp;
    main_db_t *current_db;
    bool quit;
};
typedef void (*do_cmd_fn_t)(connection_context &context, std::vector<std::string> &cmd_vector);
#pragma pack(pop)

/* }}} */

/* {{{ main node */
main_db_t default_db;

static void main_node_set_key(main_node_t *node, std::string &key)
{
    char *ptr = (char *)key.c_str();
    int klen = (int)key.size();
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            node->key.str[i] = ptr[i];
        }
    } else {
        node->key.ptr = (char *)memdup(ptr, klen);
    }
    node->key_len = klen;
}

static void main_node_get_key(main_node_t *node, std::string &key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        key.append(node->key.str, node->key_len);
    } else {
        key.append(node->key.ptr, node->key_len);
    }
}

static void main_node_set_value(main_node_t *node, std::string &val)
{
    char *ptr = (char *)val.c_str();
    int vlen = (int)val.size();
    if (vlen <= (int)sizeof(char *)) {
        for (int i=0; i<vlen; i++) {
            node->val.str[i] = ptr[i];
        }
    } else {
        node->val.ptr = (char *)memdup(ptr, vlen);
    }
    node->val_len = vlen;
    node->type = node_type_string;
}

static void main_node_clear_value(main_node_t *node)
{
    if (node->type == node_type_init) {
    } else if (node->type == node_type_string){
        if (node->val_len > sizeof(char *)) {
            free(node->val.ptr);
        }
    } else if (node->type == node_type_integer){
    } else if (node->type == node_type_hash){
        rbtree_t *tree = node->val.hash_tree;
        while (tree && rbtree_have_data(tree)) {
            rbtree_node_t *n = rbtree_first(tree);
            hash_node_t *hn = zcc_container_of(n, hash_node_t, rbkey);
            rbtree_detach(tree, n);
            if (hn->key_len > (int)sizeof(char *)) {
                free(hn->key.ptr);
            }
            if (hn->type == node_type_init) {
            } else if (hn->type == node_type_string) {
                if (hn->val_len > (int)sizeof(char *)) {
                    free(hn->val.ptr);
                }
            } else if (hn->type == node_type_integer) {
            }
            free(hn);
        }
        free(tree);
    }
    node->type = node_type_init;
    node->val_len = 0;
    node->val.ptr = 0;
}

static void main_node_get_value(main_node_t *node, std::string &val)
{
    if (node->type == node_type_string) {
        int vlen = node->val_len;
        if (vlen <= (int)sizeof(char *)) {
            val.append(node->val.str, vlen);
        } else {
            val.append(node->val.ptr, vlen);
        }
    } else if (node->type == node_type_integer) {
        to_string(val, node->val.num);
    }
}

static main_node_t *main_node_create(main_db_t *db, std::string &key)
{
    main_node_t *node = (main_node_t *)calloc(1, sizeof(main_node_t));
    node->type = node_type_init;
    main_node_set_key(node, key);
    node->val_len = 0;
    rbtree_attach(&(db->key_tree), &(node->rbkey));
    db->count ++;
    return node;
}

static main_node_t *main_node_find(main_db_t *db, std::string &k)
{
    char *ptr = (char *)k.c_str();
    int klen = k.size();

    main_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    rbtree_node_t *rnp = rbtree_find(&(db->key_tree), &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return zcc_container_of(rnp, main_node_t, rbkey);
}

static void main_node_free(main_db_t *db, main_node_t *node)
{
    rbtree_detach(&(db->key_tree), &(node->rbkey));
    db->count --;
    if (node->key_len > (int)sizeof(char *)) {
        free(node->key.ptr);
    }
    main_node_clear_value(node);
    free(node);
}

#define RETURN_WRONG_VALUE_TYPE() \
    context.fp.write("-WRONGTYPE Operation against a key holding the wrong kind of value\r\n", 68); return;

#define RETURN_WRONG_NUMBER_ARGUMENTS(c) \
    context.fp.write("-ERR wrong number of arguments for '", 36).\
    write(c, sizeof(c)-1).write("' command\r\n", 11); return;

#define RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE() \
    context.fp.write("-ERR value is not an integer or out of range\r\n", 46); return;

static bool get_check_long_integer(const char *str, long *r)
{
    long l = atoll(str);
    char buf[64];
    sprintf(buf, "%ld", l);
    if (r) {
        *r = l;
    }
    if (!strcmp(str, buf)) {
        return true;
    }
    return false;
}

static void do_cmd_unkonwn(connection_context &context, std::vector<std::string> &cmd_vector)
{
    context.fp.append("-ERR unknown command '").append(tolower(cmd_vector[0])).append("'\r\n");
}

static void do_cmd_flushdb(connection_context &context, std::vector<std::string> &cmd_vector)
{
    while(1) {
        rbtree_node_t *rbn = rbtree_first(&(context.current_db->key_tree));
        if (!rbn) {
            break;
        }
        main_node_t *node = zcc_container_of(rbn, main_node_t, rbkey);
        if (node->timeout != -1) {
            rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
        }
        main_node_free(context.current_db, node);
    }
    context.fp.write("+OK\r\n", 5);
}

static void do_cmd_flushall(connection_context &context, std::vector<std::string> &cmd_vector)
{
    do_cmd_flushdb(context, cmd_vector);
}

static void do_cmd_quit(connection_context &context, std::vector<std::string> &cmd_vector)
{
    context.quit = true;
    context.fp.puts("+OK\r\n");
}

static void do_cmd_dbsize(connection_context &context, std::vector<std::string> &cmd_vector)
{
    context.fp.printf_1024(":%d\r\n", context.current_db->count);
}

static void do_cmd_ping(connection_context &context, std::vector<std::string> &cmd_vector)
{
    context.fp.write("+PONG\r\n", 7);
}

static void do_cmd_del(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() < 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("del");
    }
    int rnum = 0;
    auto cit = cmd_vector.begin() + 1;
    for (; cit != cmd_vector.end(); cit++) {
        main_node_t *node = main_node_find(context.current_db, *cit);
        if (!node) {
            continue;
        }
        rnum++;
        if (node->timeout != -1) {
            rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
        }
        main_node_free(context.current_db, node);
    }
    context.fp.printf_1024(":%d\r\n", rnum);
}

static void do_cmd_exists(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("exists");
    }
    context.fp.printf_1024(":%d\r\n", main_node_find(context.current_db, cmd_vector[1])?1:0);
}

static void do_cmd_expire(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("expire");
    }
    int rnum = 0;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    long expire;
    if (!get_check_long_integer(cmd_vector[2].c_str(), &expire)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        rnum = 0;
    } else {
        rnum = 1;
        if (expire < 1) {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
            }
            main_node_free(context.current_db, node);
        } else {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = timeout_set(expire * 1000);
                rbtree_attach(&(context.current_db->timeout_tree), &(node->rbtimeout));
            }
        }
    }
    context.fp.printf_1024(":%d\r\n", rnum);
}

static void do_cmd_pexpire(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("pexpire");
    }
    int rnum = 0;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    long expire;
    if (!get_check_long_integer(cmd_vector[2].c_str(), &expire)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        rnum = 0;
    } else {
        rnum = 1;
        if (expire < 1) {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
            }
            main_node_free(context.current_db, node);
        } else {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = timeout_set(expire);
                rbtree_attach(&(context.current_db->timeout_tree), &(node->rbtimeout));
            }
        }
    }
    context.fp.printf_1024(":%d\r\n", rnum);
}

static void do_cmd_keys(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("keys");
    }
    /* 返回 全部 */
    int rnum = 0;
    std::string result;
    for (rbtree_node_t *rbn = rbtree_first(&(context.current_db->key_tree)); rbn; rbn = rbtree_next(rbn)) {
        main_node_t * node = zcc_container_of(rbn, main_node_t, rbkey);
        sprintf_1024(result, "$%d\r\n", node->key_len);
        main_node_get_key(node, result);
        result.append("\r\n");
        rnum ++;
    }
    context.fp.printf_1024("*%d\r\n", rnum);
    if (!result.empty()) {
        context.fp.write(result.c_str(), result.size());
    }
}

static void do_cmd_ttl(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("ttl");
    }
    long r = 0;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        r = -2;
    } else {
        if (node->timeout == -1) {
            r = -1;
        }
        r = timeout_left(node->timeout)/1000;
        if (r < 1) {
            r = 1;
        }
    }
    context.fp.printf_1024(":%d\r\n", r);
}

static void do_cmd_pttl(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("pttl");
    }
    long r = 0;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        r = -2;
    } else {
        if (node->timeout == -1) {
            r = -1;
        }
        r = timeout_left(node->timeout);
        if (r < 1) {
            r = 1;
        }
    }
    context.fp.printf_1024(":%ld\r\n", r);
}

static void do_cmd_persist(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("persist");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if ((!node) || (node->timeout==-1)) {
        context.fp.write(":0\r\n", 4);
    } else {
        context.fp.write(":1\r\n", 4);
        rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
        node->timeout = -1;
    }
}

static void do_cmd_rename(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("rename");
    }
    if (cmd_vector[1] == cmd_vector[2]) {
        context.fp.puts("-ERR source and destination objects are the same\r\n");
        return;
    }
    main_node_t *fn = main_node_find(context.current_db, cmd_vector[1]);
    if (!fn) {
        context.fp.puts("-ERR no such key\r\n");
        return;
    }

    main_node_t *tn = main_node_find(context.current_db, cmd_vector[2]);
    if (tn) {
        if (tn->timeout != -1) {
            rbtree_detach(&(context.current_db->timeout_tree), &(tn->rbtimeout));
        }
        main_node_free(context.current_db, tn);
    }

    rbtree_detach(&(context.current_db->key_tree), &(fn->rbkey));
    if (fn->key_len > (int)sizeof(char *)) {
        free(fn->key.ptr);
    }
    main_node_set_key(fn, cmd_vector[2]);
    rbtree_attach(&(context.current_db->key_tree), &(fn->rbkey));
    context.fp.puts("+OK\r\n");
}

static void do_cmd_renamenx(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("rename");
    }
    if (cmd_vector[1] == cmd_vector[2]) {
        context.fp.puts("-ERR source and destination objects are the same\r\n");
        return;
    }
    main_node_t *fn = main_node_find(context.current_db, cmd_vector[1]);
    if (!fn) {
        context.fp.puts("-ERR no such key\r\n");
        return;
    }

    main_node_t *tn = main_node_find(context.current_db, cmd_vector[2]);
    if (tn) {
        context.fp.write(":0\r\n", 4);
        return;
    }

    rbtree_detach(&(context.current_db->key_tree), &(fn->rbkey));
    if (fn->key_len > (int)sizeof(char *)) {
        free(fn->key.ptr);
    }
    main_node_set_key(fn, cmd_vector[2]);
    rbtree_attach(&(context.current_db->key_tree), &(fn->rbkey));
    context.fp.write(":1\r\n", 4);
}
static void do_cmd_type(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("type");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    const char *r = "none";
    if (!node) {
        r = "none";
    } else if (node->type == node_type_string) {
        r = "string";
    } else if (node->type == node_type_integer) {
        r = "string";
    } else if (node->type == node_type_hash) {
        r = "hash";
    } else if (node->type == node_type_set) {
        r = "set";
    }
    context.fp.printf_1024("+%s\r\n", r);
}

static void do_cmd_integer_count(connection_context &context, std::string &key, int op, const char *incr)
{
    main_node_t *node = main_node_find(context.current_db, key);
    long num;

    if (!get_check_long_integer(incr, &num)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    if (!node) {
        node = main_node_create(context.current_db, key);
        node->type = node_type_integer;
        node->val.num = 0;
    } else {
        if (node->type ==node_type_integer) {
        } else if (node->type == node_type_string) {
            std::string k;
            long num;
            main_node_get_value(node, k);
            if (!get_check_long_integer(k.c_str(), &num)) {
                RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
            }
            node->type = node_type_integer;
            node->val.num = num;
        } else {
            RETURN_WRONG_VALUE_TYPE();
        }
    }
    if (op == '+') {
        node->val.num += atoll(incr);
    } else {
        node->val.num -= atoll(incr);
    }

    context.fp.printf_1024(":%d\r\n", node->val.num);
}

static void do_cmd_decr(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("decr");
    }
    return do_cmd_integer_count(context, cmd_vector[1], '-', "1");
}

static void do_cmd_decrby(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("decrby");
    }
    return do_cmd_integer_count(context, cmd_vector[1], '-', cmd_vector[2].c_str());
}

static void do_cmd_incr(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("incr");
    }
    return do_cmd_integer_count(context, cmd_vector[1], '+', "1");
}

static void do_cmd_incrby(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("incrby");
    }
    return do_cmd_integer_count(context, cmd_vector[1], '+', cmd_vector[2].c_str());
}

static void do_cmd_get(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("get");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (node == 0) {
        context.fp.puts("$-1\r\n");
    } else if (node->type == node_type_integer || node->type == node_type_string) {
        std::string tmp;
        main_node_get_value(node, tmp);
        context.fp.printf_1024("$%d\r\n", (int)tmp.size()).append(tmp).append("\r\n");
    } else {
        RETURN_WRONG_VALUE_TYPE();
    }
}
static void do_cmd_set(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if (pcount < 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("set");
    }
    int idx = 3;
    int have_ex = false, have_px = false, have_nx = false, have_xx = false;
    long epx = 1, epx_tmp;
    bool syntax_error = false, number_ok = true;
    while (pcount > idx) {
        char *epnx = (char *)cmd_vector[idx].c_str();
        if ((cmd_vector[idx].size() != 2) || (toupper(epnx[1]) != 'X')) {
            syntax_error = true;
            break;
        }

        char ch1 = toupper(epnx[0]);
        if ((ch1 == 'E') || (ch1 == 'P')) {
            if (idx+1 == pcount) {
                syntax_error = true;
                break;
            }
            number_ok = get_check_long_integer(cmd_vector[idx+1].c_str(), &epx_tmp);
            if (!number_ok) {
                break;
            }
            if (ch1 == 'E') {
                have_ex = true;
                epx = epx_tmp * 1000;
            } else {
                have_px = true;
                epx = epx_tmp;
            }
            idx += 2;
        } else if (ch1 == 'N'){
            have_nx = true;
            idx += 1;
        } else if (ch1 == 'X'){
            have_xx = true;
            idx += 1;
        } else {
            syntax_error = true;
            break;
        }
    }
    if (have_ex && have_px) {
        syntax_error = true;
    }
    if (syntax_error) {
        RETURN_WRONG_NUMBER_ARGUMENTS("set");
    }
    if (!number_ok) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }

    bool gogogo = false;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (have_xx) {
        if (node) {
            gogogo = true;
        }
    } else if (have_nx) {
        if (!node) {
            gogogo = true;
        }
    } else {
        gogogo = true;
    }

    if (gogogo) {
        if (node) {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = -1;
            }
            main_node_clear_value(node);
        } else {
            node = main_node_create(context.current_db, (cmd_vector[1]));
        }
    } else if (!gogogo) {
        context.fp.puts("$-1\r\n");
        return;
    }
    if (have_ex || have_px) {
        node->timeout = timeout_set(epx);
        rbtree_attach(&(context.current_db->timeout_tree), &(node->rbtimeout));
    }
    main_node_set_value(node, cmd_vector[2]);
    context.fp.puts("+OK\r\n");
    return;
}

static void do_cmd_getset(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("getset");
    }
    bool have_old = false;
    std::string old_val;
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (node) {
        if ((node->type != node_type_integer) && (node->type != node_type_string)) {
            RETURN_WRONG_VALUE_TYPE();
        }
        if (node->timeout != -1) {
            rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
            node->timeout = -1;
        }
        main_node_get_value(node, old_val);
        main_node_clear_value(node);
        have_old = true;
    } else {
        node = main_node_create(context.current_db, cmd_vector[1]);
    }
    main_node_set_value(node, cmd_vector[1]);
    if (have_old) {
        context.fp.printf_1024("$%d\r\n", (int)old_val.size());
        context.fp.write(old_val.c_str(), old_val.size());
        context.fp.puts("\r\n");
    } else {
        context.fp.puts("$-1\r\n");
    }
}

static void do_cmd_mget(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() < 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("mget");
    }
    auto cit = cmd_vector.begin() + 1;
    std::list<std::string *> result_list;
    int count = 0;
    for (; cit != cmd_vector.end(); cit++) {
        count ++;
        std::string &key = *cit;
        main_node_t *node = main_node_find(context.current_db, key);
        if (!node) {
            result_list.push_back(0);
        } else if ((node->type == node_type_integer) || (node->type == node_type_string)) {
            std::string *s = new std::string();
            main_node_get_value(node, *s);
            result_list.push_back(s);
        } else {
            result_list.push_back(0);
        }
    }
    context.fp.printf_1024("*%d\r\n", count);
    while(!result_list.empty()) {
        std::string *s = result_list.front();
        result_list.pop_front();
        if (s) {
            context.fp.printf_1024("$%d\r\n", (int)s->size());
            context.fp.write(s->c_str(), s->size()).puts("\r\n");
            delete s;
        } else {
            context.fp.puts("$-1\r\n");
        }
    }
}

static void do_cmd_mset(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3) || (pcount%2 != 1)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("mset");
    }

    std::list<std::string *> result_list;
    for (auto cit = cmd_vector.begin() + 1; cit != cmd_vector.end(); cit++) {
        std::string &key = *cit++;
        std::string &val = *cit;
        main_node_t *node = main_node_find(context.current_db, key);
        if (node) {
            if (node->timeout != -1) {
                rbtree_detach(&(context.current_db->timeout_tree), &(node->rbtimeout));
                node->timeout = -1;
            }
            main_node_clear_value(node);
        } else {
            node = main_node_create(context.current_db, key);
        }
        main_node_set_value(node, val);
    }
    context.fp.puts("+OK\r\n");
}

static void do_cmd_msetnx(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3) || (pcount%2 != 1)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("msetnx");
    }
    for (auto cit = cmd_vector.begin() + 1; cit != cmd_vector.end(); cit++) {
        std::string &key = *cit++;
        main_node_t *node = main_node_find(context.current_db, key);
        if (node) {
            context.fp.puts(":0\r\n");
            return;
        }
    }
    for (auto cit = cmd_vector.begin() + 1; cit != cmd_vector.end(); cit++) {
        std::string &key = *cit++;
        std::string &val = *cit;
        main_node_t *node = main_node_create(context.current_db, key);
        main_node_set_value(node, val);
    }
    context.fp.puts(":1\r\n");
}

/* }}} */

/* {{{ hash node */
static hash_node_t *hash_node_create(main_node_t *node, std::string &key)
{
    hash_node_t *hnode = (hash_node_t *)calloc(1, sizeof(hash_node_t));
    hnode->type = node_type_init;
    char *ptr = (char *)key.c_str();
    int klen = (int)key.size();
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            hnode->key.str[i] = ptr[i];
        }
    } else {
        hnode->key.ptr = (char *)memdup(ptr, klen);
    }
    hnode->key_len = klen;
    rbtree_attach(node->val.hash_tree, &(hnode->rbkey));
    node->val_len++;
    return hnode;
}

static hash_node_t *hash_node_find(main_node_t *node, std::string &k)
{
    char *ptr = (char *)k.c_str();
    int klen = k.size();

    hash_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    rbtree_node_t *rnp = rbtree_find(node->val.hash_tree, &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return zcc_container_of(rnp, hash_node_t, rbkey);
}

static void hash_node_set_value(hash_node_t *node, std::string &val)
{
    char *ptr = (char *)val.c_str();
    int vlen = (int)val.size();
    if (vlen <= (int)sizeof(char *)) {
        for (int i=0; i<vlen; i++) {
            node->val.str[i] = ptr[i];
        }
    } else {
        node->val.ptr = (char *)memdup(ptr, vlen);
    }
    node->val_len = vlen;
    node->type = node_type_string;
}

static void hash_node_get_key(hash_node_t *node, std::string &key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        key.append(node->key.str, node->key_len);
    } else {
        key.append(node->key.ptr, node->key_len);
    }
}

static void hash_node_get_value(hash_node_t *node, std::string &val)
{
    if (node->type == node_type_string) {
        int vlen = node->val_len;
        if (vlen <= (int)sizeof(char *)) {
            val.append(node->val.str, vlen);
        } else {
            val.append(node->val.ptr, vlen);
        }
    } else if (node->type == node_type_integer) {
        to_string(val, node->val.num);
    }
}

static void hash_node_clear_value(hash_node_t *node)
{
    if (node->type == node_type_init) {
    } else if (node->type == node_type_string){
        if (node->val_len > sizeof(char *)) {
            free(node->val.ptr);
        }
    } else if (node->type == node_type_integer){
    }
    node->type = node_type_init;
    node->val_len = -1;
    node->val.ptr = 0;
}
static void hash_node_free(main_node_t *node, hash_node_t *hnode)
{
    node->val_len--;
    rbtree_detach(node->val.hash_tree, &(hnode->rbkey));
    if (hnode->key_len > (int)sizeof(char *)) {
        free(hnode->key.ptr);
    }
    hash_node_clear_value(hnode);
    free(hnode);
}


static main_node_t * main_node_create_with_hash_val(connection_context &context, std::string &key)
{
    main_node_t * node = main_node_create(context.current_db, key);
    node->type = node_type_hash;
    node->val_len = 0;
    node->val.hash_tree = (rbtree_t *)calloc(1, sizeof(rbtree_t));
    rbtree_init(node->val.hash_tree, hash_node_cmp);
    return node;
}

static void do_cmd_hdel(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hdel");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write(":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int dcount = 0;
    for(auto it = cmd_vector.begin()+ 2; it != cmd_vector.end(); it++) {
        hash_node_t *hnode = hash_node_find(node, *it);
        if (!hnode) {
            continue;
        }
        dcount++;
        hash_node_free(node, hnode);
    }
    if ( node->val_len == 0) {
        main_node_free(context.current_db, node);
    }
    context.fp.printf_1024(":%d\r\n", dcount);
}

static void do_cmd_hexists(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hexists");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write(":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    if (hash_node_find(node, cmd_vector[2])) {
        context.fp.write(":1\r\n", 4);
    } else {
        context.fp.write(":0\r\n", 4);
    }
}

static void do_cmd_hlen(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hlen");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write(":0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    context.fp.printf_1024(":%d\r\n", node->val_len);
}

static void do_cmd_hget(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hget");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write("$-1\r\n", 5);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, cmd_vector[2]);
    if (hnode) {
        std::string tmpv;
        hash_node_get_value(hnode, tmpv);
        context.fp.printf_1024("$%zd\r\n", tmpv.size());
        context.fp.append(tmpv).append("\r\n");
    } else {
        context.fp.write("$-1\r\n", 5);
    }
}

static void do_cmd_hset(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hset");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        node = main_node_create_with_hash_val(context, cmd_vector[1]);
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, cmd_vector[2]);
    if (!hnode) {
        hnode = hash_node_create(node, cmd_vector[2]);
        context.fp.write(":0\r\n", 4);
    } else {
        hash_node_clear_value(hnode);
        context.fp.write(":1\r\n", 4);
    }
    hash_node_set_value(hnode, cmd_vector[3]);
}

static void do_cmd_hsetnx(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hsetnx");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        node = main_node_create_with_hash_val(context, cmd_vector[1]);
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, cmd_vector[2]);
    if (!hnode) {
        hnode = hash_node_create(node, cmd_vector[2]);
        hash_node_set_value(hnode, cmd_vector[3]);
        context.fp.write(":1\r\n", 4);
    } else {
        context.fp.write(":0\r\n", 4);
    }
}

static void do_cmd_hincrby(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 4) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hincrby");
    }
    long num;
    if (!get_check_long_integer(cmd_vector[3].c_str(), &num)) {
        RETURN_NOT_INTEGER_OUR_OUT_OF_RANGE();
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        node = main_node_create_with_hash_val(context, cmd_vector[1]);
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    hash_node_t *hnode = hash_node_find(node, cmd_vector[2]);
    if (!hnode) {
        hnode = hash_node_create(node, cmd_vector[2]);
        hnode->type = node_type_integer;
        hnode->val.num = 0;
    } else {
        std::string tmpv;
        hash_node_get_value(hnode, tmpv);
        long num2;
        if (!get_check_long_integer(tmpv.c_str(), &num2)) {
            context.fp.write("-ERR hash value is not an integerr\r\n", 36);
            return;
        }
        hash_node_clear_value(hnode);
        hnode->val.num = num2;
        hnode->type = node_type_integer;
    }
    hnode->val.num += num;
    context.fp.printf_1024(":%d\r\n", hnode->val.num);
}

static void do_cmd_hkeys(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hkeys");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write("*0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    std::string result;
    for (rbtree_node_t *rbn = rbtree_first(node->val.hash_tree); rbn; rbn = rbtree_next(rbn)) {
        hash_node_t * hnode = zcc_container_of(rbn, hash_node_t, rbkey);
        sprintf_1024(result, "$%d\r\n", hnode->key_len);
        hash_node_get_key(hnode, result);
        result.append("\r\n");
        rnum ++;
    }
    context.fp.printf_1024("*%d\r\n", rnum);
    if (!result.empty()) {
        context.fp.write(result.c_str(), result.size());
    }
}

static void do_cmd_hmget(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() < 3) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hmget");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (node && (node->type != node_type_hash)) {
        RETURN_WRONG_VALUE_TYPE();
    }

    std::string tmpval;
    std::string result;
    sprintf_1024(result, "*%d\r\n", (int)(cmd_vector.size()) - 2);
    for (auto it = cmd_vector.begin() + 2; it != cmd_vector.end(); it++) {
        if (!node) {
            result.append("$-1\r\n", 5);
        } else {
            hash_node_t *hnode = hash_node_find(node, *it);
            if (hnode) {
                tmpval.clear();
                hash_node_get_value(hnode, tmpval);
                sprintf_1024(result, "$%d\r\n", (int)tmpval.size());
                result.append(tmpval).append("\r\n");
            } else {
                result.append("$-1\r\n", 5);
            }
        }
    }
    context.fp.write(result.c_str(), result.size());
}

static void do_cmd_hmset(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 4) || (pcount%2 != 0)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hmset");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        node = main_node_create_with_hash_val(context, cmd_vector[1]);
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    for (auto cit = cmd_vector.begin() + 2; cit != cmd_vector.end(); cit++) {
        std::string &key = *cit++;
        std::string &val = *cit;
        hash_node_t *hnode = hash_node_find(node, key);
        if (hnode) {
            hash_node_clear_value(hnode);
        } else {
            hnode = hash_node_create(node, key);
        }
        hash_node_set_value(hnode, val);
    }
    context.fp.puts("+OK\r\n");
}

static void do_cmd_hgetall(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("hgetall");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write("*0\r\n", 4);
        return;
    }
    if (node->type != node_type_hash) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    std::string result, tmpv;
    for (rbtree_node_t *rbn = rbtree_first(node->val.hash_tree); rbn; rbn = rbtree_next(rbn)) {
        hash_node_t * hnode = zcc_container_of(rbn, hash_node_t, rbkey);
        sprintf_1024(result, "$%d\r\n", hnode->key_len);
        hash_node_get_key(hnode, result);
        result.append("\r\n");
        tmpv.clear();
        hash_node_get_value(hnode, tmpv);
        sprintf_1024(result, "$%zd\r\n", tmpv.size());
        result.append(tmpv).append("\r\n");
        rnum +=2;
    }
    context.fp.printf_1024("*%d\r\n", rnum);
    if (!result.empty()) {
        context.fp.write(result.c_str(), result.size());
    }
}

/* }}} */

/* {{{ set node */
static set_node_t *set_node_create(main_node_t *node, std::string &key)
{
    set_node_t *hnode = (set_node_t *)calloc(1, sizeof(set_node_t));
    hnode->type = node_type_init;
    char *ptr = (char *)key.c_str();
    int klen = (int)key.size();
    if (klen <= (int)sizeof(char *)) {
        for (int i=0; i<klen; i++) {
            hnode->key.str[i] = ptr[i];
        }
    } else {
        hnode->key.ptr = (char *)memdup(ptr, klen);
    }
    hnode->key_len = klen;
    rbtree_attach(node->val.set_tree, &(hnode->rbkey));
    node->val_len++;
    return hnode;
}

static set_node_t *set_node_find(main_node_t *node, std::string &k)
{
    char *ptr = (char *)k.c_str();
    int klen = k.size();

    set_node_t vnode;
    vnode.key_len = klen;
    if (klen <= (int)sizeof(char *)) {
        for (int i=0;i<klen;i++) {
            vnode.key.str[i]=ptr[i];
        }
    } else {
        vnode.key.ptr = ptr;
    }

    rbtree_node_t *rnp = rbtree_find(node->val.set_tree, &(vnode.rbkey));
    if (!rnp) {
        return 0;
    }
    return zcc_container_of(rnp, set_node_t, rbkey);
}

static void set_node_get_key(set_node_t *node, std::string &key)
{
    if (node->key_len <= (int)sizeof(char *)) {
        key.append(node->key.str, node->key_len);
    } else {
        key.append(node->key.ptr, node->key_len);
    }
}

static void set_node_free(main_node_t *node, set_node_t *hnode)
{
    node->val_len--;
    rbtree_detach(node->val.set_tree, &(hnode->rbkey));
    if (hnode->key_len > (int)sizeof(char *)) {
        free(hnode->key.ptr);
    }
    free(hnode);
}


static main_node_t * main_node_create_with_set_val(connection_context &context, std::string &key)
{
    main_node_t * node = main_node_create(context.current_db, key);
    node->type = node_type_set;
    node->val_len = 0;
    node->val.set_tree = (rbtree_t *)calloc(1, sizeof(rbtree_t));
    rbtree_init(node->val.set_tree, set_node_cmp);
    return node;
}

static void do_cmd_sadd(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("sadd");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        node = main_node_create_with_set_val(context, cmd_vector[1]);
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int add_count = 0;
    for (auto cit = cmd_vector.begin() + 2; cit != cmd_vector.end(); cit++) {
        std::string &key = *cit;
        set_node_t *hnode = set_node_find(node, key);
        if (!hnode) {
            set_node_create(node, key);
            add_count++;
        }
    }
    context.fp.printf_1024(":%d\r\n", add_count);
}

static void do_cmd_scard(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 2)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("scard");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.printf_1024(":0\r\n");
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    context.fp.printf_1024(":%d\r\n", node->val_len);
}

static void do_cmd_sismember(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("sismember");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write(":0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    set_node_t *hnode = set_node_find(node, cmd_vector[2]);
    if (!hnode) {
        context.fp.write(":0\r\n", 4);
    } else {
        context.fp.write(":1\r\n", 4);
    }
}

static void do_cmd_smembers(connection_context &context, std::vector<std::string> &cmd_vector)
{
    if (cmd_vector.size() != 2) {
        RETURN_WRONG_NUMBER_ARGUMENTS("smembers");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write("*0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int rnum = 0;
    std::string result, tmpv;
    for (rbtree_node_t *rbn = rbtree_first(node->val.set_tree); rbn; rbn = rbtree_next(rbn)) {
        set_node_t * hnode = zcc_container_of(rbn, set_node_t, rbkey);
        sprintf_1024(result, "$%d\r\n", hnode->key_len);
        set_node_get_key(hnode, result);
        result.append("\r\n");
        rnum +=1;
    }
    context.fp.printf_1024("*%d\r\n", rnum);
    if (!result.empty()) {
        context.fp.write(result.c_str(), result.size());
    }
}

static void do_cmd_srem(connection_context &context, std::vector<std::string> &cmd_vector)
{
    int pcount = (int)cmd_vector.size();
    if ((pcount < 3)) {
        RETURN_WRONG_NUMBER_ARGUMENTS("srem");
    }
    main_node_t *node = main_node_find(context.current_db, cmd_vector[1]);
    if (!node) {
        context.fp.write(":0\r\n", 4);
        return;
    }
    if (node->type != node_type_set) {
        RETURN_WRONG_VALUE_TYPE();
    }
    int dcount = 0;
    for(auto it = cmd_vector.begin()+ 2; it != cmd_vector.end(); it++) {
        set_node_t *hnode = set_node_find(node, *it);
        if (!hnode) {
            continue;
        }
        dcount++;
        set_node_free(node, hnode);
    }
    if ( node->val_len == 0) {
        main_node_free(context.current_db, node);
    }
    context.fp.printf_1024(":%d\r\n", dcount);
}
/* }}} */

/* {{{ cmd tree */
static std::map<std::string, do_cmd_fn_t> redis_cmd_tree;
static void redis_cmd_tree_init()
{
    redis_cmd_tree["FLUSHDB"] = do_cmd_flushdb;
    redis_cmd_tree["FLUSHALL"] = do_cmd_flushall;
    redis_cmd_tree["QUIT"] = do_cmd_quit;
    redis_cmd_tree["DBSIZE"] = do_cmd_dbsize;
    redis_cmd_tree["PING"] = do_cmd_ping;
    redis_cmd_tree["DEL"] = do_cmd_del;
    redis_cmd_tree["EXISTS"] = do_cmd_exists;
    redis_cmd_tree["EXPIRE"] = do_cmd_expire;
    redis_cmd_tree["PEXPIRE"] = do_cmd_pexpire;
    redis_cmd_tree["KEYS"] = do_cmd_keys;
    redis_cmd_tree["TTL"] = do_cmd_ttl;
    redis_cmd_tree["PTTL"] = do_cmd_pttl;
    redis_cmd_tree["PERSIST"] = do_cmd_persist;
    redis_cmd_tree["RENAME"] = do_cmd_rename;
    redis_cmd_tree["RENAMENX"] = do_cmd_renamenx;
    redis_cmd_tree["TYPE"] = do_cmd_type;
    redis_cmd_tree["DECR"] = do_cmd_decr;
    redis_cmd_tree["DECRBY"] = do_cmd_decrby;
    redis_cmd_tree["INCR"] = do_cmd_incr;
    redis_cmd_tree["INCRBY"] = do_cmd_incrby;
    redis_cmd_tree["GET"] = do_cmd_get;
    redis_cmd_tree["SET"] = do_cmd_set;
    redis_cmd_tree["GETSET"] = do_cmd_getset;
    redis_cmd_tree["MGET"] = do_cmd_mget;
    redis_cmd_tree["MSET"] = do_cmd_mset;
    redis_cmd_tree["MSETNX"] = do_cmd_msetnx;
    redis_cmd_tree["HDEL"] = do_cmd_hdel;
    redis_cmd_tree["HEXISTS"] = do_cmd_hexists;
    redis_cmd_tree["HLEN"] = do_cmd_hlen;
    redis_cmd_tree["HGET"] = do_cmd_hget;
    redis_cmd_tree["HSET"] = do_cmd_hset;
    redis_cmd_tree["HSETNX"] = do_cmd_hsetnx;
    redis_cmd_tree["HINCRBY"] = do_cmd_hincrby;
    redis_cmd_tree["HKEYS"] = do_cmd_hkeys;
    redis_cmd_tree["HMGET"] = do_cmd_hmget;
    redis_cmd_tree["HMSET"] = do_cmd_hmset;
    redis_cmd_tree["HGETALL"] = do_cmd_hgetall;
    redis_cmd_tree["SADD"] = do_cmd_sadd;
    redis_cmd_tree["SCARD"] = do_cmd_scard;
    redis_cmd_tree["SMEMBERS"] = do_cmd_smembers;
    redis_cmd_tree["SISMEMBER"] = do_cmd_sismember;
    redis_cmd_tree["SREM"] = do_cmd_srem;
}
/* }}} */

/* {{{ server */
static void parse_query_logic_line(std::vector<std::string> &cmd_vector, std::string &strbuf)
{
    char *ps = (char *)strbuf.c_str();
    size_t size = strbuf.size();
    std::string tmpbuf;
    if (size > 0) {
        if (ps[size-1] =='\n') {
            size--;
        }
    }
    if (size > 0) {
        if (ps[size-1] =='\r') {
            size--;
        }
    }
    ps[size] = 0;
    strtok sk(ps);
    while(sk.tok(" \t")) {
        tmpbuf.clear();
        tmpbuf.append(sk.ptr(), sk.size());
        cmd_vector.push_back(tmpbuf);
    }
}

static int get_query_data(std::vector<std::string> &cmd_vector, std::string &strbuf, connection_context &context)
{
    stream &fp = context.fp;
    const char *ps = strbuf.c_str();
    int count = atoi(ps+1);
    if (count < 1) {
        return -2;
    }
    cmd_vector.reserve(count);
    for (int i = 0; i < count; i++) {
        strbuf.clear();
        fp.gets(strbuf);
        if (fp.is_exception()) {
            return -2;
        }
        ps = (char *)strbuf.c_str();
        if (*ps != '$') {
            return -2;
        }
        int slen = atoi(ps + 1);
        if (slen < 0 || slen > 1024 * 1024) {
            return -2;
        }
        strbuf.clear();
        if (slen > 0) {
            fp.readn(strbuf, slen);
        }
        fp.readn(0, 2);
        if (fp.is_exception()) {
            return -2;
        }
        cmd_vector.push_back(strbuf);
    }
    return 0;
}

static int do_one_query(connection_context &context)
{
    stream &fp = context.fp;
    int ret;
    while(1) {
        ret = fp.timed_wait_readable(10 * 1000);
        if (ret > 0) {
            break;
        }
        if (ret < 0) {
            return ret;
        }
    }
    std::vector<std::string> cmd_vector;
    std::string strbuf;
    /* do not care about too much long line */
    fp.gets(strbuf);
    if (fp.is_exception()) {
        return -2;
    }
    if (strbuf.c_str()[0] != '*') {
        parse_query_logic_line(cmd_vector, strbuf);
    } else if (get_query_data(cmd_vector, strbuf, context) < 0) {
        return -2;
    }
    if (cmd_vector.empty()) {
        fp.puts("\r\n");
    } else {
        do_cmd_fn_t cmdfn;
        std::string &cmd_name = cmd_vector.front();
        toupper(cmd_name);
        auto cit = redis_cmd_tree.find(cmd_name);
        if (cit == redis_cmd_tree.end()) {
            cmdfn = do_cmd_unkonwn;
        } else {
            cmdfn = cit->second;
        }
        cmdfn(context, cmd_vector);
    }
    context.fp.flush();
    if (context.fp.is_exception()) { 
        return -2;
    }
    return 1;
}

static void *do_job(void *arg)
{
    connection_context context;
    context.fd = (int)(long)arg;
    context.fp.open(context.fd);
    context.current_db = &default_db;
    std::vector<std::string *> cmd_vector;
    while (1) {
        if ((do_one_query(context) < 0) || context.quit) {
            break;
        }
    }
    context.fp.close();
    if (context.fd > 0) {
        close(context.fd);
    }
    return arg;
}

static void *do_accept(void *arg)
{
    int sock = (int)(long)arg, fd, ret;
    while(1) {
        ret = timed_wait_readable(sock, 100 * 1000);
        if (ret < 0) {
            zcc_fatal("redis_puny_server, socket fd error");
        }
        if (ret == 0) {
            continue;
        }
        fd = inet_accept(sock);
        if (fd < 0) {
            continue;
        }
        coroutine_go(do_job, (void *)(long)fd);
    }
    return arg;
}

void redis_puny_server::service_register(const char *service_name, int fd, int fd_type)
{
    coroutine_go(do_accept, (void *)(long)fd);
}

static void *do_timeout(void *arg)
{
    while(1) {
        zcc::coroutine_msleep(1000);
        long nowtime = timeout_set(0);
        while(1) {
            rbtree_node_t *rbn = rbtree_first(&(default_db.timeout_tree));
            if (!rbn) {
                break;
            }
            main_node_t *node = zcc_container_of(rbn, main_node_t, rbtimeout);
            if (node->timeout <= nowtime) {
                rbtree_detach(&(default_db.timeout_tree), &(node->rbtimeout));
                main_node_free(&default_db, node);
            } else {
                break;
            }
        }
    }
    return arg;
}

void redis_puny_server::before_service()
{
    rbtree_init(&(default_db.key_tree), main_node_cmp_key);
    rbtree_init(&(default_db.timeout_tree), main_node_cmp_timeout);
    redis_cmd_tree_init();

    const char *attr;
    attr = default_config.get_str("redis-server-prepare-cmd", "");
    if (!empty(attr)) {
        system(attr);
    }

    attr = default_config.get_str("redis-server-prepare-load", "");
    do {
        if (empty(attr)) {
            break;
        }
        connection_context ctx;
        ctx.current_db = &default_db;
        fstream fp;
        if (!fp.open(attr, "r")) {
            zcc_fatal("can not open %s(%m)", attr);
        }
        char *linebuf = (char *)malloc(1024*1024 + 1);
        int linelen;
        while((linelen = fp.gets(linebuf, 1024 * 1204)) > 0) {
            linebuf[linelen] = 0;
            std::vector<std::string> cmds;
            json jss;
            jss.unserialize(linebuf);
            if (!jss.is_array()) {
                continue;
            }
            bool exception = false;
            auto &jsarr = jss.get_array_value();
            for (auto jit = jsarr.begin(); jit != jsarr.end(); jit++) {
                json *js = *jit;
                if (!js->is_string()) {
                    exception = true;
                    break;
                }
                cmds.push_back(js->get_string_value());
            }
            if (exception) {
                continue;
            }
            if (cmds.empty()) {
                continue;
            }
            toupper(cmds[0]);
            auto cit = redis_cmd_tree.find(cmds[0]);
            if (cit == redis_cmd_tree.end()) {
                continue;
            }
            cit->second(ctx, cmds);
        }
        free(linebuf);
    } while(0);

    coroutine_go(do_timeout, 0);
}

void redis_puny_server::before_exit()
{
}

void redis_puny_server::exec_redis_cmd(std::vector<std::string> &cmds)
{
    if (cmds.empty()){
        return;
    }
    toupper(cmds[0]);
    auto cit = redis_cmd_tree.find(cmds[0]);
    if (cit == redis_cmd_tree.end()) {
        return;
    }
    connection_context ctx;
    ctx.current_db = &default_db;
    cit->second(ctx, cmds);
}

void redis_puny_server::exec_redis_cmd(const char *cmdline)
{
    std::vector<std::string> cmds;
    std::string tmp = cmdline;
    parse_query_logic_line(cmds, tmp);
    exec_redis_cmd(cmds);
}

redis_puny_server::redis_puny_server()
{
}

redis_puny_server::~redis_puny_server()
{
}

}

/* }}} */

/* Local variables:
* End:
* vim600: fdm=marker
*/

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-30
 * ================================
 */

#include "memkv.h"

#pragma pack(push, 1)
namespace zcc
{
#define memkv_node_type_partition   1
#define memkv_node_type_string      2
#define memkv_node_type_int         3

static void after_recv_request(async_io &aio);

/* {{{ memkv memory structure */ 
typedef union memkv_magic_char_t memkv_magic_char_t;
union memkv_magic_char_t {
    char *ptr;
    char buf[sizeof(char *)];
};

#define memkv_node_fragment \
    rbtree_node_t rbnode; \
    unsigned char type; \
    short int klen; \
    memkv_magic_char_t key

typedef struct memkv_node_basic_t memkv_node_basic_t;
struct memkv_node_basic_t {
    memkv_node_fragment;
};

typedef struct memkv_node_partition_t memkv_node_partition_t;
struct memkv_node_partition_t {
    memkv_node_fragment;
    rbtree_t tree;
};

typedef struct memkv_node_string_t memkv_node_string_t;
struct memkv_node_string_t {
    memkv_node_fragment;
    memkv_magic_char_t val;
    int vlen;
};

typedef struct memkv_node_int_t memkv_node_int_t;
struct memkv_node_int_t {
    memkv_node_fragment;
    long val;
};
/* }}} */

static std::string req_buf;
static char *req_partition, *req_key, *req_val;
static int req_partition_len, req_key_len, req_val_len;
static rbtree_t partition_tree;
static pm_pool node_partition_pmp, node_string_pmp, node_int_pmp;
static memkv_node_partition_t *current_partition;

/* {{{ memkv inner op */
static int memkv_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    memkv_node_basic_t *b1, *b2;
    b1 = zcc_container_of(n1, memkv_node_basic_t, rbnode);
    b2 = zcc_container_of(n2, memkv_node_basic_t, rbnode);
    int len1, len2, mlen;
    char *c1, *c2;
    len1= b1->klen;
    c1 = ((b1->klen<=(int)sizeof(char*))?b1->key.buf:b1->key.ptr);
    len2= b2->klen;
    c2 = ((b2->klen<=(int)sizeof(char*))?b2->key.buf:b2->key.ptr);

    if ((len1==0) || (len2==0)) {
        return len1 - len2;
    }
    if (len1 == len2) {
        return memcmp(c1, c2, len1);
    }
    if (len1 < len2) {
        mlen = len1;
    } else {
        mlen = len2;
    }
    int result = memcmp(c1, c2, mlen);
    if (!result) {
        if (len1 > len2) {
            return 1;
        }
        return -1;
    }
    return result;
}

static memkv_node_basic_t * memkv_node_basic_find(rbtree_t *tree, const char *key, int klen)
{
    memkv_node_basic_t vnode;
    vnode.klen = klen;
    if (klen == 0) {
        vnode.key.buf[0] = 0;
    } else if (klen <= (int)sizeof(char *)) {
        memcpy(vnode.key.buf, key, klen);
    } else {
        vnode.key.ptr = (char *)(void *)key;
    }
    rbtree_node_t *rbn = rbtree_find(tree, &(vnode.rbnode));
    if (!rbn) {
        return 0;
    }
    return zcc_container_of(rbn, memkv_node_basic_t, rbnode);
}

static memkv_node_partition_t *memkv_node_partition_create(const char *key, int klen)
{
    memkv_node_partition_t *node = (memkv_node_partition_t *)node_partition_pmp.require();
    node->type = memkv_node_type_partition;
    if (klen <= (int)sizeof(char*)) {
        if (klen) {
            memcpy(node->key.buf, key, klen);
        }
    } else {
        node->key.ptr = memdup(key, klen);
    }
    node->klen = klen;
    rbtree_init(&(node->tree), memkv_cmp);
    rbtree_attach(&partition_tree, &(node->rbnode));
    return node;
}

static void memkv_node_partition_free(memkv_node_partition_t *node)
{
    rbtree_detach(&(partition_tree), &(node->rbnode));
    if (node->klen > (int)sizeof(char *)) {
        free(node->key.ptr);
    }
    node_partition_pmp.release(node);
}

static memkv_node_string_t *memkv_node_string_create(memkv_node_partition_t *pnode, const char *key, int klen)
{
    memkv_node_string_t *node = (memkv_node_string_t *)node_string_pmp.require();
    node->type = memkv_node_type_string;
    if (klen <= (int)sizeof(char*)) {
        if (klen) {
            memcpy(node->key.buf, key, klen);
        }
    } else {
        node->key.ptr = memdup(key, klen);
    }
    node->klen = klen;
    rbtree_attach(&(pnode->tree), &(node->rbnode));
    return node;
}

static void memkv_node_string_free(memkv_node_partition_t *partition, memkv_node_string_t *node)
{
    rbtree_detach(&(partition->tree), &(node->rbnode));
    if (node->klen > (int)sizeof(char *)) {
        free(node->key.ptr);
    }
    node_string_pmp.release(node);
}

static memkv_node_int_t *memkv_node_int_create(memkv_node_partition_t *pnode, const char *key, int klen)
{
    memkv_node_int_t *node = (memkv_node_int_t *)node_int_pmp.require();
    node->type = memkv_node_type_int;
    if (klen <= (int)sizeof(char*)) {
        if (klen) {
            memcpy(node->key.buf, key, klen);
        }
    } else {
        node->key.ptr = memdup(key, klen);
    }
    node->klen = klen;
    rbtree_attach(&(pnode->tree), &(node->rbnode));
    return node;
}

static void memkv_node_int_free(memkv_node_partition_t *partition, memkv_node_int_t *node)
{
    rbtree_detach(&(partition->tree), &(node->rbnode));
    if (node->klen > (int)sizeof(char *)) {
        free(node->key.ptr);
    }
    node_int_pmp.release(node);
}
/* }}} */

/* {{{ exception, response, unknown */
static void exception_do(async_io &aio)
{
    int fd = aio.get_fd();
    delete &aio;
    close(fd);
}

static void after_send_response(async_io &aio)
{
    if (aio.get_result() < 1) {
        exception_do(aio);
        return;
    }
    aio.read_size_data(after_recv_request, 0);
}

static void response_status(async_io &aio, int status)
{
    char obuf[2];
    obuf[0] = status;
    obuf[1] = 0X80;
    aio.cache_write(obuf, 2);
    aio.cache_flush(after_send_response, 0);
}

static void response_result(async_io &aio, char *data, int len)
{
    char obuf[2], buf[32];
    obuf[0] = memkv_op_result_want;
    obuf[1] = 0;
    aio.cache_write(obuf, 1);
    if (len ==-2) {
        len = sprintf(buf, "%ld", (long)data);
        data = buf;
    }
    aio.cache_write_size_data(data, len);
    aio.cache_flush(after_send_response, 0);
}
/* }}} */

/* {{{ op_set */
static void op_set(async_io &aio)
{
    if (!current_partition) {
        current_partition = memkv_node_partition_create(req_partition, req_partition_len);
    }
    memkv_node_string_t *node = (memkv_node_string_t *)memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (node) {
        if (node->vlen > (int)sizeof(char *)) {
            free(node->val.ptr);
        }
    } else {
        node = memkv_node_string_create(current_partition, req_key, req_key_len);
    }
    node->vlen = req_val_len;
    if (req_val_len <= (int)sizeof(char *)) {
        if (req_val_len) {
            memcpy(node->val.buf, req_val, req_val_len);
        }
    } else {
        node->val.ptr = memdup(req_val, req_val_len);
    }
    response_status(aio, memkv_op_result_want);
}
/* }}} */

/* {{{ op_set_int */
static void op_set_int(async_io &aio)
{
    if (!current_partition) {
        current_partition = memkv_node_partition_create(req_partition, req_partition_len);
    }
    memkv_node_int_t *node = (memkv_node_int_t *)memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (!node) {
        node = memkv_node_int_create(current_partition, req_key, req_key_len);
    }
    long val = 0;
    if (req_val_len) {
        val = atol(req_val);
    }
    node->val = val;
    response_status(aio, memkv_op_result_want);
}
/* }}} */

/* {{{ op_del */
static void op_del(async_io &aio)
{
    if (!current_partition) {
        response_status(aio, memkv_op_result_want);
        return;
    }
    memkv_node_basic_t *node = memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (!node) {
        response_status(aio, memkv_op_result_want);
        return;
    }
    memkv_node_string_t *snode;
    memkv_node_int_t *inode;
    if (node->type == memkv_node_type_string) {
        snode = (memkv_node_string_t *)node;
        if (snode->vlen > (int)sizeof(char *)) {
            free(snode->val.ptr);
        }
        memkv_node_string_free(current_partition, snode);
    } else if (node->type == memkv_node_type_int) {
        inode = (memkv_node_int_t *)node;
        memkv_node_int_free(current_partition, inode);
    } else {
    }
    response_status(aio, memkv_op_result_want);
}
/* }}} */

/* {{{ op_get */
static void op_get(async_io &aio)
{
    if (!current_partition) {
        response_status(aio, memkv_op_result_unwant);
        return;
    }
    memkv_node_basic_t *node = memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (!node) {
        response_status(aio, memkv_op_result_unwant);
        return;
    }
    memkv_node_string_t *snode;
    memkv_node_int_t *inode;
    if (node->type == memkv_node_type_string) {
        snode = (memkv_node_string_t *)node;
        if (snode->vlen > (int)sizeof(char *)) {
            response_result(aio, snode->val.ptr, snode->vlen);
        } else {
            response_result(aio, snode->val.buf, snode->vlen);
        }
    } else if (node->type == memkv_node_type_int) {
        inode = (memkv_node_int_t *)node;
        char buf[32];
        sprintf(buf, "%ld", inode->val);
        response_result(aio, buf, strlen(buf));
    } else {
        response_status(aio, memkv_op_result_error);
    }
}
/* }}} */

/* {{{ op_exists */
static void op_exists(async_io &aio)
{
    if (!current_partition) {
        response_status(aio, memkv_op_result_unwant);
        return;
    }
    memkv_node_basic_t *node = memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (!node) {
        response_status(aio, memkv_op_result_unwant);
        return;
    }
    response_status(aio, memkv_op_result_want);
}
/* }}} */

/* {{{ op_inc */
static void op_inc(async_io &aio)
{
    if (!current_partition) {
        current_partition = memkv_node_partition_create(req_partition, req_partition_len);
    }
    memkv_node_int_t *node = (memkv_node_int_t *)memkv_node_basic_find(&(current_partition->tree), req_key, req_key_len);
    if (!node) {
        node = memkv_node_int_create(current_partition, req_key, req_key_len);
        node->val = 0;
    }
    long val = 0;
    if (req_val_len) {
        val = atol(req_val);
    }
    node->val += val;
    response_result(aio, (char *)(node->val), -2);
}

static void op_clear_by_partition(memkv_node_partition_t *partition)
{
    rbtree_node_t *rbn;
    memkv_node_basic_t *node;
    memkv_node_string_t *snode;
    memkv_node_int_t *inode;
    while((rbn = rbtree_first(&(partition->tree)))) {
        node = zcc_container_of(rbn, memkv_node_basic_t, rbnode);
        if (node->type == memkv_node_type_string) {
            snode = (memkv_node_string_t *)node;
            if (snode->vlen > (int)sizeof(char *)) {
                free(snode->val.ptr);
                memkv_node_string_free(partition, snode);
            }
        } else if (node->type == memkv_node_type_int) {
            inode = (memkv_node_int_t *)node;
            memkv_node_int_free(partition, inode);
        } else {
        }
    }
    memkv_node_partition_free(partition);
}
/* }}} */

/* {{{ op_clear */
static void op_clear(async_io &aio)
{
    if (current_partition) {
        op_clear_by_partition(current_partition);
        return;
    } else {
        rbtree_node_t *rbn;
        memkv_node_partition_t *pnode;
        while((rbn = rbtree_first(&partition_tree))) {
            pnode = zcc_container_of(rbn, memkv_node_partition_t, rbnode);
            op_clear_by_partition(pnode);
        }
    }
    response_status(aio, memkv_op_result_want);
}
/* }}} */

/* {{{ request, parse */
static void after_recv_request(async_io &aio)
{
    int ret = aio.get_result();
    if (ret < 2) {
        exception_do(aio);
        return;
    }
    char opbuf[2];
    aio.fetch_rbuf(opbuf, 1);
    req_buf.clear();
    aio.fetch_rbuf(req_buf, ret - 1);
    req_buf.push_back('\0');

    size_data_t sdvec[3];
    if (size_data_unescape_all(req_buf.c_str(), req_buf.size(), sdvec, 3) < 3) {
        response_status(aio, memkv_op_result_error);
        return;
    }
    req_partition = sdvec[0].data;
    req_partition_len = (int)sdvec[0].size;
    req_partition[req_partition_len] = 0;

    req_key = sdvec[1].data;
    req_key_len = (int)sdvec[1].size;
    req_key[req_key_len] = 0;

    req_val = sdvec[2].data;
    req_val_len = (int)sdvec[2].size;
    req_val[req_val_len] = 0;

    void (*op_handler)(async_io &aio) = 0;
    switch (opbuf[0]) {
    case memkv_op_type_set:
        op_handler = op_set;
        break;
    case memkv_op_type_set_int:
        op_handler = op_set_int;
        break;
    case memkv_op_type_del:
        op_handler = op_del;
        break;
    case memkv_op_type_get:
        op_handler = op_get;
        break;
    case memkv_op_type_exists:
        op_handler = op_exists;
        break;
    case memkv_op_type_inc:
        op_handler = op_inc;
        break;
    case memkv_op_type_clear:
        op_handler = op_clear;
        break;
    default:
        response_status(aio, memkv_op_result_error);
        return;
        break;
    }
    current_partition = 0;
    if (req_partition_len) {
        current_partition = (memkv_node_partition_t *)memkv_node_basic_find(&partition_tree, req_partition, req_partition_len);
    }
    op_handler(aio);
    return;
}
/* }}} */

/* {{{ service */
memkvd::memkvd()
{
}

memkvd::~memkvd()
{
}

void memkvd::simple_service(int fd)
{
    async_io *aio = new async_io();
    aio->bind(fd);
    aio->read_size_data(after_recv_request, 0);
}

void memkvd::before_service()
{
    rbtree_init(&partition_tree, memkv_cmp);
    node_partition_pmp.option_piece_size(sizeof(memkv_node_partition_t));
    node_string_pmp.option_piece_size(sizeof(memkv_node_string_t));
    node_int_pmp.option_piece_size(sizeof(memkv_node_int_t));
}
/* }}} */

}
#pragma pack(pop)

/* Local variables:
* End:
* vim600: fdm=marker
*/

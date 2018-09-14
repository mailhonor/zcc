/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-11-27
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static const size_t defaul_single_buffer_size = 1024 * 100;

typedef struct pool_node_t pool_node_t;
struct pool_node_t {
    size_t left;
    pool_node_t *next;
};
typedef struct sys_node_t sys_node_t;
struct sys_node_t {
    sys_node_t *next;
};

gm_pool::gm_pool()
{
    ___single_buffer_size = 0;
}

gm_pool::~gm_pool()
{
    clear();
}

void gm_pool::set_buffer_size(size_t single_buffer_size)
{
    if (___single_buffer_size) {
        return;
    }
    ___single_buffer_size = single_buffer_size;
    if (single_buffer_size < 1024 * 10) {
        ___single_buffer_size = 1024 * 10;
    }
    ___single_buffer_size_10percent = ___single_buffer_size/10;

    ___head = 0;
    ___current = 0;
    ___sys_head = 0;
    ___sys_tail = 0;
}

void gm_pool::clear()
{
    if (___single_buffer_size == 0) {
        return;
    }

    {
        pool_node_t *node, *next;
        for (node = (pool_node_t *)___head; node; node=next) {
            next = node->next;
            zcc::free(node);
        }
        ___head = ___current = 0;
    }
    {
        sys_node_t *node, *next;
        for (node = (sys_node_t *)___sys_head; node; node=next) {
            next = node->next;
            zcc::free(node);
        }
        ___sys_head = ___sys_tail = 0;
    }
}

void *gm_pool::malloc(size_t size)
{
    char *r;
    sys_node_t *sn;
    pool_node_t *pn;

    if (size < 1) {
        return blank_buffer;
    }

    if (___single_buffer_size == 0) {
        set_buffer_size(defaul_single_buffer_size);
    }

    if (size > ___single_buffer_size_10percent) {
        r = (char *)zcc::malloc(sizeof(sys_node_t) + size);
        sn = (sys_node_t *) r;
        sn->next = 0;
        if (___sys_head == 0) {
            ___sys_head = ___sys_tail = (char *)sn;
        } else {
            ((sys_node_t *)___sys_tail)->next = sn;
            ___sys_tail = (char *)sn;
        }

        return (void *)(r + sizeof(sys_node_t));
    }

    if ((___current == 0) || (((pool_node_t *)___current)->left < size)) {
        pn = (pool_node_t *)zcc::malloc(sizeof(pool_node_t) + ___single_buffer_size);
        pn->next = 0;
        pn->left = ___single_buffer_size;
        if (___current == 0) {
            ___head = ___current = (char *)pn;
        } else {
            ((pool_node_t *)___current)->next = pn;
            ___current = (char *)pn;
        }
    }
    r = (char *)___current + sizeof(pool_node_t) + (___single_buffer_size - ((pool_node_t *)___current)->left);
    ((pool_node_t *)___current)->left -= size;

    return (void *)r;
}

char *gm_pool::strdup(const char *ptr)
{
    char *r;
    size_t len;

    if (!ptr) {
        return blank_buffer;
    }
    len = strlen(ptr);
    r = (char *)malloc(len+1);
    memcpy(r, ptr, len+1);

    return r;
}

char *gm_pool::strndup(const char *ptr, size_t n)
{
    size_t i, len;
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    if (!n) {
        return blank_buffer;
    }

    len = n;
    for (i = 0; i < n; i++) {
        if (ptr[i] == '\0') {
            len = i;
            break;
        }
    }
    r = memdup(ptr, len + 1);
    r[len] = 0;

    return r;
}

char *gm_pool::memdup(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    r =(char *)malloc(n);
    if (n > 0) {
        memcpy(r, ptr, n);
    }

    return r;
}

char *gm_pool::memdupnull(const void *ptr, size_t n)
{
    char *r;

    if (!ptr) {
        return blank_buffer;
    }

    r =(char *)malloc(n+1);
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

}

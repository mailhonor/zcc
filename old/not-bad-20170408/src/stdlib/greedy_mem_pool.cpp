/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-27
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static const size_t defaul_single_buffer_size = 1024 * 1024 * 1;

typedef struct pool_node_t pool_node_t;
struct pool_node_t {
    size_t left;
    pool_node_t *next;
};
typedef struct sys_node_t sys_node_t;
struct sys_node_t {
    sys_node_t *next;
};

greedy_mem_pool::greedy_mem_pool()
{
    ___single_buffer_size = 0;
}

greedy_mem_pool::~greedy_mem_pool()
{
    reset();
}

void greedy_mem_pool::init(size_t single_buffer_size)
{
    if (___single_buffer_size) {
        return;
    }
    ___single_buffer_size = single_buffer_size;
    if (single_buffer_size < 1024 * 100) {
        ___single_buffer_size = 1024 * 100;
    }
    ___single_buffer_size_10percent = ___single_buffer_size/10;

    ___head = 0;
    ___current = 0;
    ___sys_head = 0;
    ___sys_tail = 0;
}

void greedy_mem_pool::reset()
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

void *greedy_mem_pool::malloc(size_t size)
{
    char *r;
    sys_node_t *sn;
    pool_node_t *pn;

    if (size < 1) {
        return blank_buffer;
    }

    if (___single_buffer_size == 0) {
        init(defaul_single_buffer_size);
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

void greedy_mem_pool::free(const void *ptr)
{
}

void *greedy_mem_pool::realloc(const void *ptr, size_t size)
{
    log_fatal("greedy_mem_pool: unsupport realloc");

    return 0;
}

}

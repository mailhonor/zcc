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

basic_list::basic_list()
{
    ___head = 0;
    ___tail = 0;
    ___size = 0;
    ___mpool = 0;
}

basic_list::~basic_list()
{
    clear_void();
}

void basic_list::init_void(mem_pool &mpool)
{
    if (___mpool) {
        return;
    }
    ___mpool = &mpool;
}

void basic_list::clear_void()
{
    node *n, *nn;
    n = ___head;
    if (!___mpool) {
        return;
    }
    while(n) {
        nn = n->___next;
        n->~node();
        ___mpool->free(n);
        n = nn;
    }
    ___head = 0;
    ___tail = 0;
    ___size = 0;
}

void basic_list::push_void(const void * v)
{
    node *n;
    if (!___mpool) {
        init_void();
    }
    n = new(___mpool->malloc(sizeof(node)))node();
    n->___data = const_cast<void *>(v);
    ZCC_MLINK_APPEND(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::pop_void(char ** v)
{
    node *n;
    if (!___mpool) {
        return false;
    }
    n = ___tail;
    ZCC_MLINK_DETACH(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    n->~node();
    ___mpool->free(n);
    ___size --;
    return true;
}

void basic_list::unshift_void(const void * v)
{
    node *n;
    if (!___mpool) {
        init_void();
    }
    n = new(___mpool->malloc(sizeof(node)))node();
    n->___data = const_cast<void *>(v);
    ZCC_MLINK_PREPEND(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::shift_void(char ** v)
{
    node *n;
    if (!___mpool) {
        return false;
    }
    n = ___head;
    ZCC_MLINK_DETACH(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    n->~node();
    ___mpool->free(n);
    ___size --;
    return true;
}

}

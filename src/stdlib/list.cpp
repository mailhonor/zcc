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
    ___gmp = 0;
}

basic_list::~basic_list()
{
    clear_void();
}

void basic_list::option_gm_pool_void(gm_pool &gmp)
{
    ___gmp = &gmp;
}

void basic_list::clear_void()
{
    node *n, *nn;
    n = ___head;
    while(n) {
        nn = n->___next;
        n->~node();
        if (!___gmp) {
            zcc::free(n);
        }
        n = nn;
    }
    ___head = 0;
    ___tail = 0;
    ___size = 0;
}

void basic_list::push_void(const void * v)
{
    node *n;
    n = new(___gmp?___gmp->malloc(sizeof(node)):zcc::malloc(sizeof(node)))node();
    n->___data = const_cast<void *>(v);
    zcc_mlink_append(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::pop_void(char ** v)
{
    node *n;
    n = ___tail;
    zcc_mlink_detach(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    n->~node();
    if (!___gmp) {
        zcc::free(n);
    }
    ___size --;
    return true;
}

void basic_list::unshift_void(const void * v)
{
    node *n;
    n = new(___gmp?___gmp->malloc(sizeof(node)):zcc::malloc(sizeof(node)))node();
    n->___data = const_cast<void *>(v);
    zcc_mlink_prepend(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::shift_void(char ** v)
{
    node *n;
    n = ___head;
    zcc_mlink_detach(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    n->~node();
    if (!___gmp) {
        zcc::free(n);
    }
    ___size --;
    return true;
}

}

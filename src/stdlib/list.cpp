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
}

basic_list::~basic_list()
{
    clear_void();
}

void basic_list::clear_void()
{
    node *n, *nn;
    n = ___head;
    while(n) {
        nn = n->___next;
        delete n;
        n = nn;
    }
    ___head = 0;
    ___tail = 0;
    ___size = 0;
}

void basic_list::push_void(const void * v)
{
    node *n;
    n = new node();
    n->___data = const_cast<void *>(v);
    zcc_mlink_append(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::pop_void(char ** v)
{
    node *n;
    n = ___tail;
    if (!n) {
        return false;
    }
    zcc_mlink_detach(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    delete n;
    ___size --;
    return true;
}

void basic_list::unshift_void(const void * v)
{
    node *n;
    n = new node();
    n->___data = const_cast<void *>(v);
    zcc_mlink_prepend(___head, ___tail, n, ___prev, ___next);
    ___size ++;
}

bool basic_list::shift_void(char ** v)
{
    node *n;
    n = ___head;
    if (!n) {
        return false;
    }
    zcc_mlink_detach(___head, ___tail, n, ___prev, ___next);
    if (v) {
        *v = (char *)(n->___data);
    }
    delete n;
    ___size --;
    return true;
}

void basic_list::erase_void(node *n)
{
    if (!n) {
        return;
    }
    zcc_mlink_detach(___head, ___tail, n, ___prev, ___next);
    delete n;
    ___size --;
}

}

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-10-15
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class basic_list
{
public:
    class node {
    public:
        inline node() {}
        inline ~node() {}
        inline void *data() { return ___data; }
        inline node *prev() { return ___prev; }
        inline node *next() { return ___next; }
        void *___data;
        node *___prev;
        node *___next;
    };
public:
    basic_list();
    ~basic_list();
    void clear_void();
    void push_void(const void * v);
    bool pop_void(char **v);
    void unshift_void(const void * v);
    bool shift_void(char **v);
    void erase_void(node *n);
    node *___head;
    node *___tail;
    unsigned int ___size;
};
typedef basic_list::node list_node;
template <typename T>
class list
{
};

template <typename T>
class list<T *>: public basic_list
{
public:
    inline list() {}
    inline ~list() {}
    inline size_t size() { return ___size; }
    inline void clear() { clear_void(); }
    inline void push(const T * v) { push_void((const void *)v); }
    inline void push_back(const T * v) { push_void((const void *)v); }
    inline bool pop(T **v) { return pop_void((char **)v); }
    inline void unshift(const T * v) { push_void((const void *)v); }
    inline bool shift(T **v) { return shift_void((char **)v); }
    inline void erase(node *n) { return erase_void(n); }
    inline node *first_node() { return ___head; }
    inline node *last_node() { return ___tail; }
    inline T* template_type() const { return 0; }
};

#define zcc_list_walk_begin(var_your_list, var_your_ptr) { \
    typeof(var_your_list) &___V_LIST = (var_your_list); \
    typeof(___V_LIST.template_type()) var_your_ptr; \
    typeof(___V_LIST.first_node()) var_zcc_list_node = ___V_LIST.first_node(); \
    typeof(___V_LIST.first_node()) var_zcc_list_node_next; \
    for (; var_zcc_list_node; var_zcc_list_node = var_zcc_list_node_next) { \
        var_zcc_list_node_next = var_zcc_list_node->next(); \
        var_your_ptr = (typeof(___V_LIST.template_type())) (var_zcc_list_node->data()); {
#define zcc_list_walk_end }}}

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



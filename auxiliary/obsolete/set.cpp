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

class set
{
public:
    struct node_t {
        char *key;
        rbtree_node_t rbnode;
    };
    typedef struct node_t node_t;
    class node {
    public:
        inline node() {}
        inline ~node() {}
        inline char *key() { return ___data.key; }
        node *prev();
        node *next();
    private:
        node_t ___data;
    };
public:
    set();
    ~set();
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    node *enter(const char *key);
    bool exists(const char *key) { return find(key)?true:false; }
    void erase(const char *key);
    void erase(node *n);
    node *find(const char *key);
    node *find_near_prev(const char *key);
    node *find_near_next(const char *key);
    node *first_node();
    node *last_node();
private:
    rbtree_t ___rbtree;
    unsigned int ___size;
};

#define zcc_set_walk_begin(var_your_set, var_your_key_ptr) {\
    typeof(var_your_set) &___V_SET = (var_your_set); \
    char * var_your_key_ptr; \
    typeof(___V_SET.first_node()) var_zcc_set_node = ___V_SET.first_node(); \
    typeof(___V_SET.first_node()) var_zcc_set_node_next; \
    for (; var_zcc_set_node; var_zcc_set_node = var_zcc_set_node_next) { \
        var_zcc_set_node_next = var_zcc_set_node->next(); \
        var_your_key_ptr = var_zcc_set_node->key(); {
#define zcc_set_walk_end }}}

set::node *set::node::prev()
{
    rbtree_node_t *rn = rbtree_prev(&(___data.rbnode));
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

set::node *set::node::next()
{
    rbtree_node_t *rn = rbtree_next(&(___data.rbnode));
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

/* **************************************************************************** */
static int ___cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    set::node_t *dn1, *dn2;

    dn1 = (set::node_t *)(zcc_container_of(n1, set::node_t, rbnode));
    dn2 = (set::node_t *)(zcc_container_of(n2, set::node_t, rbnode));

    return strcmp(dn1->key, dn2->key);
}

set::set()
{
    rbtree_init(&___rbtree, ___cmp);
    ___size = 0;
}

set::~set()
{
    node *n;
    while ((n=first_node())) {
        erase(n);
    }
}

set::node *set::enter(const char *key)
{
    node_t a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.key = const_cast<char *>(key);
    r_n = rbtree_attach(&___rbtree, &(a_n.rbnode));

    if (r_n != &(a_n.rbnode)) {
        a_np = zcc_container_of(r_n, node_t, rbnode);
    } else {
        a_np = (node_t *) (new node());
        a_np->key = strdup(key);
        rbtree_replace_node(&___rbtree, &(a_n.rbnode), &(a_np->rbnode));
        ___size++;
    }
    return (node *)a_np;
}

void set::erase(const char *key)
{
    node *result_n = find(key);
    if (result_n) {
        erase(result_n);
    }
}

void set::erase(node *n)
{
    node_t *nt = (node_t *)n;
    rbtree_detach(&___rbtree, &(nt->rbnode));
    free(nt->key);
    delete(n);
}

set::node *set::find(const char *key)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_find(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        return (node *)result_n;
    }

    return 0;
}

set::node *set::find_near_prev(const char *key)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        return (node *)result_n;
    }

    return 0;
}

set::node *set::find_near_next(const char *key)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        return (node *)result_n;
    }

    return 0;
}

set::node *set::first_node()
{
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

set::node *set::last_node()
{
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

}

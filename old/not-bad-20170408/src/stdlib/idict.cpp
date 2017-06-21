/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-06
 * ================================
 */

#include "zcc.h"

namespace zcc
{

static int ___rbnode_offset = -1;
static int ___cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    basic_idict::node *dn1, *dn2;

    dn1 = (basic_idict::node *)((char *)n1 - ___rbnode_offset);
    dn2 = (basic_idict::node *)((char *)n2 - ___rbnode_offset);

    return (dn1->key() - dn2->key());
}

basic_idict::basic_idict()
{
    basic_idict::node *n;
    if (___rbnode_offset == -1) {
        n = new basic_idict::node;
        ___rbnode_offset = n->get_rbnode_offset();
        delete n;
    }
    ___mpool = 0;
    ___size = 0;
    rbtree_init(&___rbtree, ___cmp);
}

basic_idict::~basic_idict()
{
    clear();
}

void basic_idict::init(mem_pool &mpool)
{
    if (___mpool == 0) {
        ___mpool = &mpool;
    }
}

void basic_idict::reset()
{
    clear();
}

void basic_idict::clear()
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node *n;
    while ((n=first_node())) {
        erase(n);
    }
}

basic_idict::node *basic_idict::___update_str(long key, const char *value, size_t len)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.___set_key(key);
    r_n = rbtree_attach(&___rbtree, a_n.get_rbnode());

    if (r_n != a_n.get_rbnode()) {
        a_np = ZCC_CONTAINER_OF2(r_n, node, ___rbnode_offset);
        ___update_str(a_np, value, len);
    } else {
        a_np = new (___mpool->malloc(sizeof(node))) node();
        a_np->___set_key(key);
        ___mpool->free(a_np->value());
        if ((int)len == -1) {
            value = ___mpool->strdup(value);
        } else {
            value = ___mpool->strndup(value, len);
        }
        a_np->___set_value(value);
        rbtree_replace_node(&___rbtree, a_n.get_rbnode(), a_np->get_rbnode());
        ___size++;
    }
    return a_np;
}

basic_idict::node *basic_idict::___update(long key, const void *value)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.___set_key(key);
    r_n = rbtree_attach(&___rbtree, a_n.get_rbnode());

    if (r_n != a_n.get_rbnode()) {
        a_np = ZCC_CONTAINER_OF2(r_n, node, ___rbnode_offset);
        ___update(a_np, value);
    } else {
        a_np = new (___mpool->malloc(sizeof(node))) node();
        a_np->___set_key(key);
        a_np->___set_value(value);
        rbtree_replace_node(&___rbtree, a_n.get_rbnode(), a_np->get_rbnode());
        ___size++;
    }
    return a_np;
}

void basic_idict::___update_str(node *n, const void *value, size_t len)
{
    char *v;
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    ___mpool->free(n->value());
    if ((int)len == -1) {
        v = ___mpool->strdup((char *)value);
    } else {
        v = ___mpool->strndup((char *)value, len);
    }
    n->___set_value(v);
}

void basic_idict::___update(node *n, const void *value)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    n->___set_value(value);
}

void basic_idict::erase(long key)
{
    node *result_n = find(key);
    if (!result_n) {
        return;
    }
    return erase(result_n);
}

void basic_idict::erase(node *n)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    rbtree_detach(&___rbtree, n->get_rbnode());
    if (___str) {
        ___mpool->free(n->value());
    }
    n->~node();
    ___mpool->free(n);
}

basic_idict::node *basic_idict::find(long key, char **value)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.___set_key(key);
    result_rbnode_n = rbtree_find(&___rbtree, virtual_n.get_rbnode());
    if (result_rbnode_n) {
        result_n = ZCC_CONTAINER_OF2(result_rbnode_n, node, ___rbnode_offset);
        if (value) {
            *value = result_n->value();
        }
        return result_n;
    }

    return 0;
}

basic_idict::node *basic_idict::find_near_prev(long key, char **value)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.___set_key(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, virtual_n.get_rbnode());
    if (result_rbnode_n) {
        result_n = ZCC_CONTAINER_OF2(result_rbnode_n, node, ___rbnode_offset);
        if (value) {
            *value = result_n->value();
        }
        return result_n;
    }

    return 0;
}

basic_idict::node *basic_idict::find_near_next(long key, char **value)
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    node virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.___set_key(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, virtual_n.get_rbnode());
    if (result_rbnode_n) {
        result_n = ZCC_CONTAINER_OF2(result_rbnode_n, node, ___rbnode_offset);
        if (value) {
            *value = result_n->value();
        }
        return result_n;
    }

    return 0;
}

basic_idict::node *basic_idict::first_node()
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return ZCC_CONTAINER_OF2(rn, node, ___rbnode_offset);
    }
    return 0;
}

basic_idict::node *basic_idict::last_node()
{
    if (___mpool == 0) {
        ___mpool = &system_mem_pool_instance;
    }
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return ZCC_CONTAINER_OF2(rn, node, ___rbnode_offset);
    }
    return 0;
}

basic_idict::node::node()
{
}

basic_idict::node::~node()
{
}

basic_idict::node *basic_idict::node::prev()
{
    rbtree_node_t *rn = rbtree_prev(get_rbnode());
    if (rn) {
        return ZCC_CONTAINER_OF2(rn, node, ___rbnode_offset);
    }
    return 0;
}

basic_idict::node *basic_idict::node::next()
{
    rbtree_node_t *rn = rbtree_next(get_rbnode());
    if (rn) {
        return ZCC_CONTAINER_OF2(rn, node, ___rbnode_offset);
    }
    return 0;
}

/* **************************************************************************** */
void str_idict::debug_show()
{
    for (str_idict::node *n = first_node();n;n=n->next()) {
        printf("%zd = %s\n", n->key(), n->value());
    }
}

}

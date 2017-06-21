/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-14
 * ================================
 */

#include "zcc.h"

namespace zcc
{

grid::node *grid::node::prev()
{
    rbtree_node_t *rn = rbtree_prev(&(___data.rbnode));
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

grid::node *grid::node::next()
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
    grid::node_t *dn1, *dn2;

    dn1 = (grid::node_t *)(zcc_container_of(n1, grid::node_t, rbnode));
    dn2 = (grid::node_t *)(zcc_container_of(n2, grid::node_t, rbnode));

    return strcmp(dn1->key, dn2->key);
}

grid::grid()
{
    rbtree_init(&___rbtree, ___cmp);
    ___size = 0;
    ___gmp = 0;
}

grid::~grid()
{
    node *n;
    while ((n=first_node())) {
        erase(n, 0);
    }
}

void grid::option_gm_pool(gm_pool &gmp)
{
    ___gmp = &gmp;
}

grid::node *grid::update(const char *key, const void *value, void **old_value)
{
    node_t a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.key = const_cast<char *>(key);
    r_n = rbtree_attach(&___rbtree, &(a_n.rbnode));

    if (r_n != &(a_n.rbnode)) {
        a_np = zcc_container_of(r_n, node_t, rbnode);
    } else {
        if (___gmp) {
            a_np = (node_t *) (new(___gmp->calloc(1, sizeof(node)))node());
        } else {
            a_np = (node_t *) (new node());
        }
        if (___gmp) {
            a_np->key = ___gmp->strdup(key);
        } else {
            a_np->key = strdup(key);
        }
        a_np->value = 0;
        rbtree_replace_node(&___rbtree, &(a_n.rbnode), &(a_np->rbnode));
        ___size++;
    }
    update((node *)a_np, value, old_value);
    return (node *)a_np;
}

void grid::update(node *n, const void *value, void **old_value)
{
    node_t *nt = (node_t *)n;
    if (old_value) {
        *old_value = nt->value;
    }
    nt->value = const_cast<void *>(value);
}

void grid::erase(const char *key, void **old_value)
{
    node *result_n = find(key);
    if (result_n) {
        erase(result_n, old_value);
    }
}

void grid::erase(node *n, void **old_value)
{
    node_t *nt = (node_t *)n;
    rbtree_detach(&___rbtree, &(nt->rbnode));
    if (old_value) {
        nt->value = const_cast<void *>(nt->value);
    }
    if (___gmp == 0) {
        free(nt->key);
        delete(n);
    }
}

grid::node *grid::find(const char *key, void **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_find(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (node *)result_n;
    }

    return 0;
}

grid::node *grid::find_near_prev(const char *key, void **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (node *)result_n;
    }

    return 0;
}

grid::node *grid::find_near_next(const char *key, void **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (node *)result_n;
    }

    return 0;
}

grid::node *grid::first_node()
{
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

grid::node *grid::last_node()
{
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

}

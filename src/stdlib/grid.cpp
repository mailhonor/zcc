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

basic_grid_node *basic_grid_node::prev_void()
{
    rbtree_node_t *rn = rbtree_prev(&(___data.rbnode));
    if (rn) {
        return (basic_grid_node *)zcc_container_of(rn, basic_grid_node_t, rbnode);
    }
    return 0;
}

basic_grid_node *basic_grid_node::next_void()
{
    rbtree_node_t *rn = rbtree_next(&(___data.rbnode));
    if (rn) {
        return (basic_grid_node *)zcc_container_of(rn, basic_grid_node_t, rbnode);
    }
    return 0;
}

/* **************************************************************************** */
static int ___cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    basic_grid_node_t *dn1, *dn2;

    dn1 = (basic_grid_node_t *)(zcc_container_of(n1, basic_grid_node_t, rbnode));
    dn2 = (basic_grid_node_t *)(zcc_container_of(n2, basic_grid_node_t, rbnode));

    return strcmp(dn1->key, dn2->key);
}

static int ___cmp_long(rbtree_node_t * n1, rbtree_node_t * n2)
{
    basic_grid_node_t *dn1, *dn2;

    dn1 = (basic_grid_node_t *)(zcc_container_of(n1, basic_grid_node_t, rbnode));
    dn2 = (basic_grid_node_t *)(zcc_container_of(n2, basic_grid_node_t, rbnode));

    return ((long)dn1->key - (long)dn2->key);
}

basic_grid::basic_grid()
{
    rbtree_init(&___rbtree, ___cmp);
    ___long_flag = 0;
    ___size = 0;
    ___gmp = 0;
}

basic_grid::~basic_grid()
{
    basic_grid_node *n;
    while ((n=first_node_void())) {
        erase_void(n, 0);
    }
}

void basic_grid::option_gm_pool_void(gm_pool &gmp)
{
    ___gmp = &gmp;
}

void basic_grid::option_long()
{
    ___long_flag = 1;
    rbtree_init(&___rbtree, ___cmp_long);
}

void basic_grid::clear_void()
{
    basic_grid_node *n;
    while ((n=first_node_void())) {
        erase_void(n, 0);
    }
}

basic_grid_node *basic_grid::update_void(const char *key, const void *value, void **old_value)
{
    basic_grid_node_t a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.key = const_cast<char *>(key);
    r_n = rbtree_attach(&___rbtree, &(a_n.rbnode));

    if (r_n != &(a_n.rbnode)) {
        a_np = zcc_container_of(r_n, basic_grid_node_t, rbnode);
    } else {
        if (___gmp) {
            a_np = (basic_grid_node_t *) (new(___gmp->calloc(1, sizeof(basic_grid_node)))basic_grid_node());
        } else {
            a_np = (basic_grid_node_t *) (new basic_grid_node());
        }
        if (___long_flag) {
            a_np->key = const_cast<char*>(key);
        }else {
            if (___gmp) {
                a_np->key = ___gmp->strdup(key);
            } else {
                a_np->key = strdup(key);
            }
        }
        a_np->value = 0;
        rbtree_replace_node(&___rbtree, &(a_n.rbnode), &(a_np->rbnode));
        ___size++;
    }
    update_void((basic_grid_node *)a_np, value, old_value);
    return (basic_grid_node *)a_np;
}

void basic_grid::update_void(basic_grid_node *n, const void *value, void **old_value)
{
    basic_grid_node_t *nt = (basic_grid_node_t *)n;
    if (old_value) {
        *old_value = nt->value;
    }
    nt->value = const_cast<void *>(value);
}

void basic_grid::erase_void(const char *key, void **old_value)
{
    basic_grid_node *result_n = find_void(key);
    if (result_n) {
        erase_void(result_n, old_value);
    }
}

void basic_grid::erase_void(basic_grid_node *n, void **old_value)
{
    basic_grid_node_t *nt = (basic_grid_node_t *)n;
    rbtree_detach(&___rbtree, &(nt->rbnode));
    if (old_value) {
        nt->value = const_cast<void *>(nt->value);
    }
    if (___gmp == 0) {
        if (___long_flag) {
        }else {
            free(nt->key);
        }
        delete(n);
    } else {
        n->~basic_grid_node();
    }
}

basic_grid_node *basic_grid::find_void(const char *key, void **value)
{
    basic_grid_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_find(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_grid_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_grid_node *)result_n;
    }

    return 0;
}

basic_grid_node *basic_grid::find_near_prev_void(const char *key, void **value)
{
    basic_grid_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_grid_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_grid_node *)result_n;
    }

    return 0;
}

basic_grid_node *basic_grid::find_near_next_void(const char *key, void **value)
{
    basic_grid_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_grid_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_grid_node *)result_n;
    }

    return 0;
}

basic_grid_node *basic_grid::first_node_void()
{
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return (basic_grid_node *)zcc_container_of(rn, basic_grid_node_t, rbnode);
    }
    return 0;
}

basic_grid_node *basic_grid::last_node_void()
{
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return (basic_grid_node *)zcc_container_of(rn, basic_grid_node_t, rbnode);
    }
    return 0;
}

}

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

basic_map_node *basic_map_node::prev_void()
{
    rbtree_node_t *rn = rbtree_prev(&(___data.rbnode));
    if (rn) {
        return (basic_map_node *)zcc_container_of(rn, basic_map_node_t, rbnode);
    }
    return 0;
}

basic_map_node *basic_map_node::next_void()
{
    rbtree_node_t *rn = rbtree_next(&(___data.rbnode));
    if (rn) {
        return (basic_map_node *)zcc_container_of(rn, basic_map_node_t, rbnode);
    }
    return 0;
}

/* **************************************************************************** */
static int ___cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    basic_map_node_t *dn1, *dn2;

    dn1 = (basic_map_node_t *)(zcc_container_of(n1, basic_map_node_t, rbnode));
    dn2 = (basic_map_node_t *)(zcc_container_of(n2, basic_map_node_t, rbnode));

    return strcmp(dn1->key, dn2->key);
}

static int ___cmp_long(rbtree_node_t * n1, rbtree_node_t * n2)
{
    basic_map_node_t *dn1, *dn2;

    dn1 = (basic_map_node_t *)(zcc_container_of(n1, basic_map_node_t, rbnode));
    dn2 = (basic_map_node_t *)(zcc_container_of(n2, basic_map_node_t, rbnode));

    return ((long)dn1->key - (long)dn2->key);
}

basic_map::basic_map()
{
    rbtree_init(&___rbtree, ___cmp);
    ___long_flag = 0;
    ___size = 0;
}

basic_map::~basic_map()
{
    basic_map_node *n;
    while ((n=first_node_void())) {
        erase_void(n, 0);
    }
}

void basic_map::option_long()
{
    ___long_flag = 1;
    rbtree_init(&___rbtree, ___cmp_long);
}

void basic_map::clear_void()
{
    basic_map_node *n;
    while ((n=first_node_void())) {
        erase_void(n, 0);
    }
}

basic_map_node *basic_map::update_void(const char *key, const void *value, void **old_value)
{
    basic_map_node_t a_n, *a_np;
    rbtree_node_t *r_n;

    a_n.key = const_cast<char *>(key);
    r_n = rbtree_attach(&___rbtree, &(a_n.rbnode));

    if (r_n != &(a_n.rbnode)) {
        a_np = zcc_container_of(r_n, basic_map_node_t, rbnode);
    } else {
        a_np = (basic_map_node_t *) (new basic_map_node());
        if (___long_flag) {
            a_np->key = const_cast<char*>(key);
        }else {
            a_np->key = strdup(key);
        }
        a_np->value = 0;
        rbtree_replace_node(&___rbtree, &(a_n.rbnode), &(a_np->rbnode));
        ___size++;
    }
    update_void((basic_map_node *)a_np, value, old_value);
    return (basic_map_node *)a_np;
}

void basic_map::update_void(basic_map_node *n, const void *value, void **old_value)
{
    basic_map_node_t *nt = (basic_map_node_t *)n;
    if (old_value) {
        *old_value = nt->value;
    }
    nt->value = const_cast<void *>(value);
}

void basic_map::erase_void(const char *key, void **old_value)
{
    basic_map_node *result_n = find_void(key);
    if (result_n) {
        erase_void(result_n, old_value);
    }
}

void basic_map::erase_void(basic_map_node *n, void **old_value)
{
    basic_map_node_t *nt = (basic_map_node_t *)n;
    rbtree_detach(&___rbtree, &(nt->rbnode));
    if (old_value) {
        nt->value = const_cast<void *>(nt->value);
    }
    if (___long_flag) {
    }else {
        free(nt->key);
    }
    delete(n);
}

basic_map_node *basic_map::find_void(const char *key, void **value)
{
    basic_map_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_find(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_map_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_map_node *)result_n;
    }

    return 0;
}

basic_map_node *basic_map::find_near_prev_void(const char *key, void **value)
{
    basic_map_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_map_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_map_node *)result_n;
    }

    return 0;
}

basic_map_node *basic_map::find_near_next_void(const char *key, void **value)
{
    basic_map_node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, basic_map_node_t, rbnode);
        if (value) {
            *value = const_cast<void *>(result_n->value);
        }
        return (basic_map_node *)result_n;
    }

    return 0;
}

basic_map_node *basic_map::first_node_void()
{
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return (basic_map_node *)zcc_container_of(rn, basic_map_node_t, rbnode);
    }
    return 0;
}

basic_map_node *basic_map::last_node_void()
{
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return (basic_map_node *)zcc_container_of(rn, basic_map_node_t, rbnode);
    }
    return 0;
}

}

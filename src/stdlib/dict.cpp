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

dict::node *dict::node::prev()
{
    rbtree_node_t *rn = rbtree_prev(&(___data.rbnode));
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

dict::node *dict::node::next()
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
    dict::node_t *dn1, *dn2;

    dn1 = (dict::node_t *)(zcc_container_of(n1, dict::node_t, rbnode));
    dn2 = (dict::node_t *)(zcc_container_of(n2, dict::node_t, rbnode));

    return strcmp(dn1->key, dn2->key);
}

dict::dict()
{
    rbtree_init(&___rbtree, ___cmp);
    ___size = 0;
    ___gmp = 0;
}

dict::~dict()
{
    clear();
}

void dict::option_gm_pool(gm_pool &gmp)
{
    ___gmp = &gmp;
}

void dict::reset()
{
    clear();
}

void dict::clear()
{
    node *n;
    while ((n=first_node())) {
        erase(n);
    }
}

dict::node *dict::update(const char *key, const char *value, size_t len)
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
    update((node *)a_np, value, len);
    return (node *)a_np;
}

void dict::update(node *n, const char *value, size_t len)
{
    node_t *nt = (node_t *)n;
    if (nt->value) {
        if (___gmp == 0) {
            free(nt->value);
        }
    }
    if (len == var_size_max) {
        if (___gmp) {
            nt->value = ___gmp->strdup(value);
        } else {
            nt->value = strdup(value);
        }
    } else {
        if (___gmp) {
            nt->value = ___gmp->memdupnull(value, len);
        } else {
            nt->value = memdupnull(value, len);
        }
    }
}

void dict::erase(const char *key)
{
    node *result_n = find(key);
    if (result_n) {
        erase(result_n);
    }
}

void dict::erase(node *n)
{
    node_t *nt = (node_t *)n;
    rbtree_detach(&___rbtree, &(nt->rbnode));
    if (___gmp == 0) {
        free(nt->key);
        free(nt->value);
        delete(n);
    }
}

dict::node *dict::find(const char *key, char **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_find(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = result_n->value;
        }
        return (node *)result_n;
    }

    return 0;
}

dict::node *dict::find_near_prev(const char *key, char **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_prev(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = result_n->value;
        }
        return (node *)result_n;
    }

    return 0;
}

dict::node *dict::find_near_next(const char *key, char **value)
{
    node_t virtual_n, *result_n;
    rbtree_node_t *result_rbnode_n;

    virtual_n.key = const_cast<char *>(key);
    result_rbnode_n = rbtree_near_next(&___rbtree, &(virtual_n.rbnode));
    if (result_rbnode_n) {
        result_n = zcc_container_of(result_rbnode_n, node_t, rbnode);
        if (value) {
            *value = result_n->value;
        }
        return (node *)result_n;
    }

    return 0;
}

dict::node *dict::first_node()
{
    rbtree_node_t *rn = rbtree_first(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

dict::node *dict::last_node()
{
    rbtree_node_t *rn = rbtree_last(&___rbtree);
    if (rn) {
        return (node *)zcc_container_of(rn, node_t, rbnode);
    }
    return 0;
}

/* **************************************************************************** */
void dict::debug_show()
{
    for (dict::node *n = first_node();n;n=n->next()) {
        printf("%s = %s\n", n->key(), n->value());
    }
}

/* **************************************************************************** */
char *dict::get_str(const char *key, const char *def)
{
    char *v;
    if (find(key, &v)) {
        return v;
    }
    return const_cast<char *>(def);
}


bool dict::get_bool(const char *key, bool def)
{
    return to_bool(get_str(key, ""), def);
}

int dict::get_int(const char *key, int def, int min, int max)
{
    int r = atoi(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_long(const char *key, long def, long min, long max)
{
    long r = atol(get_str(key, ""));
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_second(const char *key, long def, long min, long max)
{
    int r = to_second(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

long dict::get_size(const char *key, long def, long min, long max)
{
    int r = to_size(get_str(key, ""), def);
    if ((r < min) || (r > max)) {
        return def;
    }
    return r;
}

}

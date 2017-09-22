/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-14
 * ================================
 */

#include "zcc.h"
#include <ctype.h>

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
}

dict::~dict()
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
        a_np = (node_t *) (new node());
        a_np->key = strdup(key);
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
        free(nt->value);
    }
    if (len == var_size_max) {
        nt->value = strdup(value);
    } else {
        nt->value = memdupnull(value, len);
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
    free(nt->key);
    free(nt->value);
    delete(n);
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
        debug_kv_show(n->key(), n->value());
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

void dict::parse_url_query(const char *query)
{
    char *q, *p, *ps = const_cast<char *>(query);
    string name(32, 0), value(128, 0);
    while(1) {
        p = ps;
        while((*p != '\0') && (*p != '&')) {
            p++;
        }
        do {
            q = ps;
            while(q < p) {
                if (*q  == '=') {
                    break;
                }
                q ++ ;
            }
            if (q == p) {
                break;
            }
            name.clear();
            name.append(ps, q - ps);
            tolower(name.c_str());
            value.clear();
            q ++;
            url_hex_decode(q, p - q, value);
        } while(0);
        update(name.c_str(), value.c_str(), value.size());
        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
}

char *dict::build_url_query(string &query, bool strict)
{
    bool first = true;
    for (dict::node *n = first_node();n;n=n->next()) {
        if (first) {
            first = false;
        } else {
            query.push_back('&');
        }
        query.append(n->key());
        query.push_back('&');
        char *v = n->value();
        size_t i, len = strlen(v);
        for (i = 0; i < len; i++) {
            unsigned char ch = v[i];
            if (ch == ' ') {
                query.push_back('+');
                continue;
            }
            if (isalnum(ch)) {
                query.push_back(ch);
                continue;
            }
            if (strict) {
                query.push_back('%');
                query.push_back(ch>>4);
                query.push_back(ch&0X0F);
                continue;
            } 
            if (ch > 127) {
                query.push_back(ch);
                continue;
            }
            if (strchr("._-", ch)) {
                query.push_back(ch);
                continue;
            }
            query.push_back('%');
            query.push_back(ch>>4);
            query.push_back(ch&0X0F);
        }
    }
    return (char *)(void *)query.c_str();
}

}

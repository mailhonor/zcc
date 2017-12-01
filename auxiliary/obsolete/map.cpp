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

struct basic_map_node_t {
    char *key;
    void *value;
    rbtree_node_t rbnode;
};
typedef struct basic_map_node_t basic_map_node_t;
class basic_map_node {
public:
    inline basic_map_node() {}
    inline ~basic_map_node() {}
    inline char *key_void() { return ___data.key; }
    inline void *value_void() { return ___data.value; }
    inline void set_value_void(const void *v) { ___data.value = (void *)(char *)v; }
    basic_map_node *prev_void();
    basic_map_node *next_void();
private:
    basic_map_node_t ___data;
};
class basic_map
{
public:
    basic_map();
    ~basic_map();
    inline size_t size_void() { return ___size; }
    inline size_t length_void() { return ___size; }
    void clear_void();
    basic_map_node *update_void(const char *key, const void *value, void **old_value = 0);
    void update_void(basic_map_node *n, const void *value, void **old_value = 0);
    bool exists_void(const char *key) { return find_void(key)?true:false; }
    void erase_void(const char *key, void **old_value = 0);
    void erase_void(basic_map_node *n, void **old_value);
    basic_map_node *find_void(const char *key, void **value=0);
    basic_map_node *find_near_prev_void(const char *key, void **value=0);
    basic_map_node *find_near_next_void(const char *key, void **value=0);
    basic_map_node *first_node_void();
    basic_map_node *last_node_void();
    void option_long();
private:
    unsigned char ___long_flag:1;
    unsigned int ___size:31;
    rbtree_t ___rbtree;
};

template <typename T>
class map
{
};

template <typename T>
class map<T *>: private basic_map
{
public:
    class node: public basic_map_node
    {
    public:
        inline node() {}
        inline ~node() {}
        inline char * key() { return key_void(); }
        inline T * value() { return (T *)(value_void()); }
        inline void set_value(const T *v) { set_value_void((void *)v); }
        inline node *prev() { return (node *)(prev_void()); }
        inline node *next() { return (node *)(next_void()); }
    };
public:
    inline map() {}
    inline ~map() {}
    inline size_t size() { return size_void(); }
    inline size_t length() { return size_void; }
    inline bool exists(const char *key) { return find_void(key)?true:false; }
    inline void clear() { clear_void(); }
    inline node *update(const char *key, const void *value, T **old_value = 0) {
        return (node *)update_void(key, value, (void **)old_value);
    }
    inline void update(node *n, const void *value, T **old_value = 0) {
        update_void((basic_map_node *)n, value, (void **)old_value);
    }
    inline void erase(const char *key, T **old_value = 0) { erase_void(key, (void **)old_value); }
    inline void erase(node *n, T **old_value) { erase_void((basic_map_node *)n, (void **)old_value); }
    inline node *find(const char *key, T **value=0) { return (node *)find_void(key, (void **)value); }
    inline node *find_near_prev(const char *key, T **value=0) {
        return (node *)find_near_prev_void(key, (void **)value);
    }
    inline node *find_near_next(const char *key, T **value=0) {
        return (node *)find_near_next_void(key, (void **)value);
    }
    inline node *first_node() { return (node *)first_node_void(); }
    inline node *last_node() { return (node *)last_node_void(); }
};

#define zcc_map_walk_begin(var_your_map, var_your_key_ptr, var_your_value_ptr) { \
    typeof(var_your_map) &___V_GRID = (var_your_map); \
    typeof(___V_GRID.first_node()->value()) var_your_value_ptr; \
    typeof(___V_GRID.first_node()) var_zcc_map_node = ___V_GRID.first_node(); \
    typeof(___V_GRID.first_node()) var_zcc_map_node_next; \
    for (; var_zcc_map_node; var_zcc_map_node = var_zcc_map_node_next) { \
        var_zcc_map_node_next = var_zcc_map_node->next(); \
        char * var_your_key_ptr = var_zcc_map_node->key(); (void)var_your_key_ptr; \
        var_your_value_ptr = (typeof(___V_GRID.first_node()->value()))var_zcc_map_node->value(); \
        (void) var_your_value_ptr; {
#define zcc_map_walk_end }}}

template <typename T>
class lmap
{
};

template <typename T>
class lmap<T *>: private basic_map
{
public:
    class node: public basic_map_node
    {
    public:
        inline node() {}
        inline ~node() {}
        inline long key() { return (long)(key_void()); }
        inline T * value() { return (T *)(value_void()); }
        inline node *prev() { return (node *)(prev_void()); }
        inline node *next() { return (node *)(next_void()); }
    };
public:
    inline lmap() {option_long();}
    inline ~lmap() {}
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    inline bool exists(long key) { return find_void(key)?true:false; }
    inline void clear() { clear_void(); }
    inline node *update(long key, const void *value, T **old_value = 0) {
        return (node *)update_void((char *)key, value, (void **)old_value);
    }
    inline void update(node *n, const void *value, T **old_value = 0) {
        update_void((basic_map_node *)n, value, (void **)old_value);
    }
    inline void erase(long key, T **old_value = 0) { erase_void((char *)key, (void **)old_value); }
    inline void erase(node *n, T **old_value) { erase_void((basic_map_node *)n, (void **)old_value); }
    inline node *find(long key, T **value=0) { return (node *)find_void((char *)key, (void **)value); }
    inline node *find_near_prev(long key, T **value=0) {
        return (node *)find_near_prev_void((char *)key, (void **)value);
    }
    inline node *find_near_next(long key, T **value=0) {
        return (node *)find_near_next_void((char *)key, (void **)value);
    }
    inline node *first_node() { return (node *)first_node_void(); }
    inline node *last_node() { return (node *)last_node_void(); }
};

#define zcc_lmap_walk_begin(var_your_lmap, var_your_key_ptr, var_your_value_ptr) { \
    typeof(var_your_lmap) &___V_GRID = (var_your_lmap); \
    typeof(___V_GRID.first_node()->value()) var_your_value_ptr; \
    typeof(___V_GRID.first_node()) var_zcc_lmap_node = ___V_GRID.first_node(); \
    typeof(___V_GRID.first_node()) var_zcc_lmap_node_next; \
    for (; var_zcc_lmap_node; var_zcc_lmap_node = var_zcc_lmap_node_next) { \
        var_zcc_lmap_node_next = var_zcc_lmap_node->next(); \
        long var_your_key_ptr = var_zcc_lmap_node->key(); (void)var_your_key_ptr; \
        var_your_value_ptr = (typeof(___V_GRID.first_node()->value()))var_zcc_lmap_node->value(); \
        (void) var_your_value_ptr; {
#define zcc_lmap_walk_end }}}


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

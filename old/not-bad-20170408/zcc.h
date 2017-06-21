/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-13
 * ================================
 */

#pragma once
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memory>

#pragma pack(push, 4)

/* inner */
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;

/* C ################################################################ */

namespace zcc
{

/* ################################################################## */
#define ZCC_CONTAINER_OF(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - offsetof(app_type,member)))
#define ZCC_CONTAINER_OF2(ptr,app_type,offset) ((app_type *) (((char *) (ptr)) - offset))

#ifndef zcc_pthread_lock
#define zcc_pthread_lock(l)   {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zcc::log_fatal("mutex:%m");}}
#define zcc_pthread_unlock(l) {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zcc::log_fatal("mutex:%m");}}
#endif

/* ################################################################## */
extern char *var_progname;
extern char *var_listen_address;
extern char *var_module_name;
extern char * charp_defarg;

extern bool var_test_mode;
/* ################################################################## */
inline bool empty(const void *ptr)
{
    return ((!ptr)||(!(*(const char *)(ptr))));
}
#define ZCC_EMPTY(ptr) ((!ptr)||(!(*(const char *)(ptr))))
#define ZCC_NEW(my_mpool, class_name, args...) \
    (new((my_mpool)->calloc(1, sizeof(class_name))) class_name(##args))

#define ZCC_DELETE(my_mpool, app_name) \
    (delete app_name, (my_mpool)->free(app_name))

typedef union type_convert_t type_convert_t;
union type_convert_t {
    const void *ptr_const_void;
    const char *ptr_const_char;
    void * ptr_void;
    char * ptr_char;
    long i_long;
    int i_int;
};
#define ZCC_CHAR_PTR_TO_INT(_ptr, _int)  {type_convert_t _ct;_ct.ptr_char=(_ptr);_int=_ct.i_int;}
#define ZCC_INT_TO_CHAR_PTR(_int, _ptr)  {type_convert_t _ct;_ct.i_int=(_int);_ptr=_ct.ptr_char;}

#define ZCC_STR_N_CASE_EQ(a, b, n)       ((ZCC_CHAR_TOLOWER((a)[0])==ZCC_CHAR_TOLOWER((b)[0])) && (!strncasecmp(a,b,n)))
#define ZCC_STR_CASE_EQ(a, b)            ((ZCC_CHAR_TOLOWER((a)[0])==ZCC_CHAR_TOLOWER((b)[0])) && (!strcasecmp(a,b)))
#define ZCC_STR_N_EQ(a, b, n)            (((a)[0] == (b)[0]) && (!strncmp(a,b,n)))
#define ZCC_STR_EQ(a, b)                 (((a)[0] == (b)[0]) && (!strcmp(a,b)))

typedef struct size_data_t size_data_t;
struct size_data_t {
    unsigned int size;
    char *data;
};

/* ################################################################## */
/* log, 通用 */
extern bool var_log_fatal_catch;
extern bool var_log_debug_inner_enable;
typedef void (*log_vprintf_t) (const char *fmt, va_list ap);
extern log_vprintf_t log_vprintf;
void log_fatal(const char *fmt, ...);
void log_info(const char *fmt, ...);

#define zcc_inner_debug(fmt,args...) { if(var_log_debug_inner_enable){zcc::log_info(fmt,##args);}}

/* for my samples */
extern bool var_log_debug_enable;
#define zcc_debug(fmt,args...) { if(zcc::var_log_debug_enable){zcc::log_info(fmt,##args);}}

/* MLINK  ############################################################ */
#define ZCC_MLINK_INIT(p)    (p=0)
#define ZCC_MLINK_APPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZCC_MLINK_PREPEND(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZCC_MLINK_ATTACH_BEFORE(head, tail, node, prev, next, before) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node, _before_1106 = before;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else if(_before_1106==0){_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    else if(_before_1106==_head_1106){_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    else {_node_1106->prev=_before_1106->prev; _node_1106->next=_before_1106; _before_1106->prev->next=_node_1106; _before_1106->prev=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define ZCC_MLINK_DETACH(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_node_1106->prev){ _node_1106->prev->next=_node_1106->next; }else{ _head_1106=_node_1106->next; }\
    if(_node_1106->next){ _node_1106->next->prev=_node_1106->prev; }else{ _tail_1106=_node_1106->prev; }\
    head = _head_1106; tail = _tail_1106; \
}

/* linker 内部使用 ################################################## */
typedef struct linker_node_t linker_node_t;
typedef struct linker_t linker_t;
struct linker_t {
    linker_node_t *head;
    linker_node_t *tail;
};
struct linker_node_t {
    linker_node_t *prev;
    linker_node_t *next;
};
void linker_init(linker_t * link);
linker_node_t *linker_attach_before(linker_t * link, linker_node_t * node, linker_node_t * before);
linker_node_t *linker_detach(linker_t * link, linker_node_t * node);
linker_node_t *linker_push(linker_t * link, linker_node_t * node);
linker_node_t *linker_pop(linker_t * link);
linker_node_t *linker_unshift(linker_t * link, linker_node_t * node);
linker_node_t *linker_shift(linker_t * link);

/* rbtree 内部使用 ################################################## */
typedef struct rbtree_t rbtree_t;
typedef struct rbtree_node_t rbtree_node_t;
typedef int (*rbtree_cmp_t) (rbtree_node_t * node1, rbtree_node_t * node2);
struct rbtree_t {
    rbtree_node_t *rbtree_node;
    rbtree_cmp_t cmp_fn;
};
struct rbtree_node_t {
    unsigned long __rbtree_parent_color;
    rbtree_node_t *rbtree_right;
    rbtree_node_t *rbtree_left;
} __attribute__ ((aligned(sizeof(long))));

inline bool rbtree_have_data(rbtree_t *tree)
{
    return ((tree)->rbtree_node!=0);
}
void rbtree_init(rbtree_t * tree, rbtree_cmp_t cmp_fn);
void rbtree_replace_node(rbtree_t * root, rbtree_node_t * victim, rbtree_node_t * _new);
rbtree_node_t *rbtree_first(rbtree_t * root);
rbtree_node_t *rbtree_last(rbtree_t * root);
rbtree_node_t *rbtree_next(rbtree_node_t * node);
rbtree_node_t *rbtree_prev(rbtree_node_t * node);
rbtree_node_t *rbtree_attach(rbtree_t * tree, rbtree_node_t * node);
rbtree_node_t *rbtree_detach(rbtree_t * tree, rbtree_node_t * node);
rbtree_node_t *rbtree_find(rbtree_t * tree, rbtree_node_t * vnode);
rbtree_node_t *rbtree_near_prev(rbtree_t * tree, rbtree_node_t * vnode);
rbtree_node_t *rbtree_near_next(rbtree_t * tree, rbtree_node_t * vnode);

#define ZRBTREE_WALK_BEGIN(root, var_your_node) {                            \
struct { rbtree_node_t *node; unsigned char lrs; } ___Z_list[64];                \
rbtree_node_t *___Z_node = (root)->rbtree_node;                            \
int ___Z_idx = 0, ___Z_lrs;                                    \
___Z_list[0].node = ___Z_node;                                    \
___Z_list[0].lrs = 0;                                        \
while (1) {                                            \
    var_your_node = ___Z_node = ___Z_list[___Z_idx].node;                    \
    ___Z_lrs = ___Z_list[___Z_idx].lrs;                            \
    if (!___Z_node || ___Z_lrs == 2) {                            \
        if (___Z_node) {
#define ZRBTREE_WALK_END                                        \
        }                                        \
        ___Z_idx--;                                    \
        if (___Z_idx == -1){                                \
            break;                                    \
               }                                        \
        ___Z_list[___Z_idx].lrs++;                            \
        continue;                                    \
    }                                            \
    ___Z_idx++;                                        \
    ___Z_list[___Z_idx].lrs = 0;                                \
    ___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->rbtree_left : ___Z_node->rbtree_right);\
}                                                \
}

#define ZRBTREE_WALK_FORWARD_BEGIN(root, var_your_node)     {                    \
    for (var_your_node = rbtree_first(root); var_your_node; var_your_node = rbtree_next(var_your_node)) {
#define ZRBTREE_WALK_FORWARD_END                }}

#define ZRBTREE_WALK_BACK_BEGIN(root, var_your_node)        {                    \
    for (var_your_node = rbtree_last(root); var_your_node; var_your_node = rbtree_prev(var_your_node)) {
#define ZRBTREE_WALK_BACK_END                   }}

/* malloc ##################################################*/
extern int none_int;
extern long none_long;
extern size_t none_size_t;
extern char *none_buffer;
extern char *blank_buffer;
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(const void *ptr, size_t size);
void free(const void *ptr);
char *strdup(const char *ptr);
char *strndup(const char *ptr, size_t n);
char *memdup(const void *ptr, size_t n);
char *memdupnull(const void *ptr, size_t n);

/* mem_pool ################################################*/
class mem_pool
{
public:
    mem_pool();
    virtual ~mem_pool();
    virtual void *malloc(size_t size) = 0;
    virtual void free(const void *ptr) = 0;
    virtual void *realloc(const void *ptr, size_t size) = 0;
    inline void *calloc(size_t nmemb, size_t size) { return memset(malloc(nmemb * size), 0, nmemb * size); }
    char *strdup(const char *ptr);
    char *strndup(const char *ptr, size_t n);
    char *memdup(const void *ptr, size_t n);
    char *memdupnull(const void *ptr, size_t n);
};
/* system_mem_pool ################################################*/
class system_mem_pool : public mem_pool
{
public:
    system_mem_pool();
    ~system_mem_pool();
    inline void *malloc(size_t size) { return zcc::malloc(size); }
    inline void free(const void *ptr) { zcc::free(ptr); }
    inline void *realloc(const void *ptr, size_t size) { return zcc::realloc(ptr, size); }
};
extern system_mem_pool system_mem_pool_instance;

/* common_mem_pool ################################################*/
typedef struct common_mem_pool_set_t common_mem_pool_set_t;
typedef struct common_mem_pool_setgroup_t common_mem_pool_setgroup_t;
class common_mem_pool : public mem_pool
{
public:
    common_mem_pool();
    common_mem_pool(size_t *register_size_list);
    ~common_mem_pool();
    /* static size_t default_register_list[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 0 }; */
    void init(size_t *register_size_list);
    inline void init_piece(size_t *register_size_list) { init(register_size_list); }
    void *malloc(size_t size);
    void free(const void *ptr);
    void *realloc(const void *ptr, size_t size);
private:
    common_mem_pool_set_t *set_create(common_mem_pool_setgroup_t * setgroup);
    common_mem_pool_set_t *set_find(const void *ptr);
    void free_by_set(common_mem_pool_set_t * set, const void *ptr);
    unsigned short int setgroup_count;
    common_mem_pool_setgroup_t *setgroup_list;
    rbtree_t set_rbtree;
    rbtree_t sys_rbtree;
};

/* greedy_mem_pool ##############################################*/
class greedy_mem_pool : public mem_pool
{
public:
    greedy_mem_pool();
    ~greedy_mem_pool();
    void *malloc(size_t size);
    void free(const void *ptr);
    void *realloc(const void *ptr, size_t size);
    void reset();
    void init(size_t single_buffer_size);
private:
    unsigned int ___single_buffer_size;
    unsigned int ___single_buffer_size_10percent;
    char *___head;
    char *___current;
    char *___sys_head;
    char *___sys_tail;

};

/* mem_piece #####################################################*/
class mem_piece:public common_mem_pool
{
public:
    inline mem_piece() {}
    inline mem_piece(size_t size) { init(size); }
    inline ~mem_piece() {}
    inline void init(size_t size) { size_t sl[2] = {size, 0}; init_piece(sl); }
    inline void * require() { return malloc(1); }
    inline void release(const void *ptr) { free(ptr); }
private:
    common_mem_pool ___mpool;
};

/* cstr ######################################################*/
extern unsigned const char lowercase_map[];
extern unsigned const char uppercase_map[];

#define ZCC_CHAR_TOLOWER(c)    ((int)zcc::lowercase_map[(unsigned char )(c)])
#define ZCC_CHAR_TOUPPER(c)    ((int)zcc::uppercase_map[(unsigned char )(c)])
static inline int to_lower(int c)
{
    return ZCC_CHAR_TOLOWER(c);
}
static inline int to_upper(int c)
{
    return ZCC_CHAR_TOUPPER(c);
}
char *to_lower(char *str);
char *to_upper(char *str);
char *trim_left(char *str);
char *trim_right(char *str);
char *trim(char *str);
size_t skip(const char *line, size_t size, const char *ignores_left, const char *ignores_right, char **start);
char *skip(const char *str, size_t size, const char *ignores);
char *skip_right(const char *str, size_t size, const char *ignores);
char *find_delim(const char *str, size_t size, const char *delims);
bool to_bool(const char *s, bool def);
long to_second(const char *s);
long to_size(const char *s);
char *memstr(const void *s, const char *needle, size_t len);
char *memcasestr(const void *s, const char *needle, size_t len);
class strtok
{
public:
    strtok();
    strtok(const char *str);
    ~strtok();
    void set_str(const char *str);
    bool tok(const char *delim);
    inline char *ptr() { return (const_cast <char *> (next_ptr));
    }
    inline int size() { return next_len; }
private:
    const char *your_str;
    const char *next_ptr;
    int next_len;
};
size_t vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

/* string ########################################################## */
class string
{
public:
    string();
    string(const char *str);
    string(const char *str, size_t size);
    string(size_t size);
    ~string();

    inline /* const */ char *c_str() { (___capability?(___data[___size]=0):0); return ___data;}
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    inline size_t capability() { return ___capability; }
    inline bool is_static() { return ___static_mode; }
    inline void push_back(int ch){(___size<___capability)?(___data[___size++]=ch):(___static_mode?0:(put_do(ch)));}
    inline string &put(int ch) { push_back(ch); return *this; }
    inline void clear() { ___size = 0; }
    string &resize(size_t n);
    inline string &truncate(size_t n) { return resize(n); }
    bool need_space(size_t need);
    inline string &append(int ch) { push_back(ch); return *this; }
    string &append(const char *str, size_t n);
    string &append(const char *str);
    string &append(string &s);
    string &printf_1024(const char *format, ...);

    inline string & operator=(const char *str) { clear(); return append(str); }
    inline string & operator=(string &s) { clear(); return append(s.c_str(), s.size()); }
    inline string & operator+=(const char *str) { return append(str); }
    inline string & operator+=(string &s) { return append(s.c_str(), s.size()); }
    inline string & operator+=(int ch) { return append(ch); }
    inline int operator[](size_t n) { return ___data[n]; }

    string &size_data_escape(const void *data, size_t n = 0);
    string &size_data_escape(int i);
    string &size_data_escape(long i);

    void set_static_buf(void *data, size_t size, mem_pool &mpool = system_mem_pool_instance);
    void set_const_buf(const void *data, size_t size, mem_pool &mpool = system_mem_pool_instance);
private:
    void _init_buf(size_t size);
    int put_do(int ch);
private:
    char *___data;
    unsigned int ___size:30;
    bool ___const_mode;
    bool ___static_mode;
    unsigned int ___capability:30;
};
extern string none_string;
#define ZCC_STACK_STRING(name, _size) \
    zcc::string name; char name ## _databuf_STACK[_size+1]; \
    name.set_static_buf(name ## _databuf_STACK, _size); 

/* rdonly_str_kv ################################################### */
class rdonly_str_kv
{
public:
    rdonly_str_kv();
    ~rdonly_str_kv();
    char *find(const char *key, const char *default_val=0);
    rdonly_str_kv &add(const char *key, const char *value);
    rdonly_str_kv &over();
private:
    int *_offset;
    char *_data;
    int _count:20;
    bool _over;
};

/* vector ########################################################## */
class basic_vector
{
public:
    basic_vector();
    ~basic_vector();
    inline void clear() { ___size = 0; }
    inline size_t size() const { return ___size; }
    void push_back_void(const void * v);
    void init(size_t capacity, mem_pool &mpool = system_mem_pool_instance);
    inline void truncate(size_t n) {if (n < ___size) { ___size = n; }}
    size_t ___capacity;
    size_t ___size;
    mem_pool *___mpool;
    char **___data;
};

template <typename T>
class vector
{
};

template <typename T>
class vector<T *>: public basic_vector
{
public:
    inline vector() {}
    inline ~vector() {}
    inline T * operator[](size_t n) const { return (T *)(___data[n]); }
    inline void push_back(T *v) { push_back_void((const void *)v); }
};
#define zcc_vector_walk_begin(var_your_vec, var_your_node) { \
    typeof(var_your_vec) &___V_VEC = (var_your_vec); \
     size_t ___I_VEC = 0, ___C_VEC=(___V_VEC).size(); \
    for (; ___I_VEC < ___C_VEC; ___I_VEC ++) { \
        var_your_node = (typeof(var_your_node)) (___V_VEC)[___I_VEC]; {
#define zcc_vector_walk_end }}}

/* list ############################################################ */
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
    typedef struct node node;
public:
    basic_list();
    ~basic_list();
    void init_void(mem_pool &mpool = system_mem_pool_instance);
    void clear_void();
    void push_void(const void * v);
    bool pop_void(char **v);
    void unshift_void(const void * v);
    bool shift_void(char **v);
    node *___head;
    node *___tail;
    unsigned int ___size;
    mem_pool *___mpool;
};

template <typename T>
class list
{
};

template <typename T>
class list<T *>: private basic_list
{
public:
    inline list() {}
    inline ~list() {}
    inline size_t size() { return ___size; }
    inline void init(mem_pool &mpool = system_mem_pool_instance) { init_void(mpool); }
    inline void clear() { clear_void(); }
    inline void push(T * v) { push_void((const void *)v); }
    inline void push_back(T * v) { push_void((const void *)v); }
    inline bool pop(T **v) { return pop_void((char **)v); }
    inline void unshift(T * v) { push_void((const void *)v); }
    inline bool shift(T **v) { return pop_void((char **)v); }
    inline node *first_node() { return ___head; }
    inline node *last_node() { return ___tail; }
};
#define zcc_list_walk_begin(var_your_list, var_your_node) { \
    typeof(var_your_list) &___V_LIST = (var_your_list); \
    typeof(___V_LIST.first_node()) ___V_NODE = ___V_LIST.first_node(); \
    for (; ___V_NODE; ___V_NODE = ___V_NODE->next()) { \
        var_your_node = (typeof(var_your_node)) (___V_NODE->data()); {
#define zcc_list_walk_end }}}

/* argv ############################################################ */
class argv
{
public:
    argv();
    ~argv();
    void clear();
    inline void reset() { clear(); }
    inline size_t size() const { return ___size; }
    inline char ** data() const { return ___data; }
    inline char * operator[](size_t n) const { return (char *)(___data[n]); }
    void add(const char * v);
    void addn(const char * v, size_t n);
    void truncate(size_t n);
    void split(const char *str, const char *delim);
    void init(size_t capacity, mem_pool &mpool = system_mem_pool_instance);
    void debug_show();
private:
    void push_back(const char *v);
    unsigned int ___capacity;
    unsigned int ___size;
    mem_pool *___mpool;
    char **___data;
};

/* basic_dict ############################################################ */
class basic_dict
{
public:
    class node
    {
    public:
        node();
        ~node();
        inline char *key() { return ___key; };
        inline char *value() { return ___value; };
        node *prev();
        node *next();
        /* private */
        inline void ___set_key(const char *key) { ___key = const_cast <char *> (key); }
        inline void ___set_value(const void *value) { ___value = (char *)value; }
        inline rbtree_node_t *get_rbnode() { return &___rbnode; }
        inline int get_rbnode_offset() { return (int)((char *)(&___rbnode) - (char *)this); }
    private:
        rbtree_node_t ___rbnode;
        char *___key;
        char *___value;
    };
public:
    basic_dict();
    ~basic_dict();
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    void init(mem_pool &mpool);
    void clear();
    void reset();
    node *___update(const char *key, const void *value);
    node *___update_str(const char *key, const char *value, size_t len);
    void ___update(node *n, const void *value);
    void ___update_str(node *n, const void *value, size_t len);
    void erase(const char *key);
    void erase(node *n);
    node *find(const char *key, char **value=0);
    node *find_near_prev(const char *key, char **value=0);
    node *find_near_next(const char *key, char **value=0);
    node *first_node();
    node *last_node();
    inline void set_str_mode(bool flag) { ___str = flag; }
private:
    rbtree_t ___rbtree;
    mem_pool *___mpool;
    unsigned int ___size:31;
    bool ___str;
};

class dict: public basic_dict
{
public:
    inline dict() {set_str_mode(false);}
    inline ~dict() {}
    node *update(const char *key, const void *value) { return ___update(key, value); }
    inline void update(node *n, const void *value) { return ___update(n, value); }
};

class str_dict: public basic_dict
{
public:
    inline str_dict() {set_str_mode(true);}
    inline ~str_dict() {}
    node *update(const char *key, const char *value, size_t len = -1) { return ___update_str(key, value, len); }
    inline void update(node *n, const char *value, size_t len = -1) { return ___update_str(n, value, len); }
    void debug_show();
};

/* basic_idict ############################################################ */
class basic_idict
{
public:
    class node
    {
    public:
        node();
        ~node();
        inline long key() { return ___key; };
        inline char *value() { return ___value; };
        node *prev();
        node *next();
        /* private */
        inline void ___set_key(long key) { ___key = key; }
        inline void ___set_value(const void *value) { ___value = (char *)value; }
        inline rbtree_node_t *get_rbnode() { return &___rbnode; }
        inline int get_rbnode_offset() { return (int)((char *)(&___rbnode) - (char *)this); }
    private:
        rbtree_node_t ___rbnode;
        long ___key;
        char *___value;
    };
public:
    basic_idict();
    ~basic_idict();
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    void init(mem_pool &mpool);
    void clear();
    void reset();
    node *___update(long key, const void *value);
    node *___update_str(long key, const char *value, size_t len);
    void ___update(node *n, const void *value);
    void ___update_str(node *n, const void *value, size_t len);
    void erase(long key);
    void erase(node *n);
    node *find(long key, char **value=0);
    node *find_near_prev(long key, char **value=0);
    node *find_near_next(long key, char **value=0);
    node *first_node();
    node *last_node();
    inline void set_str_mode(bool flag) { ___str = flag; }
private:
    rbtree_t ___rbtree;
    mem_pool *___mpool;
    unsigned int ___size:31;
    bool ___str;
};

class idict: public basic_idict
{
public:
    inline idict() {set_str_mode(false);}
    inline ~idict() {}
    node *update(long key, const void *value) { return ___update(key, value); }
    inline void update(node *n, const void *value) { return ___update(n, value); }
};

class str_idict: public basic_idict
{
public:
    inline str_idict() {set_str_mode(true);}
    inline ~str_idict() {}
    node *update(long key, const char *value, size_t len = -1) { return ___update_str(key, value, len); }
    inline void update(node *n, const char *value, size_t len = -1) { return ___update_str(n, value, len); }
    void debug_show();
};

/* size_data ######################################################## */
ssize_t size_data_unescape(const void *src_data, size_t src_size, char **result_data, size_t *result_size);
size_t size_data_unescape_all(const void *src_data, size_t src_size, size_data_t *sdvector, size_t sdsize);
size_t size_data_put_size(size_t size, char *buf);

/* time ############################################################ */
long timeout_set(long timeout);
long timeout_left(long timeout);
void msleep(long delay);
void sleep(long delay);

/* dns ############################################################ */
ssize_t get_localaddr(char * addr_list, size_t max_count);
ssize_t get_hostaddr(const char *host, char * addr_list, size_t max_count);
bool get_peername(int sockfd, int *host, int *port);

/* tcp socket ##################################################### */
const int var_tcp_listen_type_inet = 'i';
const int var_tcp_listen_type_unix = 'u';
const int var_tcp_listen_type_fifo = 'f';
int unix_accept(int fd);
int inet_accept(int fd);
int unix_listen(char *addr, int backlog);
int inet_listen(const char *sip, int port, int backlog);
int listen(const char *netpath, int backlog, int *type = 0);
int fifo_listen(const char *path);
int unix_connect(const char *addr, long timeout);
int inet_connect(const char *dip, int port, long timeout);
int host_connect(const char *host, int port, long timeout);
int connect(const char *netpath, long timeout);

/* io ############################################################# */
bool is_rwable(int fd, bool *rable, bool *wable);
bool is_readable(int fd, bool *rable);
bool is_writeable(int fd, bool *wable);
bool flock(int fd, int flags);
bool flock_share(int fd);
bool flock_exclusive(int fd);
bool funlock(int fd);
bool nonblocking(int fd, bool no = true);
bool close_on_exec(int fd, bool on = true);
int get_readable_count(int fd);
ssize_t writen(int fd, const void *buf, size_t size);

/* timed_io ######################################################## */
bool wait_readable(int fd, long timeout);
ssize_t timed_read(int fd, void *buf, size_t size, long timeout);
ssize_t timed_readn(int fd, void *buf, size_t size, long timeout);
bool wait_writeable(int fd, long timeout);
ssize_t timed_write(int fd, const void *buf, size_t size, long timeout);
ssize_t timed_writen(int fd, const void *buf, size_t size, long timeout);

/* openssl ######################################################## */
extern bool openssl_debug_enable;
void openssl_init(void);
void openssl_fini(void);
SSL_CTX *openssl_create_SSL_CTX_server(void);
SSL_CTX *openssl_create_SSL_CTX_client(void);
bool openssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file);
void openssl_SSL_CTX_free(SSL_CTX * ctx);
void openssl_get_error(unsigned long *ecode, char *buf, int buf_len);
SSL *openssl_create_SSL(SSL_CTX * ctx, int fd);
void openssl_SSL_free(SSL * ssl);
int openssl_SSL_get_fd(SSL *ssl);
bool openssl_timed_connect(SSL * ssl, long timeout);
bool openssl_timed_accept(SSL * ssl, long timeout);
bool openssl_timed_shutdown(SSL * ssl, long timeout);
ssize_t openssl_timed_read(SSL * ssl, void *buf, size_t len, long timeout);
ssize_t openssl_timed_write(SSL * ssl, const void *buf, size_t len, long timeout);

/* stream ########################################################## */
const size_t stream_read_buf_size = 4096;
const size_t stream_write_buf_size = 4096;
class stream
{
public:
    stream();
    virtual ~stream();
    virtual ssize_t read_fn(void *buf, size_t size, long timeout) = 0;
    virtual ssize_t write_fn(const void *buf, size_t size, long timeout) = 0;
    stream &set_timeout(long timeout);
    long get_timeout();
    inline bool is_error() { return ___stream_error; }
    inline bool is_eof() { return ___stream_eof; }
    /* read */
    inline int get()
    {
        return ((read_buf_p1<read_buf_p2)?(read_buf[read_buf_p1++]):(get_char_do()));
    };
    ssize_t readn(void *buf, size_t size);
    ssize_t readn(string &str, size_t size);
    ssize_t gets(void *buf, size_t size, int delimiter='\n');
    ssize_t gets(string &str, int delimiter = '\n');
    ssize_t size_data_get_size();
    /* write */
    inline stream &put(int ch)
    {
        write_buf[write_buf_len++]=ch;
        (write_buf_len<stream_write_buf_size)?(1):(flush());
        return *this;
    };
    bool flush();
    stream &writen(const void *buf, size_t size);
    stream &puts(const char *str);
    stream &printf_1024(const char *format, ...);
private:
    int get_char_do();
    int read_buf_p1:20;
    int read_buf_p2:20;
    size_t write_buf_len:20;
    long _timeout;
    char read_buf[stream_read_buf_size + 1];
    char write_buf[stream_write_buf_size + 1];
    bool ___stream_error;
    bool ___stream_eof;
};


/* io stream ######################################################## */
class iostream : public stream
{
public:
    iostream(int fd);
    ~iostream();
    ssize_t read_fn(void *buf, size_t size, long timeout);
    ssize_t write_fn(const void *buf, size_t size, long timeout);
    int get_fd();
private:
    int _fd;
};

/* ssl stream ####################################################### */
class sslstream : public stream
{
public:
    sslstream(SSL *ssl);
    ~sslstream();
    SSL *get_SSL();
    ssize_t read_fn(void *buf, size_t size, long timeout);
    ssize_t write_fn(const void *buf, size_t size, long timeout);
private:
    SSL *_ssl;
};

/* tls stream ####################################################### */
class tlsstream : public stream
{
public:
    tlsstream(int fd);
    ~tlsstream();
    int get_fd();
    SSL *get_SSL();
    bool tls_connect(SSL_CTX *ctx);
    bool tls_accept(SSL_CTX *ctx);
    ssize_t read_fn(void *buf, size_t size, long timeout);
    ssize_t write_fn(const void *buf, size_t size, long timeout);
private:
    int _fd;
    SSL *_ssl;
};


/* event ####################################################### */
const unsigned char var_event_none         =    0x00;
const unsigned char var_event_read         =    0x01;
const unsigned char var_event_write        =    0x02;
const unsigned char var_event_rdwr         =    0x03;
const unsigned char var_event_persist      =    0x04;
const unsigned char var_event_rdhup        =    0x10;
const unsigned char var_event_hup          =    0x20;
const unsigned char var_event_error        =    0x40;
const unsigned char var_event_errors       =    0x70;
const unsigned char var_event_timeout      =    0x80;
const unsigned char var_event_exception    =    0xF0;
const unsigned char var_event_type_event   =    0x01;
const unsigned char var_event_type_aio     =    0x02;

typedef struct ev_t ev_t;
typedef struct aio_t aio_t;
typedef struct evtimer_t evtimer_t;
typedef struct evbase_t evbase_t;
class event_io;
class async_io;
class event_timer;
class event_base;

extern event_base default_evbase;

typedef void (*event_io_cb_t) (event_io &ev);
struct ev_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char _init:1;
    unsigned char events;
    unsigned char recv_events;
    int fd;
    event_io_cb_t callback;
    void *context;
    evbase_t *evbase;
};

class event_io
{
public:
    event_io();
    ~event_io();
    void init(int fd, event_base &eb = default_evbase);
    void fini();
    inline bool error() { return ___data.recv_events & var_event_exception; }
    inline bool timeout() { return ___data.recv_events & var_event_timeout; }
    inline bool exception() { return ___data.recv_events & var_event_exception; }
    inline bool is_local() { return ___data.is_local; }
    inline void set_local() { ___data.is_local = 1; }
    inline int get_fd() { return ___data.fd; }
    void enable_event(int events, event_io_cb_t callback);
    inline void enable_read(event_io_cb_t callback) { enable_event(var_event_read, callback); }
    inline void enable_write(event_io_cb_t callback) { enable_event(var_event_write, callback); }
    inline void disable() { enable_event(0, 0); }
    inline void set_context(const void *ctx) { ___data.context = const_cast <void *> (ctx); }
    void * get_context() { return ___data.context; }
    event_base *get_event_base();
    ev_t ___data;
};

typedef struct aio_rwbuf_t aio_rwbuf_t;
typedef struct aio_rwbuf_list_t aio_rwbuf_list_t;
const int aio_rwbuf_size  = 4096;
struct aio_rwbuf_t {
    aio_rwbuf_t *next;
    unsigned int long_flag:1;
    unsigned int p1:15;
    unsigned int p2:15;
    char data[aio_rwbuf_size];
};

typedef struct aio_rwbuf_longbuf_t aio_rwbuf_longbuf_t;
struct aio_rwbuf_longbuf_t {
    size_t p1;
    size_t p2;
    char *data;
};
struct aio_rwbuf_list_t {
    int len;
    aio_rwbuf_t *head;
    aio_rwbuf_t *tail;
};
typedef void (*async_io_cb_t) (async_io &as);
struct aio_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char in_time:1;
    unsigned char enable_time:1;
    unsigned char want_read:1;
    unsigned char is_size_data:1;
    unsigned char events;
    unsigned char recv_events;
    unsigned char rw_type;
    char delimiter;
    int fd;
    int read_magic_len;
    int ret;
    async_io_cb_t callback;
    void *context;
    aio_rwbuf_list_t read_cache;
    aio_rwbuf_list_t write_cache;
    long timeout;
    rbtree_node_t rbnode_time;
    evbase_t *evbase;
    aio_t *queue_prev;
    aio_t *queue_next;
    unsigned char ssl_server_or_client:1;
    unsigned char ssl_session_init:1;
    unsigned char ssl_read_want_read:1;
    unsigned char ssl_read_want_write:1;
    unsigned char ssl_write_want_read:1;
    unsigned char ssl_write_want_write:1;
    unsigned char ssl_error:1;
    SSL *ssl;
};

class async_io
{
public:
    async_io();
    ~async_io();
    inline int get_ret() { return ___data.ret; }
    inline bool is_local() { return ___data.is_local; }
    inline void set_local() { ___data.is_local = 1; }
    inline int get_fd() { return ___data.fd; }
    inline void set_context(const void *ctx) { ___data.context = const_cast <void *> (ctx); }
    void * get_context() { return ___data.context; }
    void init(int fd, event_base &eb = default_evbase);
    void fini();
    void ssl_init(SSL_CTX * ctx, async_io_cb_t callback, long timeout, bool server_or_client);
    inline void tls_connect(SSL_CTX * ctx, async_io_cb_t callback, long timeout)
    { return ssl_init(ctx, callback, timeout, false); }
    inline void tls_accept(SSL_CTX * ctx, async_io_cb_t callback, long timeout)
    { return ssl_init(ctx, callback, timeout, true); }
    SSL *detach_SSL();
    void fetch_rbuf(char *buf, int len);
    void fetch_rbuf(string &dest, int len);
    void read(int max_len, async_io_cb_t callback, long timeout);
    void readn(int strict_len, async_io_cb_t callback, long timeout);
    void read_size_data(async_io_cb_t callback, long timeout);
    void read_delimiter(int delimiter, int max_len, async_io_cb_t callback, long timeout);
    inline void read_line(int max_len, async_io_cb_t callback, long timeout)
    { read_delimiter('\n', max_len, callback, timeout); }
    void cache_printf_1024(const char *fmt, ...);
    void cache_puts(const char *s);
    void cache_write(const void *buf, size_t len);
    void cache_write_size_data(const void *buf, size_t len);
    void cache_write_direct(const void *buf, size_t len);
    void cache_flush(async_io_cb_t callback, long timeout);
    inline size_t get_cache_size() { return ___data.write_cache.len; }
    void sleep(async_io_cb_t callback, long timeout);
    event_base *get_event_base();

    aio_t ___data;
};

typedef void (*event_timer_cb_t) (event_timer &);
struct evtimer_t {
    long timeout;
    event_timer_cb_t callback;
    void *context;
    rbtree_node_t rbnode_time;
    unsigned char init:1;
    unsigned char in_time:1;
    unsigned char is_local:1;
    evbase_t *evbase;
};

class event_timer
{
public:
    event_timer();
    ~event_timer();
    void init(event_base &eb = default_evbase);
    void fini();
    void start(event_timer_cb_t callback, long timeout);
    void stop();
    inline bool is_local() { return ___data.is_local; }
    inline void set_local() { ___data.is_local = 1; }
    inline void set_context(const void *ctx) { ___data.context = const_cast <void *> (ctx); }
    void * get_context() { return ___data.context; }
    event_base *get_event_base();
/* private */
    evtimer_t ___data;
};

struct evbase_t {
    int epoll_fd;
    struct epoll_event *epoll_event_list;
    rbtree_t event_timer_tree;
    rbtree_t aio_timer_tree;
    event_io *eventfd_event;
    void *context;
    mem_piece *aio_rwbuf_mpool;

    aio_t *queue_head;
    aio_t *queue_tail;

    aio_t *extern_queue_head;
    aio_t *extern_queue_tail;

    void *plock;
};

class event_base
{
public:
    event_base();
    ~event_base();
    inline void set_context(const void *ctx) { ___data.context = const_cast <void *> (ctx); }
    void * get_context() { return ___data.context; }
    void notify();
    void option_plock();
    void dispatch();
    evbase_t ___data;
};

/* iopipe ########################################################### */
typedef void (*iopipe_after_close_fn_t) (void *);
class iopipe
{
public:
    iopipe();
    ~iopipe();
    void run();
    void stop_notify();
    void enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl);
    void enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, void *context);
private:
    void *___data;
};

/* hash ########################################################### */
unsigned int get_crc32_result(const void *data, size_t size, unsigned int init_value = 0);
unsigned long get_crc64_result(const void *data, size_t size, unsigned long init_value = 0);

struct md5_runtime {
    uint_fast32_t lo, hi;
    uint_fast32_t a, b, c, d;
    unsigned char buffer[64];
    uint_fast32_t block[16];
};
class md5
{
public:
    md5();
    md5(const char *data, size_t size);
    ~md5();
    md5 *update (const void *data, size_t size);
    char *get_result (void *result);
    md5 *reset();
private:
    md5_runtime _ctx;
};
char *get_md5_result (const void *data, size_t size, void *result);

struct sha1_runtime {
    union {
        uint8_t b8[20];
        uint32_t b32[5];
    } h;
    union {
        uint8_t b8[8];
        uint64_t b64[1];
    } c;
    union {
        uint8_t b8[64];
        uint32_t b32[16];
    } m;
    uint8_t count;
};
class sha1
{
public:
    sha1();
    sha1(const char *data, size_t size);
    ~sha1();
    sha1 *update (const void *data, size_t size);
    char *get_result (void *result);
    sha1 *reset();
private:
    sha1_runtime _ctx;
};
char *get_sha1_result (const void *data, size_t size, void *result);

/* encode ########################################################### */
ssize_t base64_encode(const void *src, size_t src_size, string &dest, bool mime_flag = false);
ssize_t base64_decode(const void *src, size_t src_size, string &dest);
ssize_t base64_encode_get_min_len(size_t in_len, bool mime_flag = false);
ssize_t base64_decode_get_valid_len(const void *src, size_t src_size);

ssize_t qp_decode_2045(const void *src, size_t src_size, string &dest);
ssize_t qp_decode_2047(const void *src, size_t src_size, string &dest);
ssize_t qp_decode_get_valid_len(const void *src, size_t src_size);

extern char hex_to_dec_table[];
ssize_t hex_encode(const void *src, size_t src_size, string &dest);
ssize_t hex_decode(const void *src, size_t src_size, string &dest);

size_t ncr_decode(size_t ins, char *wchar);

/* file ############################################################## */
ssize_t file_get_size(const char *filename);
bool file_put_contents(const char *filename, const void *data, size_t len);
ssize_t file_get_contents(const char *filename, string &str);
ssize_t file_get_contents_sample(const char *filename, string &str);
ssize_t stdin_get_contents(string &str);
class file_mmap
{
public:
    file_mmap();
    ~file_mmap();
    bool mmap(const char *filename);
    void munmap();
    inline const char *data() { return _data; }
    inline size_t size() { return _size; }
private:
    int _fd;
    char *_data;
    size_t _size;
};

/* util ############################################################# */
int parameter_run(int argc, char **argv);
void parameter_run_2();
#define ZCC_PARAMETER_BEGIN() { zcc::var_progname = argv[0]; \
    int opti, ___optret_123, optval_count; char *optname, *optval; \
    for (opti = 1; opti < argc;) { optname = argv[opti]; optval = 0; optval_count = 0;\
        ___optret_123 = zcc::parameter_run(argc-opti, argv+opti); \
        if (___optret_123 > 0) { opti += ___optret_123; continue; } \
        if (___optret_123 < 0) { ___usage(argv[opti]); exit(1); } \
        if (opti+1 < argc) { optval = argv[opti+1];} \
        for(optval_count = 0; opti + 1 + optval_count < argc; optval_count++) { \
            if(argv[opti + 1+ optval_count][0] == '-') break; \
        } (void)optname; (void)optval; (void)opti; (void)optval_count; {
#define ZCC_PARAMETER_END      } ___usage(argv[opti]); exit(1);} zcc::parameter_run_2(); }


/* system ########################################################### */
bool chroot_user(const char *root_dir, const char *user_name);

/* license ########################################################## */
bool license_mac_check(const char *salt, const char *license);
void license_mac_build(const char *salt, const char *_mac, char *rbuf);

/* 配置文件 ######################################################### */
typedef struct {
    const char *name;
    const char *defval;
    char **target;
} config_str_table_t;
typedef struct {
    const char *name;
    bool defval;
    bool *target;
} config_bool_table_t;
typedef struct {
    const char *name;
    int defval;
    int *target;
    int min;
    int max;
} config_int_table_t;
typedef struct {
    const char *name;
    long defval;
    long *target;
    long min;
    long max;
} config_long_table_t;
typedef config_long_table_t config_second_table_t;
typedef config_long_table_t config_size_table_t;

class config : public str_dict
{
public:
    config();
    ~config();
    bool exists(const char *key);
    bool load_from_filename(const char *filename);
    char *get_str(const char *key, const char *def);
    bool get_bool(const char *key, bool def);
    int get_int(const char *key, int def, int min, int max);
    long get_long(const char *key, long def, long min, long max);
    long get_second(const char *key, long def, long min, long max);
    long get_size(const char *key, long def, long min, long max);
    /* table */
    void get_str_table(config_str_table_t * table);
    void get_int_table(config_int_table_t * table);
    void get_long_table(config_long_table_t * table);
    void get_bool_table(config_bool_table_t * table);
    void get_second_table(config_second_table_t * table);
    void get_size_table(config_size_table_t * table);
};
extern config default_config;

/* master ############################################################# */
const int var_master_server_status_fd = 3;
const int var_master_master_status_fd = 4;
const int var_master_server_listen_fd = 5;

int master_main(int argc, char **argv);
extern void (*master_load_server_config_fn)(vector<config *> &cfs);
void master_load_server_config_by_dir(const char *config_path, vector<config *> &cfs);


/* server */
class master_server
{
public:
    master_server();
    ~master_server();
    virtual void before_service();
    virtual void event_loop();
    virtual void before_reload();
    virtual void before_exit();
    virtual void simple_service(int fd);
    virtual void service_register(const char *service_name, int fd, int fd_type);
    event_io *general_service_register(int fd, int fd_type , void (*service) (int, event_io &)
            , event_base &eb = default_evbase);
    void run(int argc, char **argv);
    static void stop_notify();
    static bool flag_reloading;
    static bool flag_stopping;
private:
    void clear();
    void test_register(char *test_url);
    void inner_service_register(char *s, char *optval);
};

/* finder ############################################################ */
class basic_finder
{
public:
    basic_finder();
    inline virtual ~basic_finder() {}
    virtual bool open(const char *url) = 0;
    virtual ssize_t find(const char *query, string &result, long timeout) = 0;
    inline virtual void disconnect() { };
    virtual bool pthread_safe() = 0;
    rdonly_str_kv kv_pairs;
    void parse_url(const char *url);
    const char *___url;
    const char *___destination;
    bool ___flock;
};

class finder
{
public:
    finder();
    ~finder();
    bool open(const char *url);
    finder &close();
    ssize_t find(const char *query, string &result, long timeout);
    inline finder &disconnect() {if (___fder) ___fder->disconnect(); return *this; }
    inline finder &option_flock() { ___flock = true; return *this; }
    inline finder &option_plock() { ___plock = true; return *this; }
    inline finder &option_uppercase() { ___uppercase = true; ___lowercase = false; return *this; }
    inline finder &option_lowercase() { ___lowercase = true; ___uppercase = false; return *this; }
private:
    basic_finder *___fder;
    void * ___mutex;
    bool ___flock;
    bool ___plock;
    bool ___uppercase;
    bool ___lowercase;
};

ssize_t finder_once(const char *url, const char *query, string &result, long timeout);
int finder_main(int argc, char **argv);

extern basic_finder *(*finder_create_extend_fn)(const char *method, char *url);

/* charset ############################################################ */
ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, char *dest, ssize_t dest_len
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count);

static inline ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, string &dest
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count)
{

    return charset_iconv(from_charset, src, src_len, to_charset, (char *)(&dest), -1
            , src_converted_len, omit_invalid_bytes_limit, omit_invalid_bytes_count);
}
char *charset_correct_charset(const char *charset);
bool charset_detect(const char *data, size_t size, char *charset_result, const char **charset_list);
bool charset_detect_cjk(const char *data, size_t size, char *charset_result);
extern const char *charset_chinese[];
extern const char *charset_japanese[];
extern const char *charset_korean[];
extern const char *charset_cjk[];

#ifdef ZCC_USE_LIBICONV
typeof(iconv) libiconv;
typeof(iconv_open) libiconv_open;
typeof(iconv_close) libiconv_close;

iconv_t iconv_open(const char *tocode, const char *fromcode)
{
    return libiconv_open(tocode, fromcode);
}

size_t iconv(iconv_t cd, char **inbuf, size_t * inbytesleft, char **outbuf, size_t * outbytesleft)
{
    return libiconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
}

int iconv_close(iconv_t cd)
{
    return libiconv_close(cd);
}
#endif

/* mail_parser ######################################################## */
#define ZMAIL_HEADER_LINE_MAX_LENGTH        102400
#define ZMAIL_HEADER_LINE_MAX_ELEMENT       10240
class mail_parser_mime;
class mail_parser;

void mime_iconv(const char *from_charset, const char *data, size_t size, string &dest);

void mime_unescape(const char *in_str, size_t in_len, string &dest);
/* the dest'size must be larger than in_len */
size_t mime_unescape(const char *in_str, size_t in_len, char *dest);
void mime_get_first_token(const char *in_str, size_t in_len, string &value);
void mime_get_first_token(const char *in_str, size_t in_len, char **value, size_t *vlen);
struct mime_element_t {
    char charset[32];
    char *data;
    size_t size;
    unsigned char encode; /* B: base64, Q: quoted_printable */
};
typedef struct mime_element_t mime_element_t;
size_t mime_get_elements(const char *in_str, size_t in_len
        , mime_element_t * vec, size_t ele_max_count);

void mime_get_utf8(const char *src_charset_def, const char *in_str, size_t in_len, string &dest);
void mime_get_utf8_2231(const char *src_charset_def, const char *in_str, size_t in_len
        , string &dest , bool with_charset = true);
void mime_get_params(const char *in_str, size_t in_len, string &value, str_dict &params);
void mime_decode_content_type(const char *data, size_t len
        , char **val, size_t *v_len
        , char **boundary, size_t *b_len
        , char **charset, size_t *c_len
        , char **name, size_t *n_len);
void mime_decode_content_disposition(const char *data, size_t len
        , char **val, size_t *v_len
        , char **filename, size_t *f_len
        , string *filename_2231
        , bool *filename_2231_with_charset);
void mime_decode_content_transfer_encoding(const char *data, size_t len, char **val, size_t *v_len);
long mime_decode_date(const char *str);

class mime_address_parser
{
    public:
        mime_address_parser();
        ~mime_address_parser();
        void parse(const char *line, size_t size);
        bool shift(string &name, string &address);
    private:
        char *___str;
        int ___len;
        bool ___over;
};

struct mime_address_t {
    char *name;
    char *address;
    char *name_utf8;
};
typedef struct mime_address_t mime_address_t;
void mime_get_address_vector(const char *in_str, size_t in_len, vector<mime_address_t *> &vec
        , mem_pool &mpool = system_mem_pool_instance);
void mime_get_address_utf8_vector(const char * in_charset_def, char *in_str, size_t in_len
        , vector<mime_address_t *> &vec, mem_pool &mpool = system_mem_pool_instance);
void mime_free_address(mime_address_t *addr, mem_pool &mpool = system_mem_pool_instance);

typedef struct mail_parser_t mail_parser_t;
typedef struct mail_parser_mime_t mail_parser_mime_t;
struct mail_parser_mime_t {
    char *type;
    char *encoding;
    char *charset;
    char *disposition;
    char *show_name;
    char *name;
    char *name_utf8;
    char *filename;
    char *filename2231;
    unsigned char filename2231_with_charset:1;
    char *filename_utf8;
    char *content_id;
    char *boundary;
    /* mime proto, for imapd */
    char *imap_section;
    int header_offset;
    int header_len;
    int body_offset;
    int body_len;

    short int mime_type;
    bool is_tnef;

    /* mime original header-logic-line */
    vector<size_data_t *> *header_lines;

    /* relationship */
    mail_parser_mime *next;
    mail_parser_mime *child;
    mail_parser_mime *parent;
    /* */
    mail_parser_t * parser;
};

struct mail_parser_t {
    char *subject;
    char *subject_utf8;
    char *date;
    long date_unix;
    short int from_flag:2;
    short int sender_flag:2;
    short int reply_to_flag:2;
    short int to_flag:2;
    short int cc_flag:2;
    short int bcc_flag:2;
    short int receipt_flag:2;
    short int references_flag:2;
    short int classify_flag:2;
    short int section_flag:2;
    mime_address_t *from;
    mime_address_t *sender;
    mime_address_t *reply_to;
    vector<mime_address_t *> *to;
    vector<mime_address_t *> *cc;
    vector<mime_address_t *> *bcc;
    mime_address_t *receipt;
    char *in_reply_to;
    char *message_id;
    vector<char *> *references;

    /* mime-tree */
    mail_parser_mime *top_mime;

    /* all-mime-vector */
    vector<mail_parser_mime *> *all_mimes;

    /* text(plain,html) type mime-list except for attachment */
    vector<mail_parser_mime *> *text_mimes;

    /* similar to the above, 
     * in addition to the case of alternative, html is preferred */
    vector<mail_parser_mime *> *show_mimes;

    /* attachment(and background-image) type mime-list */
    vector<mail_parser_mime *> *attachment_mimes;

    /* option */
    short int mime_max_depth;
    char src_charset_def[32];

    /* other */
    mem_pool *mpool;
    char *mail_data;
    char *mail_pos;
    int mail_size;
    /* tmp or cache */
    vector<size_data_t *> * tmp_header_lines;
};

class mail_parser_mime
{
public:
     mail_parser_mime(mail_parser_t *parser);
    ~mail_parser_mime();
    inline const char *type(){ return td.type; }
    inline const char *encoding(){ return (td.encoding?td.encoding:wrap_get(1)); }
    inline const char *charset(){ return td.charset; }
    inline const char *disposition(){ return (td.disposition?td.disposition:wrap_get(2)); }
    inline const char *show_name(){ return (td.show_name?td.show_name:wrap_get(8)); }
    inline const char *name(){ return td.name; }
    inline const char *name_utf8(){ return (td.name_utf8?td.name_utf8:wrap_get(3)); }
    inline const char *filename(){ return (td.filename?td.filename:wrap_get(4)); }
    inline const char *filename2231() {if(!td.filename2231)wrap_get(5); return td.filename2231; } 
    inline const bool filename2231_with_charset() {
        if(!td.filename2231)wrap_get(5); return td.filename2231_with_charset; } 
    inline const char *filename_utf8(){return (td.filename_utf8?td.filename_utf8:wrap_get(6));}
    inline const char *content_id(){return (td.content_id?td.content_id:wrap_get(7));} 
    inline const char *boundary(){ return td.boundary; }
    inline const char *imap_section() {if(!td.imap_section) wrap_get(61); return td.imap_section; }
    inline size_t header_offset(){ return td.header_offset; }
    inline size_t header_size(){ return td.header_len; }
    inline size_t body_offset(){ return td.body_offset; }
    inline size_t body_size(){ return td.body_len; }
    inline bool is_tnef(){ return td.is_tnef; }
    inline mail_parser_mime * next(){ return td.next; }
    inline mail_parser_mime * child(){ return td.child; }
    inline mail_parser_mime * parent(){ return td.parent; }
    inline const vector<size_data_t *> *header_lines(){ return td.header_lines; }
    /* sn == 0: first, sn == -1: last */
    bool header_line(const char *header_name, char **data, size_t *size, int n = 0);
    bool header_line(const char *header_name, string &result, int n = 0);
    bool header_line(const char *header_name, vector<const size_data_t *> &vec);
    void decoded_content(string &dest);
    void decoded_content_utf8(string &dest);

private:
    mail_parser_mime_t td;
    char *wrap_get(int module);
};

class mail_parser
{
public:
    mail_parser(mem_pool &mpool = system_mem_pool_instance);
    ~mail_parser();
    mail_parser &option_mime_max_depth(size_t depth);
    mail_parser &option_src_charset_def(const char *src_charset_def);
    mail_parser &parse(const char *mail_data, size_t mail_data_len);
    void debug_show();
    inline const char *data(){return td.mail_data; }
    inline size_t size() { return td.mail_size; }
    inline size_t header_size() { return td.top_mime->header_size(); }
    inline size_t body_offset() { return td.top_mime->body_offset(); }
    inline size_t body_size() { return td.top_mime->body_size(); }
    inline const char *message_id(){return td.message_id?td.message_id:wrap_get(13);}
    inline const char *subject() { return td.subject?td.subject:wrap_get( 1); }
    inline const char *subject_utf8(){return td.subject_utf8?td.subject_utf8:wrap_get(2);}
    inline const char *date(){return td.date?td.date:wrap_get(3);}
    inline long date_unix(){ if(!td.date_unix)wrap_get(4); return td.date_unix; }
    inline const mime_address_t *from(){ if(!td.from_flag)wrap_get(5); return td.from; }
    inline const mime_address_t *from_utf8() { if((td.from_flag!=2))wrap_get(105); return td.from; }
    inline const mime_address_t *sender() { if(!td.sender_flag)wrap_get(6); return td.sender; }
    inline const mime_address_t *reply_to() {if(!td.reply_to_flag)wrap_get(7); return td.reply_to; }
    inline const mime_address_t *receipt() { if(!td.receipt_flag)wrap_get(11);return td.receipt; }
    inline const char *in_reply_to(){return td.in_reply_to?td.in_reply_to:wrap_get(12);}
    inline const vector<mime_address_t *> *to(){ if(!td.to_flag)wrap_get(8); return td.to; }
    inline const vector<mime_address_t *> *to_utf8(){if(td.to_flag!=2)wrap_get(108); return td.to; }
    inline const vector<mime_address_t *> *cc(){ if(!td.cc_flag)wrap_get(9); return td.cc; }
    inline const vector<mime_address_t *> *cc_utf8(){if(td.cc_flag!=2)wrap_get(109); return td.cc; }
    inline const vector<mime_address_t *> *bcc(){ if(!td.bcc_flag)wrap_get(10); return td.bcc; }
    inline const vector<mime_address_t *> *bcc_utf8(){if(td.bcc_flag!=2)wrap_get(110); return td.bcc; }
    inline const vector<char *> *references() { if(!td.references_flag)wrap_get(14); return td.references; }
    inline const mail_parser_mime *top_mime() { return td.top_mime; }
    inline const vector<mail_parser_mime *> *all_mimes() { return td.all_mimes; }
    inline const vector<mail_parser_mime *> *text_mimes(){if(!td.classify_flag)wrap_get(62);return td.text_mimes;}
    inline const vector<mail_parser_mime *> *show_mimes(){if(!td.classify_flag)wrap_get(62);return td.show_mimes;}
    inline const vector<mail_parser_mime *> *attachment_mimes() {
        if (!td.classify_flag) wrap_get(62); return td.attachment_mimes;
    }
    inline const vector<size_data_t *> *header_lines(){ return td.top_mime->header_lines(); }
    /* sn == 0: first, sn == -1: last */
    inline bool header_line(const char *header_name, char **data, size_t *size, int n = 0) {
        return td.top_mime->header_line(header_name, data, size, n);
    }
    inline bool header_line(const char *header_name, string &result, int n = 0) {
        return td.top_mime->header_line(header_name, result, n);
    }
    inline bool header_line(const char *header_name, vector<const size_data_t *> &vec) {
        return td.top_mime->header_line(header_name, vec);
    }
private:
    mail_parser_t td;
    char *wrap_get(int module);
};

/* tnef */
typedef struct tnef_parser_mime_t tnef_parser_mime_t;
typedef struct tnef_parser_t tnef_parser_t;
class tnef_parser_mime;
class tnef_parser;

struct tnef_parser_mime_t {
    char *type;
    char *filename;
    char *filename_utf8;
    char *content_id;
    int body_offset;
    int body_len;

    /* relationship */
    tnef_parser_mime_t *all_last;
    tnef_parser_mime_t *all_next;

    /* */
    short int mime_type;
    tnef_parser_t *parser;
};

struct tnef_parser_t {
    char src_charset_def[32];
    /* */
    mem_pool *mpool;
    char *data_orignal;
    char *tnef_data;
    char *tnef_pos;
    int tnef_size;
};

class tnef_parser_mime
{
public:
    tnef_parser_mime(tnef_parser_t *parser);
    ~tnef_parser_mime();
    inline const char * type() { return td.type; }
    inline const char * show_name() {const char *n = filename_utf8();if(n==blank_buffer)n=filename(); return n;}
    inline const char * filename() { return td.filename; }
    inline const char * filename_utf8() { if (!td.filename_utf8) _filename_utf8(); return td.filename_utf8; }
    inline const char * content_id() { return td.content_id; }
    inline size_t body_offset() { return td.body_offset; }
    inline size_t body_size() { return td.body_len; }

private:
    tnef_parser_mime_t td;
    void _filename_utf8();
};

class tnef_parser
{
public:
    tnef_parser(mem_pool &mpool = system_mem_pool_instance);
    ~tnef_parser();
    tnef_parser &option_src_charset_def(const char *src_charset_def);
    void parse(const char *mail_data, size_t mail_data_len);
    inline const char *data() { return td.tnef_data;}
    inline size_t size() { return td.tnef_size;}
    inline const vector<tnef_parser_mime *> *all_mimes() { return &___all_mimes; }

private:
    tnef_parser_t td;
    vector<tnef_parser_mime *> ___all_mimes;
};

}

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

#if defined(__i386__)
#error only support X64
#endif

#include <memory>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma GCC diagnostic ignored "-Winvalid-offsetof"

#pragma pack(push, 4)

/* inner */
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_st SSL;

/* C ################################################################ */
#ifdef ZCC_USE_LIBICONV
#include <iconv.h>

extern "C" {
typeof(iconv) libiconv;
typeof(iconv_open) libiconv_open;
typeof(iconv_close) libiconv_close;
}

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

#define _ZCC_SIZEOF_DEBUG(a,b) { \
    if((sizeof(a)-zcc_offsetof(a, ___data))!=sizeof(b)) { \
        printf("\nsizeof(%s)==%zd\n\n",#b,sizeof(b)); \
        exit(1); \
    } \
}

/* std */
#define std_vector_walk_begin(var_your_vec, var_your_ptr) { \
    typeof(var_your_vec) &___V_VEC = (var_your_vec); \
    for (typeof(___V_VEC.begin()) var_std_vector_it = ___V_VEC.begin(); \
            var_std_vector_it!=___V_VEC.end(); var_std_vector_it ++) { \
        typeof(*(___V_VEC.begin())) var_your_ptr = (*var_std_vector_it); {
#define std_vector_walk_end }}}

namespace zcc
{

/* ################################################################## */
#define zcc_offsetof(type, member) ((size_t) &((type *)0)->member)
#define zcc_container_of(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - zcc_offsetof(app_type,member)))

#ifndef zcc_pthread_lock
#define zcc_pthread_lock(l)   {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zcc_fatal("mutex:%m");}}
#define zcc_pthread_unlock(l) {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zcc_fatal("mutex:%m");}}
#endif

/* ################################################################## */
const size_t var_size_max = 18446744073709551615UL;
const long var_long_max = 9223372036854775807L;
const int var_fd_max = 102400;
static inline bool empty(const void *ptr)
{
    return ((!ptr)||(!(*(const char *)(ptr))));
}
#define zcc_empty(ptr) ((!ptr)||(!(*(const char *)(ptr))))

typedef union type_convert_t type_convert_t;
union type_convert_t {
    const void *ptr_const_void;
    const char *ptr_const_char;
    void * ptr_void;
    char * ptr_char;
    long n_long;
    double n_double;
    int n_int;
};
#define zcc_char_ptr_to_int(_ptr, _int)  {zcc::type_convert_t _ct;_ct.ptr_char=(_ptr);_int=_ct.n_int;}
#define zcc_int_to_char_ptr(_int, _ptr)  {zcc::type_convert_t _ct;_ct.n_int=(_int);_ptr=_ct.ptr_char;}

#define zcc_str_n_case_eq(a, b, n)  ((zcc_char_tolower((a)[0])==zcc_char_tolower((b)[0])) && (!strncasecmp(a,b,n)))
#define zcc_str_case_eq(a, b)       ((zcc_char_tolower((a)[0])==zcc_char_tolower((b)[0])) && (!strcasecmp(a,b)))
#define zcc_str_n_eq(a, b, n)       (((a)[0] == (b)[0]) && (!strncmp(a,b,n)))
#define zcc_str_eq(a, b)            (((a)[0] == (b)[0]) && (!strcmp(a,b)))

typedef struct size_data_t size_data_t;
struct size_data_t {
    unsigned int size;
    char *data;
};

inline unsigned int int_unpack(const char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    n <<= 8; n |= p[3];
    return n;
}

inline void int_pack(int num, char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[3] = num & 255;
    num >>= 8; p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

inline unsigned int int_unpack3(const char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    unsigned n = p[0];
    n <<= 8; n |= p[1];
    n <<= 8; n |= p[2];
    return n;
}

inline void int_pack3(int num, char *buf)
{
    unsigned char *p = (unsigned char *)buf;
    p[2] = num & 255;
    num >>= 8; p[1] = num & 255;
    num >>= 8; p[0] = num & 255;
}

/* mlink  ############################################################ */
#define zcc_mlink_append(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define zcc_mlink_prepend(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else {_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define zcc_mlink_attach_before(head, tail, node, prev, next, before) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node, _before_1106 = before;\
    if(_head_1106 == 0){_node_1106->prev=_node_1106->next=0;_head_1106=_tail_1106=_node_1106;}\
    else if(_before_1106==0){_tail_1106->next=_node_1106;_node_1106->prev=_tail_1106;_node_1106->next=0;_tail_1106=_node_1106;}\
    else if(_before_1106==_head_1106){_head_1106->prev=_node_1106;_node_1106->next=_head_1106;_node_1106->prev=0;_head_1106=_node_1106;}\
    else {_node_1106->prev=_before_1106->prev; _node_1106->next=_before_1106; _before_1106->prev->next=_node_1106; _before_1106->prev=_node_1106;}\
    head = _head_1106; tail = _tail_1106; \
}
#define zcc_mlink_detach(head, tail, node, prev, next) {\
    typeof(head) _head_1106=head, _tail_1106=tail, _node_1106 = node;\
    if(_node_1106->prev){ _node_1106->prev->next=_node_1106->next; }else{ _head_1106=_node_1106->next; }\
    if(_node_1106->next){ _node_1106->next->prev=_node_1106->prev; }else{ _tail_1106=_node_1106->prev; }\
    head = _head_1106; tail = _tail_1106; \
}

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

#define zcc_rbtree_walk_begin(root, var_your_node)     {                    \
    zcc::rbtree_node_t * var_your_node; \
    for (var_your_node = rbtree_first(root); var_your_node; var_your_node = rbtree_next(var_your_node)) {
#define zcc_rbtree_walk_end                }}

#define zcc_rbtree_walk_back_begin(root, var_your_node)        {                    \
    zcc::rbtree_node_t * var_your_node; \
    for (var_your_node = rbtree_last(root); var_your_node; var_your_node = rbtree_prev(var_your_node)) {
#define zcc_rbtree_walk_back_end                   }}

/* malloc ################################################## */ 
extern char *blank_buffer;
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(const void *ptr, size_t size);
void free(const void *ptr);
char *strdup(const char *ptr);
char *strndup(const char *ptr, size_t n);
char *memdup(const void *ptr, size_t n);
char *memdupnull(const void *ptr, size_t n);

/* autobuffer ############################################# */
class autobuffer
{
public:
    inline autobuffer() { data = 0; }
    inline ~autobuffer() { free(data); }
    char *data;
};

/* string ################################################## */
class string
{
public:
    string();
    string(const string &_x);
    string(const char *str);
    string(const char *str, size_t size);
    string(size_t n, int ch);
    string(size_t size);
    ~string();

    inline /* const */ char *c_str() const {___data[___size] = 0; return ___data;}
    inline size_t empty() const { return !___size; }
    inline size_t size() const { return ___size; }
    inline size_t length() const { return ___size; }
    inline size_t capability() const { return ___capacity; }
    inline void push_back(int ch){(___size<___capacity)?(___data[___size++]=ch):put_do(ch);}
    inline string &put(int ch) { push_back(ch); return *this; }
    inline void clear() { ___size = 0; }
    string &resize(size_t n);
    string &reserve(size_t n);
    inline string &truncate(size_t n) { return resize(n); }
    inline string &append(int ch) { push_back(ch); return *this; }
    string &append(string &s);
    string &append(const char *str);
    string &append(const char *str, size_t n);
    string &append(size_t n, int ch);
    string &printf_1024(const char *format, ...);

    inline string & operator=(const char *str) { clear(); return append(str); }
    inline string & operator=(const string &s) { clear(); return append(s.___data, s.___size); }
    inline string & operator+=(const char *str) { return append(str); }
    inline string & operator+=(const string &s) { return append(s.___data, s.___size); }
    inline string & operator+=(int ch) { return append(ch); }
    inline int operator[](size_t n) { return ___data[n]; }

    string &size_data_escape(const void *data, size_t n = 0);
    string &size_data_escape(int i);
    string &size_data_escape(long i);

private:
    void _init_buf(size_t size);
    int put_do(int ch);
private:
    char *___data;
    unsigned int ___size;
    unsigned int ___capacity;
};

/* ################################################################## */
/* log, 通用 */
extern string var_masterlog_listen;
extern bool var_log_fatal_catch;
extern bool var_log_debug_enable;
extern void (*log_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap);
void log_fatal(const char *source_fn, size_t line_number, const char *fmt, ...);
void log_info(const char *source_fn, size_t line_number, const char *fmt, ...);
#define zcc_fatal(fmt, args...) { zcc::log_fatal(__FILE__, __LINE__, fmt, ##args); }
#define zcc_info(fmt, args...) { zcc::log_info(__FILE__, __LINE__, fmt, ##args); }

extern bool var_log_debug_enable;
#define zcc_debug(fmt,args...) { if(zcc::var_log_debug_enable){zcc_info(fmt, ##args);} }

void log_use_syslog(int facility, const char *identity);
void log_use_syslog(const char *facility, const char *identity);

void log_use_masterlog(const char *dest, const char *facility, const char *identity);

bool log_use_by_config(char *progname);

/* greedy_mem_pool ############################################## */
class gm_pool
{
public:
    gm_pool();
    ~gm_pool();
    void option_buffer_size(size_t single_buffer_size);
    void *malloc(size_t size);
    inline void *calloc(size_t nmemb, size_t size) { return memset(malloc(nmemb * size), 0, nmemb * size); }
    char *strdup(const char *ptr);
    char *strndup(const char *ptr, size_t n);
    char *memdup(const void *ptr, size_t n);
    char *memdupnull(const void *ptr, size_t n);
    void clear();
private:
    unsigned int ___single_buffer_size;
    unsigned int ___single_buffer_size_10percent;
    char *___head;
    char *___current;
    char *___sys_head;
    char *___sys_tail;
};

/* piece_mem_pool ############################################## */
typedef struct ___piece_group_t ___piece_group_t;
class pm_pool
{
public:
    pm_pool();
    ~pm_pool();
    void option_piece_size(size_t piece_size, size_t element_count_of_group = 0);
    void *require();
    void release(const void *);
private:
    unsigned short int ___element_size;
    unsigned short int ___element_count_of_group;
    unsigned int ___element_used_count;
    unsigned int ___group_count;
    ___piece_group_t *___unused_piece_group;
    ___piece_group_t *___space_piece_group_head;
    ___piece_group_t *___space_piece_group_tail;
    rbtree_t ___piece_group_tree;
};

/* vector ########################################################## */
void vector_reserve(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_resize(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_erase(size_t, unsigned int *, unsigned int *, char **, size_t);
void vector_pop(size_t, unsigned int *, unsigned int *, char **, char **v);
template <typename T>
class vector
{
public:
    inline vector() { _c = _s = 0; _d = 0; }
    inline ~vector() { free(_d); }
    inline T &operator[](size_t n) const { return _d[n]; }
    inline void clear() { _s = 0; }
    inline size_t size() const { return _s; }
    inline void push_back(const T v){if(_c==_s){vector_reserve(sizeof(T),&_c,&_s,(char **)&_d, _c);}_d[_s++]=v;}
    inline bool pop(T *v) {if(_s<1)return false;if(v)*v=_d[_s-1];_s--;return true;}
    inline void reserve(size_t size) { vector_reserve(sizeof(T), &_c, &_s, (char **)&_d, size); }
    inline void resize(size_t size) { vector_resize(sizeof(T), &_c, &_s, (char **)&_d, size); }
    inline void erase(size_t n) { vector_erase(sizeof(T), &_c, &_s, (char **)&_d, n); }
private:
    unsigned int _c; /* capacity */
    unsigned int _s; /* size */
    T *_d; /* data */
};
#define zcc_vector_walk_begin(var_your_vec, var_your_ptr) { \
    typeof(var_your_vec) &___V_VEC = (var_your_vec); \
    typeof(___V_VEC[0]) var_your_ptr; \
     size_t var_zcc_vector_opti = 0, ___C_VEC=(___V_VEC).size(); \
    for (; var_zcc_vector_opti < ___C_VEC; var_zcc_vector_opti ++) { \
        var_your_ptr = (___V_VEC)[var_zcc_vector_opti]; {
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

/* cstr ######################################################*/
extern unsigned const char lowercase_map[];
extern unsigned const char uppercase_map[];

#define zcc_char_tolower(c)    ((int)zcc::lowercase_map[(unsigned char )(c)])
#define zcc_char_toupper(c)    ((int)zcc::uppercase_map[(unsigned char )(c)])
static inline int tolower(int c) { return zcc_char_tolower(c); }
static inline int toupper(int c) { return zcc_char_toupper(c); }
char *tolower(const char *str);
char *toupper(const char *str);
char *trim_left(char *str);
char *trim_right(char *str);
char *trim(char *str);
size_t skip(const char *line, size_t size, const char *ignores_left, const char *ignores_right, char **start);
char *skip(const char *str, size_t size, const char *ignores);
char *skip_right(const char *str, size_t size, const char *ignores);
char *find_delim(const char *str, size_t size, const char *delims);
bool to_bool(const char *s, bool def);
long to_second(const char *s, long def);
long to_size(const char *s, long def);
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
    inline char *ptr() { return (const_cast <char *> (next_ptr)); }
    inline int size() { return next_len; }
private:
    const char *your_str;
    const char *next_ptr;
    int next_len;
};

size_t vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
char *multi_strdup(size_t *offsets, size_t count, const char *first, ...);
class stringsdup
{
public:
    stringsdup();
    ~stringsdup();
    void push_back(const char *v);
    void push_back(const char *v, size_t size);
    void clear();
    size_t count();
    char *dup();
    vector<size_t> &offsets();
private:
    string *___data;
    vector<size_t> ___offsets;
};

/* argv ############################################################ */
class argv
{
public:
    argv();
    ~argv();
    inline size_t size() const { return ___size; }
    inline char ** data() const { return ___data; }
    inline char * operator[](size_t n) const { return (char *)(___data[n]); }
    void push_back(const string &v);
    void push_back(const char *v);
    void push_back(const char *v, size_t n);
    void truncate(size_t n);
    void clear();
    void split(const char *str, const char *delim);
    void debug_show();
private:
    void ___append(const char *v);
    unsigned int ___capacity;
    unsigned int ___size;
    char **___data;
};
#define zcc_argv_walk_begin(var_your_argv, var_your_value) { \
    char * var_your_value; \
    typeof(var_your_argv) &___V_ARGV = (var_your_argv); \
    for (size_t var_zcc_argv_opti = 0; var_zcc_argv_opti < ___V_ARGV.size(); var_zcc_argv_opti++) { \
        var_your_value = (char *)(___V_ARGV[var_zcc_argv_opti]); {
#define zcc_argv_walk_end }}}

/* dict ############################################################ */
class dict
{
public:
    struct node_t {
        char *key;
        char *value;
        rbtree_node_t rbnode;
    };
    typedef struct node_t node_t;
    class node {
    public:
        inline node() {}
        inline ~node() {}
        inline char *key() { return ___data.key; }
        inline char *value() { return ___data.value; }
        node *prev();
        node *next();
    private:
        node_t ___data;
    };
public:
    dict();
    ~dict();
    inline size_t size() { return ___size; }
    inline size_t length() { return ___size; }
    void clear();
    node *update(const char *key, const char *value, size_t len = var_size_max);
    void update(node *n, const char *value, size_t len = var_size_max);
    bool exists(const char *key) { return find(key)?true:false; }
    void erase(const char *key);
    void erase(node *n);
    node *find(const char *key, char **value=0);
    node *find_near_prev(const char *key, char **value=0);
    node *find_near_next(const char *key, char **value=0);
    node *first_node();
    node *last_node();
    void debug_show();
    /* extend */
    char *get_str(const char *key, const char *def = blank_buffer);
    bool get_bool(const char *key, bool def);
    int get_int(const char *key, int def, int min, int max);
    long get_long(const char *key, long def, long min, long max);
    long get_second(const char *key, long def, long min, long max);
    long get_size(const char *key, long def, long min, long max);
    void parse_url_query(const char *query);
    char *build_url_query(string &query, bool strict = true);
private:
    rbtree_t ___rbtree;
    unsigned int ___size;
};

#define zcc_dict_walk_begin(var_your_dict, var_your_key_ptr, var_your_value_ptr) { \
    typeof(var_your_dict) &___V_DICT = (var_your_dict); \
    char * var_your_key_ptr, * var_your_value_ptr; \
    typeof(___V_DICT.first_node()) var_zcc_dict_node = ___V_DICT.first_node(); \
    typeof(___V_DICT.first_node()) var_zcc_dict_node_next; \
    for (; var_zcc_dict_node; var_zcc_dict_node = var_zcc_dict_node_next) { \
        var_zcc_dict_node_next = var_zcc_dict_node->next(); \
        var_your_key_ptr = var_zcc_dict_node->key(); (void)var_your_key_ptr; \
        var_your_value_ptr = var_zcc_dict_node->value(); (void)var_your_value_ptr; {
#define zcc_dict_walk_end }}}

/* map INNER USE ################################################## */
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

/* set INNER USE ################################################### */
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

/* size_data ######################################################## */
ssize_t size_data_unescape(const void *src_data, size_t src_size, char **result_data, size_t *result_size);
size_t size_data_unescape_all(const void *src_data, size_t src_size, size_data_t *vec, size_t sdsize);
size_t size_data_put_size(size_t size, char *buf);
class size_data_parser
{
public:
    size_data_parser(const void *src_data, size_t src_size);
    ~size_data_parser();
    int shift(const char **data, size_t *size);
    int shift(string &data);
private:
    char *ptr;
    size_t left;
};

/* encode ########################################################### */
typedef unsigned char encode_type;

const encode_type var_encode_none = 0;
const encode_type var_encode_base64 = 1;
const encode_type var_encode_base32 = 2;
const encode_type var_encode_qp = 3;
const encode_type var_encode_unknown = 126;

ssize_t base64_encode(const void *src, size_t src_size, string &dest, bool mime_flag = false);
ssize_t base64_decode(const void *src, size_t src_size, string &dest, size_t *dealed_size = 0);
ssize_t base64_encode_get_min_len(size_t in_len, bool mime_flag = false);
ssize_t base64_decode_get_valid_len(const void *src, size_t src_size);
class base64_decoder
{
public:
    base64_decoder();
    ~base64_decoder();
    ssize_t decode(const void *src, size_t src_size, string &str);
private:
    string tmpstring;
    char leftbuf[9];
};

ssize_t qp_decode_2045(const void *src, size_t src_size, string &dest);
ssize_t qp_decode_2047(const void *src, size_t src_size, string &dest);
ssize_t qp_decode_get_valid_len(const void *src, size_t src_size);

extern char hex_to_dec_table[];
ssize_t hex_encode(const void *src, size_t src_size, string &dest);
ssize_t hex_decode(const void *src, size_t src_size, string &dest);
ssize_t url_hex_decode(const void *src, size_t src_size, string &str);

size_t ncr_decode(size_t ins, char *wchar);

/* crc32 crc64 ###################################################### */
unsigned int get_crc32_result(const void *data, size_t size, unsigned int init_value = 0);
unsigned long get_crc64_result(const void *data, size_t size, unsigned long init_value = 0);

/* time ############################################################ */
long timeout_set(long timeout);
long timeout_left(long timeout);
void msleep(long delay);
void sleep(long delay);

/* date ############################################################ */
char *build_rfc1123_date_string(long t, char *buf);

/* dns ############################################################ */
ssize_t get_localaddr(char * addr_list, size_t max_count);
ssize_t get_hostaddr(const char *host, char * addr_list, size_t max_count);
bool get_peername(int sockfd, int *host, int *port);
bool get_peername(int sockfd, char *host, int *port);
bool get_ipstring(int ip, char *str);

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

class config:public dict
{
public:
    config();
    ~config();
    bool load_by_filename(const char *filename);
    /* table */
    void get_str_table(config_str_table_t * table);
    void get_int_table(config_int_table_t * table);
    void get_long_table(config_long_table_t * table);
    void get_bool_table(config_bool_table_t * table);
    void get_second_table(config_second_table_t * table);
    void get_size_table(config_size_table_t * table);
};
extern config default_config;

/* system ########################################################### */
bool chroot_user(const char *root_dir, const char *user_name);

/* mime types */
extern const char *var_mime_type_application_cotec_stream;
const char *mime_type_from_suffix(const char *suffix, const char *def = blank_buffer);
const char *mime_type_from_filename(const char *filename, const char *def = blank_buffer);

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

/* license ########################################################## */
bool license_mac_check(const char *salt, const char *license);
bool license_mac_check_by_config(const char *salt, const char *config_file, const char *key);
void license_mac_build(const char *salt, const char *_mac, char *rbuf); /* char rbuf[16+1]; */

/* main main_parameter ################################################## */
extern int var_main_argc_start;
extern char *var_progname;
extern char *var_listen_address;
extern char *var_module_name;
extern bool var_test_mode;
extern long var_proc_timeout;
extern bool var_proc_stop;

extern void (*show_usage) (const char *ctx);
int main_parameter_run(int argc, char **argv);
void main_parameter_run_over();
void main_parameter_fatal(char *arg);

#define zcc_main_parameter_begin() { zcc::var_progname = argv[0]; \
    int opti, ___optret_123, optval_count; char *optname, *optval; \
    for (opti = zcc::var_main_argc_start; opti < argc;) { \
        optname = argv[opti]; optval = 0; optval_count = 0;\
        ___optret_123 = zcc::main_parameter_run(argc-opti, argv+opti); \
        if (___optret_123 > 0) { opti += ___optret_123; continue; } \
        if (___optret_123 < 0) { zcc::main_parameter_fatal(argv[opti]); } \
        if (opti+1 < argc) { optval = argv[opti+1];} \
        for(optval_count = 0; opti + 1 + optval_count < argc; optval_count++) { \
            if(argv[opti + 1+ optval_count][0] == '-') break; \
        } (void)optname; (void)optval; (void)opti; (void)optval_count; {
#define zcc_main_parameter_end      } zcc::main_parameter_fatal(argv[opti]);} \
    zcc::main_parameter_run_over(); }

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
int timed_wait_readable(int fd, long timeout);
ssize_t timed_read(int fd, void *buf, size_t size, long timeout);
ssize_t timed_readn(int fd, void *buf, size_t size, long timeout);
int timed_wait_writeable(int fd, long timeout);
ssize_t timed_write(int fd, const void *buf, size_t size, long timeout);
ssize_t timed_writen(int fd, const void *buf, size_t size, long timeout);

/* tcp socket ##################################################### */
const int var_tcp_listen_type_inet = 'i';
const int var_tcp_listen_type_unix = 'u';
const int var_tcp_listen_type_fifo = 'f';
int unix_accept(int fd);
int inet_accept(int fd);
int accept(int sock, int type = var_tcp_listen_type_inet);
int unix_listen(char *addr, int backlog = 5);
int inet_listen(const char *sip, int port, int backlog = 5);
int listen(const char *netpath, int *type = 0, int backlog = 5);
int fifo_listen(const char *path);
int unix_connect(const char *addr);
int inet_connect(const char *dip, int port);
int host_connect(const char *host, int port);
int connect(const char *netpath);

/* openssl ######################################################## */
extern bool var_openssl_debug;
void openssl_init(void);
void openssl_fini(void);
void openssl_phtread_fini(void);
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
    inline bool is_error() { return ___error; }
    inline bool is_eof() { return ___eof; }
    inline bool is_exception() { return ___error || ___eof; }
    /* read */
    inline int get() {
        return ((read_buf_p1<read_buf_p2)?(read_buf[read_buf_p1++]):(get_char_do()));
    }
    ssize_t readn(void *buf, size_t size);
    ssize_t readn(string &str, size_t size);
    ssize_t gets(void *buf, size_t size, int delimiter='\n');
    ssize_t gets(string &str, int delimiter = '\n');
    ssize_t size_data_get_size();
    /* write */
    inline stream &put(int ch) {
        write_buf[write_buf_len++]=ch; ___flushed = false;
        (write_buf_len<stream_write_buf_size)?(1):(flush());
        return *this;
    };
    bool flush();
    stream &write(const void *buf, size_t size);
    stream &puts(const char *str);
    stream &printf_1024(const char *format, ...);
private:
    int get_char_do();
    int read_buf_p1:16;
    int read_buf_p2:16;
    unsigned short int write_buf_len;
    long ___timeout;
    char read_buf[stream_read_buf_size + 1];
    char write_buf[stream_write_buf_size + 1];
    bool ___error;
    bool ___eof;
    bool ___flushed;
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

/* tls stream ################################################## */
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

/* lock ####################################################### */
class locker
{
public:
    inline locker() {}
    inline virtual ~locker() {};
    virtual void rlock() = 0;
    virtual void wlock() = 0;
    inline void lock() { wlock(); }
    virtual void unlock() = 0;
};

locker *pthread_locker_create();
void pthread_locker_free(locker *lock);

/* event ####################################################### */
class event_io;
class async_io;
class event_timer;
class event_base;
extern event_base default_evbase;

class event_io
{
public:
    event_io();
    ~event_io();
    void bind(int fd, event_base &evbase = default_evbase);
    void option_local();
    ssize_t get_result();
    int get_fd();
    void enable_read(void (*callback)(event_io &));
    void enable_write(void (*callback)(event_io &));
    void disable();
    void set_context(const void *ctx);
    void * get_context();
    event_base &get_event_base();
    char ___data[31];
};

class async_io
{
public:
    async_io();
    ~async_io();
    void option_local();
    int get_result();
    int get_fd();
    void set_context(const void *ctx);
    void * get_context();
    void bind(int fd, event_base &eb = default_evbase);
    void tls_connect(SSL_CTX * ctx, void (*callback)(async_io &), long timeout);
    void tls_accept(SSL_CTX * ctx, void (*callback)(async_io &), long timeout);
    SSL *detach_SSL();
    void fetch_rbuf(char *buf, int len);
    void fetch_rbuf(string &dest, int len);
    void read(size_t max_len, void (*callback)(async_io &), long timeout);
    void readn(size_t strict_len, void (*callback)(async_io &), long timeout);
    void read_size_data(void (*callback)(async_io &), long timeout);
    void read_delimiter(int delimiter, size_t max_len, void (*callback)(async_io &), long timeout);
    void read_line(size_t max_len, void (*callback)(async_io &), long timeout);
    void cache_printf_1024(const char *fmt, ...);
    void cache_puts(const char *s);
    void cache_write(const void *buf, size_t len);
    void cache_write_size_data(const void *buf, size_t len);
    void cache_write_direct(const void *buf, size_t len);
    void cache_flush(void (*callback)(async_io &), long timeout);
    size_t get_cache_size();
    void sleep(void (*callback)(async_io &), long timeout);
    event_base &get_event_base();
    char ___data[122];
};

class event_timer
{
public:
    event_timer();
    ~event_timer();
    void bind(event_base &eb = default_evbase);
    void start(void (*callback)(event_timer &), long timeout);
    void stop();
    void option_local();
    void set_context(const void *ctx);
    void * get_context();
    event_base &get_event_base();
    char ___data[57];
};

class event_base
{
public:
    event_base();
    ~event_base();
    void set_context(const void *ctx);
    void * get_context();
    void notify();
    void option_local();
    void dispatch(long delay = 1000);
    char ___data[117];
};

void async_io_list_append(async_io **list_head, async_io **list_tail, async_io *aio);
void async_io_list_detach(async_io **list_head, async_io **list_tail, async_io *aio);

/* iopipe ########################################################### */
typedef void (*iopipe_after_close_fn_t) (void *);
class iopipe
{
public:
    iopipe();
    ~iopipe();
    void option_after_peer_closed_timeout(long timeout);
    size_t get_count();
    void run();
    void stop_notify();
    void enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl);
    void enter(int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, iopipe_after_close_fn_t after_close, void *context);
private:
    void *___data;
};


/* coroutine ########################################################## */
class coroutine;
typedef struct coroutine_mutex_t coroutine_mutex_t;
typedef struct coroutine_cond_t coroutine_cond_t;
void coroutine_base_init();
void coroutine_base_loop();
void coroutine_base_stop_notify();
void coroutine_base_fini();
void coroutine_go(void *(*start_job)(void *ctx), void *ctx);
coroutine * coroutine_self();
void coroutine_yield(coroutine *co = 0);
void coroutine_exit(coroutine *co = 0);
void coroutine_sleep(long s);
void coroutine_msleep(long ms);
void *coroutine_get_context(coroutine *co = 0);
void coroutine_set_context(coroutine *co, const void *ctx);
void coroutine_set_context(const void *ctx);
void coroutine_enable_fd(int fd);
void coroutine_disable_fd(int fd);
coroutine_mutex_t * coroutine_mutex_create();
void coroutine_mutex_free(coroutine_mutex_t *);
void coroutine_mutex_lock(coroutine_mutex_t *);
void coroutine_mutex_unlock(coroutine_mutex_t *);
coroutine_cond_t * coroutine_cond_create();
void coroutine_cond_free(coroutine_cond_t *);
void coroutine_cond_wait(coroutine_cond_t *, coroutine_mutex_t *);
void coroutine_cond_signal(coroutine_cond_t *);
void coroutine_cond_broadcast(coroutine_cond_t *);

/* master ############################################################# */
const int var_master_server_status_fd = 3;
const int var_master_master_status_fd = 4;
const int var_master_server_listen_fd = 5;
class master
{
public:
    master();
    ~master();
    void run(int argc, char **argv);
    void load_server_config_from_dir(const char *config_path, vector<config *> &cfs);
    virtual void load_server_config(vector<config *> &cfs);
    virtual void before_service();
    virtual void before_service_for_enduser();
    virtual void event_loop();
    void set_reload_signal(int sig);
};

/* master_event_server */
class master_event_server
{
public:
    master_event_server();
    ~master_event_server();
    virtual void before_service();
    virtual void before_service_for_enduser();
    virtual void event_loop();
    virtual void before_exit();
    virtual void simple_service(int fd);
    virtual void service_register(const char *service_name, int fd, int fd_type);
    event_io *general_service_register(int sock, int sock_type , void (*service) (int after_accept_fd)
            , event_base &eb = default_evbase);
    void run(int argc, char **argv);
    static void stop_notify();
private:
    void clear();
    void alone_register(char *urls);
    void inner_service_register(char *s, char *optval);
    void run_begin(int argc, char **argv);
    void run_loop();
    void run_over();
};

/* master_coroutine_server */
class master_coroutine_server
{
public:
    master_coroutine_server();
    ~master_coroutine_server();
    virtual void before_service();
    virtual void before_service_for_enduser();
    virtual void before_exit();
    virtual void service_register(const char *service_name, int fd, int fd_type) = 0;
    void run(int argc, char **argv);
    static void stop_notify();
private:
    void alone_register(char *urls);
    void run_begin(int argc, char **argv);
    void run_loop();
    void run_over();
};

/* charset ############################################################ */
extern bool var_charset_debug;

ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, char *dest, size_t dest_len
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count);

ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, string &dest
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count);

char *charset_correct_charset(const char *charset);
bool charset_detect(const char *data, size_t size, char *charset_result, const char **charset_list);
bool charset_detect_cjk(const char *data, size_t size, char *charset_result);
extern const char *charset_chinese[];
extern const char *charset_japanese[];
extern const char *charset_korean[];
extern const char *charset_cjk[];

/* finder ############################################################ */
class basic_finder
{
public:
    basic_finder();
    inline virtual ~basic_finder() {}
    virtual bool open(const char *url) = 0;
    virtual ssize_t find(const char *query, string &result, long timeout) = 0;
    inline virtual void disconnect() { }
    bool parse_url(const char *url, string &destination, dict &parameters);
};

class finder
{
public:
    finder();
    ~finder();
    bool open(const char *url);
    void close();
    ssize_t find(const char *query, string &result, long timeout);
    inline void disconnect() {if (___fder) ___fder->disconnect(); }
    inline void option_uppercase() { ___uppercase = true; ___lowercase = false; }
    inline void option_lowercase() { ___lowercase = true; ___uppercase = false; }
private:
    basic_finder *___fder;
    bool ___uppercase;
    bool ___lowercase;
};

ssize_t finder_once(const char *url, const char *query, string &result, long timeout);
int finder_main(int argc, char **argv);

extern basic_finder *(*finder_create_extend_fn)(const char *method, const char *url);

/* cdb ############################################################## */
void debug_kv_show(const char *k, const char *v);
void debug_kv_show(const char *k, long v);

char *build_unique_filename_id(char *buf);

/* cdb ############################################################## */
class cdb
{
public:
    cdb();
    cdb(const char *db_fn);
    cdb(int fd);
    ~cdb();
    bool open(const char *db_fn);
    bool open(int fd);
    void close();
    bool find(const void *key, size_t klen, string &result);
    bool find(const void *key, size_t klen, char **val, size_t *vlen);
    inline int get_fd() { return ___fd; }
private:
    void *___db;
    int ___fd:31;
    bool ___need_close_fd;
};

class cdb_walker
{
public:
    cdb_walker(cdb &db);
    ~cdb_walker();
    bool get_data(char **key, size_t *klen, char **val, size_t *vlen);
    bool get_data(string &key, string &val);
    void clear();
private:
    unsigned char *___data;
    unsigned ___pos;
    unsigned ___end;
};

class cdb_make
{
public:
    cdb_make();
    ~cdb_make();
    bool start(const char *db_fn);
    bool start(int fd);
    bool update(const void *key, size_t klen, const void *val, size_t vlen);
    bool finish();
private:
    bool ___need_close_fd;
    int ___fd:31;
    void *___db;
};

/* mime utils ######################################################## */
class mail_parser_mime;
class mail_parser;

void mime_iconv(const char *from_charset, const char *data, size_t size, string &dest);
size_t mime_iconv(const char *from_charset, const char *data, size_t size, char *dest, size_t dest_size);

void mime_header_line_unescape(const char *in_str, size_t in_len, string &dest);
size_t mime_header_line_unescape(const char *in_str, size_t in_len, char *dest, size_t dest_size);
void mime_header_line_get_first_token(const char *in_str, size_t in_len, string &value);
size_t mime_header_line_get_first_token(const char *in_str, size_t in_len, char **value);
struct mime_header_line_element_t {
    char charset[32];
    char *data;
    unsigned int size;
    encode_type encode;
};
typedef struct mime_header_line_element_t mime_header_line_element_t;
size_t mime_header_line_get_elements(const char *in_str, size_t in_len
        , mime_header_line_element_t * vec, size_t ele_max_count);

void mime_header_line_get_utf8(const char *src_charset_def, const char *in_str, size_t in_len, string &dest);
void mime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_str, size_t in_len
        , string &dest , bool with_charset = true);

void mime_header_line_get_params(const char *in_str, size_t in_len, string &value, dict &params);
void mime_header_line_decode_content_type(const char *data, size_t len
        , char **val, size_t *v_len
        , char **boundary, size_t *b_len
        , char **charset, size_t *c_len
        , char **name, size_t *n_len);
void mime_header_line_decode_content_disposition(const char *data, size_t len
        , char **val, size_t *v_len
        , char **filename, size_t *f_len
        , string &filename_2231
        , bool *filename_2231_with_charset);
void mime_header_line_decode_content_transfer_encoding(const char *data, size_t len, char **val, size_t *v_len);

long mime_header_line_decode_date(const char *str);

class mime_address
{
public:
    mime_address();
    mime_address(const mime_address &_x);
    ~mime_address();
    inline const char *name() const { return ___name; }
    inline const char *address() const { return ___address; }
    inline const char *name_utf8() const { return ___name_utf8; }
    mime_address &update_name(const char *name);
    mime_address &update_address(const char *address);
    mime_address &update_name_utf8(const char *name_utf8);
    mime_address & operator=(const mime_address &_x);
    void set_values(const char *name, const char *address, const char *name_utf8);
private:
    char *___name;
    char *___address;
    char *___name_utf8;
    bool ___do_not_free;
};

class mime_address_parser
{
public:
    mime_address_parser();
    ~mime_address_parser();
    void parse(const char *line, size_t size);
    bool shift(string &name, string &address);
    bool shift(char **name, char **address);
private:
    char *___cache;
    char *___str;
    int ___len;
    bool ___over;
};
void mime_header_line_get_address(const char *in_str, size_t in_len, vector<mime_address *> &rvec);
void mime_header_line_get_address_utf8(const char *src_charset_def , const char *in_str, size_t in_len
        , vector<mime_address *> &rvec);


/* mime parser ##################################################### */
class mail_parser_inner;
class mail_parser_mime_inner;

class mail_parser_mime
{
public:
     mail_parser_mime(mail_parser_inner *parser);
    ~mail_parser_mime();
    const char *type();
    const char *encoding();
    const char *charset();
    const char *disposition();
    const char *show_name();
    const char *name();
    const char *name_utf8();
    const char *filename();
    const char *filename2231();
    bool filename2231_with_charset();
    const char *filename_utf8();
    const char *content_id();
    const char *boundary();
    const char *imap_section();
    size_t header_offset();
    size_t header_size();
    size_t body_offset();
    size_t body_size();
    bool tnef();
    mail_parser_mime * next();
    mail_parser_mime * child();
    mail_parser_mime * parent();
    const vector<size_data_t *> &header_line();
    /* sn == 0: first, sn == -1: last */
    size_t header_line(const char *header_name, char **data, int n = 0);
    bool header_line(const char *header_name, string &result, int n = 0);
    bool header_line(const char *header_name, vector<const size_data_t *> &vec);
    void decoded_content(string &dest);
    void decoded_content_utf8(string &dest);
/* private: */
    inline mail_parser_mime_inner *get_inner_data() { return ___data; }
private:
    mail_parser_mime_inner *___data;
};

class mail_parser
{
private:
    mail_parser(mail_parser &_x);
public:
    mail_parser();
    ~mail_parser();
    void option_mime_max_depth(size_t depth);
    void option_src_charset_def(const char *src_charset_def);
    void parse(const char *mail_data, size_t mail_data_len);
    void debug_show();
    const char *data();
    size_t size();
    size_t header_size();
    size_t body_offset();
    size_t body_size();
    const char *message_id();
    const char *subject();
    const char *subject_utf8();
    const char *date();
    long date_unix();
    const mime_address &from();
    const mime_address &from_utf8();
    const mime_address &sender();
    const mime_address &reply_to();
    const mime_address &receipt();
    const char *in_reply_to();
    const vector<mime_address *> &to();
    const vector<mime_address *> &to_utf8();
    const vector<mime_address *> &cc();
    const vector<mime_address *> &cc_utf8();
    const vector<mime_address *> &bcc();
    const vector<mime_address *> &bcc_utf8();
    const vector<char *> &references();
    const mail_parser_mime *top_mime();
    const vector<mail_parser_mime *> &all_mimes();
    const vector<mail_parser_mime *> &text_mimes();
    const vector<mail_parser_mime *> &show_mimes();
    const vector<mail_parser_mime *> &attachment_mimes();
    const vector<size_data_t *> &header_line();
    /* sn == 0: first, sn == -1: last */;
    size_t header_line(const char *header_name, char **data, int n = 0);
    bool header_line(const char *header_name, string &result, int n = 0);
    bool header_line(const char *header_name, vector<const size_data_t *> &vec);
/* private: */
    inline mail_parser_inner *get_inner_data() { return ___data; }
private:
    mail_parser_inner *___data;
};

/* tnef */
typedef struct tnef_parser_mime_t tnef_parser_mime_t;
typedef struct tnef_parser_t tnef_parser_t;
class tnef_parser_mime;
class tnef_parser;

class tnef_parser_mime
{
public:
    tnef_parser_mime(tnef_parser_t *parser);
    ~tnef_parser_mime();
    const char * type();
    const char * show_name();
    const char * filename();
    const char * filename_utf8();
    const char * content_id();
    size_t body_offset();
    size_t body_size();
private:
    tnef_parser_mime_t *___data;
};

class tnef_parser
{
public:
    tnef_parser();
    ~tnef_parser();
    void option_src_charset_def(const char *src_charset_def);
    void parse(const char *mail_data, size_t mail_data_len);
    const char *data();
    size_t size();
    const vector<tnef_parser_mime *> &all_mimes();
private:
    tnef_parser_t *___data;
};

/* http httpd ##################################################### */
class http_url
{
public:
    http_url();
    ~http_url();
    /* parse */
    http_url(const char *url);
    void clear();
    void parse(const char *url);
    char *get_scheme(const char *def_val = blank_buffer);
    char *get_destination();
    char *get_host();
    int get_port(int def_val = -1);
    char *get_path();
    char *get_query();
    char *get_query_variate(const char *name, const char *def_val = blank_buffer);
    dict &get_query_variate();
    char *get_fragment();
    void debug_show();
    char ___data[80];
};

void http_cookie_parse_request(dict &result, const char *raw_cookie);
void http_cookie_build(string &result, const char *name, const char *value, long expires = 0, const char *path = 0, const char *domain = 0, bool secure = false, bool httponly = false);

extern bool var_httpd_debug;
struct httpd_upload_file {
    char *name;
    char *filename;
    char *saved_filename;
    size_t size;
};
typedef struct httpd_upload_file httpd_upload_file;

void httpd_upload_file_parse_pthread();

class httpd
{
public:
    httpd();
    ~httpd();
    void bind(int sock);
    void bind(SSL *ssl);
    bool bind(int sock, SSL_CTX *sslctx, long timeout);
    bool run();
    virtual void handler();
    virtual void handler_after_request_header();
    void get_post_data_default();
    void set_exception();
    void stop();

    /* option */
    virtual long keep_alive_timeout();
    virtual long request_header_timeout();
    virtual long max_length_for_post();
    virtual char *tmp_path_for_post();
    virtual char *gzip_file_suffix();

    /* request */
    char *request_method();
    char *request_path();
    char *request_uri();
    char *request_version();
    char *request_header(const char *name, const char *def_val = blank_buffer);
    dict &request_header();
    char *request_query_variate(const char *name, const char *def_val = blank_buffer);
    dict &request_query_variate();
    char *request_post_variate(const char *name, const char *def_val = blank_buffer);
    dict &request_post_variate();
    char *request_cookie(const char *name, const char *def_val = blank_buffer);
    dict &request_cookie();
    vector<httpd_upload_file *> &upload_files();

    /* response completly*/
    virtual void response_304(const char *etag);
    virtual void response_404();
    virtual void response_500();
    void response_200(const char *data, size_t size);
    inline void response_200(const char *data) { response_200(data, strlen(data)); }
    void response_file(const char *filename, const char *content_type = 0);

    /* response header */
    void response_header_initialization(const char *initialization = 0);
    void response_header(const char *name, const char *value);
    void response_header(const char *name, long d);
    void response_header_content_type(const char *value, const char *charset = 0);
    void response_header_content_length(long length);
    void response_header_set_cookie(const char *name, const char *value, long expires = 0, const char *path = 0, const char *domain = 0, bool secure = false, bool httponly = false);
    void response_header_unset_cookie(const char *name);
    void response_header_over();

    /* response */
    bool response_flush();
    stream &get_stream();
    /* */
    char ___data[176];
};

/* sqlite3 */
class sqlite3_proxyd: public master_event_server
{
public:
    sqlite3_proxyd();
    ~sqlite3_proxyd();
    void simple_service(int fd);
    void before_service();
    void before_exit();
};

extern bool var_sqlite3_proxy_debug;
class sqlite3_proxy
{
public:
    sqlite3_proxy(const char *_destination, string *_cache = 0);
    ~sqlite3_proxy();
    bool log(const char *sql, size_t size, long timeout);
    bool exec(const char *sql, size_t size, long timeout);
    bool query(const char *sql, size_t size, long timeout);
    int get_row(size_data_t **row);
    inline const char *get_errmsg() { return cache->c_str(); }
    inline size_t get_column() { return ncolumns; }
private:
    bool connect();
    bool disconnect(bool tf = false);
    char *destination;
    iostream *fp;
    string *cache;
    short int ncolumns;
    bool cache_flag;
};

/* memkv ****************************************************************/
class memkvd: public master_event_server
{
public:
    memkvd();
    ~memkvd();
    void before_service();
    void simple_service(int fd);
};

class memkv
{
public:
    memkv(const char *_destination);
    ~memkv();
    int set(const char *partition, const char *key, const char *val, ssize_t vlen = -1);
    int set(const char *partition, const char *key, long val);
    int del(const char *partition, const char *key);
    int inc(const char *partition, const char *key, long num, long *result = 0);
    int clear(const char *partition = 0);
    int exists(const char *partition, const char *key);
    int get(const char *partition, const char *key, string &result);
    int get(const char *partition, const char *key, long *result);
private:
    int require(char op, const char *partition, const char *key, const char *val, ssize_t vlen, string *result);
    char *destination;
    iostream *fp;
};

/* json ##################################################### */
#pragma pack(push, 1)
class json;
typedef map<json *>::node json_object_walker;
const unsigned char json_type_null = 0;
const unsigned char json_type_string = 1;
const unsigned char json_type_long = 2;
const unsigned char json_type_double = 3;
const unsigned char json_type_object = 4;
const unsigned char json_type_array = 5;
const unsigned char json_type_bool = 6;
const unsigned char json_type_unknown = 7;
class json
{
public:
    json();
    json(const string &val);
    json(const char *val);
    json(const char *val, size_t size);
    json(long val);
    json(double val);
    json(bool val);
    ~json();
    bool load_by_filename(const char *filename);
    bool unserialize(const char *jstr);
    bool unserialize(const char *jstr, size_t jsize);
    void serialize(string &result, int flag = 0);
    inline int get_type() { return ___type; }
    inline bool is_string() { return ___type==json_type_string; }
    inline bool is_long() { return ___type==json_type_long; }
    inline bool is_double() { return ___type==json_type_double; }
    inline bool is_object() { return ___type==json_type_object; }
    inline bool is_array()  { return ___type==json_type_array; }
    inline bool is_bool()   { return ___type==json_type_bool; }
    inline bool is_null()   { return ___type==json_type_null; }
    /* */
    json *reset();
    json *used_for_bool();
    json *used_for_long();
    json *used_for_double();
    json *used_for_string();
    json *used_for_array();
    json *used_for_object();
    /* get */
    string *get_string_value();
    long *get_long_value();
    double *get_double_value();
    bool *get_bool_value();
    json * get_array_element(size_t idx);
    json * get_object_element(const char *key);
    json_object_walker *get_object_first_walker();
    json_object_walker *get_object_last_walker();
    size_t get_array_size();
    size_t get_object_size();
    /* set */
    void push_back_array(json *j);
    void set_array_element(size_t idx, json *j, json **old = 0);
    void set_object_element(const char *key, json *j, json **old = 0);
    /* */
    json * erase_array_key(size_t idx);
    json * erase_object_key(const char *key);
    /* */
    json * get_element_by_path(const char *path);
    json * get_element_by_path_vec(const char *path0, ...);
private:
    void clear_value();
    unsigned char ___type;
    union {
        bool b;
        long number_long;
        double number_double;
        char str[sizeof(string)];
        vector<json *> *v;
        map<json *> *m;
    } ___val;
};
#pragma pack(pop)

}

#pragma pack(pop)

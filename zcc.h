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

#include <string>
#include <vector>
#include <list>
#include <map>
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

#define _ZCC_SIZEOF_DEBUG(a,b) { \
    if((sizeof(a)-zcc_offsetof(a, ___data))!=sizeof(b)) { \
        printf("\nsizeof(%s)==%zd\n\n",#b,sizeof(b)); \
        exit(1); \
    } \
}

/* std */
#define std_vector_walk_begin(var_your_vec, var_your_ptr) { \
    typeof(var_your_vec) &___V_VEC = (var_your_vec); \
    for (typeof(___V_VEC.begin()) var_std_vector_iterator = ___V_VEC.begin(); \
            var_std_vector_iterator!=___V_VEC.end(); var_std_vector_iterator ++) { \
        typeof(*(___V_VEC.begin())) var_your_ptr = (*var_std_vector_iterator); {
#define std_vector_walk_end }}}

#define std_list_walk_begin(var_your_vec, var_your_ptr) { \
    typeof(var_your_vec) &___V_LIST = (var_your_vec); \
    for (typeof(___V_LIST.begin()) var_std_list_iterator = ___V_LIST.begin(); \
            var_std_list_iterator!=___V_LIST.end(); var_std_list_iterator ++) { \
        typeof(*(___V_LIST.begin())) var_your_ptr = (*var_std_list_iterator); {
#define std_list_walk_end }}}

#define std_map_walk_begin(var_your_map, var_your_first_ptr, var_your_value_ptr) { \
    typeof(var_your_map) &___V_MAP = (var_your_map); \
    typeof(___V_MAP.begin()) var_std_map_iterator = ___V_MAP.begin(); \
    for (; var_std_map_iterator!=___V_MAP.end(); var_std_map_iterator ++) { \
        typeof(___V_MAP.begin()->first)&var_your_first_ptr=var_std_map_iterator->first; \
        (void)var_your_first_ptr; \
        typeof(___V_MAP.begin()->second)&var_your_value_ptr=var_std_map_iterator->second; \
        (void)var_your_value_ptr; \
        {
#define std_map_walk_end }}}

#define std_map_update(var_your_map, var_your_map_first, var_your_map_second, var_your_map_second_old) { \
    var_your_map_second_old = 0;\
    typeof(var_your_map) &___V_MAP = (var_your_map); \
    typeof(var_your_map_first) &___V_MAP_FIRST = (var_your_map_first); \
    typeof(___V_MAP.begin()) var_std_map_iterator = ___V_MAP.find(___V_MAP_FIRST); \
    if (var_std_map_iterator == ___V_MAP.end()) { \
        ___V_MAP[___V_MAP_FIRST] = var_your_map_second; \
    } else {  \
        var_your_map_second_old = var_std_map_iterator->second; \
        var_std_map_iterator->second = var_your_map_second; \
    } \
}

namespace zcc
{

/* ################################################################## */
#define zcc_offsetof(type, member) ((size_t) &((type *)0)->member)
#define zcc_container_of(ptr,app_type,member) ((app_type *) (((char *) (ptr)) - zcc_offsetof(app_type,member)))

#define zcc_pthread_lock(l)   {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zcc_fatal("mutex:%m");}}
#define zcc_pthread_unlock(l) {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zcc_fatal("mutex:%m");}}

/* ################################################################## */
const size_t var_size_max = 18446744073709551615UL;
const long var_max_timeout = 1000L * 3600 * 24 * 365 * 10;
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

/* std::string extend ################################################### */
std::string &sprintf_1024(std::string &str, const char *fmt, ...);
std::string &tolower(std::string &str);
std::string &toupper(std::string &str);
std::string &size_data_escape(std::string &str, const void *data, size_t n = 0);
std::string &size_data_escape(std::string &str, int i);
std::string &size_data_escape(std::string &str, long i);

/* ################################################################## */
/* log, 通用 */
extern std::string var_masterlog_listen;
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

bool log_use_by(char *progname, char *log_info);

/* greedy_mem_pool ############################################## */
class gm_pool
{
public:
    gm_pool();
    ~gm_pool();
    void set_buffer_size(size_t single_buffer_size);
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
    void set_piece_size(size_t piece_size, size_t element_count_of_group = 0);
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
char *skip_left(const char *str, size_t size, const char *ignores);
static inline char *skip_left(const char *str, const char *ignores)
{
    return skip_left(str, strlen(str), ignores);
}
char *skip_right(const char *str, size_t size, const char *ignores);
static inline char *skip_right(const char *str, const char *ignores)
{
    return skip_right(str, strlen(str), ignores);
}
size_t skip(const char *line, size_t size, const char *ignores_left, const char *ignores_right, char **start);
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
    std::vector<size_t> &offsets();
private:
    std::string ___data;
    std::vector<size_t> ___offsets;
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
    void push_back(const std::string &v);
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
class dict: public std::map<std::string, std::string>
{
public:
    using std::map<std::string, std::string>::find;
    inline dict() {}
    inline ~dict() {}
    /* extend */
    bool find(const std::string &key, char **val);
    bool find(const char *key, char **val);
    void debug_show();
    char *get_str(const std::string &key, const char *def = blank_buffer);
    char *get_str(const char *key, const char *def = blank_buffer);
    bool get_bool(const std::string &key, bool def);
    bool get_bool(const char *key, bool def);
    int get_int(const std::string &key, int def, int min, int max);
    int get_int(const char *key, int def, int min, int max);
    long get_long(const std::string &key, long def, long min, long max);
    long get_long(const char *key, long def, long min, long max);
    long get_second(const std::string &key, long def, long min, long max);
    long get_second(const char *key, long def, long min, long max);
    long get_size(const std::string &key, long def, long min, long max);
    long get_size(const char *key, long def, long min, long max);
    void parse_url_query(const char *query);
    char *build_url_query(std::string &query, bool strict = true);
};

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
    int shift(std::string &data);
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

ssize_t base64_encode(const void *src, size_t src_size, std::string &dest, bool mime_flag = false);
ssize_t base64_decode(const void *src, size_t src_size, std::string &dest, size_t *dealed_size = 0);
ssize_t base64_encode_get_min_len(size_t in_len, bool mime_flag = false);
ssize_t base64_decode_get_valid_len(const void *src, size_t src_size);
class base64_decoder
{
public:
    base64_decoder();
    ~base64_decoder();
    ssize_t decode(const void *src, size_t src_size, std::string &str);
private:
    std::string tmpstring;
    char leftbuf[9];
};

ssize_t qp_decode_2045(const void *src, size_t src_size, std::string &dest);
ssize_t qp_decode_2047(const void *src, size_t src_size, std::string &dest);
ssize_t qp_decode_get_valid_len(const void *src, size_t src_size);

extern char hex_to_dec_table[];
ssize_t hex_encode(const void *src, size_t src_size, std::string &dest);
ssize_t hex_decode(const void *src, size_t src_size, std::string &dest);
ssize_t url_hex_decode(const void *src, size_t src_size, std::string &str);

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
char *build_rfc1123_date_string(long t, std::string &result);

/* dns ############################################################ */
ssize_t get_localaddr(std::list<std::string> &hosts, size_t max_count = 0);
ssize_t get_hostaddr(const char *host, std::list<std::string> &hosts, size_t max_count = 0);
bool get_peername(int sockfd, int *host, int *port);
bool get_peername(int sockfd, std::string &host, int *port);
bool get_ipstring(int ip, std::string &host);

/* device ######################################################### */
ssize_t get_mac_list(std::list<std::string> &mac_list);

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
    inline config() {}
    inline ~config() {}
    bool load_by_filename(const char *filename);
    void load_another(config &cf);
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
ssize_t file_get_contents(const char *filename, std::string &str);
ssize_t file_get_contents_sample(const char *filename, std::string &str);
ssize_t stdin_get_contents(std::string &str);
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
void license_mac_build(const char *salt, const char *_mac, std::string &license);

/* main main_parameter ################################################## */
extern char *var_progname;
extern bool var_proc_stop;

extern std::vector<char *> main_parameter_values;
void main_parameter_run(int argc, char **argv);

/* io ############################################################# */
/* return , -1: error, 0: not, 1: yes */
int is_rwable(int fd);
int is_readable(int fd);
int is_writeable(int fd);

/* */
bool flock(int fd, int flags);
bool flock_share(int fd);
bool flock_exclusive(int fd);
bool funlock(int fd);
bool nonblocking(int fd, bool no = true);
bool close_on_exec(int fd, bool on = true);
int get_readable_count(int fd);
ssize_t writen(int fd, const void *buf, size_t size);

/* timed_io ######################################################## */
/* -1: error, 0: not, 1: yes */
int timed_wait_readable(int fd, long timeout);

/* < 0: error, >=0: bytes of read */
ssize_t timed_read(int fd, void *buf, size_t size, long timeout);

/* strictly read n bytes */
ssize_t timed_readn(int fd, void *buf, size_t size, long timeout);

/* -1: error, 0: not, 1: yes */
int timed_wait_writeable(int fd, long timeout);

/* < 0: error, >=0: bytes of wrote */
ssize_t timed_write(int fd, const void *buf, size_t size, long timeout);

/* strictly write n btyes */
ssize_t timed_writen(int fd, const void *buf, size_t size, long timeout);

/* tcp socket ##################################################### */
const int var_tcp_listen_type_inet = 'i';
const int var_tcp_listen_type_unix = 'u';
const int var_tcp_listen_type_fifo = 'f';
/* <0: error for these funcs */
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
    stream(SSL *ssl);
    stream(int fd);
    ~stream();
    stream &open(SSL *ssl);
    stream &open(int fd);
    stream &close();
    stream &set_timeout(long timeout);
    inline long get_timeout() { return ___timeout; }
    stream &set_auto_close_fd(bool flag = true) { ___auto_release=flag; return *this; }
    inline bool is_error() { return ___error; }
    inline bool is_eof() { return ___eof; }
    inline bool is_exception() { return ___error || ___eof; }
    inline int get_fd() { return (___opened?(___ssl_mode?openssl_SSL_get_fd(___fd.ssl):___fd.fd):-1); }
    inline SSL *get_SSL() { return (___opened&&___ssl_mode)?___fd.ssl:0; }
    bool tls_connect(SSL_CTX *ctx);
    bool tls_accept(SSL_CTX *ctx);
    /* read */
    inline int get() {
        return ((read_buf_p1<read_buf_p2)?(read_buf[read_buf_p1++]):(get_char_do()));
    }
    ssize_t readn(void *buf, size_t size);
    ssize_t readn(std::string &str, size_t size);
    ssize_t gets(void *buf, size_t size, int delimiter='\n');
    ssize_t gets(std::string &str, int delimiter = '\n');
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
    inline stream &append(const char *str) { return puts(str); }
    inline stream &append(int i) {return printf_1024("%d", i);}
    inline stream &append(unsigned int i) {return printf_1024("%u", i);}
    inline stream &append(long i) {return printf_1024("%ld", i);}
    inline stream &append(unsigned long i) {return printf_1024("%lu", i);}
    inline stream &append(double i) {return printf_1024("%f", i);}
    inline stream &append(float i) {return printf_1024("%f", i);}
private:
    void init_all();
    int get_char_do();
    int read_buf_p1:16;
    int read_buf_p2:16;
    union {
        int fd;
        SSL *ssl;
    } ___fd;
    long ___timeout;
    unsigned short int write_buf_len;
    char read_buf[stream_read_buf_size + 1];
    char write_buf[stream_write_buf_size + 1];
    bool ___opened;
    bool ___auto_release;
    bool ___ssl_mode;
    bool ___ssl_opened_mode;
    bool ___error;
    bool ___eof;
    bool ___flushed;
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
    void set_local();
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
    void set_local();
    int get_result();
    int get_fd();
    void set_context(const void *ctx);
    void * get_context();
    void bind(int fd, event_base &eb = default_evbase);
    void tls_connect(SSL_CTX * ctx, void (*callback)(async_io &), long timeout = 0);
    void tls_accept(SSL_CTX * ctx, void (*callback)(async_io &), long timeout = 0);
    SSL *detach_SSL();
    void fetch_rbuf(char *buf, int len);
    void fetch_rbuf(std::string &dest, int len);
    void read(size_t max_len, void (*callback)(async_io &), long timeout = 0);
    void readn(size_t strict_len, void (*callback)(async_io &), long timeout = 0);
    void read_size_data(void (*callback)(async_io &), long timeout);
    void read_delimiter(int delimiter, size_t max_len, void (*callback)(async_io &), long timeout = 0);
    void read_line(size_t max_len, void (*callback)(async_io &), long timeout = 0);
    void cache_printf_1024(const char *fmt, ...);
    void cache_puts(const char *s);
    void cache_write(const void *buf, size_t len);
    void cache_write_size_data(const void *buf, size_t len);
    void cache_write_direct(const void *buf, size_t len);
    void cache_flush(void (*callback)(async_io &), long timeout = 0);
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
    void set_local();
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
    void set_local();
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
    void set_after_peer_closed_timeout(long timeout);
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

/* pthread job ###################################################### */
class tjob
{
public:
    typedef struct {
        void (*callback)(void *);
        void *context;
    } item;
public:
    tjob();
    ~tjob();
    void enter(void(*callback)(void *), void *context);
    bool get(void(**callback)(void *), void **context, long timeout);
    ssize_t size();
    bool empty();
private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::list<item> list;
};

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
    void load_server_config_from_dir(const char *config_path, std::list<config *> &cfs);
    virtual void load_server_config(std::list<config *> &cfs);
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
    void master_register(char *urls);
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
    void master_register(char *urls);
};

/* charset ############################################################ */
extern bool var_charset_debug;

ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, char *dest, size_t dest_len
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count);

ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, std::string &dest
        , size_t *src_converted_len
        , ssize_t omit_invalid_bytes_limit, size_t *omit_invalid_bytes_count);

inline ssize_t charset_iconv(const char *from_charset, const char *src, size_t src_len
        , const char *to_charset, std::string &dest) {
    return charset_iconv(from_charset, src, src_len, to_charset, dest, 0, -1, 0);
}

char *charset_correct_charset(const char *charset);
bool charset_detect(const char *data, size_t size, std::string &charset_result, const char **charset_list);
bool charset_detect_cjk(const char *data, size_t size, std::string &charset_result);
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
    virtual ssize_t find(const char *query, std::string &result, long timeout) = 0;
    inline virtual void disconnect() { }
};

class finder
{
public:
    finder();
    ~finder();
    bool open(const char *url);
    void close();
    ssize_t find(const char *query, std::string &result, long timeout);
    inline finder& disconnect() {if (___fder) ___fder->disconnect(); return *this; }
    inline finder& set_uppercase() { ___uppercase = true; ___lowercase = false; return *this; }
    inline finder& set_lowercase() { ___lowercase = true; ___uppercase = false; return *this; }
private:
    basic_finder *___fder;
    bool ___uppercase;
    bool ___lowercase;
};

ssize_t finder_once(const char *url, const char *query, std::string &result, long timeout);
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
    bool find(const void *key, size_t klen, std::string &result);
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
    bool get_data(std::string &key, std::string &val);
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

size_t mime_iconv(const char *from_charset, const char *data, size_t size, char *dest, size_t dest_size);
void mime_iconv(const char *from_charset, const char *data, size_t size, std::string &dest);

size_t mime_raw_header_line_unescape(const char *in_line, size_t in_len, char *dest, size_t dest_size);
void mime_raw_header_line_unescape(const char *in_line, size_t in_len, std::string &dest);

void mime_header_line_get_first_token(const char *in_line, size_t in_len, std::string &value);
size_t mime_header_line_get_first_token(const char *in_line, size_t in_len, char **value);

struct mime_header_line_element_t {
    char charset[32];
    char *data;
    unsigned int size;
    encode_type encode;
};
typedef struct mime_header_line_element_t mime_header_line_element_t;
size_t mime_header_line_get_elements(const char *in_line, size_t in_len
        , mime_header_line_element_t * vec, size_t ele_max_count);

void mime_header_line_get_utf8(const char *src_charset_def, const char *in_line, size_t in_len, std::string &dest);
void mime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, size_t in_len
        , std::string &dest , bool with_charset = true);

void mime_header_line_get_params(const char *in_line, size_t in_len, std::string &value, dict &params);
void mime_header_line_decode_content_type(const char *data, size_t len
        , std::string &value, std::string &boundary, std::string &charset, std::string &name);
void mime_header_line_decode_content_disposition(const char *data, size_t len
        , std::string &value, std::string &filename, std::string &filename_2231, bool *filename_2231_with_charset);
void mime_header_line_decode_content_transfer_encoding(const char *data, size_t len, std::string &value);
long mime_header_line_decode_date(const char *str);

class mime_address
{
public:
    inline mime_address() {}
    inline ~mime_address() {}
    std::string name;
    std::string address;
    std::string name_utf8;
};

class mime_address_parser
{
public:
    mime_address_parser();
    ~mime_address_parser();
    void parse(const char *line, size_t size);
    bool shift(std::string &name, std::string &address);
    bool shift(char **name, char **address);
private:
    char *___cache;
    char *___str;
    int ___len;
    bool ___over;
};
std::list<mime_address> mime_header_line_get_address(const char *in_str, size_t in_len);
void mime_header_line_get_address(const char *in_str, size_t in_len, std::list<mime_address> &rvec);

std::list<mime_address> mime_header_line_get_address_utf8(const char *src_charset_def,
        const char *in_str, size_t in_len);
void mime_header_line_get_address_utf8(const char *src_charset_def , const char *in_str, size_t in_len
        , std::list<mime_address> &rvec);


/* mime parser ##################################################### */
class mail_parser_engine;
class mail_parser_mime_engine;

class mail_parser_mime
{
public:
     mail_parser_mime(mail_parser_mime_engine *engine);
    ~mail_parser_mime();
    const std::string &type();
    const std::string &encoding();
    const std::string &charset();
    const std::string &disposition();
    const std::string &show_name();
    const std::string &name();
    const std::string &name_utf8();
    const std::string &filename();
    const std::string &filename2231();
    bool filename2231_with_charset();
    const std::string &filename_utf8();
    const std::string &content_id();
    const std::string &boundary();
    const std::string &imap_section();
    const char *header_data();
    size_t header_offset();
    size_t header_size();
    const char *body_data();
    size_t body_offset();
    size_t body_size();
    bool tnef();
    mail_parser_mime *next();
    mail_parser_mime *child();
    mail_parser_mime *parent();
    const std::list<size_data_t> &raw_header_line();
    /* n == 0: first, n == -1: last */
    size_t raw_header_line(const char *header_name, char **data, int n = 0);
    bool raw_header_line(const char *header_name, std::string &result, int n = 0);
    bool raw_header_line(const char *header_name, std::list<size_data_t> &vec);
    bool raw_header_line(const char *header_name, std::list<std::string> &vec);
    bool header_line_value(const char *header_name, std::string &result, int n = 0);
    bool header_line_value(const char *header_name, std::list<std::string> &vec);
    void decoded_content(std::string &dest);
    void decoded_content_utf8(std::string &dest);
private:
    mail_parser_mime_engine *___data;
};

class mail_parser
{
private:
    mail_parser(mail_parser &_x);
public:
    mail_parser();
    ~mail_parser();
    void set_mime_max_depth(size_t depth);
    void set_src_charset_def(const char *src_charset_def);
    void parse(const char *mail_data, size_t mail_data_len);
    bool parse(const char *filename);
    void debug_show();
    const char *data();
    size_t size();
    const char *header_data();
    size_t header_offset();
    size_t header_size();
    const char *body_data();
    size_t body_offset();
    size_t body_size();
    const std::string &message_id();
    const std::string &subject();
    const std::string &subject_utf8();
    const std::string &date();
    long date_unix();
    const mime_address &from();
    const mime_address &from_utf8();
    const mime_address &sender();
    const mime_address &reply_to();
    const mime_address &receipt();
    const std::string &in_reply_to();
    const std::list<mime_address> &to();
    const std::list<mime_address> &to_utf8();
    const std::list<mime_address> &cc();
    const std::list<mime_address> &cc_utf8();
    const std::list<mime_address> &bcc();
    const std::list<mime_address> &bcc_utf8();
    const std::list<std::string> &references();
    const mail_parser_mime *top_mime();
    const std::list<mail_parser_mime *> &all_mimes();
    const std::list<mail_parser_mime *> &text_mimes();
    const std::list<mail_parser_mime *> &show_mimes();
    const std::list<mail_parser_mime *> &attachment_mimes();
    const std::list<size_data_t> &raw_header_line();
    /* n == 0: first, n == -1: last */;
    size_t raw_header_line(const char *header_name, char **data, int n = 0);
    bool raw_header_line(const char *header_name, std::string &result, int n = 0);
    bool raw_header_line(const char *header_name, std::list<size_data_t> &vec);
    bool raw_header_line(const char *header_name, std::list<std::string> &vec);
    bool header_line_value(const char *header_name, std::string &result, int n = 0);
    bool header_line_value(const char *header_name, std::list<std::string> &vec);
private:
    void init();
    mail_parser_engine *___data;
};

/* tnef */
class tnef_parser_mime_engine;
class tnef_parser_engine;
class tnef_parser_mime;
class tnef_parser;

class tnef_parser_mime
{
public:
    tnef_parser_mime(tnef_parser_mime_engine *_engine);
    ~tnef_parser_mime();
    const std::string &type();
    const std::string &show_name();
    const std::string &filename();
    const std::string &filename_utf8();
    const std::string &content_id();
    size_t body_offset();
    size_t body_size();
private:
    tnef_parser_mime_engine *___data;
};

class tnef_parser
{
public:
    tnef_parser();
    ~tnef_parser();
    void set_src_charset_def(const char *src_charset_def);
    void parse(const char *mail_data, size_t mail_data_len);
    const char *data();
    size_t size();
    const std::list<tnef_parser_mime *> &all_mimes();
private:
    tnef_parser_engine *___data;
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
    char ___data[104];
};

void http_cookie_parse_request(dict &result, const char *raw_cookie);
void http_cookie_build(std::string &result, const char *name, const char *value, long expires = 0, const char *path = 0, const char *domain = 0, bool secure = false, bool httponly = false);

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
    std::list<httpd_upload_file *> &upload_files();

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
    char ___data[286];
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
    sqlite3_proxy(const char *_destination, std::string *_cache = 0);
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
    stream *fp;
    std::string *cache;
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
    int get(const char *partition, const char *key, std::string &result);
    int get(const char *partition, const char *key, long *result);
private:
    int require(char op, const char *partition, const char *key, const char *val, ssize_t vlen, std::string *result);
    char *destination;
    stream *fp;
};

/* json ##################################################### */
#pragma pack(push, 1)
class json;
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
    json(const std::string &val);
    json(const char *val);
    json(const char *val, size_t size);
    json(long val);
    json(double val);
    json(bool val);
    ~json();
    bool load_by_filename(const char *filename);
    bool unserialize(const char *jstr);
    bool unserialize(const char *jstr, size_t jsize);
    void serialize(std::string &result, int flag = 0);
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
    /* set value */
    json *set_string_value(const std::string &val);
    json *set_string_value(const char *val);
    json *set_string_value(const char *val, size_t size);
    json *set_long_value(long val);
    json *set_double_value(double val);
    json *set_bool_value(bool val);
    /* get */
    std::string &get_string_value();
    long &get_long_value();
    double &get_double_value();
    bool &get_bool_value();
    const std::vector<json *> &get_array_value();
    const std::map<std::string, json *> &get_object_value();
    json * array_get_element(size_t idx);
    json * object_get_element(const char *key);
    size_t array_get_size();
    size_t object_get_size();

    /*
     * @array_add_element 向array类型的json追加成员 j
     * @return_child 为true 返回 j, 否则 返回self.    下同...
     */
    json * array_add_element(json *j, bool return_child = false);

    /* 
     * @array_add_element 给键idx设置成员 j. 如果键idx存在则, 首先销毁对应的成员
     * 如: 已知 json [1, {}, "ss" "aaa"], 则:
     * 操作 array_add_element(2, j), 意味着 先销毁 "ss", 并替换为 j, 结果为:
     * [1, {}, j, "aaa"]
     */
    json * array_add_element(size_t idx, json *j, bool return_child = false);

    /* @object_add_element 如上 */
    json * object_add_element(const char *key, json *j, bool return_child = false);

    /* 
     * @array_add_element 给键idx设置成员 j. 
     * 如果键idx存在则, 则把idx对应的json赋给 *old, 如果old不存在, 则销毁
     */
    json * array_add_element(size_t idx, json *j, json **old, bool return_child = false);

    /* @object_add_element 如上 */
    json * object_add_element(const char *key, json *j, json **old, bool return_child = false);


#define ___zcc_json_add_element(TTT) \
    inline json * array_add_element(TTT val) { \
        return array_add_element(new json(val)); \
    } \
    inline json * array_add_element(size_t idx, TTT val, bool return_child = false) { \
        return array_add_element(idx, new json(val), return_child); \
    } \
    inline json * object_add_element(const char *key, TTT val, bool return_child = false) { \
        return object_add_element(key, new json(val), return_child); \
    } \
    inline json * array_add_element(size_t idx, TTT val, json **old, bool return_child = false) { \
        return array_add_element(idx, new json(val), old, return_child); \
    } \
    inline json * object_add_element(const char *key, TTT val, json **old, bool return_child = false) { \
        return object_add_element(key, new json(val), old, return_child); \
    }

    /* 
     * @___zcc_json_add_element
     * 这几组方法类似 array_add_element,object_add_element, 只不过参数不同
     */
    ___zcc_json_add_element(const std::string &);
    ___zcc_json_add_element(const char *);
    ___zcc_json_add_element(long);
    ___zcc_json_add_element(double);
    ___zcc_json_add_element(bool);
#undef ___zcc_json_add_element

    /*  @array_detach_element 删除索引为idx的键, 但是保留其值, 并返回这个值 */
    json * array_detach_element(size_t idx);

    /*  @object_detach_element 删除key键, 但是保留其值, 并返回这个值 */
    json * object_detach_element(const char *key);

    /*  @array_erase_element 删除索引为idx的键, 同时销毁其值, 并返回self */
    json * array_erase_element(size_t idx);

    /*  @object_erase_element 删除key键, 同时销毁其值, 并返回self */
    json * object_erase_element(const char *key);

    /*
     * @get_element_by_path 得到路径path对应的json值, 并返回
     * 如 已知json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}, 则
     * get_element_by_path("group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} 
     */
    json * get_element_by_path(const char *path);

    /*
     * @get_element_by_path_vec 如上 
     * get_element_by_path_vec("group", "linux", "2", "me", 0);
     */
    json * get_element_by_path_vec(const char *path0, ...);
    /* */
    inline json * get_parent() { return ___parent; }
    json * get_top();
private:
    unsigned char ___type;
    union {
        bool b;
        long number_long;
        double number_double;
        char str[sizeof(std::string)];
        std::vector<json *> *v;
        std::map<std::string, json *> *m;
    } ___val;
    json *___parent;
};
#pragma pack(pop)

}

#pragma pack(pop)

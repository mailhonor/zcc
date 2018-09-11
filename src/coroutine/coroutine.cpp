/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-06-26
 * ================================
 */

#pragma GCC diagnostic ignored "-Wredundant-decls"
#include "zcc.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#pragma pack(push, 4)
extern "C"
{
}

namespace zcc
{
/* {{{ syscall */
int syscall_pipe(int pipefd[2]);
int syscall_pipe2(int pipefd[2], int flags);
int syscall_dup(int oldfd);
int syscall_dup2(int oldfd, int newfd);
int syscall_dup3(int oldfd, int newfd, int flags);
int syscall_socketpair(int domain, int type, int protocol, int sv[2]);
int syscall_socket(int domain, int type, int protocol);
int syscall_accept(int fd, struct sockaddr *addr, socklen_t *len);
int syscall_connect(int socket, const struct sockaddr *address, socklen_t address_len);
int syscall_close(int fd);
ssize_t syscall_read(int fildes, void *buf, size_t nbyte);
ssize_t syscall_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t syscall_write(int fildes, const void *buf, size_t nbyte);
ssize_t syscall_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t syscall_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t syscall_recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);
size_t syscall_send(int socket, const void *buffer, size_t length, int flags);
ssize_t syscall_recv(int socket, void *buffer, size_t length, int flags);
int syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);
int syscall_setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
int syscall_fcntl(int fildes, int cmd, ...);
pid_t syscall_gettid(void);
int syscall_open(const char *pathname, int flags, ...);
int syscall_openat(int dirfd, const char *pathname, int flags, ...);
int syscall_creat(const char *pathname, mode_t mode);
off_t syscall_lseek(int fd, off_t offset, int whence);
int syscall_fdatasync(int fd);
int syscall_fsync(int fd);
int syscall_rename(const char *oldpath, const char *newpath);
int syscall_truncate(const char *path, off_t length);
int syscall_ftruncate(int fd, off_t length);
int syscall_rmdir(const char *pathname);
int syscall_mkdir(const char *pathname, mode_t mode);
int syscall_getdents(unsigned int fd, void *dirp, unsigned int count);
int syscall_stat(const char *pathname, struct stat *buf);
int syscall_fstat(int fd, struct stat *buf);
int syscall_lstat(const char *pathname, struct stat *buf);
int syscall_link(const char *oldpath, const char *newpath);
int syscall_symlink(const char *target, const char *linkpath);
ssize_t syscall_readlink(const char *pathname, char *buf, size_t bufsiz);
int syscall_unlink(const char *pathname);
int syscall_chmod(const char *pathname, mode_t mode);
int syscall_fchmod(int fd, mode_t mode);
int syscall_chown(const char *pathname, uid_t owner, gid_t group);
int syscall_fchown(int fd, uid_t owner, gid_t group);
int syscall_lchown(const char *pathname, uid_t owner, gid_t group);
int syscall_utime(const char *filename, const struct utimbuf *times);
int syscall_utimes(const char *filename, const struct timeval times[2]);
int syscall_futimes(int fd, const struct timeval tv[2]);
int syscall_lutimes(const char *filename, const struct timeval tv[2]);
/* }}} */
extern pthread_mutex_t *var_general_pthread_mutex;

static int var_coroutine_block_pthread_count_limit = 0;
void coroutine_set_block_pthread_limit(int limit) 
{
    var_coroutine_block_pthread_count_limit = limit;
}
static int var_coroutine_block_pthread_count_current = 0;

static void *coroutine_hook_fileio_worker(void *arg);

int syscall_poll(struct pollfd fds[], nfds_t nfds, int timeout);

/* ################################################################################# */
typedef struct coroutine_sys_context coroutine_sys_context;
typedef struct coroutine_base coroutine_base;
typedef union coroutine_hook_arg_t coroutine_hook_arg_t;
typedef struct coroutine_hook_fileio_t coroutine_hook_fileio_t;

typedef struct gethostbyname_buf_t gethostbyname_buf_t;
struct gethostbyname_buf_t 
{
	struct hostent host;
	char* buf_ptr;
	size_t buf_size;
	int h_errno2;
};

union coroutine_hook_arg_t {
    const void *const_void_ptr_t;
    void *void_ptr_t;
    const char *const_char_ptr_t;
    char *char_ptr_t;
    struct iovec *iovec_t;
    const struct iovec *const_iovec_t;
    long long_t;
    int int_t;
    unsigned uint_t;
    size_t size_size_t;
    ssize_t ssize_ssize_t;
    mode_t mode_mode_t;
    off_t off_off_t;
    uid_t uid_uid_t;
    gid_t gid_gid_t;
    void *(*block_func)(void *);
};
struct coroutine_hook_fileio_t {
    coroutine *current_coroutine;
    coroutine_hook_arg_t args[4];
    coroutine_hook_fileio_t *prev;
    coroutine_hook_fileio_t *next;
    zcc::coroutine_hook_arg_t retval;
    int co_errno;
    int cmdcode;
    unsigned int is_regular_file:1;
    unsigned int is_block_func:1;
};

static pthread_mutex_t var_coroutine_hook_fileio_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t var_coroutine_hook_fileio_cond = PTHREAD_COND_INITIALIZER;

static coroutine_hook_fileio_t *var_coroutine_hook_fileio_head = 0;
static coroutine_hook_fileio_t *var_coroutine_hook_fileio_tail = 0;
static int var_coroutine_hook_fileio_count = 0;

enum coroutine_hook_fileio_cmd_t {
    coroutine_hook_fileio_open = 1,
    coroutine_hook_fileio_openat,
    coroutine_hook_fileio_close,
    coroutine_hook_fileio_read,
    coroutine_hook_fileio_readv,
    coroutine_hook_fileio_write,
    coroutine_hook_fileio_writev,
    coroutine_hook_fileio_lseek,
    coroutine_hook_fileio_fdatasync,
    coroutine_hook_fileio_fsync,
    coroutine_hook_fileio_rename,
    coroutine_hook_fileio_truncate,
    coroutine_hook_fileio_ftruncate,
    coroutine_hook_fileio_rmdir,
    coroutine_hook_fileio_mkdir,
    coroutine_hook_fileio_getdents,
    coroutine_hook_fileio_stat,
    coroutine_hook_fileio_fstat,
    coroutine_hook_fileio_lstat,
    coroutine_hook_fileio_link,
    coroutine_hook_fileio_symlink,
    coroutine_hook_fileio_readlink,
    coroutine_hook_fileio_unlink,
    coroutine_hook_fileio_chmod,
    coroutine_hook_fileio_fchmod,
    coroutine_hook_fileio_chown,
    coroutine_hook_fileio_fchown,
    coroutine_hook_fileio_lchown,
    coroutine_hook_fileio_utime,
    coroutine_hook_fileio_utimes,
    coroutine_hook_fileio_unknown
};
typedef enum coroutine_hook_fileio_cmd_t coroutine_hook_fileio_cmd_t;

#define coroutine_hook_fileio_run_part0() \
    zcc::coroutine_base *cobs = 0; \
    if ((cobs = zcc::coroutine_base_get_current())==0)

#define coroutine_hook_fileio_run_part1() \
    zcc::coroutine_base *cobs = 0; \
    if ((zcc::var_coroutine_block_pthread_count_limit<1) || ((cobs = zcc::coroutine_base_get_current())==0))

#define coroutine_hook_fileio_run_part2(func)  \
    zcc::coroutine *current_coroutine = cobs->current_coroutine; \
    zcc::coroutine_hook_fileio_t fileio;\
    fileio.current_coroutine = cobs->current_coroutine; \
    fileio.co_errno = 0; \
    fileio.cmdcode = zcc::coroutine_hook_fileio_ ## func;

#define coroutine_hook_fileio_run_part3()  \
    zcc_pthread_lock(&zcc::var_coroutine_hook_fileio_lock); \
    zcc_mlink_append(zcc::var_coroutine_hook_fileio_head, zcc::var_coroutine_hook_fileio_tail, &fileio, prev, next); \
    zcc::var_coroutine_hook_fileio_count++; \
    if (zcc::var_coroutine_block_pthread_count_current < zcc::var_coroutine_block_pthread_count_limit) { \
        if ((zcc::var_coroutine_block_pthread_count_current == 0)||(zcc::var_coroutine_hook_fileio_count > (zcc::var_coroutine_block_pthread_count_current))) { \
            pthread_t pth; \
            if (pthread_create(&pth, 0, zcc::coroutine_hook_fileio_worker, 0)) { \
                zcc_fatal("pthread_create error(%m)"); \
            } \
            zcc::var_coroutine_block_pthread_count_current++; \
        } \
    } \
    pthread_cond_signal(&zcc::var_coroutine_hook_fileio_cond); \
    zcc_pthread_unlock(&zcc::var_coroutine_hook_fileio_lock); \
    current_coroutine->___inner_yield = 1; \
    coroutine_yield(current_coroutine); \
    errno = fileio.co_errno; \
    zcc::coroutine_hook_arg_t retval; \
    retval.long_t = fileio.retval.long_t; \

/* ######################################## */
static bool var_coroutine_mode_flag = false;

struct coroutine_sys_context {
	void *regs[ 14 ];
	size_t ss_size;
	char *ss_sp;
};
class coroutine
{
public:
    coroutine(coroutine_base *base, size_t stack_size = 128 * 1024);
    ~coroutine();
    void *(*___start_job)(void *ctx);
    void *___context;
    long sleep_timeout;
    rbtree_node_t sleep_rbnode;
    /* system */
    coroutine_sys_context ___sys_context;
    coroutine *___prev;
    coroutine *___next;
    coroutine_base * ___base; /* FIXME, should be removed */
    std::list<coroutine_mutex_t *> *___mutex_list;
    struct __res_state *___res_state;
    gethostbyname_buf_t *___gethostbyname;
    /* flags */
    unsigned char ___ended:1;
    unsigned char ___ep_loop:1;
    unsigned char ___active_list:1;
    unsigned char ___inner_yield:1;
};

static const int var_epoll_event_size = 4096;
struct coroutine_base {
    int epoll_fd;
    int event_fd;
    struct epoll_event epoll_event_vec[var_epoll_event_size];
    rbtree_t sleep_rbtree;
    rbtree_t fd_timeout_rbtree;
    coroutine *self_coroutine;
    coroutine *current_coroutine;
    coroutine *fileio_coroutines_head;
    coroutine *fileio_coroutines_tail;
    coroutine *active_coroutines_head;
    coroutine *active_coroutines_tail;
    coroutine *prepare_coroutines_head;
    coroutine *prepare_coroutines_tail;
    coroutine *deleted_coroutine_head;
    coroutine *deleted_coroutine_tail;
    unsigned char ___break:1;
};

typedef struct coroutine_fd_attribute coroutine_fd_attribute;
struct coroutine_fd_attribute {
    unsigned short int nonblock:1;
    unsigned short int pseudo_mode:1;
    unsigned short int in_epoll:1;
    unsigned short int by_epoll:1;
    unsigned short int is_regular_file:1;
    unsigned short int read_timeout;
    unsigned short int write_timeout;
    unsigned int revents;
    long timeout;
    rbtree_node_t rbnode;
    coroutine *co;
};

struct  coroutine_mutex_t {
    std::list<coroutine *> colist;
};

struct coroutine_cond_t {
    std::list<coroutine *> colist;
};

static void coroutine_base_remove_coroutine(coroutine_base *cobs);
static coroutine_base *coroutine_base_create();

/* ################################################################################# */

static __thread coroutine_base *var_coroutine_base_per_pthread = 0;
static coroutine_fd_attribute **coroutine_fd_attribute_vec = 0;

static inline coroutine_base * coroutine_base_get_current()
{
    return (var_coroutine_mode_flag?var_coroutine_base_per_pthread:0);
}

/* {{{ coroutine_sys_context */
static int coroutine_start_wrap(coroutine *co, void *unused);
static void coroutine_sys_context_init(coroutine_sys_context *ctx, const void *s)
{
	char *sp = ctx->ss_sp + ctx->ss_size;
	sp = (char*) ((unsigned long)sp & -16LL  );
	memset(ctx->regs, 0, sizeof(ctx->regs));
	ctx->regs[13] = sp - 8;
	ctx->regs[9] = (char*)coroutine_start_wrap;
	ctx->regs[7] = (char *)s;
	ctx->regs[8] = 0;
}

static void coroutine_sys_context_fini(coroutine_sys_context *ctx)
{
}

asm("\n"
".type ___coroutine_context_swap, @function\n"
"___coroutine_context_swap:\n"
"\tleaq 8(%rsp),%rax\n"
"\tleaq 112(%rdi),%rsp\n"
"\tpushq %rax\n"
"\tpushq %rbx\n"
"\tpushq %rcx\n"
"\tpushq %rdx\n"
"\tpushq -8(%rax)\n"
"\tpushq %rsi\n"
"\tpushq %rdi\n"
"\tpushq %rbp\n"
"\tpushq %r8\n"
"\tpushq %r9\n"
"\tpushq %r12\n"
"\tpushq %r13\n"
"\tpushq %r14\n"
"\tpushq %r15\n"
"\tmovq %rsi, %rsp\n"
"\tpopq %r15\n"
"\tpopq %r14\n"
"\tpopq %r13\n"
"\tpopq %r12\n"
"\tpopq %r9\n"
"\tpopq %r8\n"
"\tpopq %rbp\n"
"\tpopq %rdi\n"
"\tpopq %rsi\n"
"\tpopq %rax\n"
"\tpopq %rdx\n"
"\tpopq %rcx\n"
"\tpopq %rbx\n"
"\tpopq %rsp\n"
"\tpushq %rax\n"
"\txorl %eax, %eax\n"
"\tret\n");
void coroutine_context_swap(coroutine_sys_context *, coroutine_sys_context *) asm("___coroutine_context_swap");

/* }}} */

/* {{{ coroutine_fd_attribute */
static inline coroutine_fd_attribute * coroutine_fd_attribute_get(int fd)
{
    if ((fd > -1) &&  (fd <= zcc::var_fd_max) && (var_coroutine_mode_flag)) {
        return coroutine_fd_attribute_vec[fd];
    }
    return 0;
}

static inline coroutine_fd_attribute *coroutine_fd_attribute_create(int fd)
{
    if ((fd > -1) &&  (fd <= zcc::var_fd_max) && (var_coroutine_mode_flag)) {
        free(coroutine_fd_attribute_vec[fd]);
        coroutine_fd_attribute *cfa = (coroutine_fd_attribute *)calloc(1, sizeof(coroutine_fd_attribute));
        coroutine_fd_attribute_vec[fd] = cfa;
        cfa->read_timeout = 10 * 1000  + (fd%1000);
        cfa->write_timeout = 10 * 1000 + (fd%1000);
        return cfa;
    }
    return 0;
}

static inline void coroutine_fd_attribute_free(int fd)
{
    if ((fd > -1) &&  (fd <= zcc::var_fd_max) && (var_coroutine_mode_flag)) {
        free(coroutine_fd_attribute_vec[fd]);
        coroutine_fd_attribute_vec[fd] = 0;
    }
}
/* }}} */

/* {{{ coroutine */
coroutine::coroutine(coroutine_base *base, size_t stack_size)
{
    ___start_job = 0;
    ___context = 0;
    ___prev = ___next = 0;
    ___base = base;
    ___ended = 0;
    ___mutex_list = 0;
    ___res_state = 0;
    ___gethostbyname = 0;
    memset(&___sys_context, 0, sizeof(___sys_context));
    ___sys_context.ss_sp = (char *)malloc(stack_size + 16 + 10);
    ___sys_context.ss_size = stack_size;
    coroutine_sys_context_init(&___sys_context, (void *)this);
}

static void coroutine_append_mutex(coroutine *co, coroutine_mutex_t *mutex)
{
    if (co->___mutex_list == 0) {
        co->___mutex_list = new std::list<coroutine_mutex_t *>();
        co->___mutex_list->push_back(mutex);
        return;
    }

    std_list_walk_begin(*(co->___mutex_list), m) {
        if (m == mutex) {
            return;
        }
    } std_list_walk_end;
    co->___mutex_list->push_back(mutex);
}

static void coroutine_remove_mutex(coroutine *co, coroutine_mutex_t *mutex)
{
    std::list<coroutine_mutex_t *>::iterator del = co->___mutex_list->end();
    std_list_walk_begin(*(co->___mutex_list), m) {
        if (m == mutex) {
            del = var_std_list_iterator;
        }
    } std_list_walk_end;
    if (del != co->___mutex_list->end()) {
        co->___mutex_list->erase(del);
    }
    if (co->___mutex_list->empty()) {
        delete co->___mutex_list;
        co->___mutex_list = 0;
    }
}

static void coroutine_release_all_mutex(coroutine *co)
{
    if (!co->___mutex_list) {
        return;
    }
    while((co->___mutex_list) && (!co->___mutex_list->empty())) {
        coroutine_mutex_t *mutex = co->___mutex_list->back();
        co->___mutex_list->pop_back();
        coroutine_mutex_unlock(mutex);
    }
    if (co->___mutex_list) {
        delete co->___mutex_list;
    }
    co->___mutex_list = 0;
}

coroutine::~coroutine()
{
    void *ptr = ___sys_context.ss_sp;
    free(___res_state);
    free(___gethostbyname);
    coroutine_sys_context_fini(&___sys_context);
    free(ptr);
}

static int coroutine_start_wrap(coroutine *co, void *unused)
{
    if (co->___start_job) {
        co->___start_job(co->___context);
    }
    coroutine_release_all_mutex(co);
    coroutine_base *cobs = co->___base;
    if (cobs->deleted_coroutine_head) {
        coroutine_base_remove_coroutine(cobs);
    }
    co->___ended = 1;
    co->___inner_yield = 1;
    coroutine_yield(co);
    return 0;
}

static int ___base_count = 0;
void coroutine_base_init()
{
    if (coroutine_fd_attribute_vec == 0) {
        pthread_mutex_lock(var_general_pthread_mutex);
        if (coroutine_fd_attribute_vec == 0) {
            coroutine_fd_attribute_vec = (coroutine_fd_attribute **)calloc(var_fd_max + 1, sizeof(void *));
            var_coroutine_mode_flag = true;
        }
        pthread_mutex_unlock(var_general_pthread_mutex);
    }
    if (var_coroutine_base_per_pthread == 0) {
        coroutine_base_create();
        pthread_mutex_lock(var_general_pthread_mutex);
        ___base_count++;
        pthread_mutex_unlock(var_general_pthread_mutex);
    }
}

void coroutine_base_fini()
{
    if (coroutine_fd_attribute_vec == 0) {
        return;
    }
    zcc::coroutine_base *cobs = zcc::var_coroutine_base_per_pthread;
    if (cobs) {
        if (cobs->deleted_coroutine_head) {
            coroutine_base_remove_coroutine(cobs);
        }
        delete cobs->self_coroutine;
        zcc::syscall_close(cobs->epoll_fd);
        zcc::syscall_close(cobs->event_fd);
        free(cobs);
        var_coroutine_base_per_pthread = 0;
        pthread_mutex_lock(var_general_pthread_mutex);
        ___base_count--;
        if (___base_count == 0) {
            free(coroutine_fd_attribute_vec);
            coroutine_fd_attribute_vec = 0;
        }
        pthread_mutex_unlock(var_general_pthread_mutex);
    }
}

void coroutine_go(void *(*start_job)(void *ctx), void *ctx, size_t stack_size)
{
    if (start_job == 0) {
        return;
    }
    coroutine_base *cobs = coroutine_base_get_current();
    if (!cobs) {
        zcc_fatal("excute coroutine_enable() when the pthread begin");
    }
    if (cobs->deleted_coroutine_head) {
        coroutine_base_remove_coroutine(cobs);
    }
    coroutine *co = new coroutine(cobs, stack_size);
    zcc_mlink_append(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, ___prev, ___next);
    co->___start_job = start_job;
    co->___context = ctx;
}

coroutine * coroutine_self()
{
    coroutine_base *cobs =  coroutine_base_get_current();
    if (cobs == 0) {
        return 0;
    }
    return cobs->current_coroutine;
}

void coroutine_yield(coroutine *co)
{
    if (!co) {
        co = coroutine_self();
    }
    if (!co) {
        return;
    }
    coroutine_base *cobs = co->___base;
    cobs->current_coroutine = 0;
    if (co->___ended) {
        zcc_mlink_append(cobs->deleted_coroutine_head, cobs->deleted_coroutine_tail, co, ___prev, ___next);
    } else if ((co->___inner_yield == 0) && (co != cobs->self_coroutine)) {
        zcc_mlink_append(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, ___prev, ___next);
    }
    coroutine *next_co = cobs->active_coroutines_head;
    if (next_co == 0) {
        if (co == cobs->self_coroutine) {
            return;
        }
        next_co = cobs->self_coroutine;
    }
    if (next_co != cobs->self_coroutine) {
        zcc_mlink_detach(cobs->active_coroutines_head, cobs->active_coroutines_tail, next_co, ___prev, ___next);
    }
    cobs->current_coroutine = next_co;
    co->___inner_yield = 0;
    coroutine_context_swap(&(co->___sys_context), &(next_co->___sys_context));
}

void coroutine_sleep(long s) {
    if (s > 0) {
        long timeout = timeout_set(s * 1000);
        int left = (int)(s * 1000);
        while (left > 0) {
            poll(0, 0, left);
            left = (int)timeout_left(timeout);
        }
    }
}
void coroutine_msleep(long ms) {
    if (ms > 0) {
        long timeout = timeout_set(ms);
        int left = (int)ms;
        while (left > 0) {
            poll(0, 0, left);
            left = (int)timeout_left(timeout);
        }
    }
}

void coroutine_exit(coroutine *co)
{
    if (!co) {
        co = coroutine_self();
    }
    if (!co) {
        return;
    }
    coroutine_release_all_mutex(co);
    co->___ended = 1;
    co->___inner_yield = 1;
    coroutine_yield(co);
}

void *coroutine_get_context(coroutine *co)
{
    if (!co) {
        co = coroutine_self();
    }
    if (!co) {
        return 0;
    }
    return const_cast<void *>(co->___context);
}

void coroutine_set_context(coroutine *co, const void *ctx)
{
    co->___context = const_cast<void *>(ctx);
}

void coroutine_set_context(const void *ctx)
{
    coroutine *co = coroutine_self();
    if (!co) {
        return;
    }
    co->___context = const_cast<void *>(ctx);
}

void coroutine_enable_fd(int fd)
{
    coroutine_fd_attribute_create(fd);
}

void coroutine_disable_fd(int fd)
{
    coroutine_fd_attribute  *cfa = coroutine_fd_attribute_get(fd);
    if (cfa) {
        int flags;
        if ((flags = zcc::syscall_fcntl(fd, F_GETFL, 0)) < 0) {
            zcc_fatal("fcntl _co(%m)");
        }
        if (zcc::syscall_fcntl(fd, F_SETFL, (cfa->nonblock?flags | O_NONBLOCK : flags & ~O_NONBLOCK)) < 0) {
            zcc_fatal("fcntl _co(%m)");
        }
        coroutine_fd_attribute_free(fd);
    }
}
/* }}} */

/* {{{ mutex cond */
coroutine_mutex_t * coroutine_mutex_create()
{
    coroutine_mutex_t *m = new coroutine_mutex_t();
    return m;
}

void coroutine_mutex_free(coroutine_mutex_t *m)
{
    if (m) {
        delete m;
    }
}

void coroutine_mutex_lock(coroutine_mutex_t *m)
{
    if (m == 0) {
        zcc_fatal("not in coroutine");
    }
    coroutine * co = coroutine_self();
    if (co == 0) {
        zcc_fatal("not in coroutine");
    }
    std::list<coroutine *> &colist = m->colist;
    if (colist.empty()) {
        colist.push_back(co);
        coroutine_append_mutex(co, m);
        return;
    }
    if (colist.front() == co) {
        coroutine_append_mutex(co, m);
        return;
    }
    colist.push_back(co);
    co->___inner_yield = 1;
    coroutine_yield(co);
}

void coroutine_mutex_unlock(coroutine_mutex_t *m)
{
    /* FIXME */
    if (m == 0) {
        zcc_fatal("mutex is null");
        return;
    }
    coroutine * co = coroutine_self();
    if (co == 0) {
        zcc_fatal("not in coroutine");
        return;
    }
    std::list<coroutine *> &colist = m->colist;
    if (colist.empty()) {
        return;
    }
    if (colist.front() != co) {
        return;
    }
    coroutine_remove_mutex(co, m);
    colist.pop_front();
    if (colist.empty()) {
        return;
    }
    co = colist.front();
    zcc_mlink_append(co->___base->prepare_coroutines_head, co->___base->prepare_coroutines_tail, co, ___prev, ___next);
    return;
}

coroutine_cond_t *coroutine_cond_create()
{
    coroutine_cond_t *c = new coroutine_cond_t();
    return c;
}

void coroutine_cond_free(coroutine_cond_t * c)
{
    if (c) {
        delete c;
    }
}

void coroutine_cond_wait(coroutine_cond_t *cond, coroutine_mutex_t * mutex)
{
    coroutine * co = coroutine_self();
    if (co == 0) {
        zcc_fatal("not in coroutine");
    }
    if (!cond) {
        zcc_fatal("cond is null");
    }
    if (!cond) {
        zcc_fatal("mutex is null");
    }

    coroutine_mutex_unlock(mutex);

    cond->colist.push_back(co);
    co->___inner_yield = 1;
    coroutine_yield(co);

    coroutine_mutex_lock(mutex);
}

void coroutine_cond_signal(coroutine_cond_t * cond)
{
    if (!cond) {
        zcc_fatal("cond is null");
    }
    coroutine_base *cobs;
    if ((cobs = coroutine_base_get_current()) == 0) {
        zcc_fatal("not in coroutine");
    }
    if (cond->colist.empty()) {
        return;
    }
    coroutine *co = cond->colist.front();
    cond->colist.pop_front();
    zcc_mlink_append(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, ___prev, ___next);
    coroutine_yield(cobs->current_coroutine);
}

void coroutine_cond_broadcast(coroutine_cond_t *cond)
{
    if (!cond) {
        zcc_fatal("cond is null");
    }
    coroutine_base *cobs;
    if ((cobs = coroutine_base_get_current()) == 0) {
        zcc_fatal("not in coroutine");
    }
    while(1) {
        if (cond->colist.empty()) {
            break;
        }
        coroutine *co = cond->colist.front();
        cond->colist.pop_front();
        zcc_mlink_append(cobs->prepare_coroutines_head, cobs->prepare_coroutines_tail, co, ___prev, ___next);
    }
    coroutine_yield(cobs->current_coroutine);
}

/* }}} */

/* {{{ sleep_rbtree */
static int coroutine_base_sleep_tree_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    coroutine *c1, *c2;
    long r;
    c1 = zcc_container_of(n1, coroutine, sleep_rbnode);
    c2 = zcc_container_of(n2, coroutine, sleep_rbnode);
    r = c1->sleep_timeout - c2->sleep_timeout;
    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}
/* }}} */

/* {{{ fd_timeout_rbtree */
static int coroutine_base_fd_timeout_tree_cmp(rbtree_node_t * n1, rbtree_node_t * n2)
{
    coroutine_fd_attribute *c1, *c2;
    long r;
    c1 = zcc_container_of(n1, coroutine_fd_attribute, rbnode);
    c2 = zcc_container_of(n2, coroutine_fd_attribute, rbnode);
    r = c1->timeout - c2->timeout;
    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}
/* }}} */

/* {{{ coroutine_base */
static coroutine_base *coroutine_base_create()
{
    coroutine_base *cobs;
    cobs = (coroutine_base *)calloc(1, sizeof(coroutine_base));
    var_coroutine_base_per_pthread = cobs;
    cobs->self_coroutine = new coroutine(cobs);
    cobs->epoll_fd = epoll_create(1024);
    close_on_exec(cobs->epoll_fd, true);

    cobs->event_fd = eventfd(0, 0);
    close_on_exec(cobs->event_fd, true);
    nonblocking(cobs->event_fd, 1);
    struct epoll_event epev;
    epev.events = EPOLLIN;
    epev.data.fd = cobs->event_fd;
    int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_ADD, cobs->event_fd, &epev);
    if (eret < 0) {
        zcc_fatal("epoll_ctl ADD event_fd:%d (%m)", cobs->event_fd);
    }

    rbtree_init(&(cobs->sleep_rbtree), coroutine_base_sleep_tree_cmp);
    rbtree_init(&(cobs->fd_timeout_rbtree), coroutine_base_fd_timeout_tree_cmp);
    return cobs;
}

static void coroutine_base_remove_coroutine(coroutine_base *cobs)
{
    coroutine *co, *next_co;
    for (co = cobs->deleted_coroutine_head; co; co = next_co) {
        next_co = co->___next;
        delete co;
    }
    cobs->deleted_coroutine_head = cobs->deleted_coroutine_tail = 0;
}

void coroutine_base_stop_notify()
{
    coroutine_base *cobs = coroutine_base_get_current();
    if (cobs) {
        cobs->___break = 1;
    }
}

/* }}} */

/* {{{ coroutine_base_loop */
void coroutine_base_loop()
{
    coroutine_base *cobs = coroutine_base_get_current();
    if (!cobs) {
        zcc_fatal("excute coroutine_enable() when the pthread begin");
    }
    coroutine *co;
    rbtree_node_t *rn;
    coroutine_fd_attribute *cfa;
    long delay, tmp_delay, tmp_ms;

    while(1) {
        if(var_proc_stop) {
            return;
        }
        if (cobs->deleted_coroutine_head) {
            coroutine_base_remove_coroutine(cobs);
        }

        delay = 1000;
        tmp_ms = timeout_set(0) - 1;

        /* fileio list */
        if (cobs->fileio_coroutines_head) {
            zcc_pthread_lock(&zcc::var_coroutine_hook_fileio_lock);
            if (cobs->fileio_coroutines_head) {
                if (cobs->active_coroutines_head == 0) {
                    cobs->active_coroutines_head = cobs->fileio_coroutines_head;
                    cobs->active_coroutines_tail = cobs->fileio_coroutines_tail;
                } else if (cobs->fileio_coroutines_head) {
                    cobs->active_coroutines_tail->___next = cobs->fileio_coroutines_head;
                    cobs->fileio_coroutines_head->___prev = cobs->active_coroutines_tail;
                    cobs->active_coroutines_tail = cobs->fileio_coroutines_tail;
                }
                cobs->fileio_coroutines_head = cobs->fileio_coroutines_tail = 0;
            }
            zcc_pthread_unlock(&zcc::var_coroutine_hook_fileio_lock);
        }

        /* prepared list */
        if (cobs->active_coroutines_head == 0) {
            cobs->active_coroutines_head = cobs->prepare_coroutines_head;
            cobs->active_coroutines_tail = cobs->prepare_coroutines_tail;
        } else if (cobs->prepare_coroutines_head) {
            cobs->active_coroutines_tail->___next = cobs->prepare_coroutines_head;
            cobs->prepare_coroutines_head->___prev = cobs->active_coroutines_tail;
            cobs->active_coroutines_tail = cobs->prepare_coroutines_tail;
        }
        cobs->prepare_coroutines_head = cobs->prepare_coroutines_tail = 0;

        /* sleep timeout */
        if (rbtree_have_data(&(cobs->sleep_rbtree))) {
            if(var_proc_stop) {
                return;
            }
            rn = rbtree_first(&(cobs->sleep_rbtree));
            co = zcc_container_of(rn, coroutine, sleep_rbnode);
            tmp_delay = co->sleep_timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = rbtree_next(rn)) {
                co = zcc_container_of(rn, coroutine, sleep_rbnode);
                if (tmp_ms < co->sleep_timeout) {
                    break;
                }
                zcc_mlink_append(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, ___prev, ___next);
            }
        }

        /* fd timeout */
        if (rbtree_have_data(&(cobs->fd_timeout_rbtree))) {
            rn = rbtree_first(&(cobs->fd_timeout_rbtree));
            cfa = zcc_container_of(rn, coroutine_fd_attribute, rbnode);
            tmp_delay = cfa->timeout - tmp_ms;
            if (tmp_delay < delay) {
                delay = tmp_delay;
            }
            for(; rn; rn = rbtree_next(rn)) {
                if(var_proc_stop) {
                    return;
                }
                cfa = zcc_container_of(rn, coroutine_fd_attribute, rbnode);
                if (tmp_ms < cfa->timeout) {
                    break;
                }
                co = cfa->co;
                if ((co->___ep_loop == 1) && (co->___active_list == 0)) {
                    zcc_mlink_append(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, ___prev, ___next);
                    co->___active_list = 1;
                }
                co->___ep_loop = 1;
            }
        }

        /* epoll_wait */
        if ((delay < 0) || cobs->active_coroutines_head) {
            delay = 0;
        }
        int nfds = epoll_wait(cobs->epoll_fd, cobs->epoll_event_vec, var_epoll_event_size, (int)delay);
        if ((nfds == -1) && (errno != EINTR)) {
            zcc_fatal("epoll_wait: %m");
        }
        for (int i = 0; i < nfds; i++) {
            if(var_proc_stop) {
                return;
            }
            struct epoll_event *epev = cobs->epoll_event_vec + i;
            int fd = epev->data.fd;
            if (fd == cobs->event_fd) {
                uint64_t u;
                zcc::syscall_read(fd, &u, sizeof(uint64_t));
                continue;
            }
            cfa = coroutine_fd_attribute_get(fd);
            if (!cfa) {
                zcc_fatal("fd:%d be closed unexpectedly", fd);
                continue;
            }
            cfa->by_epoll = 1;
            cfa->revents = epev->events;
            co = cfa->co;
            if (co->___active_list == 1) {
                zcc_mlink_detach(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, ___prev, ___next);
                co->___active_list = 0;
            }
            if (co->___active_list == 0) {
                zcc_mlink_append(cobs->active_coroutines_head, cobs->active_coroutines_tail, co, ___prev, ___next);
                co->___active_list = 1;
            }
        }

        /* */
        if (cobs->active_coroutines_head == 0) {
            continue;
        }
        coroutine_yield(cobs->self_coroutine);
        if (cobs->___break) {
            break;
        }
    }
    if (cobs->deleted_coroutine_head) {
        coroutine_base_remove_coroutine(cobs);
    }
}

/* }}} */

/* {{{ coroutine_poll */
static int coroutine_poll(coroutine *co, struct pollfd fds[], nfds_t nfds, int timeout)
{
    co->___active_list = 0;
    co->___ep_loop = 0;

    coroutine_base *cobs = co->___base;
    long now_ms = timeout_set(0);
    bool is_epoll_ctl = false;

    do {
        int fdcount = 0;
        int last_fd = -1;
        for (nfds_t i = 0; i < nfds; i++) {
            fds[i].revents = 0;
            int fd  = fds[i].fd;
            if (fd < 0) {
                continue;
            }
            fdcount++;
            last_fd = fd;
            if (fdcount > 1) {
                break;
            }
        }
        if (fdcount != 1) {
            break;
        }

        coroutine_fd_attribute *cfa = coroutine_fd_attribute_get(last_fd);
        if (cfa == 0) {
            return zcc::syscall_poll(fds, nfds, timeout);
        }

    } while(0);

    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        int fd  = fds[i].fd;
        if (fd < 0) {
            continue;
        }
        coroutine_fd_attribute *cfa = coroutine_fd_attribute_get(fd);
        if (cfa == 0) {
            cfa = coroutine_fd_attribute_create(fd);
            cfa->pseudo_mode = 1;
        }
        if (cfa->in_epoll) {
            zcc_fatal("mutli monitor fd:%d", fd);
        }
        short int events = fds[i].events;
        unsigned int ep_events = 0;	
        if(events & POLLIN) 	ep_events |= EPOLLIN;
        if(events & POLLOUT)    ep_events |= EPOLLOUT;
        if(events & POLLHUP) 	ep_events |= EPOLLHUP;
        if(events & POLLERR)	ep_events |= EPOLLERR;
        if(events & POLLRDNORM) ep_events |= EPOLLRDNORM;
        if(events & POLLWRNORM) ep_events |= EPOLLWRNORM;
        struct epoll_event epev;
        epev.events = ep_events;
        epev.data.fd = fd;
        int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_ADD, fd, &epev);
        if ((eret<0) && (errno==EPERM) && (nfds==1)) {
            return zcc::syscall_poll(fds, nfds, timeout);
        }
        is_epoll_ctl = true;
        cfa->by_epoll = 0;
        cfa->co = co;
        cfa->timeout = now_ms + timeout;
        rbtree_attach(&(cobs->fd_timeout_rbtree), &(cfa->rbnode));
    }

    if (is_epoll_ctl == false) {
        if (timeout < 1) {
            return 0;
        }
        co->sleep_timeout = now_ms + timeout;
        rbtree_attach(&(cobs->sleep_rbtree), &(co->sleep_rbnode));
        co->___inner_yield = 1;
        coroutine_yield(co);
        rbtree_detach(&(cobs->sleep_rbtree), &(co->sleep_rbnode));
        return 0;
    }

    co->___inner_yield = 1;
    coroutine_yield(co);

    int raise_count = 0;
    for (nfds_t i = 0; i < nfds; i++) {
        int fd  = fds[i].fd;
        if (fd < 0) {
            continue;
        }
        coroutine_fd_attribute *cfa = coroutine_fd_attribute_get(fd);
        if (!cfa) {
            /* the fd be closed */
        } else {
            cfa->in_epoll = 0;
            unsigned int revents = cfa->revents;
            fds[i].revents = 0;
            if (cfa->by_epoll && revents) {
                raise_count++;
                short int p_events = 0;	
                if(revents & EPOLLIN)     p_events |= POLLIN;
                if(revents & EPOLLOUT)    p_events |= POLLOUT;
                if(revents & EPOLLHUP)    p_events |= POLLHUP;
                if(revents & EPOLLERR)    p_events |= POLLERR;
                if(revents & EPOLLRDNORM) p_events |= POLLRDNORM;
                if(revents & EPOLLWRNORM) p_events |= POLLWRNORM;
                fds[i].revents = p_events;
            }
            int eret = epoll_ctl(cobs->epoll_fd, EPOLL_CTL_DEL, fd, 0);
            if (eret < 0) {
                zcc_fatal("epoll_ctl del fd:%d (%m)", fd);
            }
        }
        rbtree_detach(&(cobs->fd_timeout_rbtree), &(cfa->rbnode));
    }
    return raise_count;
}
/* }}} */

/* {{{ file io worker pthread */
static void coroutine_hook_fileio_worker_do(zcc::coroutine_hook_fileio_t &cio)
{
    auto &args = cio.args;
    auto &retval = cio.retval;
    int errno_bak = 0;
    if (cio.is_block_func) {
        retval.void_ptr_t = args[0].block_func(args[1].void_ptr_t);
    }
    switch(cio.cmdcode) {
    case zcc::coroutine_hook_fileio_open:
        retval.int_t = zcc::syscall_open(args[0].char_ptr_t, args[1].int_t, args[2].int_t);
        if (retval.int_t > -1) {
            struct stat st;
            if (zcc::syscall_fstat(retval.int_t, &st) == -1) {
                errno_bak = errno;
                zcc::syscall_close(retval.int_t);
                retval.int_t = -1;
            } else {
                if (S_ISFIFO(st.st_mode)) {
                    cio.is_regular_file = 0;
                } else {
                    cio.is_regular_file = 1;
                }
            }
        }
        break;
    case zcc::coroutine_hook_fileio_openat:
        retval.int_t = zcc::syscall_openat(args[0].int_t, args[1].char_ptr_t, args[2].int_t, args[3].int_t);
        if (retval.int_t > -1) {
            struct stat st;
            if (zcc::syscall_fstat(retval.int_t, &st) == -1) {
                errno_bak = errno;
                zcc::syscall_close(retval.int_t);
                retval.int_t = -1;
            } else {
                if (S_ISFIFO(st.st_mode)) {
                    cio.is_regular_file = 0;
                } else {
                    cio.is_regular_file = 1;
                }
            }
        }
        break;
    case zcc::coroutine_hook_fileio_read:
        retval.ssize_ssize_t = zcc::syscall_read(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
        break;
    case zcc::coroutine_hook_fileio_readv:
        retval.ssize_ssize_t = zcc::syscall_readv(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
        break;
    case zcc::coroutine_hook_fileio_write:
        retval.ssize_ssize_t = zcc::syscall_write(args[0].int_t, args[1].void_ptr_t, args[2].size_size_t);
        break;
    case zcc::coroutine_hook_fileio_writev:
        retval.ssize_ssize_t = zcc::syscall_writev(args[0].int_t, args[1].const_iovec_t, args[2].int_t);
        break;
    case zcc::coroutine_hook_fileio_lseek:
        retval.off_off_t = zcc::syscall_lseek(args[0].int_t, args[1].off_off_t, args[2].int_t);
        break;
    case zcc::coroutine_hook_fileio_fdatasync:
        retval.int_t = zcc::syscall_fdatasync(args[0].int_t);
        break;
    case coroutine_hook_fileio_fsync:
        retval.int_t = zcc::syscall_fsync(args[0].int_t);
        break;
    case coroutine_hook_fileio_rename:
        retval.int_t = zcc::syscall_rename(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case coroutine_hook_fileio_truncate:
        retval.int_t = zcc::syscall_truncate(args[0].const_char_ptr_t, args[1].off_off_t);
        break;
    case coroutine_hook_fileio_ftruncate:
        retval.int_t = zcc::syscall_ftruncate(args[0].int_t, args[1].off_off_t);
        break;
    case coroutine_hook_fileio_rmdir:
        retval.int_t = zcc::syscall_rmdir(args[0].const_char_ptr_t);
        break;
    case coroutine_hook_fileio_mkdir:
        retval.int_t = zcc::syscall_mkdir(args[0].const_char_ptr_t, args[1].mode_mode_t);
        break;
    case coroutine_hook_fileio_getdents:
        retval.int_t = zcc::syscall_getdents(args[0].uint_t, args[1].void_ptr_t, args[1].uint_t);
        break;
    case coroutine_hook_fileio_stat:
        retval.int_t = zcc::syscall_stat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
        break;
    case coroutine_hook_fileio_fstat:
        retval.int_t = zcc::syscall_fstat(args[0].int_t, (struct stat *)args[0].char_ptr_t);
        break;
    case coroutine_hook_fileio_lstat:
        retval.int_t = zcc::syscall_lstat(args[0].const_char_ptr_t, (struct stat *)args[0].char_ptr_t);
        break;
    case coroutine_hook_fileio_link:
        retval.int_t = zcc::syscall_link(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case coroutine_hook_fileio_symlink:
        retval.int_t = zcc::syscall_symlink(args[0].const_char_ptr_t, args[1].const_char_ptr_t);
        break;
    case coroutine_hook_fileio_readlink:
        retval.int_t = zcc::syscall_readlink(args[0].const_char_ptr_t, args[1].char_ptr_t, args[2].size_size_t);
        break;
    case coroutine_hook_fileio_unlink:
        retval.int_t = zcc::syscall_unlink(args[0].const_char_ptr_t);
        break;
    case coroutine_hook_fileio_chmod:
        retval.int_t = zcc::syscall_chmod(args[0].const_char_ptr_t, args[1].mode_mode_t);
        break;
    case coroutine_hook_fileio_fchmod:
        retval.int_t = zcc::syscall_fchmod(args[0].int_t, args[1].mode_mode_t);
        break;
    case coroutine_hook_fileio_chown:
        retval.int_t = zcc::syscall_chown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case coroutine_hook_fileio_fchown:
        retval.int_t = syscall_fchown(args[0].int_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case coroutine_hook_fileio_lchown:
        retval.int_t = zcc::syscall_lchown(args[0].const_char_ptr_t, args[1].uid_uid_t, args[2].gid_gid_t);
        break;
    case coroutine_hook_fileio_utime:
        retval.int_t = zcc::syscall_utime(args[0].const_char_ptr_t, (const struct utimbuf *)args[1].char_ptr_t);
        break;
    case coroutine_hook_fileio_utimes:
        retval.int_t = zcc::syscall_utimes(args[0].const_char_ptr_t, (struct timeval *)args[1].char_ptr_t);
        break;
    }
    if (errno_bak) {
        cio.co_errno = errno_bak;
    } else {
        cio.co_errno = errno;
    }
}

static pthread_key_t ___gethostbyname_pthread_key;
void ___gethostbyname_pthread_key_destroy(void *buf)
{
    char **g = (char **)buf;
    if (g) {
        if (*g) {
            free(*g);
        }
        free(g);
    }
    pthread_setspecific(___gethostbyname_pthread_key, 0);
}

static void coroutine_hook_fileio_worker_init()
{
    pthread_detach(pthread_self());
    pthread_key_create(&___gethostbyname_pthread_key, ___gethostbyname_pthread_key_destroy);
    char **g = (char **)malloc(sizeof(char **));
    *g = 0;
    pthread_setspecific(___gethostbyname_pthread_key, g);
}

static void *coroutine_hook_fileio_worker(void *arg)
{
    coroutine_hook_fileio_worker_init();
    while (1) {
        zcc_pthread_lock(&var_coroutine_hook_fileio_lock);
        while(!var_coroutine_hook_fileio_head) {
            if (var_proc_stop) {
                zcc_pthread_unlock(&var_coroutine_hook_fileio_lock);
                return arg;
            }
            struct timespec ts;
            ts.tv_sec = time(0) + 1;
            ts.tv_nsec = 0;
            pthread_cond_timedwait(&var_coroutine_hook_fileio_cond, &var_coroutine_hook_fileio_lock, &ts);
        }
        coroutine_hook_fileio_t *cio = var_coroutine_hook_fileio_head;
        zcc_mlink_detach(var_coroutine_hook_fileio_head, var_coroutine_hook_fileio_tail, cio, prev, next);
        var_coroutine_hook_fileio_count--;
        zcc_pthread_unlock(&var_coroutine_hook_fileio_lock);
        if (var_proc_stop) {
            break;
        }
        coroutine_hook_fileio_worker_do(*cio);
        if (var_proc_stop) {
            break;
        }

        zcc_pthread_lock(&var_coroutine_hook_fileio_lock);
        coroutine  *co = cio->current_coroutine;
        coroutine_base *cobs = co->___base;
        zcc_mlink_append(cobs->fileio_coroutines_head, cobs->fileio_coroutines_tail, co, ___prev, ___next);
        uint64_t u = 1;
        zcc::syscall_write(cobs->event_fd, &u, sizeof(uint64_t));
        zcc_pthread_unlock(&var_coroutine_hook_fileio_lock);
    }
    return arg;
}
/* }}} */

/* {{{ block*/
void *coroutine_block_do(void *(*block_func)(void *ctx), void *ctx)
{
    coroutine_hook_fileio_run_part1() {
        return block_func(ctx);
    }
    coroutine_hook_fileio_run_part2(unknown);
    fileio.args[0].block_func = block_func;
    fileio.args[1].void_ptr_t = ctx;
    fileio.is_block_func = 1;
    coroutine_hook_fileio_run_part3();
    return retval.void_ptr_t;
}
/* }}} */

} /* namespace zcc */

/* ############## SYS CALL  HOOK ############################## */
extern "C"
{
/* {{{ general read/write wait */

static int ___general_read_wait(int fd)
{
    zcc::coroutine_fd_attribute *cfa =  zcc::coroutine_fd_attribute_get(fd);
	struct pollfd pf;
	pf.fd = fd;
	pf.events = (POLLIN | POLLERR | POLLHUP);
    pf.revents = 0;
	poll(&pf, 1, cfa->read_timeout);
    return pf.revents;
}

static int ___general_write_wait(int fd)
{
    zcc::coroutine_fd_attribute *cfa =  zcc::coroutine_fd_attribute_get(fd);
	struct pollfd pf;
	pf.fd = fd;
	pf.events = (POLLOUT | POLLERR | POLLHUP);
    pf.revents = 0;
	poll(&pf, 1, cfa->write_timeout);
    return pf.revents;
}

/* }}} */

/* {{{ sleep */
unsigned int sleep(unsigned int seconds)
{
    long end = zcc::timeout_set(seconds * 1000);
    zcc::sleep(seconds);
    long left = zcc::timeout_left(end);
    if (left < 0) {
        return 0;
    }
    return left;
}
/* }}} */

/* {{{ poll hook */
int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    if (timeout < 1) {
        return zcc::syscall_poll(fds, nfds, 0);
    }
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    if ((cobs == 0) || (cobs->current_coroutine == 0)) {
        return zcc::syscall_poll(fds, nfds, timeout);
    }
    return  zcc::coroutine_poll(cobs->current_coroutine, fds, nfds, timeout);
}

int __poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    return poll(fds, nfds, timeout);
}

/* }}} */

/* {{{ pipe hook */
int pipe(int pipefd[2])
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    int ret = zcc::syscall_pipe(pipefd);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    zcc::coroutine_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    zcc::coroutine_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ pipe2 hook */
int pipe2(int pipefd[2], int flags)
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    int ret = zcc::syscall_pipe2(pipefd, flags);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;;
    }
    zcc::coroutine_fd_attribute_create(pipefd[0]);
    fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0));
    zcc::coroutine_fd_attribute_create(pipefd[1]);
    fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ dup hook */
int dup(int oldfd)
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    int newfd = zcc::syscall_dup(oldfd);
    if (newfd < 0) {
        return newfd;
    }
    if (!cobs) {
        return newfd;
    }
    zcc::coroutine_fd_attribute *cfa = zcc::coroutine_fd_attribute_get(oldfd);
    zcc::coroutine_fd_attribute *new_cfa = zcc::coroutine_fd_attribute_create(newfd);
    if (cfa) {
        new_cfa->is_regular_file = cfa->is_regular_file;
        new_cfa->pseudo_mode = cfa->pseudo_mode;
        new_cfa->read_timeout = cfa->read_timeout;
        new_cfa->write_timeout = cfa->write_timeout;
        if (cfa && (cfa->pseudo_mode == 0)) {
            fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0));
        }
    }
    return newfd;
}
/* }}} */

/* {{{ dup2 hook */
int dup2(int oldfd, int newfd)
{
    int ret;
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    if ((ret = zcc::syscall_dup2(oldfd, newfd)) < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    zcc::coroutine_fd_attribute *cfa = zcc::coroutine_fd_attribute_get(oldfd);
    if (cfa && (cfa->pseudo_mode == 0)) {
        cfa = zcc::coroutine_fd_attribute_get(newfd);
        if (cfa) {
            zcc_fatal("the newfd be used by other coroutine");
#if 0
            /* note: the newfd be used by other coroutine. */
            zcc::coroutine_fd_attribute_free(newfd);
            zcc::coroutine_fd_attribute_create(newfd);
#endif
        } else {
            zcc::coroutine_fd_attribute *new_cfa = zcc::coroutine_fd_attribute_create(newfd);
            new_cfa->is_regular_file = cfa->is_regular_file;
            new_cfa->pseudo_mode = cfa->pseudo_mode;
            new_cfa->read_timeout = cfa->read_timeout;
            new_cfa->write_timeout = cfa->write_timeout;
        }
        fcntl(newfd, F_SETFL, fcntl(newfd, F_GETFL, 0));
    }
    return ret;
}
/* }}} */

/* {{{ socketpair hook */
int socketpair(int domain, int type, int protocol, int sv[2])
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    int ret = zcc::syscall_socketpair(domain, type, protocol, sv);
    if (ret < 0) {
        return ret;
    }
    if (!cobs) {
        return ret;
    }
    zcc::coroutine_fd_attribute_create(sv[0]);
    fcntl(sv[0], F_SETFL, fcntl(sv[0], F_GETFL, 0));
    zcc::coroutine_fd_attribute_create(sv[1]);
    fcntl(sv[1], F_SETFL, fcntl(sv[1], F_GETFL, 0));
    return ret;
}
/* }}} */

/* {{{ open hook */
int open(const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_open(pathname, flags, mode);
    }
    coroutine_hook_fileio_run_part2(open);
    fileio.args[0].void_ptr_t = (void *)pathname;
    fileio.args[1].int_t = flags;
    fileio.args[2].int_t = mode;
    coroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        zcc::coroutine_fd_attribute *fdatts = zcc::coroutine_fd_attribute_create(retfd);
        fdatts->is_regular_file = fileio.is_regular_file;
        if (fileio.is_regular_file == 0) {
            fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
        }
    }
	return retfd;
}

int openat(int dirid, const char *pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_openat(dirid, pathname, flags, mode);
    }
    coroutine_hook_fileio_run_part2(open);
    fileio.args[0].int_t = dirid;
    fileio.args[1].void_ptr_t = (void *)pathname;
    fileio.args[2].int_t = flags;
    fileio.args[3].int_t = mode;
    coroutine_hook_fileio_run_part3();
    int retfd = retval.int_t;
    if (retfd > -1) {
        zcc::coroutine_fd_attribute *fdatts = zcc::coroutine_fd_attribute_create(retfd);
        fdatts->is_regular_file = fileio.is_regular_file;
        if (fileio.is_regular_file == 0) {
            fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
        }
    }
	return retfd;
}
/* }}} */

/* {{{ creat hook */
int creat(const char *pathname, mode_t mode)
{
    return open (pathname, O_WRONLY|O_CREAT|O_TRUNC, mode);
}
/* }}} */

/* {{{ socket hook */
int socket(int domain, int type, int protocol)
{
	int fd = zcc::syscall_socket(domain, type, protocol);
	if(fd < 0) {
		return fd;
	}

    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    if (cobs == 0) {
        return fd;
    }

    zcc::coroutine_fd_attribute_create(fd);
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0));

	return fd;
}
/* }}} */

/* {{{ return_zcc_call */
#define return_zcc_call_co(fd)  \
    zcc::coroutine_base *cobs = 0; \
    zcc::coroutine_fd_attribute *fdatts = 0; \
    if (((cobs = zcc::coroutine_base_get_current()) ==0) \
            || ((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0)  \
            || (fdatts->nonblock == 1) \
            || (fdatts->pseudo_mode == 1))

/* }}} */

/* {{{ accept hook */
int accept(int fd, struct sockaddr *addr, socklen_t *len)
{
    const int ___accept_timeout = 100 * 1000;
    return_zcc_call_co(fd) {
        int sock = zcc::syscall_accept(fd, addr, len);
        if (cobs && (sock > -1)) {
            zcc::coroutine_fd_attribute_create(sock);
            fcntl(sock, F_SETFL, zcc::syscall_fcntl(sock, F_GETFL,0));
        }
        return sock;
    }
	struct pollfd pf;
    memset(&pf,0,sizeof(pf));
    pf.fd = fd;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    poll(&pf, 1, ___accept_timeout);
	if (pf.revents & (POLLERR|POLLHUP)) {
        errno = ECONNABORTED;
		return -1;
	}
    if (!(pf.revents & (POLLIN))) {
        errno = EINTR;
		return -1;
    }

    int sock = zcc::syscall_accept(fd, addr, len);
    if (sock > -1) {
        zcc::coroutine_fd_attribute_create(sock);
        fcntl(sock, F_SETFL, zcc::syscall_fcntl(sock, F_GETFL,0));
    } else {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
    return sock;
}

/* }}} */

/* {{{ connect hook */
int connect(int fd, const struct sockaddr *address, socklen_t address_len)
{
    const int ___connect_timeout = 100 * 1000;
    int ret = zcc::syscall_connect(fd, address, address_len);
    return_zcc_call_co(fd) {
        return ret;
    }
    
	if (!((ret < 0) && (errno == EINPROGRESS))) {
		return ret;
	}

	struct pollfd pf;
    memset(&pf,0,sizeof(pf));
    pf.fd = fd;
    pf.events = (POLLOUT | POLLERR | POLLHUP);
    poll(&pf, 1, ___connect_timeout);
	if (pf.revents & POLLOUT) {
		errno = 0;
		return 0;
	}

	int err = 0;
	socklen_t errlen = sizeof(err);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
	if(err) {
		errno = err;
	} else {
		errno = ETIMEDOUT;
	} 
	return ret;
}
/* }}} */

/* {{{ close hook */
int close(int fd)
{
    int ret;
    coroutine_hook_fileio_run_part1() {
        ret = zcc::syscall_close(fd);
        if (ret > -1) {
            zcc::coroutine_fd_attribute_free(fd);
        }
        return ret;
    }

    zcc::coroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        ret = zcc::syscall_close(fd);
        if (ret > -1) {
            zcc::coroutine_fd_attribute_free(fd);
        }
        return ret;
    }
    if (fdatts->is_regular_file) {
        coroutine_hook_fileio_run_part2(close);
        fileio.args[0].int_t = fd;
        coroutine_hook_fileio_run_part3();
        ret = retval.int_t;
        if (ret > -1) {
            zcc::coroutine_fd_attribute_free(fd);
        }
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return ret;
    }
    ret = zcc::syscall_close(fd);
    if (ret > -1) {
        zcc::coroutine_fd_attribute_free(fd);
    }
    return ret;
}
/* }}} */

/* {{{ read hook */
ssize_t read(int fd, void *buf, size_t nbyte)
{
    coroutine_hook_fileio_run_part0() {
        return zcc::syscall_read(fd, buf, nbyte);
    }

    zcc::coroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zcc::syscall_read(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zcc::var_coroutine_block_pthread_count_limit < 1) {
            return zcc::syscall_read(fd, buf, nbyte);
        }
        coroutine_hook_fileio_run_part2(read);
        fileio.args[0].int_t = fd;
        fileio.args[1].void_ptr_t = buf;
        fileio.args[2].size_size_t = nbyte;
        coroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return zcc::syscall_read(fd, buf, nbyte);
    }
#if 0
    ___general_read_wait(fd);
	ssize_t readret = zcc::syscall_read(fd,(char*)buf ,nbyte);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return readret;
#else 
    while(1) {
        ___general_read_wait(fd);
        ssize_t readret = zcc::syscall_read(fd,(char*)buf ,nbyte);
        int ec = errno;
        if ((readret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return readret;
        }
    }
	return -1;
#endif
}
/* }}} */


/* {{{ readv hook */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    coroutine_hook_fileio_run_part0() {
        return zcc::syscall_readv(fd, iov, iovcnt);
    }

    zcc::coroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zcc::syscall_readv(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zcc::var_coroutine_block_pthread_count_limit < 1) {
            return zcc::syscall_readv(fd, iov, iovcnt);
        }
        coroutine_hook_fileio_run_part2(readv);
        fileio.args[0].int_t = fd;
        fileio.args[1].const_iovec_t = iov;
        fileio.args[2].int_t = iovcnt;
        coroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return zcc::syscall_readv(fd, iov, iovcnt);
    }
#if 0
    ___general_read_wait(fd);
	ssize_t readret = zcc::syscall_readv(fd, iov, iovcnt);
    if (readret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return readret;
#else
    while(1) {
        ___general_read_wait(fd);
        ssize_t readret = zcc::syscall_readv(fd, iov, iovcnt);
        int ec = errno;
        if ((readret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return readret;
        }
    }
	return -1;
#endif
}
/* }}} */

/* {{{ write hook */
ssize_t write(int fd, const void *buf, size_t nbyte)
{
    coroutine_hook_fileio_run_part0() {
        return zcc::syscall_write(fd, buf, nbyte);
    }

    zcc::coroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zcc::syscall_write(fd, buf, nbyte);
    }
    if (fdatts->is_regular_file) {
        if (zcc::var_coroutine_block_pthread_count_limit < 1) {
            return zcc::syscall_write(fd, buf, nbyte);
        }
        coroutine_hook_fileio_run_part2(write);
        fileio.args[0].int_t = fd;
        fileio.args[1].void_ptr_t = (void *)buf;
        fileio.args[2].size_size_t = nbyte;
        coroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return zcc::syscall_write(fd, buf, nbyte);
    }

#if 0
    ___general_write_wait(fd);
	ssize_t writeret = zcc::syscall_write(fd, buf ,nbyte);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return writeret;
#else
    while(1) {
        ___general_write_wait(fd);
        ssize_t writeret = zcc::syscall_write(fd, buf ,nbyte);
        int ec = errno;
        if ((writeret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return writeret;
        }
    }
	return -1;
#endif
}
/* }}} */

/* {{{ writev hook */
ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    coroutine_hook_fileio_run_part0() {
        return zcc::syscall_writev(fd, iov, iovcnt);
    }

    zcc::coroutine_fd_attribute *fdatts = 0;
    if (((fdatts = zcc::coroutine_fd_attribute_get(fd)) == 0) || (fdatts->pseudo_mode == 1)) {
        return zcc::syscall_writev(fd, iov, iovcnt);
    }
    if (fdatts->is_regular_file) {
        if (zcc::var_coroutine_block_pthread_count_limit < 1) {
            return zcc::syscall_writev(fd, iov, iovcnt);
        }
        coroutine_hook_fileio_run_part2(write);
        fileio.args[0].int_t = fd;
        fileio.args[1].const_iovec_t = iov;
        fileio.args[2].int_t = iovcnt;
        coroutine_hook_fileio_run_part3();
        ssize_t retfd = retval.ssize_ssize_t;
#if 0
        fcntl(retfd, F_SETFL, fcntl(retfd, F_GETFL, 0));
#endif
        return retfd;
    }

    if (fdatts->nonblock) {
        return zcc::syscall_writev(fd, iov, iovcnt);
    }
#if 0
    ___general_write_wait(fd);
	ssize_t writeret = zcc::syscall_writev(fd, iov, iovcnt);
    if (writeret < 0) {
        if (errno == EAGAIN) {
            errno = EINTR;
        }
    }
	return writeret;
#else
    while(1) {
        ___general_write_wait(fd);
        ssize_t writeret = zcc::syscall_writev(fd, iov, iovcnt);
        int ec = errno;
        if ((writeret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return writeret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ sendto hook */
ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    int ret;
    return_zcc_call_co(socket) {
        return zcc::syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    }
#if 0
    ___general_write_wait(socket);
    ret = zcc::syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        ___general_write_wait(socket);
        ret = zcc::syscall_sendto(socket,message,length,flags,dest_addr,dest_len);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ recvfrom hook */
ssize_t recvfrom(int socket, void *buf, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    return_zcc_call_co(socket) {
		return zcc::syscall_recvfrom(socket,buf,length,flags,address,address_len);
    }
#if 0
    ___general_read_wait(socket);
	ssize_t ret = zcc::syscall_recvfrom(socket,buf,length,flags,address,address_len);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
	return ret;
#else
    while(1) {
        ___general_read_wait(socket);
        ssize_t ret = zcc::syscall_recvfrom(socket,buf,length,flags,address,address_len);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ send hook */
ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
    return_zcc_call_co(socket) {
		return zcc::syscall_send(socket,buffer,length,flags);
    }
#if 0
    ___general_write_wait(socket);
    int ret = zcc::syscall_send(socket,(const char*)buffer, length, flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
    return ret;
#else
    while(1) {
        ___general_write_wait(socket);
        int ret = zcc::syscall_send(socket,(const char*)buffer, length, flags);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif

}
/* }}} */

/* {{{ recv hook */
ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
    return_zcc_call_co(socket) {
		return zcc::syscall_recv(socket,buffer,length,flags);
    }
#if 0
    ___general_read_wait(socket);
	ssize_t ret = zcc::syscall_recv(socket,buffer,length,flags);
    if (ret > -1) {
        return ret;
    }
    if (errno == EAGAIN) {
        errno = EINTR;
    }
	return ret;
#else
    while(1) {
        ___general_read_wait(socket);
        ssize_t ret = zcc::syscall_recv(socket,buffer,length,flags);
        int ec = errno;
        if ((ret >= 0) || (ec == EINTR) || (ec != EAGAIN)) {
            return ret;
        }
    }
    return -1;
#endif
}
/* }}} */

/* {{{ setsockopt hook */
int setsockopt(int fd, int level, int option_name, const void *option_value, socklen_t option_len)
{
    return_zcc_call_co(fd) {
		return zcc::syscall_setsockopt(fd,level,option_name,option_value,option_len);
    }

	if(SOL_SOCKET == level) {
		struct timeval *val = (struct timeval*)option_value;
        long t = val->tv_sec * 1000 + val->tv_usec/1000;
        if (t > 256 * 128 -1) {
            t = 256 * 128 -1;
        }
        if (t < 0) {
            t = 1;
        }
		if(SO_RCVTIMEO == option_name ) {
            fdatts->read_timeout = t;
		} else if(SO_SNDTIMEO == option_name) {
            fdatts->write_timeout = t;
		}
	}
	return zcc::syscall_setsockopt(fd,level,option_name,option_value,option_len);
}
/* }}} */

/* {{{ fcntl hook */
int fcntl(int fildes, int cmd, ...)
{
	if(fildes < 0) {
        errno = EINVAL;
		return -1;
	}
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();

	va_list args;
	va_start(args,cmd);

	int ret = -1;
	switch(cmd)
	{
		case F_DUPFD:
		{
			int param = va_arg(args,int);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
            if (cobs == 0) {
                break;
            }
            if (ret > -1) {
                zcc::coroutine_fd_attribute *cfa = zcc::coroutine_fd_attribute_get(fildes);
                if (cfa && (cfa->pseudo_mode == 0)) {
                    zcc::coroutine_fd_attribute_create(ret);
                    fcntl(ret, F_SETFL, zcc::syscall_fcntl(ret, F_GETFL,0));
                }
            }
			break;
		}
		case F_GETFD:
		{
			ret = zcc::syscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETFD:
		{
			int param = va_arg(args,int);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_GETFL:
		{
			ret = zcc::syscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETFL:
		{
			int param = va_arg(args,int);
            if (cobs == 0) {
                ret = zcc::syscall_fcntl(fildes,cmd,param);
                break;
            }
			int flag = param;
            zcc::coroutine_fd_attribute *cfa = zcc::coroutine_fd_attribute_get(fildes);
            if (cfa) {
				flag |= O_NONBLOCK;
			}
			ret = zcc::syscall_fcntl(fildes,cmd,flag);
			if((0 == ret) && cfa) {
                cfa->nonblock = ((param&O_NONBLOCK)?1:0);
			}
			break;
		}
		case F_GETOWN:
		{
			ret = zcc::syscall_fcntl(fildes,cmd);
			break;
		}
		case F_SETOWN:
		{
			int param = va_arg(args,int);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_GETLK:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_SETLK:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
			break;
		}
		case F_SETLKW:
		{
			struct flock *param = va_arg(args,struct flock *);
			ret = zcc::syscall_fcntl(fildes,cmd,param);
			break;
		}
	}
	va_end(args);

	return ret;
}
/* }}} */

/* {{{ __res_state  hook */
extern __thread struct __res_state *__resp;
res_state __res_state2(void)
{
    printf("__res_state2 AAAAAAAAAAAAAAAAAAAAAAAAAA new\n");
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    if (cobs == 0) {
        return __resp;
    }
    printf("__res_state2 AAAAAAAAAAAAAAAAAAAAAAAAAA new\n");
    if (cobs->current_coroutine->___res_state == 0) {
        cobs->current_coroutine->___res_state = (struct __res_state *)zcc::calloc(1, sizeof(struct __res_state));
    }
    return cobs->current_coroutine->___res_state;
}
/* }}} */

/* {{{ gethostbyname  hook */
struct hostent *gethostbyname(const char *name)
{
    return gethostbyname2(name, AF_INET);
}

struct hostent* gethostbyname2(const char* name, int af)
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    char **_hp_char_ptr = 0;
    zcc::gethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->___gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(zcc::___gethostbyname_pthread_key);
        _hp = (zcc::gethostbyname_buf_t *)(*_hp_char_ptr);
    }
   
    if (_hp && (_hp->buf_size > 1024)) {
        zcc::free(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zcc::gethostbyname_buf_t *)zcc::malloc(sizeof(zcc::gethostbyname_buf_t) + 1024 + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zcc::gethostbyname_buf_t);
        _hp->buf_size = 1024;
        if (cobs) {
            cobs->current_coroutine->___gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    }
    struct hostent *host = &(_hp->host);
    struct hostent *result = NULL;
    int *h_errnop = &(_hp->h_errno2);

    int ret = -1;
    while (((ret = gethostbyname2_r(name, af, host, _hp->buf_ptr, _hp->buf_size, &result, h_errnop)) == ERANGE) && (*h_errnop == NETDB_INTERNAL)) {
        size_t nsize = _hp->buf_size * 2;
        zcc::free(_hp);
        _hp = (zcc::gethostbyname_buf_t *)zcc::malloc(sizeof(zcc::gethostbyname_buf_t) + nsize + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zcc::gethostbyname_buf_t);
        _hp->buf_size = nsize;
        if (cobs) {
            cobs->current_coroutine->___gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    } 
    if ((ret == 0) && (host == result)){
        return host;
    }
    h_errno = _hp->h_errno2;
    return 0;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type)
{
    zcc::coroutine_base *cobs = zcc::coroutine_base_get_current();
    char **_hp_char_ptr = 0;
    zcc::gethostbyname_buf_t *_hp;
    if (cobs) {
        _hp = cobs->current_coroutine->___gethostbyname;
    } else {
        _hp_char_ptr = (char **)pthread_getspecific(zcc::___gethostbyname_pthread_key);
        _hp = (zcc::gethostbyname_buf_t *)(*_hp_char_ptr);
    }
   
    if (_hp && (_hp->buf_size > 1024)) {
        zcc::free(_hp);
        _hp = 0;
    }
    if (_hp == 0) {
        _hp = (zcc::gethostbyname_buf_t *)zcc::malloc(sizeof(zcc::gethostbyname_buf_t) + 1024 + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zcc::gethostbyname_buf_t);
        _hp->buf_size = 1024;
        if (cobs) {
            cobs->current_coroutine->___gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    }
    struct hostent *host = &(_hp->host);
    struct hostent *result = NULL;
    int *h_errnop = &(_hp->h_errno2);

    int ret = -1;
    while (((ret = gethostbyaddr_r(addr, len, type, host, _hp->buf_ptr, _hp->buf_size, &result, h_errnop)) == ERANGE) && (*h_errnop == NETDB_INTERNAL)) {
        size_t nsize = _hp->buf_size * 2;
        zcc::free(_hp);
        _hp = (zcc::gethostbyname_buf_t *)zcc::malloc(sizeof(zcc::gethostbyname_buf_t) + nsize + 1);
        _hp->buf_ptr = ((char *)_hp) + sizeof(zcc::gethostbyname_buf_t);
        _hp->buf_size = nsize;
        if (cobs) {
            cobs->current_coroutine->___gethostbyname = _hp;
        } else {
            *_hp_char_ptr = (char *)_hp;
        }
    } 
    if ((ret == 0) && (host == result)){
        return host;
    }
    h_errno = _hp->h_errno2;
    return 0;
}

/* }}} */

/* file io hook {{{ */
off_t lseek(int fd, off_t offset, int whence)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_lseek(fd, offset, whence);
    }
    coroutine_hook_fileio_run_part2(lseek);
    fileio.args[0].int_t = fd;
    fileio.args[1].off_off_t = offset;
    fileio.args[2].int_t = whence;
    coroutine_hook_fileio_run_part3();
    return retval.off_off_t;
}

int fdatasync(int fd)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_fdatasync(fd);
    }
    coroutine_hook_fileio_run_part2(fdatasync);
    fileio.args[0].int_t = fd;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fsync(int fd)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_fsync(fd);
    }
    coroutine_hook_fileio_run_part2(fsync);
    fileio.args[0].int_t = fd;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int rename(const char *oldpath, const char *newpath)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_rename(oldpath, newpath);
    }
    coroutine_hook_fileio_run_part2(rename);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int truncate(const char *path, off_t length)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_truncate(path, length);
    }
    coroutine_hook_fileio_run_part2(truncate);
    fileio.args[0].const_char_ptr_t = path;
    fileio.args[1].long_t = (long)length;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int ftruncate(int fd, off_t length)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_ftruncate(fd, length);
    }
    coroutine_hook_fileio_run_part2(ftruncate);
    fileio.args[0].int_t = fd;
    fileio.args[1].long_t = (long)length;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int rmdir(const char *pathname)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_rmdir(pathname);
    }
    coroutine_hook_fileio_run_part2(rmdir);
    fileio.args[0].const_char_ptr_t = pathname;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int mkdir(const char *pathname, mode_t mode)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_mkdir(pathname, mode);
    }
    coroutine_hook_fileio_run_part2(mkdir);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_getdents(fd, (void *)dirp, count);
    }
    coroutine_hook_fileio_run_part2(getdents);
    fileio.args[0].uint_t = fd;
    fileio.args[1].void_ptr_t = dirp;
    fileio.args[1].uint_t = count;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int stat(const char *pathname, struct stat *buf)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_stat(pathname, buf);
    }
    coroutine_hook_fileio_run_part2(stat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fstat(int fd, struct stat *buf)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_fstat(fd, buf);
    }
    coroutine_hook_fileio_run_part2(fstat);
    fileio.args[0].int_t = fd;
    fileio.args[1].void_ptr_t = buf;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int lstat(const char *pathname, struct stat *buf)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_lstat(pathname, buf);
    }
    coroutine_hook_fileio_run_part2(lstat);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].void_ptr_t = buf;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int link(const char *oldpath, const char *newpath)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_link(oldpath, newpath);
    }
    coroutine_hook_fileio_run_part2(link);
    fileio.args[0].const_char_ptr_t = oldpath;
    fileio.args[1].const_char_ptr_t = newpath;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int symlink(const char *target, const char *linkpath)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_symlink(target, linkpath);
    }
    coroutine_hook_fileio_run_part2(symlink);
    fileio.args[0].const_char_ptr_t = target;
    fileio.args[1].const_char_ptr_t = linkpath;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_readlink(pathname, buf, bufsiz);
    }
    coroutine_hook_fileio_run_part2(readlink);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].char_ptr_t = buf;
    fileio.args[2].size_size_t = bufsiz;
    coroutine_hook_fileio_run_part3();
    return retval.ssize_ssize_t;
}

int unlink(const char *pathname)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_unlink(pathname);
    }
    coroutine_hook_fileio_run_part2(unlink);
    fileio.args[0].const_char_ptr_t = pathname;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int chmod(const char *pathname, mode_t mode)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_chmod(pathname, mode);
    }
    coroutine_hook_fileio_run_part2(chmod);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].mode_mode_t = mode;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fchmod(int fd, mode_t mode)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_fchmod(fd, mode);
    }
    coroutine_hook_fileio_run_part2(fchmod);
    fileio.args[0].int_t = fd;
    fileio.args[1].mode_mode_t = mode;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int chown(const char *pathname, uid_t owner, gid_t group)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_chown(pathname, owner, group);
    }
    coroutine_hook_fileio_run_part2(chown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_fchown(fd, owner, group);
    }
    coroutine_hook_fileio_run_part2(fchown);
    fileio.args[0].int_t = fd;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int lchown(const char *pathname, uid_t owner, gid_t group)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_lchown(pathname, owner, group);
    }
    coroutine_hook_fileio_run_part2(lchown);
    fileio.args[0].const_char_ptr_t = pathname;
    fileio.args[1].uid_uid_t = owner;
    fileio.args[2].gid_gid_t = group;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int utime(const char *filename, const struct utimbuf *times)
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_utime(filename, times);
    }
    coroutine_hook_fileio_run_part2(utime);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

int utimes(const char *filename, const struct timeval times[2])
{
    coroutine_hook_fileio_run_part1() {
        return zcc::syscall_utimes(filename, times);
    }
    coroutine_hook_fileio_run_part2(utimes);
    fileio.args[0].const_char_ptr_t = filename;
    fileio.args[1].const_char_ptr_t = (const char *)times;
    coroutine_hook_fileio_run_part3();
    return retval.int_t;
}

/* }}} */

} /* extern "C" */

#pragma pack(pop)
/* Local variables:
* End:
* vim600: fdm=marker
*/

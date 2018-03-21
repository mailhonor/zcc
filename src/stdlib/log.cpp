/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-13
 * ================================
 */

#include "zcc.h"
#include <syslog.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace zcc
{
static volatile int fatal_times = 0;
static void vprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap);

static void vprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, " [%s:%zu]\n", source_fn, line_number);
}
void (*log_vprintf) (const char *source_fn, size_t line_number, const char *fmt, va_list ap) = vprintf_default;

void log_info(const char *source_fn, size_t line_number, const char *fmt, ...)
{
    va_list ap;
    if (log_vprintf) {
        va_start(ap, fmt);
        log_vprintf(source_fn, line_number, fmt, ap);
        va_end(ap);
    }
}

void log_fatal(const char *source_fn, size_t line_number, const char *fmt, ...)
{
    va_list ap;

    if (fatal_times++ == 0) {
        if (log_vprintf) {
            va_start(ap, fmt);
            log_vprintf(source_fn, line_number, fmt, ap);
            va_end(ap);
        }
    }

    if (var_log_fatal_catch) {
        /* 段错误,方便 gdb 调试 */
        char *p = 0;
        *p = 0;
    }

    _exit(1);
}

/* SYSLOG ############################################## */
static void vprintf_syslog(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char buf[10240 + 10];
    vsnprintf(buf, 10240, fmt, ap);
    syslog(LOG_INFO, "%s [%s:%ld]", buf, source_fn, (long)line_number);
}

void log_use_syslog(const char *facility, const char *identity)
{
    int fa = LOG_SYSLOG;
    if (!strncasecmp(facility, "LOG_", 4)) {
        facility += 4;
    }
#define ___LOG_S_I(S)   if (!strcasecmp(#S, facility)) { fa = LOG_ ## S; }
    ___LOG_S_I(KERN);
    ___LOG_S_I(USER);
    ___LOG_S_I(MAIL);
    ___LOG_S_I(DAEMON);
    ___LOG_S_I(AUTH);
    ___LOG_S_I(SYSLOG);
    ___LOG_S_I(LPR);
    ___LOG_S_I(NEWS);
    ___LOG_S_I(UUCP);
    ___LOG_S_I(CRON);
    ___LOG_S_I(AUTHPRIV);
    ___LOG_S_I(FTP);
    ___LOG_S_I(LOCAL0);
    ___LOG_S_I(LOCAL1);
    ___LOG_S_I(LOCAL2);
    ___LOG_S_I(LOCAL3);
    ___LOG_S_I(LOCAL4);
    ___LOG_S_I(LOCAL5);
    ___LOG_S_I(LOCAL6);
    ___LOG_S_I(LOCAL7);
#undef ___LOG_S_I

    log_use_syslog(fa, identity);
}

void log_use_syslog(int facility, const char *identity)
{
    openlog(identity, LOG_NDELAY | LOG_PID, facility);
    log_vprintf = vprintf_syslog;
}

/* MASTER LOG ########################################################## */
std::string var_masterlog_listen;
static autobuffer var_masterlog_prefix;
static int var_masterlog_prefix_len = 0;
static int var_masterlog_sock = -1;
struct sockaddr_un var_masterlog_client_un;

static void vprintf_masterlog(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char *buf = (char *)malloc(10240 + 10);
    int len = 0, tmplen;
    int left = 10240;

    memcpy(buf, var_masterlog_prefix.data, var_masterlog_prefix_len);
    len = var_masterlog_prefix_len;
    left = left - len;

    if (left > 1) {
        vsnprintf(buf + len, left, fmt, ap);
        tmplen = strlen(buf + len);
        len += tmplen;
        left -= tmplen;
    }
    if (left > 1) {
        snprintf(buf + len, left, " [%s:%ld]",  source_fn, (long)line_number);
        tmplen = strlen(buf + len);
        len += tmplen;
        left -= tmplen;
    }
    buf[len] = 0;
    sendto(var_masterlog_sock,buf,len,0,(struct sockaddr *)(&var_masterlog_client_un),sizeof(struct sockaddr_un));
    free(buf);
}

void log_use_masterlog(const char *dest, const char *identity)
{
    log_vprintf = vprintf_masterlog;
    char buf[256];
    snprintf(buf, 255, "%s,%d,", identity, getpid());
    var_masterlog_prefix.data = strdup(buf);
    var_masterlog_prefix_len = strlen(buf);


    int syscall_socket(int domain, int type, int protocol);
    if ((var_masterlog_sock=syscall_socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "ERR socket (%m)");
        exit(1);
    }
    memset(&var_masterlog_client_un, 0, sizeof(struct sockaddr_un));
    var_masterlog_client_un.sun_family = AF_UNIX;
    if (strlen(dest) >= sizeof(var_masterlog_client_un.sun_path)) {
        fprintf(stderr, "socket unix path too long: %s", dest);
        exit(1);
    }
    strcpy(var_masterlog_client_un.sun_path, dest);
#if 0
    coroutine_disable_fd(var_masterlog_sock);
#endif
}

/* log_use_by_config ###################################################### */
bool log_use_by(char *progname, char *log_info)
{
    argv lv;
    lv.split(log_info, ", ");
    char *type = lv[0];
    if ((lv.size()== 0) || empty(type)) {
        return false;
    }
    char *facility = 0;
    char *identity = progname;
    identity = strrchr(identity, '/');
    if (identity) {
        identity++;
    } else {
        identity = progname;
    }
    if (!strcmp(type, "syslog")) {
        if ((lv.size() <2) || (lv.size() > 3)) {
            zcc_fatal("syslog mode, value: syslog,facility,identity or syslog,facility");
        }
        facility = lv[1];
        if (lv.size() == 3) {
            identity = lv[2];
        }
        log_use_syslog(facility, identity);
        return true;
    } else if (!strcmp(type, "masterlog")) {
        if (var_masterlog_listen.empty()) {
            zcc_info("masterlog mode, need parameter '-log-service x,x' on master cmd");
            return false;
        }
        if (lv.size() > 2) {
            zcc_fatal("masterlog mode, value: masterlog,identity or masterlog");
        }
        if (lv.size() == 2) {
            identity = lv[1];
        }
        log_use_masterlog(var_masterlog_listen.c_str(), identity);
        return true;
    }
    return false;
}

}

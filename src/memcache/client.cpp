/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

memcache_client::memcache_client()
{
    r_fd = -1;
    r_timeout = -1;
}

memcache_client::memcache_client(const char *destination)
{
    r_fd = -1;
    r_timeout = -1;
    open(destination);
}

memcache_client::~memcache_client()
{
    close();
}

void memcache_client::open(const char *destination)
{
    close();
    r_destination = (destination?destination:"");
}

void memcache_client::open()
{
    close();
    r_fd = connect(r_destination.c_str());
}

void memcache_client::close()
{
    if (r_fd > -1) {
        ::close(r_fd);
        r_fd = -1;
    }
}

#define ___check_fd() { \
    if (r_fd == -1) { open(); } \
    if (r_fd == -1) { r_msg = "can not open "; r_msg.append(r_destination); return -1; } \
}

#define ___check_key() { if (!is_valid_key(key)) { r_msg = "key invalid";  return -1; } }
static bool is_valid_key(const char *key)
{
    if (empty(key)) {
        return false;
    }
    while (*key) {
        if (iscntrl(*key) || isblank(*key)) {
            return false;
        }
        key ++;
    }
    return true;
}

int memcache_client::get(std::string &result, int *flags, const char *key)
{
    ___check_fd();
    ___check_key();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.printf_1024("get %s\r\n", key);
    bool have_val = false;
    bool protocol_ok = false;
    while(1) {
        str.clear();
        if (fp.gets(str) < 0) {
            close();
            r_msg = "read/write error";
            return -1;
        }
        char *p, *ps = const_cast<char *>(str.c_str());
        if (!strncmp(ps, "VALUE ", 6)) {
            p = strchr(ps + 6, ' ');
            if (!p) {
                break;
            }
            ps = p + 1;
            p = strchr(ps, ' ');
            if (!p) {
                break;
            }
            if (flags) {
                *p = 0;
                *flags = atoi(ps);
                *p = ' ';
            }
            ps = p + 1;
            int len = atoi(ps);
            if (len < 0) {
                break;
            }
            result.clear();
            if (fp.readn(result, len) < len) {
                break;
            }
            fp.readn(0, 2);
            have_val = true;
            continue;
        }
        if (!strncmp(ps, "END", 3)) {
            protocol_ok = true;
            break;
        }
        fp.close();
        close();
        r_msg = ps;
        return -1;
    }
    if (!protocol_ok) {
        fp.close();
        close();
        r_msg = "read/write error";
        return -1;
    }

    return (have_val?1:0);
}

int memcache_client::set(const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    return op_set("set", key, flags, timeout_second, data, size);
}


int memcache_client::add(const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    return op_set("add", key, flags, timeout_second, data, size);
}


int memcache_client::replace(const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    return op_set("replace", key, flags, timeout_second, data, size);
}


int memcache_client::append(const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    return op_set("append", key, flags, timeout_second, data, size);
}


int memcache_client::prepend(const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    return op_set("prepend", key, flags, timeout_second, data, size);
}


long memcache_client::incr(const char *key, size_t n)
{
    return op_incr("incr", key, n);
}


long memcache_client::decr(const char *key, size_t n)
{
    return op_incr("decr", key, n);
}


int memcache_client::del(const char *key)
{
    ___check_fd();
    ___check_key();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.printf_1024("delete %s\r\n", key);
    str.clear();
    if (fp.gets(str) < 0) {
        close();
        r_msg = "read/write error";
        return -1;
    }
    char *ps = const_cast<char *>(str.c_str());

    if (!strncmp(ps, "DELETED", 7)) {
        return 1;
    } else if (!strncmp(ps, "NOT_FOUND", 9)) {
        return 0;
    }

    return -1;
}


int memcache_client::flush_all(long after_second)
{
    ___check_fd();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.puts("flush");
    if (after_second != 0) {
        fp.printf_1024(" %ld\r\n", after_second);
    } else {
        fp.puts("\r\n");
    }
    str.clear();
    if (fp.gets(str) < 0) {
        close();
        r_msg = "read/write error";
        return -1;
    }
    char *ps = const_cast<char *>(str.c_str());

    if (!strncmp(ps, "OK", 3)) {
        return 1;
    }

    return -1;
}


int memcache_client::quit()
{
    ___check_fd();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.puts("flush\r\n");
    fp.flush();
    close();
    return 1;
}

int memcache_client::version(std::string &result)
{
    ___check_fd();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.puts("version\r\n");
    str.clear();
    if (fp.gets(str) < 0) {
        close();
        r_msg = "read/write error";
        return -1;
    }
    char *ps = const_cast<char *>(str.c_str());
    if (strncmp(ps, "VERSION ", 8)) {
        return -1;
    }
    result = ps+8;

    return 1;
}

int memcache_client::op_set(const char *op, const char *key, int flags, long timeout_second, const void *data, size_t size)
{
    ___check_fd();
    ___check_key();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    if (timeout_second >= 30 * 24 * 3600) {
        timeout_second = time(0) + timeout_second;
    }
    fp.printf_1024("%s %s %d %ld %zd\r\n", op, key, flags, timeout_second, size);
    fp.write(data, size);
    fp.write("\r\n", 2);
    if (fp.gets(str) < 1) {
        fp.close();
        close();
        r_msg = "read/write error";
        return -1;
    }
    const char *ps = str.c_str();
    if (!strncmp(ps, "STORED", 6)) {
        return 1;
    }

    r_msg = str;
    return -1;
}

long memcache_client::op_incr(const char *op, const char *key, size_t n)
{
    ___check_fd();
    ___check_key();
    std::string str;
    stream fp(r_fd);
    fp.set_timeout(r_timeout);
    fp.printf_1024("%s %s %zd\r\n", op, key, n);
    if (fp.gets(str) < 1) {
        fp.close();
        close();
        r_msg = "read/write error";
        return -1;
    }
    const char *ps = str.c_str();
    if (isdigit(ps[0])) {
        return atoll(ps);
    }
    r_msg = str;
    return -1;
}

}

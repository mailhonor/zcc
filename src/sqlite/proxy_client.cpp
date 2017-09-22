/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-03-15
 * ================================
 */

#include "zcc.h"

namespace zcc
{

#define proxy_set_errmsg(fmt, args...)      cache->printf_1024(fmt, ##args)

bool var_sqlite3_proxy_set_errmsg = false;

sqlite3_proxy::sqlite3_proxy(const char *_destination, string *_cache)
{
    destination = strdup(_destination);
    fp = 0;
    if (_cache) {
        cache = cache;
        cache_flag = false;
    } else {
        cache = new string();
        cache_flag = true;
    }
}

sqlite3_proxy::~sqlite3_proxy()
{
    if (fp) {
        int fd = fp->get_fd();
        delete fp;
        close(fd);
    }
    if (cache_flag) {
        delete cache;
    }
}

bool sqlite3_proxy::connect()
{
    int retry;
    int fd;
    if (fp) {
        return true;
    }
    for (retry = 0; retry < 2; retry++) {
        if (fp == 0) {
            fd = zcc::connect(destination);
            if(fd < 0) {
                continue;
            }
            fp = new iostream(fd);
            break;
        }
    }
    if (fp == 0) {
        proxy_set_errmsg("connect %s", destination);
        return false;
    }
    return true;
}

bool sqlite3_proxy::disconnect(bool tf)
{
    if (fp) {
        int fd = fp->get_fd();
        delete fp;
        fp = 0;
        close(fd);
    }
    return tf;
}

bool sqlite3_proxy::log(const char *sql, size_t size, long timeout)
{
    char buf[32];
    int slen;
    string &cbuf = *cache;

    if (empty(sql) || (size<1)) {
        return true;
    }
    if (!connect()) {
        return false;
    }

    if (timeout < 1) {
        timeout = var_long_max;
    }
    fp->set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'L';

    fp->write(buf, slen+1);
    fp->write(sql, size);
    fp->flush();
    if (fp->is_exception()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    return true;
}

bool sqlite3_proxy::exec(const char *sql, size_t size, long timeout)
{
    char buf[32];
    int len, slen;
    string &cbuf = *cache;

    if (empty(sql) || (size<1)) {
        return true;
    }
    if (!connect()) {
        return false;
    }

    if (timeout < 1) {
        timeout = var_long_max;
    }
    fp->set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'E';

    fp->write(buf, slen+1);
    fp->write(sql, size);
    fp->flush();
    if (fp->is_exception()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    len = fp->size_data_get_size();
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    cbuf.clear();
    int r = fp->get();
    if ((len > 1) && (fp->readn(cbuf, len-1) < len - 1)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    if (r == 'O') {
        return true;
    }
    return false;
}

bool sqlite3_proxy::query(const char *sql, size_t size, long timeout)
{
    char buf[32];
    int len, slen;
    string &cbuf = *cache;

    if (empty(sql) || (size<1)) {
        return true;
    }

    if (!connect() < 0) {
        return false;
    }
    if (timeout < 1) {
        timeout = var_long_max;
    }
    fp->set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'Q';
    fp->write(buf, slen+1);
    fp->write(sql, size);
    fp->flush();
    if (fp->is_error()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    len = fp->size_data_get_size();
    if (len < 1) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }

    cbuf.clear();
    int r = fp->get();
    if ((len > 1) && (fp->readn(cbuf, len-1) < len - 1)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    if (r == 'E') {
        return false;
    }

    ncolumns = atoi(cbuf.c_str());

    return true;
}

int sqlite3_proxy::get_row(size_data_t **row)
{
    size_data_t *sdvector;
    char buf[32];
    int len;
    string &cbuf = *cache;

    if (!fp) {
        proxy_set_errmsg("read from %s", destination);
        return -1;
    }
    len = fp->size_data_get_size();
    if (len < 1) {
        proxy_set_errmsg("read from %s", destination);
        disconnect();
        return -1;
    }
    if ((fp->readn(buf, 1) < 0) || ((len>1) && (fp->readn(cbuf, len-1) < len - 1))) {
        proxy_set_errmsg("read from %s", destination);
        disconnect();
        return -1;
    }
    if (buf[0] == 'E') {
        return -1;
    }

    if (buf[0] == 'O') {
        *row = 0;
        return 0;
    }

    if (buf[0] != '*') {
        proxy_set_errmsg("read star from %s", destination);
        disconnect();
        return -1;
    }

    cbuf.reserve(sizeof(size_data_t) * ncolumns+10);
    *row = sdvector = (size_data_t *)(cbuf.c_str() + cbuf.size() + 8);
    if ((int)zcc::size_data_unescape_all(cbuf.c_str(), cbuf.size(), sdvector, ncolumns) < ncolumns) {
        proxy_set_errmsg("unescape row");
        disconnect();
        return -1;
    }
    for (int i = 0; i < ncolumns; i++) {
        sdvector[i].data[sdvector[i].size] = 0;
    }

    return 1;
}

}

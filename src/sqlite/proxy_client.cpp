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

#define proxy_set_errmsg(fmt, args...)      sprintf_1024(errmsg, fmt, ##args)

bool var_sqlite3_proxy_set_errmsg = false;

sqlite3_proxy::sqlite3_proxy(const char *_destination)
{
    destination = strdup(_destination);
    rows = 0;
    query_over = true;
}

sqlite3_proxy::~sqlite3_proxy()
{
    free(destination);
    if (rows) {
        delete[] rows;
    }
}

bool sqlite3_proxy::connect()
{
    int retry;
    int fd = -1;
    if (fp.get_fd() > -1) {
        return true;
    }
    query_over = true;
    for (retry = 0; retry < 2; retry++) {
        fd = zcc::connect(destination);
        if(fd < 0) {
            continue;
        }
        break;
    }
    if (fd == -1) {
        proxy_set_errmsg("connect %s", destination);
        return false;
    }
    fp.open(fd);
    fp.set_auto_close_fd();
    return true;
}

bool sqlite3_proxy::disconnect(bool tf)
{
    fp.close();
    query_over = true;
    return tf;
}

bool sqlite3_proxy::log(const char *sql, size_t size, long timeout)
{
    errmsg.clear();
    char buf[32];
    int slen;

    if (!query_over) {
        clear_query();
    }

    if (empty(sql) || (size<1)) {
        return true;
    }
    if (!connect()) {
        return false;
    }

    fp.set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'L';

    fp.write(buf, slen+1);
    fp.write(sql, size);
    fp.flush();
    if (fp.is_exception()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    return true;
}

bool sqlite3_proxy::exec(const char *sql, size_t size, long timeout)
{
    errmsg.clear();
    char buf[32];
    int len, slen;

    if (!query_over) {
        clear_query();
    }

    if (empty(sql) || (size<1)) {
        return true;
    }
    if (!connect()) {
        return false;
    }

    fp.set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'E';

    fp.write(buf, slen+1);
    fp.write(sql, size);
    fp.flush();
    if (fp.is_exception()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    len = fp.size_data_get_size();
    if ((len < 1) || (len > 10000)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    int r = fp.get();
    std::string cbuf;
    if ((len > 1) && (fp.readn(cbuf, len-1) < len - 1)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    if (r == 'O') {
        return true;
    }

    proxy_set_errmsg("server: %s", cbuf.c_str());
    return false;
}

bool sqlite3_proxy::query(const char *sql, size_t size, long timeout)
{
    char buf[32];
    int len, slen;

    if (!query_over) {
        clear_query();
    }

    if (empty(sql) || (size<1)) {
        return true;
    }

    if (!connect() < 0) {
        return false;
    }

    query_over =false;
    fp.set_timeout(timeout);

    slen = size_data_put_size(size + 1, buf);
    buf[slen] = 'Q';
    fp.write(buf, slen+1);
    fp.write(sql, size);
    fp.flush();
    if (fp.is_error()) {
        proxy_set_errmsg("write to %s", destination);
        return disconnect();
    }

    len = fp.size_data_get_size();
    if (len < 1) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }

    int r = fp.get();
    std::string cbuf;
    if ((len > 1) && (fp.readn(cbuf, len-1) < len - 1)) {
        proxy_set_errmsg("read from %s", destination);
        return disconnect();
    }
    if (r == 'E') {
        query_over = true;
        proxy_set_errmsg("server: %s", cbuf.c_str());
        return false;
    }

    ncolumns = atoi(cbuf.c_str());

    return true;
}

int sqlite3_proxy::get_row(std::string **row)
{
    char buf[32];
    int len;

    if (fp.get_fd() == -1) {
        proxy_set_errmsg("read from %s", destination);
        query_over = true;
        return -1;
    }
    len = fp.size_data_get_size();
    if (len < 1) {
        proxy_set_errmsg("read from %s", destination);
        disconnect();
        return -1;
    }
    std::string cbuf;
    if ((fp.readn(buf, 1) < 0) || ((len>1) && (fp.readn(cbuf, len-1) < len - 1))) {
        proxy_set_errmsg("read from %s", destination);
        disconnect();
        return -1;
    }
    if (buf[0] == 'E') {
        proxy_set_errmsg("server: %s", cbuf.c_str());
        query_over = true;
        return -1;
    }

    if (buf[0] == 'O') {
        *row = 0;
        query_over = true;
        return 0;
    }

    if (buf[0] != '*') {
        proxy_set_errmsg("read star from %s", destination);
        disconnect();
        return -1;
    }

    if (rows) {
        delete[] rows;
    }

    rows = new std::string[ncolumns];
    *row = rows;
    size_data_parser sdparser(cbuf.c_str(), cbuf.size());
    for (int i = 0; i < ncolumns; i++) {
        const char *sdata;
        size_t slen;
        if (sdparser.shift(&sdata, &slen) < 1) {
            proxy_set_errmsg("unescape row");
            disconnect();
            return -1;
        }
        if (slen) {
            rows[i].append(sdata, slen);
        }
    }

    return 1;
}

bool sqlite3_proxy::clear_query()
{
    if (query_over) {
        return true;
    }
    if (!connect()) {
        return false;
    }
    if (query_over) {
        return true;
    }
    query_over = true;
    std::string *row;
    while(1) {
        int ret = get_row(&row);
        if (ret == 0) {
            break;
        }
        if (ret < 0) {
            return false;
        }
    }
    return 0;
}

}

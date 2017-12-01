/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-10-15
 * ================================
 */


/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-13
 * ================================
 */

#include "zcc.h"
#include <streambuf>
#include <iostream>

namespace zcc
{

class iostream;
class iostreambuf: public std::streambuf {
public:
    iostreambuf();
    ~iostreambuf();
    void fdopen(int fd, bool auto_close_fd);
    void sslopen(SSL *ssl, bool auto_free_ssl);
    void close();
    void set_timeout(long timeout);
    long get_timeout();
    int get_fd();
    SSL *get_SSL();
    bool tls_connect(SSL_CTX *ctx);
    bool tls_accept(SSL_CTX *ctx);
    inline bool opened() { return m_opened; }
protected:
    int_type underflow();
    std::streamsize xsgetn (char* s, std::streamsize n);
    int_type overflow(int_type c = traits_type::eof());
    std::streamsize xsputn(const char *s, std::streamsize n);
    int_type sync();
private:
    void set_cache_buf();
    ssize_t write(const void *data, size_t size, bool strict = false);
    long m_timeout;
    union {
        int fd;
        SSL *ssl;
    } m_fd;
    char *m_cache_buf;
    bool m_exception;
    bool m_opened;
    bool m_ssl_mode;
    bool m_opened_ssl_mode;
    bool m_auto_close;
};

class iostream:public std::iostream
{
public:
    inline iostream():std::iostream(&mbuf) {}
    inline ~iostream() {}
    inline bool opened() { return mbuf.opened(); }
    inline iostream &fdopen(int fd, bool auto_close_fd = false) { mbuf.fdopen(fd, auto_close_fd); return *this; }
    inline iostream &sslopen(SSL *ssl, bool auto_free_ssl = false){ mbuf.sslopen(ssl, auto_free_ssl); return *this;}
    inline iostream &close() { mbuf.close(); return *this; }
    inline iostream &set_timeout(long timeout) { mbuf.set_timeout(timeout); return *this; }
    inline long get_timeout() { return mbuf.get_timeout(); }
    inline int get_fd() {return mbuf.get_fd();}
    inline SSL *get_SSL() {return mbuf.get_SSL();}
    inline bool tls_connect(SSL_CTX *ctx) { return mbuf.tls_connect(ctx); }
    inline bool tls_accept(SSL_CTX *ctx) { return mbuf.tls_accept(ctx); }
private:
    iostreambuf mbuf;
};


static const ssize_t var_cache_buf_size = 4096;

iostreambuf::iostreambuf()
{
    m_cache_buf = 0;
    m_opened = m_opened_ssl_mode = m_ssl_mode = m_exception = false;
    set_cache_buf();
}

iostreambuf::~iostreambuf()
{
    close();
    if (m_cache_buf) {
        delete [] m_cache_buf;
    }
}

void iostreambuf::fdopen(int fd, bool auto_close_fd)
{
    if (m_opened) {
        close();
    }
    m_auto_close = auto_close_fd;
    m_opened = true;
    m_ssl_mode = false;
    m_fd.fd = fd;
    m_opened_ssl_mode = false;
    set_timeout(-1);
    m_exception = false;
}

void iostreambuf::sslopen(SSL *ssl, bool auto_free_ssl)
{
    if (m_opened) {
        close();
    }
    m_auto_close = auto_free_ssl;
    m_opened = true;
    m_ssl_mode = true;
    m_fd.ssl = ssl;
    m_opened_ssl_mode = ssl;
    set_timeout(-1);
    m_exception = false;
}

void iostreambuf::close()
{
    if (!m_opened) {
        return;
    }
    sync();
    m_opened = false;
    if (!m_opened_ssl_mode) {
        if (m_ssl_mode) {
            int fd = openssl_SSL_get_fd(m_fd.ssl);
            openssl_SSL_free(m_fd.ssl);
            m_fd.fd = fd;
            m_ssl_mode = false;
        }
    }
    if (m_auto_close) {
        if (m_opened_ssl_mode) {
            int fd = openssl_SSL_get_fd(m_fd.ssl);
            openssl_SSL_free(m_fd.ssl);
            ::close(fd);
        } else {
            ::close(m_fd.fd);
        }
    }
}

int iostreambuf::get_fd()
{
    if (m_ssl_mode) {
        return openssl_SSL_get_fd(m_fd.ssl);
    }
    return m_fd.fd;
}

SSL *iostreambuf::get_SSL()
{
    if (m_ssl_mode) {
        return m_fd.ssl;
    }
    return 0;
}

void iostreambuf::set_timeout(long timeout)
{
    if (timeout < 1) {
        timeout = 1000L * 3600 * 24 * 365 * 10;
    }
    m_timeout = timeout_set(timeout);
}

long iostreambuf::get_timeout()
{
    return m_timeout;
}

bool iostreambuf::tls_connect(SSL_CTX *ctx)
{
    if (m_ssl_mode) {
        return false;
    }
    long time_left = timeout_left(m_timeout);
    SSL * _ssl = openssl_create_SSL(ctx, m_fd.fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_connect(_ssl, time_left)) {
        openssl_SSL_free(_ssl);
        return false;
    }
    m_fd.ssl = _ssl;
    m_ssl_mode = true;
    return true;
}

bool iostreambuf::tls_accept(SSL_CTX *ctx)
{
    if (m_ssl_mode) {
        return false;
    }
    long time_left = timeout_left(m_timeout);
    SSL * _ssl = openssl_create_SSL(ctx, m_fd.fd);
    if (!_ssl) {
        return false;
    }
    if (!openssl_timed_accept(_ssl, time_left)) {
        openssl_SSL_free(_ssl);
        return false;
    }

    m_fd.ssl = _ssl;
    m_ssl_mode = true;
    return true;
}


#if 0
std::streambuf *iostreambuf::setbuf(char *s, std::streamsize n)
{
    return this;
}
#endif

void iostreambuf::set_cache_buf()
{
    if (m_cache_buf) {
        return;
    }
    m_cache_buf = new char [ var_cache_buf_size * 2];
    setg(m_cache_buf, m_cache_buf, m_cache_buf);
    setp(m_cache_buf + var_cache_buf_size, m_cache_buf + var_cache_buf_size + var_cache_buf_size);
}

ssize_t iostreambuf::write(const void *data, size_t size, bool strict)
{
    size_t left_len =  size;
    ssize_t wrote_len;
    long left_timeout;
    do {
        left_timeout = timeout_left(m_timeout);
        if (m_ssl_mode) {
            wrote_len = openssl_timed_write(m_fd.ssl, (char *)data + (size - left_len), left_len, left_timeout);
        } else {
            wrote_len = timed_write(m_fd.fd, (char *)data + (size - left_len), left_len, left_timeout);
        }
        if (wrote_len > 0) {
            left_len -= wrote_len;
        } else {
            m_exception = true;
            break;
        }
    } while (strict && (left_len > 0));
    return size - left_len;
}

iostreambuf::int_type iostreambuf::underflow()
{
    int_type result(traits_type::eof());
    if (!m_opened){
        return result;
    }
    ssize_t ret;
    char *eb = eback();
    if (m_ssl_mode) {
        ret = openssl_timed_read(m_fd.ssl, eb, var_cache_buf_size, timeout_left(m_timeout));
    } else {
        ret = timed_read(m_fd.fd, eb, var_cache_buf_size, timeout_left(m_timeout));
    }
    if (ret > 0) {
        setg(eb, eb, eb + ret);
        result = *gptr();
    }
    return result;
}

std::streamsize iostreambuf::xsgetn (char* s, std::streamsize n)
{
    std::streamsize result(0), avail;
    if (!m_opened){
        return result;
    }
    avail = egptr() - gptr();
    if (avail >= n) {
        memcpy(s, gptr(), n);
        gbump(n);
        result = n;
    } else {
        memcpy(s, gptr(), avail);
        gbump(avail);
        ssize_t ret;
        if (m_ssl_mode) {
            ret = openssl_timed_read(m_fd.ssl, s + avail, n - avail, timeout_left(m_timeout));
        } else {
            ret = timed_read(m_fd.fd, s + avail, n - avail, timeout_left(m_timeout));
        }
        if (ret > 0) {
            result = avail + ret;
        } else {
            result = avail;
        }
    }
    return result;
}

iostream::int_type iostreambuf::overflow(iostream::int_type c)
{
    int_type result(traits_type::eof());
    int_type result_ok(traits_type::not_eof(c));
    if ((!m_opened) || (m_exception) ){
        return result;
    }
    std::streamsize put, pending;
    if (c == traits_type::eof()) {
        pending = pptr() - pbase();
        if (pending > 0) {
            put = write(pbase(), pending, true);
            if (put == pending) {
                setp(m_cache_buf + var_cache_buf_size, m_cache_buf + var_cache_buf_size + var_cache_buf_size);
                result =  result_ok;
            }
        } else {
            result = result_ok;
        }
#if 0
    } else if (pbase() == epptr()) {
        ssize_t ret;
        char wbuf[2];
        wbuf[0] = c;
        ret = write(wbuf, 1, false);
        if (ret == 1) {
            result = result_ok;
        }
#endif
    } else if (pptr() < epptr()) {
        *pptr() = c;
        pbump(1);
        result =  result_ok;
    } else {
        pending = pptr() - pbase();
        put = write(pbase(), pending, true);
        if (put == pending) {
            setp(m_cache_buf + var_cache_buf_size, m_cache_buf + var_cache_buf_size + var_cache_buf_size);
            *pptr() = c;
            pbump(1);
            result =  result_ok;
        }
    }
    return result;
}

std::streamsize iostreambuf::xsputn(const char *s, std::streamsize n)
{
    int_type eof((traits_type::eof()));
    std::streamsize result(0), pending, put;

    if ((!m_opened) || (m_exception) ){
        return result;
    }

    pending = pptr() - pbase();
    if ((pending + n) <= (epptr() - pbase())) {
        memcpy(pptr(), s, n);
        pbump(n);
        result = n;
    } else {
        if (overflow(eof) != 0) {
            return result;
        }
        if (n < (epptr() - pbase() /* == var_cache_buf_size */)) {
            memcpy(pptr(), s, n);
            pbump(n);
            result = n;
        } else {
            put = write(s, n, true);
            if (put == n) {
                result = n;
            }
        }
    }
    return result;
}

iostream::int_type iostreambuf::sync()
{
    int_type eof((traits_type::eof()));
    int result(0);
    if (pptr() != 0) {
        result = ((overflow(eof) != eof) ? 0 : -1);
    }
    return result;
}

}

static char *server = 0;
static bool ssl_mode = false;
static bool tls_mode = false;
static void ___usage(char *arg = 0)
{
    printf("%s -server smtp_server:port [-ssl] [-tls]\n", zcc::var_progname);
    exit(1);
}

static void  write_line_read_line(std::iostream &fp, std::string &tmpline, const char *query)
{
    fp << query << "\r" << std::endl;
    printf("C: %s\r\n", query);
    std::getline(fp, tmpline);
    printf("S: %s\r\n", tmpline.c_str());
}

int main(int argc, char **argv)
{
    zcc_main_parameter_begin() {
        if (!strcmp(optname, "-ssl")) {
            ssl_mode = true;
            opti += 1;
            continue;
        }
        if (!strcmp(optname, "-tls")) {
            tls_mode = true;
            opti += 1;
            continue;
        }
        if (!optval) {
            ___usage();
        }
        if (!strcmp(optname, "-server")) {
            server = optval;
            opti += 2;
            continue;
        }
    } zcc_main_parameter_end;

    if (zcc::empty(server)) {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    if (tls_mode || ssl_mode) {
        zcc::openssl_init();
        ssl_ctx = zcc::openssl_create_SSL_CTX_client();
    }

    int fd;
    fd = zcc::connect(server);
    if (fd < 0) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }
    zcc::nonblocking(fd);

    std::string str;
    zcc::iostream fp;
    if (ssl_mode) {
        SSL *ssl = zcc::openssl_create_SSL(ssl_ctx, fd);
        if (!zcc::openssl_timed_connect(ssl, 10 * 1000)) {
            printf("ERR ssl initialization error (%m)\n");
            exit(1);
        }
        fp.sslopen(ssl, true);
    } else {
        fp.fdopen(fd, true);
    }

    std::getline(fp, str);
    printf("S: %s\r\n", str.c_str());

    if (tls_mode && !ssl_mode) {
        write_line_read_line(fp, str, "STARTTLS");
        fp.tls_connect(ssl_ctx);
    }
    write_line_read_line(fp, str, "helo 163.com");
    write_line_read_line(fp, str, "mail from: <xxx@163.com>");
    write_line_read_line(fp, str, "quit");

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zcc.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace zcc
{

bool var_httpd_debug = false;
static const int ___header_line_max_size = 10240;
class httpd_inner
{
public:
    inline httpd_inner() {}
    inline ~httpd_inner() {} 
    void loop_clear();
    void request_header_do(bool first);
    void request_data_do();
    void request_data_do_true();
    int fd;
    SSL *ssl;
    stream *http_fp;
    /* control */
    bool stop;
    /* request */
    char *method;
    char *uri;
    char *version;
    char *path;
    int request_content_length;
    dict request_query;
    dict request_post;
    dict request_headers;
    dict request_cookies;
    /* */
    bool request_keep_alive;
    bool response_initialization;
    bool response_content_type;
    bool get_post_data_hander_myself;
    bool exception;
};

void httpd_inner::loop_clear()
{
    stop = false;
#define ___FR(m) free(m); m=blank_buffer;
    ___FR(method);
    ___FR(path);
#undef ___FR
    request_content_length = -1;
    request_query.clear();
    request_post.clear();
    request_headers.clear();
    request_cookies.clear();
    request_keep_alive = false;
    response_initialization = false;
    response_content_type = false;
}

void httpd_inner::request_header_do(bool first)
{
    httpd *hddata = (httpd *)zcc_container_of(this, httpd, ___data);
    autobuffer readline_auto;
    char *linebuf = (char *)malloc(___header_line_max_size + 1);
    readline_auto.data = linebuf;
    char *p, *ps = linebuf;
    int llen;
    bool need_body = false;

    /* read first header line */
    if (first) {
        http_fp->set_timeout(hddata->request_header_timeout());
    } else {
        http_fp->set_timeout(hddata->keep_alive_timeout());
    }
    int first_ch = http_fp->get();
    if (first_ch == -1) {
        exception = true; return;
    }
    if (!first) {
        http_fp->set_timeout(hddata->keep_alive_timeout());
    }
    linebuf[0] = first_ch;
    if ((llen = http_fp->gets(linebuf + 1, ___header_line_max_size - 1)) < 10) {
        exception = true; return;
    }
    llen ++;
    if (linebuf[llen-1] != '\n') {
        exception = true; return;
    }
    llen--;
    if (linebuf[llen-1] == '\r') {
        llen --;
    }
    linebuf[llen] = 0;
    method = memdupnull(linebuf, llen);
    ps = method;
    p = strchr(ps, ' ');
    if (!p) {
        exception = true; return;
    }
    *p = 0;
    toupper(ps);
    if (zcc_str_eq(ps, "GET")) {
    } else if (zcc_str_eq(ps, "POST")) {
        need_body = true;
    } else if (zcc_str_eq(ps, "HEAD")) {
    } else if (zcc_str_eq(ps, "PUT")) {
        need_body = true;
    } else if (zcc_str_eq(ps, "TRACE")) {
    } else if (zcc_str_eq(ps, "OPTIONS")) {
    } else {
        exception = true; return;
    }
    llen -= (p - ps) + 1;
    ps = uri = p + 1;

    if (llen < 10) {
        exception = true; return;
    }
    p += llen;
    for (;p > ps; p--) {
        if (*p ==' ') {
            break;
        }
    }
    if (ps == p) {
        exception = true; return;
    }
    *p = 0;
    version = p + 1;
    toupper(version);

    ps = uri;
    p = strchr(ps, '?');
    if (!p) {
        path = strdup(ps);
    } else {
        path = memdupnull(ps, p - ps);
        ps = p + 1;
        p = strchr(ps, '#');
        if (p) {
            *p = 0;
        }
        request_query.parse_url_query(ps);
        if (p) {
            *p = '#';
        }
    }

    /* read other header lines */
    while(1) {
        if ((llen = http_fp->gets(linebuf, ___header_line_max_size)) < 1) {
            exception = true; return;
        }
        if (linebuf[llen-1] != '\n') {
            exception = true; return;
        }
        llen--;
        if ((llen>0) && (linebuf[llen-1] == '\r')) {
            llen --;
        }
        linebuf[llen] = 0;
        if (llen == 0) {
            break;
        }
        ps = linebuf;
        p = strchr(ps, ':');
        if (!p) {
            continue;
        }
        *p = 0;
        tolower(linebuf);
        ps = p + 1;
        while (*ps == ' ') {
            ps ++;
        }
        request_headers.update(linebuf, ps);
        if (zcc_str_eq(linebuf, "content-length")) {
            request_content_length = atoi(ps);
        } else if (zcc_str_eq(linebuf, "cookie")) {
            http_cookie_parse_request(request_cookies, ps);
        } else if (zcc_str_eq(linebuf, "connection")) {
            if (strcasestr(p, "keep-alive")) {
                request_keep_alive = true;
            }
        }
    }

    /* read body */
    if (!need_body) {
        return;
    }

    if (request_content_length < 1) {
        return;
    }
}

void httpd_inner::request_data_do()
{
    httpd *hddata = (httpd *)zcc_container_of(this, httpd, ___data);
    get_post_data_hander_myself = false;
    hddata->get_post_data_hander();
    if (!get_post_data_hander_myself) {
        return;
    }
    if (request_content_length < 1) {
        return;
    }
    if (hddata->max_length_for_post() > request_content_length) {
        exception = true;
        return;
    }
    request_data_do_true();
}

void httpd_inner::request_data_do_true()
{
    httpd *hddata = (httpd *)zcc_container_of(this, httpd, ___data);
    char *p = hddata->request_header("content-type");
    int ctype = 0;
    if (!strcasecmp(p, "application/x-www-form-urlencoded")) {
        ctype = 'u';
    } else if (!strcasecmp(p, "multipart/form-data")) {
        ctype = 'f';
    }
    if (ctype == 0) {
        return;
    }

    if (ctype == 'u') {
        std::string mbuf;
        mbuf.reserve(request_content_length);
        if (http_fp->readn(mbuf, request_content_length) < request_content_length) {
            exception = true;
            return;
        }
        request_post.parse_url_query(mbuf.c_str());
        return;
    }

    /* FIXME */
    autobuffer readline_auto;
    char *linebuf = (char *)malloc(___header_line_max_size + 1);
    readline_auto.data = linebuf;

}

httpd::httpd()
{
    memset(___data, 0, sizeof(___data));
    new (___data) httpd_inner();
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->loop_clear();
    httpddata->fd = -1;
    httpddata->exception = false;
}

httpd::~httpd()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->loop_clear();
    if (httpddata->http_fp) {
        delete httpddata->http_fp;
    }
    httpddata->~httpd_inner();
}

void httpd::bind(int fd)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->http_fp = new iostream(fd);
}

void httpd::bind(SSL *ssl)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->http_fp = new sslstream(ssl);
}

void httpd::get_post_data_hander()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->get_post_data_hander_myself = true;
}

void httpd::get_post_data_hander_default()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->request_data_do_true();
}

void httpd::set_exception()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->exception = true;
}

bool httpd::run()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    bool first = true;
    while(1) {
        httpddata->loop_clear();
        httpddata->request_header_do(first);
        first = false;
        if (httpddata->exception || http_fp->is_exception()) {
            return false;
        }
        httpddata->http_fp->set_timeout(-1);
        httpddata->request_data_do();
        if (httpddata->exception || http_fp->is_exception()) {
            return false;
        }
        handler();
        if (httpddata->exception || http_fp->is_exception()) {
            return false;
        }
        http_fp->flush();
        if (httpddata->exception || http_fp->is_exception()) {
            return false;
        }
        if (httpddata->stop) {
            return true;
        }
        if (!httpddata->request_keep_alive) {
            return true;
        }
    }
    return true;
}

void httpd::handler()
{
    response_404();
}

void httpd::stop()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    httpddata->stop = true;
}

long httpd::keep_alive_timeout()
{
    return 100 * 1000;
}

long httpd::request_header_timeout()
{
    return 100 * 1000;
}

long httpd::max_length_for_post()
{
    return 1024 * 1024 * 10;
}

char *httpd::tmp_path_for_post()
{
    return const_cast<char *>("/tmp/");
}

char *httpd::request_method()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->method;
}

char *httpd::request_uri()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->uri;
}

char *httpd::request_version()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->version;
}

static char *request_something_fn(dict &dt, const char *name, const char *def_val)
{
    char nbuf[1024 + 1];
    int len = strlen(name);
    if (len > 1024) {
        return const_cast<char *>(def_val);
    }
    if (len == 0) {
        return const_cast<char *>(def_val);
    }
    memcpy(nbuf, name, len);
    nbuf[len] = 0;
    tolower(nbuf);
    return dt.get_str(nbuf, def_val);
}

char *httpd::request_header(const char *name, const char *def_val)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return request_something_fn(httpddata->request_headers, name, def_val);
}

dict &httpd::request_header()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->request_headers;
}

char *httpd::request_query_variate(const char *name, const char *def_val)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return request_something_fn(httpddata->request_query, name, def_val);
}

dict &httpd::request_query_variate()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->request_query;
}

char *httpd::request_post_variate(const char *name, const char *def_val)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return request_something_fn(httpddata->request_post, name, def_val);
}

dict &httpd::request_post_variate()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->request_post;
}

char *httpd::request_cookie(const char *name, const char *def_val)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return request_something_fn(httpddata->request_cookies, name, def_val);
}

dict &httpd::request_cookie()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    return httpddata->request_cookies;
}

void httpd::response_500()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    char output[] = "HTTP/1.0 500 Error\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "500 Internal Server Error ";
    http_fp->write(output, sizeof(output) - 1);
    http_fp->flush();
}

void httpd::response_file_by_absolute_path(const char *filename, const char *content_type)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    if (empty(content_type)) {
        content_type = mime_type_from_filename(filename, var_mime_type_application_cotec_stream);
    }
    int fd;
    while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
        continue;
    }
    if (fd == -1) {
        response_404();
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        response_404();
        return;
    }
    char *old_etag = request_header("if_none_match", "");
    autobuffer rwbuffer;
    rwbuffer.data = (char *)malloc(4096 + 1);
    char *new_etag = rwbuffer.data;
    sprintf(new_etag, "%lx_%lx", st.st_size, st.st_mtime);
    if (!strcmp(old_etag, new_etag)) {
        response_304(new_etag);
        close(fd);
        return;
    }

    response_header_content_type(content_type);
    response_header_content_length(st.st_size);
    response_header("Etag", new_etag);
    response_header("Last-Modified", st.st_mtime);
    if (httpddata->request_keep_alive) {
        response_header("Connection", "keep-alive");
    }
    response_header_over();

    char *rwline = rwbuffer.data;
    long rlen_sum = 0;
    while(rlen_sum < st.st_size) {
        int rlen = st.st_size - rlen_sum;
        if (rlen > 4096) {
            rlen = 4096;
        }
        ssize_t syscall_read(int fildes, void *buf, size_t nbyte);
        rlen = syscall_read(fd, rwline, rlen);
        if (rlen > 0) {
            rlen_sum += rlen;
            http_fp->write(rwline, rlen);
            if (http_fp->is_exception()) {
                break;
            }
            continue;
        }
        if (rlen == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
    close(fd);
    if (rlen_sum != st.st_size) {
        stop();
    } else {
        http_fp->flush();
    }
}

void httpd::response_404()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    char output[] = "HTTP/1.0 404 Not Found\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "404 Not Found ";
    http_fp->write(output, sizeof(output) - 1);
    http_fp->flush();
}

void httpd::response_304(const char *etag)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    std::string output = "Etag: ";
    output += etag;
    output += "\r\nHTTP/1.1 304 Not Modified\r\n";
    http_fp->write(output.c_str(), output.size());
    http_fp->flush();
}

void httpd::response_header_initialization(const char *initialization)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    if (initialization == 0) {
        initialization = "HTTP/1.1 200 ZCC";
    }
    httpddata->http_fp->puts(initialization);
    httpddata->http_fp->puts("\r\n");
    httpddata->response_initialization = true;
}

void httpd::response_header(const char *name, const char *value)
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    stream *http_fp = httpddata->http_fp;
    if (!httpddata->response_initialization) {
        response_header_initialization();
    }
    http_fp->puts(name);
    http_fp->puts(": ");
    http_fp->puts(value);
    http_fp->puts("\r\n");
    if (!httpddata->response_content_type) {
        if (!strcasecmp(name, "Content-Type")) {
            httpddata->response_content_type = true;
        }
    }
}

void httpd::response_header(const char *name, long d)
{
    char buf[32];
    build_rfc1123_date_string(d, buf);
    response_header(name, buf);
}

void httpd::response_header_content_type(const char *value, const char *charset)
{
    std::string val;
    val.append(value);
    if (!empty(charset)) {
        val.append("; chrset=");
        val.append(charset);
    }
    response_header("Content-Type", val.c_str());
}

void httpd::response_header_content_length(long length)
{
    char val[32];
    if (length > -1) {
        sprintf(val, "%ld", length);
        response_header("Content-Length", val);
    }
}

void httpd::response_header_set_cookie(const char *name, const char *value, long expires, const char *path, const char *domain, bool secure, bool httponly)
{
    std::string result;
    http_cookie_build(result, name, value, expires, path, domain, secure, httponly);
    response_header("Set-Cookie", result.c_str());
}

void httpd::response_header_unset_cookie(const char *name)
{
    return response_header_set_cookie(name, 0);
}

void httpd::response_header_over()
{
    httpd_inner *httpddata = (httpd_inner *)___data;
    if (!httpddata->response_content_type) {
        response_header("Content-Type", "text/html");
    }
    httpddata->http_fp->puts("\r\n");
}

}

#ifdef __ZCC_SIZEOF_PROBE__
int main()
{ 
    _ZCC_SIZEOF_DEBUG(zcc::httpd, zcc::httpd_inner);
    return 0;
}
#endif

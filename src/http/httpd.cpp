/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

namespace zcc
{

bool var_httpd_debug = false;

httpd::httpd()
{
    memset(___data, 0, sizeof(___data));
    new (___data) httpd_engine();
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->loop_clear();
    httpddata->fd = -1;
    httpddata->exception = false;
}

httpd::~httpd()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->loop_clear();
    if (httpddata->http_fp) {
        SSL *s = 0;
        if (httpddata->ssl_auto_release) {
            s = ((stream *)(httpddata->http_fp))->get_SSL();
        }
        delete httpddata->http_fp;
        if (s) {
            openssl_SSL_free(s);
        }
    }
    httpddata->~httpd_engine();
}

void httpd::bind(int fd)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->http_fp = new stream(fd);
}

void httpd::bind(SSL *ssl)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->http_fp = new stream(ssl);
}

bool httpd::bind(int sock, SSL_CTX *sslctx, long timeout)
{
    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    httpd_engine *httpddata = (httpd_engine *)___data;
    SSL *s = openssl_create_SSL(sslctx, sock);
    if (!s) {
        return false;
    }
    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    if (!openssl_timed_accept(s, timeout)) {
        openssl_SSL_free(s);
        return false;
    }
    httpddata->http_fp = new stream(s);
    httpddata->ssl_auto_release = true;
    return true;
}

void httpd::handler_after_request_header()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->get_post_data_myself = true;
}

void httpd::get_post_data_default()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->request_data_do_true();
}

void httpd::set_exception()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    httpddata->exception = true;
}

bool httpd::run()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
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
    httpd_engine *httpddata = (httpd_engine *)___data;
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

char *httpd::gzip_file_suffix()
{
    return 0;
}

char *httpd::request_method()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->method;
}

char *httpd::request_uri()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->uri;
}

char *httpd::request_path()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->path;
}

char *httpd::request_version()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
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
    httpd_engine *httpddata = (httpd_engine *)___data;
    return request_something_fn(httpddata->request_headers, name, def_val);
}

dict &httpd::request_header()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->request_headers;
}

char *httpd::request_query_variate(const char *name, const char *def_val)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return request_something_fn(httpddata->request_query, name, def_val);
}

dict &httpd::request_query_variate()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->request_query;
}

char *httpd::request_post_variate(const char *name, const char *def_val)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return request_something_fn(httpddata->request_post, name, def_val);
}

dict &httpd::request_post_variate()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->request_post;
}

char *httpd::request_cookie(const char *name, const char *def_val)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return request_something_fn(httpddata->request_cookies, name, def_val);
}

dict &httpd::request_cookie()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->request_cookies;
}

std::list<httpd_upload_file *> &httpd::upload_files()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->request_upload_files;
}

void httpd::response_500()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
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

void httpd::response_file(const char *filename, const char *content_type)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    stream *http_fp = httpddata->http_fp;
    if (empty(content_type)) {
        content_type = mime_type_from_filename(filename, var_mime_type_application_cotec_stream);
    }
    int fd;
    struct stat st;
    int times;
    bool is_gzip = false;
    for (times = 0; times < 2; times++) {
        if (times == 0) {
            if (zcc::empty(gzip_file_suffix())) {
                continue;
            }
            if (!strcasestr(request_header("accept-encoding", ""), "gzip")) {
                continue;
            }
            std::string fn(filename);
            fn.push_back('.');
            fn.append(gzip_file_suffix());
            while ((fd = open(fn.c_str(), O_RDONLY)) == -1 && errno == EINTR) {
                continue;
            }
            is_gzip = true;
        } else {
            while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
                continue;
            }
            is_gzip = false;
        }
        if (fd == -1) {
            continue;
        }

        if (fstat(fd, &st) == -1) {
            close(fd);
            continue;
        }
        break;
    }
    if (times == 2) {
        response_404();
        return;
    }
    char *old_etag = request_header("if-none-match", "");
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
    if (is_gzip) {
        response_header("Content-Encoding", "gzip");
    }
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
    httpd_engine *httpddata = (httpd_engine *)___data;
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

void httpd::response_200(const char *data, size_t size)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    stream *http_fp = httpddata->http_fp;
    response_header_content_length((int)size);
    response_header_over();
    if (size > 0) {
        http_fp->write(data, size);
    }
    response_flush();
}

void httpd::response_304(const char *etag)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    stream *http_fp = httpddata->http_fp;
    std::string output = "HTTP/1.1 304 Not Modified\r\n";
    output += "Etag: ";
    output += etag;
    output += "\r\n";
    http_fp->write(output.c_str(), output.size());
    http_fp->flush();
}

void httpd::response_header_initialization(const char *initialization)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    if (initialization == 0) {
        initialization = "HTTP/1.1 200 ZCC";
    }
    httpddata->http_fp->puts(initialization);
    httpddata->http_fp->puts("\r\n");
    httpddata->response_initialization = true;
}

void httpd::response_header(const char *name, const char *value)
{
    httpd_engine *httpddata = (httpd_engine *)___data;
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
    std::string buf;
    build_rfc1123_date_string(d, buf);
    response_header(name, buf.c_str());
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
    httpd_engine *httpddata = (httpd_engine *)___data;
    if (!httpddata->response_content_type) {
        response_header("Content-Type", "text/html");
    }
    httpddata->http_fp->puts("\r\n");
}

bool httpd::response_flush()
{
    httpd_engine *httpddata = (httpd_engine *)___data;
    return httpddata->http_fp->flush();
}

}

#ifdef __ZCC_SIZEOF_PROBE__
int main()
{ 
    _ZCC_SIZEOF_DEBUG(zcc::httpd, zcc::httpd_engine);
    return 0;
}
#endif

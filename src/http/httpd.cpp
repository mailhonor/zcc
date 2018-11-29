#if 0
    if (!strcasestr(dict_get_str(h_engine->request_headers,"accept-encoding", ""), "gzip")) {
        response_500();
        return;
    }
#endif
/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zcc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


namespace zcc
{

bool var_httpd_debug = false;
bool var_httpd_no_cache = false;

static const int var_httpd_header_line_max_size = 10240;

class httpd_engine
{
public:
    inline httpd_engine() {}
    inline ~httpd_engine() {}
    int fd;
    SSL *ssl;
    stream http_fp;
    /* control */
    bool stop;
    /* request */
    char * method;
    char * host;
    int port;
    char * uri;
    char * version;
    char * path;
    int request_content_length;
    std::map<std::string, std::string> request_query;
    std::map<std::string, std::string> request_post;
    std::map<std::string, std::string> request_headers;
    std::map<std::string, std::string> request_cookies;
    std::list<httpd_upload_file> request_upload_files;
    /* */
    bool request_keep_alive;
    bool response_initialization;
    bool response_content_type;
    bool get_post_data_myself;
    bool exception;
    bool tls_mode;
    unsigned char request_gzip:2;
    unsigned char request_deflate:2;
    /* */
    int response_max_age;
    int response_expires;
};

httpd::httpd()
{
    h_engine = new httpd_engine();
    h_engine->method = blank_buffer;
    h_engine->host = blank_buffer;
    h_engine->port = -1;
    h_engine->uri = blank_buffer;
    h_engine->version = blank_buffer;
    h_engine->path = blank_buffer;
    h_engine->fd = -1;
    h_engine->ssl = 0;
    h_engine->request_keep_alive = false;
    h_engine->response_initialization = false;
    h_engine->response_content_type = false;
    h_engine->get_post_data_myself = false;
    h_engine->exception = false;
    h_engine->tls_mode = false;

    h_engine->request_gzip = 0;
    h_engine->request_deflate = 0;

    h_engine->response_max_age = -1;
    h_engine->response_expires = -1;

    loop_clear();
    h_engine->fd = -1;
    h_engine->exception = false;
}

httpd::~httpd()
{
    h_engine->http_fp.close();
    loop_clear();
    delete h_engine;
}

void httpd::bind(int fd)
{
    h_engine->http_fp.open(fd);
}

void httpd::bind(SSL *ssl)
{
    h_engine->http_fp.open(ssl);
}

bool httpd::bind(int sock, SSL_CTX *sslctx, long timeout)
{
    if (timeout < 1) {
        timeout = var_max_timeout;
    }
    h_engine->http_fp.open(sock);
    if (!h_engine->http_fp.tls_accept(sslctx)) {
        return false;
    }
    return true;
}

httpd &httpd::set_auto_close_fd(bool flag)
{
    h_engine->http_fp.set_auto_close_fd(flag);
    return *this;
}

void httpd::handler_after_request_header()
{
    h_engine->get_post_data_myself = true;
}

void httpd::get_post_data_default()
{
    request_data_do_true();
}

void httpd::set_exception()
{
    h_engine->exception = true;
}

bool httpd::run()
{
    bool first = true;
    while(1) {
        loop_clear();
        request_header_do(first);
        first = false;
        if (h_engine->exception || h_engine->http_fp.is_exception()) {
            return false;
        }
        h_engine->http_fp.set_timeout(-1);
        request_data_do();
        if (h_engine->exception || h_engine->http_fp.is_exception()) {
            return false;
        }
        handler();
        if (h_engine->exception || h_engine->http_fp.is_exception()) {
            return false;
        }
        h_engine->http_fp.flush();
        if (h_engine->exception || h_engine->http_fp.is_exception()) {
            return false;
        }
        if (h_engine->stop) {
            return true;
        }
        if (!h_engine->request_keep_alive) {
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
    h_engine->stop = true;
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
    return h_engine->method;
}

char *httpd::request_host()
{
    return h_engine->host;
}

char *httpd::request_uri()
{
    return h_engine->uri;
}

char *httpd::request_path()
{
    return h_engine->path;
}

char *httpd::request_version()
{
    return h_engine->version;
}

bool httpd::request_gzip()
{
    if (h_engine->request_gzip == 0) {
        h_engine->request_gzip = (strcasestr(dict_get_str(h_engine->request_headers,"accept-encoding", ""), "gzip")?1:2);
    }
    return (h_engine->request_gzip==1);
}

bool httpd::request_deflate()
{
    if (h_engine->request_deflate == 0) {
        h_engine->request_deflate = (strcasestr(dict_get_str(h_engine->request_headers,"accept-encoding", ""), "deflate")?1:2);
    }
    return (h_engine->request_deflate==1);
}

long httpd::request_content_length()
{
    return h_engine->request_content_length;
}

const std::map<std::string, std::string> &httpd::request_headers()
{
    return h_engine->request_headers;
}

const std::map<std::string, std::string> &httpd::request_query_variates()
{
    return h_engine->request_query;
}

const std::map<std::string, std::string> &httpd::request_post_variates()
{
    return h_engine->request_post;
}

const std::map<std::string, std::string> &httpd::request_cookies()
{
    return h_engine->request_cookies;
}

const std::list<httpd_upload_file> &httpd::upload_files()
{
    return h_engine->request_upload_files;
}

void httpd::response_500()
{
    char output[] = "HTTP/1.0 500 Error\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "500 Internal Server Error ";
    h_engine->http_fp.write(output, sizeof(output) - 1);
    h_engine->http_fp.flush();
}

static const int response_file_flag_gzip = (1<<0);
static const int response_file_flag_try_gzip = (1<<1);
static const int response_file_flag_regular = (1<<2);

void httpd::response_file_by_flag(const char *filename, const char *content_type, int flag, bool *catch_missing)
{
    if (catch_missing) {
        *catch_missing = false;
    }
    if (empty(content_type)) {
        content_type = mime_type_from_filename(filename, var_mime_type_application_cotec_stream);
    }
    int fd;
    struct stat st;
    bool is_gzip = false;
    if ((flag & response_file_flag_gzip) || (flag & response_file_flag_regular)) {
        while ((fd = open(filename, O_RDONLY)) == -1 && errno == EINTR) {
            continue;
        }
        if (fd == -1) {
            if (catch_missing) {
                *catch_missing = true;
            } else {
                response_404();
            }
            return;
        }

        if (fstat(fd, &st) == -1) {
            close(fd);
            response_500();
            return;
        }
        if (flag & response_file_flag_gzip) {
            is_gzip = true;
        }
    } else if (flag & response_file_flag_try_gzip) {
        int times;
        for (times = 0; times < 2; times++) {
            if (times == 0) {
                if (zcc::empty(gzip_file_suffix())) {
                    continue;
                }
                if (!strcasestr(dict_get_str(h_engine->request_headers,"accept-encoding", ""), "gzip")) {
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
            if (catch_missing) {
                *catch_missing = true;
            } else {
                response_404();
            }
            return;
        }
    } else {
        response_500();
        return;
    }

    autobuffer rwbuffer;
    rwbuffer.data = (char *)malloc(4096 + 1);

    if (h_engine->response_max_age > 0) {
        sprintf(rwbuffer.data, "max-age=%d", h_engine->response_max_age);
        response_header("Cache-Control", rwbuffer.data);
    }
    if (h_engine->response_expires > 0) {
        response_header("Expires", h_engine->response_expires + 1 + time(0));
    }
    char *old_etag = dict_get_str(h_engine->request_headers,"if-none-match");
    char *new_etag = rwbuffer.data;
    sprintf(new_etag, "%lx_%lx", st.st_size, st.st_mtime);
    if (!strcmp(old_etag, new_etag)) {
        if (var_httpd_no_cache == false) {
            response_304(new_etag);
            close(fd);
            return;
        }
    }

    response_header_content_type(content_type);
    response_header_content_length(st.st_size);
    if (var_httpd_no_cache == false) {
        response_header("Etag", new_etag);
    }

    if (is_gzip) {
        response_header("Content-Encoding", "gzip");
    }

    response_header("Last-Modified", st.st_mtime);
    if (h_engine->request_keep_alive) {
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
        rlen = read(fd, rwline, rlen);
        if (rlen > 0) {
            rlen_sum += rlen;
            h_engine->http_fp.write(rwline, rlen);
            if (h_engine->http_fp.is_exception()) {
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
    h_engine->http_fp.flush();
    if (rlen_sum != st.st_size) {
        stop();
    } else {
        h_engine->http_fp.flush();
    }
}

void httpd::response_file_with_gzip(const char *filename, const char *content_type, bool *catch_missing)
{
    response_file_by_flag(filename, content_type, response_file_flag_gzip, catch_missing);
}

void httpd::response_file(const char *filename, const char *content_type, bool *catch_missing)
{
    response_file_by_flag(filename, content_type, response_file_flag_regular, catch_missing);
}

void httpd::response_file_try_gzip(const char *filename, const char *content_type, bool *catch_missing)
{
    response_file_by_flag(filename, content_type, response_file_flag_try_gzip, catch_missing);
}

void httpd::response_file_set_max_age(int left_second)
{
    h_engine->response_max_age = left_second;
}

void httpd::response_file_set_expires(int left_second)
{
    h_engine->response_expires = left_second;
}

void httpd::response_404()
{
    char output[] = "HTTP/1.0 404 Not Found\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "404 Not Found ";
    h_engine->http_fp.write(output, sizeof(output) - 1);
    h_engine->http_fp.flush();
}

void httpd::response_200(const char *data, size_t size)
{
    response_header_content_length((int)size);
    response_header_over();
    if (size > 0) {
        h_engine->http_fp.write(data, size);
    }
    response_flush();
}

void httpd::response_304(const char *etag)
{
    std::string output = "HTTP/1.1 304 Not Modified\r\n";
    output += "Etag: ";
    output += etag;
    output += "\r\n";
    h_engine->http_fp.write(output.c_str(), output.size());
    h_engine->http_fp.flush();
}

void httpd::response_header_initialization(const char *initialization)
{
    if (initialization == 0) {
        initialization = "HTTP/1.1 200 ZCC";
    }
    h_engine->http_fp.puts(initialization);
    h_engine->http_fp.puts("\r\n");
    h_engine->response_initialization = true;
}

void httpd::response_header(const char *name, const char *value)
{
    if (!h_engine->response_initialization) {
        response_header_initialization();
    }
    h_engine->http_fp.puts(name);
    h_engine->http_fp.puts(": ");
    h_engine->http_fp.puts(value);
    h_engine->http_fp.puts("\r\n");
    if (!h_engine->response_content_type) {
        if (!strcasecmp(name, "Content-Type")) {
            h_engine->response_content_type = true;
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
    if (!h_engine->response_content_type) {
        response_header("Content-Type", "text/html");
    }
    h_engine->http_fp.puts("\r\n");
}

bool httpd::response_flush()
{
    return h_engine->http_fp.flush();
}

stream &httpd::get_stream()
{
    return h_engine->http_fp;
}

void httpd::loop_clear()
{

#define ___FR(m) free(m); m=blank_buffer;
    ___FR(h_engine->method);
    ___FR(h_engine->host);
    ___FR(h_engine->path);
#undef ___FR
    h_engine->port = -1;

    h_engine->stop = false;
    h_engine->request_content_length = -1;
    h_engine->request_query.clear();
    h_engine->request_post.clear();
    h_engine->request_headers.clear();
    h_engine->request_cookies.clear();
    h_engine->request_upload_files.clear();
    h_engine->request_keep_alive = false;
    h_engine->response_initialization = false;
    h_engine->response_content_type = false;

    h_engine->response_max_age = -1;
    h_engine->response_expires = -1;

}

void httpd::request_header_do(bool first)
{
    autobuffer readline_auto;
    char *linebuf = (char *)malloc(var_httpd_header_line_max_size + 1);
    readline_auto.data = linebuf;
    char *p, *ps = linebuf;
    int llen;
    bool need_body = false;

    /* read first header line */
    if (first) {
        h_engine->http_fp.set_timeout(request_header_timeout());
    } else {
        h_engine->http_fp.set_timeout(keep_alive_timeout());
    }
    int first_ch = h_engine->http_fp.get();
    if (first_ch == -1) {
        h_engine->exception = true; return;
    }
    if (!first) {
        h_engine->http_fp.set_timeout(keep_alive_timeout());
    }
    linebuf[0] = first_ch;
    if ((llen = h_engine->http_fp.gets(linebuf + 1, var_httpd_header_line_max_size - 1)) < 10) {
        h_engine->exception = true; return;
    }
    llen ++;
    if (linebuf[llen-1] != '\n') {
        h_engine->exception = true; return;
    }
    llen--;
    if (linebuf[llen-1] == '\r') {
        llen --;
    }
    linebuf[llen] = 0;
    h_engine->method = memdupnull(linebuf, llen);
    ps = h_engine->method;
    p = strchr(ps, ' ');
    if (!p) {
        h_engine->exception = true; return;
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
        h_engine->exception = true; return;
    }
    llen -= (p - ps) + 1;
    ps = h_engine->uri = p + 1;

    if (llen < 10) {
        h_engine->exception = true; return;
    }
    p += llen;
    for (;p > ps; p--) {
        if (*p ==' ') {
            break;
        }
    }
    if (ps == p) {
        h_engine->exception = true; return;
    }
    *p = 0;
    h_engine->version = p + 1;
    toupper(h_engine->version);

    ps = h_engine->uri;
    p = strchr(ps, '?');
    if (!p) {
        h_engine->path = strdup(ps);
    } else {
        h_engine->path = memdupnull(ps, p - ps);
        ps = p + 1;
        p = strchr(ps, '#');
        if (p) {
            *p = 0;
        }
        http_url_parse_query(h_engine->request_query, ps);
        if (p) {
            *p = '#';
        }
    }

    /* read other header lines */
    while(1) {
        if ((llen = h_engine->http_fp.gets(linebuf, var_httpd_header_line_max_size)) < 1) {
            h_engine->exception = true; return;
        }
        if (linebuf[llen-1] != '\n') {
            h_engine->exception = true; return;
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
        h_engine->request_headers[linebuf] = ps;
        if (zcc_str_eq(linebuf, "host")) {
            h_engine->host = strdup(ps);
            p = strchr(ps, ':');
            if (p) {
                h_engine->host = strndup(ps, p-ps);
                h_engine->port = atoi(p+1);
            } else {
                h_engine->host = strdup(ps);
                h_engine->port = -1;
            }
            zcc::tolower(h_engine->host);
        } else if (zcc_str_eq(linebuf, "content-length")) {
            h_engine->request_content_length = atoi(ps);
        } else if (zcc_str_eq(linebuf, "cookie")) {
            http_cookie_parse_request(h_engine->request_cookies, ps);
        } else if (zcc_str_eq(linebuf, "connection")) {
            if (strcasestr(p, "keep-alive")) {
                h_engine->request_keep_alive = true;
            }
        }
    }

    /* read body */
    if (!need_body) {
        return;
    }

    if (h_engine->request_content_length < 1) {
        return;
    }
}

void httpd::request_data_do()
{
    h_engine->get_post_data_myself = false;
    handler_after_request_header();
    if (!h_engine->get_post_data_myself) {
        return;
    }
    if (h_engine->request_content_length < 1) {
        return;
    }
    if (max_length_for_post() <  h_engine->request_content_length) {
        h_engine->exception = true;
        return;
    }
    request_data_do_true();
}

void httpd::request_data_do_true()
{
    char *p = dict_get_str(h_engine->request_headers,"content-type");
    int ctype = 0;
    if (!strncasecmp(p, "application/x-www-form-urlencoded", 33)) {
        ctype = 'u';
    } else if (!strncasecmp(p, "multipart/form-data", 19)) {
        ctype = 'f';
    }
    if (ctype == 0) {
        return;
    }

    if (ctype == 'u') {
        /* application/x-www-form-urlencoded */
        std::string mbuf;
        if (h_engine->http_fp.readn(mbuf, h_engine->request_content_length) < h_engine->request_content_length) {
            h_engine->exception = true;
            return;
        }
        http_url_parse_query(h_engine->request_post, mbuf.c_str());
        return;
    }

    /* multipart/form-data */
    autobuffer readline_auto;
    char *linebuf = (char *)malloc(var_httpd_header_line_max_size + 1);
    readline_auto.data = linebuf;

    /* tmp filename */
    strcpy(linebuf, tmp_path_for_post());
    int len = strlen(linebuf);
    if (linebuf[0] == 0) {
        strcpy(linebuf, "/tmp/");
        len = 5;
    } else {
        if (linebuf[len-1] != '/') {
            linebuf[len++] = '/';
            linebuf[len] = 0;
        }
    }

    build_unique_filename_id(linebuf+len);
    char *data_filename = strdup(linebuf);
    autobuffer data_filename_auto;
    data_filename_auto.data = data_filename;

    /* save to tmp file */
    fstream fp;
    if (!fp.open(data_filename, "w+")) {
        zcc_info("error open tmp file %s(%m)", data_filename);
        h_engine->exception = true;
        return;
    }
    long left = h_engine->request_content_length;
    while(left > 0) {
        int rlen = left;
        if (rlen > var_httpd_header_line_max_size) {
            rlen = var_httpd_header_line_max_size;
        }
        rlen = h_engine->http_fp.readn(linebuf, rlen);
        if (rlen < 1) {
            unlink(data_filename);
            h_engine->exception = true;
            return;
        }
        left -= rlen;
    }
    fp.flush();
    if (fp.is_error()) {
        unlink(data_filename);
        h_engine->exception = true;
        return;
    }
}


void httpd::upload_file_parse_dump_file(const char *data_filename, std::string &saved_path, std::string &content, int file_id_plus, const char *name, const char *filename)
{
    saved_path = data_filename;
    saved_path.append("_");
    saved_path += file_id_plus;
    if (!file_put_contents(saved_path.c_str(), content.c_str(), content.size())) {
        h_engine->exception = true;
        return;
    }

    httpd_upload_file huf;
    huf.name = name;
    huf.filename = filename;
    huf.saved_filename = saved_path;
    huf.size = content.size();
    h_engine->request_upload_files.push_back(huf);
}

void httpd::upload_file_parse_walk_mime(mail_parser_mime * mime, const char *data_filename, std::string &saved_path, std::string &content, int file_id_plus, std::string &disposition_raw, std::map<std::string, std::string> &params)
{
    const char *disposition = mime->disposition().c_str();
    if (strncasecmp(disposition, "form-data", 9)) {
        return;
    }
    disposition_raw.clear();
    if(!mime->raw_header_line("content-disposition", disposition_raw)){
        return;
    }
    params.clear();
    content.clear();
    mime_header_line_get_params(disposition_raw.c_str(), disposition_raw.size(), content, params);
    char *name = dict_get_str(params, "name");
    tolower(name);
    char *filename = dict_get_str(params, "filename");
    const char *ctype = mime->type().c_str();
    if (strncasecmp(ctype, "multipart/", 10)) {
        mime->decoded_content(content);
        if (empty(filename)) {
            h_engine->request_post[name]= content;
            return;
        } else {
            upload_file_parse_dump_file(data_filename, saved_path, content, file_id_plus, name, filename);
        }
    } else {
        for (mail_parser_mime *mm = mime->child(); mm; mm = mm->next()) {
            ctype = mm->type().c_str();
            if (!strncasecmp(ctype, "multipart/", 10)) {
                return;
            }
            disposition_raw.clear();
            filename = blank_buffer;
            if(mime->raw_header_line("content-disposition", disposition_raw)){
                params.clear();
                content.clear();
                mime_header_line_get_params(disposition_raw.c_str(), disposition_raw.size(), content, params);
                filename = dict_get_str(params, "filanme");
            }
            upload_file_parse_dump_file(data_filename, saved_path, content, file_id_plus, name, filename);
            if (h_engine->exception) {
                return;
            }
        }
    }
}

void httpd::upload_file_parse(const char *data_filename)
{
    std::string saved_path;
    file_mmap mreader;

    if (!mreader.mmap(data_filename)) {
        h_engine->exception = true;
        return;
    }
    mail_parser mparser;
    mparser.parse(mreader.data(), mreader.size());

    const std::list<mail_parser_mime *> &all_mimes = mparser.all_mimes();
    std::string disposition_raw, content;
    std::map<std::string, std::string> params;
    int file_id_plus = 0;
    std_list_walk_begin(all_mimes, mime) {
        upload_file_parse_walk_mime(mime, data_filename, saved_path, content, file_id_plus++, disposition_raw, params);
        if (h_engine->exception) {
            return;
        }
    } std_list_walk_end;
}

}

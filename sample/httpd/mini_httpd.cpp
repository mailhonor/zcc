/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zcc.h"

#define RFC1123_TIME "%a, %d %b %Y %H:%M:%S GMT"

namespace zcc
{

class mini_httpd
{
public:
    mini_httpd();
    void bind(int sock, event_base &eb = default_evbase);
    virtual ~mini_httpd();
    void use_SSL_CTX(SSL_CTX *sslctx);
    void run();
    virtual void handler();
    virtual void after_ssl_initialization();
    inline bool request_gzip() { return ___gzip; }
    inline bool request_deflate() { return ___deflate; }
    inline bool request_keep_alive() { return ___keep_alive; }
    const char *request_header(const char *name, const char *def_val = blank_buffer);
    virtual void response_500();
    virtual void response_404();
    void response_304(const char *etag);
    void response_gzip(bool tf=true);
    void response_content_length(long length);
    void response_content_type(const char *content_type, const char *charset = blank_buffer);
    void response_etag(const char *etag);
    void response_last_modified(long last_modified);
    void response_date(long date);
    void response_expires(long expires);

    void response_body(const char *content, size_t size);
    void response_file(const char *path, const char *content_type);

    void response_part_body(const char *content, size_t size, void (*after_part_flush)(mini_httpd &httpd));
/* private */
    void deal_header_line();
    void deal_first_line();
    void after_flush();
 private: 
    void response_headers(long length); /* length < 0: means no header of Content-Length */
    void response_done();
    void loop_clear();
    void read_post_body();
    char *___method;
    SSL_CTX *___sslctx;
    std::string ___uri;
    std::string ___host;
    dict ___cookies;
    dict ___request_headers;
    std::string ___etag;
    int ___content_length;
    int ___content_read_length;
    std::string ___content_data;
    FILE *___content_fp;
    std::string ___content_path;
    /* flag */
    bool ___bind;
    /* flag */
    bool ___100_continue;
    bool ___gzip;
    bool ___deflate;
    bool ___keep_alive;
    /* flag */
    bool ___cookie_parsed;
    /* */
    bool ___response_header_output_flag;
    bool ___response_part_flag_mode;
    bool ___response_gzip;
    long ___response_content_length;
    std::string ___response_content_type;
    std::string ___response_charset;
    std::string ___response_etag;
    long ___response_last_modified;
    long ___response_date;
    long ___response_expires;
    dict ___response_cookies;
    /* */
    async_io aio;
    void *context;
    void *inner_context;
    void (*___after_part_flush)(mini_httpd &httpd);
};

static const int ___header_line_max_size = 10240;

static void ___delete_mini_httpd(mini_httpd *o)
{
    delete o;
}

static void ___deal_header_line(async_io &aio)
{
    mini_httpd &hs = *((mini_httpd *)aio.get_context());
    hs.deal_header_line();
}

static void ___deal_first_line(async_io &aio)
{
    mini_httpd &hs = *((mini_httpd *)aio.get_context());
    hs.deal_first_line();
}

static void ___after_flush(async_io &aio)
{
    mini_httpd &hs = *((mini_httpd *)aio.get_context());
    hs.after_flush();
}

mini_httpd::mini_httpd()
{
    loop_clear();
    aio.set_context(this);
}

void mini_httpd::bind(int sock, event_base &eb)
{
    ___bind = true;
    aio.init(sock, eb);
}

mini_httpd::~mini_httpd()
{
    loop_clear();
}

void mini_httpd::loop_clear()
{
    ___uri.clear();
    ___host.clear();
    ___content_length = -1;
    ___cookies.clear();
    ___request_headers.clear();
    ___content_data.clear();
    if (___content_fp) {
        fclose(___content_fp);
        ___content_fp = 0;
    }
    ___content_path.clear();

    /* flag */
    ___gzip = false;
    ___deflate = false;
    ___100_continue = false;
    ___keep_alive = false;

    /* */
    ___cookie_parsed = false;
    ___response_header_output_flag = false;
    ___response_part_flag_mode = false;
    ___response_gzip = false;
    ___response_content_length = -1;
    ___response_content_type.clear();
    ___response_charset.clear();
    ___response_etag.clear();
    ___response_last_modified = -1;
    ___response_date = -1;
    ___response_expires = -1;
    ___response_cookies.clear();
    /* */
    ___after_part_flush = 0;
}

void mini_httpd::run()
{
    if (___bind == false) {
        zcc_info("ERROR mini_httpd need bind before execute 'run'");
        ___delete_mini_httpd(this);
        return;
    }
    aio.read_line(1024 * 10, ___deal_first_line, 10 * 1000);
}

void mini_httpd::handler()
{
    response_404();
}

void mini_httpd::after_ssl_initialization()
{
}

void mini_httpd::response_500()
{
    char output[] = "HTTP/1.0 500 Error\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "500 Internal Server Error ";
    aio.cache_write(output, sizeof(output) -1);
    ___keep_alive = false;
    response_done();
}

void mini_httpd::response_404()
{
    char output[] = "HTTP/1.0 404 Not Found\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "404 Not Found ";
    aio.cache_write(output, sizeof(output) -1);
    ___keep_alive = false;
    response_done();
}

void mini_httpd::response_304(const char *etag)
{
    std::string output = "Etag: ";
    output += etag;
    output += "\r\nHTTP/1.1 304 Not Modified\r\n";
    aio.cache_write(output.c_str(), output.size());
    response_done();
}

void mini_httpd::response_done()
{
    /* FIXME timeout */
    size_t wsize = aio.get_cache_size();
    long timeout = 0;
    timeout = 10 * 1000 + (wsize/1024) * 1000;
    aio.cache_flush(___after_flush, timeout);
}

void mini_httpd::deal_first_line()
{
    mini_httpd &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = aio.get_ret();
    if ((ret < 0) || (ret >___header_line_max_size)) {
        ___delete_mini_httpd(this);
        return;
    }
    aio.fetch_rbuf(buf, ret);
    buf[ret] = 0;
    char *ps, *p;
    ps = buf;
    p = strchr(buf, ' ');
    if (!p) {
        ___delete_mini_httpd(this);
        return;
    }
    *p = 0;
    to_upper(ps);
    const char *meths;
    if (zcc_str_eq(ps, "GET")) {
        meths = "GET";
    } else if (zcc_str_eq(ps, "POST")) {
        meths = "POST";
    } else if (zcc_str_eq(ps, "HEAD")) {
        meths = "HEAD";
    } else if (zcc_str_eq(ps, "PUT")) {
        meths = "PUT";
    } else if (zcc_str_eq(ps, "TRACE")) {
        meths = "TRACE";
    } else if (zcc_str_eq(ps, "OPTIONS")) {
        meths = "OPTIONS";
    } else {
        ___delete_mini_httpd(this);
        return;
    }
    ___method = const_cast<char *>(meths);
    
    ps = p+1;
    p = strchr(ps, ' ');
    if (!p) {
        ___delete_mini_httpd(this);
        return;
    }

    *p++ = 0;
    if (strncasecmp(p, "HTTP/1.", 7)) {
        ___delete_mini_httpd(this);
        return;
    }
    p += 7;
    if (*p == '1') {
        /* version 1.1 */
    }
    p = strstr(ps, "://");
    if (p) {
        ps = p + 3;
        p = strchr(ps, '/');
        if (!p) {
            ___delete_mini_httpd(this);
            return;
        }
        ps = p;
    }
    ___uri = ps;

    aio.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

void mini_httpd::deal_header_line()
{
    mini_httpd &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = aio.get_ret();
    if ((ret < 0) || (ret>___header_line_max_size)) {
        ___delete_mini_httpd(this);
        return;
    }
    aio.fetch_rbuf(buf, ret);
    buf[ret] = 0;
    if (((ret == 1) &&(buf[0] == '\n'))||((ret==2)&&(buf[0]=='\r')&&(buf[1]=='\n'))) {
        if (zcc_str_eq(___method, "POST") || zcc_str_eq(___method, "PUT")) {
            read_post_body();
            return;
        }
        handler();
        return;
    }
    if (buf[ret - 1] == '\n') {
        ret --;
    }
    if (ret > 0 && buf[ret - 1] == '\n') {
        ret --;
    }
    buf[ret] = 0;
    char *p;
    p = strchr(buf, ' ');
    if (!p) {
        aio.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
        return;
    }
    *p++ = 0;
    while(*p == ' ') {
        p++;
    }
    to_lower(buf);
    ___request_headers.update(buf, p);
    if (zcc_str_eq(buf, "host")){
        ___host = p;
    } else if (zcc_str_eq(buf, "content-length")){
        ___content_length = atoi(p);
    } else if (zcc_str_eq(buf, "expect")){
        if (!strncasecmp(p, "100-continue", 12)) {
            ___100_continue = true;
        }
    } else if (zcc_str_eq(buf, "accept-encoding")){
        to_lower(p);
        if (strstr(p, "gzip")) {
            ___gzip = true;
        }
        if (strstr(p, "deflate")) {
            ___deflate = true;
        }
    } else if (zcc_str_eq(buf, "connection")){
        to_lower(p);
        if (strstr(p, "keep-alive")) {
            ___keep_alive = true;
        }
    } else if (zcc_str_eq(buf, "if_none_match")){
        ___etag = p;
    }
    aio.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

void mini_httpd::after_flush()
{
    int ret = aio.get_ret();
    if (ret < 0 || (!request_keep_alive())) {
        ___delete_mini_httpd(this);
        return;
    }
    loop_clear();
    aio.read_line(1024 * 10, ___deal_first_line, 100 * 1000);
}

const char *mini_httpd::request_header(const char *name, const char *def_val)
{
    char nbuf[1024 + 1];
    int len = strlen(name);
    if (len > 1024) {
        return def_val;
    }
    if (len == 0) {
        return def_val;
    }
    memcpy(nbuf, name, len);
    nbuf[len] = 0;
    to_lower(nbuf);
    return ___request_headers.get_str(nbuf, def_val);
}

void mini_httpd::read_post_body()
{
    if (___content_length == -1) {
        ___delete_mini_httpd(this);
        return;
    }
}

void mini_httpd::response_gzip(bool tf)
{
    ___response_gzip = tf;
}

void mini_httpd::response_content_length(long length)
{
    ___response_content_length = length;
}

void mini_httpd::response_content_type(const char *content_type, const char *charset)
{
    ___response_content_type = content_type;
    ___response_charset = charset;
}

void mini_httpd::response_etag(const char *etag)
{
    ___response_etag = etag;
}

void mini_httpd::response_last_modified(long last_modified)
{
    ___response_last_modified = last_modified;
}

void mini_httpd::response_date(long date)
{
    ___response_date = date;
}

void mini_httpd::response_expires(long expires)
{
    ___response_expires = expires;
}

void mini_httpd::response_headers(long length)
{
    std::string hbuf;
    char timestringbuf[64 + 1];
    struct tm tmbuf;

    hbuf.append("HTTP/1.1 200 ZCC\r\n");

    if (___keep_alive) {
        hbuf.append("Connection:keep-alive\r\n");
    }

    if (___response_gzip) {
        hbuf.append("Content-Encoding:gzip\r\n");
    }

    if (length == -1) {
        
    }

    if (length > -1 ) {
        hbuf.append("Content-Length: ");
        hbuf += size;
        hbuf.append("\r\n");
    } else if (___response_content_length > -1) {
        hbuf.append("Content-Length: ");
        hbuf += ___response_content_length;
        hbuf.append("\r\n");
    }

    do {
        hbuf.append("Content-Type:");
        if (___response_content_type.empty()) {
            hbuf.append("text/html");
        } else {
            hbuf.append(___response_content_type.c_str());
        }
        if (!___response_charset.empty()) {
            hbuf.append("; charset=");
            hbuf.append(___response_charset.c_str());
        }
        hbuf.append("\r\n");
    } while(0);

    if (!___response_etag.empty()) {
        hbuf.append("Etag:" );
        hbuf.append(___response_etag.c_str());
        hbuf.append("\r\n");
    }

    if (___response_last_modified != -1) {
        gmtime_r((time_t *)(&___response_last_modified), &tmbuf);
        strftime(timestringbuf, 64, RFC1123_TIME, &tmbuf);
        hbuf.append("Last-Modified: ");
        hbuf.append(timestringbuf);
        hbuf.append("\r\n");
    }
    if (___response_date == -1) {
        ___response_date = time(0);
    }
    do {
        gmtime_r((time_t *)(&___response_date), &tmbuf);
        strftime(timestringbuf, 64, RFC1123_TIME, &tmbuf);
        hbuf.append("Date: ");
        hbuf.append(timestringbuf);
        hbuf.append("\r\n");
    } while(0);

    if (___response_expires != -1) {
        gmtime_r((time_t *)(&___response_expires), &tmbuf);
        strftime(timestringbuf, 64, RFC1123_TIME, &tmbuf);
        hbuf.append("Expires: ");
        hbuf.append(timestringbuf);
        hbuf.append("\r\n");
    }

    do {
        hbuf.append("Server: ZCC/1.0\r\n");
    } while(0);

    hbuf.append("\r\n");
}

void mini_httpd::response_body(const char *content, size_t size)
{
    aio.cache_write(hbuf.c_str(), hbuf.size());
    aio.cache_write(content, size);
    response_done();
}

void mini_httpd::response_part_body(const char *content, size_t size, void (*after_part_flush)(mini_httpd &httpd))
{
}

void response_file(const char *path, const char *content_type)
{
}

}


class test_httpd: public zcc::mini_httpd
{
    public:
        inline test_httpd() {}
        inline ~test_httpd() {}
};

class test_server: public zcc::master_event_server
{
    public:
        void simple_service(int fd);
};

void test_server::simple_service(int fd)
{
    test_httpd *httpd = new test_httpd();
    httpd->bind(fd);
    httpd->run();
}

int main(int argc, char **argv)
{
    test_server ms;
    ms.run(argc, argv);
    return 0;
}


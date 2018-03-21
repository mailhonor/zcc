/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zcc.h"
#include <time.h>

#define RFC1123_TIME "%a, %d %b %Y %H:%M:%S GMT"

namespace zcc
{

class async_httpd
{
public:
    async_httpd();
    void bind(int sock, event_base &eb = default_evbase);
    virtual ~async_httpd();
    inline void set_context(const void *ctx) { ___context = const_cast <void *> (ctx); }
    inline void *get_context() { return ___context; }
    inline void set_stop() { ___stop = true; }
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

    void response_body(const char *content, size_t size, void(*after_flush)(async_httpd &httpd, bool ret) = 0);
    void response_file(const char *path, const char *content_type);

    void response_part_body(const char *con, size_t n, void(*after_flush)(async_httpd &httpd, bool ret));
    void response_chunked_body(const char *con, size_t n, void(*after_flush)(async_httpd &httpd, bool ret));

    void sleep(void(*after_sleep)(async_httpd &httpd, bool ret), long timeout);

/* private */
    void deal_header_line();
    void deal_first_line();
    void after_flush_inner();
 private: 
    void response_headers(long length); /* length < 0: means no header of Content-Length */
    void response_done();
    void loop_clear();
    void read_post_body();
    char *___method;
    SSL_CTX *___sslctx;
    string ___uri;
    string ___host;
    dict ___cookies;
    dict ___request_headers;
    string ___etag;
    int ___content_length;
    int ___content_read_length;
    string ___content_data;
    FILE *___content_fp;
    string ___content_path;
    /* flag */
    bool ___bind;
    bool ___stop;
    /* flag */
    bool ___100_continue;
    bool ___gzip;
    bool ___deflate;
    bool ___keep_alive;
    /* flag */
    bool ___cookie_parsed;
    /* */
    bool ___response_header_output_flag;
    bool ___response_normal_flag_mode;
    bool ___response_specail_flag_mode;
    bool ___response_gzip;
    long ___response_content_length;
    string ___response_content_type;
    string ___response_charset;
    string ___response_etag;
    long ___response_last_modified;
    long ___response_date;
    long ___response_expires;
    dict ___response_cookies;
    /* */
    async_io aio;
    void *___context;
    void *___inner_context;
    void (*___after_flush)(async_httpd &httpd, bool ret);
};

static const int ___header_line_max_size = 10240;

static void ___delete_async_httpd(async_httpd *o)
{
    delete o;
}

static void ___deal_header_line(async_io &aio)
{
    async_httpd &hs = *((async_httpd *)aio.get_context());
    hs.deal_header_line();
}

static void ___deal_first_line(async_io &aio)
{
    async_httpd &hs = *((async_httpd *)aio.get_context());
    hs.deal_first_line();
}

static void ___after_flush_inner(async_io &aio)
{
    async_httpd &hs = *((async_httpd *)aio.get_context());
    hs.after_flush_inner();
}

async_httpd::async_httpd()
{
    loop_clear();
    aio.set_context(this);
}

void async_httpd::bind(int sock, event_base &eb)
{
    ___bind = true;
    aio.bind(sock, eb);
}

async_httpd::~async_httpd()
{
    loop_clear();
}

void async_httpd::loop_clear()
{
    ___stop = false;
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
    ___response_normal_flag_mode = false;
    ___response_specail_flag_mode = false;
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
    ___after_flush = 0;
}

void async_httpd::run()
{
    if (___bind == false) {
        zcc_info("ERROR async_httpd need bind before execute 'run'");
        ___delete_async_httpd(this);
        return;
    }
    if (___stop) {
        ___delete_async_httpd(this);
    }
    loop_clear();
    aio.read_line(1024 * 10, ___deal_first_line, 10 * 1000);
}

void async_httpd::handler()
{
    response_404();
}

void async_httpd::after_ssl_initialization()
{
}

void async_httpd::response_500()
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

void async_httpd::response_404()
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

void async_httpd::response_304(const char *etag)
{
    string output = "Etag: ";
    output += etag;
    output += "\r\nHTTP/1.1 304 Not Modified\r\n";
    aio.cache_write(output.c_str(), output.size());
    response_done();
}

void async_httpd::response_done()
{
    size_t wsize = aio.get_cache_size();
    long timeout = 10 * 1000 + (wsize/1024) * 1000;
    aio.cache_flush(___after_flush_inner, timeout);
}

void async_httpd::deal_first_line()
{
    async_httpd &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = aio.get_result();
    if ((ret < 0) || (ret >___header_line_max_size)) {
        ___delete_async_httpd(this);
        return;
    }
    aio.fetch_rbuf(buf, ret);
    buf[ret] = 0;
    char *ps, *p;
    ps = buf;
    p = strchr(buf, ' ');
    if (!p) {
        ___delete_async_httpd(this);
        return;
    }
    *p = 0;
    toupper(ps);
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
        ___delete_async_httpd(this);
        return;
    }
    ___method = const_cast<char *>(meths);
    
    ps = p+1;
    p = strchr(ps, ' ');
    if (!p) {
        ___delete_async_httpd(this);
        return;
    }

    *p++ = 0;
    if (strncasecmp(p, "HTTP/1.", 7)) {
        ___delete_async_httpd(this);
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
            ___delete_async_httpd(this);
            return;
        }
        ps = p;
    }
    ___uri = ps;

    aio.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

void async_httpd::deal_header_line()
{
    async_httpd &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = aio.get_result();
    if ((ret < 0) || (ret>___header_line_max_size)) {
        ___delete_async_httpd(this);
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
    tolower(buf);
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
        tolower(p);
        if (strstr(p, "gzip")) {
            ___gzip = true;
        }
        if (strstr(p, "deflate")) {
            ___deflate = true;
        }
    } else if (zcc_str_eq(buf, "connection")){
        tolower(p);
        if (strstr(p, "keep-alive")) {
            ___keep_alive = true;
        }
    } else if (zcc_str_eq(buf, "if_none_match")){
        ___etag = p;
    }
    aio.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

void async_httpd::after_flush_inner()
{
    int ret = aio.get_result();
    if (ret < 0 || (!request_keep_alive())) {
        if (___after_flush) {
            ___after_flush(*this, false);
        }
        ___delete_async_httpd(this);
        return;
    }

    if (___response_normal_flag_mode) {
        if (___after_flush) {
            ___after_flush(*this, true);
        }
        run();
    }

    if (___after_flush) {
        ___stop = false;
        ___after_flush(*this, true);
        if (___stop) {
            ___delete_async_httpd(this);
        }
        return;
    }
    ___delete_async_httpd(this);
}

const char *async_httpd::request_header(const char *name, const char *def_val)
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
    tolower(nbuf);
    return ___request_headers.get_str(nbuf, def_val);
}

void async_httpd::read_post_body()
{
    if (___content_length == -1) {
        ___delete_async_httpd(this);
        return;
    }
}

void async_httpd::response_gzip(bool tf)
{
    ___response_gzip = tf;
}

void async_httpd::response_content_length(long length)
{
    ___response_content_length = length;
}

void async_httpd::response_content_type(const char *content_type, const char *charset)
{
    ___response_content_type = content_type;
    ___response_charset = charset;
}

void async_httpd::response_etag(const char *etag)
{
    ___response_etag = etag;
    /* maybe do nothing follow, so this set ___stop = true; */
}

void async_httpd::response_last_modified(long last_modified)
{
    ___response_last_modified = last_modified;
}

void async_httpd::response_date(long date)
{
    ___response_date = date;
}

void async_httpd::response_expires(long expires)
{
    ___response_expires = expires;
}

void async_httpd::response_headers(long length)
{
    string hbuf;
    char timestringbuf[64 + 1];
    struct tm tmbuf;

    ___response_header_output_flag = true;

    hbuf.append("HTTP/1.1 200 ZCC\r\n");

    if (___keep_alive) {
        hbuf.append("Connection:keep-alive\r\n");
    }

    if (___response_gzip) {
        hbuf.append("Content-Encoding:gzip\r\n");
    }

    if (length == -1) {
        
    }

    if (length == -1) {
        if (___response_content_length > -1) {
            hbuf.append("Content-Length: ");
            hbuf += ___response_content_length;
            hbuf.append("\r\n");
        }
    } else if (length == -2) {
        hbuf.append("Transfer-Encoding: chunked\r\n");
    } else if (length > -1 ) {
        hbuf.append("Content-Length: ");
        hbuf += length;
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
    aio.cache_write(hbuf.c_str(), hbuf.size());
}

void async_httpd::response_body(const char *content, size_t size, void(*after_flush)(async_httpd &, bool))
{
    ___after_flush = after_flush;
    ___response_normal_flag_mode = true;
    ___stop = false;
    response_headers(size);
    aio.cache_write(content, size);
    response_done();
}

void async_httpd::response_part_body(const char *content, size_t size, void (*after_flush)(async_httpd &, bool))
{
    ___after_flush = after_flush;
    if (___response_header_output_flag) {
        response_headers(-1);
    }
    ___response_specail_flag_mode = true;
    ___stop = false;
    aio.cache_write(content, size);
    response_done();
}

void async_httpd::response_chunked_body(const char *content, size_t size, void (*after_flush)(async_httpd &, bool))
{
    ___after_flush = after_flush;
    if (___response_header_output_flag) {
        response_headers(-2);
    }
    ___response_specail_flag_mode = true;
    ___stop = false;

    char hbuf[64];
    sprintf(hbuf, "%x\r\n", (unsigned int)size);
    aio.cache_puts(hbuf);
    aio.cache_write(content, size);
    aio.cache_puts("\r\n");
    response_done();
}

void async_httpd::response_file(const char *path, const char *content_type)
{
}

void async_httpd::sleep(void(*after_sleep)(async_httpd &httpd, bool ret), long timeout)
{
    ___after_flush = after_sleep;
    aio.sleep(___after_flush_inner, timeout);
}

}


class test_httpd: public zcc::async_httpd
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


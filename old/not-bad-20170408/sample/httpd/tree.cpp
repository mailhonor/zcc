/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zcc.h"

namespace zcc
{

class httpd_service: public async_io
{
public:
    httpd_service();
    virtual ~httpd_service();
    void set_release_handler(void (*release_handler)(httpd_service &));
    void use_SSL_CTX(SSL_CTX *sslctx);
    void run();
    virtual void after_ssl_initialization();
    virtual void response_handler();
    virtual void before_post_data();
    virtual void response_500(const char *data = 0, size_t size = 0);
    virtual void response_404(const char *data = 0, size_t size = 0);
    void response_title(const char *name);
    void response_header(const char *name, const char *value);
    void response_body(const char *content, size_t size);
    void response_done(); 
    inline bool flag_gzip() { return ___gzip; }
    inline bool flag_deflate() { return ___deflate; }
    inline bool flag_keep_alive() { return ___keep_alive; }
    char *get_version();
    void deal_header_line();
    void deal_first_line();
    void (*___release_handler)(httpd_service &);
 private: 
    void loop_clear();
    char *___method;
    SSL_CTX *___sslctx;
    char *___uri;
    char * ___host;
    char * ___raw_cookie;
    list<char *> ___cookies;
    list<char *> ___headers;
    ssize_t ___content_length;
    /* flag */
    bool ___gzip;
    bool ___deflate;
    bool ___version;
    bool ___100_continue;
    bool ___keep_alive;
    /* */
    bool ___ignore_post_data;
    bool ___stop;
    /* */
    str_dict ___response_headers;
};

const int ___header_line_max_size = 1024 * 10;

static void ___release_do(httpd_service &hs)
{
    int fd = hs.get_fd();
    if (hs.___release_handler) {
        hs.___release_handler(hs);
        return;
    }
    delete &hs;
    close(fd);
}


static void ___deal_header_line(async_io &aio)
{
    httpd_service &hs = (httpd_service &)aio;
    hs.deal_header_line();
}

static void ___deal_first_line(async_io &aio)
{
    httpd_service &hs = (httpd_service &)aio;
    hs.deal_first_line();
}

static void ___after_flush(async_io &aio)
{
    httpd_service &hs = (httpd_service &)aio;
    int ret = hs.get_ret();
    if (ret < 0 || (!hs.flag_keep_alive())) {
        ___release_do(hs);
        return;
    }

    hs.read_line(1024 * 10, ___deal_first_line, 100 * 1000);
}

httpd_service::httpd_service()
{
    ___sslctx = 0;
    ___release_handler = 0;
    ___uri = blank_buffer;
    ___host = blank_buffer;
    ___raw_cookie = blank_buffer;
    ___content_length = -1;
    /* flag */
    ___gzip = false;
    ___deflate = false;
    ___version = false;
    ___100_continue = false;
    ___keep_alive = false;
    /* */
    ___ignore_post_data = false;
    ___stop = false;
}

httpd_service::~httpd_service()
{
    loop_clear();
}

void httpd_service::use_SSL_CTX(SSL_CTX *sslctx)
{
    ___sslctx = sslctx;
}

void httpd_service::after_ssl_initialization()
{
}

void httpd_service::response_handler()
{
    response_404();
}

void httpd_service::before_post_data()
{
}

void httpd_service::response_500(const char *data, size_t size)
{
    char output[] = "HTTP/1.0 500 Error\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "500 Internal Server Error ";
    if (!data || !size) {
        cache_write(output, sizeof(output) -1);
    } else {
        cache_write(data, size);
    }
    response_done();
}

void httpd_service::response_404(const char *data, size_t size)
{
    char output[] = "HTTP/1.0 404 Not Found\r\n"
        "Server: ZCC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "404 Not Found ";
    if (!data || !size) {
        cache_write(output, sizeof(output) -1);
    } else {
        cache_write(data, size);
    }
    response_done();
}

void httpd_service::response_done()
{
    /* FIXME : timeout */
    cache_flush(___after_flush, 10 * 1000);
}

char *httpd_service::get_version()
{
    return const_cast<char *>(___version?"1.1":"1.0");
}

void httpd_service::loop_clear()
{
#define ___free(p) free(p); p=blank_buffer;
    char *p;
    ___free(___uri);
    ___free(___host);
    ___free(___raw_cookie);
    ___content_length = -1;

    zcc_list_walk_begin(___cookies, p) {
        free(p);
    } zcc_list_walk_end;
    ___cookies.clear();

    zcc_list_walk_begin(___headers, p) {
        free(p);
    } zcc_list_walk_end;
    ___headers.clear();

    /* flag */
    ___gzip = false;
    ___deflate = false;
    ___keep_alive = false;

    /* */
    ___ignore_post_data = false;
    ___stop = false;
    /* */
    ___response_headers.reset();
}

void httpd_service::run()
{
    read_line(1024 * 10, ___deal_first_line, 10 * 1000);
}

void httpd_service::deal_header_line()
{
    httpd_service &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = hs.get_ret();
    if ((ret < 0) || (ret>___header_line_max_size)) {
        ___release_do(hs);
        return;
    }
    fetch_rbuf(buf, ret);
    buf[ret] = 0;
    if (((ret == 1) &&(buf[0] == '\n'))||((ret==2)&&(buf[0]=='\r')&&(buf[1]=='\n'))) {
        if (ZCC_STR_EQ(___method, "POST") || ZCC_STR_EQ(___method, "PUT")) {
                before_post_data();
                return;
        }
        response_handler();
        return;
    }
    if (buf[ret - 1] == '\n') {
        ret --;
    }
    if (ret > 0 && buf[ret - 1] == '\n') {
        ret --;
    }
    buf[ret] = 0;
    ___headers.push_back((char *)memdupnull(buf, ret));
    char *p;
    p = strchr(buf, ' ');
    if (!p) {
        read_line(1024 * 10, ___deal_header_line, 10 * 1000);
        return;
    }
    *p++ = 0;
    while(*p == ' ') {
        p++;
    }
    to_lower(buf);
    if (ZCC_STR_EQ(buf, "host")){
        ___host = strdup(p);
    } else if (ZCC_STR_EQ(buf, "content-length")){
        ___content_length = atoi(p);
    } else if (ZCC_STR_EQ(buf, "expect")){
        if (!strncasecmp(p, "100-continue", 12)) {
            ___100_continue = true;
        }
    } else if (ZCC_STR_EQ(buf, "accept-encoding")){
        to_lower(p);
        if (strstr(p, "gzip")) {
            ___gzip = true;
        }
        if (strstr(p, "deflate")) {
            ___deflate = true;
        }
    } else if (ZCC_STR_EQ(buf, "connection")){
        to_lower(p);
        if (strstr(p, "keep-alive")) {
            ___keep_alive = true;
        }
    }
    hs.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

void httpd_service::deal_first_line()
{
    httpd_service &hs = *this;
    char buf[___header_line_max_size + 10];
    int ret = get_ret();
    if ((ret < 0) || (ret>___header_line_max_size)) {
        ___release_do(hs);
        return;
    }
    fetch_rbuf(buf, ret);
    buf[ret] = 0;
    char *ps, *p;
    ps = buf;
    p = strchr(buf, ' ');
    if (!p) {
        ___release_do(hs);
        return;
    }
    *p = 0;
    to_upper(ps);
    const char *meths;
    if (ZCC_STR_EQ(ps, "GET")) {
        meths = "GET";
    } else if (ZCC_STR_EQ(ps, "POST")) {
        meths = "POST";
    } else if (ZCC_STR_EQ(ps, "HEAD")) {
        meths = "HEAD";
    } else if (ZCC_STR_EQ(ps, "PUT")) {
        meths = "PUT";
    } else if (ZCC_STR_EQ(ps, "TRACE")) {
        meths = "TRACE";
    } else if (ZCC_STR_EQ(ps, "OPTIONS")) {
        meths = "OPTIONS";
    } else {
        ___release_do(hs);
        return;
    }
    ___method = const_cast<char *>(meths);
    
    ps = p+1;
    p = strchr(ps, ' ');
    if (!p) {
        ___release_do(hs);
        return;
    }

    *p++ = 0;
    if (strncasecmp(p, "HTTP/1.", 7)) {
        ___release_do(hs);
        return;
    }
    p += 7;
    if (*p == '1') {
        ___version = true;
    }
    p = strstr(ps, "://");
    if (p) {
        ps = p + 3;
        p = strchr(ps, '/');
        if (!p) {
            ___release_do(hs);
            return;
        }
        ps = p;
    }
    ___uri = strdup(ps);

    hs.read_line(1024 * 10, ___deal_header_line, 10 * 1000);
}

}

class mytree:public zcc::httpd_service
{
public:
    inline mytree() {}
    inline ~mytree() {}
};


/* ########################################################################### */

class httpd_test_server: public zcc::master_server
{
public:
    inline httpd_test_server() {};
    inline ~httpd_test_server() {};
    void simple_service(int fd);
};

void httpd_test_server::simple_service(int fd)
{
    mytree *tr = new mytree();
    tr->init(fd);
    tr->run();
}

int main(int argc, char **argv)
{
    httpd_test_server ms;
    ms.run(argc, argv);
    return 0;
}

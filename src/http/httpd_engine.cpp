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

void httpd_engine::loop_clear()
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
    std_list_walk_begin(request_upload_files, uf) {
        free(uf->name);
        free(uf->filename);
        unlink(uf->saved_filename);
        free(uf);
    } std_list_walk_end;
    request_upload_files.clear();
    request_keep_alive = false;
    response_initialization = false;
    response_content_type = false;
}

void httpd_engine::request_header_do(bool first)
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
        request_headers[linebuf] = ps;
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

void httpd_engine::request_data_do()
{
    httpd *hddata = (httpd *)zcc_container_of(this, httpd, ___data);
    get_post_data_myself = false;
    hddata->handler_after_request_header();
    if (!get_post_data_myself) {
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

void httpd_engine::request_data_do_true()
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
        /* application/x-www-form-urlencoded */
        std::string mbuf;
        mbuf.reserve(request_content_length);
        if (http_fp->readn(mbuf, request_content_length) < request_content_length) {
            exception = true;
            return;
        }
        request_post.parse_url_query(mbuf.c_str());
        return;
    }

    /* multipart/form-data */
    autobuffer readline_auto;
    char *linebuf = (char *)malloc(___header_line_max_size + 1);
    readline_auto.data = linebuf;

    /* tmp filename */
    strcpy(linebuf, hddata->tmp_path_for_post());
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
    FILE *fp = fopen(data_filename, "w+");
    if (!fp) {
        zcc_info("error open tmp file %s(%m)", data_filename);
        exception = true;
        return;
    }
    long left = request_content_length;
    while(left > 0) {
        int rlen = left;
        if (rlen > ___header_line_max_size) {
            rlen = ___header_line_max_size;
        }
        rlen = http_fp->readn(linebuf, rlen);
        if (rlen < 1) {
            fclose(fp);
            unlink(data_filename);
            exception = true;
            return;
        }
        left -= rlen;
    }
    fflush(fp);
    if (ferror(fp)) {
        fclose(fp);
        unlink(data_filename);
        exception = true;
        return;
    }
    fclose(fp);
}

}

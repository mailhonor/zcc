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
#include <pthread.h>

namespace zcc
{

static const int ___header_line_max_size = 10240;
class httpd_engine
{
public:
    inline httpd_engine() {}
    inline ~httpd_engine() {} 
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
    list<httpd_upload_file *> request_upload_files;
    /* */
    bool request_keep_alive;
    bool response_initialization;
    bool response_content_type;
    bool get_post_data_myself;
    bool exception;
};

void httpd_upload_file_parse(httpd *hddata, const char *data_filename);

}

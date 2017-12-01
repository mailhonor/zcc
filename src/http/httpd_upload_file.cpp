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

static void ___dump_file(httpd_engine *httpddata, const char *data_filename, std::string &saved_path, std::string &content, int file_id_plus, const char *name, const char *filename)
{
    saved_path = data_filename;
    saved_path.append("_");
    saved_path += file_id_plus;
    if (!file_put_contents(saved_path.c_str(), content.c_str(), content.size())) {
        httpddata->exception = true;
        return;
    }

    httpd_upload_file *huf = (httpd_upload_file *)calloc(1, sizeof(httpd_upload_file));
    huf->name = strdup(name);
    huf->filename = strdup(filename);
    huf->saved_filename = strdup(saved_path.c_str());
    huf->size = content.size();
    httpddata->request_upload_files.push_back(huf);
}

static void ___walk_mime(mail_parser_mime * mime, httpd_engine *httpddata, const char *data_filename, std::string &saved_path, std::string &content, int file_id_plus, std::string &disposition_raw, dict &params)
{
    const char *disposition = mime->disposition();
    if (strncasecmp(disposition, "form-data", 9)) {
        return;
    }
    disposition_raw.clear();
    if(!mime->header_line("content-disposition", disposition_raw)){
        return;
    }
    params.clear();
    content.clear();
    mime_header_line_get_params(disposition_raw.c_str(), disposition_raw.size(), content, params);
    char *name = params.get_str("name");
    tolower(name);
    char *filename = params.get_str("filename");
    const char *ctype = mime->type();
    if (strncasecmp(ctype, "multipart/", 10)) {
        mime->decoded_content(content);
        if (empty(filename)) {
            httpddata->request_post[name]= content;
            return;
        } else {
            ___dump_file(httpddata, data_filename, saved_path, content, file_id_plus, name, filename);
        }
    } else {
        for (mail_parser_mime *mm = mime->child(); mm; mm = mm->next()) {
            ctype = mm->type();
            if (!strncasecmp(ctype, "multipart/", 10)) {
                return;
            }
            disposition_raw.clear();
            filename = blank_buffer;
            if(mime->header_line("content-disposition", disposition_raw)){
                params.clear();
                content.clear();
                mime_header_line_get_params(disposition_raw.c_str(), disposition_raw.size(), content, params);
                filename = params.get_str("filanme");
            }
            ___dump_file(httpddata, data_filename, saved_path, content, file_id_plus, name, filename);
            if (httpddata->exception) {
                return;
            }
        }
    }
}

static void httpd_upload_file_parse_do(httpd_engine *httpddata, const char *data_filename)
{
    std::string saved_path;
    file_mmap mreader;

    if (!mreader.mmap(data_filename)) {
        httpddata->exception = true;
        return;
    }
    mail_parser mparser;
    mparser.parse(mreader.data(), mreader.size());

    const std::list<mail_parser_mime *> &all_mimes = mparser.all_mimes();
    std::string disposition_raw, content;
    dict params;
    int file_id_plus = 0;
    std_list_walk_begin(all_mimes, mime) {
        ___walk_mime(mime, httpddata, data_filename, saved_path, content, file_id_plus++, disposition_raw, params);
        if (httpddata->exception) {
            return;
        }
    } std_list_walk_end;
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
typedef struct parser_linker_t parser_linker_t;
struct parser_linker_t {
    httpd_engine *httpddata;
    const char *data_filename;
    int finished;
    parser_linker_t *prev;
    parser_linker_t *next;
};
parser_linker_t *pl_head = 0;
parser_linker_t *pl_tail = 0;
static bool parse_pthread_mode = false;
static void *___parser_pthread(void *arg)
{
    pthread_detach(pthread_self());
    while(1) {
        zcc_pthread_lock(&mutex);
        while(!pl_head) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (!pl_head) {
            continue;
        }
        parser_linker_t *pl = pl_head;
        zcc_mlink_detach(pl_head, pl_tail, pl, prev, next);
        zcc_pthread_unlock(&mutex);
        httpd_upload_file_parse_do(pl->httpddata, pl->data_filename);
        pl->finished = 1;
    }
    return 0;
}

void httpd_upload_file_parse_pthread()
{
    pthread_t pth;
    pthread_create(&pth, 0, ___parser_pthread, 0);
    parse_pthread_mode = true;
}

void httpd_upload_file_parse(httpd *hddata, const char *data_filename)
{
    httpd_engine *httpddata = (httpd_engine *)(hddata->___data);
    if ((!parse_pthread_mode) || (!coroutine_self())) {
        httpd_upload_file_parse_do(httpddata, data_filename);
        return;
    }

    parser_linker_t pl_buffer, *pl = &pl_buffer;
    pl->httpddata = httpddata;
    pl->data_filename = data_filename;
    pl->finished = 0;
    zcc_pthread_lock(&mutex);
    zcc_mlink_append(pl_head, pl_tail, pl, prev, next);
    zcc_pthread_unlock(&mutex);
    pthread_cond_broadcast(&cond);
    while(1) {
        msleep(1);
        if (pl->finished) {
            break;
        }
    }
    return;
}

}

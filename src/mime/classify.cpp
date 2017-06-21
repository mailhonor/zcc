/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

#define _ZPMT_MULTIPART        1
#define _ZPMT_ATTACHMENT       2
#define _ZPMT_PLAIN            3
#define _ZPMT_HTML             4
static int ___mime_identify_type(mail_parser_mime_inner * mime)
{
    char *type;

    type = mime->type;
    if (empty(type)) {
        return _ZPMT_MULTIPART;
    }
    if (zcc_str_n_eq(type, "multipart/", 10)) {
        return _ZPMT_MULTIPART;
    }
    if (zcc_str_n_eq(type, "application/", 12)) {
        if (strstr(type + 12, "tnef")) {
            mime->is_tnef = true;
        }
        return _ZPMT_ATTACHMENT;
    }
    if ((!empty(mime->disposition)) && (zcc_str_n_eq(mime->disposition, "attachment", 10))) {
        return _ZPMT_ATTACHMENT;
    }
    if (zcc_str_n_eq(type, "image/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (zcc_str_n_eq(type, "audio/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (zcc_str_n_eq(type, "video/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (zcc_str_n_eq(type, "text/", 5)) {
        if (!strcmp(type + 5, "html")) {
            if (strstr(mime->disposition, "attachment")) {
                return _ZPMT_ATTACHMENT;
            }
            return _ZPMT_HTML;
        }
        if (!strcmp(type + 5, "plain")) {
            if (strstr(mime->disposition, "attachment")) {
                return _ZPMT_ATTACHMENT;
            }
            return _ZPMT_PLAIN;
        }
        return _ZPMT_ATTACHMENT;
    }
    if (zcc_str_n_eq(type, "message/", 8)) {
        if (strstr(type + 8, "delivery")) {
            return _ZPMT_PLAIN;
        }
        if (strstr(type + 8, "notification")) {
            return _ZPMT_PLAIN;
        }
        return _ZPMT_ATTACHMENT;
    }

    return _ZPMT_ATTACHMENT;
}

/* ################################################################## */
typedef struct {
    mail_parser_mime_inner *alternative;
    mail_parser_mime_inner *self;
} ___view_mime_t;

static int ___mime_identify_view_part(mail_parser_mime_inner * mime, ___view_mime_t * view_list, int *view_len)
{
    char *type;
    int i, mime_type;
    mail_parser_mime_inner *parent;

    type = mime->type;
    mime_type = mime->mime_type;

    if ((mime_type != _ZPMT_PLAIN) && (mime_type != _ZPMT_HTML)) {
        return 0;
    }
    if (empty(type)) {
        return 0;
    }

    for (parent = mime->parent; parent; parent = parent->parent) {
        if (zcc_str_eq(parent->type, "multipart/alternative")) {
            break;
        }
    }
    if (!parent) {
        view_list[*view_len].alternative = 0;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return 0;
    }

    for (i = 0; i < *view_len; i++) {
        if (view_list[i].alternative == parent) {
            break;
        }
    }

    if (i == *view_len) {
        view_list[*view_len].alternative = parent;
        view_list[*view_len].self = mime;
        *view_len = *view_len + 1;
        return 0;
    }

    view_list[i].alternative = parent;
    if (zcc_str_eq(mime->type, "text/html")) {
        view_list[i].self = mime;
    }

    return 0;
}

void mime_classify(mail_parser_inner * parser)
{
    mail_parser_mime *mw;
    mail_parser_mime_inner *m;
    gm_pool &gmp = *(parser->gmp);
    int i;

    if (parser->classify_flag) {
        return;
    }
    parser->classify_flag = 1;

    do {
        /* classify */
        int type;
        std::vector<mail_parser_mime *>::iterator it = parser->all_mimes.begin();
        for (; it != parser->all_mimes.end(); it++) {
            mw = *it;
            mw->disposition();
            m = mw->get_inner_data();
            type = ___mime_identify_type(m);
            m->mime_type = type;
            if ((type == _ZPMT_PLAIN) || (type == _ZPMT_HTML)) {
                parser->text_mimes.push_back(mw);
            } else if (type == _ZPMT_ATTACHMENT) {
                parser->attachment_mimes.push_back(mw);
            }
        }
    } while(0);

    do {
        /* similar to the above text-mime, 
         * in addition to the case of alternative, html is preferred */
        mime_parser_cache_magic mcm(parser->mcm);
        ___view_mime_t *view_mime = (___view_mime_t *)(mcm.cache->line_cache);
        int view_len = 0;
        for (std::vector<mail_parser_mime *>::iterator it = parser->all_mimes.begin();
                it != parser->all_mimes.end(); it++) {
            mw = *it;
            m = mw->get_inner_data();
            if (view_len < 10000) {
                ___mime_identify_view_part(m,  view_mime, &view_len);
            }
        }

        for (i = 0; i < view_len; i++) {
            parser->show_mimes.push_back(view_mime[i].self->wrap);
        }
    } while(0);
}

}

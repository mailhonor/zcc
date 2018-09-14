/*
 * ================================
 * eli960@qq.com
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
static int ___mime_identify_type(mail_parser_mime_engine * mime)
{
    char *type;

    type = (char *)(mime->type.c_str());
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
    if ((!mime->disposition.empty()) && (zcc_str_n_eq(mime->disposition.c_str(), "attachment", 10))) {
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
            if (strstr(mime->disposition.c_str(), "attachment")) {
                return _ZPMT_ATTACHMENT;
            }
            return _ZPMT_HTML;
        }
        if (!strcmp(type + 5, "plain")) {
            if (strstr(mime->disposition.c_str(), "attachment")) {
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
    mail_parser_mime_engine *alternative;
    mail_parser_mime_engine *self;
} ___view_mime_t;

static int ___mime_identify_view_part(mail_parser_mime_engine * mime, ___view_mime_t * view_list, int *view_len)
{
    char *type;
    int i, mime_type;
    mail_parser_mime_engine *parent;

    type = (char *)(mime->type.c_str());
    mime_type = mime->mime_type;

    if ((mime_type != _ZPMT_PLAIN) && (mime_type != _ZPMT_HTML)) {
        return 0;
    }
    if (empty(type)) {
        return 0;
    }

    for (parent = mime->parent; parent; parent = parent->parent) {
        if (zcc_str_eq(parent->type.c_str(), "multipart/alternative")) {
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
    if (zcc_str_eq(mime->type.c_str(), "text/html")) {
        view_list[i].self = mime;
    }

    return 0;
}

void mime_classify(mail_parser_engine * parser)
{
    if (parser->classify_flag) {
        return;
    }
    parser->classify_flag = 1;

    do {
        /* classify */
        int type;
        std_list_walk_begin(parser->all_mimes_engine, m) {
            m->wrap->disposition();
            type = ___mime_identify_type(m);
            m->mime_type = type;
            if ((type == _ZPMT_PLAIN) || (type == _ZPMT_HTML)) {
                parser->text_mimes.push_back(m->wrap);
            } else if (type == _ZPMT_ATTACHMENT) {
                parser->attachment_mimes.push_back(m->wrap);
            }
        } std_list_walk_end;
    } while(0);

    do {
        /* similar to the above text-mime, 
         * in addition to the case of alternative, html is preferred */
        int view_len_max = parser->all_mimes.size() + 10;
        ___view_mime_t *view_mime = (___view_mime_t *)malloc(sizeof(___view_mime_t) * view_len_max);
        int view_len = 0;
        std_list_walk_begin(parser->all_mimes_engine, m) {
            if (view_len < view_len_max) {
                ___mime_identify_view_part(m,  view_mime, &view_len);
            }
        } std_list_walk_end;

        for (int i = 0; i < view_len; i++) {
            parser->show_mimes.push_back(view_mime[i].self->wrap);
        }
        free(view_mime);
    } while(0);
}

}

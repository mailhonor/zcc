/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-10
 * ================================
 */

#include "zcc.h"

namespace zcc
{

#define _ZPMT_MULTIPART        1
#define _ZPMT_ATTACHMENT       2
#define _ZPMT_PLAIN            3
#define _ZPMT_HTML             4
static int ___mime_identify_type(mail_parser_mime_t * mime)
{
    char *type;

    type = mime->type;
    if (empty(type)) {
        return _ZPMT_MULTIPART;
    }
    if (ZCC_STR_N_EQ(type, "multipart/", 10)) {
        return _ZPMT_MULTIPART;
    }
    if (ZCC_STR_N_EQ(type, "application/", 12)) {
        if (strstr(type + 12, "tnef")) {
            mime->is_tnef = true;
        }
        return _ZPMT_ATTACHMENT;
    }
    if ((!empty(mime->disposition)) && (ZCC_STR_N_EQ(mime->disposition, "attachment", 10))) {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "image/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "audio/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "video/", 6)) {
        return _ZPMT_ATTACHMENT;
    }
    if (ZCC_STR_N_EQ(type, "text/", 5)) {
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
    if (ZCC_STR_N_EQ(type, "message/", 8)) {
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
    mail_parser_mime *alternative;
    mail_parser_mime *self;
} ___view_mime_t;

static int ___mime_identify_view_part(mail_parser_mime_t * mime, mail_parser_mime *mimew
        , ___view_mime_t * view_list, int *view_len)
{
    char *type;
    int i, mime_type;
    mail_parser_mime_t *parent;
    mail_parser_mime *parentw;

    type = mime->type;
    mime_type = mime->mime_type;

    if ((mime_type != _ZPMT_PLAIN) && (mime_type != _ZPMT_HTML)) {
        return 0;
    }
    if (empty(type)) {
        return 0;
    }

    for (parentw = mime->parent; parentw; parentw = parent->parent) {
        parent = (mail_parser_mime_t *)parentw;
        if (ZCC_STR_EQ(parent->type, "multipart/alternative")) {
            break;
        }
    }
    if (!parentw) {
        view_list[*view_len].alternative = 0;
        view_list[*view_len].self = mimew;
        *view_len = *view_len + 1;
        return 0;
    }

    for (i = 0; i < *view_len; i++) {
        if (view_list[i].alternative == parentw) {
            break;
        }
    }

    if (i == *view_len) {
        view_list[*view_len].alternative = parentw;
        view_list[*view_len].self = mimew;
        *view_len = *view_len + 1;
        return 0;
    }

    view_list[i].alternative = parentw;
    if (ZCC_STR_EQ(mime->type, "text/html")) {
        view_list[i].self = mimew;
    }

    return 0;
}

void mime_classify(mail_parser_t * parser)
{
    mail_parser_mime *mw;
    mail_parser_mime_t *m;
    mem_pool &mpool = *(parser->mpool);
    int i;

    if (parser->classify_flag) {
        return;
    }
    parser->classify_flag = 1;

    {
        /* classify */
        int type;
        mail_parser_mime *text_mime[10240];
        int text_len = 0;
        mail_parser_mime *att_mime[10240];
        int att_len = 0;

        zcc_vector_walk_begin(*parser->all_mimes, mw) {
            m = (mail_parser_mime_t *)mw;
            type = ___mime_identify_type(m);
            m->mime_type = type;
            if ((type == _ZPMT_PLAIN) || (type == _ZPMT_HTML)) {
                if (text_len < 10000) {
                    text_mime[text_len++] = mw;
                }
            } else if (type == _ZPMT_ATTACHMENT) {
                if (att_len < 10000) {
                    att_mime[att_len++] = mw;
                }
            }
        } zcc_vector_walk_end;

        parser->text_mimes = new(mpool.calloc(1, sizeof(vector<void *>)))vector<mail_parser_mime *>;
        parser->text_mimes->init(text_len, mpool);
        for (i = 0; i < text_len; i++) {
            parser->text_mimes->push_back(text_mime[i]);
        }

        parser->attachment_mimes = new(mpool.calloc(1, sizeof(vector<void *>)))vector<mail_parser_mime *>;
        for (i = 0; i < att_len; i++) {
            parser->attachment_mimes->push_back(att_mime[i]);
        }
    }

    {
        /* similar to the above text-mime, 
         * in addition to the case of alternative, html is preferred */
        ___view_mime_t view_mime[10240];
        int view_len = 0;
        zcc_vector_walk_begin(*parser->all_mimes, mw) {
            m = (mail_parser_mime_t *)mw;
            if (view_len < 10000) {
                ___mime_identify_view_part(m, (mail_parser_mime *)m,  view_mime, &view_len);
            }
        } zcc_vector_walk_end;

        parser->show_mimes = new(mpool.calloc(1, sizeof(vector<void *>)))vector<mail_parser_mime *>;
        for (i = 0; i < view_len; i++) {
            parser->show_mimes->push_back(view_mime[i].self);
        }
    }
}

}

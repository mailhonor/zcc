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

mail_parser_mime::mail_parser_mime(mail_parser_t *parser)
{
    memset(&td, 0, sizeof(mail_parser_mime_t));
    td.parser = parser;
}

mail_parser_mime::~mail_parser_mime()
{
#define ___ha(a)	{if(td.a) td.parser->mpool->free(td.a);}
    ___ha(type);
    ___ha(encoding);
    ___ha(charset);
    ___ha(disposition);
    ___ha(boundary);
    ___ha(name);
    ___ha(filename);
    ___ha(filename2231);
    ___ha(name_utf8);
    ___ha(filename_utf8);
    ___ha(content_id);
    ___ha(imap_section);
#undef ___ha

    /* header */
    if (td.header_lines) {
        for (size_t i = 0; i < td.header_lines->size(); i ++) {
            td.parser->mpool->free((*td.header_lines)[i]);
        }
        td.header_lines->~vector();
        td.parser->mpool->free(td.header_lines);
    }
}

/* ################################################################## */
mail_parser::mail_parser(mem_pool &mpool)
{
    memset(&td, 0, sizeof(mail_parser_t));
    td.mpool = &mpool;

    td.mime_max_depth = 5;

    td.top_mime = new(mpool.calloc(1, sizeof(mail_parser_mime))) mail_parser_mime(&td);
    td.all_mimes = new vector<mail_parser_mime *>;
    td.all_mimes->init(128);
    td.all_mimes->push_back(td.top_mime);

    td.tmp_header_lines = new vector<size_data_t *>;
    td.tmp_header_lines->init(128);
}

mail_parser &mail_parser::option_mime_max_depth(size_t depth)
{
    if (depth > 10) {
        depth = 10;
    } 
    if (depth < 1) {
        depth = 1;
    }
    td.mime_max_depth = depth;
    return *this;
}

mail_parser &mail_parser::option_src_charset_def(const char *src_charset_def)
{
    if (src_charset_def) {
        snprintf(td.src_charset_def, 31, "%s", src_charset_def);
    }
    return *this;
}

mail_parser &mail_parser::parse(const char *mail_data, size_t mail_data_len)
{
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 16];
    td.mail_data = (char *)mail_data;
    td.mail_pos = (char *)mail_data;
    td.mail_size = (int)mail_data_len;
    int ___mail_decode_mime(mail_parser_t *, mail_parser_mime_t *, mail_parser_mime_t *, char *);
    ___mail_decode_mime(&td, 0, (mail_parser_mime_t *)(td.top_mime), buf);
    return *this;
}

mail_parser::~mail_parser()
{
    mem_pool &mp = *(td.mpool);

#define ___fh(a)	{if(td.a) mp.free(td.a);}
#define ___ft(a)	{if(td.a) mime_free_address(td.a, mp);}
#define ___ftv(a)	{if(td.a){ \
    for(size_t i=0;i<td.a->size();i++){ mime_free_address((*(td.a))[i],mp);} td.a->~vector(); mp.free(td.a); \
}}
    ___fh(subject);
    ___fh(subject_utf8);
    ___fh(date);
    ___ft(from);
    ___ft(sender);
    ___ft(reply_to);
    ___ftv(to);
    ___ftv(cc);
    ___ftv(bcc);
    ___fh(in_reply_to);
    ___fh(message_id);
    ___ft(receipt);
#undef ___fh
#undef ___ft
#undef ___ftv

    /* references */
    if (td.references) {
        for (size_t i=0;i<td.references->size();i++) {
            mp.free((*td.references)[i]);
        }
        td.references->~vector();
        mp.free(td.references);
    }

    /* mime */
    for (size_t i=0;i<td.all_mimes->size();i++){
        mail_parser_mime *m = (*td.all_mimes)[i];
        m->~mail_parser_mime();
        mp.free(m);
    }
    delete td.all_mimes;

    if (td.text_mimes) {
        td.text_mimes->~vector();
        mp.free(td.text_mimes);
    }
    if (td.show_mimes) {
        td.show_mimes->~vector();
        mp.free(td.show_mimes);
    }
    if (td.attachment_mimes) {
        td.attachment_mimes->~vector();
        mp.free(td.attachment_mimes);
    }

    delete td.tmp_header_lines;
}

/* ################################################################## */
char *mail_parser_mime::wrap_get(int module)
{
    mail_parser_mime_t *mime = &td;
    mail_parser_t *parser = mime->parser;
    mem_pool &mpool = *(parser->mpool);
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 10], *val;
    size_t blen, vlen;
    ZCC_STACK_STRING(zb, ZMAIL_HEADER_LINE_MAX_LENGTH);
    ZCC_STACK_STRING(zb2, 1024 * 100);
    vector<size_data_t *> &hls = *(mime->header_lines);
    size_data_t *sd;

    if (module == 1) {
        if (!mime->encoding) {
            mime->encoding = blank_buffer;
            for (size_t i = 0; i < hls.size(); i++) {
                sd = const_cast<size_data_t *>(hls[i]);
                if ((sd->size>25) && (!strncasecmp(sd->data, "Content-Transfer-Encoding:", 26))) {
                    blen = mime_unescape(sd->data + 26, sd->size - 26, buf);
                    buf[blen] = 0;
                    mime_decode_content_transfer_encoding(buf, blen, &val, &vlen);
                    if (vlen > 0) {
                        mime->encoding = mpool.memdupnull(val, vlen);
                        to_lower(mime->encoding);
                    }
                    break;
                }
            }
        }
        return mime->encoding;
    } else if ((module == 2) || (module == 4) || (module == 5) || (module == 6)) {
        if (!mime->disposition) {
            char *fn; size_t fnl;
            mime->disposition = blank_buffer;
            mime->filename = blank_buffer;
            mime->filename2231 = blank_buffer;
            for (size_t i = 0; i < hls.size(); i++) {
                sd = const_cast<size_data_t *>(hls[i]);
                if ((sd->size>19) && (!strncasecmp(sd->data, "Content-Disposition:", 20))) {
                    blen = mime_unescape(sd->data + 20, sd->size - 20, buf);
                    buf[blen] = 0;
                    bool with_charset;
                    mime_decode_content_disposition(buf, blen, &val, &vlen, &fn, &fnl, &zb2, &with_charset);
                    if (vlen > 0) {
                        mime->disposition = mpool.memdupnull(val, vlen);
                        to_lower(mime->disposition);
                    }
                    if (fnl > 0) {
                        mime->filename = mpool.memdupnull(fn, fnl);
                    }
                    if (zb2.size()) {
                        mime->filename2231 = mpool.memdupnull(zb2.c_str(), zb2.size());
                        mime->filename2231_with_charset = with_charset;
                    }
                    break;
                }
            }
        }
        if ((module == 6) && (!mime->filename_utf8)) {
            mime->filename_utf8 = blank_buffer;
            if (*(mime->filename2231)) {
                zb2.clear();
                mime_get_utf8_2231(parser->src_charset_def, mime->filename2231, strlen(mime->filename2231), zb2, mime->filename2231_with_charset);
                mime->filename_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size());
            }
            if ((!*(mime->filename_utf8)) && (*(mime->filename))) {
                zb2.clear();
                mime_get_utf8(parser->src_charset_def, mime->filename, strlen(mime->filename), zb2);
                mime->filename_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size());
            }
        }
        if (module == 2) {
            return mime->disposition;
        } else if (module == 4) {
            return mime->filename;
        } else if (module == 5) {
            return mime->filename2231;
        } else if (module == 6) {
            return mime->filename_utf8;
        }
    } else if (module == 3) {
        if (!mime->name_utf8) {
            mime->name_utf8 = blank_buffer;
            mime_get_utf8(parser->src_charset_def, mime->name, strlen(mime->name), zb2);
            mime->name_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size());
        }
        return mime->name_utf8;
    } else if (module == 7) {
        if (!mime->content_id) {
            mime->content_id = blank_buffer;
            for (size_t i = 0; i < hls.size(); i++) {
                sd = const_cast<size_data_t *>(hls[i]);
                if ((sd->size>10) && (!strncasecmp(sd->data, "Content-ID:", 11))) {
                    blen = mime_unescape(sd->data + 11, sd->size - 11, buf);
                    buf[blen] = 0;
                    mime_get_first_token(buf, blen, &val, &vlen);
                    if (vlen > 0) {
                        mime->content_id = mpool.memdupnull(val, vlen);
                    }
                    break;
                }
            }
        }
        return mime->content_id;
    } else if (module == 8) {
        if (!mime->show_name) {
            mime->show_name = blank_buffer;
            const char *n;
            n = name_utf8();
            if (n == blank_buffer) {
                n = filename_utf8();
            }
            if (n == blank_buffer) {
                n = filename();
            }
            if (n == blank_buffer) {
                n = name();
            }
            if (n == blank_buffer) {
                n = filename2231();
            }
            mime->show_name = const_cast<char *>(n);
        }
        return mime->show_name;
    }

    return blank_buffer;
}

bool mail_parser_mime::header_line(const char *header_name, vector<const size_data_t *> &vec)
{
    size_t i, hv = 0, name_len;
    size_data_t *sd;

    name_len = strlen(header_name);
    if (name_len == 0) {
        return false;
    }
    if (header_name[name_len-1] == ':') {
        name_len --;
    }
    if (name_len == 0) {
        return false;
    }

    for (i=0; i<td.header_lines->size(); i++) {
        sd = const_cast<size_data_t *>((*td.header_lines)[i]);
        if (sd->size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':') {
            continue;
        }
        if (!strncasecmp(data, header_name, name_len)) {
            continue;
        }
        vec.push_back(sd);
        hv = 1;
    }

    if (hv) {
        return true;
    }
    return false;
}

bool mail_parser_mime::header_line(const char *header_name, char **result, size_t *result_size, int n)
{
    int i, sni = -1, last_sn = -100, last_i = -100;
    size_t name_len;
    size_data_t *sd;

    name_len = strlen(header_name);
    if (name_len == 0) {
        return false;
    }
    if (header_name[name_len-1] == ':') {
        name_len --;
    }
    if (name_len == 0) {
        return false;
    }

    for (i=0; i<(int)td.header_lines->size(); i++) {
        sd = const_cast<size_data_t *>((*td.header_lines)[i]);
        if (sd->size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':') {
            continue;
        }
        if (strncasecmp(data, header_name, name_len)) {
            continue;
        }
        sni++;
        last_sn = sni;
        last_i = i;
        if (last_sn == n) {
            break;
        }
    }
    if (last_sn == -100) {
        return false;
    }
    if ((n == -1) || (last_sn==n)) {
        sd = const_cast<size_data_t *>((*td.header_lines)[last_i]);
        *result = sd->data;
        *result_size = sd->size;
        return true;
    }

    return false;
}

bool mail_parser_mime::header_line(const char *header_name, string &result, int n)
{
    char *data;
    size_t len;

    result.clear();
    if (header_line(header_name, &data, &len, n)) {
        result.append(data, len);
        return true;
    }
    return false;
}

void mail_parser_mime::decoded_content(string &dest)
{
    mail_parser_mime_t *m = &td;
    mail_parser_t *parser = m->parser;
    char *in_src = parser->mail_data + m->body_offset;
    int in_len = m->body_len;
    const char *enc = encoding();

    dest.clear();
    if (!empty(enc)) {
        if (!strcmp(enc, "base64")) {
            base64_decode(in_src, in_len, dest);
            return;
        } else if (!strcmp(enc, "quoted-printable")) {
            qp_decode_2045(in_src, in_len, dest);
            return;
        }
    }
    dest.append(in_src, in_len);
}

void mail_parser_mime::decoded_content_utf8(string &dest)
{
    mail_parser_mime_t *m = &td;
    mail_parser_t *parser = m->parser;
    int bq = 0;
    char *in_src = parser->mail_data + m->body_offset;
    int in_len = m->body_len;
    const char *enc = encoding();
    const char *cha = charset();
    string *zbuf3 = 0;
    char *buf3;
    int buf3_len;

    dest.clear();
    if (!*enc) {
        bq = 0;
    } else if (!strcmp(enc, "base64")) {
        bq = 'B';
    } else if (!strcmp(enc, "quoted-printable")) {
        bq = 'Q';
    } else {
        bq = 0;
    }

    if (bq) {
        zbuf3 = new string(m->body_len>10240?m->body_len:10240);
        if (bq == 'B') {
            buf3_len = base64_decode(in_src, in_len, *zbuf3);
        } else {
            buf3_len = qp_decode_2045(in_src, in_len, *zbuf3);
        }
        buf3 = zbuf3->c_str();
        buf3_len = zbuf3->size();
    } else {
        buf3 = in_src;
        buf3_len = in_len;
    }

    mime_iconv((*cha)?cha:parser->src_charset_def, buf3, buf3_len, dest);
    if (zbuf3) {
        delete zbuf3;
    }
}

/* ################################################################## */
static size_t ___get_header_value(mail_parser_mime *mime, const char *name, char *buf, char **start)
{
    char *data;
    size_t rlen, blen = strlen(name);
    if (mime->header_line(name, &data, &rlen)){
        rlen = mime_unescape(data + blen, rlen - blen, buf);
        buf[rlen] = 0;
        return skip(buf, rlen, " \t", 0, start);
    }
    return 0;
}

char *mail_parser::wrap_get(int module)
{
    mail_parser_t *parser = &td;
    mail_parser_mime *mime = parser->top_mime;
    mem_pool &mpool = *(parser->mpool);
    char buf[ZMAIL_HEADER_LINE_MAX_LENGTH + 10], *val, *start;
    size_t blen, vlen;
    ZCC_STACK_STRING(zb, ZMAIL_HEADER_LINE_MAX_LENGTH);
    ZCC_STACK_STRING(zb2, 1024 * 100);
    
    if (module == 13) {
#define ___mail_hval_first_token(a, b) \
        if (!parser->a) { \
            parser->a = blank_buffer; \
            blen = ___get_header_value(mime, b, buf, &start); \
            mime_get_first_token(start, blen, &val, &vlen); \
            if (vlen > 0) { \
                parser->a = mpool.memdupnull(val, vlen); \
            } \
        } \
        return parser->a; 
        ___mail_hval_first_token(message_id, "message-id:");
    } else if((module == 1) || (module == 2)) {
        if (!parser->subject) {
            parser->subject = blank_buffer;
            blen = ___get_header_value(mime, "subject:", buf, &start);
            if (blen > 0) {
                parser->subject = mpool.memdupnull(start, blen);
            }
        }
        if ((module == 2) && (!parser->subject_utf8)) {
            parser->subject_utf8 = blank_buffer;
            mime_get_utf8(parser->src_charset_def, parser->subject, strlen(parser->subject), zb2);
            parser->subject_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size());
        }
        if (module == 1) {
            return parser->subject;
        } else {
            return parser->subject_utf8;
        }
    } else if((module == 3) || (module == 4)) {
        if (!parser->date) {
            parser->date = blank_buffer;
            blen = ___get_header_value(mime, "date:", buf, &start);
            if (blen > 0) {
                parser->date = mpool.memdupnull(start, blen);
            }
        }
        if (module == 4) {
            parser->date_unix = mime_decode_date(parser->date);
        }
        return parser->date;
    } else if((module == 5) || (module == 105)) {
        if (!parser->from_flag) {
            parser->from_flag = 1;
            blen = ___get_header_value(mime, "from:", buf, &start);
            if (blen > 0) {
                mime_address_parser ap;
                ap.parse(start, blen);
                zb.clear(); zb2.clear();
                if (ap.shift(zb, zb2)) {
                    parser->from = (mime_address_t *)mpool.calloc(1, sizeof(mime_address_t));
                    parser->from->name = mpool.memdupnull(zb.c_str(), zb.size());
                    parser->from->address = mpool.memdupnull(zb2.c_str(), zb2.size());
                }
            }
        }
        if ((module == 105) && (parser->from_flag !=2 )) {
            parser->from_flag = 2;
            if (parser->from) {
                mime_get_utf8(parser->src_charset_def, parser->from->name, strlen(parser->from->name), zb2);
                parser->from->name_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size());
            }
        }
        return blank_buffer;
    } else if(module == 6) {
#define ___mail_noutf8(a, b, c) \
        if (!parser->a) { \
            parser->a = 1; \
            blen = ___get_header_value(mime, b, buf, &start); \
            if (blen > 0) { \
                mime_address_parser ap; \
                ap.parse(start, blen);  \
                zb.clear(); zb2.clear(); \
                if (ap.shift(zb, zb2)) { \
                    parser->c = (mime_address_t *)mpool.calloc(1, sizeof(mime_address_t)); \
                    parser->c->name = mpool.memdupnull(zb.c_str(), zb.size()); \
                    parser->c->address = mpool.memdupnull(zb2.c_str(), zb2.size()); \
                } \
            } \
        } \
        return blank_buffer;
        ___mail_noutf8(sender_flag, "sender:", sender);
    } else if(module == 7) {
        ___mail_noutf8(reply_to_flag, "reply-to:", reply_to);
    } else if(module == 11) {
        ___mail_noutf8(receipt_flag, "disposition-notification-to:", receipt);
    } else if(module == 12) {
        ___mail_hval_first_token(in_reply_to, "in-reply-to:");
    } else if((module == 8) || (module==108)) {
#define ___mail_parser_tcb_(tcb_flag, tcbname, tcb, md)  \
        if (!parser->tcb_flag) {  \
            parser->tcb_flag = 1; \
            blen = ___get_header_value(mime, tcbname, buf, &start); \
            vector<mime_address_t *> vec; vec.init(128); \
            mime_get_address_vector(start, blen, vec, mpool); \
            parser->tcb = new(mpool.calloc(1, sizeof(vector<mime_address_t *>)))vector<mime_address_t *>; \
            parser->tcb->init(vec.size(), mpool); \
            for (size_t i=0;i<vec.size();i++) { \
                parser->tcb->push_back(vec[i]); \
            } \
        } \
        if ((module==md) && (parser->tcb_flag!=2)) { \
            parser->tcb_flag = 2; \
            mime_address_t *addr; \
            for (size_t i=0;i<parser->tcb->size();i++) { \
                addr = (*parser->tcb)[i]; \
                zb2.clear(); \
                mime_get_utf8(parser->src_charset_def, addr->name, strlen(addr->name), zb2); \
                addr->name_utf8 = mpool.memdupnull(zb2.c_str(), zb2.size()); \
            } \
        } \
        return blank_buffer;
        ___mail_parser_tcb_(to_flag, "to:", to, 108);
    } else if((module == 9) || (module==109)) {
        ___mail_parser_tcb_(cc_flag, "cc:", cc, 109);
    } else if((module == 10) || (module==110)) {
        ___mail_parser_tcb_(bcc_flag, "bcc:", bcc, 110);
    } else if(module == 14) {
        if (!parser->references_flag) {
            parser->references_flag = 1;
            blen = ___get_header_value(mime, "references:", buf, &start);
            if (blen < 1) {
                return blank_buffer;
            }
            start[blen] = 0;
            vector<char *> rr;
            rr.init(128);
            strtok splitor;
            char *mid;
            splitor.set_str(start);
            while (splitor.tok("<> \t,\r\n")) {
                mid = (char *)mpool.memdupnull(splitor.ptr(), splitor.size());
                rr.push_back(mid);
            }
            if (rr.size()) {
                parser->references = new(mpool.calloc(1, sizeof(vector<char *>))) vector<char *>;
                parser->references->init(rr.size(), mpool);
                for (size_t i=0;i<rr.size();i++) {
                    parser->references->push_back(rr[i]);
                }
            }
        }
        return blank_buffer;
    } else if(module == 61) {
        void mime_set_imap_section(mail_parser_t * parser);
        mime_set_imap_section(parser);
    } else if(module == 62) {
        void mime_classify(mail_parser_t * parser);
        mime_classify(parser);
    }
    return blank_buffer;
}

}

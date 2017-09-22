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

static size_t ___get_header_value(mail_parser_mime *mime, const char *name, char *buf, char **start)
{
    char *data;
    size_t rlen, blen = strlen(name);
    if ((rlen = mime->header_line(name, &data))){
        rlen = mime_header_line_unescape(data + blen, rlen - blen, buf, var_mime_header_line_max_length);
        buf[rlen] = 0;
        rlen = skip(buf, rlen, " \t", 0, start);
        return rlen;
    }
    return 0;
}

#define ___get_buf_mime() (___data->parser->mcm.cache->line_cache)
#define ___get_buf() (___data->mcm.cache->line_cache)

mail_parser_mime::mail_parser_mime(mail_parser_inner *parser)
{
    ___data = (mail_parser_mime_inner *)parser->gmp->calloc(1, sizeof(mail_parser_mime_inner));
    ___data->parser = parser;
    ___data->wrap = this;
}

mail_parser_mime::~mail_parser_mime()
{
    ___data->~mail_parser_mime_inner();
}

const char *mail_parser_mime::type()
{
    return ___data->type;
}

const char *mail_parser_mime::encoding()
{
    if (___data->encoding) {
        return ___data->encoding;
    }
    ___data->encoding = blank_buffer;
    char *buf = ___get_buf_mime(), *start;
    size_t blen = ___get_header_value(___data->wrap, "Content-Transfer-Encoding:", buf, &start);
    if (blen) {
        char *val;
        size_t vlen;
        mime_header_line_decode_content_transfer_encoding(start, blen, &val, &vlen);
        if (vlen > 0) {
            ___data->encoding = ___data->parser->gmp->memdupnull(val, vlen);
            tolower(___data->encoding);
        }
    }
    return ___data->encoding;
}

const char *mail_parser_mime::charset()
{
    return ___data->charset;
}

const char *mail_parser_mime::disposition()
{
    if (___data->disposition) {
        return ___data->disposition;
    }
    ___data->disposition = blank_buffer;
    ___data->filename = blank_buffer;
    ___data->filename2231 = blank_buffer;
    char *buf = ___get_buf_mime(), *start;
    size_t blen = ___get_header_value(___data->wrap, "Content-Disposition:", buf, &start);
    if (blen) {
        mime_parser_cache_magic mcm(___data->parser->mcm);
        char *val, *fn;
        size_t vlen, f_len;
        string &fn2231 = mcm.require_string();
        bool with_charset;
        fn2231.clear();
        mime_header_line_decode_content_disposition(start, blen, &val, &vlen, &fn, &f_len, fn2231, &with_charset);
        if (vlen > 0) {
            ___data->disposition = ___data->parser->gmp->memdupnull(val, vlen);
            tolower(___data->disposition);
        }
        if (f_len > 0) {
            ___data->filename = ___data->parser->gmp->memdupnull(fn, f_len);
        }
        if (!fn2231.empty()) {
            ___data->filename2231 = ___data->parser->gmp->memdupnull(fn2231.c_str(), fn2231.size());
            ___data->filename2231_with_charset = with_charset;
        }
    }
    return ___data->disposition;
}

const char *mail_parser_mime::show_name()
{
    if (!___data->show_name) {
        ___data->show_name = blank_buffer;
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
        ___data->show_name = const_cast<char *>(n);
    }
    return ___data->show_name;
}

const char *mail_parser_mime::name()
{
    return ___data->name;
}

const char *mail_parser_mime::name_utf8()
{
    if (___data->name_utf8) {
        return ___data->name_utf8;
    }
    ___data->name_utf8 = blank_buffer;
    mime_parser_cache_magic mcm(___data->parser->mcm);
    string &uname = mcm.require_string();
    uname.clear();
    mcm.true_data = ___data->name;
    mime_header_line_get_utf8(___data->parser->src_charset_def, (char *)(&mcm), strlen(___data->name), uname);
    ___data->name_utf8 = ___data->parser->gmp->memdupnull(uname.c_str(), uname.size());
    return ___data->name_utf8;
}

const char *mail_parser_mime::filename()
{
    if (___data->filename) {
        return ___data->filename;
    }
    disposition();
    return ___data->filename;
}

const char *mail_parser_mime::filename_utf8()
{
    if (___data->filename_utf8) {
        return ___data->filename_utf8;
    }
    if (!___data->filename) {
        disposition();
    }
    ___data->filename_utf8 = blank_buffer;
    mime_parser_cache_magic mcm(___data->parser->mcm);
    string &uname = mcm.require_string();
    uname.clear();
    if (*(___data->filename2231)) {
        mcm.true_data = ___data->filename2231;
        mime_header_line_get_utf8_2231(___data->parser->src_charset_def, (char *)(&mcm), strlen(___data->filename2231), uname, ___data->filename2231_with_charset);
        ___data->filename_utf8 = ___data->parser->gmp->memdupnull(uname.c_str(), uname.size());
    }
    if ((!*(___data->filename_utf8)) && (*(___data->filename))) {
        mcm.true_data = ___data->filename;
        mime_header_line_get_utf8(___data->parser->src_charset_def, (char *)&mcm, strlen(___data->filename), uname);
        ___data->filename_utf8 = ___data->parser->gmp->memdupnull(uname.c_str(), uname.size());
    }
    return ___data->filename_utf8;
}

const char *mail_parser_mime::filename2231()
{
    if (___data->filename2231) {
        return ___data->filename2231;
    }
    disposition();
    return ___data->filename2231;
}

bool mail_parser_mime::filename2231_with_charset()
{
    if (___data->filename2231) {
      return ___data->filename2231_with_charset;
    }
    disposition();
    return ___data->filename2231_with_charset;
}

const char *mail_parser_mime::content_id()
{
    if (___data->content_id) {
        return ___data->content_id;
    }
    ___data->content_id = blank_buffer;
    char *buf = ___get_buf_mime(), *start, *val;
    size_t blen = ___get_header_value(___data->wrap, "Content-ID:", buf, &start), vlen;
    if (blen) {
        vlen = mime_header_line_get_first_token(start, blen, &val);
        if (vlen > 0) {
            ___data->content_id = (char *)___data->parser->gmp->memdupnull(val, vlen);
        }
    }
    return ___data->content_id;
}

const char *mail_parser_mime::boundary()
{
    return ___data->boundary;
}

const char *mail_parser_mime::imap_section()
{
    void mime_set_imap_section(mail_parser_inner *);
    if (!___data->imap_section) {
        ___data->imap_section = blank_buffer;
        mime_set_imap_section(___data->parser);
    }
    return ___data->imap_section;
}

size_t mail_parser_mime::header_offset()
{
    return ___data->header_offset;
}

size_t mail_parser_mime::header_size()
{
    return ___data->header_len;
}
size_t mail_parser_mime::body_offset()
{
    return ___data->body_offset;
}

size_t mail_parser_mime::body_size()
{
    return ___data->body_len;
}

bool mail_parser_mime::tnef() 
{
    return ___data->is_tnef;
}

mail_parser_mime * mail_parser_mime::next()
{
    return ___data->next?___data->next->wrap:0;
}

mail_parser_mime * mail_parser_mime::child()
{
    return ___data->child?___data->child->wrap:0;
}

mail_parser_mime * mail_parser_mime::parent() 
{
    return ___data->parent?___data->parent->wrap:0;
}


const vector<size_data_t *> &mail_parser_mime::header_line()
{
    return ___data->header_lines;
}

bool mail_parser_mime::header_line(const char *header_name, vector<const size_data_t *> &vec)
{
    int firstch;
    size_t hv = 0, name_len;

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

    firstch = toupper(header_name[0]);
    zcc_vector_walk_begin(___data->header_lines, sd) {
        if (sd->size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':') {
            continue;
        }
        if (firstch != toupper(data[0])) {
            continue;
        }
        if (!strncasecmp(data, header_name, name_len)) {
            continue;
        }
        vec.push_back(sd);
        hv = 1;
    } zcc_vector_walk_end;

    if (hv) {
        return true;
    }
    return false;
}

size_t mail_parser_mime::header_line(const char *header_name, char **result, int n)
{
    int firstch, nn = 0;
    size_t name_len;
    size_data_t *sdn = 0;

    name_len = strlen(header_name);
    if (name_len == 0) {
        return 0;
    }
    if (header_name[name_len-1] == ':') {
        name_len --;
    }
    if (name_len == 0) {
        return 0;
    }

    firstch = toupper(header_name[0]);
    zcc_vector_walk_begin(___data->header_lines, sd) {
        if (sd->size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd->data);
        if (data[name_len] != ':') {
            continue;
        }
        if (firstch != toupper(data[0])) {
            continue;
        }
        if (strncasecmp(data, header_name, name_len)) {
            continue;
        }
        if (nn == n) {
            *result = sd->data;
            return sd->size;
        }
        nn ++;
        sdn = sd;
    } zcc_vector_walk_end;
    if (!sdn) {
        return 0;
    }
    if (n != -1) {
        return 0;
    }

    *result = sdn->data;
    return sdn->size;
}

bool mail_parser_mime::header_line(const char *header_name, string &result, int n)
{
    char *data;
    size_t len;

    result.clear();
    if ((len = header_line(header_name, &data, n))) {
        result.append(data, len);
        return true;
    }
    return false;
}

void mail_parser_mime::decoded_content(string &dest)
{
    mail_parser_inner *parser = ___data->parser;
    char *in_src = parser->mail_data + ___data->body_offset;
    int in_len = ___data->body_len;
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
    mail_parser_inner *parser = ___data->parser;
    char *in_src = parser->mail_data + ___data->body_offset;
    int in_len = ___data->body_len;
    const char *enc = encoding();
    const char *cha = charset();
    const char *str;
    size_t str_len;
    mime_parser_cache_magic mcm(___data->parser->mcm);
    string &tmp_cache_buf = mcm.require_string();

    dest.clear();
    tmp_cache_buf.clear();

    if (!strcmp(enc, "base64")) {
        str_len = base64_decode(in_src, in_len, tmp_cache_buf);
        str = tmp_cache_buf.c_str();
        str_len = tmp_cache_buf.size();
    } else if (!strcmp(enc, "quoted-printable")) {
        str_len = qp_decode_2045(in_src, in_len, tmp_cache_buf);
        str = tmp_cache_buf.c_str();
        str_len = tmp_cache_buf.size();
    } else {
        str = in_src;
        str_len = in_len;
    }

    mime_iconv((*cha)?cha:parser->src_charset_def, str, str_len, dest);
}


/* ################################################################## */
mail_parser::mail_parser()
{
    gm_pool *gmp = new gm_pool();
    gmp->option_buffer_size(1024 * 1024);
    ___data = new (gmp->calloc(1, sizeof(mail_parser_inner))) mail_parser_inner();
    ___data->mcm.gmp = gmp;
    ___data->gmp = gmp;

    ___data->date_unix = -1;

    ___data->mime_max_depth = 5;
    ___data->top_mime = new(gmp->calloc(1, sizeof(mail_parser_mime))) mail_parser_mime(___data);
    ___data->all_mimes.push_back(___data->top_mime);
}

void mail_parser::option_mime_max_depth(size_t depth)
{
    if (depth > 10) {
        depth = 10;
    } 
    if (depth < 1) {
        depth = 1;
    }
    ___data->mime_max_depth = depth;
}

void mail_parser::option_src_charset_def(const char *src_charset_def)
{
    if (src_charset_def) {
        snprintf(___data->src_charset_def, 31, "%s", src_charset_def);
    }
}

void mail_parser::parse(const char *mail_data, size_t mail_data_len)
{
    char *buf = ___data->mcm.cache->line_cache;
    ___data->mail_data = (char *)mail_data;
    ___data->mail_pos = (char *)mail_data;
    ___data->mail_size = mail_data_len;
    int ___mail_decode_mime(mail_parser_inner *, mail_parser_mime_inner *, mail_parser_mime_inner *, char *);
    ___mail_decode_mime(___data, 0, ___data->top_mime->get_inner_data(), buf);
}

mail_parser::~mail_parser()
{
    gm_pool *gmp = ___data->gmp;

    /* from, sender, ... no need to release */

    /* to, cc, bcc */
    zcc_vector_walk_begin(___data->to, m) {
        m->~mime_address(); /* free(*it); */
    } zcc_vector_walk_end;
    zcc_vector_walk_begin(___data->cc, m) {
        m->~mime_address(); /* free(*it); */
    } zcc_vector_walk_end;
    zcc_vector_walk_begin(___data->bcc, m) {
        m->~mime_address(); /* free(*it); */
    } zcc_vector_walk_end;

    /* references, no need to release */

    /* mime */
    zcc_vector_walk_begin(___data->all_mimes, m) {
        m->~mail_parser_mime();
        /* free(m); */
    } zcc_vector_walk_end;
    
    ___data->~mail_parser_inner();
    /* free(___data) */

    delete gmp;
}

const char *mail_parser::data()
{
    return ___data->mail_data;
}

size_t mail_parser::size()
{
    return ___data->mail_size;
}

size_t mail_parser::header_size()
{
    return ___data->top_mime->header_size();
}

size_t mail_parser::body_offset()
{
    return ___data->top_mime->body_offset();
}

size_t mail_parser::body_size()
{
    return ___data->top_mime->body_size();
}

const char *mail_parser::message_id()
{
#define ___mail_hval_first_token(a, b) \
    if (!___data->a) { \
        ___data->a = blank_buffer; \
        char *buf = ___get_buf(), *start, *val; \
        size_t blen = ___get_header_value(___data->top_mime, b, buf, &start), vlen; \
        vlen = mime_header_line_get_first_token(start, blen, &val); \
        if (vlen > 0) { \
            ___data->a = ___data->gmp->memdupnull(val, vlen); \
        } \
    } \
    return ___data->a; 
    ___mail_hval_first_token(message_id, "message-id:");
}

const char *mail_parser::subject()
{
#define ___mail_hval_skip_left(a, b) \
    if (!___data->a) { \
        ___data->a = blank_buffer; \
        char *buf = ___get_buf(), *start; \
        size_t blen = ___get_header_value(___data->top_mime, b, buf, &start); \
        while ((blen > 0)) { \
            if (*start ==' ' || *start == '\t') { \
                start ++; \
                blen --; \
                continue; \
            } \
            break; \
        } \
        if (blen > 0) { \
            ___data->a = ___data->gmp->memdupnull(start, blen); \
        } \
    } \
    return ___data->a; 
    ___mail_hval_skip_left(subject, "Subject:");
}

const char *mail_parser::subject_utf8()
{
    if (!___data->subject) {
        subject();
    }
    if (!___data->subject_utf8) {
        ___data->subject_utf8 = blank_buffer;
        mime_parser_cache_magic mcm(___data->mcm);
        mcm.true_data = ___data->subject;
        string &uname = mcm.require_string();
        uname.clear();
        mime_header_line_get_utf8(___data->src_charset_def, (char *)(&mcm), strlen(___data->subject), uname);
        ___data->subject_utf8 = ___data->gmp->memdupnull(uname.c_str(), uname.size());
    }
    return ___data->subject_utf8;
}

const char *mail_parser::date()
{
    ___mail_hval_skip_left(date, "Date:");
}

long mail_parser::date_unix()
{
    if (!___data->date) {
        date();
    }
    if(___data->date_unix == -1) {
        ___data->date_unix = mime_header_line_decode_date(___data->date);
    }
    return ___data->date_unix;
}

const mime_address &mail_parser::from()
{
#define ___mail_noutf8(a, b, c) \
    if (!___data->a) { \
        ___data->a = 1; \
        char *buf = ___get_buf(), *start; \
        size_t blen = ___get_header_value(___data->top_mime, b, buf, &start); \
        if (blen > 0) { \
            char *name, *address; \
            mime_address_parser &ap = ___data->addr_parser; \
            ap.parse(start, blen); \
            if (ap.shift(&name, &address)) { \
                ___data->c.set_values(___data->gmp->strdup(name), ___data->gmp->strdup(address), 0); \
            } \
        } \
    } \
    return ___data->c;
    ___mail_noutf8(from_flag, "From:", from);
}
const mime_address &mail_parser::from_utf8()
{
    if (___data->from_flag != 2){
        from();
        ___data->from_flag = 2;
        mime_parser_cache_magic mcm(___data->mcm);
        mcm.true_data = const_cast<char *>(___data->from.name());
        string &uname = mcm.require_string();
        uname.clear();
        mime_header_line_get_utf8(___data->src_charset_def, (char *)(&mcm), strlen(mcm.true_data), uname);
        ___data->from.set_values(0, 0, ___data->gmp->memdupnull(uname.c_str(), uname.size()));
    }
    return ___data->from;
}

const mime_address &mail_parser::sender()
{
    ___mail_noutf8(sender_flag, "Sender:", sender);
}

const mime_address &mail_parser::reply_to()
{
    ___mail_noutf8(reply_to_flag, "Reply-To:", reply_to);
}

const mime_address &mail_parser::receipt()
{
    ___mail_noutf8(receipt_flag, "Disposition-Notification-To:", receipt);
}

const char *mail_parser::in_reply_to()
{
    ___mail_hval_first_token(in_reply_to, "In-Reply-To:");
}

const vector<mime_address *> &mail_parser::to()
{
#define ___mail_parser_inner_tcb(tcb_flag, tcbname, tcb, utf8_tf)  \
    if (!___data->tcb_flag) {  \
        ___data->tcb_flag = 1; \
        char *buf = ___get_buf(), *start; \
        size_t blen = ___get_header_value(___data->top_mime, tcbname, buf, &start); \
        mime_parser_cache_magic mcm(___data->mcm); \
        mcm.true_data = start; \
        mime_header_line_get_address((char *)(&mcm), blen, ___data->tcb); \
    } \
    if (utf8_tf && (___data->tcb_flag!=2)) { \
        ___data->tcb_flag = 2; \
        mime_parser_cache_magic mcm(___data->mcm); \
        string &dest = mcm.require_string(); \
        dest.clear(); \
        zcc_vector_walk_begin(___data->tcb, addr) { \
            const char * name = addr->name(); \
            if (*name) { \
                mcm.true_data = const_cast<char *>(name); \
                mime_header_line_get_utf8(___data->src_charset_def, (char *)&mcm, strlen(name), dest); \
                addr->set_values(0, 0, mcm.gmp->memdupnull(dest.c_str(), dest.size())); \
            } \
        } zcc_vector_walk_end; \
    } \
    return ___data->tcb;
    ___mail_parser_inner_tcb(to_flag, "To:", to, false);
}

const vector<mime_address *> &mail_parser::to_utf8()
{
    ___mail_parser_inner_tcb(to_flag, "To:", to, true);
}

const vector<mime_address *> &mail_parser::cc()
{
    ___mail_parser_inner_tcb(cc_flag, "Cc:", cc, false);
}

const vector<mime_address *> &mail_parser::cc_utf8()
{
    ___mail_parser_inner_tcb(cc_flag, "Cc:", cc, true);
}

const vector<mime_address *> &mail_parser::bcc()
{
    ___mail_parser_inner_tcb(bcc_flag, "Bcc:", bcc, false);
}

const vector<mime_address *> &mail_parser::bcc_utf8()
{
    ___mail_parser_inner_tcb(bcc_flag, "Bcc:", bcc, true);
}

const vector<char *> &mail_parser::references()
{
    if (___data->references_flag) {
        return ___data->references;
    }
    ___data->references_flag = 1;
    char *buf = ___get_buf(), *start;
    size_t blen = ___get_header_value(___data->top_mime, "References:", buf, &start);
    if (blen < 1) {
        return ___data->references;
    }
    start[blen] = 0;
    strtok splitor;
    char *mid;
    splitor.set_str(start);
    while (splitor.tok("<> \t,\r\n")) {
        mid = (char *)___data->gmp->memdupnull(splitor.ptr(), splitor.size());
        ___data->references.push_back(mid);
    }
    return ___data->references;
}

const mail_parser_mime *mail_parser::top_mime()
{
    return ___data->top_mime;
}

const vector<mail_parser_mime *> &mail_parser::all_mimes()
{
    return ___data->all_mimes;
}

const vector<mail_parser_mime *> &mail_parser::text_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_inner * parser);
        mime_classify(___data);
    }
    return ___data->text_mimes;
}

const vector<mail_parser_mime *> &mail_parser::show_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_inner * parser);
        mime_classify(___data);
    }
    return ___data->show_mimes;
}
const vector<mail_parser_mime *> &mail_parser::attachment_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_inner * parser);
        mime_classify(___data);
    }
    return ___data->attachment_mimes;
}

const vector<size_data_t *> &mail_parser::header_line()
{
    return ___data->top_mime->header_line();
}

/* sn == 0: first, sn == -1: last */
size_t mail_parser::header_line(const char *header_name, char **data, int n)
{
    return ___data->top_mime->header_line(header_name, data, n);
}

bool mail_parser::header_line(const char *header_name, string &result, int n)
{
    return ___data->top_mime->header_line(header_name, result, n);
}
bool mail_parser::header_line(const char *header_name, vector<const size_data_t *> &vec) {
    return ___data->top_mime->header_line(header_name, vec);
}

}

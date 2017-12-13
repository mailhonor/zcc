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

mail_parser_mime_engine::mail_parser_mime_engine(mail_parser_engine *_parser)
{
    encoding_flag = 0;
    disposition_flag = 0;
    show_name_flag = 0;
    name_flag = 0;
    name_utf8_flag = 0;
    filename_flag = 0;
    filename2231_flag = 0;
    filename_utf8_flag = 0;
    content_id_flag = 0;

    header_offset = 0;
    header_len = 0;
    body_offset = 0;
    body_len = 0;
    mime_type = 0;

    is_tnef = false;
    filename2231_with_charset = false;

    wrap = 0;
    next = 0;
    child = 0;
    parent = 0;
    parser = _parser;
}


mail_parser_mime_engine::~mail_parser_mime_engine()
{
}

mail_parser_engine::mail_parser_engine()
{
    parsed_flag = 0;
    date_unix = 0;
    from_flag = 0;
    sender_flag = 0;
    reply_to_flag = 0;
    message_id_flag = 0;
    in_reply_to_flag = 0;
    to_flag = 0;
    cc_flag = 0;
    bcc_flag = 0;
    receipt_flag = 0;
    references_flag = 0;
    classify_flag = 0;
    section_flag = 0;

    top_mime = 0;
    top_mime_engine = 0;

    mime_max_depth = 0;
    src_charset_def[0] = 0;

    mail_data = 0;
    mail_pos = 0;
    mail_size = 0;
}

mail_parser_engine::~mail_parser_engine()
{
    std_list_walk_begin(all_mimes_engine, m) {
        delete m;
    } std_list_walk_end;
};

/* ################################################################## */
mail_parser_mime::mail_parser_mime(mail_parser_mime_engine *engine)
{
    ___data = engine;
    engine->wrap = this;
}

mail_parser_mime::~mail_parser_mime()
{
}

const std::string &mail_parser_mime::type()
{
    return ___data->type;
}

const std::string &mail_parser_mime::encoding()
{
    if (___data->encoding_flag) {
        return ___data->encoding;
    }
    ___data->encoding_flag = 1;
    std::string tmp;
    header_line_value("Content-Transfer-Encoding:", tmp);
    if (!tmp.empty()) {
        mime_header_line_decode_content_transfer_encoding(tmp.c_str(), tmp.size(), ___data->encoding);
        tolower(___data->encoding);
    }
    return ___data->encoding;
}

const std::string &mail_parser_mime::charset()
{
    return ___data->charset;
}

const std::string &mail_parser_mime::disposition()
{
    if (___data->disposition_flag) {
        return ___data->disposition;
    }
    ___data->disposition_flag = 1;
    std::string tmp;
    header_line_value("Content-Disposition:", tmp);
    if (!tmp.empty()) {
        mime_header_line_decode_content_disposition(tmp.c_str(), tmp.size(), ___data->disposition, ___data->filename, ___data->filename2231, &___data->filename2231_with_charset);
    }
    return ___data->disposition;
}

const std::string &mail_parser_mime::show_name()
{
    if (!___data->show_name_flag) {
        ___data->show_name_flag = 1;
        const std::string *n = &name_utf8();
        if (n->empty()) {
            n = &filename_utf8();
        }
        if (n->empty()) {
            n = &filename();
        }
        if (n->empty()) {
            n = &name();
        }
        ___data->show_name = const_cast<std::string *>(n);
    }
    return *(___data->show_name);
}

const std::string &mail_parser_mime::name()
{
    return ___data->name;
}

const std::string &mail_parser_mime::name_utf8()
{
    if (___data->name_utf8_flag) {
        return ___data->name_utf8;
    }
    ___data->name_utf8_flag = 1;
    mime_header_line_get_utf8(___data->parser->src_charset_def.c_str(), ___data->name.c_str(), ___data->name.size(), ___data->name_utf8);
    return ___data->name_utf8;
}

const std::string &mail_parser_mime::filename()
{
    if (!___data->disposition_flag) {
        disposition();
    }
    return ___data->filename;
}

const std::string &mail_parser_mime::filename_utf8()
{
    if (___data->filename_utf8_flag) {
        return ___data->filename_utf8;
    }
    ___data->filename_utf8_flag = 1;
    disposition();
    std::string uname;
    uname.clear();
    if (!___data->filename2231.empty()) {
        mime_header_line_get_utf8_2231(___data->parser->src_charset_def.c_str(), ___data->filename2231.c_str(), ___data->filename2231.size(), ___data->filename_utf8, ___data->filename2231_with_charset);
    } else  if (!___data->filename.empty()) {
        mime_header_line_get_utf8(___data->parser->src_charset_def.c_str(), ___data->filename.c_str(), ___data->filename.size(), ___data->filename_utf8);
    }
    return ___data->filename_utf8;
}

const std::string &mail_parser_mime::filename2231()
{
    if (___data->filename2231_flag) {
        return ___data->filename2231;
    }
    ___data->filename2231_flag = 1;
    if (!___data->disposition_flag) {
        disposition();
    }
    return ___data->filename2231;
}

bool mail_parser_mime::filename2231_with_charset()
{
    if (___data->filename2231_flag) {
      return ___data->filename2231_with_charset;
    }
    if (!___data->disposition_flag) {
        disposition();
    }
    return ___data->filename2231_with_charset;
}

const std::string &mail_parser_mime::content_id()
{
#define ___mail_hval_first_token(a, b) \
    if (___data->a ## _flag == 0) { \
        ___data->a ## _flag = 1; \
        std::string tmp; \
        header_line_value(b, tmp); \
        mime_header_line_get_first_token(tmp.c_str(), tmp.size(), ___data->a); \
    } \
    return ___data->a; 
    ___mail_hval_first_token(content_id, "Content-ID:");
}

const std::string &mail_parser_mime::boundary()
{
    return ___data->boundary;
}

const std::string &mail_parser_mime::imap_section()
{
    void mime_set_imap_section(mail_parser_engine *);
    if (!___data->parser->section_flag) {
        mime_set_imap_section(___data->parser);
    }
    ___data->parser->section_flag = 1;
    return ___data->imap_section;
}

const char *mail_parser_mime::header_data()
{
    return ___data->parser->mail_data + ___data->header_offset;
}

size_t mail_parser_mime::header_offset()
{
    return ___data->header_offset;
}

size_t mail_parser_mime::header_size()
{
    return ___data->header_len;
}

const char *mail_parser_mime::body_data()
{
    return ___data->parser->mail_data + ___data->body_offset;
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


const std::list<size_data_t> &mail_parser_mime::raw_header_line()
{
    return ___data->raw_header_lines;
}

size_t mail_parser_mime::raw_header_line(const char *header_name, char **result, int n)
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
    std_list_walk_begin(___data->raw_header_lines, sd) {
        if (sd.size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd.data);
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
            *result = sd.data;
            return sd.size;
        }
        nn ++;
        sdn = &sd;
    } std_list_walk_end;
    if (!sdn) {
        return 0;
    }
    if (n != -1) {
        return 0;
    }

    *result = sdn->data;
    return sdn->size;
}

bool mail_parser_mime::raw_header_line(const char *header_name, std::string &result, int n)
{
    char *data;
    size_t len;
    result.clear();
    if ((len = raw_header_line(header_name, &data, n))) {
        result.append(data, len);
        return true;
    }
    return false;
}

bool mail_parser_mime::raw_header_line(const char *header_name, std::list<size_data_t> &vec)
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
    std_list_walk_begin(___data->raw_header_lines, sd) {
        if (sd.size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd.data);
        if (data[name_len] != ':') {
            continue;
        }
        if (firstch != toupper(data[0])) {
            continue;
        }
        if (strncasecmp(data, header_name, name_len)) {
            continue;
        }
        vec.push_back(sd);
        hv = 1;
    } std_list_walk_end;

    if (hv) {
        return true;
    }
    return false;
}
bool mail_parser_mime::raw_header_line(const char *header_name, std::list<std::string> &vec)
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
    std_list_walk_begin(___data->raw_header_lines, sd) {
        if (sd.size < name_len) {
            continue;
        }
        const char *data = (const char *)(sd.data);
        if (data[name_len] != ':') {
            continue;
        }
        if (firstch != toupper(data[0])) {
            continue;
        }
        if (strncasecmp(data, header_name, name_len)) {
            continue;
        }
        std::string tmp(data, sd.size);
        vec.push_back(tmp);
        hv = 1;
    } std_list_walk_end;

    if (hv) {
        return true;
    }
    return false;
}

bool mail_parser_mime::header_line_value(const char *header_name, std::string &result, int n)
{
    int len = strlen(header_name);
    if (len < 1) {
        return false;
    }
    if (header_name[len-1] == ':') {
        len --;
    }
    if (len < 1) {
        return false;
    }
    std::string tmp;
    char *vp;
    size_t vlen;
    vlen = raw_header_line(header_name, &vp, n);
    result.clear();
    if (vlen) {
        std::string tmp;
        mime_raw_header_line_unescape(vp + (len + 1), vlen - (len + 1), tmp);
        char *start;
        int rlen = skip(tmp.c_str(), tmp.size(), " \t\r\n", 0, &start);
        if (rlen > 0) {
            result.append(start, rlen);
        }
    }
    return true;
}

bool mail_parser_mime::header_line_value(const char *header_name, std::list<std::string> &vec)
{
    int len = strlen(header_name);
    if (len < 1) {
        return false;
    }
    if (header_name[len-1] == ':') {
        len --;
    }
    if (len < 1) {
        return false;
    }
    std::list<size_data_t> tmpvec;
    bool tf = raw_header_line(header_name, tmpvec);
    if (tf) {
        std_list_walk_begin(tmpvec, sd) {
            std::string tmp;
            mime_raw_header_line_unescape(sd.data + (len + 1), sd.size - (len + 1), tmp);
            char *start;
            int rlen = skip(tmp.c_str(), tmp.size(), " \t\r\n", 0, &start);
            std::string result;
            if (rlen > 0) {
                result.append(start, rlen);
            }
            vec.push_back(result);
        } std_list_walk_end;
    }
    return true;
}

void mail_parser_mime::decoded_content(std::string &dest)
{
    mail_parser_engine *parser = ___data->parser;
    char *in_src = parser->mail_data + ___data->body_offset;
    int in_len = ___data->body_len;
    const char *enc = encoding().c_str();

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

void mail_parser_mime::decoded_content_utf8(std::string &dest)
{
    mail_parser_engine *parser = ___data->parser;
    char *in_src = parser->mail_data + ___data->body_offset;
    int in_len = ___data->body_len;
    const char *enc = encoding().c_str();
    const char *cha = charset().c_str();
    const char *str;
    size_t str_len;
    std::string tmp_cache_buf;

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

    mime_iconv((*cha)?cha:parser->src_charset_def.c_str(), str, str_len, dest);
}


/* ################################################################## */
mail_parser::mail_parser()
{
    init();
}

void mail_parser::set_mime_max_depth(size_t depth)
{
    if (depth > 10) {
        depth = 10;
    } 
    if (depth < 1) {
        depth = 1;
    }
    ___data->mime_max_depth = depth;
}

void mail_parser::set_src_charset_def(const char *src_charset_def)
{
    if (src_charset_def) {
        ___data->src_charset_def = src_charset_def;
    }
}

void mail_parser::parse(const char *mail_data, size_t mail_data_len)
{
    if (___data->parsed_flag) {
        zcc_fatal("mail_parser::parse must be executed only once");
        return;
    }
    ___data->parsed_flag = 1;

    ___data->mail_data = (char *)mail_data;
    ___data->mail_pos = (char *)mail_data;
    ___data->mail_size = mail_data_len;
    int ___mail_decode_mime(mail_parser_engine *, mail_parser_mime_engine *, mail_parser_mime_engine *, char *);
    char *buf = (char *)malloc(var_mime_header_line_max_length + 10);
    ___mail_decode_mime(___data, 0, ___data->top_mime_engine, buf + 6);
    free(buf);

    mail_parser_mime *mw;
    std_list_walk_begin(___data->all_mimes_engine, me) {
        mw = new mail_parser_mime(me);
        ___data->all_mimes.push_back(mw);
        if (___data->top_mime == 0) {
            ___data->top_mime = mw;
        }
    } std_list_walk_end;
}

bool mail_parser::parse(const char *filename)
{
    if (!___data->fmmap.mmap(filename)) {
        zcc_info("can not open %s(%m)", filename);
        return false;
    }
    parse(___data->fmmap.data(), ___data->fmmap.size());
    return true;
}

mail_parser::~mail_parser()
{
    std_list_walk_begin(___data->all_mimes, m) {
        delete m;
    } std_list_walk_end;

    delete ___data;
}

void mail_parser::init()
{
    ___data = new mail_parser_engine();
    ___data->date_unix = -1;
    ___data->mime_max_depth = 5;
    ___data->top_mime_engine = new mail_parser_mime_engine(___data);
    ___data->all_mimes_engine.push_back(___data->top_mime_engine);
}

const char *mail_parser::data()
{
    return ___data->mail_data;
}

size_t mail_parser::size()
{
    return ___data->mail_size;
}

const char *mail_parser::header_data()
{
    return ___data->mail_data + ___data->top_mime->header_offset();
}

size_t mail_parser::header_offset()
{
    return ___data->top_mime->header_offset();
}

size_t mail_parser::header_size()
{
    return ___data->top_mime->header_size();
}

const char *mail_parser::body_data()
{
    return ___data->mail_data + ___data->top_mime->body_offset();
}

size_t mail_parser::body_offset()
{
    return ___data->top_mime->body_offset();
}

size_t mail_parser::body_size()
{
    return ___data->top_mime->body_size();
}

const std::string &mail_parser::message_id()
{
    ___mail_hval_first_token(message_id, "message-id:");
}

const std::string &mail_parser::subject()
{
    if (!___data->subject_flag) {
        header_line_value("Subject:", ___data->subject);
        ___data->subject_flag = 1;
    }
    return ___data->subject;
}

const std::string &mail_parser::subject_utf8()
{
    if (!___data->subject_flag) {
        subject();
    }
    if (!___data->subject_utf8_flag) {
        mime_header_line_get_utf8(___data->src_charset_def.c_str(), ___data->subject.c_str(), ___data->subject.size(), ___data->subject_utf8);
    }
    return ___data->subject_utf8;
}

const std::string &mail_parser::date()
{
    if (!___data->date_flag) {
        header_line_value("Date:", ___data->date);
        ___data->date_flag = 1;
    }
    return ___data->date;
}

long mail_parser::date_unix()
{
    if (!___data->date_flag) {
        date();
    }
    if(___data->date_unix == -1) {
        ___data->date_unix = mime_header_line_decode_date(___data->date.c_str());
    }
    return ___data->date_unix;
}

const mime_address &mail_parser::from()
{
#define ___mail_noutf8(a, b, c) \
    if (!___data->a) { \
        ___data->a = 1; \
        std::string tmp; \
        header_line_value(b, tmp); \
        if (!tmp.empty()) { \
            char *name, *address; \
            mime_address_parser ap; \
            ap.parse(tmp.c_str(), tmp.size()); \
            if (ap.shift(&name, &address)) { \
                ___data->c.name = name; \
                ___data->c.address = address; \
            } \
        } \
    } \
    return ___data->c;
    ___mail_noutf8(from_flag, "From:", from);
}
const mime_address &mail_parser::from_utf8()
{
    mime_address &f = ___data->from;
    if (___data->from_flag != 2){
        from();
        ___data->from_flag = 2;
        mime_header_line_get_utf8(___data->src_charset_def.c_str(), f.name.c_str(), f.name.size(), f.name_utf8);
    }
    return f;
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

const std::string &mail_parser::in_reply_to()
{
    ___mail_hval_first_token(in_reply_to, "In-Reply-To:");
}

const std::list<mime_address> &mail_parser::to()
{
#define ___mail_parser_engine_tcb(tcb_flag, tcbname, tcb, utf8_tf)  \
    if (!___data->tcb_flag) {  \
        ___data->tcb_flag = 1; \
        if (0)  {\
            std::string tmp; \
            header_line_value(tcbname, tmp); \
            mime_header_line_get_address(tmp.c_str(), tmp.size(), ___data->tcb); \
        } \
        if (1) {  \
            int tcbnamelen = strlen(tcbname); \
            std::list<size_data_t> tcbline_vec; \
            ___data->top_mime->raw_header_line(tcbname, tcbline_vec); \
            std::string utmp; \
            std_list_walk_begin(tcbline_vec, sdp) { \
                if (sdp.size <= (size_t)tcbnamelen)  { continue; } \
                utmp.clear(); \
                mime_raw_header_line_unescape(sdp.data + tcbnamelen, sdp.size - tcbnamelen, utmp); \
                mime_header_line_get_address(utmp.c_str(), utmp.size(), ___data->tcb); \
            } std_list_walk_end; \
        } \
    } \
    if (utf8_tf && (___data->tcb_flag!=2)) { \
        ___data->tcb_flag = 2; \
        std_list_walk_begin(___data->tcb, addr) { \
            if (!addr.name.empty()) { \
                mime_header_line_get_utf8(___data->src_charset_def.c_str(), addr.name.c_str(), addr.name.size(), addr.name_utf8); \
            } \
        } std_list_walk_end; \
    } \
    return ___data->tcb;
    ___mail_parser_engine_tcb(to_flag, "To:", to, false);
}

const std::list<mime_address> &mail_parser::to_utf8()
{
    ___mail_parser_engine_tcb(to_flag, "To:", to, true);
}

const std::list<mime_address> &mail_parser::cc()
{
    ___mail_parser_engine_tcb(cc_flag, "Cc:", cc, false);
}

const std::list<mime_address> &mail_parser::cc_utf8()
{
    ___mail_parser_engine_tcb(cc_flag, "Cc:", cc, true);
}

const std::list<mime_address> &mail_parser::bcc()
{
    ___mail_parser_engine_tcb(bcc_flag, "Bcc:", bcc, false);
}

const std::list<mime_address> &mail_parser::bcc_utf8()
{
    ___mail_parser_engine_tcb(bcc_flag, "Bcc:", bcc, true);
}

const std::list<std::string> &mail_parser::references()
{
    if (___data->references_flag) {
        return ___data->references;
    }
    ___data->references_flag = 1;
    std::string tmp;
    header_line_value("References:", tmp);
    if (tmp.size() < 2) {
        return ___data->references;
    }
    strtok splitor;
    splitor.set_str(tmp.c_str());
    while (splitor.tok("<> \t,\r\n")) {
        ___data->references.push_back(std::string(splitor.ptr(), splitor.size()));
    }
    return ___data->references;
}

const mail_parser_mime *mail_parser::top_mime()
{
    return ___data->top_mime;
}

const std::list<mail_parser_mime *> &mail_parser::all_mimes()
{
    return ___data->all_mimes;
}

const std::list<mail_parser_mime *> &mail_parser::text_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_engine * parser);
        mime_classify(___data);
    }
    return ___data->text_mimes;
}

const std::list<mail_parser_mime *> &mail_parser::show_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_engine * parser);
        mime_classify(___data);
    }
    return ___data->show_mimes;
}
const std::list<mail_parser_mime *> &mail_parser::attachment_mimes()
{
    if(!___data->classify_flag) {
        void mime_classify(mail_parser_engine * parser);
        mime_classify(___data);
    }
    return ___data->attachment_mimes;
}

const std::list<size_data_t> &mail_parser::raw_header_line()
{
    return ___data->top_mime->raw_header_line();
}

/* n == 0: first, n == -1: last */
size_t mail_parser::raw_header_line(const char *header_name, char **data, int n)
{
    return ___data->top_mime->raw_header_line(header_name, data, n);
}

bool mail_parser::raw_header_line(const char *header_name, std::string &result, int n)
{
    return ___data->top_mime->raw_header_line(header_name, result, n);
}

bool mail_parser::raw_header_line(const char *header_name, std::list<size_data_t> &vec) {
    return ___data->top_mime->raw_header_line(header_name, vec);
}

bool mail_parser::raw_header_line(const char *header_name, std::list<std::string> &vec) {
    return ___data->top_mime->raw_header_line(header_name, vec);
}

bool mail_parser::header_line_value(const char *header_name, std::string &result, int n)
{
    return ___data->top_mime->header_line_value(header_name, result, n);
}

bool mail_parser::header_line_value(const char *header_name, std::list<std::string> &vec)
{
    return ___data->top_mime->header_line_value(header_name, vec);
}

}

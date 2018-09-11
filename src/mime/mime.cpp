/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2015-12-10
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

int ___mail_decode_mime(mail_parser_engine * parser, mail_parser_mime_engine * pmime, mail_parser_mime_engine * cmime, char *buf);

#define ___mftell(parser) ((parser)->mail_pos - (parser)->mail_data)

/* ################################################################## */
static int ___get_header_line(mail_parser_engine * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *ps, *p;
    int len = 0;

    *ptr = pbegin;
    if (pbegin > pend) {
        return 0;
    }
    if (pbegin[0] == '\n') {
        parser->mail_pos += 1;
        return 0;
    }
    if (pend > pbegin) {
        if ((pbegin[0] == '\r') && (pbegin[1] == '\n')) {
            parser->mail_pos += 2;
            return 0;
        }
    }

    ps = pbegin;
    while (pend > ps) {
        p = (char *)memchr(ps, '\n', pend - ps);
        if ((!p) || (p + 1 == pend)) {
            /* not found or to end */
            len = pend - pbegin;
            break;
        }
        if ((p[1] == ' ') || (p[1] == '\t')) {
            ps = p + 1;
            continue;
        }
        len = p - pbegin + 1;
        break;
    }

    parser->mail_pos += len;
    return len;
}

static inline int ___get_body_line(mail_parser_engine * parser, char **ptr)
{
    char *pbegin = parser->mail_pos;
    char *pend = parser->mail_data + parser->mail_size;
    char *p;
    int len;

    *ptr = pbegin;

    len = pend - pbegin;
    if (len < 1) {
        return 0;
    }
    p = (char *)memchr(pbegin, '\n', len);

    if (p) {
        len = p - pbegin + 1;
        parser->mail_pos += len;
    } else {
        parser->mail_pos += len;
    }
    return len;
}

/* ################################################################## */
static int deal_content_type(mail_parser_engine * parser, mail_parser_mime_engine * cmime, char *buf, int len)
{
    if (!cmime->type.empty()) {
        return 0;
    }

    mime_header_line_decode_content_type(buf, len, cmime->type, cmime->boundary, cmime->charset, cmime->name);
    tolower(cmime->type);

    return 0;
}

/* ################################################################## */
static int deal_single(mail_parser_engine * parser, mail_parser_mime_engine * pmime, mail_parser_mime_engine * cmime, char *buf)
{
    int ret = 2, len, blen, tell;
    char *line;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    blen = pmime->boundary.size();
    while (1) {
        tell = ___mftell(parser);
        if ((len = ___get_body_line(parser, &line)) < 1) {
            break;
        }
        if ((len < (blen + 2)) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }

        line += 2;
        if (!zcc_str_n_case_eq(line, pmime->boundary.c_str(), blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
static int deal_multpart(mail_parser_engine * parser, mail_parser_mime_engine * pmime, mail_parser_mime_engine * cmime, char *buf)
{
    int ret = 2;
    int len, blen;
    int have = 0;
    mail_parser_mime_engine *nmime = 0, *prev = 0;
    char *line;

    blen = cmime->boundary.size();
    while (1) {
        //int offset_bak = ___mftell(parser);
        len = ___get_body_line(parser, &line);
        if (len < 1) {
            break;
        }
        have = 1;

        if (cmime->boundary.empty()) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!zcc_str_n_case_eq(line, cmime->boundary.c_str(), blen)) {
            continue;
        }
        //cmime->body_offset = offset_bak;
        while (1) {
            int used = 0;
            nmime = new mail_parser_mime_engine(parser);
            parser->all_mimes_engine.push_back(nmime);
            ret = ___mail_decode_mime(parser, cmime, nmime, buf);
            if (ret == 2 || ret == 3) {
                used = 1;
                nmime->parent = cmime;
                if (!prev) {
                    cmime->child = nmime;
                } else {
                    prev->next = nmime;
                }
                prev = nmime;
            }
            if (!used) {
                parser->all_mimes_engine.resize(parser->all_mimes_engine.size() - 1);
                delete nmime;
            }
            if (ret != 2) {
                break;
            }
        }
        break;
    }
    /* XXX */
    if (ret == 5)
        return (5);
    if (!have) {
        return (5);
    }
    ret = 2;
    if (!pmime) {
        return (ret);
    }

    blen = pmime->boundary.size();
    len = 0;
    while ((len = ___get_body_line(parser, &line)) > 0) {
        if (pmime->boundary.empty()) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!zcc_str_n_case_eq(line, pmime->boundary.c_str(), blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        break;
    }
    cmime->body_len = ___mftell(parser) - cmime->body_offset - len;
    if (cmime->body_len < 0) {
        cmime->body_len = 0;
    }

    return (ret);
}

/* ################################################################## */
static int deal_message(mail_parser_engine * parser, mail_parser_mime_engine * pmime, mail_parser_mime_engine * cmime, char *buf)
{
    return deal_single(parser, pmime, cmime, buf);

    int ret = 2,  decode = 1, len, blen, tell;
    char *line, *bdata;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    const char *encoding = cmime->encoding.c_str();
    if (cmime->encoding.empty()) {
        decode = 0;
    } else if (zcc_str_case_eq(encoding, "7bit")) {
        decode = 0;
    } else if (zcc_str_case_eq(encoding, "8bit")) {
        decode = 0;
    } else if (zcc_str_case_eq(encoding, "binary")) {
        decode = 0;
    }
    if (decode) {
        return deal_single(parser, pmime, cmime, buf);
    }

    blen = pmime->boundary.size();
    bdata = (char *)(pmime->boundary.c_str());
    while (1) {
        tell = ___mftell(parser);
        if ((len = ___get_body_line(parser, &line)) <= 0) {
            break;
        }
        tell = ___mftell(parser);

        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!zcc_str_n_case_eq(line, bdata, blen)) {
            continue;
        }

        if ((len >= blen + 2) && (line[blen] == '-') && (line[blen + 1] == '-')) {
            ret = 3;
        }
        cmime->body_len = tell - cmime->body_offset;
        break;
    }
    return (ret);
}

/* ################################################################## */
#define ___clear_tail_rn(lll) { \
    int idx = cmime->header_offset + cmime->header_len; \
    if ((cmime->lll > 0) && (parser->mail_data[idx - 1] == '\n')) { idx--; cmime->lll--; } \
    if ((cmime->lll > 0) && (parser->mail_data[idx - 1] == '\r')) { cmime->lll--; } \
}

int ___mail_decode_mime(mail_parser_engine * parser, mail_parser_mime_engine * pmime, mail_parser_mime_engine * cmime, char *buf)
{
    char *line;
    int have_header = 0, ret = 0, llen, safe_llen;

    cmime->parser = parser;

    cmime->header_offset = ___mftell(parser);
    while (1) {
        safe_llen = llen = ___get_header_line(parser, &line);
        if (safe_llen > (int)var_mime_header_line_max_length) {
            safe_llen = var_mime_header_line_max_length - 2;
        }
        if (llen == 0) {
            cmime->header_len = ___mftell(parser) - cmime->header_offset;
            ___clear_tail_rn(header_len);
            cmime->body_offset = ___mftell(parser);
            break;
        }
        if (1) {
            size_data_t sd;
            sd.size = llen;
            sd.data = line;
            cmime->raw_header_lines.push_back(sd);
        }

        have_header = 1;
        if ((llen > 12) && (line[7]=='-') && (!strncasecmp(line, "Content-Type:", 13))) {
            int rlen = mime_raw_header_line_unescape(line, safe_llen, buf, var_mime_header_line_max_length);
            buf[rlen] = 0;
            if (!strncasecmp(line, "Content-Type:", 13)){
                ret = deal_content_type(parser, cmime, buf + 13, rlen - 13);
            }
        }
        if (ret == 5) {
            return 5;
        }
    }

    /* deal mail body */
    if (cmime->type.empty()) {
        /* gmp.free(cmime->type); */
        cmime->type = blank_buffer;
        /* cmime->type = gmp.strdup("text/plain"); */
    }
    if (!have_header) {
        return 4;
    }

    const char *ctype = cmime->type.c_str();
    if (zcc_str_n_case_eq(ctype, "multipart/", 10)) {
        int ppp = 1;
        mail_parser_mime_engine *parent = cmime->parent;
        for (; parent; parent = parent->parent) {
            ppp++;
        }
        if (ppp >= parser->mime_max_depth) {
            ret = deal_single(parser, pmime, cmime, buf);
        } else {
            ret = deal_multpart(parser, pmime, cmime, buf);
        }
    } else if (zcc_str_n_case_eq(ctype, "message/", 8)) {
        ret = deal_message(parser, pmime, cmime, buf);
    } else {
        ret = deal_single(parser, pmime, cmime, buf);
    }

    if (cmime->body_len == 0) {
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
    }

    ___clear_tail_rn(body_len);

    return (ret);
}
}

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

int ___mail_decode_mime(mail_parser_t * parser, mail_parser_mime_t * pmime, mail_parser_mime_t * cmime, char *buf);

#define ___mftell(parser) ((parser)->mail_pos - (parser)->mail_data)

/* ################################################################## */
static int ___get_header_line(mail_parser_t * parser, char **ptr)
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

static inline int ___get_body_line(mail_parser_t * parser, char **ptr)
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
static int deal_content_type(mail_parser_t * parser, mail_parser_mime_t * cmime, char *buf, int len)
{
    if (!empty(cmime->type)) {
        return 0;
    }
    char *val, *boundary, *charset, *name;
    size_t v_len, b_len, c_len, n_len;

    mime_decode_content_type(buf, len, &val, &v_len, &boundary, &b_len, &charset, &c_len, &name, &n_len);
    if (v_len) {
        cmime->type = parser->mpool->memdupnull(val, v_len);
        to_lower(cmime->type);
        printf("bbb; %s\n", cmime->type);
    }
    if (b_len) {
        cmime->boundary = parser->mpool->memdupnull(boundary, b_len);
    }
    if (c_len) {
        cmime->charset = parser->mpool->memdupnull(charset, c_len);
    }
    if (n_len) {
        cmime->name = parser->mpool->memdupnull(name, n_len);
    }

    return 0;
}

/* ################################################################## */
static int deal_single(mail_parser_t * parser, mail_parser_mime_t * pmime, mail_parser_mime_t * cmime, char *buf)
{
    int ret = 2, len, blen, tell;
    char *line;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    blen = (pmime->boundary?strlen(pmime->boundary):0);
    while (1) {
        tell = ___mftell(parser);
        if ((len = ___get_body_line(parser, &line)) < 1) {
            break;
        }
        if ((len < (blen + 2)) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }

        line += 2;
        if (!ZCC_STR_N_CASE_EQ(line, pmime->boundary, blen)) {
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
static int deal_multpart(mail_parser_t * parser, mail_parser_mime_t * pmime, mail_parser_mime_t * cmime, char *buf)
{
    int ret = 2;
    int len, blen;
    int have = 0;
    mail_parser_mime_t *nmime = 0;
    mail_parser_mime *nmime_w = 0, *prev = 0;;
    char *line;

    blen = (cmime->boundary?strlen(cmime->boundary):0);
    while (1) {
        //int offset_bak = ___mftell(parser);
        len = ___get_body_line(parser, &line);
        if (len < 1) {
            break;
        }
        have = 1;

        if (!cmime->boundary) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!ZCC_STR_N_CASE_EQ(line, cmime->boundary, blen)) {
            continue;
        }
        //cmime->body_offset = offset_bak;
        while (1) {
            int used = 0;
            nmime_w = new(parser->mpool->calloc(1, sizeof(mail_parser_mime))) mail_parser_mime(parser);
            nmime = (mail_parser_mime_t *)nmime_w;
            parser->all_mimes->push_back(nmime_w);
            ret = ___mail_decode_mime(parser, cmime, nmime, buf);
            if (ret == 2 || ret == 3) {
                used = 1;
                nmime->parent = (mail_parser_mime *)cmime;
                if (!prev) {
                    cmime->child = nmime_w;
                } else {
                    ((mail_parser_mime_t *)prev)->next = nmime_w;
                }
                prev = nmime_w;
            }
            if (!used) {
                parser->all_mimes->truncate(parser->all_mimes->size() - 1);
                nmime_w->~mail_parser_mime();
                parser->mpool->free(nmime_w);
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

    blen = (pmime->boundary?strlen(pmime->boundary):0);
    len = 0;
    while ((len = ___get_body_line(parser, &line)) > 0) {
        if (!pmime->boundary) {
            continue;
        }
        if ((len < blen + 2) || (line[0] != '-') || (line[1] != '-')) {
            continue;
        }
        line += 2;
        if (!ZCC_STR_N_CASE_EQ(line, pmime->boundary, blen)) {
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
static int deal_message(mail_parser_t * parser, mail_parser_mime_t * pmime, mail_parser_mime_t * cmime, char *buf)
{
    return deal_single(parser, pmime, cmime, buf);

    int ret = 2,  decode = 1, len, blen, tell;
    char *line;

    if (!pmime) {
        parser->mail_pos = parser->mail_data + parser->mail_size;
        cmime->body_len = ___mftell(parser) - cmime->body_offset;
        return 0;
    }
    if (!cmime->encoding) {
        decode = 0;
    } else if (ZCC_STR_CASE_EQ(cmime->encoding, "7bit")) {
        decode = 0;
    } else if (ZCC_STR_CASE_EQ(cmime->encoding, "8bit")) {
        decode = 0;
    } else if (ZCC_STR_CASE_EQ(cmime->encoding, "binary")) {
        decode = 0;
    }
    if (decode) {
        return deal_single(parser, pmime, cmime, buf);
    }

    blen = (pmime->boundary?strlen(pmime->boundary):0);
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
        if (!ZCC_STR_N_CASE_EQ(line, pmime->boundary, blen)) {
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

int ___mail_decode_mime(mail_parser_t * parser, mail_parser_mime_t * pmime, mail_parser_mime_t * cmime, char *buf)
{
    char *line;
    int have_header = 0, ret = 0, llen, safe_llen;
    size_data_t *sd;
    mem_pool &mpool = *(parser->mpool);

    cmime->parser = parser;

    cmime->type = blank_buffer;
    cmime->boundary = blank_buffer;
    cmime->charset = blank_buffer;
    cmime->name = blank_buffer;
    cmime->imap_section = blank_buffer;
    cmime->header_lines = new (mpool.malloc(sizeof(zcc::vector<size_data_t *>))) vector<size_data_t *>;

    cmime->header_offset = ___mftell(parser);
    parser->tmp_header_lines->clear();
    while (1) {
        safe_llen = llen = ___get_header_line(parser, &line);
        if (safe_llen > ZMAIL_HEADER_LINE_MAX_LENGTH) {
            safe_llen = ZMAIL_HEADER_LINE_MAX_LENGTH - 2;
        }
        if (llen == 0) {
            cmime->header_len = ___mftell(parser) - cmime->header_offset;
            ___clear_tail_rn(header_len);
            cmime->body_offset = ___mftell(parser);
            if (1) {
                /* re-save header to vector */
                cmime->header_lines->init(parser->tmp_header_lines->size(), mpool);
                vector<size_data_t *> &vec = *(parser->tmp_header_lines);
                for (size_t i = 0, len=vec.size(); i < len; i++) {
                    cmime->header_lines->push_back(vec[i]);
                };
            }
            break;
        }
        if (1) {
            /* save header to tmp vector*/
            sd = (size_data_t *)mpool.malloc(sizeof(size_data_t));
            sd->size = llen;
            sd->data = line;
            parser->tmp_header_lines->push_back(sd);
        }

        have_header = 1;
        if ((llen > 12) && (line[7]=='-') && (!strncasecmp(line, "Content-Type:", 13))) {

            int rlen = mime_unescape(line, safe_llen, buf);
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
    if (cmime->type == 0 || *(cmime->type) == 0) {
        parser->mpool->free(cmime->type);
        cmime->type = parser->mpool->strdup("text/plain");
    }
    if (!have_header) {
        return 4;
    }

    if (ZCC_STR_N_CASE_EQ(cmime->type, "multipart/", 10)) {
        int ppp = 1;
        mail_parser_mime *parent = ((mail_parser_mime_t *)cmime)->parent;
        for (; parent; parent = ((mail_parser_mime_t *)parent)->parent) {
            ppp++;
        }
        if (ppp >= parser->mime_max_depth) {
            ret = deal_single(parser, pmime, cmime, buf);
        } else {
            ret = deal_multpart(parser, pmime, cmime, buf);
        }
    } else if (ZCC_STR_N_CASE_EQ(cmime->type, "message/", 8)) {
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

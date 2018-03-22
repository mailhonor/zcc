/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-01-06
 * ================================
 */

#include "zcc.h"

#define zdebug          if (var_charset_debug)zcc_info

namespace zcc
{

#include "charset_utf8.h"

static inline int utf8_len(char *buf, int len)
{
    unsigned char *ptr;
    int ret;
    ptr = (unsigned char *)buf;
    if (((*ptr) <= 0x7F)) {
        ret = 1;
    } else if (((*ptr) & 0xF0) == 0xF0) {
        ret = 4;
    } else if (((*ptr) & 0xE0) == 0xE0) {
        ret = 3;
    } else if (((*ptr) & 0xC0) == 0xC0) {
        ret = 2;
    } else {
        ret = 5;
    }

    return ret;
}

static inline unsigned long ___chinese_word_score(unsigned char *word, int ulen)
{
    int start = 0, middle, end;
    unsigned int mint, wint;
    unsigned char *wp;
    unsigned char *wlist;

    if (ulen == 2) {
        wlist = (unsigned char *)utf8_list2;
        end = utf8_list_count2 - 1;
        wint = (word[0] << 8) | (word[1]);
    } else {
        wlist = (unsigned char *)(utf8_list3[(*word) & 0X0F]);
        end = utf8_list_count3[(*word) & 0X0F] - 1;
        wint = (word[1] << 8) | (word[2]);
    }

    while (1) {
        if (start > end) {
            return 0;
        }
        middle = (start + end) / 2;
        wp = wlist + middle * 3;
        mint = ((wp[0] << 8) | (wp[1]));

        if (wint < mint) {
            end = middle - 1;
            continue;
        }
        if (mint < wint) {
            start = middle + 1;
            continue;
        }
        return wp[2];
    }

    return 0;
}

static double ___chinese_score(const char *fromcode, char *str, int len, int omit_invalid_bytes_count)
{
    int i = 0, ulen;
    unsigned long score = 0;
    unsigned long count = 0;;

    while (i + 1 < len) {
        ulen = utf8_len(str + i, len - i);
        if ((ulen == 2) || (ulen == 3)) {
            score += ___chinese_word_score((unsigned char *)str + i, ulen);
            count++;
        }
        i += ulen;
    }

    if (count == 0) {
        return 0;
    }

    zdebug("        # %-20s, score:%lu, count:%lu, omit:%d" , fromcode, score, count, omit_invalid_bytes_count);
    return ((double)score / (count + omit_invalid_bytes_count));
}

bool charset_detect(const char *data, size_t size, std::string &charset_result, const char **charset_list)
{
    size_t i;
    ssize_t ret, max_i;
    const char **csp, *fromcode;
    size_t len_to_use, list_len;
    double result_score, max_score;
    int out_string_len = 4096 * 5 + 16;
    char *out_string = (char *) malloc(out_string_len  + 1);
    size_t converted_len, omit_invalid_bytes_count;
    charset_result.clear();

    list_len = 0;
    len_to_use = (size>4096?4096:size);
    csp = charset_list;
    for (fromcode = *csp; fromcode; csp++, fromcode = *csp) {
        list_len++;
    }
    if (list_len > 1000) {
        list_len = 1000;
    }

    max_score = 0;
    max_i = -1;
    zdebug("");
    zdebug("###########");
    for (i = 0; i < list_len; i++) {
        result_score = 0;
        fromcode = charset_list[i];

        ret = charset_iconv(fromcode, data, len_to_use
                , "UTF-8", out_string, out_string_len
                , &converted_len
                , -1, &omit_invalid_bytes_count);
        if (ret < 0) {
            zdebug("        # %-20s, iconv failure", fromcode);
            continue;
        }
        if (omit_invalid_bytes_count > 5) {
            zdebug("        # %-20s, omit_invalid_bytes: %ld", fromcode, omit_invalid_bytes_count);
            continue;
        }
        if (converted_len < 1) {
            continue;
        }
        result_score = ___chinese_score(fromcode, out_string, ret, omit_invalid_bytes_count);
        if (max_score < result_score) {
            max_i = i;
            max_score = result_score;
        }
    }
    free(out_string);

    if (max_i == (ssize_t)-1) {
        return false;
    }
    charset_result.append(charset_list[max_i]);

    return true;
}

bool charset_detect_cjk(const char *data, size_t size, std::string &charset_result)
{
    return charset_detect(data, size, charset_result, charset_cjk);
}

/* ################################################################## */

const char *charset_chinese[] = { "UTF-8", "GB18030", "BIG5", "UTF-7", 0 };
const char *charset_japanese[] = { "UTF-8", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "UTF-7", 0 };
const char *charset_korean[] = { "UTF-8", "KS_C_5601", "KS_C_5861", "UTF-7", 0 };
const char *charset_cjk[] = { "UTF-8", "GB18030", "BIG5", "EUC-JP", "JIS", "SHIFT-JIS", "ISO-2022-JP", "KS_C_5601", "KS_C_5861", "UTF-7", 0 };

}

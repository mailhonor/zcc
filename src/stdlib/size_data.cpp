/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2016-02-23
 * ================================
 */

#include "zcc.h"

namespace zcc
{

ssize_t size_data_unescape(const void *src_data, size_t src_size, char **result_data, size_t *result_size)
{
    size_t i = 0, len = 0, shift = 0;
    int ch;
    unsigned char *buf = (unsigned char *)src_data;
    while (1) {
        ch = ((i++ == src_size) ? -1 : *buf++);
        if (ch == -1) {
            return 0;
        }
        len |= ((ch & 0177) << shift);
        if (ch & 0200) {
            break;
        }
        shift += 7;
    }
    if (i + len > src_size) {
        return -1;
    }
    *result_data = (char *)buf;
    *result_size = len;
    return i + len;
}

size_t size_data_unescape_all(const void *src_data, size_t src_size, size_data_t *sdvector, size_t sdsize)
{
    size_t count = 0, i = 0, len = 0, shift = 0;
    int ch;
    unsigned char *buf = (unsigned char *)src_data;
    size_data_t *sd;
    while (1) {
        if (count >= sdsize) {
            return count;
        }
        i = 0; len = 0; shift = 0;

        while (1) {
            ch = ((i++ == src_size) ? -1 : *buf++);
            if (ch == -1) {
                return count;
            }
            len |= ((ch & 0177) << shift);
            if (ch & 0200) {
                break;
            }
            shift += 7;
        }
        if (i + len> src_size) {
            return count;
        }
        sd = sdvector + count++;
        sd->data = (char *)buf;
        sd->size = len;
        buf += len;
    }
    return count;
}

size_t size_data_put_size(size_t size, char *buf)
{
    int ch, left = (int)size, len = 0;
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
        ((unsigned char *)buf)[len++] = ch;
	} while (left);
    return len;
}

size_data_parser::size_data_parser(const void *src_data, size_t src_size)
{
    ptr = (char *)src_data;
    left = src_size;
}

size_data_parser::~size_data_parser()
{
}

int size_data_parser::shift(const char **data, size_t *size)
{
    if (left < 1){
        return 0;
    }
    int offset = size_data_unescape(ptr, left, (char **)data, size);
    if (offset < 1) {
        return offset;
    }
    ptr += offset;
    left -= offset;
    return 1;
}

int size_data_parser::shift(std::string &data)
{
    if (left < 1){
        return 0;
    }
    char *result;
    size_t rlen;
    int offset = size_data_unescape(ptr, left, &result, &rlen);
    if (offset < 1) {
        return offset;
    }
    data.append(result, rlen);
    ptr += offset;
    left -= offset;
    return 1;
}

}

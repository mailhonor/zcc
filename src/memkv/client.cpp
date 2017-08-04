/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-01
 * ================================
 */

#include "memkv.h"

namespace zcc
{

memkv::memkv(const char *_destination)
{
    destination = strdup(_destination);
    fp = 0;
}

memkv::~memkv()
{
    free(destination);
    if (fp) {
        int fd = fp->get_fd();
        delete fp;
        close(fd);
    }
}


int memkv::set(const char *partition, const char *key, const char *val, ssize_t vlen)
{
    if (vlen == -1) {
        vlen = strlen(val);
    }
    return require(memkv_op_type_set, partition, key, val, vlen, 0);
}

int memkv::set(const char *partition, const char *key, long val)
{
    return require(memkv_op_type_set_int, partition, key, (char *)val, -2, 0);
}

int memkv::del(const char *partition, const char *key)
{
    return require(memkv_op_type_del, partition, key, blank_buffer, 0, 0);
}

int memkv::inc(const char *partition, const char *key, long num, long *result)
{
    int r;
    if (result) {
        std::string v;
        r = require(memkv_op_type_inc, partition, key, (char *)num, -2, &v);
        *result = atoll(v.c_str());
    } else {
        r = require(memkv_op_type_inc, partition, key, (char *)num, -2, 0);
    }
    return r;
}

int memkv::clear(const char *partition)
{
    return require(memkv_op_type_clear, partition, blank_buffer, blank_buffer, 0, 0);
}

int memkv::exists(const char *partition, const char *key)
{
    return require(memkv_op_type_clear, partition, key, blank_buffer, 0, 0);
}

int memkv::get(const char *partition, const char *key, std::string &result)
{
    return require(memkv_op_type_get, partition, key, blank_buffer, 0, &result);
}

int memkv::get(const char *partition, const char *key, long *result)
{
    std::string v;
    int r = require(memkv_op_type_get, partition, key, blank_buffer, 0, &v);
    *result = atoll(v.c_str());
    return r;
}

int memkv::require(char op, const char *part, const char *key, const char *val, ssize_t vlen, std::string *result)
{
    std::string instr;
    instr.push_back(op);
    if (!part) {
        part = blank_buffer;
    }
    if (!key) {
        key = blank_buffer;
    }
    size_data_escape(instr, part, strlen(part));
    size_data_escape(instr, key, strlen(key));
    if (vlen == -2) {
        size_data_escape(instr, (long)val);
    } else {
        size_data_escape(instr, val, vlen);
    }
    for (int times = 0; times < 2; times ++) {
        if (times && fp) {
            int fd = fp->get_fd();
            delete fp;
            fp = 0;
            close(fd);
        }
        if (!fp) {
            int fd = connect(destination);
            if (fd < 0) {
                continue;
            }
            fp = new iostream(fd);
        }
        if (result) {
            result->clear();
        }
        char sizebuf[32];
        int len = size_data_put_size(instr.size(), sizebuf);
        fp->write(sizebuf, len);
        fp->write(instr.c_str(), instr.size());

        int ch = fp->get();
        if (ch < 0) {
            continue;
        }
        len = fp->size_data_get_size();
        if (len < 0) {
            continue;
        }
        if (len > 0) {
            if (result) {
                if (fp->readn(*result, len) < len) {
                    continue;
                }
            } else {
                while(len > 0) {
                    int rlen = len;
                    if (rlen > 30) {
                        rlen = 30;
                    }
                    if (fp->readn(sizebuf, rlen) < rlen) {
                        break;
                    }
                    len -= rlen;
                }
                if (len > 0) {
                    continue;
                }
            }
        }
        if (ch == memkv_op_result_error) {
            return -1;
        } else if (ch == memkv_op_result_unwant) {
            return 0;
        } else if (ch == memkv_op_result_want) {
            return 1;
        } else {
            return -1;
        }
        break;
    }
    if (fp) {
        int fd = fp->get_fd();
        delete fp;
        fp = 0;
        close(fd);
    }
    return -1;
}

}

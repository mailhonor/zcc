/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

namespace zcc
{

redis_client_basic_engine::redis_client_basic_engine()
{
}

redis_client_basic_engine::~redis_client_basic_engine()
{
}

static void ___exec_command_prepare(std::list<std::string> &ptokens, const char *redis_fmt, va_list ap)
{
    if (redis_fmt == 0) {
        return;
    }
    char nbuf[1024];
    int ch;
    const char *fmt = redis_fmt;
    while((ch = *fmt++)) {
        if (ch == 's') {
            const char *sv = va_arg(ap, const char *);
            if (sv != (const char *) -1) {
                ptokens.push_back(sv?sv:"");
            }
        } else if (ch == 'S') {
            const std::string *Sv = va_arg(ap, const std::string *);
            if (Sv != (const std::string *)-1) {
                if (Sv) {
                    ptokens.push_back(*Sv);
                } else {
                    ptokens.push_back("");
                }
            }
        } else if (ch == 'd') {
            long dv = va_arg(ap, long);
            sprintf(nbuf, "%ld", dv);
            ptokens.push_back(nbuf);
        } else if (ch == 'f') {
            double fv = va_arg(ap, double);
            sprintf(nbuf, "%lf", fv);
            ptokens.push_back(nbuf);
        } else if (ch == 'L') {
            const std::list<std::string> *lv = va_arg(ap, const std::list<std::string> *);
            if (!lv) {
                ptokens.push_back("");
            } else if (lv != (const std::list<std::string> *)-1) {
                for (std::list<std::string>::const_iterator it = lv->begin(); it != lv->end(); it++) {
                    ptokens.push_back(*it);
                }
            }
        } else if (ch == 'A') {
            argv *ts = va_arg(ap, argv *);
            if (!ts) {
                ptokens.push_back("");
            } else if (ts != (argv *) -1) {
                zcc_argv_walk_begin(*ts, p) {
                    ptokens.push_back(p);
                } zcc_argv_walk_end;
            }
        } else if (ch == 'P') {
            const char **pp = va_arg(ap, const char **);
            if (!pp) {
                ptokens.push_back("");
            } else if (pp != (const char **)-1) {
                while(*pp) {
                    ptokens.push_back(*pp++);
                }
            }
        }
    }
}

static int ___query_protocol_io_list_list(std::string &r_msg, int lnum, std::list<std::string> &lval, stream &fp)
{
    std::string rstr, rstr_tmp;
    for (int i = 0; i < lnum ;i++) {
        rstr.clear();
        if (fp.gets(rstr) < 3) {
            r_msg =  "gthe length of response < 4";
            return -2;
        }
        char *rp = (char *)rstr.c_str();
        int rlen = (int)rstr.size() - 2;
        char firstch = rp[0];
        if (firstch  == '*') {
            int tmp = atoi(rp + 1);
            if (tmp < 1) {
                lval.push_back("");
            } else {
                lnum += tmp;
            }
        } else if (firstch == ':') {
            if (rlen  < 1) {
                lval.push_back("");
            } else {
                rstr_tmp.clear();
                rstr_tmp.append(rp+1, rlen-1);
                lval.push_back(rstr_tmp);
            }
        } else if (firstch == '+') {
        } if (firstch == '$') {
            int num2 = atoi(rp+1);
            rstr_tmp.clear();
            if (num2 >0){
                fp.readn(rstr_tmp, num2);
            }
            if (num2 > -1) {
                fp.readn(0, 2);
            }
            if (fp.is_exception()) {
                r_msg = "gredis read/write";
                return -2;
            }
            lval.push_back(rstr_tmp);
        } else {
            r_msg = "gthe initial of response shold be $";
            return -2;
        }
    }
    return 1;
}

static int ___query_protocol_io_list_json(std::string &r_msg, int lnum, json &jval, stream &fp)
{
    std::string rstr;

    std::list<int> num_list;
    std::list<int> idx_list;
    std::list<json *> json_list;
    num_list.push_back(lnum);
    idx_list.push_back(-1);
    json_list.push_back(&jval);

    jval.used_for_array();

    while(!json_list.empty()) {
        int idx = idx_list.back() + 1; idx_list.pop_back();
        int num = num_list.back(); num_list.pop_back();
        json *jn = json_list.back(), *jn_tmp; json_list.pop_back();
        for (; idx < num ;idx++) {
            rstr.clear();
            if (fp.gets(rstr) < 3) {
                r_msg =  "the length of response < 4";
                return -2;
            }
            char *rp = (char *)rstr.c_str();
            int rlen = (int)rstr.size() - 2;
            rp[rlen] = 0;
            char firstch = rp[0];
            if (firstch == '*') {
                int tmp = atoi(rp + 1);
                if (tmp < 1) {
                    jn->array_add_element("");
                } else {
                    idx_list.push_back(idx);
                    num_list.push_back(num);
                    json_list.push_back(jn);

                    idx_list.push_back(-1);
                    num_list.push_back(tmp);
                    json_list.push_back(jn->array_add_element(new json(), true)->used_for_array());
                }
                break;
            }
            if (firstch == ':') {
                jn->array_add_element(new json(), true)->get_long_value() = ((rlen < 1)?-1:atol(rp+1));
                continue;
            }
            if (firstch == '+') {
                continue;
            }
            if (firstch == '$') {
                int num2 = atoi(rp+1);
                jn_tmp = jn->array_add_element(new json(), true);
                if (num2 < 0){
                    jn_tmp->get_bool_value() = false;
                } else if (num2 < 1){
                    jn_tmp->get_string_value().clear();
                } else {
                    fp.readn(jn_tmp->get_string_value(), num2);
                }
                if (num2 > -1) {
                    fp.readn(0, 2);
                }
                if (fp.is_exception()) {
                    r_msg = "edis read/write";
                    return -2;
                }
                continue;
            }
            r_msg = "the initial of response shold be $";
            return -2;
        }
    }
    return 1;
}

static int ___query_protocol_io_string_string(std::string &r_msg, int length, std::string *sval, stream &fp)
{
    if (length < 0) {
        return 0;
    }
    if (length >0){
        if (sval) {
            fp.readn(*sval, length);
        } else {
            fp.readn(0, length);
        }
    }
    fp.readn(0, 2);
    if (fp.is_exception()) {
        r_msg = "gredis read/write";
        return -2;
    }
    return 1;
}

static int ___query_protocol_io_string_json(std::string &r_msg, int length, json &jval, stream &fp)
{
    if (length < 0) {
        jval.get_bool_value() = false;
        return 0;
    }
    jval.used_for_string();
    if (length >0){
        fp.readn(jval.get_string_value(), length);
    }
    fp.readn(0, 2);
    if (fp.is_exception()) {
        r_msg = "gredis read/write";
        return -2;
    }
    return 1;
}

int redis_client_basic_engine::query_protocol_io(long *r_number_ret, std::string *r_string_ret, std::list<std::string> *r_list_ret, json *r_json_ret, std::list<std::string> &ptokens, long r_timeout, std::string &r_msg, stream &fp)
{
    if (ptokens.empty()) {
        return -1;
    }

    std::string rstr;
    char *rp;
    int rlen, num, firstch, ret;
    r_msg.clear();

    if (r_string_ret) {
        r_string_ret->clear();
    }

    std::list<std::string> list_ret_buffer, &list_ret = *(r_list_ret?r_list_ret:&list_ret_buffer);
    list_ret.clear();

    if (r_json_ret) {
        r_json_ret->reset();
    }

    if (!ptokens.empty()) {
        if (r_timeout  > 0) {
            if (fp.timed_wait_writeable(r_timeout) < 1) {
                r_msg =  "gtimed_wait_writeable error/timeout";
                return -2;
            }
        }
        if (r_timeout>0) {
            fp.set_timeout(r_timeout);
        }
        fp.printf_1024("*%zd\r\n", ptokens.size());
        for (std::list<std::string>::iterator it = ptokens.begin(); it != ptokens.end(); it++) {
            fp.printf_1024("$%zd\r\n", it->size());
            fp.write(it->c_str(), it->size());
            fp.write("\r\n", 2);
        }
    } else {
        if (r_timeout  > 0) {
            if ((ret=fp.timed_wait_readable(r_timeout)) < 0) {
                r_msg = "gtimed_wait_readable error/timeout";
                return -2;
            } else if (ret == 0 ){
                r_msg = "gno available readable data";
                return 0;
            }
        }
        if (r_timeout>0) {
            fp.set_timeout(r_timeout);
        }
    }

    if (fp.gets(rstr) < 3) {
        r_msg = "gredis read/write, or unknown protocol";
        return -2;
    }
    rp = (char *)rstr.c_str();
    rlen = (int)rstr.size() - 2;
    rp[rlen] = 0;
    firstch = rp[0];
    if (firstch == '-') {
        r_msg = rp;
        if (r_json_ret) {
            (r_json_ret->get_string_value()) = rp;
        }
        return -1;
    }
    if (firstch == '+') {
        r_msg = rp;
        if (r_json_ret) {
            (r_json_ret->get_string_value()) = rp;
        }
        return 1;
    }
    if (firstch == '*') {
        num = atoi(rp + 1);
        if (r_json_ret) {
            return ___query_protocol_io_list_json(r_msg, num, *r_json_ret, fp);
        }
        return ___query_protocol_io_list_list(r_msg, num, list_ret, fp);
    }

    if (firstch == ':') {
        long lret = atol(rp + 1);
        if (r_json_ret) {
            r_json_ret->get_long_value() = lret;
        }
        if (r_number_ret) {
            *r_number_ret = lret;
        } else {
            return (int)lret;
        }
        return 1;
    }
    if (firstch == '$') {
        num = atoi(rp+1);
        if (r_json_ret) {
            return ___query_protocol_io_string_json(r_msg, num, *r_json_ret, fp);
        }
        return ___query_protocol_io_string_string(r_msg, num, r_string_ret, fp);
    }
    r_msg = "gredis read/write, or unknown protocol";
    return -2;
}

}

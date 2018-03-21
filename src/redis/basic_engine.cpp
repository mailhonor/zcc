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

static const int var_redis_slot_count = 16384;
static int ___query_protocol_io_list_list(std::string &msg_info, int lnum, std::list<std::string> &lval, stream &fp)
{
    std::string rstr, rstr_tmp;
    for (int i = 0; i < lnum ;i++) {
        rstr.clear();
        if (fp.gets(rstr) < 3) {
            msg_info =  "ERR the length of response < 4";
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
                msg_info = "ERR redis read/write";
                return -2;
            }
            lval.push_back(rstr_tmp);
        } else {
            msg_info = "ERR the initial of response shold be $";
            return -2;
        }
    }
    return 1;
}

static int ___query_protocol_io_list_json(std::string &msg_info, int lnum, json &jval, stream &fp)
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
                msg_info =  "ERR the length of response < 4";
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
                    msg_info = "ERR redis read/write";
                    return -2;
                }
                continue;
            }
            msg_info = "ERR the initial of response shold be $";
            return -2;
        }
    }
    return 1;
}

static int ___query_protocol_io_string_string(std::string &msg_info, int length, std::string *sval, stream &fp)
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
        msg_info = "ERR redis read/write";
        return -2;
    }
    return 1;
}

static int ___query_protocol_io_string_json(std::string &msg_info, int length, json &jval, stream &fp)
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
        msg_info = "ERR redis read/write";
        return -2;
    }
    return 1;
}

int redis_client_basic_engine::query_protocol_io(redis_client_query &query, stream &fp)
{
    std::string rstr;
    char *rp;
    int rlen, num, firstch, ret;
    if (query.protocol_tokens == 0) {
        return -1;
    }
    std::list<std::string> &ptokens = *(query.protocol_tokens);

    std::string msg_info_buffer, &msg_info = *(query.msg_info?query.msg_info:&msg_info_buffer);
    msg_info.clear();

    if (query.number_ret) {
        *query.number_ret = -1;
    }

    if (query.string_ret) {
        query.string_ret->clear();
    }

    std::list<std::string> list_ret_buffer, &list_ret = *(query.list_ret?query.list_ret:&list_ret_buffer);
    list_ret.clear();

    if (query.json_ret) {
        query.json_ret->reset();
    }

    if (!ptokens.empty()) {
        if (query.timeout_milliseconds  > 0) {
            if (fp.timed_wait_writeable(query.timeout_milliseconds) < 1) {
                msg_info =  "ERR timed_wait_writeable error/timeout";
                return -2;
            }
        }
        if (query.timeout_milliseconds>0) {
            fp.set_timeout(query.timeout_milliseconds);
        }
        fp.printf_1024("*%zd\r\n", ptokens.size());
        for (std::list<std::string>::iterator it = ptokens.begin(); it != ptokens.end(); it++) {
            fp.printf_1024("$%zd\r\n", it->size());
            fp.write(it->c_str(), it->size());
            fp.write("\r\n", 2);
        }
    } else {
        if (query.timeout_milliseconds  > 0) {
            if ((ret=fp.timed_wait_readable(query.timeout_milliseconds)) < 0) {
                msg_info = "ERR timed_wait_readable error/timeout";
                return -2;
            } else if (ret == 0 ){
                msg_info = "ERR no available readable data";
                return 0;
            }
        }
        if (query.timeout_milliseconds>0) {
            fp.set_timeout(query.timeout_milliseconds);
        }
    }

    if (fp.gets(rstr) < 3) {
        msg_info = "ERR redis read/write, or unknown protocol";
        return -2;
    }
    rp = (char *)rstr.c_str();
    rlen = (int)rstr.size() - 2;
    rp[rlen] = 0;
    firstch = rp[0];
    if (firstch == '-') {
        msg_info = rp;
        *(query.msg_info) = rp;
        if (query.json_ret) {
            (query.json_ret->get_string_value()) = rp+1;
        }
        return -1;
    }
    if (firstch == '+') {
        msg_info = rp;
        if (query.json_ret) {
            (query.json_ret->get_string_value()) = rp+1;
        }
        return 1;
    }
    if (firstch == '*') {
        num = atoi(rp + 1);
        if (query.json_ret) {
            return ___query_protocol_io_list_json(msg_info, num, *query.json_ret, fp);
        }
        return ___query_protocol_io_list_list(msg_info, num, list_ret, fp);
    }

    if (firstch == ':') {
        long lret = atol(rp + 1);
        if (query.json_ret) {
            query.json_ret->get_long_value() = lret;
        }
        if (query.number_ret) {
            *query.number_ret = lret;
        } else {
            return (int)lret;
        }
        return 1;
    }
    if (firstch == '$') {
        num = atoi(rp+1);
        if (query.json_ret) {
            return ___query_protocol_io_string_json(msg_info, num, *query.json_ret, fp);
        }
        return ___query_protocol_io_string_string(msg_info, num, query.string_ret, fp);
    }
    msg_info = "ERR redis read/write, or unknown protocol";
    return -2;
}

int redis_client_basic_engine::query_protocol(redis_client_query &query)
{
    return -1;
}

int redis_client_basic_engine::timed_wait_readable(long timeout_millisecond)
{
    return 1;
}

int redis_client_basic_engine::timed_wait_writeable(long timeout_millisecond)
{
    return 1;
}

}

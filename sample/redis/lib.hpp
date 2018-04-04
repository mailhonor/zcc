/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc.h"

static long nval;
static std::string sval, sval2;
static std::list<std::string> lval;
static zcc::json jval;

static void _test_test(zcc::redis_basic_client &rc, const char *cmd, int cmd_ret, size_t line, int test_type)
{
    printf("\n%s\n%-8d", cmd, cmd_ret);
    if (cmd_ret < 0) {
        printf("%s  ### line:%zd", rc.get_msg().c_str(), line);
    } else if (test_type == 'r') {
        if (cmd_ret == 0) {
            printf("none/no/not");
        } else {
            printf("exists/yes/ok/count");
        }
    } else if(test_type == 'n') {
        printf("number: %ld", nval);
    } else if(test_type == 's') {
        if (cmd_ret == 0) {
            printf("none");
        } else {
            printf("string: %s", sval.c_str());
        }
    } else if(test_type == 'l') {
        if (cmd_ret == 0){
            printf("none");
        } else {
            printf("list: ");
            std_list_walk_begin(lval, v) {
                printf("%s, ", v.c_str());
            } std_list_walk_end;
        }
    } else if(test_type == 'j') {
        if (cmd_ret == 0){
            printf("none");
        } else {
            std::string out;
            jval.serialize(out);
            printf("json: %s", out.c_str());
        }
    }
    printf("\n"); fflush(stdout);
    nval = -1000;
    sval.clear();
    sval2.clear();
    lval.clear();
    jval.reset();
}

#define _test_return(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'r')
#define _test_number(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'n')
#define _test_string(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 's')
#define _test___list(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'l')
#define _test___json(cmd)  _test_test(rc, #cmd, cmd, __LINE__, 'j')

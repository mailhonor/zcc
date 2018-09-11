/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-25
 * ================================
 */

#include "./lib.hpp"

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    zcc::redis_client rc;
    if (zcc::default_config.get_bool("cluster", false)) {
        rc.cluster_open(zcc::default_config.get_str("server", "127.0.0.1:6379"));
    } else {
        rc.open(zcc::default_config.get_str("server", "127.0.0.1:6379"));
    }
    if (zcc::main_parameter_argc > 0 ) {
        _test___json(rc.exec_command(jval, "P", zcc::main_parameter_argv));
        return 0;
    }

    _test_return(rc.exec_command("sss", "SET", 0, "ssssss"));
    _test_string(rc.exec_command(sval, "ss", "GET", "abc"));
    _test_string(rc.exec_command(sval, "sss", "HGET", "xxx.com_u", "ac.tai"));

    _test_return(rc.exec_command("ss", "STRLEN", "abc"));
    _test_number(rc.exec_command(nval, "ss", "STRLEN", "abc"));

    _test___list(rc.exec_command(lval, "sss", "mget", "abc", "fasdfdsaf"));

    _test___json(rc.exec_command(jval, "sss", "MGET", "abc", "sss"));
    _test___json(rc.exec_command(jval, "sd", "SCAN", 0));
    _test___json(rc.exec_command(jval, "ssd", "EVAL", "return {1,2,{3,'Hello World!'}}", 0));
    _test___json(rc.exec_command(jval, "sd", "fffSCAN", 0));
    
    printf("\n");
    printf("%s [ -server 127.0.0.1:6379 ] redis_cmd arg1 arg2 ...\n", argv[0]);

    return 0;
}

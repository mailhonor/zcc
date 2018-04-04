/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "./lib.hpp"

int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    zcc::redis_basic_client *rc_p;
    if (zcc::default_config.get_bool("cluster", false)) {
        rc_p = new zcc::redis_cluster_client(zcc::default_config.get_str("server", "127.0.0.1:6379"));
    } else {
        rc_p = new zcc::redis_client(zcc::default_config.get_str("server", "127.0.0.1:6379"));
    }
    zcc::redis_basic_client &rc = *rc_p;
    if (zcc::main_parameter_argc > 0 ) {
        _test___json(rc.exec_command(jval, "P", zcc::main_parameter_argv));
        delete rc_p;
        return 0;
    }

    _test_return(rc.exec_command("sss", "SET", 0, "ssssss"));
    _test_string(rc.exec_command(sval, "ss", "GET", "abc"));
    _test_return(rc.exec_command("ss", "STRLEN", "abc"));
    _test_number(rc.exec_command(nval, "ss", "STRLEN", "abc"));

    _test___json(rc.exec_command(jval, "sss", "MGET", "abc", "sss"));
    _test___json(rc.exec_command(jval, "sd", "SCAN", 0));
    _test___json(rc.exec_command(jval, "ssd", "EVAL", "return {1,2,{3,'Hello World!'}}", 0));
    _test___json(rc.exec_command(jval, "sd", "fffSCAN", 0));
    
    printf("\n");
    printf("%s [ -server 127.0.0.1:6379 ] redis_cmd arg1 arg2 ...\n", argv[0]);

    delete rc_p;

    return 0;
}

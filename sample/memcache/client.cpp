/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2017-11-25
 * ================================
 */

#include "zcc.h"


int main(int argc, char **argv)
{
    zcc::main_parameter_run(argc, argv);
    zcc::memcache_client mc(zcc::default_config.get_str("server", "127.0.0.1:11211"));

    std::string str;
    long l;
    int flags;

    mc.set("iii", 0, 0, "123", 3);
    l = mc.incr("iii", 3);
    printf("incr iii: %ld\n", l);
    mc.get(str, &flags, "iii");
    printf("get iii: %s\n", str.c_str());

    mc.version(str);
    printf("version: %s\n", str.c_str());

    printf("\n");
    printf("%s [ -server 127.0.0.1:6379 ]\n", argv[0]);

    return 0;
}

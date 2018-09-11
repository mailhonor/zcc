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
    printf("%s [ -server 127.0.0.1:6379 ] [ -channel news.a1,news.a2]\n", argv[0]);

    zcc::main_parameter_run(argc, argv);

    zcc::redis_client rc(zcc::default_config.get_str("server", "127.0.0.1:6379"));

    zcc::argv channels(zcc::default_config.get_str("channel", "news.a1,news.a2"), ",");

    _test___list(rc.exec_command(lval, "sA", "SUBSCRIBE", &channels));

    for (int i = 0; i < 100; i++) {
        rc.set_timeout(1000);
        int ret = rc.fetch_channel_message(lval);
        if (ret == -2) {
            break;
        }
        if (ret == -1) {
            break;
        }
        if (ret == 0) {
            printf("no message\n");
        } else {
            std_list_walk_begin(lval, v) {
                printf("%s, ", v.c_str());
            } std_list_walk_end;
            printf("\n");
        }
    }

    printf("\n");
    return 0;
}

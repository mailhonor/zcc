/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-04
 * ================================
 */

#include "zcc.h"

int main()
{
    zcc::pm_pool *pm = new zcc::pm_pool();
    pm->option_piece_size(10, 5);

    std::list<char *> ms;

    for (size_t i = 0; i < 1000; i++) {
        ms.push_back((char *)pm->require());
    }

    bool tf = true;
    std_list_walk_begin(ms, p) {
        if (tf) {
            pm->release(p);
            tf = false;
        } else {
            tf = true;
        }
    } std_list_walk_end;

    tf = false;
    std_list_walk_begin(ms, p) {
        if (tf) {
            pm->release(p);
            tf = false;
        } else {
            tf = true;
        }
    } std_list_walk_end;

    delete pm;

    return 0;
}

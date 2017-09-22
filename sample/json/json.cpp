/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-23
 * ================================
 */

#include "zcc.h"


int main(int argc, char **argv)
{
    if (argc !=2 ) {
        printf("USAGE: %s json_filename\n", argv[0]);
        exit(1);
    }
    zcc::file_mmap fmap;
    if (!fmap.mmap(argv[1])) {
        printf("can not open %s (%m)\n", argv[1]);
    }

    zcc::string result;
    zcc::json jo;
    jo.unserialize(fmap.data(), fmap.size());

    jo.serialize(result);
    puts(result.c_str());

    return 0;
}

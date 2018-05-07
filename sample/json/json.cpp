/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-23
 * ================================
 */

#include "zcc.h"


static void test(const char *src, size_t size)
{
    std::string result;
    zcc::json jo;
    jo.unserialize(src, size);

    jo.serialize(result);
    puts(result.c_str());
}

int main(int argc, char **argv)
{
    if (argc !=2 ) {
        std::string s = "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
        test(s.c_str(), s.size());
        printf("USAGE: %s json_filename\n", argv[0]);
        exit(1);
    }
    zcc::file_mmap fmap;
    if (!fmap.mmap(argv[1])) {
        printf("can not open %s (%m)\n", argv[1]);
    }

    test(fmap.data(), fmap.size());

    return 0;
}

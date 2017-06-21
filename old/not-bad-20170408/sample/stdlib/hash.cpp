/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

#include "zcc.h"

static void ___usage(char *arg = 0)
{
    printf("USAGE:\n\t%s -f filename\n", zcc::var_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = 0;

    zcc::var_progname = argv[0];
    ZCC_PARAMETER_BEGIN() {
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-f")) {
            fn = optval;
            opti += 2;
            continue;
        }
    } ZCC_PARAMETER_END;

    if (zcc::empty(fn)) {
        ___usage(0);
    }

    zcc::file_mmap fm;
    fm.mmap(fn);
    const char *data = fm.data();
    size_t size = fm.size();

    printf("CRC32(unsigned int):     %ld\n", (long )zcc::get_crc32_result(data, size));
    printf("CRC64(unsigned long):    %zd\n", zcc::get_crc64_result(data, size));

    char result[128];
    zcc::string bstr;

    zcc::get_md5_result(data, size, result);
    bstr.clear();
    zcc::base64_encode(result, 20, bstr);
    printf("md5(base64):             %s\n", bstr.c_str());

    zcc::get_sha1_result(data, size, result);
    bstr.clear();
    zcc::base64_encode(result, 24, bstr);
    printf("sha1(base64):            %s\n", bstr.c_str());

    return 0;
}

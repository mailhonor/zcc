/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-12-07
 * ================================
 */

#include "zcc.h"

int main()
{
    zcc::rdonly_str_kv kv;
    const char *result;

    kv.add("abc", "123");
    kv.add("abd", "123b");
    kv.add("abe", "123c");
    kv.add("abf", "123d");
    kv.over();

    result = kv.find("abe");
    printf("%s\n", result?result:"not found");

    result = kv.find("BBB");
    printf("%s\n", result?result:"not found");

    return 0;
}

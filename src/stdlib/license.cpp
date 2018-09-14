/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-09-09
 * ================================
 */

#include "zcc.h"

namespace zcc
{

bool license_mac_check(const char *salt, const char *license)
{
    std::string license_c;
    std::list<std::string> mac_list;

    if (empty(salt) || empty(license)) {
        return false;
    }
    get_mac_list(mac_list);
    std_list_walk_begin(mac_list, mac) {
        license_c.clear();
        license_mac_build(salt, mac.c_str(), license_c);
        if (!strncasecmp(license_c.c_str(), license, 16)) {
            return true;
        }
    } std_list_walk_end;

    return false;
}

void license_mac_build(const char *salt, const char *_mac, std::string &rbuf)
{
    std::string builder;
    builder.append(salt);
    builder.append(",");
    size_t len = builder.size();
    builder.append(_mac);
    tolower(builder.c_str() + len);

    long crc = get_crc64_result(builder.c_str(), builder.size(), 0);
    rbuf.clear();
    hex_encode(&crc, 8, rbuf);
}

bool license_mac_check_by_config(const char *salt, const char *config_file, const char *key)
{
    zcc::config  cf;
    char *license;

    cf.load_by_filename(config_file);
    license = cf.get_str(key, "");

    if (empty(license)) {
        return false;
    }
    if (!zcc::license_mac_check(salt, license)) {
        return false;
    }
    return true;
}

}

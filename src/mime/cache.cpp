/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zcc.h"
#include "mime.h"

namespace zcc
{

mime_parser_cache_magic::mime_parser_cache_magic()
{
    magic = (char *)'M';
    gmp = 0;
    cache = (mime_parser_cache_magic_t *)malloc(sizeof(mime_parser_cache_magic_t));
    cache->used = 1;
    for(int i=0;i<10;i++) { cache->tmp_string[i] = 0; }
    tmp_string_idx_start = tmp_string_used_count = 0;
}

mime_parser_cache_magic::mime_parser_cache_magic(const mime_parser_cache_magic &_x)
{
    magic = (char *)'M';
    true_data = _x.true_data;
    gmp = _x.gmp;
    cache = _x.cache;
    cache->used ++;
    tmp_string_idx_start = _x.tmp_string_idx_start + _x.tmp_string_used_count;
    tmp_string_used_count = 0;
}

mime_parser_cache_magic::mime_parser_cache_magic(const void *data)
{
    magic = (char *)'M';
    mime_parser_cache_magic *mcm = (mime_parser_cache_magic *)data;
    if (mcm->magic == (char *)'M') {
        true_data = mcm->true_data;
        gmp = mcm->gmp;
        cache = mcm->cache;
        cache->used ++;
        tmp_string_idx_start = mcm->tmp_string_idx_start + mcm->tmp_string_used_count;
        tmp_string_used_count = 0;
    } else {
        true_data = (char *)data;
        gmp = 0;
        cache = (mime_parser_cache_magic_t *)malloc(sizeof(mime_parser_cache_magic_t));
        cache->used = 1;
        for(int i=0;i<10;i++) { cache->tmp_string[i] = 0; }
        tmp_string_idx_start = tmp_string_used_count = 0;
    }
}

mime_parser_cache_magic::~mime_parser_cache_magic()
{
    cache->used --;
    if (cache->used == 0) {
        for(int i=0;i<10;i++) {
            if (cache->tmp_string[i]) {
                delete cache->tmp_string[i];
            }
        }
        free(cache);
    }
}

string &mime_parser_cache_magic::require_string()
{
    if (!cache->tmp_string[tmp_string_idx_start+tmp_string_used_count]) {
        cache->tmp_string[tmp_string_idx_start+tmp_string_used_count] = new string();
    }
    return *(cache->tmp_string[tmp_string_idx_start+tmp_string_used_count++]);
}

}

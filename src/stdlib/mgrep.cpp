/*
 * ================================
 * eli960@qq.com
 * http://blog.chinaunix.net/uid/31513553.html
 * 2018-06-06
 * ================================
 */

#include "zcc.h"
#include <set>

namespace zcc
{
#pragma pack(push, 1)
struct mgrep_token_t {
    unsigned int offset;
    unsigned short int len;
};
typedef struct mgrep_token_t mgrep_token_t;

struct mgrep_index_t {
    unsigned short int begin;
    unsigned short int end;
};
typedef struct mgrep_index_t mgrep_index_t;
#pragma pack(pop)

mgrep::mgrep()
{
    m_ascii = 0;
    m_data = (char *)(-1);
    m_index = (char *)(new std::set<std::string>());
}

mgrep::~mgrep()
{
    if (m_ascii) {
        free(m_ascii);
    }
    if (m_data == (char *)(-1)) {
        delete (std::set<std::string> *)(m_index);
    } else if (m_data) {
        free(m_data);
    }
}

mgrep &mgrep::add_token(std::string &token)
{
    size_t s=token.size();
    if (s == 1) {
        if (m_ascii==0) {
            m_ascii = (unsigned char *)calloc(1, 256);
        }
        m_ascii[(unsigned char)token[0]] = 1;
    } else if (s > 1) {
        if (m_data == (char *)-1) {
            std::set<std::string> *tokens = (std::set<std::string> *)m_index;
            if (tokens->size() < 10000) {
                tokens->insert(token);
            }
        }
    }
    return *this;
}

mgrep &mgrep::add_token(const void *token, size_t size)
{
    std::string tmp_token((const char *)token, size);
    return add_token(tmp_token);
}

mgrep &mgrep::add_token(const char *token)
{
    std::string tmp_token(token);
    return add_token(tmp_token);
}

mgrep &mgrep::add_token_over()
{
    if (m_data != (char *)-1) {
        return *this;
    }
    std::set<std::string> *tokens = (std::set<std::string> *)m_index;
    m_data = 0;
    m_index = 0;
    m_count = tokens->size();
    if (m_count == 0) {
        return *this;
    }
    size_t all_size = 0;
    for (auto it = tokens->begin(); it != tokens->end(); it++) {
        all_size += it->size();
    }
    if (all_size > (1<<30)) {
        zcc_fatal("too long too more");
    }
    m_hash_size = ((2*m_count < 257)?257:(2*m_count+1));
    m_data = (char *)calloc(1, 1 + all_size + sizeof(mgrep_token_t)*(m_count+1) + sizeof(mgrep_index_t)*m_hash_size);
    m_index = (char *)m_data + 1 + all_size;
    m_hash = (char *)(m_data + 1 + all_size + sizeof(mgrep_token_t)*(m_count+1));

    std::list<mgrep_token_t> ** tmp_hash_vector = (std::list<mgrep_token_t> ** )calloc(sizeof(std::list<mgrep_token_t> *), m_hash_size);
    for (unsigned short int i=0;i<m_hash_size;i++) {
        tmp_hash_vector[i] = new std::list<mgrep_token_t>();
    }

    /* copy token */
    size_t char_offset = 1;
    for (auto it = tokens->begin(); it != tokens->end(); it++) {
        mgrep_token_t tt;
        tt.offset = char_offset;
        tt.len = it->size();
        unsigned char *data = (unsigned char *)(void *)(it->c_str());
        unsigned short int hv = (((unsigned short int)(data[0]))<<8) + (unsigned short int)(data[1]);
        hv = hv%m_hash_size;
        tmp_hash_vector[hv]->push_back(tt);
        memcpy(m_data + char_offset, it->c_str(), it->size());
        char_offset += it->size();
    }
    delete tokens;

    mgrep_token_t *t_index = (mgrep_token_t *)m_index, *t_index2;
    mgrep_index_t *t_hash = (mgrep_index_t *)m_hash;
    unsigned short int tokens_i = 1;
    for (unsigned short int i=0; i<m_hash_size;i++, t_hash++) {
        std::list<mgrep_token_t> * tmp_hash = tmp_hash_vector[i];
        if (tmp_hash->empty()) {
            t_hash->begin = 0;
            t_hash->end = 0;
        } else {
            t_hash->begin = tokens_i;
            t_hash->end = tokens_i;
            for (auto it = tmp_hash->begin(); it != tmp_hash->end(); it++) {
                t_hash->end = tokens_i;
                t_index2 = t_index + tokens_i;
                tokens_i++;
                t_index2->offset = it->offset;
                t_index2->len = it->len;
            }
        }
    }

    for (unsigned short int i=0; i<m_hash_size;i++) {
        delete tmp_hash_vector[i];
    }
    free(tmp_hash_vector);

    return *this;
}

int mgrep::match(const void *data, size_t size)
{
    std::string tmp;
    return match(tmp, data, size);
}

int mgrep::match(std::string &mathed_token, const void *data, size_t size)
{
    size_t i, left, i_left, i_middle, i_right;
    unsigned char *ptr = (unsigned char *)data, *ps, *tp;
    mgrep_token_t *t_index = (mgrep_token_t *)m_index, *t_index2;
    mgrep_index_t *t_hash = (mgrep_index_t *)m_hash, *t_hash2;
    unsigned short int hv, tlen, j;
    if ((!m_ascii) && ((!m_data)||(m_data==(char *)-1))) {
        return -1;
    }

    for (i = 0; i < size; i++) {
        ps = ptr + i;
        left = size - i;
        if (m_ascii) {
            if (m_ascii[*ps]) {
                mathed_token.push_back(*ps);
                return (ps - ptr);
            }
        }
        if (left == 1) {
            return -1;
        }
        hv = ((((unsigned short int)(ps[0]))<<8) + (unsigned short int)(ps[1]))%m_hash_size;
        t_hash2 = t_hash + hv;
        if (t_hash2->begin == 0) {
            continue;
        }
        i_left =t_hash2->begin;
        i_right = t_hash2->end;
        while(i_left <= i_right) {
            i_middle = (i_left + i_right)/2;
            t_index2 = t_index + i_middle;
            tp = (unsigned char *)m_data + t_index2->offset;
            tlen = t_index2->len;
            if (tlen <= left) {
                for (j=0;j<tlen;j++) {
                    if (tp[j] < ps[j]) {
                        i_left = i_middle +1;
                        break;
                    } else if (tp[j] > ps[j]) {
                        i_right = i_middle - 1;
                        break;
                    }
                }
                if (j == tlen) {
                    mathed_token.append((char *)tp, tlen);
                    return (ps - ptr);
                }
            } else {
                tlen = left;
                for (j=0;j<tlen;j++) {
                    if (tp[j] < ps[j]) {
                        i_left = i_middle +1;
                        break;
                    } else if (tp[j] > ps[j]) {
                        i_right = i_middle - 1;
                        break;
                    }
                }
                if (j == tlen) {
                    i_right = i_middle -1;
                }
            }
        }
    }
    return -1;
}

}

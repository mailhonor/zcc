/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zcc.h"

namespace zcc
{

std_vector_release_assistant::std_vector_release_assistant(const void *vec, void *handler)
{
    ___vec = (std::vector<void *> *) (vec);
    ___handler = (void (*)(void*))handler;
}

std_vector_release_assistant::~std_vector_release_assistant()
{
    if (___vec && ___handler) {
        for (std::vector<void *>::iterator it = ___vec->begin(); it != ___vec->end(); it++) {
            ___handler(*it);
        }
    }
}

}

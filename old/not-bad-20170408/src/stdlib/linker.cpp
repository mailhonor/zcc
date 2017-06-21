/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-27
 * ================================
 */

#include "zcc.h"

namespace zcc
{

void linker_init(linker_t * link)
{
    link->head = 0;
    link->tail = 0;
}

linker_node_t *linker_attach_before(linker_t * link, linker_node_t * node, linker_node_t * before)
{
    ZCC_MLINK_ATTACH_BEFORE(link->head, link->tail, node, prev, next, before);

    return node;
}

linker_node_t *linker_detach(linker_t * link, linker_node_t * node)
{
    if (!node) {
        return 0;
    }
    ZCC_MLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

linker_node_t *linker_push(linker_t * link, linker_node_t * node)
{
    ZCC_MLINK_APPEND(link->head, link->tail, node, prev, next);

    return node;
}

linker_node_t *linker_unshift(linker_t * link, linker_node_t * node)
{
    ZCC_MLINK_PREPEND(link->head, link->tail, node, prev, next);

    return node;
}

linker_node_t *linker_pop(linker_t * link)
{
    linker_node_t *node;

    node = link->tail;
    if (node == 0) {
        return 0;
    }
    ZCC_MLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

linker_node_t *linker_shift(linker_t * link)
{
    linker_node_t *node;

    node = link->head;
    if (node == 0) {
        return 0;
    }
    ZCC_MLINK_DETACH(link->head, link->tail, node, prev, next);

    return node;
}

void linker_fini(linker_t * link, void (*fini_fn) (linker_node_t *))
{
    linker_node_t *n, *next;

    n = link->head;
    for (; n; n = next) {
        next = n->next;
        if (fini_fn) {
            (*fini_fn) (n);
        }
    }
}

}

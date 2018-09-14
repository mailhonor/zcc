/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-11-27
 * ================================
 */

#include "zcc.h"

namespace zcc
{

typedef struct ___element_id_t ___element_id_t;
struct ___element_id_t {
    short int id;
};

struct ___piece_group_t {
    ___piece_group_t *prev;
    ___piece_group_t *next;
    rbtree_node_t rbnode;
    short int unused_id;
    unsigned short int used_count;
    char *element_vec;
};

static int ___piece_group_cmp(rbtree_node_t *n1, rbtree_node_t *n2)
{
    ___piece_group_t *p1 = zcc_container_of(n1, ___piece_group_t, rbnode);
    ___piece_group_t *p2 = zcc_container_of(n2, ___piece_group_t, rbnode);
    return (p1->element_vec - p2->element_vec);
}

___piece_group_t *___piece_group_create(short int esize, short int ecount)
{
    ___piece_group_t *pg = (___piece_group_t *)malloc(sizeof(___piece_group_t) + esize * ecount);
    memset(pg, 0, sizeof(___piece_group_t));
    pg->element_vec = (char *)((char *)pg + sizeof(___piece_group_t));
    for (short int i = 0; i < ecount; i++) {
        ((___element_id_t *)(pg->element_vec + esize * i))->id = i + 1;
    }
    ((___element_id_t *)(pg->element_vec + esize * (ecount-1)))->id = -1;
    pg->unused_id = 0;
    return pg;
}

void ___piece_group_free(___piece_group_t *pg)
{
    free(pg);
}

pm_pool::pm_pool()
{
    ___element_size = 0;
    ___element_used_count = 0;
    ___element_count_of_group = 0;
    ___group_count = 0;
    ___unused_piece_group = 0;
    ___space_piece_group_head = 0;
    ___space_piece_group_tail = 0;
    rbtree_init(&___piece_group_tree, ___piece_group_cmp);
}

pm_pool::~pm_pool()
{
    if (rbtree_have_data(&___piece_group_tree)) {
        zcc_rbtree_walk_begin(&___piece_group_tree, n) {
            ___piece_group_t *pg = zcc_container_of(n, ___piece_group_t, rbnode);
        ___piece_group_free(pg);
        } zcc_rbtree_walk_end;
    }
}

void pm_pool::set_piece_size(size_t piece_size, size_t element_count_of_group)
{
    if (piece_size  < sizeof(short int)) {
        zcc_fatal("piece_size must >= %zd", sizeof(short int)/2);
    }
    if (___element_size != 0) {
        zcc_fatal("piece_size already be set");
    }
    ___element_size = piece_size;
    if ((element_count_of_group == 0) || (element_count_of_group > 30000)) {
        ___element_count_of_group = (10 * 1024) / piece_size;
        if (___element_count_of_group < 100) {
            ___element_count_of_group = 100;
        } else if (___element_count_of_group > 4096) {
            ___element_count_of_group = 4096;
        }
    } else {
        ___element_count_of_group = element_count_of_group;
    }
}

void *pm_pool::require()
{
    ___piece_group_t *pg;
    if (___space_piece_group_head == 0) {
        if (___unused_piece_group) {
            ___space_piece_group_head = ___space_piece_group_tail = ___unused_piece_group;
            ___unused_piece_group = 0;
        } else {
            pg = ___piece_group_create(___element_size, ___element_count_of_group);
            rbtree_attach(&(___piece_group_tree), &(pg->rbnode));
            ___space_piece_group_head = ___space_piece_group_tail = pg;
            ___group_count++;
        }
    }
    pg = ___space_piece_group_head;
    char *r =pg->element_vec + pg->unused_id * ___element_size; 
    pg->unused_id = ((___element_id_t *)r)->id;
    pg->used_count++;
    ___element_used_count++;
    if (pg->used_count == ___element_count_of_group) {
        zcc_mlink_detach(___space_piece_group_head, ___space_piece_group_tail, pg, prev, next);
    }
    return (void *)r;
}

void pm_pool::release(const void *data)
{
    char *ptr = (char *)data;
    ___piece_group_t pgbuf, *pg;
    rbtree_node_t *node;
    
    pgbuf.element_vec = ptr;
    node = rbtree_near_prev(&(___piece_group_tree), &(pgbuf.rbnode));
    if (node == 0) {
        zcc_fatal("pm_pool::release(data), data is not mine");
    }
    pg = zcc_container_of(node, ___piece_group_t, rbnode);
    short int id = (ptr - pg->element_vec)/___element_size;
    if (id > (___element_count_of_group-1)) {
        zcc_fatal("pm_pool::release(data), data is not mine");
    }
    ((___element_id_t *)ptr)->id = pg->unused_id;
    pg->unused_id = id;
    pg->used_count--;
    ___element_used_count--;
    if (pg->used_count == ___element_count_of_group - 1) {
        zcc_mlink_append(___space_piece_group_head, ___space_piece_group_tail, pg, prev, next);
    }
    if (pg->used_count == 0) {
        zcc_mlink_detach(___space_piece_group_head, ___space_piece_group_tail, pg, prev, next);
        if ((___group_count * ___element_count_of_group) > (___element_count_of_group * 1.5)) {
            rbtree_detach(&(___piece_group_tree), &(pg->rbnode));
            ___piece_group_free(pg);
            ___group_count --;
        } else {
            if (___unused_piece_group) {
                rbtree_detach(&(___piece_group_tree), &(___unused_piece_group->rbnode));
                ___piece_group_free(___unused_piece_group);
            }
            ___unused_piece_group = pg;
        }
    }
}

}

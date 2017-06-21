/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-27
 * ================================
 */

#include "zcc.h"
#include<algorithm>

namespace zcc
{

struct common_mem_pool_set_t {
    unsigned short int setgroup_id;
    unsigned short int element_unused_id;
    unsigned short int element_unused_sum;
    char *element_list;
    rbtree_node_t rbnode;
    linker_node_t link_node;
};

struct common_mem_pool_setgroup_t {
    unsigned short int setgroup_id;
    unsigned short int element_size;
    unsigned short int element_count_per_set;
    int element_unused_sum;
    linker_t set_used;
    linker_t set_unused;
};

typedef struct {
    unsigned short int e_id;
} common_mem_pool_t_USI;

static size_t default_register_list[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 0 };
static int common_mem_pool_set_cmp(rbtree_node_t * rn1, rbtree_node_t * rn2)
{
    common_mem_pool_set_t *set1, *set2;

    set1 = ZCC_CONTAINER_OF(rn1, common_mem_pool_set_t, rbnode);
    set2 = ZCC_CONTAINER_OF(rn2, common_mem_pool_set_t, rbnode);

    return (set1->element_list - set2->element_list);
}

static int common_mem_pool_sys_cmp(rbtree_node_t * rn1, rbtree_node_t * rn2)
{
    return (rn1 - rn2);
}

common_mem_pool_set_t *common_mem_pool::set_create(common_mem_pool_setgroup_t * setgroup)
{
    int i;
    char *data;
    common_mem_pool_set_t *set;
    common_mem_pool_t_USI *usi;

    data = (char *)zcc::malloc(sizeof(common_mem_pool_set_t) + setgroup->element_size * setgroup->element_count_per_set);
    set = (common_mem_pool_set_t *) data;
    memset(set, 0, sizeof(common_mem_pool_set_t));

    set->element_list = data + sizeof(common_mem_pool_set_t);
    for (i = 0; i < setgroup->element_count_per_set; i++) {
        usi = (common_mem_pool_t_USI *) (set->element_list + setgroup->element_size * i);
        usi->e_id = (unsigned short int)(i + 1);
    }
    set->element_unused_id = 0;
    set->element_unused_sum = setgroup->element_count_per_set;

    return set;
}

common_mem_pool_set_t *common_mem_pool::set_find(const void *const_ptr)
{
    common_mem_pool_setgroup_t *setgroup;
    common_mem_pool_set_t *set, rs;
    rbtree_node_t *rn;
    char *ptr = (char *)(const_cast <void *>(const_ptr));

    rs.element_list = ptr + 1;
    rn = rbtree_near_prev(&set_rbtree, &(rs.rbnode));
    if (rn == 0) {
        return 0;
    }

    set = ZCC_CONTAINER_OF(rn, common_mem_pool_set_t, rbnode);
    setgroup = setgroup_list + set->setgroup_id;

    if ((ptr - (char *)(set->element_list)) > ((unsigned int)(setgroup->element_count_per_set) * (unsigned int)(setgroup->element_size))) {
        return 0;
    }

    return set;
}

void common_mem_pool::free_by_set(common_mem_pool_set_t * set, const void *ptr)
{
    common_mem_pool_setgroup_t *setgroup;
    common_mem_pool_t_USI *usi;

    setgroup = setgroup_list + set->setgroup_id;

    usi = (common_mem_pool_t_USI *) ptr;
    usi->e_id = set->element_unused_id;
    set->element_unused_id = (unsigned short int)((((char *)ptr) - set->element_list) / setgroup->element_size);
    set->element_unused_sum++;

    setgroup->element_unused_sum++;

    if (set->element_unused_sum == 1) {
        linker_unshift(&(setgroup->set_used), &(set->link_node));
    } else if (set->element_unused_sum == setgroup->element_count_per_set) {
        linker_detach(&(setgroup->set_used), &(set->link_node));
        linker_unshift(&(setgroup->set_unused), &(set->link_node));
    }

    if (setgroup->element_unused_sum > setgroup->element_count_per_set * 2) {
        if (setgroup->set_unused.head) {
            set = ZCC_CONTAINER_OF(setgroup->set_unused.head, common_mem_pool_set_t, link_node);
            rbtree_detach(&set_rbtree, &(set->rbnode));
            linker_detach(&(setgroup->set_unused), &(set->link_node));
            zcc::free(set);
            setgroup->element_unused_sum -= setgroup->element_count_per_set;
        }
    }
}

/* ################################################################## */
common_mem_pool::common_mem_pool()
{
}

common_mem_pool::common_mem_pool(size_t *register_size_list)
{
    init(register_size_list);
}

common_mem_pool::~common_mem_pool()
{
    rbtree_node_t *rn;
    common_mem_pool_set_t *set;

    while ((rn = rbtree_first(&set_rbtree))) {
        set = ZCC_CONTAINER_OF(rn, common_mem_pool_set_t, rbnode);
        rbtree_detach(&set_rbtree, rn);
        zcc::free(set);
    }
    zcc::free(setgroup_list);

    while ((rn = rbtree_first(&sys_rbtree))) {
        rbtree_detach(&sys_rbtree, rn);
        zcc::free(rn);
    }
}

void common_mem_pool::init(size_t *register_size_list)
{
    common_mem_pool_setgroup_t *setgroup;
    size_t *register_one, element_size;
    int i, ecount;

    if (register_size_list == 0) {
        register_size_list = default_register_list;
    }

    setgroup_count = 0;
    for (register_one = register_size_list; *register_one; register_one++) {
        setgroup_count++;
    }
    register_size_list = (size_t *)zcc::memdup(register_size_list, sizeof(size_t) *  (setgroup_count + 1));
    std::sort(register_size_list, register_size_list + setgroup_count);

    register_one = register_size_list;
    for (; *register_one; register_one++) {
        element_size = *register_one;
        if ((unsigned short int)element_size < sizeof(unsigned short int)) {
            log_fatal("common_mem_pool: element_size must > %d ", (int)(sizeof(unsigned short int)));
        }
    }
    setgroup_list = (common_mem_pool_setgroup_t *)zcc::calloc(sizeof(common_mem_pool_setgroup_t), setgroup_count);

    for (i = 0; i < setgroup_count; i++) {
        element_size = register_size_list[i];
        setgroup = setgroup_list + i;

        setgroup->setgroup_id = i;
        setgroup->element_size = element_size;
        ecount = 10 * 1024 / setgroup->element_size;
        if (ecount < 100) {
            ecount = 100;
        }
        if (ecount > 4096) {
            ecount = 4096;
        }
        setgroup->element_count_per_set = ecount;

        linker_init(&(setgroup->set_used));
        linker_init(&(setgroup->set_unused));
    }
    rbtree_init(&set_rbtree, common_mem_pool_set_cmp);
    rbtree_init(&sys_rbtree, common_mem_pool_sys_cmp);
    zcc::free(register_size_list);
}

void *common_mem_pool::malloc(size_t size)
{
    common_mem_pool_setgroup_t *setgroup;
    common_mem_pool_set_t *set;
    common_mem_pool_t_USI *usi;
    unsigned short int e_id;
    char *ptr;

    if (size < 1) {
        return blank_buffer;
    }

    if (size > (setgroup_list[setgroup_count - 1].element_size)) {
        ptr =  (char *)zcc::malloc(sizeof(rbtree_node_t) + size);
        rbtree_attach(&sys_rbtree, (rbtree_node_t *)&ptr);
        return (void *) (ptr + sizeof(rbtree_node_t));
    }

    setgroup = 0;
    {
        /* find setgroup */
        int left, right, center;
        common_mem_pool_setgroup_t *center_node;
        left = 0;
        right = setgroup_count - 1;
        while (1) {
            if (left == right) {
                setgroup = setgroup_list + left;
                break;
            }
            center = (left + right) / 2;
            center_node = setgroup_list + center;

            if (center_node->element_size == size) {
                setgroup = center_node;
                break;
            }
            if (center_node->element_size < size) {
                left = center + 1;
                continue;
            }
            right = right - 1;
        }
    }

    if (setgroup->set_used.head == 0) {
        if (setgroup->set_unused.head) {
            set = ZCC_CONTAINER_OF(setgroup->set_unused.head, common_mem_pool_set_t, link_node);
            linker_detach(&(setgroup->set_unused), setgroup->set_unused.head);
        } else {
            set = set_create(setgroup);
            rbtree_attach(&set_rbtree, &(set->rbnode));
            setgroup->element_unused_sum += setgroup->element_count_per_set;
        }
        linker_push(&(setgroup->set_used), &(set->link_node));
    }
    setgroup->element_unused_sum--;

    set = ZCC_CONTAINER_OF(setgroup->set_used.head, common_mem_pool_set_t, link_node);
    e_id = set->element_unused_id;
    ptr = (char *)(set->element_list + e_id * setgroup->element_size);
    usi = (common_mem_pool_t_USI *) ptr;
    set->element_unused_id = usi->e_id;
    set->element_unused_sum--;

    if (set->element_unused_sum == 0) {
        linker_detach(&(setgroup->set_used), &(set->link_node));
    }

    return (void *)ptr;
}

void common_mem_pool::free(const void *void_ptr)
{
    common_mem_pool_set_t *set;
    char *ptr = (char *)void_ptr;

    if (!ptr) {
        return;
    }
    if (ptr == blank_buffer) {
        return;
    }

    set = set_find(ptr);
    if (!set) {
        rbtree_node_t *old_n;
        old_n = rbtree_detach(&sys_rbtree, (rbtree_node_t *)(ptr - sizeof(rbtree_node_t)));
        zcc::free(old_n);
        return;
    }

    free_by_set(set, ptr);
}

void *common_mem_pool::realloc(const void *ptr, size_t size)
{
    common_mem_pool_set_t *set;
    common_mem_pool_setgroup_t *setgroup;
    char *rp;

    if (!ptr) {
        return malloc(size);
    }
    if (ptr == blank_buffer) {
        return malloc(size);
    }

    set = set_find(ptr);
    if (!set) {
        rbtree_node_t *old_n, *new_n;
        old_n = rbtree_detach(&sys_rbtree, (rbtree_node_t *)((char *)ptr - sizeof(rbtree_node_t)));
        rp = (char *)zcc::realloc(old_n, sizeof(rbtree_node_t) + size);
        new_n = (rbtree_node_t *)rp;
        if (old_n == new_n) {
            rbtree_attach(&sys_rbtree, old_n);
        } else {
            rbtree_attach(&sys_rbtree, new_n);
        }

        return (void *)(rp + sizeof(rbtree_node_t));
    }

    setgroup = setgroup_list + set->setgroup_id;
    if (setgroup->element_size <= size) {
        return (void *)ptr;
    }

    rp = (char *)malloc(size);
    memcpy(rp, ptr, setgroup->element_size);
    free_by_set(set, ptr);

    return (void *)rp;
}

}

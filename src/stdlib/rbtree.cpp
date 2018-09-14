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

#define ZCC_RBTREE_PARENT(node)    ((rbtree_node_t *)((node)->__rbtree_parent_color & ~3))

static inline void rbtree_link_node(rbtree_node_t * node, rbtree_node_t * parent, rbtree_node_t ** rbtree_link)
{
    node->__rbtree_parent_color = (unsigned long)parent;
    node->rbtree_left = node->rbtree_right = 0;

    *rbtree_link = node;
}
static void __rbtree_insert_augmented(rbtree_t * root, rbtree_node_t * node, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new));
static void __rbtree_detach_color(rbtree_t * root, rbtree_node_t * parent, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new));

#define ZCC_RB_RED          0
#define ZCC_RB_BLACK        1

#define  rbtree_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)  ((root)->rbtree_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbree */
#define RB_EMPTY_NODE(node)  \
    ((node)->__rbtree_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node)  \
    ((node)->__rbtree_parent_color = (unsigned long)(node))

struct rbtree_augment_callbacks {
    void (*propagate) (rbtree_node_t * node, rbtree_node_t * stop);
    void (*copy) (rbtree_node_t * old, rbtree_node_t * _new);
    void (*rotate) (rbtree_node_t * old, rbtree_node_t * _new);
};

static inline void rbtree_insert_augmented(rbtree_node_t * node, rbtree_t * root, const struct rbtree_augment_callbacks *augment)
{
    __rbtree_insert_augmented(root, node, augment->rotate);
}

#define RB_DECLARE_CALLBACKS(rbstatic, rbname, rbstruct, rbfield,    \
                 rbtype, rbaugmented, rbcompute)        \
static inline void                            \
rbname ## _propagate(rbtree_node_t *rb, rbtree_node_t *stop)        \
{                                    \
    while (rb != stop) {                        \
        rbstruct *node = rbtree_entry(rb, rbstruct, rbfield);    \
        rbtype augmented = rbcompute(node);            \
        if (node->rbaugmented == augmented)            \
            break;                        \
        node->rbaugmented = augmented;                \
        rb = ZCC_RBTREE_PARENT(&node->rbfield);                \
    }                                \
}                                    \
static inline void                            \
rbname ## _copy(rbtree_node_t *rbtree_old, rbtree_node_t *rbtree_new)        \
{                                    \
    rbstruct *old = rbtree_entry(rbtree_old, rbstruct, rbfield);        \
    rbstruct *_new = rbtree_entry(rbtree_new, rbstruct, rbfield);        \
    _new->rbaugmented = old->rbaugmented;                \
}                                    \
static void                                \
rbname ## _rotate(rbtree_node_t *rbtree_old, rbtree_node_t *rbtree_new)    \
{                                    \
    rbstruct *old = rbtree_entry(rbtree_old, rbstruct, rbfield);        \
    rbstruct *_new = rbtree_entry(rbtree_new, rbstruct, rbfield);        \
    _new->rbaugmented = old->rbaugmented;                \
    old->rbaugmented = rbcompute(old);                \
}                                    \
rbstatic const struct rbtree_augment_callbacks rbname = {            \
    rbname ## _propagate, rbname ## _copy, rbname ## _rotate    \
};

#define    ZCC_RB_RED        0
#define    ZCC_RB_BLACK    1

#define __ZCC_RBTREE_PARENT(pc)    ((rbtree_node_t *)(pc & ~3))

#define ___zcc_rbtree_color(pc)     ((pc) & 1)
#define __zcc_rbtree_is_black(pc)  ___zcc_rbtree_color(pc)
#define __zcc_rbtree_is_red(pc)    (!___zcc_rbtree_color(pc))
#define zcc_rbtree_color(rb)       ___zcc_rbtree_color((rb)->__rbtree_parent_color)
#define zcc_rbtree_is_red(rb)      __zcc_rbtree_is_red((rb)->__rbtree_parent_color)
#define zcc_rbtree_is_black(rb)    __zcc_rbtree_is_black((rb)->__rbtree_parent_color)

static inline void rbtree_set_parent(rbtree_node_t * rb, rbtree_node_t * p)
{
    rb->__rbtree_parent_color = zcc_rbtree_color(rb) | (unsigned long)p;
}

static inline void rbtree_set_parent_color(rbtree_node_t * rb, rbtree_node_t * p, int color)
{
    rb->__rbtree_parent_color = (unsigned long)p | color;
}

static inline void __rbtree_change_child(rbtree_node_t * old, rbtree_node_t * _new, rbtree_node_t * parent, rbtree_t * root)
{
    if (parent) {
        if (parent->rbtree_left == old)
            parent->rbtree_left = _new;
        else
            parent->rbtree_right = _new;
    } else
        root->rbtree_node = _new;
}

static __always_inline rbtree_node_t *__rbtree_detach_augmented(rbtree_node_t * node, rbtree_t * root, const struct rbtree_augment_callbacks *augment)
{
    rbtree_node_t *child = node->rbtree_right, *tmp = node->rbtree_left;
    rbtree_node_t *parent, *rebalance;
    unsigned long pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __rbtree_detach_color() later on.
         */
        pc = node->__rbtree_parent_color;
        parent = __ZCC_RBTREE_PARENT(pc);
        __rbtree_change_child(node, child, parent, root);
        if (child) {
            child->__rbtree_parent_color = pc;
            rebalance = NULL;
        } else
            rebalance = __zcc_rbtree_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->rbtree_left */
        tmp->__rbtree_parent_color = pc = node->__rbtree_parent_color;
        parent = __ZCC_RBTREE_PARENT(pc);
        __rbtree_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    } else {
        rbtree_node_t *successor = child, *child2;
        tmp = child->rbtree_left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->rbtree_right;
            augment->copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rbtree_left;
            } while (tmp);
            parent->rbtree_left = child2 = successor->rbtree_right;
            successor->rbtree_right = child;
            rbtree_set_parent(child, successor);
            augment->copy(node, successor);
            augment->propagate(parent, successor);
        }

        successor->rbtree_left = tmp = node->rbtree_left;
        rbtree_set_parent(tmp, successor);

        pc = node->__rbtree_parent_color;
        tmp = __ZCC_RBTREE_PARENT(pc);
        __rbtree_change_child(node, successor, tmp, root);
        if (child2) {
            successor->__rbtree_parent_color = pc;
            rbtree_set_parent_color(child2, parent, ZCC_RB_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2 = successor->__rbtree_parent_color;
            successor->__rbtree_parent_color = pc;
            rebalance = __zcc_rbtree_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    augment->propagate(tmp, NULL);
    return rebalance;
}

static __always_inline void rbtree_detach_augmented(rbtree_node_t * node, rbtree_t * root, const struct rbtree_augment_callbacks *augment)
{
    rbtree_node_t *rebalance = __rbtree_detach_augmented(node, root, augment);
    if (rebalance)
        __rbtree_detach_color(root, rebalance, augment->rotate);
}

static inline void rbtree_set_black(rbtree_node_t * rb)
{
    rb->__rbtree_parent_color |= ZCC_RB_BLACK;
}

static inline rbtree_node_t *rbtree_red_parent(rbtree_node_t * red)
{
    return (rbtree_node_t *) red->__rbtree_parent_color;
}

/*
 * Helper function for rotations:
 * - old's parent and color get assigned to _new
 * - old gets assigned _new as a parent and 'color' as a color.
 */
static inline void __rbtree_rotate_set_parents(rbtree_node_t * old, rbtree_node_t * _new, rbtree_t * root, int color)
{
    rbtree_node_t *parent = ZCC_RBTREE_PARENT(old);
    _new->__rbtree_parent_color = old->__rbtree_parent_color;
    rbtree_set_parent_color(old, _new, color);
    __rbtree_change_child(old, _new, parent, root);
}

static __always_inline void __rbtree_insert(rbtree_node_t * node, rbtree_t * root, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new))
{
    rbtree_node_t *parent = rbtree_red_parent(node), *gparent, *tmp;

    while (1) {
        /*
         * Loop invariant: node is red
         *
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as we don't
         * want a red root or two consecutive red nodes.
         */
        if (!parent) {
            rbtree_set_parent_color(node, NULL, ZCC_RB_BLACK);
            break;
        } else if (zcc_rbtree_is_black(parent))
            break;

        gparent = rbtree_red_parent(parent);

        tmp = gparent->rbtree_right;
        if (parent != tmp) {    /* parent == gparent->rbtree_left */
            if (tmp && zcc_rbtree_is_red(tmp)) {
                /*
                 * Case 1 - color flips
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            N
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                rbtree_set_parent_color(tmp, gparent, ZCC_RB_BLACK);
                rbtree_set_parent_color(parent, gparent, ZCC_RB_BLACK);
                node = gparent;
                parent = ZCC_RBTREE_PARENT(node);
                rbtree_set_parent_color(node, parent, ZCC_RB_RED);
                continue;
            }

            tmp = parent->rbtree_right;
            if (node == tmp) {
                /*
                 * Case 2 - left rotate at parent
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                parent->rbtree_right = tmp = node->rbtree_left;
                node->rbtree_left = parent;
                if (tmp)
                    rbtree_set_parent_color(tmp, parent, ZCC_RB_BLACK);
                rbtree_set_parent_color(parent, node, ZCC_RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->rbtree_right;
            }

            /*
             * Case 3 - right rotate at gparent
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gparent->rbtree_left = tmp;    /* == parent->rbtree_right */
            parent->rbtree_right = gparent;
            if (tmp)
                rbtree_set_parent_color(tmp, gparent, ZCC_RB_BLACK);
            __rbtree_rotate_set_parents(gparent, parent, root, ZCC_RB_RED);
            augment_rotate(gparent, parent);
            break;
        } else {
            tmp = gparent->rbtree_left;
            if (tmp && zcc_rbtree_is_red(tmp)) {
                /* Case 1 - color flips */
                rbtree_set_parent_color(tmp, gparent, ZCC_RB_BLACK);
                rbtree_set_parent_color(parent, gparent, ZCC_RB_BLACK);
                node = gparent;
                parent = ZCC_RBTREE_PARENT(node);
                rbtree_set_parent_color(node, parent, ZCC_RB_RED);
                continue;
            }

            tmp = parent->rbtree_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                parent->rbtree_left = tmp = node->rbtree_right;
                node->rbtree_right = parent;
                if (tmp)
                    rbtree_set_parent_color(tmp, parent, ZCC_RB_BLACK);
                rbtree_set_parent_color(parent, node, ZCC_RB_RED);
                augment_rotate(parent, node);
                parent = node;
                tmp = node->rbtree_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->rbtree_right = tmp;   /* == parent->rbtree_left */
            parent->rbtree_left = gparent;
            if (tmp)
                rbtree_set_parent_color(tmp, gparent, ZCC_RB_BLACK);
            __rbtree_rotate_set_parents(gparent, parent, root, ZCC_RB_RED);
            augment_rotate(gparent, parent);
            break;
        }
    }
}

/*
 * Inline version for rbtree_detach() use - we want to be able to inline
 * and eliminate the dummy_rotate callback there
 */
static __always_inline void ____rbtree_detach_color(rbtree_node_t * parent, rbtree_t * root, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new))
{
    rbtree_node_t *node = NULL, *sibling, *tmp1, *tmp2;

    while (1) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->rbtree_right;
        if (node != sibling) {  /* node == parent->rbtree_left */
            if (zcc_rbtree_is_red(sibling)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                parent->rbtree_right = tmp1 = sibling->rbtree_left;
                sibling->rbtree_left = parent;
                rbtree_set_parent_color(tmp1, parent, ZCC_RB_BLACK);
                __rbtree_rotate_set_parents(parent, sibling, root, ZCC_RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->rbtree_right;
            if (!tmp1 || zcc_rbtree_is_black(tmp1)) {
                tmp2 = sibling->rbtree_left;
                if (!tmp2 || zcc_rbtree_is_black(tmp2)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    rbtree_set_parent_color(sibling, parent, ZCC_RB_RED);
                    if (zcc_rbtree_is_red(parent))
                        rbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZCC_RBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   Sl
                 *     / \             \
                 *    sl  Sr            s
                 *                       \
                 *                        Sr
                 */
                sibling->rbtree_left = tmp1 = tmp2->rbtree_right;
                tmp2->rbtree_right = sibling;
                parent->rbtree_right = tmp2;
                if (tmp1)
                    rbtree_set_parent_color(tmp1, sibling, ZCC_RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            parent->rbtree_right = tmp2 = sibling->rbtree_left;
            sibling->rbtree_left = parent;
            rbtree_set_parent_color(tmp1, sibling, ZCC_RB_BLACK);
            if (tmp2)
                rbtree_set_parent(tmp2, parent);
            __rbtree_rotate_set_parents(parent, sibling, root, ZCC_RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        } else {
            sibling = parent->rbtree_left;
            if (zcc_rbtree_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                parent->rbtree_left = tmp1 = sibling->rbtree_right;
                sibling->rbtree_right = parent;
                rbtree_set_parent_color(tmp1, parent, ZCC_RB_BLACK);
                __rbtree_rotate_set_parents(parent, sibling, root, ZCC_RB_RED);
                augment_rotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->rbtree_left;
            if (!tmp1 || zcc_rbtree_is_black(tmp1)) {
                tmp2 = sibling->rbtree_right;
                if (!tmp2 || zcc_rbtree_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    rbtree_set_parent_color(sibling, parent, ZCC_RB_RED);
                    if (zcc_rbtree_is_red(parent))
                        rbtree_set_black(parent);
                    else {
                        node = parent;
                        parent = ZCC_RBTREE_PARENT(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - right rotate at sibling */
                sibling->rbtree_right = tmp1 = tmp2->rbtree_left;
                tmp2->rbtree_left = sibling;
                parent->rbtree_left = tmp2;
                if (tmp1)
                    rbtree_set_parent_color(tmp1, sibling, ZCC_RB_BLACK);
                augment_rotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - left rotate at parent + color flips */
            parent->rbtree_left = tmp2 = sibling->rbtree_right;
            sibling->rbtree_right = parent;
            rbtree_set_parent_color(tmp1, sibling, ZCC_RB_BLACK);
            if (tmp2)
                rbtree_set_parent(tmp2, parent);
            __rbtree_rotate_set_parents(parent, sibling, root, ZCC_RB_BLACK);
            augment_rotate(parent, sibling);
            break;
        }
    }
}

/* Non-inline version for rbtree_detach_augmented() use */
static void __rbtree_detach_color(rbtree_t * root, rbtree_node_t * parent, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new))
{
    ____rbtree_detach_color(parent, root, augment_rotate);
}

/*
 * Non-augmented rbtree manipulation functions.
 *
 * We use dummy augmented callbacks here, and have the compiler optimize them
 * out of the rbtree_insert_color() and rbtree_detach() function definitions.
 */

static inline void dummy_propagate(rbtree_node_t * node, rbtree_node_t * stop)
{
}

static inline void dummy_copy(rbtree_node_t * old, rbtree_node_t * _new)
{
}

static inline void dummy_rotate(rbtree_node_t * old, rbtree_node_t * _new)
{
}

static const struct rbtree_augment_callbacks dummy_callbacks = {
    dummy_propagate, dummy_copy, dummy_rotate
};

void rbtree_insert_color(rbtree_t * root, rbtree_node_t * node)
{
    __rbtree_insert(node, root, dummy_rotate);
}

rbtree_node_t * rbtree_detach(rbtree_t * root, rbtree_node_t * node)
{
    rbtree_node_t *rebalance;
    rebalance = __rbtree_detach_augmented(node, root, &dummy_callbacks);
    if (rebalance)
        ____rbtree_detach_color(rebalance, root, dummy_rotate);

    return node;
}

/*
 * Augmented rbtree manipulation functions.
 *
 * This instantiates the same __always_inline functions as in the non-augmented
 * case, but this time with user-defined callbacks.
 */

void __rbtree_insert_augmented(rbtree_t * root, rbtree_node_t * node, void (*augment_rotate) (rbtree_node_t * old, rbtree_node_t * _new))
{
    __rbtree_insert(node, root, augment_rotate);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
rbtree_node_t *rbtree_first(rbtree_t * root)
{
    rbtree_node_t *n;

    n = root->rbtree_node;
    if (!n)
        return NULL;
    while (n->rbtree_left)
        n = n->rbtree_left;
    return n;
}

rbtree_node_t *rbtree_last(rbtree_t * root)
{
    rbtree_node_t *n;

    n = root->rbtree_node;
    if (!n)
        return NULL;
    while (n->rbtree_right)
        n = n->rbtree_right;
    return n;
}

rbtree_node_t *rbtree_next(rbtree_node_t * node)
{
    rbtree_node_t *parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->rbtree_right) {
        node = node->rbtree_right;
        while (node->rbtree_left)
            node = node->rbtree_left;
        return (rbtree_node_t *) node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = ZCC_RBTREE_PARENT(node)) && node == parent->rbtree_right)
        node = parent;

    return parent;
}

rbtree_node_t *rbtree_prev(rbtree_node_t * node)
{
    rbtree_node_t *parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->rbtree_left) {
        node = node->rbtree_left;
        while (node->rbtree_right)
            node = node->rbtree_right;
        return (rbtree_node_t *) node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = ZCC_RBTREE_PARENT(node)) && node == parent->rbtree_left)
        node = parent;

    return parent;
}

void rbtree_replace_node(rbtree_t * root, rbtree_node_t * victim, rbtree_node_t * _new)
{
    rbtree_node_t *parent = ZCC_RBTREE_PARENT(victim);

    /* Set the surrounding nodes to point to the replacement */
    __rbtree_change_child(victim, _new, parent, root);
    if (victim->rbtree_left)
        rbtree_set_parent(victim->rbtree_left, _new);
    if (victim->rbtree_right)
        rbtree_set_parent(victim->rbtree_right, _new);

    /* Copy the pointers/colour from the victim to the replacement */
    *_new = *victim;
}

/* XXX added by zc*/

void rbtree_init(rbtree_t * tree, rbtree_cmp_t cmp_fn)
{
    tree->rbtree_node = 0;
    tree->cmp_fn = cmp_fn;
}

rbtree_node_t *rbtree_attach(rbtree_t * tree, rbtree_node_t * node)
{
    rbtree_node_t **_new = &(tree->rbtree_node), *parent = 0;
    int cmp_result;

    while (*_new) {
        parent = *_new;
        cmp_result = tree->cmp_fn(node, *_new);
        if (cmp_result < 0) {
            _new = &((*_new)->rbtree_left);
        } else if (cmp_result > 0) {
            _new = &((*_new)->rbtree_right);
        } else {
            return (*_new);
        }
    }
    rbtree_link_node(node, parent, _new);
    rbtree_insert_color(tree, node);

    return node;
}

rbtree_node_t *rbtree_find(rbtree_t * tree, rbtree_node_t * vnode)
{
    rbtree_node_t *node;
    int cmp_result;

    node = tree->rbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result < 0) {
            node = node->rbtree_left;
        } else if (cmp_result > 0) {
            node = node->rbtree_right;
        } else {
            return node;
        }
    }

    return 0;
}

rbtree_node_t *rbtree_near_prev(rbtree_t * tree, rbtree_node_t * vnode)
{
    rbtree_node_t *node, *ret_node;
    int cmp_result;

    ret_node = 0;
    node = tree->rbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result < 0) {
            node = node->rbtree_left;
        } else if (cmp_result > 0) {
            ret_node = node;
            node = node->rbtree_right;
        } else {
            return node;
            return rbtree_prev(node);
        }
    }
    return ret_node;
}

rbtree_node_t *rbtree_near_next(rbtree_t * tree, rbtree_node_t * vnode)
{
    rbtree_node_t *node, *ret_node;
    int cmp_result;

    ret_node = 0;
    node = tree->rbtree_node;
    while (node) {
        cmp_result = tree->cmp_fn(vnode, node);
        if (cmp_result > 0) {
            ret_node = node;
            node = node->rbtree_right;
        } else if (cmp_result < 0) {
            node = node->rbtree_left;
        } else {
            return node;
            return rbtree_next(node);
        }
    }
    return ret_node;
}

}

/* walk */
#if 0
#define zcc_rbtree_walk_begin(root, var_your_node) {                            \
struct { rbtree_node_t *node; unsigned char lrs; } ___Z_list[64];                \
rbtree_node_t *___Z_node = (root)->rbtree_node;                            \
int ___Z_idx = 0, ___Z_lrs;                                    \
___Z_list[0].node = ___Z_node;                                    \
___Z_list[0].lrs = 0;                                        \
while (1) {                                            \
    var_your_node = ___Z_node = ___Z_list[___Z_idx].node;                    \
    ___Z_lrs = ___Z_list[___Z_idx].lrs;                            \
    if (!___Z_node || ___Z_lrs == 2) {                            \
        if (___Z_node) {
#define zcc_rbtree_walk_end                                        \
        }                                        \
        ___Z_idx--;                                    \
        if (___Z_idx == -1){                                \
            break;                                    \
               }                                        \
        ___Z_list[___Z_idx].lrs++;                            \
        continue;                                    \
    }                                            \
    ___Z_idx++;                                        \
    ___Z_list[___Z_idx].lrs = 0;                                \
    ___Z_list[___Z_idx].node = ((___Z_lrs == 0) ? ___Z_node->rbtree_left : ___Z_node->rbtree_right);\
}                                                \
}
#endif

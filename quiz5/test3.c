#include <stddef.h>

#if !defined(container_of)
#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - offsetof(type, member)))
#endif

enum { RB_RED, RB_BLACK };
struct rb_node {
    unsigned long rb_parent_color;
    struct rb_node *rb_right, *rb_left;
} __attribute__((aligned(sizeof(long))));

struct rb_root {
    struct rb_node *rb_node;
};

#define rb_parent(r) ((struct rb_node *) ((r)->rb_parent_color & ~3))
#define rb_color(r) ((r)->rb_parent_color & 1)
#define rb_is_red(r) (!rb_color(r))
#define rb_is_black(r) rb_color(r)
#define rb_set_red(r)               \
    do {                            \
        (r)->rb_parent_color &= ~1; \
    } while (0)
#define rb_set_black(r)            \
    do {                           \
        (r)->rb_parent_color |= 1; \
    } while (0)

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long) p;
}

static inline void rb_set_color(struct rb_node *rb, int color)
{
    rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root) (!(root)->rb_node)

static inline void rb_link_node(struct rb_node *node,
                                struct rb_node *parent,
                                struct rb_node **rb_link)
{
    node->rb_parent_color = (unsigned long) parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}

static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = rb_parent(node);

    if ((node->rb_right = right->rb_left))
        rb_set_parent(right->rb_left, node);
    right->rb_left = node;

    rb_set_parent(right, parent);

    if (parent) {
        if (node == parent->rb_left)
            parent->rb_left = right;
        else
            parent->rb_right = right;
    } else {
        root->rb_node = right;
    }
    rb_set_parent(node, right);
}

static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = rb_parent(node);

    if ((node->rb_left = left->rb_right))
        rb_set_parent(left->rb_right, node);
    left->rb_right = node;

    rb_set_parent(left, parent);

    if (parent) {
        if (node == parent->rb_right)
            parent->rb_right = left;
        else
            parent->rb_left = left;
    } else {
        root->rb_node = left;
    }
    rb_set_parent(node, left);
}

void rb_insert_color(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *parent, *gparent;

    while ((parent = rb_parent(node)) && rb_is_red(parent)) {
        gparent = rb_parent(parent);

        if (parent == gparent->rb_left) {
            {
                struct rb_node *uncle = gparent->rb_right;
                if (uncle && rb_is_red(uncle)) {
                    rb_set_black(uncle);
                    rb_set_black(parent);
                    rb_set_red(gparent);
                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_right == node) {
                __rb_rotate_left(parent, root);
                struct rb_node *tmp = parent;
                parent = node;
                node = tmp;
            }

            rb_set_black(node);
            rb_set_red(parent);
            __rb_rotate_right(gparent, root);
        } else {
            {
                struct rb_node *uncle = gparent->rb_left;
                if (uncle && rb_is_red(uncle)) {
                    rb_set_black(uncle);
                    rb_set_black(parent);
                    rb_set_red(gparent);
                    node = gparent;
                    continue;
                }
            }

            if (parent->rb_left == node) {
                __rb_rotate_right(parent, root);
                struct rb_node *tmp = parent;
                parent = node;
                node = tmp;
            }

            rb_set_black(node);
            rb_set_red(parent);
            __rb_rotate_left(gparent, root);
        }
    }

    rb_set_black(root->rb_node);
}

static void __rb_erase_color(struct rb_node *node,
                             struct rb_node *parent,
                             struct rb_root *root)
{
    struct rb_node *other;

    while ((!node || rb_is_black(node)) && node != root->rb_node) {
        if (parent->rb_left == node) {
            other = parent->rb_right;
            if (rb_is_red(other)) {
                rb_set_black(other);
                rb_set_red(parent);
                __rb_rotate_left(parent, root);
                other = parent->rb_right;
            }
            if ((!other->rb_left || rb_is_black(other->rb_left)) &&
                (!other->rb_right || rb_is_black(other->rb_right))) {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!other->rb_right || rb_is_black(other->rb_right)) {
                    rb_set_black(other->rb_left);
                    rb_set_red(other);
                    __rb_rotate_left(other, root);
                    other = parent->rb_right;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->rb_right);
                __rb_rotate_left(other, root);
                node = root->rb_node;
                break;
            }
        } else {
            other = parent->rb_left;
            if (rb_is_red(other)) {
                rb_set_black(other);
                rb_set_red(parent);
                __rb_rotate_right(parent, root);
                other = parent->rb_left;
            }
            if ((!other->rb_left || rb_is_black(other->rb_left)) &&
                (!other->rb_right || rb_is_black(other->rb_right))) {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            } else {
                if (!other->rb_left || rb_is_black(other->rb_left)) {
                    rb_set_black(other->rb_right);
                    rb_set_red(other);
                    __rb_rotate_right(other, root);
                    other = parent->rb_left;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->rb_left);
                __rb_rotate_right(other, root);
                node = root->rb_node;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}

void rb_erase(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *child, *parent;
    int color;

    if (!node->rb_left)
        child = node->rb_right;
    else if (!node->rb_right)
        child = node->rb_left;
    else {
        struct rb_node *old = node, *left;

        node = node->rb_right;
        while ((left = node->rb_left))
            node = left;

        if (rb_parent(old)) {
            if (rb_parent(old)->rb_left == old)
                rb_parent(old)->rb_left = node;
            else
                rb_parent(old)->rb_right = node;
        } else {
            root->rb_node = node;
        }

        child = node->rb_right;
        parent = rb_parent(node);
        color = rb_color(node);

        if (parent == old) {
            parent = node;
        } else {
            if (child)
                rb_set_parent(child, parent);
            parent->rb_left = child;

            node->rb_right = old->rb_right;
            rb_set_parent(old->rb_right, node);
        }

        node->rb_parent_color = old->rb_parent_color;
        node->rb_left = old->rb_left;
        rb_set_parent(old->rb_left, node);

        goto color;
    }

    parent = rb_parent(node);
    color = rb_color(node);

    if (child)
        rb_set_parent(child, parent);
    if (parent) {
        if (parent->rb_left == node)
            parent->rb_left = child;
        else
            parent->rb_right = child;
    } else {
        root->rb_node = child;
    }

color:
    if (color == RB_RED)
        __rb_erase_color(child, parent, root);
}

/* Return the first (smallest) node in the tree. */
struct rb_node *rb_first(const struct rb_root *root)
{
    struct rb_node *n = root->rb_node;
    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}

/* Return the next node in an in-order traversal. */
struct rb_node *rb_next(const struct rb_node *node)
{
    struct rb_node *parent;

    if (rb_parent(node) == node)
        return NULL;

    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node = node->rb_left;
        return (struct rb_node *) node;
    }

    while ((parent = rb_parent(node)) && node == parent->rb_right)
        node = parent;

    return parent;
}

#include <string.h>

/*
 * Map implementation using a red-black tree.
 */
typedef struct rb_node map_node_t;

/* Function pointer types for key extraction and key comparison. */
typedef void *(*map_key_pt)(map_node_t *);
typedef int (*map_cmp_pt)(void *a, void *b);

/* Map structure encapsulating the red-black tree and function pointers. */
typedef struct {
    struct rb_root root;
    map_key_pt key_pt;
    map_cmp_pt cmp_pt;
} map_t;

#define map_data(ptr, type, member) rb_entry(ptr, type, member)

#define map_first(map) rb_first(&(map)->root)
#define map_next(node) rb_next(node)
#define map_empty(map) RB_EMPTY_ROOT(&(map)->root)

#define map_foreach(n, m) for ((n) = map_first(m); (n); (n) = map_next(n))
#define map_foreach_safe(n, next, m)                 \
    for ((n) = map_first(m); (n) && ({               \
                                 next = map_next(n); \
                                 1;                  \
                             });                     \
         (n) = (next))

/**
 * Default key comparison function.
 *
 * This function is used if no custom comparator is provided. It performs a
 * string comparison.
 *
 * @a : Pointer to the first key.
 * @b : Pointer to the second key.
 * Return Negative value if a < b, zero if a == b, positive value if a > b.
 */
static int _map_def_cmp(void *a, void *b)
{
    return strcmp((const char *) a, (const char *) b);
}

/**
 * Initializes the map structure by setting its red-black tree root to empty
 * and assigning the key extraction and key comparison functions.
 *
 * @map : Pointer to the map to initialize.
 * @key_pt : Function pointer to extract the key from a node.
 * @cmp_pt : Function pointer for key comparison. If NULL, a default
 * strcmp-based comparator is used.
 */
static inline void map_init(map_t *map, map_key_pt key_pt, map_cmp_pt cmp_pt)
{
    if (!cmp_pt)
        cmp_pt = _map_def_cmp;
    map->key_pt = key_pt;
    map->cmp_pt = cmp_pt;
    map->root.rb_node = NULL;
}

/**
 * Inserts a node with the specified key into the map. The insertion is based
 * on the comparison function. Duplicate keys are not allowed.
 *
 * @map : Pointer to the map.
 * @key : Pointer to the key.
 * @node : Pointer to the node to insert.
 * Return 0 on successful insertion, -1 if a duplicate key is found.
 */
static inline int map_push(map_t *map, void *key, map_node_t *node)
{
    map_node_t **pnode = &(map->root.rb_node);
    map_node_t *parent = NULL;

    while (*pnode) {
        int rc = map->cmp_pt(key, map->key_pt(*pnode));

        parent = *pnode;
        if (rc < 0)
            pnode = &((*pnode)->rb_left);
        else if (rc > 0)
            pnode = &((*pnode)->rb_right);
        else
            return -1; /* Duplicate key */
    }

    rb_link_node(node, parent, pnode);
    rb_insert_color(node, &map->root);

    return 0;
}

/**
 * Searches the map for a node matching the given key.
 *
 * @map Pointer to the map.
 * @key Pointer to the key to search for.
 * Return Pointer to the matching node if found, NULL otherwise.
 */
static inline map_node_t *map_find(map_t *map, void *key)
{
    map_node_t *node = map->root.rb_node;
    while (node) {
        int rc = map->cmp_pt(key, map->key_pt(node));
        if (rc < 0)
            node = node->rb_left;
        else if (rc > 0)
            node = node->rb_right;
        else
            return node;
    }
    return NULL;
}

/**
 * Removes the specified node from the map and rebalances the red-black tree.
 *
 * @map : Pointer to the map.
 * @node : Pointer to the node to remove.
 */
static inline void map_erase(map_t *map, map_node_t *node)
{
    rb_erase(node, &map->root);
}

/* Test program */

#include <stdio.h>
#include <stdlib.h>

/* Custom entry structure embedding a map node. */
typedef struct {
    map_node_t node;     /* Must be the first member if using container_of on node. */
    char *key;     /* Key used for ordering. */
    int value;           /* Associated value. */
} my_entry_t;

/* Extracts the key from the given node. */
static void *my_get_key(map_node_t *node)
{
    return (void *)(((my_entry_t *)node)->key);
}

int main(void)
{
    /* Initialize the map with our key extraction function and default comparator. */
    map_t map;
    map_init(&map, my_get_key, NULL);
    
    /* Allocate and initialize several entries. */
    my_entry_t *entry1 = malloc(sizeof(my_entry_t));
    entry1->key = "apple";
    entry1->value = 10;
    
    my_entry_t *entry2 = malloc(sizeof(my_entry_t));
    entry2->key = "banana";
    entry2->value = 20;
    
    my_entry_t *entry3 = malloc(sizeof(my_entry_t));
    entry3->key = "cherry";
    entry3->value = 30;
    
    /* Insert the entries into the map. */
    if (map_push(&map, entry1->key, &entry1->node) != 0)
        printf("Duplicate key: %s\n", entry1->key);
    if (map_push(&map, entry2->key, &entry2->node) != 0)
        printf("Duplicate key: %s\n", entry2->key);
    if (map_push(&map, entry3->key, &entry3->node) != 0)
        printf("Duplicate key: %s\n", entry3->key);
    
    /* Search for an entry by key ("banana") and then erase it. */
    map_node_t *node = map_find(&map, "banana");
    if (node) {
        my_entry_t *found_entry = container_of(node, my_entry_t, node);
        printf("Found entry: Key = %s, Value = %d\n", found_entry->key, found_entry->value);
        /* Erase the found entry from the map. */
        map_erase(&map, node);
        /* After erasing, free the memory for the entry. */
        free(found_entry);
    } else {
        printf("Key 'banana' not found.\n");
    }
    
    /* Traverse and print all remaining entries in the map in sorted order. */
    printf("All entries in the map after erasing 'banana':\n");
    map_foreach(node, &map) {
        my_entry_t *entry = container_of(node, my_entry_t, node);
        printf("Key: %s, Value: %d\n", entry->key, entry->value);
    }
    
    free(entry1);
    free(entry3);
    
    return 0;
}
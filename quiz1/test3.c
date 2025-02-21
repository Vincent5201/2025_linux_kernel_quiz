#include "list.h"

typedef struct __node {
    long value;
    struct list_head list;
} node_t;

void list_construct(struct list_head *list, int n)
{
    node_t *node = malloc(sizeof(node_t));
    node->value = n;
    list_add(&node->list, list);
}

void list_free(const struct list_head *head)
{
    node_t *entry, *safe;
    list_for_each_entry_safe (entry, safe, head, list) {
        free(entry);
    }
}

/* Verify if list is order */
static bool list_is_ordered(const struct list_head *head)
{
    int value = list_entry(head->next, node_t, list)->value;
    node_t *entry;
    list_for_each_entry (entry, head, list) {
        if (entry->value < value)
            return false;
        value = entry->value;
    }

    return true;
}

/* shuffle array, only work if n < RAND_MAX */
void shuffle(int *array, size_t n)
{
    if (n <= 0)
        return;

    for (size_t i = 0; i < n - 1; i++) {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

struct list_head *list_tail(struct list_head *head)
{
    while (head && head->next)
        head = head->next;
    return head;
}

int list_length(struct list_head *left)
{
    int n = 0;
    struct list_head *node;
    list_for_each(node, left) n++;
    return n;
}

static void rebuild_list_link(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *node, *prev;
    prev = head;
    node = head->next;
    while (node) {
        node->prev = prev;
        prev = node;
        node = node->next;
    }
    prev->next = head;
    head->prev = prev;
}

void quick_sort(struct list_head *list)
{
    int n = list_length(list);
    int value;
    int i = 0;
    int max_level = 2 * n;
    struct list_head *begin[max_level];
    struct list_head *result = NULL, *left = NULL, *right = NULL;
    begin[0] = list->next;
    list->prev->next = NULL;
    while (i >= 0) {
        struct list_head *L = begin[i], *R = list_tail(begin[i]);
        if (L != R) {
            struct list_head *pivot = L;
            value = list_entry(pivot->next, node_t, list)->value;
            struct list_head *p = pivot->next;
            pivot->next = NULL; /* break the list */

            while (p) {
                struct list_head *n = p;
                p = p->next;
                int n_value = list_entry(n->next, node_t, list)->value;
                if (n_value > value) {
                    n->next = right;
                    right = n;
                } else {
                    n->next = left;
                    left = n;
                }
            }

            begin[i] = left;
            begin[i + 1] = pivot;
            begin[i + 2] = right;
            left = right = NULL;
            i += 2;
        } else {
            if (L) {
                L->next = result;
                result = L;
            }
            i--;
        }
    }
    list->next = result;
    rebuild_list_link(list);
}

int main(int argc, char **argv)
{
    struct list_head *list = malloc(sizeof(struct list_head));
    INIT_LIST_HEAD(list);

    size_t count = 100000;
    int *test_arr = malloc(sizeof(int) * count);
    for (int i = 0; i < count; ++i)
        test_arr[i] = i;
    shuffle(test_arr, count);

    while (count--)
        list_construct(list, test_arr[count]);
    quick_sort(list);
    assert(list_is_ordered(list));
    list_free(list);
    free(test_arr);
    return 0;
}
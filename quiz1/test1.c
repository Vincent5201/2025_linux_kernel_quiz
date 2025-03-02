#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct list_item {
    int value;
    struct list_item *next;
} list_item_t;

typedef struct {
    struct list_item *head;
} list_t;

#define my_assert(test, message) \
    do {                         \
        if (!(test))             \
            return message;      \
    } while (0)
#define my_run_test(test)       \
    do {                        \
        char *message = test(); \
        tests_run++;            \
        if (message)            \
            return message;     \
    } while (0)

#define N 1000

static list_item_t items[N];
static list_t l;

static list_t *list_reset(void)
{
    for (size_t i = 0; i < N; i++) {
        items[i].value = i;
        items[i].next = NULL;
    }
    l.head = NULL;
    return &l;
}

static void list_insert_before(list_t *l, list_item_t *before, list_item_t *item)
{
    list_item_t **p;
    for (p = &(l->head); *p != before; p = &(*p)->next)
        ;
    *p = item;
    (*p)->next = before;
}

static int list_size(list_t *l)
{
    if (!l || !(l->head))
        return 0;
    int count = 0;
    list_item_t *n = l->head;
    while (n) {
        count++;
        n = n->next;
    }
    return count;
}

static char *test_list(void)
{
    /* Test inserting at the beginning */
    list_reset();
    my_assert(list_size(&l) == 0, "Initial list size is expected to be zero.");
    for (size_t i = 0; i < N; i++)
        list_insert_before(&l, l.head, &items[i]);
    my_assert(list_size(&l) == N, "Final list size should be N");
    size_t k = N - 1;
    list_item_t *cur = l.head;
    while (cur) {
        my_assert(cur->value == k, "Unexpected list item value");
        k--;
        cur = cur->next;
    }

    /* Test inserting at the end */
    list_reset();
    my_assert(list_size(&l) == 0, "Initial list size is expected to be zero.");
    for (size_t i = 0; i < N; i++)
        list_insert_before(&l, NULL, &items[i]);
    my_assert(list_size(&l) == N, "Final list size should be N");
    k = 0;
    cur = l.head;
    while (cur) {
        my_assert(cur->value == k, "Unexpected list item value");
        k++;
        cur = cur->next;
    }

    /* Reset the list and insert elements in order (i.e. at the end) */
    list_reset();
    my_assert(list_size(&l) == 0, "Initial list size is expected to be zero.");
    for (size_t i = 0; i < N; i++)
        list_insert_before(&l, NULL, &items[i]);
    my_assert(list_size(&l) == N, "list size should be N");

    return NULL;
}

int tests_run = 0;

static char *test_suite(void)
{
    my_run_test(test_list);
    return NULL;
}

int main(void)
{
    printf("---=[ List tests\n");
    char *result = test_suite();
    if (result)
        printf("ERROR: %s\n", result);
    else
        printf("ALL TESTS PASSED\n");
    printf("Tests run: %d\n", tests_run);
    return !!result;
}

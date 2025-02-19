#include <stddef.h>
typedef struct list_item {
    int value;
    struct list_item *next;
} list_item_t;

typedef struct {
    struct list_item *head;
} list_t;
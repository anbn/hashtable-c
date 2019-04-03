#include <stdint.h>

/* HT_BASE_SIZE defines the minimal size of the hash_table
 * from which it will grow. The size will never fall below
 * this value when shrinking */
#define HT_BASE_SIZE 16

/* Resizing parameters, double or half hash_table size when
 * the ratio count/size*100 HT_MIN_FILL or exceeds HT_MAX_FILL
 * in percent. */
#define HT_MIN_FILL  10
#define HT_MAX_FILL  70

/* HT_INTERLINK if defined, the items will be linked according to
 * their insertion order via next_insert and prev_insert members,
 * which makes iterating the items straightforward. */
#define HT_INTERLINK 1

typedef struct ht_item ht_item;
struct ht_item {
    unsigned long hash;
    char* key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    ht_item* next;

#if HT_INTERLINK
    ht_item* next_insert, *prev_insert;
#endif
};

typedef struct ht_type ht_type;
struct ht_type {
    void *(*valdup)(const void *obj);
    void (*valfree)(void *obj);
};

typedef struct ht_hash_table ht_hash_table;
struct ht_hash_table {
    ht_type *type;
    unsigned long size;
    unsigned long count;
    ht_item** items;

#if HT_INTERLINK
    ht_item *oldest, *newest;
#endif
};

/* API */
ht_hash_table* ht_new(ht_type *type);
void ht_del_hash_table(ht_hash_table* ht);

ht_item* ht_search_raw(ht_hash_table *ht, const char *key);
ht_item* ht_insert_raw(ht_hash_table *ht, const char* key);
void* ht_search(ht_hash_table* ht, const char* key);
int ht_insert(ht_hash_table* ht, const char* key, const void* value);
int ht_delete(ht_hash_table* h, const char* key);

/* Debugging */
void ht_print(ht_hash_table *ht);

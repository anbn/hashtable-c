#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hash_table.h"

#if 0
#define dprint(...)                                              \
  do {                                                           \
    printf("%s:%d %s():  \t", __FILE__, __LINE__, __FUNCTION__); \
    printf(__VA_ARGS__);                                         \
    fflush(stdout);                                              \
  } while (0);
#else
#define dprint(...)
#endif

/* djb2 hash by Dan Bernstein
 * see http://www.cse.yorku.ca/~oz/hash.html */
static unsigned long
ht_get_hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

/* creates a new empty item for a given key and its hash
 * returns the newly allocated item, only to be called internally */
static ht_item*
ht_new_item(ht_hash_table *ht, const char* key, unsigned long hash)
{
    ht_item* i = malloc(sizeof(*i));
    if (i) {
        i->hash = hash;
        i->v.val = 0;
        i->next = NULL;
        i->key = malloc(strlen(key)+1);
        if (!i->key) return NULL;
        strcpy(i->key, key);
#if HT_INTERLINK
        if (ht->oldest==NULL) ht->oldest = i;
        if (ht->newest!=NULL) ht->newest->next_insert = i;
        i->prev_insert = ht->newest;
        i->next_insert = NULL;
        ht->newest = i;
#endif
    }
    return i;
}

/* creates a sized hashtable with size base_size
 * return the newly allocated hashtable, only to be called internally */
static ht_hash_table*
ht_new_sized(ht_type *type, const int base_size)
{
    ht_hash_table* ht = malloc(sizeof(*ht)*base_size);
    if (ht) {
        ht->type = type;
        ht->size = base_size;
        ht->items = calloc(ht->size, sizeof(*ht->items));
        ht->count = 0;
#if HT_INTERLINK
        ht->newest = NULL;
        ht->oldest = NULL;
#endif
    }
    return ht;
}

/* free an item be freeing its key and struct,
 * calls type->valfree if assigned */
static void
ht_free_item(ht_hash_table *ht, ht_item* i)
{
#if HT_INTERLINK
    if (i->prev_insert) i->prev_insert->next_insert = i->next_insert;
    if (i->next_insert) i->next_insert->prev_insert = i->prev_insert;
    if (ht->oldest==i) ht->oldest = i->next_insert;
    if (ht->newest==i) ht->newest = i->prev_insert;
#endif
    free(i->key);
    if (ht->type && ht->type->valfree)
        ht->type->valfree(i->v.val);
    free(i);
}

/* insert an item at its right position by pushing it up front
 * only to be called internally */
static void
ht_insert_item(ht_hash_table* ht, ht_item *item)
{
    /* key must not be in list before */
    unsigned long index = item->hash%ht->size;
    if (ht->items[index]!=NULL)
        item->next = ht->items[index];
    ht->items[index] = item;
    ht->count++;
}

/* grow or shrink the table, but never below HT_BASE_SIZE */
static void
ht_resize(ht_hash_table* ht, unsigned int new_size)
{
    if (new_size<HT_BASE_SIZE)
        return;
    ht_hash_table* new_ht = ht_new_sized(ht->type, new_size);
    ht_item *item, *next;
#if HT_INTERLINK
    item = ht->oldest;
    while (item!=NULL) {
        next = item->next_insert;
        item->next = NULL;
        ht_insert_item(new_ht, item);
        item=next;
    }
#else
    for (int i=0; i<ht->size; i++) {
        item = ht->items[i];
        while (item!=NULL) {
            next = item->next;
            item->next = NULL;
            ht_insert_item(new_ht, item);
            item = next;
        }
    }
#endif
    free(ht->items);
    ht->items = new_ht->items;
    ht->size = new_ht->size;
    free(new_ht);
}

/* search for key, return raw item or NULL if not found
 * key and hash must not be modified externally */
ht_item*
ht_search_raw(ht_hash_table *ht, const char *key)
{
    unsigned long index = ht_get_hash(key)%ht->size;
    ht_item* item = ht->items[index];
    while (item!=NULL) {
        if (strcmp(item->key, key)==0)
            return item;
        item = item->next;
    } 
    return NULL;
}

/* search for key, return val or NULL if not found
 * if val can be NULL as well, you may want to call ht_search_raw instead */
void*
ht_search(ht_hash_table* ht, const char* key)
{
    ht_item *result = ht_search_raw(ht, key);
    return result ? result->v.val : NULL;
}


/* insert a new key, return raw item to be modified by caller
 * if the key already exists, its item is returned instead of a new one */
ht_item*
ht_insert_raw(ht_hash_table *ht, const char* key)
{
    /* the empty string is a valid key, only check for NULL */
    if (!key)
        return NULL;
    ht_item *result = ht_search_raw(ht, key);
    if (!result) {
        unsigned long hash = ht_get_hash(key);
        /* key was not found, a new item will be added, resize if necessary */
        if (ht->count*100/ht->size > HT_MAX_FILL)
            ht_resize(ht, ht->size*2);
        result = ht_new_item(ht, key, hash);
        if (result)
            ht_insert_item(ht, result);
    }
    return result;
}

/* insert a key with val, return 1 on success
 * if key already exits, update val and return 0 */
int
ht_insert(ht_hash_table* ht, const char* key, const void* val)
{
    int count = ht->count;
    ht_item *result = ht_insert_raw(ht, key);
    int is_new = count!=ht->count;
    if (result!=NULL) {
        if (!is_new && ht->type && ht->type->valfree)
            ht->type->valfree(result->v.val);
        if (ht->type && ht->type->valdup)
            result->v.val = ht->type->valdup(val);
    }
    return result && is_new;
}

/* delete an item from the hashtable by its key, resize if necessary
 * return 1 on success, 0 otherwise */
int
ht_delete(ht_hash_table* ht, const char* key)
{
    unsigned long index = ht_get_hash(key)%ht->size;
    ht_item **item = &(ht->items[index]);
    while (*item!=NULL) {
        if (strcmp((*item)->key, key)==0) {
            ht_item* next = (*item)->next;
            ht_free_item(ht, *item);
            *item = next;
            ht->count--;
            if (ht->count*100/ht->size < HT_MIN_FILL)
                ht_resize(ht, ht->size/2);
            return 1;
        }
        item = &(*item)->next;
    } 
    return 0;
}

void
ht_print(ht_hash_table *ht)
{
    for (int i=0; i<ht->size; i++) {
        ht_item* item = ht->items[i];
        printf("%2d: ", i);
        while (item!=NULL) {
            printf("%s -> ", item->key);
            item = item->next;
        }
        printf("\n");
    }
}

/* create a new hashtable given a type, which may by NULL */
ht_hash_table*
ht_new(ht_type *type)
{
    /* make a copy of type so that caller can free it */
    ht_type *t = NULL;
    if (type) {
        t = malloc(sizeof(*t));
        if (t) memcpy(t, type, sizeof(*type));
    }
    return ht_new_sized(t, HT_BASE_SIZE);
}

/* delete the hashtable, free its items */
void
ht_del_hash_table(ht_hash_table* ht)
{
    ht_item *next, *item;
#if HT_INTERLINK
    item = ht->oldest;
    while (item!=NULL) {
        next = item->next_insert;
        ht_free_item(ht, item);
        item = next;
    }
#else
    for (int i=0; i<ht->size; i++) {
        item = ht->items[i];
        while (item!=NULL) {
            next = item->next;
            ht_free_item(ht, item);
            item = next;
        }
    }
#endif
    free(ht->type); /* may be NULL */
    free(ht->items);
    free(ht);
}

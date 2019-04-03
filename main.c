#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hash_table.h"

char *cities[] = {
    #include "cities.txt"
};

void*
sdup(const void *str)
{
    char *dup = malloc(strlen(str)+1);
    if (dup)
        strcpy(dup, str);
    return dup;
}

void
sfree(void *value)
{
    free(value);
}

void
search(ht_hash_table* ht, const char *key)
{
    char *p = ht_search(ht, key);
    printf("%s : %s\n", key, p ? p : "not found");
}

void
test_strings()
{
    int i;
    ht_type type = {
        sdup,
        sfree
    };
    ht_hash_table* ht = ht_new(&type);
    int updates = 0;
    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        if (ht_search(ht, cities[i]))
            updates++;
        ht_insert(ht, cities[i], cities[i+2]);
    }
    search(ht, "Jena");
    search(ht, "Berlin");
    search(ht, "Bangkok");
    search(ht, "Phnom Penh");
    printf("[%lu] cities, %d updates\n", ht->count, updates);

#if 1
    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        if (i==99 || i==999 || i==1269 || strcmp(cities[i], "Berlin")==0)
            continue;
        ht_delete(ht, cities[i]);
    }
#endif

#if HT_INTERLINK
    ht_item *item;
    item = ht->oldest;
    while (item!=NULL) {
        printf("_%s_ ", item->key);
        item = item->next_insert;
    }
    printf("\n");
    item = ht->newest;
    while (item!=NULL) {
        printf("_%s_ ", item->key);
        item = item->prev_insert;
    }
    printf("\n");
#endif
    ht_print(ht);
    search(ht, "Baraki Barak");
    search(ht, "Karokh");
    search(ht, "Berlin");
    printf("[%lu]\n", ht->count);

    ht_del_hash_table(ht);
}

/*----------------------------------------------------------------------------*/

typedef struct city city;
struct city {
    double lat;
    double lng;
    int pop;
    char *country;
};

void*
citydup(const void *val)
{
    city *c = (city*) val;
    city *res = (city*) malloc(sizeof(*res));
    res->lat = c->lat;
    res->lng = c->lng;
    res->pop = c->pop;
    res->country = malloc(strlen(c->country)+1);
    strcpy(res->country, c->country);
    return res;

}

void
cityfree(void *val)
{
    city *c = (city*) val;
    free(c->country);
    free(c);
}

void
searchcity(ht_hash_table *ht, char *key)
{
    city *c = ht_search(ht, key);
    if (c)
        printf("%s : (%f %f) pop:%d in %s\n", key, c->lat, c->lng, c->pop, c->country);
    else
        printf("%s not found\n", key);

    if (c && strcmp(key, "Berlin")==0) {
        /* modify object in place */
        c->country = realloc (c->country, 100);
        strcpy(c->country, "Deutsches Reich");
    }
}

void
test_structs()
{
    int i;
    ht_type type = {
        citydup,
        cityfree
    };
    ht_hash_table* ht = ht_new(&type);

    city city;
    city.country = malloc(64);
    int updates = 0;
    
    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        city.lat = atof(cities[i+2]);
        city.lng = atof(cities[i+3]);
        city.pop = atoi(cities[i+4]);
        strcpy(city.country, cities[i+5]);

        if (ht_search(ht, cities[i]))
            updates++;
        ht_insert(ht, cities[i], &city);
    }

    printf("[%lu]\n", ht->count);

    searchcity(ht, "Jena");
    searchcity(ht, "Berlin");
    searchcity(ht, "Arequipa");
    searchcity(ht, "Cusco");
    searchcity(ht, "Tórshavn");

#if 1
    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        if (i==99 || i==999 || i==1269 || strcmp(cities[i], "Berlin")==0)
            continue;
        ht_delete(ht, cities[i]);
    }
#endif
    searchcity(ht, "Jena");
    searchcity(ht, "Berlin");
    searchcity(ht, "Quipungo");

    free(city.country);
    ht_del_hash_table(ht);
}

/*----------------------------------------------------------------------------*/

void
search_raw(ht_hash_table *ht, const char *key)
{
    ht_item *item = ht_search_raw(ht, key);
    if (item)
        printf("%s : %ld\n", key, item->v.u64);
    else
        printf("%s : not found\n", key);
}

void
test_primitives()
{
    ht_hash_table* ht = ht_new(NULL); /* no type assigned */
    ht_item *item;
    int i;

    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        item = ht_search_raw(ht, cities[i]);
        if (item) {
            item->v.u64++;
        } else {
            item = ht_insert_raw(ht, cities[i]);
            item->v.u64 = 1;
        }
    }

    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        item = ht_search_raw(ht, cities[i]);
        if (item && item->v.u64>4)
            printf("%s : %ld\n", item->key, item->v.u64);
    }

#if 1
    for (i=0; i<sizeof(cities)/sizeof(cities[0]); i+=9) {
        if (i==99 || i==999 || i==1269 || strcmp(cities[i], "San Jose")==0)
            continue;
        ht_delete(ht, cities[i]);
    }
#endif
    search_raw(ht, "San Jose");
    search_raw(ht, "Jena");

    item = ht_insert_raw(ht, "Bürgel");
    item->v.u64 = 23;
    if (item) printf("%s : %ld\n", "Bürgel", item->v.u64);
    item = ht_insert_raw(ht, "Bürgel");
    if (item) printf("%s : %ld\n", "Bürgel", item->v.u64);
    ht_del_hash_table(ht);
}

void
test_updates()
{
    ht_type type = {
        sdup,
        sfree
    };
    ht_hash_table* ht = ht_new(&type);
    
    printf("%d", ht_insert(ht, "Berlin", "Deutsches Reich"));
    printf("%d", ht_insert(ht, "Berlin", "BRD"));
    printf("%d", ht_insert(ht, "Berlin", "Deutschland"));
    printf("%d", ht_insert(ht, "Paris", "Frankreich"));
    printf("%d", ht_insert(ht, "London", "England"));
    printf("%d\n", ht_insert(ht, "", "World")); /* the empty string is a valid key */

    printf("[%lu]\n", ht->count);
    search(ht, "Berlin");
    search(ht, "Paris");
    search(ht, "");

#if HT_INTERLINK
    ht_item *i;
    i = ht->oldest;
    while (i!=NULL) {
        printf("_%s_ ", i->key);
        i = i->next_insert;
    }
    printf("\n");
    i = ht->newest;
    while (i!=NULL) {
        printf("_%s_ ", i->key);
        i = i->prev_insert;
    }
    printf("\n");
#endif

    ht_del_hash_table(ht);
}

int
main(int argc, char *argv[])
{
    test_strings();
    test_structs();
    test_primitives();
    test_updates();
}

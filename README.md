# Hashmap

A hashmap written in C, collisions are solved by chaining, the size of the hashmap is automatically adjusted. Keys are strings, values may be primitive values, strings or even structs.

By defining `HT_INTERLINK 1` in the header file, the items can be linked in the order of their insertion, which makes iterating over keys easy.


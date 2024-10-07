
typedef struct StringMap
{
    struct StringMap *children[4];
    s8 key;
    void *value;
} StringMap;

// NOTE(michiel): Lookup if perm == 0
func void **strmap_upsert(StringMap **map, s8 key, Arena *perm)
{
    StringMap *item = 0;
    for (u64 h = s8hash(key); *map; h <<= 2) {
        if (s8eq(key, (*map)->key)) {
            item = *map;
            break;
        }
        map = &(*map)->children[h >> 62];
    }

    void **result = 0;
    if (perm && (item == 0)) {
        *map = item = create(perm, StringMap);
        item->key = key;
    }
    if (item) {
        result = &item->value;
    }
    return result;
}

// NOTE(michiel): This can be embedded in your own struct
typedef struct HashNode
{
    struct HashNode *next;
    u64 hashValue;
} HashNode;

typedef struct HashTable
{
    HashNode **table;
    u64 mask;
    sze count;
} HashTable;

typedef struct HashMap
{
    HashTable t1;
    HashTable t2;
    sze resizeIdx;
} HashMap;

#define HASH_NODE_EQUAL(name)   b32 name(HashNode *a, HashNode *b)
typedef HASH_NODE_EQUAL(HashNodeEqual);

#ifndef HASH_LOAD_FACTOR
#define HASH_LOAD_FACTOR   8
#endif

#ifndef HASH_RESIZE_WORK
#define HASH_RESIZE_WORK   128
#endif

func void hash_table_init(HashTable *hTable, sze count, Arena *perm)
{
    assert(ispow2(count));
    hTable->table = create(perm, HashNode *, count);
    hTable->mask = (u64)count - 1;
    hTable->count = 0;
}

func void hash_table_insert(HashTable *hTable, HashNode *node)
{
    sze idx = (sze)(node->hashValue & hTable->mask);
    HashNode *next = hTable->table[idx];
    node->next = next;
    hTable->table[idx] = node;
    ++hTable->count;
}

func HashNode **hash_table_lookup(HashTable *hTable, HashNode *key, HashNodeEqual *equal)
{
    HashNode **result = 0;
    if (hTable->table)
    {
        sze idx = (sze)(key->hashValue & hTable->mask);
        for (HashNode *test = hTable->table[idx]; test; test = test->next)
        {
            if ((test->hashValue == key->hashValue) && equal(test, key)) {
                result = &test;
                break;
            }
        }
    }

    return result;
}

func HashNode *hash_table_delete(HashTable *hTable, HashNode **from)
{
    HashNode *node = *from;
    *from = node->next;
    --hTable->count;
    return node;
}

func void hash_map_start_resize(HashMap *map, Arena *perm)
{
    assert(map->t2.table == 0);
    map->t2.table = map->t1.table;
    hash_table_init(&map->t1, (sze)((map->t1.mask + 1) * 2), perm);
    map->resizeIdx = 0;
}

func void hash_map_resize(HashMap *map)
{
    sze work = 0;
    while ((work < HASH_RESIZE_WORK) && map->t2.count)
    {
        HashNode **from = &map->t2.table[map->resizeIdx];
        if (*from)
        {
            hash_table_insert(&map->t1, hash_table_delete(&map->t2, from));
            ++work;
        }
        else
        {
            ++map->resizeIdx;
        }
    }

    if (map->t2.table && (map->t2.count == 0))
    {
        // TODO(michiel): Free/Dealloc???
        map->t2.table = 0;
    }
}

func void hash_map_insert(HashMap *map, HashNode *node, Arena *perm)
{
    if (!map->t1.table) {
        hash_table_init(&map->t1, 16, perm);
    }
    hash_table_insert(&map->t1, node);

    if (!map->t2.table) {
        sze loadFactor = map->t1.count / (sze)(map->t1.mask + 1);
        if (loadFactor >= HASH_LOAD_FACTOR) {
            hash_map_start_resize(map, perm);
        }
    }

    hash_map_resize(map);
}

func HashNode *hash_map_lookup(HashMap *map, HashNode *key, HashNodeEqual *equal)
{
    hash_map_resize(map);
    HashNode **from = hash_table_lookup(&map->t1, key, equal);
    from = from ? from : hash_table_lookup(&map->t2, key, equal);
    return from ? *from : 0;
}

func HashNode *hash_map_delete(HashMap *map, HashNode *key, HashNodeEqual *equal)
{
    hash_map_resize(map);
    HashNode *result = 0;
    HashNode **from = hash_table_lookup(&map->t1, key, equal);
    if (from) {
        result = hash_table_delete(&map->t1, from);
    } else {
        from = hash_table_lookup(&map->t2, key, equal);
        if (from) {
            result = hash_table_delete(&map->t2, from);
        }
    }
    return result;
}

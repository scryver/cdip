
typedef struct StringMap StringMap;
struct StringMap
{
    StringMap *children[4];
    s8 key;
    void *value;
};

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

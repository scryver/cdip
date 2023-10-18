#include "defines.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <unistd.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#include "types.h"
#include "helpers.h"
#include "memory.h"
#include "strings.h"
#include "files.h"

func void do_asserts(void)
{
    assert(1);
    assert(0);
}

func u64 hash(s8 s)
{
    u64 result = IMM_U64(0x100);
    for (sze index = 0; index < s.size; ++index) {
        result ^= s.data[index];
        result *= IMM_U64(1111111111111111111);
    }
    return result;
}

#if 0
typedef struct HashMap HashMap;
struct HashMap
{
    HashMap *children[4];
};
#endif

typedef struct StringSet StringSet;
struct StringSet
{
    StringSet *children[4];
    s8 key;
};

func b32 equals(s8 a, s8 b)
{
    b32 result = (a.size == b.size) && (memcmp(a.data, b.data, a.size) == 0);
    return result;
}

func b32 is_member(StringSet **set, s8 key, Arena *perm)
{
    for (u64 h = hash(key); *set; h <<= 2) {
        if (equals(key, (*set)->key)) {
            return 1;
        }
        set = &(*set)->children[h >> 62];
    }
    *set = create(perm, StringSet);
    (*set)->key = key;
    return 0;
}

typedef struct UniqResult
{
    sze count;
    StringSet *map;
} UniqResult;
func UniqResult unique(sze stringCount, s8 *strings, Arena *perm)
{
    UniqResult result = {0};
    while (result.count < stringCount)
    {
        if (is_member(&result.map, strings[result.count], perm)) {
            strings[result.count] = strings[--stringCount];
        } else {
            ++result.count;
        }
    }
    return result;
}

int main(int argCount, char **arguments)
{
    s8 strings[] = {
        cstr("LAALALALAL"),
        cstr("Hallo"),
        cstr("What?"),
        cstr("HowTo"),
        cstr("LAALALALALA"),
        cstr("HowTa"),
        cstr("HowTi"),
        cstr("Hallo"),
        cstr("Hello"),
        cstr("HowTi"),
        cstr("LAALALALALA"),
    };

    Arena tempArena = create_arena(1024*1024*1024);
    UniqResult uniqueCount = unique(countof(strings), strings, &tempArena);
    char uniqueChar = '0' + (char)uniqueCount.count;
    write(1, &uniqueChar, 1);
    uniqueChar = '\n';
    write(1, &uniqueChar, 1);
    debugbreak();

    s8 testC = cstr("hallo?\n");
    write(1, testC.data, testC.size);
    do_asserts();
    return 0;
}


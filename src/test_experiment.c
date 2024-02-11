#include "defines.h"
#include <stdint.h>
#include <stddef.h>

#if PLATFORM_WIN32

#pragma comment(linker, "/subsystem:console")
#pragma comment(lib, "kernel32.lib")

#else

#include <stdlib.h>

#include <unistd.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include "types.h"
#include "helpers.h"
#include "memory.h"
#include "memarena.h"
#include "membound.h"
#include "strings.h"
#include "files.h"

func void do_asserts(void)
{
    assert(1);
    //assert(0);
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

func void sze_to_file(OsFile *file, sze size)
{
    char numBuf[64];
    buf writeBuf = buf(0, numBuf);
    if (size)
    {
        sze index = 0;
        while (size)
        {
            sze digit = size % 10;
            size = size / 10;
            writeBuf.data[writeBuf.size++] = (char)digit + '0';
        }
        sze minIdx = 0;
        sze maxIdx = writeBuf.size - 1;
        while (minIdx < maxIdx)
        {
            char temp = numBuf[minIdx];
            numBuf[minIdx++] = numBuf[maxIdx];
            numBuf[maxIdx--] = temp;
        }
    }
    else
    {
        writeBuf.data[writeBuf.size++] = '0';
    }
    write_to_file(file, writeBuf);
}

func void ptr_to_file(OsFile *file, void *ptr)
{
    char numBuf[64];
    buf writeBuf = buf(0, numBuf);
    writeBuf.data[writeBuf.size++] = '0';
    writeBuf.data[writeBuf.size++] = 'x';
    if (ptr)
    {
        sze index = 0;
        uptr ptrHex = (uptr)ptr;
        while (ptrHex)
        {
            uptr digit = ptrHex % 16;
            ptrHex = ptrHex / 16;
            if (digit < 10) {
                writeBuf.data[writeBuf.size++] = (char)digit + '0';
            } else {
                writeBuf.data[writeBuf.size++] = (char)(digit - 10) + 'A';
            }
        }
        sze minIdx = 2;
        sze maxIdx = writeBuf.size - 1;
        while (minIdx < maxIdx)
        {
            char temp = numBuf[minIdx];
            numBuf[minIdx++] = numBuf[maxIdx];
            numBuf[maxIdx--] = temp;
        }
    }
    else
    {
        writeBuf.data[writeBuf.size++] = '0';
    }
    write_to_file(file, writeBuf);
}

int main(int argCount, char **arguments)
{
    unused(argCount);
    unused(arguments);

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

    Arena permArena = create_arena(1024*1024*1024);
    Arena tempArena = create_arena(1024*1024*1024);
    UniqResult uniqueCount = unique(countof(strings), strings, &tempArena);
    char uniqueChar = '0' + (char)uniqueCount.count;

    OsFile output = open_file(cstr("stdout"), OsFile_Write, &permArena, tempArena);
    write_to_file(&output, buf(1, &uniqueChar));
    uniqueChar = '\n';
    write_to_file(&output, buf(1, &uniqueChar));
    //debugbreak();

    s8 testC = cstr("hallo?\n");
    write_to_file(&output, buf(testC.size, testC.data));

    OsFile filer = open_file(cstr("jaja.txt"), OsFile_Write, &permArena, tempArena);
    write_to_file(&filer, buf(testC.size, testC.data));
    close_file(&filer);

    BoundMemory boundMem = create_bound_memory(128*1024*1024);
    s8 preIdx = cstr("Index ");
    s8 midIdx = cstr(": ");
    s8 postIdx = cstr("\n");
    for (sze idx = 0; idx < countof(boundMem.blocks); ++idx) {
        write_to_file(&output, buf(preIdx.size, preIdx.data));
        sze_to_file(&output, idx);
        write_to_file(&output, buf(midIdx.size, midIdx.data));
        ptr_to_file(&output, boundMem.blocks[idx]);
        write_to_file(&output, buf(postIdx.size, postIdx.data));
    }
    byte *test01 = bound_create(&boundMem, byte, 14);
    byte *test02 = bound_create(&boundMem, byte, 514);
    byte *test03 = bound_create(&boundMem, byte, 89);
    byte *test04 = bound_create(&boundMem, byte, 52);
    byte *test05 = bound_create(&boundMem, byte, 67);
    byte *test06 = bound_create(&boundMem, byte, 521);
    byte *test07 = bound_create(&boundMem, byte, 213);
    byte *test08 = bound_create(&boundMem, byte, 102);
    byte *test09 = bound_create(&boundMem, byte, 8);
    byte *test10 = bound_create(&boundMem, byte, 1);
    byte *test11 = bound_create(&boundMem, byte, 45);
    byte *test12 = bound_create(&boundMem, byte, 450);
    byte *test13 = bound_create(&boundMem, byte, 3045);
    for (sze idx = 0; idx < countof(boundMem.blocks); ++idx) {
        write_to_file(&output, buf(preIdx.size, preIdx.data));
        sze_to_file(&output, idx);
        write_to_file(&output, buf(midIdx.size, midIdx.data));
        ptr_to_file(&output, boundMem.blocks[idx]);
        write_to_file(&output, buf(postIdx.size, postIdx.data));
    }
    bound_dealloc(&boundMem, test10, 1);
    bound_dealloc(&boundMem, test03, 89);
    bound_dealloc(&boundMem, test12, 450);
    bound_dealloc(&boundMem, test01, 14);
    bound_dealloc(&boundMem, test05, 67);
    bound_dealloc(&boundMem, test13, 3045);
    bound_dealloc(&boundMem, test02, 514);
    bound_dealloc(&boundMem, test09, 8);
    bound_dealloc(&boundMem, test06, 521);
    bound_dealloc(&boundMem, test08, 102);
    bound_dealloc(&boundMem, test11, 45);
    bound_dealloc(&boundMem, test07, 213);
    bound_dealloc(&boundMem, test04, 52);
    for (sze idx = 0; idx < countof(boundMem.blocks); ++idx) {
        write_to_file(&output, buf(preIdx.size, preIdx.data));
        sze_to_file(&output, idx);
        write_to_file(&output, buf(midIdx.size, midIdx.data));
        ptr_to_file(&output, boundMem.blocks[idx]);
        write_to_file(&output, buf(postIdx.size, postIdx.data));
    }

    do_asserts();
    return 0;
}


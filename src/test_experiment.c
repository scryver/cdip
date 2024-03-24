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
#include "helper_funcs.h"
#include "memory.h"
#include "memarena.h"
#include "membound.h"
#include "strings.h"
#include "files.h"
#include "format.h"
#include "image.h"
#include "bitmap.h"

func void do_asserts(void)
{
    assert(1);
    //assert(0);
}

func u64 hash(s8 s)
{
    u64 result = IMM_U64(0x100);
    for (sze index = 0; index < s.size; ++index) {
        result ^= (u64)s.data[index];
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

func b32 is_member(StringSet **set, s8 key, Arena *perm)
{
    for (u64 h = hash(key); *set; h <<= 2) {
        if (s8eq(key, (*set)->key)) {
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

    OsFile outFile = open_file(cstr("stdout"), OsFile_Write, &permArena, tempArena);
    fmt_buf output = fmt_buf_fd(1024, create(&permArena, byte, 1024), &outFile);
    append_byte(&output, uniqueChar);
    append_byte(&output, '\n');
    //debugbreak();

    s8 testC = cstr("hallo?\n");
    append_s8(&output, testC);

    OsFile filer = open_file(cstr("jaja.txt"), OsFile_Write, &permArena, tempArena);
    write_str_to_file(&filer, testC);
    close_file(&filer);

    BoundMemory boundMem = create_bound_memory(128*1024*1024);
    for (sze idx = 0; idx < countof(boundMem.blocks); ++idx) {
        append_cstr(&output, "Index ");
        append_i64(&output, idx);
        append_cstr(&output, ": ");
        append_ptr(&output, boundMem.blocks[idx]);
        append_byte(&output, '\n');
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
        append_cstr(&output, "Index ");
        append_i64(&output, idx);
        append_cstr(&output, ": ");
        append_ptr(&output, boundMem.blocks[idx]);
        append_byte(&output, '\n');
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
        append_cstr(&output, "Index ");
        append_i64(&output, idx);
        append_cstr(&output, ": ");
        append_ptr(&output, boundMem.blocks[idx]);
        append_byte(&output, '\n');
    }

    FileResult readBitmapFile = read_entire_file(cstr("data/test_image.bmp"), &permArena, tempArena);
    if (readBitmapFile.error == OsFile_NoError) {
        Image bitmapImage = bitmap_load(readBitmapFile.fileBuf, &permArena);
        if (bitmapImage.pixels) {
            bitmap_save(&bitmapImage, cstr("outtest.bmp"), tempArena);
        }
    }

    flush(&output);
    close_file(&outFile);

    do_asserts();
    return 0;
}


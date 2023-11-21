#include "../src/defines.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

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

#include "../src/types.h"
#include "../src/helpers.h"
#include "../src/memory.h"
#include "../src/strings.h"
#include "../src/files.h"

#include "utf_encoding.h"

func i32 utf8_try_decode(s8 str)
{
    i32 state = 0;
    u32 codepoint = 0;
    for (sze idx = 0; idx < str.size; ++idx) {
        state = utf8_decode_internal(state, &codepoint, str.data[idx]);
        if (state <= 0) {
            break;
        }
    }
    return state;
}

i32 main(int argCount, char **arguments)
{
    unused(argCount);
    unused(arguments);

    b32 failure = false;

    for (u32 codepoint = 0; codepoint < 0x200000; ++codepoint)
    {
        u8 strBuf[4];
        s8 utf8str = utf8_encode(codepoint, strBuf, sizeof(strBuf));

        i32 state = 0;
        u32 outCode = 0;
        for (sze idx = 0; idx < utf8str.size; ++idx) {
            state = utf8_decode_internal(state, &outCode, utf8str.data[idx]);
            if (state <= 0) {
                break;
            }
        }

        if (codepoint > 0x10FFFF)
        {
            if (state == 0) {
                printf("FAIL: Accepted out-of-range codepoint: U+%06X [decode: U+%06X]\n", codepoint, outCode);
                failure = true;
            } else if (state > 0) {
                printf("FAIL: Incomplete out-of-rangeU+%06X [state: %d]\n", codepoint, state);
                failure = true;
            }
        }
        else if (is_utf_surrogate(codepoint))
        {
            if (state == 0) {
                printf("FAIL: Accepted surrogate codepoint: U+%06X [decode: U+%06X]\n", codepoint, outCode);
                failure = true;
            } else if (state > 0) {
                printf("FAIL: Incomplete surrogate U+%06X [state: %d]\n", codepoint, state);
                failure = true;
            }
        }
        else
        {
            if ((state == 0) && (codepoint != outCode)) {
                printf("FAIL: Decoding failed: U+%06X -> U+%06X\n", codepoint, outCode);
                failure = true;
            } else if (state < 0) {
                printf("FAIL: Codepoint rejected: U+%06X\n", codepoint);
                failure = true;
            } else if (state > 0) {
                printf("FAIL: Incomplete U+%06X [state: %d]\n", codepoint, state);
                failure = true;
            }
        }
    }

    for (u32 codepoint = 0; codepoint < 0x80; ++codepoint)
    {
        u8 strBuf[4];
        strBuf[0] = (u8)(0xC0 | (codepoint >> 6));
        strBuf[1] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(2, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [2]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }

        strBuf[0] = 0xE0;
        strBuf[1] = (u8)(0x80 | (codepoint >> 6));
        strBuf[2] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(3, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [3]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }

        strBuf[0] = 0xF0;
        strBuf[1] = 0x80;
        strBuf[2] = (u8)(0x80 | (codepoint >> 6));
        strBuf[3] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(4, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [4]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }
    }

    for (u32 codepoint = 0x80; codepoint < 0x800; ++codepoint)
    {
        u8 strBuf[4];

        strBuf[0] = 0xE0;
        strBuf[1] = (u8)(0x80 | (codepoint >> 6));
        strBuf[2] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(3, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [3]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }

        strBuf[0] = 0xF0;
        strBuf[1] = 0x80;
        strBuf[2] = (u8)(0x80 | (codepoint >> 6));
        strBuf[3] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(4, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [4]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }
    }

    for (u32 codepoint = 0x800; codepoint < 0x10000; ++codepoint)
    {
        u8 strBuf[4];

        strBuf[0] = 0xF0;
        strBuf[1] = (u8)(0x80 | (codepoint >> 12));
        strBuf[2] = (u8)(0x80 | ((codepoint >> 6) & 0x3F));
        strBuf[3] = (u8)(0x80 | (codepoint & 0x3F));
        switch (utf8_try_decode(s8(4, strBuf)))
        {
            case 0: {
                printf("FAIL: Accepted too long encoding [4]: U+%06X\n", codepoint);
                failure = true;
            } break;

            case -1: {} break;

            default: {
                printf("FAIL: Incomplete U+%06X\n", codepoint);
                failure = true;
            } break;
        }
    }

    if (failure == false) {
        printf("PASS!\n");
    }

    return failure;
}

#include "../src/defines.h"
#include <stdint.h>
#include <stddef.h>

#if PLATFORM_WIN32

#pragma comment(linker, "/subsystem:console")
#pragma comment(lib, "kernel32.lib")

#include <intrin.h>

#else

#include <stdlib.h>

#include <unistd.h>
#include <time.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#endif

#include "../src/types.h"
#include "../src/helpers.h"
#include "../src/helper_funcs.h"
#include "../src/memory.h"
#include "../src/memarena.h"
#include "../src/strings.h"
#include "../src/files.h"

func u64 random_wyhash(u64 *state)
{
    u64 s = *state;
    s += IMM_U64(0x60BEE2BEE120FC15);
    *state = s;
#if COMPILER_MSVC
    u64 high;
    u64 low = _umul128(s, IMM_U64(0xA3B195354A39B70D), &high);
    u64 m1 = high ^ low;
    low = _umul128(m1, IMM_U64(0x1B03738712FAD5C9), &high);
    u64 m2 = high ^ low;
#else
    __uint128_t tmp;
    tmp = (__uint128_t)s * IMM_U64(0xA3B195354A39B70D);
    u64 m1 = (u64)(tmp >> 64) ^ (u64)tmp;
    tmp = (__uint128_t)m1 * IMM_U64(0x1B03738712FAD5C9);
    u64 m2 = (u64)(tmp >> 64) ^ (u64)tmp;
#endif
    return m2;
}

func f64 random01(u64 *state)
{
    u64 largerNumber = random_wyhash(state);
    f64 result = (f64)(largerNumber >> 11) * 0x1.0p-53;
    return result;
}

func sze random_choice(u64 *state, sze count)
{
    u64 r = random_wyhash(state);
    sze result = (sze)(r % (u64)count);
    return result;
}

#if 0
global const char gSimpleConsonants[] = "ptkmnsl";
global const char gConsonants[] = "bptkmnqsl12345678'";
global const char gVowels[] = "aeiou";
{
    char c = cons.data[randNum % cons.size];
    if ((c == '1') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 's';
        dest.data[dest.size++] = 'h';
    } else if ((c == '2') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 'z';
        dest.data[dest.size++] = 'h';
    } else if ((c == '3') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 'c';
        dest.data[dest.size++] = 'h';
    } else if (c == '4') {
        dest.data[dest.size++] = 'j';
    } else if ((c == '5') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 'n';
        dest.data[dest.size++] = 'g';
    } else if ((c == '6') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 'k';
        dest.data[dest.size++] = 'h';
    } else if ((c == '7') && (dest.size < (maxSize - 1))) {
        dest.data[dest.size++] = 'g';
        dest.data[dest.size++] = 'h';
    } else if (c == 'j') {
        dest.data[dest.size++] = 'y';
    } else {
        dest.data[dest.size++] = c;
    }
}
#endif

// NOTE(michiel): Consonant sounds:
// 1 -> 'sh' sound
// 2 -> 'zh', 's' from pleasure, or a French 'j'
// 3 -> 'ch', as in chair ('tsh')
// 4 -> 'j', as in judge ('dj')
// 5 -> 'ng', as in hang
// j -> 'y', as in year
// x -> 'ch', as in German 'Bach' or Scottish 'loch'
// 6 -> 'gh', as in the Spanish amigo
// q -> like 'k', but pronounced further in the back. Like Arabic Qu'ran or Iraq
// ' -> glottal stop, middle of uh-oh or in some accents the 'tt' in button
global const s8 gMinimalCons      = cstr("ptkmnsl");
global const s8 gEnglishCons      = cstr("ptkbdgmnlrs1z23");
global const s8 gPirahaCons       = cstr("ptkmnh");
global const s8 gHawaiianCons     = cstr("hklmnpw'");
global const s8 gGreenlandicCons  = cstr("ptkqvsgrmn5lj");
global const s8 gArabicCons       = cstr("tks1dbq6xmnlrwj");
global const s8 gArabicLiteCons   = cstr("tkdgmns1");
global const s8 gEnglishLiteCons  = cstr("ptkbdgmnsz23hjw");

global const s8 g5Vowels          = cstr("aeiou");
global const s8 g3Vowels          = cstr("aiu");
global const s8 g5VowelsAEI       = cstr("aeiouAEI");
global const s8 g5VowelsU         = cstr("aeiouU");
global const s8 g3VowelsAI        = cstr("aiuAI");
global const s8 g3VowelsAlt       = cstr("eou");
global const s8 g5VowelsAOU       = cstr("aeiouAOU");

global const s8 gConsonants[]     = {
    gMinimalCons, gEnglishCons, gPirahaCons, gHawaiianCons,
    gGreenlandicCons, gArabicCons, gArabicLiteCons, gEnglishLiteCons,
};
global const s8 gSibilants[]      = { cstr("s"), cstr("s1"), cstr("s1f"), };
global const s8 gLiquids[]        = { cstr("rl"), cstr("r"), cstr("l"), cstr("wj"), cstr("rlwj"), };
global const s8 gFinalizers[]     = { cstr("mn"), cstr("sk"), cstr("mn5"), cstr("s1z2"), };
global const s8 gVowels[]         = {
    g5Vowels, g3Vowels, g5VowelsAEI, g5VowelsU,
    g3VowelsAI, g3VowelsAlt, g5VowelsAOU,
};

global const s8 gSyllableFormats[] = {
    cstr("CVC"),
    cstr("CVV?C"),
    cstr("CVVC?"), cstr("CVC?"), cstr("CV"), cstr("VC"), cstr("CVF"), cstr("C?VC"), cstr("CVF?"),
    cstr("CL?VC"), cstr("CL?VF"), cstr("S?CVC"), cstr("S?CVF"), cstr("S?CVC?"),
    cstr("C?VF"), cstr("C?VC?"), cstr("C?VF?"), cstr("C?L?VC"), cstr("VC"),
    cstr("CVL?C?"), cstr("C?VL?C"), cstr("C?VLC?"),
};

typedef enum LangReplace
{
    LangReplace_Default,

    LangReplace_ConsSlavic  = 0x01,
    LangReplace_ConsGerman  = 0x02,
    LangReplace_ConsFrench  = 0x03,
    LangReplace_ConsChinese = 0x04,
    LangReplace_ConsMask    = 0x0F,

    LangReplace_VowelAcutes     = 0x10,
    LangReplace_VowelUmlauts    = 0x20,
    LangReplace_VowelWelsh      = 0x30,
    LangReplace_VowelDiphthongs = 0x40,
    LangReplace_VowelDoubles    = 0x50,
    LangReplace_VowelMask       = 0xF0
} LangReplace;

typedef enum LangRestrictions
{
    LangRestrict_None,
    LangRestrict_Doubles,
    LangRestrict_DoubleAndHardClusters,
} LangRestrictions;

typedef struct LangSet
{
    const s8 cons;
    const s8 vowels;
    const s8 sibils;
    const s8 liquids;
    const s8 finals;
    u32 replaceFlags;
    LangRestrictions restrictions;
} LangSet;

func u8 get_random_char(const s8 str, u64 *randBase)
{
    f64 randF = random01(randBase);
    randF *= randF;
    sze randIdx = (sze)(randF * (f64)str.size);
    if (randIdx < 0) { randIdx = 0; }
    else if (randIdx >= str.size) { randIdx = str.size - 1; }
    u8 result = str.data[randIdx];
    return result;
}

func s8 gen_from_format(const s8 format, u64 *randBase, LangSet *lang, LangRestrictions restriction, Arena *perm)
{
    s8 dest = create_empty_s8(perm, 1024);
    sze maxSize = dest.size;
    dest.size = 0;
    while (dest.size == 0)
    {
        for (sze idx = 0; (idx < format.size) && (dest.size < maxSize); ++idx) {
            u8 baseFmt = format.data[idx];
            if (((idx + 1) < format.size) && (format.data[idx + 1] == '?')) {
                ++idx;
                if (random_wyhash(randBase) & 0x1) {
                    continue;
                }
            }
            switch (baseFmt)
            {
                case 'C': { dest.data[dest.size++] = get_random_char(lang->cons, randBase); } break;
                case 'V': { dest.data[dest.size++] = get_random_char(lang->vowels, randBase); } break;
                case 'S': { dest.data[dest.size++] = get_random_char(lang->sibils, randBase); } break;
                case 'L': { dest.data[dest.size++] = get_random_char(lang->liquids, randBase); } break;
                case 'F': { dest.data[dest.size++] = get_random_char(lang->finals, randBase); } break;
                default:  { dest.data[dest.size++] = baseFmt; } break;
            }
        }

        if ((restriction == LangRestrict_Doubles) ||
            (restriction == LangRestrict_DoubleAndHardClusters))
        {
            u8 prev = dest.data[0];
            for (sze idx = 1; idx < dest.size; ++idx) {
                if (dest.data[idx] == prev) {
                    dest.size = 0;
                    break;
                } else {
                    prev = dest.data[idx];
                }
            }
        }
        if (restriction == LangRestrict_DoubleAndHardClusters)
        {
            u8 prev = dest.data[0];
            for (sze idx = 1; idx < dest.size; ++idx) {
                if (((prev == 's') || (prev == 'f') || (prev == '1')) &&
                    ((dest.data[idx] == 's') || (dest.data[idx] == '1'))) {
                    dest.size = 0;
                    break;
                } else if (((prev == 'r') || (prev == 'l')) &&
                           ((dest.data[idx] == 'r') || (dest.data[idx] == 'l'))) {
                    dest.size = 0;
                    break;
                } else {
                    prev = dest.data[idx];
                }
            }
        }
    }
    sze shorten = maxSize - dest.size;
    perm->begin -= shorten;
    return dest;
}

func s8 gen_random_syllable(u64 *randBase, LangRestrictions restriction, Arena *perm)
{
    LangSet lang = {
        .cons = gConsonants[random_choice(randBase, countof(gConsonants))],
        .vowels = gVowels[random_choice(randBase, countof(gVowels))],
        .sibils = gSibilants[random_choice(randBase, countof(gSibilants))],
        .liquids = gLiquids[random_choice(randBase, countof(gLiquids))],
        .finals = gFinalizers[random_choice(randBase, countof(gFinalizers))],
    };
    s8 result = gen_from_format(gSyllableFormats[random_choice(randBase, countof(gSyllableFormats))], randBase, &lang, restriction, perm);
    return result;
}

func s8 gen_random_syllables(u64 *randBase, LangRestrictions restriction, sze minSylCount, sze maxSylCount, Arena *perm)
{
    LangSet lang = {
        .cons = gConsonants[random_choice(randBase, countof(gConsonants))],
        .vowels = gVowels[random_choice(randBase, countof(gVowels))],
        .sibils = gSibilants[random_choice(randBase, countof(gSibilants))],
        .liquids = gLiquids[random_choice(randBase, countof(gLiquids))],
        .finals = gFinalizers[random_choice(randBase, countof(gFinalizers))],
    };

    sze sylCount = minSylCount;
    if (maxSylCount > minSylCount) {
        sylCount = minSylCount + random_choice(randBase, maxSylCount - minSylCount);
    }
    s8 result = s8(0, perm->begin);;
    for (sze sylIdx = 0; sylIdx < sylCount; ++sylIdx) {
        const s8 format = gSyllableFormats[random_choice(randBase, countof(gSyllableFormats))];
        s8 syl = gen_from_format(format, randBase, &lang, restriction, perm);
        result.size += syl.size;
    }
    return result;
}

func sze cons_replace_default(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case '1': { if (maxSize > 1) { dest[0] = 's'; dest[1] = 'h'; result = 2; } } break;
        case '2': { if (maxSize > 1) { dest[0] = 'z'; dest[1] = 'h'; result = 2; } } break;
        case '3': { if (maxSize > 1) { dest[0] = 'c'; dest[1] = 'h'; result = 2; } } break;
        case '4': { dest[0] = 'j'; result = 1; } break;
        case '5': { if (maxSize > 1) { dest[0] = 'n'; dest[1] = 'g'; result = 2; } } break;
        case 'j': { dest[0] = 'y'; result = 1; } break;
        case 'x': { if (maxSize > 1) { dest[0] = 'k'; dest[1] = 'h'; result = 2; } } break;
        case '6': { if (maxSize > 1) { dest[0] = 'g'; dest[1] = 'h'; result = 2; } } break;
        case 'b': case 'c': case 'd': case 'f': case 'g': case 'h': case 'k':
        case 'l': case 'm': case 'n': case 'p': case 'q': case 'r': case 's':
        case 't': case 'v': case 'w': case 'y': case 'z': { dest[0] = character; result = 1; } break;
        default: {} break;
    }
    return result;
}

func sze cons_replace_slavic(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case '1': { if (maxSize > 1) { dest[0] = 0xC5; dest[1] = 0xA1; result = 2; } } break;
        case '2': { if (maxSize > 1) { dest[0] = 0xC5; dest[1] = 0xBE; result = 2; } } break;
        case '3': { if (maxSize > 1) { dest[0] = 0xC4; dest[1] = 0x8D; result = 2; } } break;
        case '4': { if (maxSize > 1) { dest[0] = 0xC7; dest[1] = 0xA7; result = 2; } } break;
        case 'j': { dest[0] = 'j'; result = 1; } break;
        default:  { result = cons_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze cons_replace_german(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case '1': { if (maxSize > 2) { dest[0] = 's'; dest[1] = 'c'; dest[2] = 'h'; result = 3; } } break;
        case '2': { if (maxSize > 1) { dest[0] = 'z'; dest[1] = 'h'; result = 2; } } break;
        case '3': { if (maxSize > 3) { dest[0] = 't'; dest[1] = 's'; dest[2] = 'c'; dest[3] = 'h'; result = 4; } } break;
        case '4': { if (maxSize > 1) { dest[0] = 'd'; dest[1] = 'z'; result = 2; } } break;
        case 'j': { dest[0] = 'j'; result = 1; } break;
        case 'x': { if (maxSize > 1) { dest[0] = 'c'; dest[1] = 'h'; result = 2; } } break;
        default:  { result = cons_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze cons_replace_french(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case '1': { if (maxSize > 1) { dest[0] = 'c'; dest[1] = 'h'; result = 2; } } break;
        case '2': { dest[0] = 'j'; result = 1; } break;
        case '3': { if (maxSize > 2) { dest[0] = 't'; dest[1] = 'c'; dest[2] = 'h'; result = 3; } } break;
        case '4': { if (maxSize > 1) { dest[0] = 'd'; dest[1] = 'j'; result = 2; } } break;
        case 'x': { if (maxSize > 1) { dest[0] = 'k'; dest[1] = 'h'; result = 2; } } break;
        default:  { result = cons_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze cons_replace_chinese(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case '1': { dest[0] = 'x'; result = 1; } break;
        case '3': { dest[0] = 'q'; result = 1; } break;
        case '4': { dest[0] = 'j'; result = 1; } break;
        default:  { result = cons_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze vowel_replace_default(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case 'A': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xA1; result = 2; } } break;
        case 'E': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xA9; result = 2; } } break;
        case 'I': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xAD; result = 2; } } break;
        case 'O': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xB3; result = 2; } } break;
        case 'U': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xBA; result = 2; } } break;
        case 'a': case 'e': case 'i': case 'o': case 'u': { dest[0] = character; result = 1; } break;
        default: {} break;
    }
    return result;
}

func sze vowel_replace_acutes(u8 *dest, sze maxSize, u8 character)
{
    return vowel_replace_default(dest, maxSize, character);
}

func sze vowel_replace_umlauts(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case 'A': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xA4; result = 2; } } break;
        case 'E': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xAB; result = 2; } } break;
        case 'I': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xAF; result = 2; } } break;
        case 'O': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xB6; result = 2; } } break;
        case 'U': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xBC; result = 2; } } break;
        default:  { result = vowel_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze vowel_replace_welsh(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case 'A': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xA2; result = 2; } } break;
        case 'E': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xAA; result = 2; } } break;
        case 'I': { dest[0] = 'y'; result = 1; } break;
        case 'O': { if (maxSize > 1) { dest[0] = 0xC3; dest[1] = 0xB4; result = 2; } } break;
        case 'U': { dest[0] = 'w'; result = 1; } break;
        default:  { result = vowel_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze vowel_replace_diphthongs(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case 'A': { if (maxSize > 1) { dest[0] = 'a'; dest[1] = 'u'; result = 2; } } break;
        case 'E': { if (maxSize > 1) { dest[0] = 'e'; dest[1] = 'i'; result = 2; } } break;
        case 'I': { if (maxSize > 1) { dest[0] = 'i'; dest[1] = 'e'; result = 2; } } break;
        case 'O': { if (maxSize > 1) { dest[0] = 'o'; dest[1] = 'u'; result = 2; } } break;
        case 'U': { if (maxSize > 1) { dest[0] = 'o'; dest[1] = 'o'; result = 2; } } break;
        default:  { result = vowel_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func sze vowel_replace_doubles(u8 *dest, sze maxSize, u8 character)
{
    sze result = 0;
    switch (character)
    {
        case 'A': { if (maxSize > 1) { dest[0] = 'a'; dest[1] = 'a'; result = 2; } } break;
        case 'E': { if (maxSize > 1) { dest[0] = 'e'; dest[1] = 'e'; result = 2; } } break;
        case 'I': { if (maxSize > 1) { dest[0] = 'i'; dest[1] = 'i'; result = 2; } } break;
        case 'O': { if (maxSize > 1) { dest[0] = 'o'; dest[1] = 'o'; result = 2; } } break;
        case 'U': { if (maxSize > 1) { dest[0] = 'u'; dest[1] = 'u'; result = 2; } } break;
        default:  { result = vowel_replace_default(dest, maxSize, character); } break;
    }
    return result;
}

func s8 gen_final_output(s8 name, u32 replaceFlags, Arena *perm)
{
    u32 consReplace  = replaceFlags & LangReplace_ConsMask;
    u32 vowelReplace = replaceFlags & LangReplace_VowelMask;

    s8 dest = create_empty_s8(perm, 1024);
    sze maxSize = dest.size;
    dest.size = 0;
    for (sze idx = 0; (idx < name.size) && (dest.size < maxSize); ++idx)
    {
        u8 c = name.data[idx];
        u8 *d = dest.data + dest.size;
        sze remSize = maxSize - dest.size;
        sze addSize = 0;
        switch (consReplace) {
            case LangReplace_ConsSlavic : { addSize = cons_replace_slavic(d, remSize, c); } break;
            case LangReplace_ConsGerman : { addSize = cons_replace_german(d, remSize, c); } break;
            case LangReplace_ConsFrench : { addSize = cons_replace_french(d, remSize, c); } break;
            case LangReplace_ConsChinese: { addSize = cons_replace_chinese(d, remSize, c); } break;
            default: { addSize = cons_replace_default(d, remSize, c); } break;
        }
        if (addSize == 0) {
            switch (vowelReplace) {
                case LangReplace_VowelAcutes    : { addSize = vowel_replace_acutes(d, remSize, c); } break;
                case LangReplace_VowelUmlauts   : { addSize = vowel_replace_umlauts(d, remSize, c); } break;
                case LangReplace_VowelWelsh     : { addSize = vowel_replace_welsh(d, remSize, c); } break;
                case LangReplace_VowelDiphthongs: { addSize = vowel_replace_diphthongs(d, remSize, c); } break;
                case LangReplace_VowelDoubles   : { addSize = vowel_replace_doubles(d, remSize, c); } break;
                default: { addSize = vowel_replace_default(d, remSize, c); } break;
            }
        }
        dest.size += addSize;
    }
    sze shorten = maxSize - dest.size;
    perm->begin -= shorten;
    return dest;
}

int main(int argCount, char **arguments)
{
    u64 randBase = (u64)time(0) * (u64)time(0);

    Arena permArena = create_arena(1024*1024);
    Arena tempArena = create_arena(1024*1024);

    OsFile output = open_file(cstr("stdout"), OsFile_Write, &permArena, tempArena);

    LangSet english = {
        .cons = gEnglishCons,
        .vowels = g5Vowels,
        .sibils = gSibilants[2],
        .liquids = gLiquids[4],
        .finals = gFinalizers[2],
    };
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));
    write_str_to_file(&output, gen_from_format(cstr("CVC\n"), &randBase, &english, 0, &permArena));

    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_random_syllable(&randBase, 0, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_random_syllable(&randBase, 0, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_random_syllable(&randBase, 0, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_random_syllable(&randBase, 0, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_random_syllable(&randBase, 0, &permArena));
    write_str_to_file(&output, cstr("\n"));

    write_str_to_file(&output, cstr("\n"));
    s8 name = gen_random_syllable(&randBase, LangRestrict_DoubleAndHardClusters,  &permArena);
    write_str_to_file(&output, gen_final_output(name, LangReplace_Default, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsSlavic, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsGerman | LangReplace_VowelUmlauts, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsFrench | LangReplace_VowelAcutes, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsChinese, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelWelsh, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelDiphthongs, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelDoubles, &permArena));
    write_str_to_file(&output, cstr("\n"));

    write_str_to_file(&output, cstr("\n"));
    name = gen_random_syllables(&randBase, LangRestrict_DoubleAndHardClusters, 2, 5, &permArena);
    write_str_to_file(&output, gen_final_output(name, LangReplace_Default, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsSlavic, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsGerman | LangReplace_VowelUmlauts, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsFrench | LangReplace_VowelAcutes, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_ConsChinese, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelWelsh, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelDiphthongs, &permArena));
    write_str_to_file(&output, cstr("\n"));
    write_str_to_file(&output, gen_final_output(name, LangReplace_VowelDoubles, &permArena));
    write_str_to_file(&output, cstr("\n"));

    close_file(&output);
    return 0;
}

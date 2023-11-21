
        // NOTE(michiel): Valid range 0x000000 - 0x00007F - 1 byte in utf8  (0x00-0x7F)
// NOTE(michiel): Valid range 0x000080 - 0x0007FF - 2 bytes in utf8 ([0xC2,0x80-0xBF]-[0xDF,0x80-0xBF])
// NOTE(michiel): Valid range 0x000800 - 0x00D7FF - 3 bytes in utf8 ([0xE0,0xA0-0xBF,0x80-0xBF]-[0xED,0x80-0x9F])
// NOTE(michiel): Valid range 0x00E000 - 0x00FFFF - 3 bytes in utf8
// NOTE(michiel): Valid range 0x010000 - 0x10FFFF - 4 bytes in utf8

// \x00 - \x7F
// \xC2\x80 - \xDF\xBF
// \xE0\xA0\x80 - \xED\x9F\xBF
// \xEE\x80\x80 - \xEF\xBF\xBF
// \xF0\x90\x80\x80 - \xF4\x8F\xBF\xBF

// S0
// \x00 - \x7F -> COMPL
// \xC2 - \xDF -> S1
// \xE0 - \xE0 -> S2
// \xE1 - \xEC -> S3
// \xEE - \xEF -> S3
// \xED - \xED -> S4
// \xF0 - \xF0 -> S5
// \xF1 - \xF3 -> S6
// \xF4 - \xF4 -> S7
// else -> ERROR

// S1
// \x80 - \xBF -> COMPL
// else -> ERROR

// S2
// \xA0 - \xBF -> S1
// else -> ERROR

// S3
// \x80 - \xBF -> S1
// else -> ERROR

// S4
// \x80 - \x9F -> S1
// else -> ERROR

// S5
// \x90 - \xBF -> S3
// else -> error

// S6
// \x80 - \xBF -> S3
// else -> error

// S7
// \x80 - \x8F -> S3
// else -> error

global const i8 gUtf8States[8][256] = {
    {   // S0
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0x00-0x1F
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0x20-0x3F
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0x40-0x5F
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0x60-0x7F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x80-0x9F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xA0-0xBF
        -1,-1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1, // 0xC0-0xDF
        +2,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+4,+3,+3,+5,+6,+6,+6,+7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S1
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0x80-0x9F
        +0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0,+0, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S2
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x80-0x9F
        +1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S3
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        +1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1, // 0x80-0x9F
        +1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S4
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        +1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1,+1, // 0x80-0x9F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S5
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3, // 0x80-0x9F
        +3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S6
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        +3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3, // 0x80-0x9F
        +3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
    {   // S7
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x00-0x1F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x20-0x3F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x40-0x5F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x60-0x7F
        +3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,+3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0x80-0x9F
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xA0-0xBF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xC0-0xDF
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 0xE0-0xFF
    },
};

global u8 gUtf8Masks[2][8] = {
    {0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,},
    {0x7F,0x1F,0x0F,0x0F,0x0F,0x07,0x07,0x07,},
};

func i32 utf8_decode_internal(i32 state, u32 *codepoint, u32 curByte)
{
    i32 nextState = gUtf8States[state][curByte];
    *codepoint = (*codepoint << 6) | (curByte & gUtf8Masks[!state][nextState & 0x7]);
    return nextState;
}

func u32 utf8_decode(s8 *str)
{
    u32 result = 0xFFFD;
    u32 codepoint = 0;
    i32 curState = 0;
    while (str->size)
    {
        curState = utf8_decode_internal(curState, &codepoint, str->data[0]);
        *str = s8adv(*str, 1);
        if (curState == 0) {
            result = codepoint;
            break;
        } else if (curState < 0) {
            // NOTE(michiel): Error
            break;
        }
    }

    return result;
}

func s8 utf8_encode(u32 codepoint, u8 *buffer, sze maxSize)
{
    s8 result = {buffer, 0};
    if ((codepoint < 0x80) && (maxSize > 0)) {
        result.data[result.size++] = (u8)codepoint;
    } else if ((codepoint < 0x800) && (maxSize > 1)) {
        result.data[result.size++] = 0xC0 | (u8)(codepoint >> 6);
        result.data[result.size++] = (u8)(0x80 | (codepoint & 0x3F));
    } else if ((codepoint < 0x10000) && (maxSize > 2)) {
        result.data[result.size++] = 0xE0 | (u8)(codepoint >> 12);
        result.data[result.size++] = (u8)(0x80 | ((codepoint >> 6) & 0x3F));
        result.data[result.size++] = (u8)(0x80 | (codepoint & 0x3F));
    } else if (maxSize > 3) {
        result.data[result.size++] = 0xF0 | (u8)(codepoint >> 18);
        result.data[result.size++] = (u8)(0x80 | ((codepoint >> 12) & 0x3F));
        result.data[result.size++] = (u8)(0x80 | ((codepoint >>  6) & 0x3F));
        result.data[result.size++] = (u8)(0x80 | (codepoint & 0x3F));
    }

    return result;
}

#define is_utf_surrogate(x)    (((x) >= 0xD800) && ((x) <= 0xDFFF))
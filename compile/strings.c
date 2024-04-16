typedef struct InternString
{
    struct InternString *link;
    sze len;
    u8 data[8];  // NOTE(michiel): At least size 8
} InternString;
global InternString *gStrBuckets[1024];

func u32 bucket_hash(s8 s)
{
    u64 preResult = IMM_U64(0x100);
    for (sze index = 0; index < s.size; ++index) {
        preResult ^= (u64)s.data[index];
        preResult *= IMM_U64(1111111111111111111);
    }
    u32 result = (u32)((preResult >> 32) ^ (preResult & 0xFFFFFFFF));
    result &= countof(gStrBuckets) - 1;
    return result;
}

func s8 string(s8 str)
{
    s8 result = {0};

    u32 hash = bucket_hash(str);
    for (InternString *p = gStrBuckets[hash]; p; p = p->link)
    {
        s8 pStr = s8(p->len, p->data);
        if (s8eq(pStr, str)) {
            result = pStr;
        }
    }

    if (!result.data)
    {
        sze newSize = offsetof(InternString, data) + str.size;
        InternString *p = allocate(newSize, ArenaType_Perm);
        p->len = str.size;
        memcpy(p->data, str.data, str.size);
        p->link = gStrBuckets[hash];
        gStrBuckets[hash] = p;
        result = s8(p->len, p->data);
    }
    return result;
}

func s8 stringd(i64 d)
{
    u8 tempBuf[25];
    u8 *at = tempBuf + sizeof(tempBuf);

    u64 u;
    if (d == I64_MIN) {
        u = (u64)I64_MAX + 1;
    } else if (d < 0) {
        u = -d;
    } else {
        u = d;
    }

    do {
        *(--at) = (u % 10) + '0';
        u /= 10;
    } while (u != 0);

    if (d < 0) {
        *(--at) = '-';
    }
    s8 result = string(s8(tempBuf + sizeof(tempBuf) - at, at));
    return result;
}

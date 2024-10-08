func b32 is_newline(u32 codepoint)
{
    b32 result = ((codepoint == '\r') || (codepoint == '\n'));
    return result;
}

func sze cstrlen(char *s)
{
    char *start = s;
    while (*s) { ++s; }
    sze result = s - start;
    return result;
}

func s8 create_s8(Arena *perm, s8 s)
{
    s8 result = {0};
    result.size = s.size;
    result.data = create(perm, u8, s.size, Alloc_NoClear);
    if (s.size > 0) {
        memcpy(result.data, s.data, (usze)s.size);
    }
    return result;
}

func s8 create_empty_s8(Arena *perm, sze cap)
{
    s8 result = {0};
    result.size = cap;
    result.data = create(perm, u8, cap);
    return result;
}

func b32 s8eq(s8 a, s8 b)
{
    b32 result = (a.size == b.size) && ((a.size == 0) || (a.data == b.data) || (memcmp(a.data, b.data, (usze)a.size) == 0));
    return result;
}

func sze s8cmp(s8 a, s8 b)
{
    sze size = a.size < b.size ? a.size : b.size;
    sze result = 0;
    for (sze index = 0; index < size; ++index) {
        result = a.data[index] - b.data[index];
        if (result) {
            break;
        }
    }
    result = result ? result : a.size - b.size;
    return result;
}

func u64 s8hash(s8 s)
{
    u64 result = IMM_U64(0x100);
    for (sze index = 0; index < s.size; ++index) {
        result ^= (u64)s.data[index];
        result *= IMM_U64(1111111111111111111);
    }
    return result;
}

func s8 s8adv(s8 s, sze amount)
{
    s8 result = s;
    amount = s.size < amount ? s.size : amount;
    result.size -= amount;
    result.data += amount;
    return result;
}

func s8 get_line(s8 s)
{
    // TODO(michiel): Make unicode compatible
    s8 result = {s.data, 0};
    while (s.size && !is_newline((u32)s.data[0])) {
        s = s8adv(s, 1);
    }
    result.size = s.data - result.data;
    return result;
}

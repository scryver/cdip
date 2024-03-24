typedef struct BitScan
{
    b32 found;
    i32 index;
} BitScan;

#if COMPILER_MSVC

func BitScan find_lsb32(u32 value)
{
    BitScan result = {0};
    result.found = _BitScanForward((unsigned long *)&result.index, value);
    return result;
}

func BitScan find_msb32(u32 value)
{
    BitScan result = {0};
    result.found = _BitScanReverse((unsigned long *)&result.index, value);
    return result;
}

#if (MEM_SIZE_BITS == 64)
func BitScan find_lsb64(u64 value)
{
    BitScan result = {0};
    result.found = _BitScanForward64((unsigned long *)&result.index, value);
    return result;
}

func BitScan find_msb64(u64 value)
{
    BitScan result = {0};
    result.found = _BitScanReverse64((unsigned long *)&result.index, value);
    return result;
}
#else
func BitScan find_lsb64(u64 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
        for(i32 test = 0; test < 64; ++test) {
            if (value & (IMM_U64(1) << test)) {
                result.index = test;
                break;
            }
        }
    }
    return result;
}

func BitScan find_msb64(u64 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
        for(i32 test = 63; test >= 0; --test) {
            if (value & (1 << test)) {
                result.index = test;
                break;
            }
        }
    }
    return result;
}
#endif // (MEM_SIZE_BITS == 64)

#else // COMPILER_MSVC

func BitScan find_lsb32(u32 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
#if __has_builtin(__builtin_ctz)
        result.index = __builtin_ctz(value);
#else
        for(i32 test = 0; test < 32; ++test) {
            if (value & (1 << test)) {
                result.index = test;
                break;
            }
        }
#endif
    }
    return result;
}

func BitScan find_msb32(u32 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
#if __has_builtin(__builtin_clz)
        result.index = __builtin_clz(value);
#else
        for(i32 test = 31; test >= 0; --test) {
            if (value & (1 << test)) {
                result.index = test;
                break;
            }
        }
#endif
    }
    return result;
}

func BitScan find_lsb64(u64 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
#if __has_builtin(__builtin_ctzl)
        result.index = __builtin_ctzl(value);
#else
        for(i32 test = 0; test < 64; ++test) {
            if (value & (IMM_U64(1) << test)) {
                result.index = test;
                break;
            }
        }
#endif
    }
    return result;
}

func BitScan find_msb64(u64 value)
{
    BitScan result = {0};
    result.found = (value > 0);
    if (result.found) {
#if __has_builtin(__builtin_clzl)
        result.index = __builtin_clzl(value);
#else
        for(i32 test = 63; test >= 0; --test) {
            if (value & (1 << test)) {
                result.index = test;
                break;
            }
        }
#endif
    }
    return result;
}

#endif // COMPILER_MSVC

func u32 popcount32(u32 value)
{
#if COMPILER_MSVC
    u32 result = __popcnt(value);
#elif __has_builtin(__builtin_popcount)
    u32 result = (u32)__builtin_popcount(value);
#else
    value = value - ((value >> 1) & IMM_U32(0x55555555));
    value = (value & IMM_U32(0x33333333)) + ((value >> 2) & IMM_U32(0x33333333));
    u32 result = ((value + ((value >> 4) & IMM_U32(0x0F0F0F0F))) * IMM_U32(0x01010101)) >> 24;
#endif
    return result;
}

func u64 popcount64(u64 value)
{
#if COMPILER_MSVC
    u64 result = __popcnt64(value);
#elif __has_builtin(__builtin_popcount)
    u64 result = (u64)__builtin_popcountll(value);
#else
    value = value - ((value >> 1) & IMM_U32(0x5555555555555555));
    value = (value & IMM_U32(0x3333333333333333)) + ((value >> 2) & IMM_U32(0x3333333333333333));
    u64 result = ((value + ((value >> 4) & IMM_U32(0x0F0F0F0F0F0F0F0F))) * IMM_U32(0x0101010101010101)) >> 56;
#endif
    return result;
}

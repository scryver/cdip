
#define maybeused(x)         (void)x

#define sizeof(x)            (sze)sizeof(x)
#define alignof(x)           (sze)_Alignof(x)
#if 0
;// NOTE(michiel): I just want my colour coding in 4coder and _Alignof throws the parser off
#endif
#define sizeof_member(T, m)  sizeof(((T *)0)->m)
#if __has_builtin(__builtin_offsetof)
#define offsetof(T, m)       (sze)__builtin_offsetof(T, m)
#else
#define offsetof(T, m)       (sze)&(((T *)0)->m)
#endif

#define countof(x)           (sizeof(x) / sizeof(*(x)))
#define lengthof(x)          (sizeof(x) - 1)
#define containerof(p, T, m) (T *)((byte *)ptr - offsetof(T, m))

#define ispow2(x)            (((x) != 0) && (((x) & ((x) - 1)) == 0))
#define roundup(x, n)        (((x) + ((n) - 1)) & ~((n) - 1))
#define rounddown(x, n)      ((x) & ~((n) - 1))

#define kilobytes(x)         ((x)*1024)
#define megabytes(x)         (kilobytes(x)*1024)
#define gigabytes(x)         (megabytes(x)*1024)

#if COMPILER_GCC || COMPILER_LLVM
#define invalid_code_path()  __builtin_unreachable()
#else
#define invalid_code_path()  __assume(0)
#endif

#define invalid_default()    default: { invalid_code_path(); } break

#if __has_builtin(__builtin_memcpy)
#define memcpy(d, s, c)      __builtin_memcpy(d, s, c)
#else
func void *memcpy(void *dest, const void *src, usze count)
{
    const byte *s = src;
    byte *d = dest;
    if (s < d) {
        s += count;
        d += count;
        while (count--) {
            *(--d) = *(--s);
        }
    } else {
        while (count--) {
            *d++ = *s++;
        }
    }
    return dest;
}
#endif

#if __has_builtin(__builtin_memset)
#define memset(d, v, c)      __builtin_memset(d, v, c)
#else
func void *memset(void *dest, i32 value, usze count)
{
    byte *d = dest;
    while (count--) {
        *d++ = (byte)value;
    }
    return dest;
}
#endif

#if __has_builtin(__builtin_memcmp)
#define memcmp(a, b, c)      __builtin_memcmp(a, b, c)
#else
func i32 memcmp(const void *a, const void *b, usze count)
{
    const byte *ax = a;
    const byte *bx = b;
    i32 result = 0;
    while (count-- && (result == 0)) {
        result = (i32)(*ax++ - *bx++);
    }
    return result;
}
#endif

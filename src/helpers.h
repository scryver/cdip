
#define func     static
#define global   static
#define persist  static

#ifndef __has_builtin
#define __has_builtin(x)     0
#endif

#define maybeused(x)         (void)x

#define sizeof(x)            (sze)sizeof(x)
#define alignof(x)           (sze)_Alignof(x)
#define countof(x)           (sizeof(x) / sizeof(*(x)))
#define lengthof(x)          (sizeof(x) - 1)
#define containerof(p, T, m) (T *)((byte *)ptr - offsetof(T, m))

#define ispow2(x)            (((x) != 0) && (((x) & ((x) - 1)) == 0))

#if COMPILER_GCC || COMPILER_LLVM
#define invalid_code_path()  __builtin_unreachable()
#else
#define invalid_code_path()  __assume(0)
#endif

#define invalid_default()    default: { invalid_code_path(); }

#define assert(c)            while (!(c)) invalid_code_path()

#if CDIP_DEVELOP

#if COMPILER_MSVC
#define debugbreak()         __debugbreak()
#elif __has_builtin(__builtin_debugtrap)
#define debugbreak()         __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define debugbreak()         __builtin_trap()
#else
#define debugbreak()         assert(0)
#endif

#define unused(x)            (void)x

#else // CDIP_DEVELOP

#define debugbreak()
#define unused(x)            x  // NOTE(michiel): Be warned about unused stuff here

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


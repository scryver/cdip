
#define func     static
#define global   static
#define persist  static

#ifndef __has_builtin
#define __has_builtin(x)     0
#endif

#define maybeused(x)         (void)x

#ifdef offsetof
#undef offsetof
#endif

#define sizeof(x)            (sze)sizeof(x)
#define sizeof_member(T, m)  sizeof(((T *)0)->m)
#define alignof(x)           (sze)_Alignof(x)

#if __has_builtin(__builtin_offsetof)
#define offsetof(T, m)       (sze)__builtin_offsetof(T, m)
#else
#define offsetof(T, m)       (sze)&(((T *)0)->m)
#endif

#define countof(x)           (sizeof(x) / sizeof(*(x)))
#define lengthof(x)          (sizeof(x) - 1)
#define containerof(p, T, m) (T *)((byte *)ptr - offsetof(T, m))

#define ispow2(x)            (((x) != 0) && (((x) & ((x) - 1)) == 0))

#define kilobytes(x)         ((x)*1024)
#define megabytes(x)         (kilobytes(x)*1024)
#define gigabytes(x)         (megabytes(x)*1024)

#if COMPILER_GCC || COMPILER_LLVM
#define invalid_code_path()  __builtin_unreachable()
#else
#define invalid_code_path()  __assume(0)
#endif

#define invalid_default()    default: { invalid_code_path(); } break

#if CDIP_DEVELOP

#if COMPILER_MSVC
#define debugbreak()         __debugbreak()
#elif __has_builtin(__builtin_debugtrap)
#define debugbreak()         __builtin_debugtrap()
#elif defined(__i386__) || defined(__x86_64__)
#define debugbreak()         __asm__ volatile ("int3")
#else
#include <signal.h>
#if defined(SIGTRAP)
#define debugbreak()         raise(SIGTRAP)
#else
#define debugbreak()         raise(SIGABRT)
#endif
#endif

#define assert(c)            while (!(c)) debugbreak()
//invalid_code_path()
#ifdef static_assert
#define compile_assert(c)    static_assert(c)
#else
#define compile_assert(c)    _Static_assert(c, "Assertion failed: " #c)
#endif

#define unused(x)            (void)x

#else // CDIP_DEVELOP

#define assert(c)
#define compile_assert(c)
#define debugbreak()
#define unused(x)            x  // NOTE(michiel): Be warned about unused stuff here

#endif

func void os_exit(s8 message, i32 returnCode);

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

#if __has_builtin(__builtin_memmove)
#define memmove(d, s, c)      __builtin_memmove(d, s, c)
#else
func void *memmove(void *dest, const void *src, usze count)
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


// NOTE(michiel): Windows functions so we don't need any special headers
#if PLATFORM_WIN32

typedef void *handle;

typedef struct FileTime
{
    u32 lowDateTime;
    u32 highDateTime;
} FileTime;

typedef struct Win32FileAttribData
{
    u32 fileAttributes;
    FileTime creationTime;
    FileTime lastAccessTime;
    FileTime lastWriteTime;
    u32 fileSizeHigh;
    u32 fileSizeLow;
} Win32FileAttribData;

#define W32FUNC(ret)  __declspec(dllimport) ret __stdcall

#define CP_UTF8                  65001
#define MEM_COMMIT               0x00001000
#define MEM_RESERVE              0x00002000
#define MEM_DECOMMIT             0x00004000
#define MEM_RELEASE              0x00008000
#define PAGE_READWRITE           0x04
#define INVALID_HANDLE_VALUE     ((void *)(uptr)-1)
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF
#define GENERIC_READ             0x80000000
#define GENERIC_WRITE            0x40000000
#define FILE_APPEND_DATA         4
#define FILE_SHARE_READ          0x00000001
#define OPEN_ALWAYS              4
#define OPEN_EXISTING            3
#define CREATE_ALWAYS            2
#define FILE_ATTRIBUTE_NORMAL    0x80

W32FUNC(u32)    GetLastError(void);
W32FUNC(byte *) VirtualAlloc(byte *address, usze size, u32 allocType, u32 protect);
W32FUNC(b32)    VirtualFree(byte *address, usze size, u32 freeType);
W32FUNC(handle) GetStdHandle(u32 stdHandle);
W32FUNC(handle) CreateFileW(wchar_t *filename, u32 desiredAccess, u32 shareMode, void *security, u32 creationDisposition, u32 flagsAttribs, handle templateFile);
W32FUNC(b32)    CloseHandle(handle file);
W32FUNC(b32)    ReadFile(handle file, byte *buffer, u32 toReadCount, u32 *readCount, void *overlapped);
W32FUNC(b32)    WriteFile(handle file, byte *buffer, u32 toWriteCount, u32 *writtenCount, void *overlapped);
W32FUNC(u32)    SetFilePointer(handle file, i32 distanceLow, i32 *distanceHigh, u32 moveMethod);
W32FUNC(b32)    GetFileAttributesExW(wchar_t *filename, i32 infoLevelId, void *fileInfo);
W32FUNC(i32)    MultiByteToWideChar(u32 codePage, u32 flags, u8 *multiByteStr, i32 multiByteCount, wchar_t *wideCharStr, i32 wideCharCount);
W32FUNC(void)   ExitProcess(u32 exitCode);

#endif // PLATFORM_WIN32


// Unsigned integers
typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;

// Signed integers
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;

// Boolean integers
typedef int32_t       b32;

// Pointer stuff
typedef char          byte;
typedef ptrdiff_t     sze;
typedef uintptr_t     uptr;
typedef size_t        usze;

// Floating point
typedef float         f32;
typedef double        f64;

typedef struct s8
{
    u8 *data;
    sze size;
} s8;
#define cstr(s)   (s8){(u8 *)s, lengthof(s)}

typedef struct s8l
{
    char data[1];  // NOTE(michiel): size encoded into data, at least 1 byte of data
} s8l;

typedef struct buf
{
    byte *data;
    sze   size;
} buf;
#define buf(s, d)  (buf){(byte *)(d), s}

#if COMPILER_MSVC
#define IMM_U8(lit)         (lit##ui8)
#define IMM_U16(lit)        (lit##ui16)
#define IMM_U32(lit)        (lit##ui32)
#define IMM_U64(lit)        (lit##ui64)
#define IMM_S8(lit)         (lit##i8)
#define IMM_S16(lit)        (lit##i16)
#define IMM_S32(lit)        (lit##i32)
#define IMM_S64(lit)        (lit##i64)
#if COMPILER_MSVC_X86
#define IMM_SIZE(lit)       (lit##i32)
#define IMM_USIZE(lit)      (lit##ui32)
#else
#define IMM_SIZE(lit)       (lit##i64)
#define IMM_USIZE(lit)      (lit##ui64)
#endif
#else
#define IMM_U8(lit)         ((u8)lit##U)
#define IMM_U16(lit)        ((u16)lit##U)
#define IMM_U32(lit)        ((u32)lit##U)
#define IMM_U64(lit)        ((u64)lit##ULL)
#define IMM_S8(lit)         ((i8)lit)
#define IMM_S16(lit)        ((i16)lit)
#define IMM_S32(lit)        ((i32)lit)
#define IMM_S64(lit)        ((i64)lit##LL)
#define IMM_SIZE(lit)       ((sze)lit##LL)
#define IMM_USIZE(lit)      ((usze)lit##ULL)
#endif

#define U8_MAX              IMM_U8(0xFF)
#define U16_MAX             IMM_U16(0xFFFF)
#define U32_MAX             IMM_U32(0xFFFFFFFF)
#define U64_MAX             IMM_U64(0xFFFFFFFFFFFFFFFF)

#define S8_MIN              IMM_S8(0x80)
#define S8_MAX              IMM_S8(0x7F)
#define S16_MIN             IMM_S16(0x8000)
#define S16_MAX             IMM_S16(0x7FFF)
#define S32_MIN             IMM_S32(0x80000000)
#define S32_MAX             IMM_S32(0x7FFFFFFF)
#define S64_MIN             IMM_S64(0x8000000000000000)
#define S64_MAX             IMM_S64(0x7FFFFFFFFFFFFFFF)


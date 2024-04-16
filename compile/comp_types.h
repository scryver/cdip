
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

#define enum8(name)   i8
#define enum16(name)  i16
#define enum32(name)  i32

#define flag8(name)   u8
#define flag16(name)  u16
#define flag32(name)  u32

#if COMPILER_MSVC

#ifndef _WIN64
#define MEM_SIZE_BITS 32
#else
#define MEM_SIZE_BITS 64
#endif

#define IMM_U8(lit)         (lit##ui8)
#define IMM_U16(lit)        (lit##ui16)
#define IMM_U32(lit)        (lit##ui32)
#define IMM_U64(lit)        (lit##ui64)
#define IMM_I8(lit)         (lit##i8)
#define IMM_I16(lit)        (lit##i16)
#define IMM_I32(lit)        (lit##i32)
#define IMM_I64(lit)        (lit##i64)
#if MEM_SIZE_BITS == 32
#define IMM_SIZE(lit)       (lit##i32)
#define IMM_USIZE(lit)      (lit##ui32)
#else
#define IMM_SIZE(lit)       (lit##i64)
#define IMM_USIZE(lit)      (lit##ui64)
#endif

#else // COMPILER_MSVC

#if (SIZE_MAX == 0xFFFFFFFF)
#define MEM_SIZE_BITS 32
#elif (SIZE_MAX == 0xFFFFFFFFFFFFFFFFULL)
#define MEM_SIZE_BITS 64
#else
#error Unknown number of bits in memory size
#endif

#define IMM_U8(lit)         ((u8)lit##U)
#define IMM_U16(lit)        ((u16)lit##U)
#define IMM_U32(lit)        ((u32)lit##U)
#define IMM_U64(lit)        ((u64)lit##ULL)
#define IMM_I8(lit)         ((i8)lit)
#define IMM_I16(lit)        ((i16)lit)
#define IMM_I32(lit)        ((i32)lit)
#define IMM_I64(lit)        ((i64)lit##LL)
#if MEM_SIZE_BITS == 32
#define IMM_SIZE(lit)       ((sze)lit##L)
#define IMM_USIZE(lit)      ((usze)lit##UL)
#else
#define IMM_SIZE(lit)       ((sze)lit##LL)
#define IMM_USIZE(lit)      ((usze)lit##ULL)
#endif // MEM_SIZE_BITS

#endif // COMPILER_MSVC

#define U8_MAX              IMM_U8(0xFF)
#define U16_MAX             IMM_U16(0xFFFF)
#define U32_MAX             IMM_U32(0xFFFFFFFF)
#define U64_MAX             IMM_U64(0xFFFFFFFFFFFFFFFF)

#define I8_MIN              IMM_I8(0x80)
#define I8_MAX              IMM_I8(0x7F)
#define I16_MIN             IMM_I16(0x8000)
#define I16_MAX             IMM_I16(0x7FFF)
#define I32_MIN             IMM_I32(0x80000000)
#define I32_MAX             IMM_I32(0x7FFFFFFF)
#define I64_MIN             IMM_I64(0x8000000000000000)
#define I64_MAX             IMM_I64(0x7FFFFFFFFFFFFFFF)

#if MEM_SIZE_BITS == 32
#define SZE_MIN             IMM_SIZE(0x80000000)
#define SZE_MAX             IMM_SIZE(0x7FFFFFFF)
#define USZE_MAX            IMM_USIZE(0xFFFFFFFF)
#else
#define SZE_MIN             IMM_SIZE(0x8000000000000000)
#define SZE_MAX             IMM_SIZE(0x7FFFFFFFFFFFFFFF)
#define USZE_MAX            IMM_USIZE(0xFFFFFFFFFFFFFFFF)
#endif

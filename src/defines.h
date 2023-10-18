
#ifndef CDIP_DEVELOP
#define CDIP_DEVELOP      1
#endif

// NOTE(michiel): Compiler is auto determined
#define COMPILER_MSVC     0
#define COMPILER_MSVC_X86 0
#define COMPILER_LLVM     0
#define COMPILER_GCC      0

// NOTE(michiel): Platform is auto determined
#define PLATFORM_WIN32    0
#define PLATFORM_LINUX    0
#define PLATFORM_APPLE    0


// NOTE(michiel): Auto determination

#if defined(_MSC_VER) && _MSC_VER

// NOTE(michiel): Windows
#undef  COMPILER_MSVC
#define COMPILER_MSVC 1
#ifndef _WIN64
#undef COMPILER_MSVC_X86
#define COMPILER_MSVC_X86 1
#endif

#elif defined(__clang__) && __clang__

// NOTE(michiel): Clang
#undef  COMPILER_LLVM
#define COMPILER_LLVM 1

#elif defined(__GNUC__) && __GNUC__

// NOTE(michiel): GCC
#undef  COMPILER_GCC
#define COMPILER_GCC 1

#else
#error Unknown compiler
#endif

#if defined(_WIN32)
#undef PLATFORM_WIN32
#define PLATFORM_WIN32 1
#elif defined(__linux__)
#undef PLATFORM_LINUX
#define PLATFORM_LINUX 1
#define _GNU_SOURCE    1
#elif defined(__APPLE__)
#undef PLATFORM_APPLE
#define PLATFORM_APPLE 1
#else
#error Unknown platform
#endif

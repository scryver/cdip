#include "../src/defines.h"
#include <stdint.h>
#include <stddef.h>

#if PLATFORM_WIN32

#pragma comment(linker, "/subsystem:console")
#pragma comment(lib, "kernel32.lib")

#include <intrin.h>

#else

#include <stdlib.h>

#include <unistd.h>
#include <time.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#endif

#include "../src/types.h"
#include "../src/helpers.h"
#include "../src/helper_funcs.h"
#include "../src/memory.h"
#include "../src/memarena.h"
#include "../src/strings.h"
#include "../src/files.h"

typedef enum LispKind
{
    Lisp_Symbol,
    Lisp_Number,
    Lisp_Atom,
    Lisp_List,
    Lisp_Expr,
    Lisp_Env,
} LispKind;

typedef struct LispValue
{
    LispKind kind;
    union {
        s8 str;
        f64 f;
        i64 d;

    };
} LispValue;

int main(int argCount, char **arguments)
{
    int result = 0;

    Arena permArena = create_arena(1024*1024);
    Arena tempArena = create_arena(1024*1024);



    return result;
}

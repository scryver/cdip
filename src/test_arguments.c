#include "defines.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#if PLATFORM_WIN32

#pragma comment(linker, "/subsystem:console")
#pragma comment(lib, "kernel32.lib")

#else

#include <stdlib.h>

#include <unistd.h>

// For memory
#include <sys/mman.h>

// For files
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include "types.h"
#include "helpers.h"
#include "memory.h"
#include "memarena.h"
#include "arrays.h"
#include "strings.h"
#include "hashmap.h"
#include "files.h"

typedef enum ArgumentError
{
    ArgumentError_NoError,
    ArgumentError_TooManyArguments,
    ArgumentError_MissingRequired,
    ArgumentError_InvalidArgument,
    ArgumentError_AlreadySet,
    ArgumentErrorCount,
} ArgumentError;
typedef struct Arguments
{
    i32 argCount;
    char **arguments;
    s8 option;
    ArgumentError error;
} Arguments;

global char *gArgumentString[ArgumentErrorCount] = {
    "success",
    "too many arguments",
    "missing required field",
    "invalid argument",
    "already set"
};

#define args_opt(args, s, l)  (((s) && ((args).option.size == 1) && ((args).option.data[0] == (s))) || \
((l) && s8eq((args).option, cstr(l))))

func b32 args_next(Arguments *args)
{
    b32 result = false;

    if (args->error == ArgumentError_NoError)
    {
        if (args->option.data && (args->option.size == 1) && (++args->option.data)[0]) {
            // NOTE(michiel): Next one of the short options
            result = true;
        } else if (args->option.data && (args->option.size > 1) && (args->option.data[args->option.size])) {
            args->error = ArgumentError_TooManyArguments;
        } else if ((args->argCount >= 2) && (args->arguments[1][0] == '-')) {
            switch (args->arguments[1][1])
            {
                case 0: {} break;
                case '-': {
                    sze len = cstrlen(args->arguments[1]);
                    for (sze strIdx = 2; strIdx < len; ++strIdx) {
                        if (args->arguments[1][strIdx] == '=') {
                            len = strIdx;
                            break;
                        }
                    }
                    ++args->arguments;
                    --args->argCount;

                    if (len > 2) {
                        args->option = s8(len - 2, args->arguments[0] + 2);
                        result = true;
                    }
                } break;
                default: {
                    ++args->arguments;
                    --args->argCount;
                    args->option = s8(1, args->arguments[0] + 1);
                    result = true;
                } break;
            }
        }
    }

    return result;
}

func char *args_arg(Arguments *args, b32 required)
{
    char *result = 0;
    if (args->option.data[args->option.size]) {
        result = (char *)args->option.data + args->option.size + (args->option.size > 1);
        args->option = (s8){0};
    } else if (required && (args->argCount >= 2)) {
        --args->argCount;
        result = (++args->arguments)[0];
    }
    args->error = required && !result ? ArgumentError_MissingRequired : ArgumentError_NoError;
    return result;
}

func void print_usage(char *progname, b32 isErr)
{
    fprintf(isErr ? stderr : stdout,
            "Usage: %s [option] [args]\n"
            "    -f, --flag              : set or not set\n"
            "    -r, --required Req      : required argument\n"
            "    -o, --optional [=Opt]   : optional argument\n"
            "    -c, --count             : can be multiple times set\n"
            "    -h, --help              : this help text\n", progname);
}

i32 main(int argCount, char **arguments)
{
    // NOTE(michiel): ./gebouw/test-arguments -f -r hallo -cooudo
    // ./gebouw/test-arguments -f --required hallo -cooudo
    // ./gebouw/test-arguments -f --required=hallo -cooudo
    // ./gebouw/test-arguments -f --required hallo -c --optional=oudo
    b32 flag = false;
    char *required = "";
    char *optional = "default";
    u32 count = 0;

    Arguments args = {argCount, arguments};
    while (args_next(&args))
    {
        if (args_opt(args, 'f', "flag")) {
            if (flag) {
                args.error = ArgumentError_AlreadySet;
            } else {
                flag = true;
            }
        } else if (args_opt(args, 'r', "required")) {
            required = args_arg(&args, true);
        } else if (args_opt(args, 'o', "optional")) {
            char *opt = args_arg(&args, false);
            optional = opt ? opt : optional;
        } else if (args_opt(args, 'c', "count")) {
            ++count;
        } else if (args_opt(args, 'h', "help")) {
            print_usage(arguments[0], false);
            exit(0);
        } else {
            args.error = ArgumentError_InvalidArgument;
        }
    }

    i32 result = 0;
    if (args.error == ArgumentError_NoError)
    {
        fprintf(stdout, "Parsed:\n");
        fprintf(stdout, "  - flag     : %s\n", flag ? "true" : "false");
        fprintf(stdout, "  - required : %s\n", required);
        fprintf(stdout, "  - optional : %s\n", optional);
        fprintf(stdout, "  - count    : %u\n", count);
        for (i32 argIdx = 1; argIdx < args.argCount; ++argIdx) {
            fprintf(stdout, "Arg %d: %s\n", argIdx, args.arguments[argIdx]);
        }
    }
    else
    {
        fprintf(stderr, "Error: '%.*s' %s\n", (int)args.option.size, args.option.data, gArgumentString[args.error]);
        print_usage(arguments[0], true);
        result = 1;
    }

    return result;
}

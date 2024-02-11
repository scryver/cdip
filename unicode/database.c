#include "../src/defines.h"

#include <stdint.h>
#include <stddef.h>

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

#include "../src/types.h"
#include "../src/helpers.h"
#include "../src/memory.h"
#include "../src/memarena.h"
#include "../src/strings.h"
#include "../src/files.h"

func s8 get_param(s8 *line)
{
    s8 result = {line->data, 0};
    while (line->size && (line->data[0] != ';')) {
        *line = s8adv(*line, 1);
    }
    result.size = line->data - result.data;
    *line = s8adv(*line, 1);
    return result;
}

int main(int argCount, char **arguments)
{
    unused(argCount);
    unused(arguments);

    Arena permArena = create_arena(megabytes(128));
    Arena scratchArena = create_arena(megabytes(1));

    OsFile output = open_file(cstr("stdout"), OsFile_Write, &permArena, scratchArena);

    FileResult ucdFileResult = read_entire_file(cstr("./unicode/UnicodeData.txt"), &permArena, scratchArena);
    if (ucdFileResult.error == OsFile_NoError)
    {
        s8 ucdFile = {(u8*)ucdFileResult.fileBuf.data, ucdFileResult.fileBuf.size};
        while (ucdFile.size)
        {
            s8 line = get_line(ucdFile);
            ucdFile = s8adv(ucdFile, line.size);
            while (ucdFile.size && is_newline(ucdFile.data[0])) {
                ucdFile = s8adv(ucdFile, 1);
            }

            s8 codepoint = get_param(&line);
            s8 name = get_param(&line);
            s8 generalCategory = get_param(&line);
            s8 canonicalCombiningClass = get_param(&line);
            s8 bidiClass = get_param(&line);
            s8 decompTypeMap = get_param(&line);
            s8 numericVal6 = get_param(&line);
            s8 numericVal7 = get_param(&line);
            s8 numericVal8 = get_param(&line);
            s8 bidiMirrored = get_param(&line);
            s8 obsoleteName = get_param(&line);
            s8 obsoleteComment = get_param(&line);
            s8 simpleUpperCase = get_param(&line);
            s8 simpleLowerCase = get_param(&line);
            s8 simpleTitleCase = get_param(&line);

            if (s8eq(numericVal6, cstr("0"))) {
                write_to_file(&output, buf(codepoint.size, codepoint.data));
                char divi = ',';
                write_to_file(&output, buf(1, &divi));
                write_to_file(&output, buf(name.size, name.data));
                divi = '\n';
                write_to_file(&output, buf(1, &divi));
            }
        }
    }
    else
    {
        os_exit(cstr("Can't open file UnicodeData.txt\n"), 1);
    }

    return 0;
}
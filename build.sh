#!/bin/bash

curDir="$PWD"
codeDir="$curDir/src"
unicodeDir="$curDir/unicode"
fantasyDir="$curDir/fantasy"
lispDir="$curDir/lips"
buildDir="$curDir/gebouw"

# Debug opts
baseOpts="-O0 -ggdb -g3"
baseClang=""
baseGcc=""
# Release opts
#baseOpts="-O2"
#baseClang="-fsanitize-trap"
#baseGcc="-fsanitize-undefined-trap-on-error"

# removed: -Wno-sign-conversion
sharedOpts="$baseOpts -std=c11 -Wall -Wextra -Werror -Wshadow -Wdouble-promotion -Wundef -Wconversion -Wno-missing-braces -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers -Wno-strict-aliasing -Wno-trigraphs -fno-delete-null-pointer-checks -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections -fsanitize=unreachable -Wl,--gc-sections"
optsClang="$sharedOpts $baseClang -march=x86-64 -msha -msse4 -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-writable-strings -Wno-c99-extensions -Wno-four-char-constants -Wno-keyword-macro"
optsGcc="$sharedOpts $baseGcc -march=x86-64 -Wno-write-strings -Wno-unused-but-set-variable -Wno-multichar" # -Wpedantic

#compiler=clang
#opts="$optsClang"
compiler=gcc
opts="$optsGcc"

mkdir -p "$buildDir"

echo "Building test-base..."

cd "$buildDir" > /dev/null

    $compiler $opts "$codeDir"/test_experiment.c -o test-experiment &
    $compiler $opts "$codeDir"/test_arguments.c -o test-arguments &
    $compiler $opts "$unicodeDir"/database.c -o unicode-db &
    $compiler $opts "$unicodeDir"/test_utfenc.c -o test-utf-encoding &

    $compiler $opts "$fantasyDir"/namegen.c -o fantasy-name &
    $compiler $opts "$lispDir"/lips.c -o lips &

    #$compiler $opts -isystem /usr/include/freetype2 "$curDir"/gui/x11test.c -o x11-test -lX11 -lXft &
    $compiler $opts "$curDir"/gui/x11test.c -o x11-test -lX11 -lz -lm &
    $compiler $opts "$curDir"/gui/wavefunccollapse.c -o wave-func-collapse -lX11 &

    $compiler $opts "$curDir"/keyval/keyval_server.c -o keyval-server &
    $compiler $opts "$curDir"/keyval/keyval_client.c -o keyval-client &

cd "$curDir" > /dev/null

wait

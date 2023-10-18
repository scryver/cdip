#!/bin/bash

curDir="$PWD"
codeDir="$curDir/src"
buildDir="$curDir/gebouw"

# Debug opts
baseOpts="-O0 -ggdb -g3"
baseClang=""
baseGcc=""
# Release opts
#baseOpts="-O2"
#baseClang="-fsanitize-trap"
#baseGcc="-fsanitize-undefined-trap-on-error"

sharedOpts="$baseOpts -std=c11 -Wall -Wextra -Werror -Wshadow -Wdouble-promotion -Wundef -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers -Wno-strict-aliasing -Wno-trigraphs -fno-delete-null-pointer-checks -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections -fsanitize=unreachable -Wl,--gc-sections"
optsClang="$sharedOpts $baseClang -march=x86-64 -msha -msse4 -pedantic -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-writable-strings -Wno-c99-extensions -Wno-four-char-constants -Wno-keyword-macro"
optsGcc="$sharedOpts $baseGcc -march=x86-64 -Wno-write-strings -Wno-unused-but-set-variable -Wno-multichar"

#compiler=clang
#opts="$optsClang"
compiler=gcc
opts="$optsGcc"

mkdir -p "$buildDir"

echo "Building test-base..."

cd "$buildDir" > /dev/null

    $compiler $opts "$codeDir"/test_experiment.c -o test-experiment &

cd "$curDir" > /dev/null

wait

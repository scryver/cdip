#!/bin/bash

# Remove trailing whitespace
find . -name "*.h" -type f -print0 | xargs -0 sed -i 's/[[:space:]]*$//'
find . -name "*.c" -type f -print0 | xargs -0 sed -i 's/[[:space:]]*$//'

# Warn for tabs
echo "TABS:"
grep -rn -P '\t' src
echo ""

set -e

#!/usr/bin/fish
find src -type f \( -name '*.c' -o -name '*.h' \) ! -name 'glad.c' -print0 | xargs -0 cat | wc -l

#!/bin/sh

find src -iname '*.c' | xargs clang-format -i
find include -iname '*.h' | xargs clang-format -i

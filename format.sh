#!/bin/sh

find src -iname '*.cc' | xargs clang-format -i
find include -iname '*.hh' | xargs clang-format -i

#!/bin/bash

# CC=g++
CC=clang++

INCLUDES="libs/fmt/include include"
LIBS="libs/fmt/build"
LIBS_TO_LINK="fmt curl"

BINARY="ranobe"

INCLUDE_FLAGS=""
LIBRARY_FLAGS=""
LINK_FLAGS=""

for inc in $INCLUDES; do
	INCLUDE_FLAGS="$INCLUDE_FLAGS -I$inc"
done

for lib in $LIBS; do
	LIBRARY_FLAGS="$LIBRARY_FLAGS -L$lib"
done

for lib in $LIBS_TO_LINK; do
	LINK_FLAGS="$LINK_FLAGS -l$lib"
done

echo "$CC $INCLUDE_FLAGS -o $BINARY src/*.cc $LIBRARY_FLAGS $LINK_FLAGS $@"
$CC $INCLUDE_FLAGS -o $BINARY src/*.cc $LIBRARY_FLAGS $LINK_FLAGS $@

if [ $? -eq 0 ]; then
	echo "Build successful. Output file: $BINARY"
else
	echo "Build failed."
fi


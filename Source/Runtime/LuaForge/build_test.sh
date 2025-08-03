#!/bin/bash

# Change to the script directory
cd "$(dirname "$0")"

echo "=== LuaForge Build Test ==="

# Create object files directory
mkdir -p obj

echo "Compiling LuaForge core files..."

# Compile essential files first
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lapi.o src/lapi.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lcode.o src/lcode.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lctype.o src/lctype.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ldebug.o src/ldebug.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ldo.o src/ldo.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ldump.o src/ldump.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lfunc.o src/lfunc.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lgc.o src/lgc.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/llex.o src/llex.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lmem.o src/lmem.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lobject.o src/lobject.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lopcodes.o src/lopcodes.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lparser.o src/lparser.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lstate.o src/lstate.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lstring.o src/lstring.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ltable.o src/ltable.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ltm.o src/ltm.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lundump.o src/lundump.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lvm.o src/lvm.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lzio.o src/lzio.c

echo "Compiling LuaForge class system..."
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lclass.o src/lclass.c

echo "Compiling library files..."
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lauxlib.o src/lauxlib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lbaselib.o src/lbaselib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lcorolib.o src/lcorolib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ldblib.o src/ldblib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/liolib.o src/liolib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lmathlib.o src/lmathlib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/loadlib.o src/loadlib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/loslib.o src/loslib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lstrlib.o src/lstrlib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/ltablib.o src/ltablib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/lutf8lib.o src/lutf8lib.c
gcc -c -O2 -Wall -DLUA_COMPAT_5_3 -o obj/linit.o src/linit.c

echo "Creating LuaForge library..."
ar rcs libluaforge.a obj/*.o
ranlib libluaforge.a

echo "Compiling test program..."
gcc -o compile_test compile_test.c libluaforge.a -lm

echo "Running test..."
./compile_test

echo "=== Build Test Complete ==="
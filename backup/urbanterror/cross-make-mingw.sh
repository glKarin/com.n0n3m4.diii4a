#!/bin/sh
exec make PLATFORM=mingw32 CC=i586-mingw32msvc-gcc LD=i586-mingw32msvc-ld WINDRES=i686-w64-mingw32-windres $*
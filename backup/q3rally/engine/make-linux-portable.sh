#!/bin/sh
ARCH=$1
ERROR=0

if [ -z "$ARCH" ] || [ $ERROR -ne 0 ]; then
	echo "Usage: make-linux-portable.sh <arch> [make options]"
	echo "  arch can be x86 or x86_64"
	echo
	echo "================================================================================"
	echo "This script allows shipping lib/x86/libSDL2-2.0.so.0 in the game downloads"
	echo "so that SDL2 does not need to be installed separately on the player's computer."
	echo "It requires compiling SDL2 with \"./configure --disable-rpath\" and compiling"
	echo "the game client with \"-Wl,-rpath,'\$ORIGIN/lib/x86'\"."
	echo
	echo "This script also simplifies compiling the game client for x86 from x86_64 host."
	echo "================================================================================"
	echo
	echo "DIRECTIONS"
	echo
	echo "1. To build the game clients (and download Spearmint SDL2 builds if needed) run"
	echo "the following commands:"
	echo
	echo "    make clean-release ARCH=x86"
	echo "    ./make-linux-portable.sh x86 -j2"
	echo "    make clean-release ARCH=x86_64"
	echo "    ./make-linux-portable.sh x86_64 -j2"
	echo
	echo "Note: make clean-release ARCH=... is only needed if you previously compiled as"
	echo "non-portable."
	echo
	echo "2. Make the following files are included relative to the game client in the"
	echo "downloads:"
	echo
	echo "    ./lib/x86/libSDL2-2.0.so.0"
	echo "    ./lib/x86_64/libSDL2-2.0.so.0"
	echo
	echo "Note: lib directories will be created in build/release-linux-x86/ and"
	echo "build/release-linux-x86/"
	echo
	echo
	echo "COMPATIBILITY"
	echo
	echo "The game client should run on any GNU/Linux distro as long as the glibc library"
	echo "installed on the computer is new enough. Spearmint releases are built on Debian"
	echo "7 (wheezy) so that the highest possible glibc verion that can be required is"
	echo "2.13. See https://distrowatch.com for which glibc version is included in each"
	echo "GNU/Linux distro/release."
	echo
	echo "The client/server may require a lower glibc version than compiled against. The"
	echo "minimum required glibc version is printed after running this script."
	exit $ERROR
fi

SDL_LIBDIR="code/libs/linux-$ARCH"

SDL_CFLAGS="-Icode/SDL2/include -D_REENTRANT"
SDL_LIBS="-L$SDL_LIBDIR -lSDL2"

# Don't pass arch ($1) to make in $*
shift 1

make ARCH=$ARCH USE_PORTABLE_RPATH=1 SDL_CFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS" $*

# Check if make failed.
if [ $? -ne 0 ] ; then
	exit 2
fi

# Copy SDL lib to where the game client will read it from
if [ ! -d build/release-linux-$ARCH/lib/$ARCH/ ] ; then
	mkdir -p build/release-linux-$ARCH/lib/$ARCH/
fi
if [ ! -f build/release-linux-$ARCH/lib/$ARCH/libSDL2-2.0.so.0 ] || [ $(stat -c %Y build/release-linux-$ARCH/lib/$ARCH/libSDL2-2.0.so.0) != $(stat -c %Y "$SDL_LIBDIR/libSDL2-2.0.so.0") ] ; then
	cp --preserve=timestamps "$SDL_LIBDIR/libSDL2-2.0.so.0" build/release-linux-$ARCH/lib/$ARCH/libSDL2-2.0.so.0
fi

# It's interesting to see why a glibc version is required.
# Another interesting thing is "ldd -v filename" to see what libaries are requires.
VERBOSE=0
if [ $VERBOSE -eq 1 ] ; then
	# Find glibc lines, remove text before GLIBC, then sort by required glibc version
	# "sort -t _ -k 2 -V" is short form of "sort --field-separator='_' --key=2 --version-sort"
	echo
	echo "q3rally-server.$ARCH:"
	objdump -T build/release-linux-$ARCH/q3rally-server.$ARCH | grep GLIBC | sed -e "s/.*GLIBC_/GLIBC_/g" | sort -t _ -k 2 -V
	echo
	echo "q3rally.$ARCH:"
	objdump -T build/release-linux-$ARCH/q3rally.$ARCH | grep GLIBC | sed -e "s/.*GLIBC_/GLIBC_/g" | sort -t _ -k 2 -V
	echo
	echo "renderer_opengl1_$ARCH.so:"
	objdump -T build/release-linux-$ARCH/renderer_opengl1_$ARCH.so | grep GLIBC | sed -e "s/.*GLIBC_/GLIBC_/g" | sort -t _ -k 2 -V
	echo
	echo "renderer_opengl2_$ARCH.so:"
	objdump -T build/release-linux-$ARCH/renderer_opengl2_$ARCH.so | grep GLIBC | sed -e "s/.*GLIBC_/GLIBC_/g" | sort -t _ -k 2 -V
	echo
	echo "lib/$ARCH/libSDL2-2.0.so.0:"
	objdump -T build/release-linux-$ARCH/lib/$ARCH/libSDL2-2.0.so.0 | grep GLIBC | sed -e "s/.*GLIBC_/GLIBC_/g" | sort -t _ -k 2 -V
fi

# Find glibc lines, remove text before and including GLIBC_, remove trailing function name, sort by required glibc version, then grab the last (highest) version
# "sort -uV" is short form of "sort --unique --version-sort"
GLIBC_SERVER=$(objdump -T build/release-linux-$ARCH/q3rally-server.$ARCH | grep GLIBC | sed -e "s/.*GLIBC_//g" | cut -d' ' -f1 | sort -uV | tail -1)
GLIBC_C1=$(objdump -T build/release-linux-$ARCH/q3rally.$ARCH | grep GLIBC | sed -e "s/.*GLIBC_//g" | cut -d' ' -f1 | sort -uV | tail -1)
GLIBC_C2=$(objdump -T build/release-linux-$ARCH/renderer_opengl1_$ARCH.so | grep GLIBC | sed -e "s/.*GLIBC_//g" | cut -d' ' -f1 | sort -uV | tail -1)
GLIBC_C3=$(objdump -T build/release-linux-$ARCH/renderer_opengl2_$ARCH.so | grep GLIBC | sed -e "s/.*GLIBC_//g" | cut -d' ' -f1 | sort -uV | tail -1)
GLIBC_C4=$(objdump -T build/release-linux-$ARCH/lib/$ARCH/libSDL2-2.0.so.0 | grep GLIBC | sed -e "s/.*GLIBC_//g" | cut -d' ' -f1 | sort -uV | tail -1)
GLIBC_CLIENT=$(printf "$GLIBC_C1\n$GLIBC_C2\n$GLIBC_C3\n$GLIBC_C4\n" | sort -uV | tail -1)

echo
echo "Information you can add to your download page:"
echo "    The GNU/Linux $ARCH client requires glibc '$GLIBC_CLIENT' or later."
echo "    The GNU/Linux $ARCH server requires glibc '$GLIBC_SERVER' or later."

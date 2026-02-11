#!/bin/bash
START=$(date +%s)

SVNROOT=$(cd "$(dirname "$(readlink "$BASH_SOURCE")")" && pwd)
FTECONFIG=$SVNROOT/build.cfg

HOME=`echo ~`
BASE=$SVNROOT/..
#set this if you want non-default branding, for customised builds.
#export BRANDING=wastes

#defaults, if we're not set up properly.
#should be overriden in build.cfg
BUILDFOLDER=$HOME/htdocs
BUILDLOGFOLDER=$BUILDFOLDER/build_logs
SVNROOT=$BASE/fteqw-code
BUILD_LINUXx86=y
BUILD_LINUXx64=y
BUILD_WIN32=y
BUILD_WIN64=y
BUILD_ANDROID=y
BUILD_WEB=y
PLUGINS_LINUXx86="qi ezhud xmpp irc hl2"
PLUGINS_LINUXx64="qi ezhud xmpp irc hl2"
PLUGINS_LINUXx32="qi ezhud xmpp irc hl2"
PLUGINS_WIN32="ffmpeg ode qi ezhud xmpp irc hl2"
PLUGINS_WIN64="ffmpeg ode qi ezhud xmpp irc hl2"
THREADS="-j 4"

TARGETS_LINUX="qcc-rel rel dbg vk-rel plugins-rel plugins-dbg"
TARGETS_WINDOWS="sv-rel m-rel qcc-rel qccgui-scintilla qccgui-dbg m-dbg sv-dbg plugins-dbg plugins-rel"
TARGETS_WEB="gl-rel"


if [ -e $FTECONFIG ]; then
	. $FTECONFIG
else
	echo "WARNING: $FTECONFIG does not exist yet."
fi

if [ "$BUILD_CLEAN" == "n" ]; then
	NOUPDATE="y"
fi

#check args (and override config as desired)
while [[ $# -gt 0 ]]
do
	case $1 in
	-r)
		SVN_REV_ARG="-r $2"
		NOUPDATE=
		shift
		;;
	-j)
		THREADS="-j $2"
		shift
		;;
	-help|--help)
		echo "  -r VER       Specifies the SVN revision to update to"
		echo "  -j THREADS   Specifies how many jobs to make with"
		echo "  --help       This text"
		echo "  --noupdate   Don't do svn updates"
		echo "  --unclean    Don't do make clean, for faster rebuilds"
		echo "  --web        Build web target (excluding all others)"
		echo "  --droid      Build android target (excluding others)"
		exit 0
		;;
	-build|--build)
		TARGET="FTE_CONFIG=$2"
		shift
		;;
	--noupdate)
		NOUPDATE="y"
		;;
	--unclean)
		BUILD_CLEAN="n"
		;;
	--web)
		BUILD_LINUXx86="n"
		BUILD_LINUXx64="n"
		BUILD_LINUXarmhf="n"
		BUILD_LINUXaarch64="n"
		BUILD_WIN32="n"
		BUILD_WIN64="n"
		BUILD_ANDROID="n"
		BUILD_WEB="y"
		;;
	--droid)
		BUILD_LINUXx86="n"
		BUILD_LINUXx64="n"
		BUILD_LINUXarmhf="n"
		BUILD_LINUXaarch64="n"
		BUILD_WIN32="n"
		BUILD_WIN64="n"
		BUILD_ANDROID="y"
		BUILD_WEB="n"
		;;
	*)
		echo "Unknown option $1"
		;;
	esac
	shift
done

MAKEARGS="$THREADS $TARGET"


########### Emscripten / Web Stuff
export EMSDK=$EMSCRIPTENROOT
#export WEB_PREJS="--pre-js $HOME/prejs.js"

########### Android Stuff. so messy...
#This is some android password that you should keep private. You should keep the keystore file private too, of course. Frankly, that part is more important than this small random number.
KEYPASSFILE=$BASE/.fte_keypass
if [ ! -e $KEYPASSFILE ]; then
	dd if=/dev/urandom count=9 bs=1 2>/dev/null | base64 > $KEYPASSFILE
	chmod 400 $KEYPASSFILE
fi
KEYPASS=`cat $KEYPASSFILE`
export JAVA_HOME=/usr
if [ ! -z "$ANDROIDROOT" ]; then 
	export ANDROID_HOME=$ANDROIDROOT
fi
if [ ! -z "$ANDROIDNDKROOT" ]; then 
	export ANDROID_NDK_ROOT=$ANDROIDNDKROOT
else
	export ANDROID_NDK_ROOT=$ANDROID_HOME/ndk-bundle
fi
export KEYTOOLARGS="-keypass $KEYPASS -storepass $KEYPASS -dname \"CN=fteqw.com, OU=ID, O=FTE, L=Unknown, S=Unknown, C=GB\""
export JARSIGNARGS="-storepass $KEYPASS"

########### Various Output etc Paths
QCCBUILDFOLDER=$BUILDFOLDER/fteqcc
SVNFOLDER=$SVNROOT/engine/release
ARCHIVEFOLDER=$BUILDFOLDER/archive
SVNDBGFOLDER=$SVNROOT/engine/debug
WARNINGLEVEL="-w"
FILELOCK=$BASE/.fte_buildlock

#./ccache-alias.sh

exec 9>$FILELOCK
if ! flock -n 9 ; then
	echo "Build script is already running!";
	exit 1
fi

mkdir -p $BUILDLOGFOLDER
if [ ! -d $SVNROOT ]; then
	#just in case...
	svn checkout https://svn.code.sf.net/p/fteqw/code/trunk $SVNROOT $SVN_REV_ARG
fi

cd $SVNROOT/

if [ "$NOUPDATE" != "y" ]; then
	echo "SVN Update"
	svn update $SVN_REV_ARG
fi

cd engine

date > $BUILDLOGFOLDER/buildlog.txt
echo "Starting build" >> $BUILDLOGFOLDER/buildlog.txt

function build {
	BUILDSTART=$(date +%s)
	NAME=$1
	DEST=$2
	shift; shift
	if [ "$BUILD_CLEAN" != "n" ]; then
		make clean >> /dev/null
	fi
	echo -n "Making $NAME... "
	date > $BUILDLOGFOLDER/$DEST.txt
	echo BUILD: $NAME >> $BUILDLOGFOLDER/$DEST.txt
	echo PLUGINS: $NATIVE_PLUGINS >> $BUILDLOGFOLDER/$DEST.txt
	echo make $MAKEARGS $* >> $BUILDLOGFOLDER/$DEST.txt 2>&1
	make $MAKEARGS $* >> $BUILDLOGFOLDER/$DEST.txt 2>&1
	if [ $? -eq 0 ]; then
		BUILDEND=$(date +%s)
		BUILDTIME=$(( $BUILDEND - $BUILDSTART ))
		echo "$BUILDTIME seconds"
		echo "$NAME done, took $BUILDTIME seconds" >> $BUILDLOGFOLDER/buildlog.txt
		rm -rf $BUILDFOLDER/$DEST >> /dev/null 2>&1
		mkdir $BUILDFOLDER/$DEST 2>> /dev/null
		mkdir $BUILDFOLDER/$DEST/debug 2>> /dev/null
		cp $SVNFOLDER/* $BUILDFOLDER/$DEST >> /dev/null 2>> /dev/null
		cp $SVNDBGFOLDER/* $BUILDFOLDER/$DEST/debug >> /dev/null 2>> /dev/null
		rm -rf $BUILDFOLDER/$DEST/*.a >> /dev/null 2>&1
		rm -rf $BUILDFOLDER/$DEST/debug/*.a >> /dev/null 2>&1
		rmdir $BUILDFOLDER/$DEST/debug 2>> /dev/null
	else
		echo "$NAME failed" >> $BUILDLOGFOLDER/buildlog.txt
		echo "failed"
	fi
}

function build_fteqcc {
	echo "--- no code ---"
}

echo "--- Engine builds ---"
#the -fno-finite-math-only is to avoid a glibc dependancy
if [ "$BUILD_LINUXx86" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_LINUXx86" build "Linux 32-bit" linux_x86 FTE_TARGET=linux32 CPUOPTIMIZATIONS=-fno-finite-math-only $TARGETS_LINUX
fi
if [ "$BUILD_LINUXx64" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_LINUXx64" build "Linux 64-bit" linux_amd64 FTE_TARGET=linux64 CPUOPTIMIZATIONS=-fno-finite-math-only $TARGETS_LINUX
fi
if [ "$BUILD_LINUXx32" != "n" ]; then
# 	CFLAGS="-DNO_JPEG"
	NATIVE_PLUGINS="$PLUGINS_LINUXx32" build "Linux x32" linux_x32 FTE_TARGET=linux_x32 CPUOPTIMIZATIONS=-fno-finite-math-only $TARGETS_LINUX
fi
if [ "$BUILD_LINUXarmhf" != "n" ]; then
	#debian/ubuntu's armhf targets armv7. we instead target armv6, because that means we work on rpi too (but still with hard-float). It should be compatible although we likely need more ops.
	NATIVE_PLUGINS="$PLUGINS_LINUXarmhf" build "Linux ARMhf" linux_armhf FTE_TARGET=linux_armhf CPUOPTIMIZATIONS=-fno-finite-math-only $TARGETS_LINUX
fi
if [ "$BUILD_LINUXaarch64" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_LINUXaarch64" build "Linux aarch64" linux_aarch64 FTE_TARGET=linux_aarch64 CPUOPTIMIZATIONS=-fno-finite-math-only $TARGETS_LINUX
fi
if [ "$BUILD_CYGWIN" != "n" ]; then
	NATIVE_PLUGINS="qi ezhud" build "Cygwin" cygwin qcc-rel rel dbg plugins-rel plugins-dbg
fi
if [ "$BUILD_WIN32" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_WIN32" build "Windows 32-bit" win32 FTE_TARGET=win32 CFLAGS="$WARNINGLEVEL" $TARGETS_WINDOWS
fi
if [ "$BUILD_WIN64" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_WIN64" build "Windows 64-bit" win64 FTE_TARGET=win64 CFLAGS="$WARNINGLEVEL" $TARGETS_WINDOWS
fi
if [ "$BUILD_MSVC" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_WIN32" build "Windows MSVC 32-bit" msvc FTE_TARGET=vc BITS=32 CFLAGS="$WARNINGLEVEL" sv-rel gl-rel vk-rel mingl-rel m-rel d3d-rel qcc-rel qccgui-scintilla qccgui-dbg gl-dbg sv-dbg plugins-dbg plugins-rel
	NATIVE_PLUGINS="$PLUGINS_WIN64" build "Windows MSVC 64-bit" msvc FTE_TARGET=vc BITS=64 CFLAGS="$WARNINGLEVEL" sv-rel gl-rel vk-rel mingl-rel m-rel d3d-rel qcc-rel qccgui-scintilla qccgui-dbg gl-dbg sv-dbg plugins-dbg plugins-rel
fi
export NATIVE_PLUGINS="qi ezhud xmpp irc"
if [ "$BUILD_ANDROID" != "n" ]; then
	NATIVE_PLUGINS="$PLUGINS_DROID" build "Android" android droid-rel
fi
if [ "$BUILD_DOS" == "y" ]; then
	#no networking makes dedicated servers useless. and only a crappy sw renderer is implemented right now.
	#the qcc might be useful to someone though!
	build "DOS" dos m-rel qcc-rel
fi
if [ "$BUILD_WEB" != "n" ]; then
	source $EMSDK/emsdk_env.sh >> /dev/null
	LTO= build "Emscripten" web FTE_TARGET=web $TARGETS_WEB CC=emcc
fi
if [ "$BUILD_SDL_LINUXx86" == "y" ]; then
	build "Linux 32-bit (SDL)" linux_x86_sdl FTE_TARGET=SDL2 BITS=32 $TARGETS_SDL
fi
if [ "$BUILD_SDL_LINUXx64" == "y" ]; then
	build "Linux 64-bit (SDL)" linux_amd64_sdl FTE_TARGET=SDL2 BITS=64 $TARGETS_SDL
fi
if [ "$BUILD_SDL_WIN32" == "y" ]; then
	build "Windows 32-bit (SDL)" win32_sdl FTE_TARGET=win32_SDL $TARGETS_SDL
#	CFLAGS="$WARNINGLEVEL -DNOLEGACY -DOMIT_QCC" build "Windows 32-bit nocompat" nocompat FTE_TARGET=win32 LTO=1 NOCOMPAT=1 BOTLIB_CFLAGS="" BOTLIB_OBJS="" $TARGETS_SDL
fi
if [ "$BUILD_SDL_WIN64" == "y" ]; then
	build "Windows 64-bit (SDL)" win64_sdl FTE_TARGET=win64_SDL $TARGETS_SDL
fi
####build "MorphOS" morphos CFLAGS="-I$BASE/morphos/os-include/ -I$BASE/morphos/lib/ -L$BASE/morphos/lib/ -I$BASE/zlib/zlib-1.2.5 -L$BASE/zlib/zlib-1.2.5 -I./libs $WARNINGLEVEL" gl-rel mingl-rel sv-rel qcc-rel
if [ "$BUILD_MAC" != "n" ]; then
	#build "MacOSX" macosx_tiger CFLAGS="-I$BASE/mac/x86/include/ -L$BASE/mac/x86/lib -I./libs" FTE_TARGET=macosx_x86 sv-rel gl-rel mingl-rel qcc-rel
	#FIXME: figure out how to do universal binaries or whatever they're called
	build "MacOSX 32-bit" osx32 CC=o32-clang CXX=o32-clang++ FTE_TARGET=osx_x86 BITS=32 sv-rel gl-rel mingl-rel qcc-rel
	build "MacOSX 64-bit" osx64 CC=o64-clang CXX=o64-clang++ FTE_TARGET=osx_x86_64 BITS=64 sv-rel gl-rel mingl-rel qcc-rel
fi


#third party stuff / misc crap
if [ "$BUILD_WEB" != "n" ]; then
	cp $BASE/3rdparty/web/* $BUILDFOLDER/web/
fi
if [ "$BUILD_WIN32" != "n" ]; then
	if [ -e "$BASE/3rdparty/win32/3rdparty.zip" ]; then
		cp $BASE/3rdparty/win32/3rdparty.zip $BUILDFOLDER/win32/3rdparty.zip
	else
		rm -f $BUILDFOLDER/win32/3rdparty.zip
	fi
#	if [ "$BUILD_SDL_WIN32" != "n" ]; then
#		cp $SVNROOT/engine/libs/SDL2-2.0.1/i686-w64-mingw32/bin/SDL2.dll $BUILDFOLDER/win32_sdl
#	fi
fi
if [ "$BUILD_WIN64" != "n" ]; then
	if [ -e "$BASE/3rdparty/win64/3rdparty.zip" ]; then
		cp $BASE/3rdparty/win64/3rdparty.zip $BUILDFOLDER/win64/3rdparty.zip
	else
		rm -f $BUILDFOLDER/win64/3rdparty.zip
	fi
#	if [ "$BUILD_SDL_WIN64" != "n" ]; then
#		cp $SVNROOT/engine/libs/SDL2-2.0.1/x86_64-w64-mingw32/bin/SDL2.dll $BUILDFOLDER/win64_sdl
#	fi
fi
if [ -e "$HOME/nocompat_readme.html" ]; then
	cp $HOME/nocompat_readme.html $BUILDFOLDER/nocompat/README.html
fi


#call out to build_qc.sh to invoke native builds as appropriate.
case "$(uname -m)" in
x86_64)
	if [ "$BUILD_LINUXx64" != "n" ]; then
		rm -rf $QCCBUILDFOLDER 2>&1
		mkdir -p $QCCBUILDFOLDER
		cd $SVNROOT/
		FTEQCC=$BUILDFOLDER/linux_amd64/fteqcc64 FTEQW=$BUILDFOLDER/linux_amd64/fteqw64 QSS=$BUILDFOLDER/qss/quakespasm-spiked-linux64 ./build_qc.sh
	fi
	;;
i386 | i486 | i586)
	if [ "$BUILD_LINUXx86" != "n" ]; then
		rm -rf $QCCBUILDFOLDER 2>&1
		mkdir -p $QCCBUILDFOLDER
		cd $SVNROOT/
		FTEQCC=$BUILDFOLDER/linux_x86/fteqcc32 FTEQW=$BUILDFOLDER/linux_x86/fteqw32 QSS= ./build_qc.sh
	fi
	;;
esac

cd $SVNROOT/engine/
svn info > $BUILDFOLDER/version.txt

if [ "$BUILD_LINUXx86" != "n" ]; then
	cp $BUILDFOLDER/linux_x86/fteqcc32 $QCCBUILDFOLDER/linux32-fteqcc
fi
if [ "$BUILD_LINUXx64" != "n" ]; then
	cp $BUILDFOLDER/linux_amd64/fteqcc64 $QCCBUILDFOLDER/linux64-fteqcc
fi
if [ "$BUILD_LINUXx32" != "n" ]; then
	cp $BUILDFOLDER/linux_x32/fteqccx32 $QCCBUILDFOLDER/linuxx32-fteqcc
fi
if [ "$BUILD_LINUXarmhf" != "n" ]; then
	cp $BUILDFOLDER/linux_armhf/fteqccarmhf $QCCBUILDFOLDER/linuxarmhf-fteqcc
fi
if [ "$BUILD_LINUXaarch64" != "n" ]; then
	cp $BUILDFOLDER/linux_armhf/fteqccaarch64 $QCCBUILDFOLDER/linuxaarch64-fteqcc
fi
if [ "$BUILD_WIN32" != "n" ]; then
	cp $BUILDFOLDER/win32/fteqcc.exe $QCCBUILDFOLDER/win32-fteqcc.exe
	cp $BUILDFOLDER/win32/fteqccgui.exe $QCCBUILDFOLDER/win32-fteqccgui.exe
fi
if [ "$BUILD_WIN64" != "n" ]; then
	cp $BUILDFOLDER/win64/fteqcc64.exe $QCCBUILDFOLDER/win64-fteqcc.exe
	cp $BUILDFOLDER/win64/fteqccgui64.exe $QCCBUILDFOLDER/win64-fteqccgui.exe
fi
#cp $BUILDFOLDER/morphos/fteqcc $QCCBUILDFOLDER/morphos-fteqcc
#cp $BUILDFOLDER/macosx_tiger/fteqcc $QCCBUILDFOLDER/macosx_tiger-fteqcc
cp $BUILDFOLDER/version.txt $QCCBUILDFOLDER/version.txt

if [ "$BUILD_WIN32" != "n" ] && [ "$BUILD_WIN64" != "n" ]; then
	echo Archiving output
	SVNVER=$(svnversion $SVNROOT)
	if [ -e $ARCHIVEFOLDER ]; then
		cd $BUILDFOLDER/
		zip -q -9 $ARCHIVEFOLDER/win_fteqw_$SVNVER.zip win32/fteglqw.exe win32/fteqwsv.exe win32/fteqccgui.exe win32/debug/fteglqw.exe win64/fteqw.exe win64/debug/fteglqw.exe
	fi

	if [ -e $BUILDFOLDER/fteqw_for_windows.zip ]; then
		cd $BUILDFOLDER/win32/
		zip -q -j -9 $BUILDFOLDER/fteqw_for_windows.zip fteglqw.exe fteqwsv.exe fteqccgui.exe fteplug_qi_x86.dll fteplug_xmpp_x86.dll fteplug_irc_x86.dll fteplug_ezhud_x86.dll
		cd $HOME/3rdparty_win32/
		zip -q -9 $BUILDFOLDER/fteqw_for_windows.zip ogg.dll vorbis.dll vorbisfile.dll freetype6.dll zlib1.dll
		mkdir -p $BASE/tmp/fte
		cd $BASE/tmp/
		cp $BUILDFOLDER/csaddon/menu.dat fte
		zip -q -9 $BUILDFOLDER/fteqw_for_windows.zip fte/menu.dat
	fi

	#~/afterquake/updatemini.sh
fi

echo "All done"

END=$(date +%s)
DIFF=$(( $END - $START ))
MINS=$(( $DIFF / 60 ))
echo "Total Compile Time: $MINS minutes" >> $BUILDLOGFOLDER/buildlog.txt
echo "Total Compile Time: $MINS minutes"

cd $HOME
#./errorlog.sh
#cd $HOME
#rm .bitchxrc
#cp ./fteqw/.bitchxrc ./
#./BitchX -a irc.quakenet.org -A -c "#fte" -n A_Gorilla

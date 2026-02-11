#FTEQW Makefile

#make m-rel plugins-rel sv-rel [FTE_TARGET=$ARCH]

#only limited forms of cross-making is supported
#only the following 3 are supported
#linux->win32 (FTE_TARGET=win32) RPM Package: "mingw32-gcc", DEB Package: "mingw32"
#linux->win64 (FTE_TARGET=win64) RPM Package: "mingw32-gcc", DEB Package: "mingw32"
#linux->win32 (FTE_TARGET=msvc32 MSVCPATH=? WINDOWSSDKDIR=?) invokes via wine.
#linux->win64 (FTE_TARGET=msvc64 MSVCPATH=? WINDOWSSDKDIR=?) invokes via wine.
#linux->linux 32 (FTE_TARGET=linux32)
#linux->linux 64 (FTE_TARGET=linux64)
#linux->linux x32 (FTE_TARGET=linux_x32)
#linux->linux armhf (FTE_TARGET=linux_armhf)
#linux->linux arm64/aarch64 (FTE_TARGET=linux_arm64)
#linux->linux *others* (FTE_TARGET=linux CC=other-gcc)
#linux->morphos (FTE_TARGET=morphos)
#linux->macosx (FTE_TARGET=macosx) or (FTE_TARGET=macosx_x86)
#linux->javascript (FTE_TARGET=web)
#linux->droid (make droid)
#win32->droid (make droid)
#if you are cross compiling, you'll need to use FTE_TARGET=mytarget
#note: cross compiling will typically require 'make makelibs FTE_TARGET=mytarget', which avoids installing lots of extra system packages.

#
CC?=gcc
WINDRES?=windres
STRIP?=strip

STRIPFLAGS?=--strip-unneeded --remove-section=.comment

VISIBILITY_FLAGS=-fvisibility=hidden	#just assume this for sanities sake. disable only for compilers that error.
CPUOPTIMIZATIONS=-Os

COMPILE_SYS:=$(shell uname -o 2>&1)

#canonicalize the source path. except emscripten warns about that like crazy. *sigh*
ifeq ($(FTE_TARGET),web)
    BASE_DIR:=.
else ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
    BASE_DIR:=.
else ifeq ($(FTE_TARGET),droid)
	#android tools suck, but plugins need to find the engine directory.
    BASE_DIR:=../engine
else
    BASE_DIR:=$(realpath .)
endif

ifeq ($(SVNREVISION),)
	#try subversion firstly...
#    SVN_VERSION:=$(shell test -d $(BASE_DIR)/../.svn && svnversion $(BASE_DIR))
#    SVN_DATE:=$(shell test -d $(BASE_DIR)/../.svn && cd $(BASE_DIR) && svn info --show-item last-changed-date --no-newline)
#    ifeq (,$(SVN_VERSION))
#        #grab the svn version from git-svn (assuming no other modifications). this fails when there's extra commits (probably a good thing).
#        SVN_VERSION=$(shell test -d $(BASE_DIR)/../.git && git svn find-rev `git rev-parse HEAD`)
#        SVN_DATE:=$(shell test -d $(BASE_DIR)/../.git && git log -1 --format=%cs $(BASE_DIR))
#    endif
    ifeq (,$(SVN_VERSION))
        #try to get git version info instead. this usually uses git-count-hash[-dirty] format
        SVN_VERSION:=$(shell test -d $(BASE_DIR)/../.git && cd $(BASE_DIR) && echo $$((`git rev-list HEAD --count` + 29)))
        GIT_VERSION:=$(shell test -d $(BASE_DIR)/../.git && cd $(BASE_DIR) && git describe --long --always --dirty)
        SVN_DATE:=$(shell test -d $(BASE_DIR)/../.git && git log -1 --format=%cs $(BASE_DIR))
        ifneq (,$(SVN_VERSION))
            #make sure its prefixed with something specific. we use versions for versioning, which will confuse the update mechanism if they're inconsistent - like random hashses that have no implied ordering...
            SVN_VERSION:=git-$(SVN_VERSION)-$(GIT_VERSION)
        endif
    endif

    SVNREVISION=
    ifneq (,$(SVN_VERSION))
        SVNREVISION+=-DSVNREVISION=$(SVN_VERSION)
    endif
ifneq (M,$(findstring M,$(SVN_VERSION)))
    SVNREVISION+=-DSVNDATE=$(SVN_DATE)
endif
endif
MAKE:=$(MAKE) --no-print-directory SVNREVISION="$(SVNREVISION)" SVN_VERSION="$(SVN_VERSION)"

#WHOAMI:=$(shell whoami)


#update these to download+build a different version. this assumes that the url+subdirs etc contain a consistant version everywhere.
JPEGVER=9c
ZLIBVER=1.3.1
PNGVER=1.6.40
OGGVER=1.3.4
VORBISVER=1.3.7
VULKANVER=1.3.275.0
SDL2VER=2.30.4
SCINTILLAVER=373
OPUSVER=1.3.1
SPEEXVER=1.2.0
SPEEXDSPVER=1.2.0
FREETYPEVER=2.10.1
BULLETVER=2.87
OPENSSLVER=3.0.1

#cygwin's make's paths confuses non-cygwin things
RELEASE_DIR=$(BASE_DIR)/release
DEBUG_DIR=$(BASE_DIR)/debug
PROFILE_DIR=$(BASE_DIR)/profile
NATIVE_ABSBASE_DIR:=$(realpath $(BASE_DIR))
ifeq ($(COMPILE_SYS),Cygwin)
    OUT_DIR?=.
    NATIVE_OUT_DIR:=$(shell cygpath -m $(OUT_DIR))
    NATIVE_BASE_DIR:=$(shell cygpath -m $(BASE_DIR))
    NATIVE_RELEASE_DIR:=$(shell cygpath -m $(RELEASE_DIR))
    NATIVE_DEBUG_DIR:=$(shell cygpath -m $(DEBUG_DIR))
    NATIVE_ABSBASE_DIR:=$(shell cygpath -m $(NATIVE_ABSBASE_DIR))
endif
NATIVE_OUT_DIR?=$(OUT_DIR)
NATIVE_BASE_DIR?=$(BASE_DIR)
NATIVE_RELEASE_DIR?=$(RELEASE_DIR)
NATIVE_DEBUG_DIR?=$(DEBUG_DIR)

EXE_NAME=fteqw

#include the appropriate games.
ifneq (,$(BRANDING))
    BRANDFLAGS+=-DBRANDING_INC=../game_$(BRANDING).h
    -include game_$(BRANDING).mak
endif
FTE_CONFIG?=fteqw
ifneq ($(findstring msvc,$(FTE_TARGET)),msvc)
ifeq (,$(FTE_CONFIG_EXTRA))
    FTE_CONFIG_EXTRA := $(shell $(CC) -xc -E -P -DFTE_TARGET_$(shell echo $(FTE_TARGET) | tr '[:lower:]' '[:upper:]') -DCOMPILE_OPTS $(CFLAGS) common/config_$(FTE_CONFIG).h)
endif
endif
BRANDFLAGS+=-DCONFIG_FILE_NAME=config_$(FTE_CONFIG).h $(FTE_CONFIG_EXTRA)
EXE_NAME=$(FTE_CONFIG)
ifeq (,$(findstring DNO_SPEEX,$(FTE_CONFIG_EXTRA)))
    USE_SPEEX?=1
endif
ifeq (,$(findstring DNO_OPUS,$(FTE_CONFIG_EXTRA)))
    USE_OPUS=1
endif
ifneq (,$(findstring DLINK_QUAKE3,$(FTE_CONFIG_EXTRA)))
    LINK_QUAKE3=1
endif
ifeq ($(findstring web,$(FTE_TARGET)),)	#the web target uses javascript/browser loaders for common audio+image files instead of needing to embed our own, which keeps sizes down slightly.
ifeq (,$(findstring DNO_VORBISFILE,$(FTE_CONFIG_EXTRA)))
    USE_VORBISFILE=1
endif
ifneq (,$(findstring DLINK_JPEG,$(FTE_CONFIG_EXTRA)))
    LINK_JPEG=1
endif
ifneq (,$(findstring DLINK_PNG,$(FTE_CONFIG_EXTRA)))
    LINK_ZLIB=1
    LINK_PNG=1
endif
endif
ifneq (,$(findstring DLINK_FREETYPE,$(FTE_CONFIG_EXTRA)))
    LINK_FREETYPE=1
    LINK_ZLIB=1
    LINK_PNG=1
endif
ifneq (,$(findstring -Os,$(FTE_CONFIG_EXTRA)))
    CPUOPTIMIZATIONS+=-Os
    BRANDFLAGS:=$(filter-out -O%,$(BRANDFLAGS))
endif
ifneq (,$(findstring DLINK_ODE,$(FTE_CONFIG_EXTRA)))
	#ode library will be statically linked.
    LINK_ODE=1
endif
ifneq (,$(findstring DLINK_INTERNAL_BULLET,$(FTE_CONFIG_EXTRA)))
	#bullet plugin will be built into the exe itself
    INTERNAL_BULLET=1
endif
ifneq (,$(findstring DLINK_EZHUD,$(FTE_CONFIG_EXTRA)))
    LINK_EZHUD=1
endif
ifneq (,$(findstring DLINK_OPENSSL,$(FTE_CONFIG_EXTRA)))
    LINK_OPENSSL=1
endif

ifeq ($(BITS),64)
    CC:=$(CC) -m64
    CXX:=$(CXX) -m64
endif
ifeq ($(BITS),32)
    CC:=$(CC) -m32
    CXX:=$(CXX) -m32
endif

#correct the gcc build when cross compiling
ifneq (,$(findstring win32,$(FTE_TARGET)))
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		#old/original mingw project, headers are not very up to date.
		ifneq ($(shell which i586-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=i586-mingw32msvc-gcc
			CXX=i586-mingw32msvc-g++
			AR=i586-mingw32msvc-ar
			WINDRES=i586-mingw32msvc-windres
			STRIP=i586-mingw32msvc-strip
#			BITS?=32
		endif
		#mingw64 provides a 32bit toolchain too, which has more up to date header files than the mingw32 project. so favour that if its installed.
		ifneq ($(shell which i686-w64-mingw32-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=i686-w64-mingw32-gcc
			CXX=i686-w64-mingw32-g++
			AR=i686-w64-mingw32-ar
			WINDRES=i686-w64-mingw32-windres
			STRIP=i686-w64-mingw32-strip
#			BITS?=32
		endif
	endif
endif

#correct the gcc build when cross compiling
ifneq (,$(findstring win64,$(FTE_TARGET)))
	ifeq ($(shell $(CC) -v 2>&1 | grep mingw),)
		#CC didn't state that it was mingw... so try fixing that up
		ifneq ($(shell which x86_64-w64-mingw32-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=x86_64-w64-mingw32-gcc -m64
			CXX=x86_64-w64-mingw32-g++ -m64
			AR=x86_64-w64-mingw32-ar
			WINDRES=x86_64-w64-mingw32-windres
			STRIP=x86_64-w64-mingw32-strip
#			BITS=64
		endif
		ifneq ($(shell which amd64-mingw32msvc-gcc 2> /dev/null),)
			#yup, the alternative exists (this matches the one debian has)
			CC=amd64-mingw32msvc-gcc -m64
			CXX=amd64-mingw32msvc-g++ -m64
			AR=amd64-mingw32msvc-ar
			WINDRES=amd64-mingw32msvc-windres
			STRIP=amd64-mingw32msvc-strip
#			BITS=64
		endif
	endif
endif

ifeq ($(FTE_TARGET),win32_sdl)
    FTE_TARGET=win32_SDL
endif

USER_TARGET:=$(FTE_TARGET)

#make droid-rel doesn't get the right stuff
#add a small default config file. its only small. and some other stuff, because we can. This makes it much easier to get it up and running.
DROID_PACKSU?= $(BASE_DIR)/droid/fte.cfg $(BASE_DIR)/droid/default.fmf $(BASE_DIR)/droid/configs/touch.cfg
ANDROID_HOME?=~/android-sdk-linux
#ANDROID_NDK_ROOT?=~/android-ndk-r8e
#ANDROID_NDK_ROOT?=$(ANDROID_HOME)/ndk-bundle
ANDROID_NDK_ROOT=$(ANDROID_HOME)/android-ndk-r14b
ANDROID_ZIPALIGN?=$(ZIPALIGN)
ANDROID_ZIPALIGN?=$(ANDROID_HOME)/tools/zipalign
ANT?=ant
JAVA_HOME?=/usr
JAVATOOL=$(JAVA_HOME)/bin/
ANDROID_SCRIPT=android
DO_CMAKE=cmake -DCMAKE_C_COMPILER="$(firstword $(CC))" -DCMAKE_C_FLAGS="$(wordlist 2,99,$(CC)) $(CPUOPTIMIZATIONS)" -DCMAKE_CXX_COMPILER="$(firstword $(CXX))" -DCMAKE_CXX_FLAGS="$(wordlist 2,99,$(CXX)) $(CPUOPTIMIZATIONS)" 

ifeq ($(DROID_ARCH),)
	#armeabi armeabi-v7a arm64-v8a x86 x86_64 mips mips64
	DROID_ARCH+=armeabi-v7a	#old 32bit android. yucky.
	DROID_ARCH+=arm64-v8a	#modern android devices are 64bit-only. urgh.
	DROID_ARCH+=x86			#mostly for testing...
	#DROID_ARCH+=x86_64	#starting with DROID_API_LEVEL 21
endif
ifeq ($(FTE_TARGET),droid)
	#figure out the host system, required to find a usable compiler
	ifneq ($(shell uname -o 2>&1 | grep Cygwin),)
#		ifeq ($(shell uname -m 2>&1), i686)
#			ANDROID_HOSTSYSTEM?=windows
#		else
#			ANDROID_HOSTSYSTEM?=windows-$(shell uname -m)
#		endif
		ANDROID_HOSTSYSTEM?=windows-x86_64
	else
		ANDROID_HOSTSYSTEM?=linux-$(shell uname -m)
	endif
	DROID_ABI_VER?=4.9

	#omfg why the FUCK do we need all this bullshit? Why isn't there some sane way to do this that actually works regardless of ndk updates?!?
	#name is some random subdir that someone at google arbitrarily picked
	#arch is some random other name for a group of ABIs...
	#prefix is the 'standard' tupple that the toolchain was compiled to target (by default)
	#ver is whatever gcc version it is, or clang. so yeah, pretty much random.
	#cflags is whatever is needed to actually target that abi properly with the specific toolchain... -m64 etc.
	DROID_ABI_NAME___armeabi=arm-linux-androideabi
	DROID_ABI_PREFIX_armeabi=arm-linux-androideabi
	DROID_ABI_ARCH___armeabi=arm
	DROID_ABI_VER____armeabi?=$(DROID_ABI_VER)
	DROID_ABI_CFLAGS_armeabi=-march=armv5te -mtune=xscale -msoft-float
	DROID_ABI_NAME___armeabi-v7a=$(DROID_ABI_NAME___armeabi)
	DROID_ABI_PREFIX_armeabi-v7a=$(DROID_ABI_PREFIX_armeabi)
	DROID_ABI_ARCH___armeabi-v7a=$(DROID_ABI_ARCH___armeabi)
	DROID_ABI_VER____armeabi-v7a=$(DROID_ABI_VER____armeabi)
	DROID_ABI_CFLAGS_armeabi-v7a=-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16
	DROID_ABI_NAME___arm64-v8a=$(DROID_ABI_NAME___armeabi)
	DROID_ABI_PREFIX_arm64-v8a=$(DROID_ABI_PREFIX_armeabi)
	DROID_ABI_ARCH___arm64-v8a=$(DROID_ABI_ARCH___armeabi)
	DROID_ABI_VER____arm64-v8a=$(DROID_ABI_VER____armeabi)
	DROID_ABI_CFLAGS_arm64-v8a=-m64
	DROID_ABI_NAME___x86=x86
	DROID_ABI_PREFIX_x86=i686-linux-android
	DROID_ABI_ARCH___x86=x86
	DROID_ABI_VER____x86?=$(DROID_ABI_VER)
	DROID_ABI_CFLAGS_x86=-march=i686 -mssse3 -mfpmath=sse -m32 -Os
	DROID_ABI_NAME___x86_64=x86_64
	DROID_ABI_PREFIX_x86_64=x86_64-linux-android
	DROID_ABI_ARCH___x86_64=x86_64
	DROID_ABI_VER____x86_64=$(DROID_ABI_VER____x86)
	DROID_ABI_CFLAGS_x86_64=-march=x86-64 -msse4.2 -mpopcnt -m64 -Os
	#DROID_ABI_NAME___mips=mipsel-linux-android
	#DROID_ABI_PREFIX_mips=mipsel-linux-android
	#DROID_ABI_ARCH___mips=mips
	#DROID_ABI_VER____mips?=$(DROID_ABI_VER)
	#DROID_ABI_CFLAGS_mips=
	#DROID_ABI_NAME___mips64=$(DROID_ABI_NAME___mips)
	#DROID_ABI_PREFIX_mips64=$(DROID_ABI_PREFIX_mips)
	#DROID_ABI_ARCH___mips64=$(DROID_ABI_ARCH___mips)
	#DROID_ABI_VER____mips64=$(DROID_ABI_VER____mips)
	#DROID_ABI_CFLAGS_mips64=-m64

	ifeq (1,$(words [$(DROID_ARCH)]))
		#try and make sense of the above nonsense.
		DROID_ABI:=$(DROID_ABI_CFLAGS_$(DROID_ARCH))
		TOOLCHAINPATH:=$(ANDROID_NDK_ROOT)/toolchains/$(DROID_ABI_NAME___$(DROID_ARCH))-$(DROID_ABI_VER____$(DROID_ARCH))/prebuilt/$(ANDROID_HOSTSYSTEM)/bin/
		TOOLCHAIN:=$(TOOLCHAINPATH)$(DROID_ABI_PREFIX_$(DROID_ARCH))-

		#4 is the min that fte requires
		DROID_API_LEVEL?=9
		ifeq ($(DROID_ARCH),x86)
			#google fecked up. anything before api_level 9 will fail to compile on x86
			DROID_API_LEVEL=9
		endif
		ifeq ($(DROID_ARCH),x86_64)
			#google fecked up. anything before api_level 9 will fail to compile on x86
			DROID_API_LEVEL=21
		endif
		DROID_API_NAME?=android-$(DROID_API_LEVEL)
		DROID_PLAT_INC=arch-$(DROID_ABI_ARCH___$(DROID_ARCH))
		DROIDSYSROOT=$(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC))
		ifeq ($(DROIDSYSROOT),)	#its possible that google removed whatever api we're trying to target, just switch up to the new default.
			BITCHANDMOAN:=$(shell echo targetting \"android-9\" instead of \"android-$(DROID_API_LEVEL)\" 1>&2)
			DROID_API_LEVEL=9
			DROID_API_NAME=android-$(DROID_API_LEVEL)

			DROIDSYSROOT=$(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC))
			ifeq ($(DROIDSYSROOT),)	#its possible that google removed whatever api we're trying to target, just switch up to the new default.
				BITCHANDMOAN:=$(shell echo $(DROID_API_NAME) not available either - $(DROID_ARCH) 1>&2)
			endif
		endif
		DROIDSYSROOT:=$(DROIDSYSROOT)
	endif

	#if we're running under windows, then we want to run some other binary
	ifeq ($(shell uname -o 2>&1 | grep Cygwin),)
		#set up for linux/mingw
		TOOLOVERRIDES=PATH="/usr/bin:$(realpath $(TOOLCHAINPATH))" CFLAGS=--sysroot="$(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC))" CPPFLAGS=--sysroot="$(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC))" 
		CONFIGARGS= --with-sysroot="$(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC))" 
	else
		#we're running upon cygwin
		#FIXME: support mingw too...

		ANDROID_SCRIPT=android.bat
		#make can't cope with absolute win32 paths in dependancy files
		DEPCC=
		DEPCXX=

		ifneq ($(realpath $(TOOLCHAINPATH)),)	#don't invoke cygpath when realpath returns nothing due to dodgy paths (which happens when stuff isn't set up right for the top-level makefile)
			#configure hates android, with its broken default sysroot and lack of path etc
			DROIDSYSROOT:=$(shell cygpath -m $(DROIDSYSROOT))
			TOOLOVERRIDES=PATH="/usr/bin:$(shell cygpath -u $(realpath $(TOOLCHAINPATH)))" CFLAGS=--sysroot="$(DROIDSYSROOT)" CPPFLAGS=--sysroot="$(DROIDSYSROOT)" 
			CONFIGARGS= --with-sysroot="$(shell cygpath -u $(realpath $(ANDROID_NDK_ROOT)/platforms/$(DROID_API_NAME)/$(DROID_PLAT_INC)))" 
		endif
	endif

	CC:=$(TOOLCHAIN)gcc --sysroot="$(DROIDSYSROOT)" -DANDROID $(DROID_ABI) -fno-strict-aliasing
	CXX:=$(TOOLCHAIN)g++ --sysroot="$(DROIDSYSROOT)" -DANDROID $(DROID_ABI) -fno-strict-aliasing
	DO_LD=+$(DO_ECHO) $(CC) -Wl,-soname,libftedroid.so -shared -Wl,--no-undefined -Wl,-z,noexecstack -o $@ $(LTO_LD) $(WCFLAGS) $(BRANDFLAGS) $(CFLAGS) -llog -lc -lm -lz
	LD:=$(TOOLCHAIN)ld
	AR:=$(TOOLCHAIN)ar
	STRIP=$(TOOLCHAIN)strip
endif


ifeq ($(FTE_TARGET),win64_sdl)
	FTE_TARGET=win64_SDL
endif

#crosscompile macosx from linux, default target ppc 32bit
ifeq ($(FTE_TARGET),macosx)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which powerpc-apple-darwin8-gcc 2> /dev/null),)
			CC=powerpc-apple-darwin8-gcc
			CXX=powerpc-apple-darwin8-g++
			STRIP=powerpc-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			BITS=32
			EXTENSION=_ppc
		endif
	endif
endif

ifeq ($(FTE_TARGET),macosx_ppc64)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which powerpc-apple-darwin8-gcc 2> /dev/null),)
			FTE_TARGET=macosx
			CC=powerpc-apple-darwin8-gcc -arch ppc64
			CXX=powerpc-apple-darwin8-g++ -arch ppc64
			STRIP=powerpc-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			BITS=64
			EXTENSION=_ppc
		endif
	endif
endif

ifeq ($(FTE_TARGET),macosx_x86)
	ifeq ($(shell $(CC) -v 2>&1 | grep apple),)
		ifneq ($(shell which i686-apple-darwin8-gcc 2> /dev/null),)
			FTE_TARGET=macosx
			# i686-apple-darwin8-gcc's default target is i386, powerpc-apple-darwin8-gcc -arch i386 just invokes i686-apple-darwin8-gcc anyway
			CC=i686-apple-darwin8-gcc
			CXX=i686-apple-darwin8-g++
			STRIP=i686-apple-darwin8-strip
			#seems, macosx has a more limited version of strip
			STRIPFLAGS=
			EXTENSION=_x86
		endif
	endif
endif

#crosscompile morphos from linux
ifeq ($(FTE_TARGET),morphos)
	ifeq ($(shell $(CC) -v 2>&1 | grep morphos),)
		ifneq ($(shell which ppc-morphos-gcc 2> /dev/null),)
			CC=ppc-morphos-gcc
			CXX=ppc-morphos-g++
			#morphos strip has a 'feature', it strips permissions
			STRIP=ppc-morphos-strip
		endif
	endif
endif

ifeq ($(FTE_TARGET),dos)
	#at least from dos.
	CC=i586-pc-msdosdjgpp-gcc
	CXX=i586-pc-msdosdjgpp-g++
	STRIP=i586-pc-msdosdjgpp-strip
	CFLAGS+=-DNO_ZLIB
endif

#if you have an x86, you can get gcc to build binaries using 3 different ABIs, instead of builds for just the default ABI
ifeq ($(FTE_TARGET),linux32)
	FTE_TARGET=linux
	CC=gcc -m32
	CXX=g++ -m32
	STRIP=strip
	BITS=32
endif
ifeq ($(FTE_TARGET),linux_x86)
	FTE_TARGET=linux
	CC=i686-linux-gnu-gcc
	PKGCONFIG=i686-linux-gnu-pkg-config
	CXX=i686-linux-gnu-g++
	STRIP=i686-linux-gnu-strip
	BITS=32
endif
ifeq ($(FTE_TARGET),linux_amd64)
	FTE_TARGET=linux
	CC=x86_64-linux-gnu-gcc
	PKGCONFIG=x86_64-linux-gnu-pkg-config
	CXX=x86_64-linux-gnu-g++
	STRIP=x86_64-linux-gnu-strip
	BITS=64
endif
ifeq ($(FTE_TARGET),linux_armhf)
	#debian's armhf is armv7, but armv6 works on RPI too.
	FTE_TARGET=linux
	CC=arm-linux-gnueabihf-gcc -marm -march=armv6 -mfpu=vfp -mfloat-abi=hard
	CXX=arm-linux-gnueabihf-g++ -marm -march=armv6 -mfpu=vfp -mfloat-abi=hard
	STRIP=arm-linux-gnueabihf-strip
	BITS=armhf
endif
ifeq ($(FTE_TARGET),linux_arm64)
	FTE_TARGET=linux
	CC=aarch64-linux-gnu-gcc
	CXX=aarch64-linux-gnu-g++
	STRIP=aarch64-linux-gnu-strip
	BITS=arm64
	USE_SPEEX=0 #fails to compile due to neon asm, I'm just going to disable it (will still soft-link).
endif
ifeq ($(FTE_TARGET),linux_aarch64)
	FTE_TARGET=linux
	CC=aarch64-linux-gnu-gcc
	CXX=aarch64-linux-gnu-g++
	STRIP=aarch64-linux-gnu-strip
	BITS=aarch64
	USE_SPEEX=0 #fails to compile due to neon asm, I'm just going to disable it (will still soft-link).
endif
ifeq ($(FTE_TARGET),linux_x32)
	#DO NOT CONFUSE WITH linux_x86. this target is amd64-with-32bit-pointers
	#note: the x32 abi is still not finished or something.
	#at the current time, you will need to edit your kernel's commandline to allow this stuff to run
	#try and use a proper cross-compiler if we can, otherwise fall back on multi-arch.
	FTE_TARGET=linux
ifneq ($(shell which x86_64-linux-gnux32-gcc 2> /dev/null),)
	CC=x86_64-linux-gnux32-gcc
	CXX=x86_64-linux-gnux32-g++
	STRIP=x86_64-linux-gnux32-strip
else
	CC=gcc -mx32
	CXX=g++ -mx32
	STRIP=strip
endif
	BITS=x32
endif
ifeq ($(FTE_TARGET),linux64)
	FTE_TARGET=linux
	CC=gcc -m64
	CXX=g++ -m64
	STRIP=strip
	BITS=64
endif
ifeq ($(FTE_TARGET),web)
	CC=emcc
	CXX=em++
	AR=emar
endif
ifeq ($(FTE_TARGET),cygwin)
	FTE_TARGET=cyg
endif
ifeq ($(shell uname -s),OpenBSD)
	PKGCONFIG=pkg-config
endif

ifeq ($(FTE_TARGET),) 	#user didn't specify prefered target
	ifneq ($(shell uname 2>&1 | grep CYGWIN),)
		FTE_TARGET=cyg
		ANDROID_SCRIPT=android.bat
	endif
	ifneq ($(shell $(CC) -v 2>&1 | grep mingw),)
		FTE_TARGET=win32
	endif
	ifeq ($(FTE_TARGET),) 	#still not set
		UNAME_SYSTEM:=$(shell uname)
		ifeq ($(UNAME_SYSTEM),Linux)
			FTE_TARGET=linux
		endif
		ifeq ($(UNAME_SYSTEM),Darwin)
			FTE_TARGET=macosx
		endif
		ifeq ($(UNAME_SYSTEM),FreeBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(UNAME_SYSTEM),NetBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(UNAME_SYSTEM),OpenBSD)
			FTE_TARGET=bsd
		endif
		ifeq ($(UNAME_SYSTEM),MorphOS)
			FTE_TARGET=morphos
		endif
		#else I've no idea what it is you're running
	endif

	FTE_TARGET ?= unk	#so go for sdl.
endif

ifneq ($(shell ls|grep config.h),)
	HAVECONFIG=-DHAVE_CONFIG_H
endif

CLIENT_DIR=$(BASE_DIR)/client
GL_DIR=$(BASE_DIR)/gl
D3D_DIR=$(BASE_DIR)/d3d
VK_DIR=$(BASE_DIR)/vk
SW_DIR=$(BASE_DIR)/sw
SERVER_DIR=$(BASE_DIR)/server
COMMON_DIR=$(BASE_DIR)/common
HTTP_DIR=$(BASE_DIR)/http
#LIBS_DIR=$(BASE_DIR)/libs
LIBS_DIR?=.
PROGS_DIR=$(BASE_DIR)/qclib

ifeq ($(NOCOMPAT),1)
	NCCFLAGS=-DNOLEGACY -DOMIT_QCC
	NCDIRPREFIX=nc
endif
ALL_CFLAGS=$(HAVECONFIG) $(VISIBILITY_FLAGS) $(BRANDFLAGS) $(CFLAGS) $(BASE_CFLAGS) $(WCFLAGS) $(ARCH_CFLAGS) $(NCCFLAGS) -I$(ARCHLIBS)
ALL_CXXFLAGS=$(subst -Wno-pointer-sign,,$(ALL_CFLAGS))

#cheap compile-everything-in-one-unit (compile becomes preprocess only)
ifneq ($(WPO),)
	LTO_CC= -E
	LTO_LD= -flto=jobserver -fwhole-program -x c
	LTO_END=ltoxnone
	LTO_START=ltoxc
endif
#proper/consistant link-time optimisations (requires gcc 4.5+ or so)
ifneq ($(LTO),)
	LTO_CC=-flto=jobserver -fvisibility=hidden
	LTO_LD=-flto=jobserver
endif

#DO_ECHO=@echo $< && 
DO_ECHO=@
#DO_ECHO=
DO_CC=$(DO_ECHO) $(CC) $(LTO_CC) $(ALL_CFLAGS) -o $@ -c $<
DO_CXX=$(DO_ECHO) $(CXX) $(LTO_CC) $(ALL_CXXFLAGS) -o $@ -c $<

ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
	BASELDFLAGS=
	VISIBILITY_FLAGS=
endif
ifeq ($(FTE_TARGET),cyg)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),dos)
	BASELDFLAGS=-lm
endif
ifeq ($(FTE_TARGET),morphos)
	BASELDFLAGS=-lm
endif

ifneq (,$(findstring bsd,$(FTE_TARGET)))
	BASELDFLAGS=-lm
endif
BASELDFLAGS ?= -lm -ldl -lpthread

ifeq (win,$(findstring cyg,$(FTE_TARGET))$(findstring win,$(FTE_TARGET)))
	BASELDFLAGS=-lm
#	MINGW_LIBS_DIR=$(LIBS_DIR)/mingw-libs

#	ifeq ($(shell echo $(FTE_TARGET)|grep -v win64),)
#		MINGW_LIBS_DIR=$(LIBS_DIR)/mingw64-libs
#	endif

#	IMAGELDFLAGS=$(MINGW_LIBS_DIR)/libpng.a $(MINGW_LIBS_DIR)/libz.a $(MINGW_LIBS_DIR)/libjpeg.a
#	OGGVORBISLDFLAGS=$(MINGW_LIBS_DIR)/libvorbisfile.a $(MINGW_LIBS_DIR)/libvorbis.a $(MINGW_LIBS_DIR)/libogg.a
endif

#BASELDFLAGS=-lm  -lz
XLDFLAGS=-L$(ARCHLIBS) $(IMAGELDFLAGS)

#hack some other arguments based upon the toolchain
ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
	WARNINGFLAGS?=-W3 -D_CRT_SECURE_NO_WARNINGS
	GNUC_FUNCS=
else
	WARNINGFLAGS?=-Wall -Wno-pointer-sign -Wno-unknown-pragmas -Wno-format-zero-length -Wno-strict-aliasing #-Wcast-align
#	GNUC_FUNCS= -Dstrnicmp=strncasecmp -Dstricmp=strcasecmp
endif

SDL_INCLUDES=
#-I$(LIBS_DIR)/sdl/include -I/usr/include/SDL -I$(LIBS_DIR)/sdl/include/SDL
BASE_INCLUDES=-I$(CLIENT_DIR) -I$(SERVER_DIR) -I$(COMMON_DIR) -I$(GL_DIR) -I$(D3D_DIR) -I$(PROGS_DIR) -I. 
BASE_CFLAGS=$(WARNINGFLAGS) $(GNUC_FUNCS) $(BASE_INCLUDES) -I$(LIBS_DIR)/dxsdk9/include -I$(LIBS_DIR)/dxsdk7/include $(SDL_INCLUDES) $(SVNREVISION)
CLIENT_ONLY_CFLAGS=-DCLIENTONLY
SERVER_ONLY_CFLAGS=-DSERVERONLY
JOINT_CFLAGS=
DEBUG_CFLAGS?=-ggdb -g
DEBUG_CFLAGS+=-DDEBUG -D_DEBUG
RELEASE_CFLAGS?=$(CPUOPTIMIZATIONS)
#
#note: RELEASE_CFLAGS used to contain -ffast-math
#however, its use resulted in the player getting stuck etc, so be warned if you try re-enabling it.
#

ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
	#msvc doesn't do -dumpmachine.
	#we might as well get it to reuse the mingw libraries, if only because that makes those libraries easier to compile...
	ifeq ($(FTE_TARGET),msvc32)
		BITS?=32
	else ifeq ($(FTE_TARGET),msvc64)
		BITS?=64
	endif
	BITS?=32
	ifeq ($(BITS),64)
		ARCH?=x86_64-w64-mingw32
	else
		ARCH?=i686-w64-mingw32
	endif
else
	#some idiot decided that -dumpmachine shouldn't respect -m32 etc.
	#at the same time, -print-multiarch is not present, buggy, or just screwed in many gcc builds (ones that target a single arch will unhelpfully just give an empty string).
	#so try multiarch first, and if that fails risk dumpmachine giving the wrong values.
	#really we want dumpmachine's more specific cpu arch included here, so lets hope that idiot burns for all eternity. or something equally melodramatic.
	ARCH:=$(shell $(CC) -print-multiarch 2>/dev/null)
	ifneq ($(words $(ARCH)),1)
		ARCH:=$(shell $(CC) -dumpmachine 2>/dev/null)
	endif
	#foo:=$(shell echo ARCH is $(ARCH) 1>&2 )
endif
ARCHLIBS:=$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)

#incase our compiler doesn't support it (mingw)
ifeq ($(shell LANG=c $(CC) -rdynamic 2>&1 | grep unrecognized),)
	DEBUG_CFLAGS+= -rdynamic
endif

PKGCONFIG?=$(ARCH)-pkg-config
ifeq ($(shell which $(PKGCONFIG) 2> /dev/null),)
	FFS:=$(shell echo $(PKGCONFIG) not found 1>&2 )
	PKGCONFIG=/bin/false #don't end up using eg /usr/include when cross-compiling. makelibs is a valid workaround.
endif
#try to statically link
ifeq ($(COMPILE_SYS),Darwin)
	ifneq (,$(findstring SDL,$(FTE_TARGET)))
		IMAGELDFLAGS := $(shell $(PKGCONFIG) libpng --variable=libdir)/libpng.a $(shell $(PKGCONFIG) libjpeg --variable=libdir)/libjpeg.a
		OGGVORBISLDFLAGS := $(shell $(PKGCONFIG) vorbisfile --variable=libdir)/libvorbisfile.a $(shell $(PKGCONFIG) vorbis --variable=libdir)/libvorbis.a $(shell $(PKGCONFIG) ogg --variable=libdir)/libogg.a
	endif
endif

# Statically link everything but SDL/AL for the SDL target,
# this assumes makelibs; but we don't really care. These are portable builds
# that can only make one assumption: SDL/AL are installed.
# Dependencies like libjpeg9 don't exist everywhere.
ifeq ($(FTE_TARGET),SDL2)
	IMAGELDFLAGS:= libs-$(ARCH)/libpng.a libs-$(ARCH)/libjpeg.a
	OGGVORBISLDFLAGS:= libs-$(ARCH)/libvorbisfile.a libs-$(ARCH)/libvorbis.a libs-$(ARCH)/libogg.a
	BASE_CFLAGS+=-DLIBJPEG_STATIC -DLIBJPEG_STATIC -DLIBPNG_STATIC -DOPUS_STATIC -DSPEEX_STATIC -DFREETYPE_STATIC -DLIBVORBISFILE_STATIC
endif

VPATH := $(BASE_DIR) : $(CLIENT_DIR) : $(GL_DIR) : $(SW_DIR) : $(COMMON_DIR) : $(SERVER_DIR) : $(HTTP_DIR) : $(QUX_DIR) : $(PROGS_DIR) : $(D3D_DIR) : $(VK_DIR) : $(BASE_DIR)/web

PROFILE_CFLAGS=-pg
DX7SDK=-I./libs/dxsdk7/include/

GLCFLAGS?=-DGLQUAKE
D3DCFLAGS?=-DD3D9QUAKE -DD3D11QUAKE
VKCFLAGS?=-DVKQUAKE

CLIENT_OBJS = \
	textedit.o	\
	fragstats.o	\
	zqtp.o	\
	cl_demo.o	\
	cl_ents.o	\
	clq2_ents.o	\
	cl_input.o	\
	in_generic.o	\
	cl_main.o	\
	cl_parse.o	\
	cl_pred.o	\
	cl_tent.o	\
	cl_cam.o	\
	cl_screen.o	\
	pr_clcmd.o	\
	cl_ignore.o \
	pr_csqc.o	\
	console.o	\
	image.o	\
	keys.o	\
	menu.o	\
	m_master.o	\
	m_multi.o	\
	m_items.o	\
	m_options.o	\
	m_single.o	\
	m_script.o	\
	m_native.o	\
	m_mp3.o	\
	roq_read.o	\
	clq2_cin.o	\
	r_part.o	\
	p_script.o	\
	p_null.o	\
	p_classic.o	\
	r_partset.o	\
	renderer.o	\
	renderque.o	\
	sbar.o	\
	skin.o	\
	snd_al.o	\
	snd_dma.o	\
	snd_mem.o	\
	snd_mix.o	\
	snd_mp3.o	\
	snd_ov.o	\
	valid.o	\
	vid_headless.o	\
	view.o	\
	wad.o			\
				\
	ftpclient.o		\
				\
				\
	pr_menu.o

VKQUAKE_OBJS =		\
	vk_init.o	\
	vk_backend.o

GLQUAKE_OBJS =		\
	gl_draw.o		\
	gl_backend.o		\
	gl_rmain.o		\
	gl_rmisc.o		\
	gl_rsurf.o		\
	gl_screen.o		\
	gl_bloom.o		\
	gl_vidcommon.o		\
	$(VKQUAKE_OBJS)

D3DQUAKE_OBJS =		\
	d3d8_backend.o \
	d3d8_image.o	\
	vid_d3d8.o	\
	d3d_backend.o \
	d3d_image.o	\
	d3d_shader.o	\
	vid_d3d.o	\
	d3d11_backend.o \
	d3d11_image.o	\
	d3d11_shader.o	\
	vid_d3d11.o
	
D3DGL_OBJS =		\
	gl_font.o \
	gl_ngraph.o		\
	gl_shader.o	\
	gl_shadow.o	\
	gl_rlight.o	\
	gl_warp.o	\
	ltface.o	\
	r_surf.o	\
	r_2d.o

MP3_OBJS =			\
	fixed.o		\
	bit.o			\
	timer.o		\
	stream.o		\
	frame.o		\
	synth.o		\
	decoder.o		\
	layer12.o		\
	layer3.o		\
	huffman.o		\
	mymad.o

QCC_OBJS=	\
	comprout.o		\
	hash.o		\
	qcc_cmdlib.o	\
	qccmain.o		\
	qcc_pr_comp.o	\
	qcc_pr_lex.o	\
	qcd_main.o
PROGS_OBJS =		\
	$(QCC_OBJS)	\
	initlib.o		\
	pr_bgcmd.o		\
	pr_skelobj.o		\
	pr_edict.o		\
	pr_exec.o		\
	pr_multi.o		\
	pr_x86.o		\
	qcdecomp.o

SERVER_OBJS = 		\
	pr_cmds.o 		\
	pr_q1qvm.o	\
	pr_lua.o	\
	sv_master.o 	\
	sv_init.o 		\
	sv_main.o 		\
	sv_nchan.o 		\
	sv_ents.o 		\
	sv_send.o 		\
	sv_user.o		\
	sv_sql.o		\
	sv_mvd.o		\
	sv_ccmds.o 		\
	sv_cluster.o		\
	sv_rankin.o 	\
	sv_chat.o 		\
	sv_demo.o		\
	net_preparse.o 	\
	savegame.o		\
	svq2_ents.o 	\
	svq2_game.o 	\
	webgen.o		\
	ftpserver.o		\
	httpserver.o

SERVERONLY_OBJS =		\
	sv_sys_unix.o		\
	sys_linux_threads.o

WINDOWSSERVERONLY_OBJS = \
	net_ssl_winsspi.o \
	sv_sys_win.o	\
	sys_win_threads.o

WINDOWS_OBJS = \
	snd_win.o \
	snd_directx.o \
	snd_xaudio.o \
	snd_wasapi.o \
	cd_win.o \
	fs_win32.o \
	in_win.o \
	sys_win.o \
	sys_win_threads.o \
	net_ssl_winsspi.o \
	$(LTO_END) resources.o $(LTO_START)

COMMON_OBJS = \
	gl_alias.o		\
	gl_hlmdl.o	\
	gl_heightmap.o		\
	gl_model.o	\
	com_bih.o	\
	com_mesh.o	\
	common.o 		\
	json.o		\
	cvar.o 		\
	cmd.o 		\
	crc.o 		\
	net_ssl_gnutls.o \
	net_master.o	\
	fs.o			\
	fs_stdio.o		\
	fs_pak.o		\
	fs_zip.o		\
	fs_dzip.o		\
	fs_xz.o		\
	m_download.o	\
	mathlib.o 		\
	huff.o		\
	md4.o 		\
	md5.o 		\
	sha1.o		\
	sha2.o		\
	log.o 		\
	net_chan.o 		\
	net_wins.o 		\
	net_ice.o 		\
	httpclient.o 	\
	zone.o 		\
	qvm.o	\
	r_d3.o	\
	gl_q2bsp.o		\
	glmod_doom.o 	\
	world.o 		\
	sv_phys.o 		\
	sv_move.o 		\
	pmove.o		\
	pmovetst.o		\
	iwebiface.o		\
	translate.o		\
	plugin.o		\
	q1bsp.o		\
	q2pmove.o

ifeq (1,$(LINK_QUAKE3))
    VPATH := $(VPATH) : $(BASE_DIR)/../plugins/quake3 : $(BASE_DIR)/../plugins/quake3/botlib
    ALL_CFLAGS+=-DBOTLIB -DBOTLIB_STATIC -DSTATIC_Q3
    COMMON_OBJS += 			\
		clq3_parse.o	\
		clq3_ui.o	\
		clq3_cg.o 	\
		svq3_game.o	\
		q3common.o	\
		be_aas_bspq3.o		\
		be_aas_cluster.o	\
		be_aas_debug.o		\
		be_aas_entity.o		\
		be_aas_file.o		\
		be_aas_main.o		\
		be_aas_move.o		\
		be_aas_optimize.o	\
		be_aas_reach.o		\
		be_aas_route.o		\
		be_aas_routealt.o	\
		be_aas_sample.o		\
		be_ai_char.o		\
		be_ai_chat.o		\
		be_ai_gen.o		\
		be_ai_goal.o		\
		be_ai_move.o		\
		be_ai_weap.o		\
		be_ai_weight.o		\
		be_ea.o			\
		be_interface.o		\
		l_crc.o			\
		l_libvar.o		\
		l_log.o			\
		l_memory.o		\
		l_precomp.o		\
		l_script.o		\
		l_struct.o	
endif

COMMONLIBFLAGS?=
COMMONLDDEPS?=
CLIENTLIBFLAGS=$(COMMONLIBFLAGS) $(LIBOPUS_STATIC) $(LIBSPEEX_STATIC) $(OGGVORBISFILE_STATIC)
SERVERLIBFLAGS=$(COMMONLIBFLAGS)
CLIENTLDDEPS=$(COMMONLDDEPS) $(LIBOPUS_LDFLAGS) $(LIBSPEEX_LDFLAGS) $(OGGVORBISLDFLAGS)
SERVERLDDEPS=$(COMMONLDDEPS)
ifeq (1,$(USE_OPUS))
    LIBOPUS_STATIC=-DOPUS_STATIC
    LIBOPUS_LDFLAGS=-lopus
    ALL_CFLAGS+=-I/usr/include/opus
    MAKELIBS+=libs-$(ARCH)/libopus.a
endif
ifeq (1,$(USE_SPEEX))
    LIBSPEEX_STATIC=-DSPEEX_STATIC
    LIBSPEEX_LDFLAGS=-lspeex -lspeexdsp
    MAKELIBS+=libs-$(ARCH)/libspeex.a libs-$(ARCH)/libspeexdsp.a
endif

ifeq (1,$(USE_VORBISFILE))
    OGGVORBISFILE_STATIC=-DLIBVORBISFILE_STATIC
    OGGVORBISLDFLAGS ?= -lvorbisfile -lvorbis -logg
else
    OGGVORBISLDFLAGS=
    OGGVORBISFILE_STATIC=
endif

#freetype is annoying.
ifeq (1,$(LINK_FREETYPE))
    CLIENTLIBFLAGS+=-DFREETYPE_STATIC
    CLIENTLDDEPS+=$(FREETYPE_LDFLAGS)
endif
ifneq ("$(wildcard $(ARCHLIBS))","")
    #makelibs has been used. we know what to use here.
    FREETYPE_CFLAGS:=
    FREETYPE_LDFLAGS:=-lfreetype
else
    #we're using system libraries... lets hope pkg-config knows it and can handle any cross compiles.
    FREETYPE_CFLAGS:=$(shell $(PKGCONFIG) freetype2 --cflags --silence-errors)
    FREETYPE_LDFLAGS:=$(shell $(PKGCONFIG) freetype2 --libs --silence-errors)
    ifeq (,$(FREETYPE_CFLAGS))
        #system freetype headers not installed. don't try to include them!
        FREETYPE_CFLAGS:=-DNO_FREETYPE
        FREETYPE_LDFLAGS:=
    endif
endif
ALL_CFLAGS+=$(FREETYPE_CFLAGS)

ifeq (1,$(LINK_PNG))
    CLIENTLIBFLAGS+=-DLIBPNG_STATIC
    CLIENTLDDEPS+=-lpng
endif
ifeq (1,$(LINK_JPEG))
    CLIENTLIBFLAGS+=-DLIBJPEG_STATIC
    CLIENTLDDEPS+=-ljpeg
endif
ifeq (1,$(LINK_ZLIB))
    CLIENTLIBFLAGS+=-DZLIB_STATIC
    CLIENTLDDEPS+=-lz
	SERVERLDDEPS+=-lz

    #and deflate64, because why not.
    ifneq ("$(wildcard $(ARCHLIBS)/infback9.h)","")
        CLIENTLIBFLAGS+=-DZLIB_DEFLATE64
        CLIENTLDDEPS+=-lz9
        QCC_CFLAGS+=-DZLIB_DEFLATE64
        QCC_LDFLAGS+=-lz9
    endif
endif
ifeq (1,$(LINK_ODE))
    ALL_CFLAGS+=$(shell $(PKGCONFIG) ode --cflags --silence-errors) -DODE_STATIC
    COMMONLDDEPS+=$(shell $(PKGCONFIG) ode --libs --silence-errors)
endif
ifeq (1,$(strip $(INTERNAL_BULLET)))
    COMMON_OBJS+=com_phys_bullet.o
    ALL_CFLAGS+=-I/usr/include/bullet -I$(ARCHLIBS)/bullet3-$(BULLETVER)/src
    COMMONLDDEPS+=-lBulletDynamics -lBulletCollision -lLinearMath
    LDCC=$(CXX)
    MAKELIBS+=libs-$(ARCH)/libBulletDynamics.a
endif
ifeq (1,$(LINK_EZHUD))
    VPATH := $(VPATH) : $(BASE_DIR)/../plugins/ezhud
    ALL_CFLAGS+=-DSTATIC_EZHUD #let the plugins code know it needs to add the appropriate entry.
    CLIENT_OBJS += \
		ezquakeisms.o	\
		hud.o			\
		hud_common.o	\
		hud_editor.o
endif
ifeq (1,$(LINK_OPENSSL))
    ifeq (1,$(shell $(PKGCONFIG) --atleast-version 3 openssl && echo 1))	#must exist... and if its not openssl3 then its not gpl3-compatible so refuse to use it.
        VPATH := $(VPATH) : $(BASE_DIR)/../plugins
        ALL_CFLAGS+=-DSTATIC_OPENSSL #let the plugins code know it needs to add the appropriate entry.
        COMMON_OBJS += \
            net_ssl_openssl.o
        ALL_DFLAGS+=$(shell $(PKGCONFIG) openssl --cflags --silence-errors)
        COMMONLDDEPS+=$(shell $(PKGCONFIG) openssl --libs --silence-errors)
    endif
endif

ifeq (1,$(shell $(PKGCONFIG) --exists gnutls && echo 1))
    ALL_CFLAGS+=$(shell $(PKGCONFIG) gnutls --cflags --silence-errors)
    #ALL_CFLAGS+=-DGNUTLS_STATIC
    #COMMONLDDEPS+=$(shell $(PKGCONFIG) gnutls --static --libs --silence-errors)	#we soft-link, so we don't need the -lgnutls stuff.
else
    ALL_CFLAGS+=-DNO_GNUTLS
endif


#the defaults for sdl come first
#CC_MACHINE:=$(shell $(CC) -dumpmachine)
ifeq ($(FTE_TARGET),SDL2)
    SDLCONFIG?=sdl2-config
    FTE_FULLTARGET?=sdl2$(BITS)
endif
ifeq ($(FTE_TARGET),SDL1)
    SDLCONFIG?=sdl-config
    FTE_FULLTARGET?=sdl1$(BITS)
endif
ifeq ($(FTE_TARGET),SDL)
    FTE_FULLTARGET?=sdl$(BITS)
endif
SDLCONFIG?=sdl-config
FTE_FULLTARGET?=sdl$(FTE_TARGET)$(BITS)

GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o
GL_EXE_NAME=../$(EXE_NAME)-gl$(FTE_FULLTARGET)
GLCL_EXE_NAME=../$(EXE_NAME)cl-gl$(FTE_FULLTARGET)

#SDLCONFIG:=libs-$(ARCH)/sdl2_mingw/$(CC_MACHINE)/bin/sdl2-config --prefix=libs-$(ARCH)/sdl2_mingw/$(CC_MACHINE)
ifdef windir
	GL_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
	VK_LDFLAGS=$(VKLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
	M_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
	SV_LDFLAGS=`$(SDLCONFIG) --static-libs`
else
	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) `$(SDLCONFIG) --libs`
	VK_LDFLAGS=$(VKLDFLAGS) $(IMAGELDFLAGS) `$(SDLCONFIG) --libs`
	M_LDFLAGS=$(MLDFLAGS) $(IMAGELDFLAGS) `$(SDLCONFIG) --libs`
	SV_LDFLAGS=`$(SDLCONFIG) --libs`
endif
GL_CFLAGS=-DFTE_SDL $(GLCFLAGS) -DMULTITHREAD `$(SDLCONFIG) --cflags`
GLB_DIR=gl_$(FTE_FULLTARGET)
GLCL_DIR=glcl_$(FTE_FULLTARGET)
SV_DIR?=sv_$(FTE_FULLTARGET)

VKCL_OBJS=$(VKQUAKE_OBJS) $(D3DGL_OBJS) gl_bloom.o gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o
VK_CFLAGS=-DFTE_SDL $(VKCFLAGS) -DMULTITHREAD `$(SDLCONFIG) --cflags`
VKB_DIR=vk_$(FTE_FULLTARGET)
VKCL_DIR=vk_$(FTE_FULLTARGET)
VK_EXE_NAME=../$(EXE_NAME)-vk$(FTE_FULLTARGET)
VKCL_EXE_NAME=../$(EXE_NAME)-vkcl$(FTE_FULLTARGET)

SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
SV_EXE_NAME=../$(EXE_NAME)-sv$(FTE_FULLTARGET)
SV_CFLAGS=-DFTE_SDL -DMULTITHREAD `$(SDLCONFIG) --cflags` $(SERVER_ONLY_CFLAGS)

MINGL_DIR=mingl_$(FTE_FULLTARGET)
MINGL_EXE_NAME=../$(EXE_NAME)-mingl$(FTE_FULLTARGET)

MB_DIR=m_$(FTE_FULLTARGET)
MCL_DIR=mcl_$(FTE_FULLTARGET)
M_EXE_NAME=../$(EXE_NAME)-$(FTE_FULLTARGET)
MCL_EXE_NAME=../$(EXE_NAME)-cl$(FTE_FULLTARGET)
MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o 
M_CFLAGS=-DFTE_SDL $(VKCFLAGS) $(GLCFLAGS) -DMULTITHREAD `$(SDLCONFIG) --cflags`

QCC_DIR=qcc$(BITS)

ifeq (,$(findstring NO_ZLIB,$(CFLAGS)))
    SV_LDFLAGS+=-lz
    GL_LDFLAGS+=-lz
    VK_LDFLAGS+=-lz
    M_LDFLAGS+=-lz
    QCC_LDFLAGS+=-L$(ARCHLIBS) -lz
    QTV_LDFLAGS+=-lz
endif



#specific targets override those defaults as needed.

#FTE_TARGET=win32_SDL | FTE_TARGET=win64_SDL (MinGW32 + SDL | MinGW64 + SDL)
ifeq (win_SDL,$(findstring win,$(FTE_TARGET))$(findstring _SDL,$(FTE_TARGET)))
    DO_CMAKE+=-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM="NEVER"

    ifneq (,$(findstring win64,$(FTE_TARGET)))
        BITS=64
    endif

    EXEPOSTFIX=.exe

    ARCH_PREDEP=$(BASE_DIR)/libs-$(ARCH)/SDL2-$(SDL2VER)/$(ARCH)/bin/sdl2-config
    SDLCONFIG=$(ARCH_PREDEP) --prefix=$(BASE_DIR)/libs-$(ARCH)/SDL2-$(SDL2VER)/$(ARCH)
    CLIENTLIBFLAGS+=`$(SDLCONFIG) --cflags`

    #the defaults for sdl come first
    GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
    GL_EXE_NAME=../$(EXE_NAME)-sdl-gl$(BITS)$(EXEPOSTFIX)
    GLCL_EXE_NAME=../$(EXE_NAME)-sdl-glcl$(BITS)$(EXEPOSTFIX)
    ifneq ($(SDL_STATIC),0)
		#statically link by default. dlls suck.
        GL_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
        VK_LDFLAGS=$(GLLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
        M_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
		D3D_LDFLAGS=$(MLDFLAGS) -lmingw32 -lws2_32 `$(SDLCONFIG) --static-libs`
        SV_LDFLAGS=-lm -lmingw32 -lws2_32 -lwinmm `$(SDLCONFIG) --static-libs`
        QCC_LDFLAGS=
    else
		#or dynamically link when SDL_STATIC=0
        GL_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 $(SDL_LDFLAGS) -mwindows -ldxguid -lwinmm -lole32 $(GLLDFLAGS) `$(SDLCONFIG) --libs` 
        VK_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 $(SDL_LDFLAGS) -mwindows -ldxguid -lwinmm -lole32 $(GLLDFLAGS) `$(SDLCONFIG) --libs` 
        M_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 $(SDL_LDFLAGS) -mwindows -ldxguid -lwinmm -lole32 $(MLDFLAGS) `$(SDLCONFIG) --libs`
		D3D_LDFLAGS=$(IMAGELDFLAGS) -lws2_32 -lmingw32 $(SDL_LDFLAGS) -mwindows -ldxguid -lwinmm -lole32 $(MLDFLAGS) `$(SDLCONFIG) --libs`
        SV_LDFLAGS=-lm -lmingw32 -lws2_32 -lwinmm `$(SDLCONFIG) --libs`
        QCC_LDFLAGS=
    endif

    GL_CFLAGS=-DFTE_SDL $(GLCFLAGS) $(CLIENTLIBFLAGS) $(DX7SDK)

    GLB_DIR=gl_mgw_sdl$(BITS)
    GLCL_DIR=glcl_mgw_sdl$(BITS)
	
	#don't use sdl at all for dedicated servers. it'd break running it as a console program etc. its not officially supported, but included anyway for ease of use.
	SV_DIR=sv_winsdl$(BITS)
    SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)
    SV_EXE_NAME=../$(EXE_NAME)-sdl-sv$(BITS)$(EXEPOSTFIX)
	SV_LDFLAGS=-lm -lole32 -lws2_32 -lwinmm
    SV_CFLAGS=$(SERVER_ONLY_CFLAGS) -mconsole #-DFTE_SDL

    MINGL_DIR=mingl_sdlwin$(BITS)
    MINGL_EXE_NAME=../$(EXE_NAME)-sdl-mingl$(BITS)$(EXEPOSTFIX)

    MB_DIR=m_mgw_sdl$(BITS)
    M_EXE_NAME=../$(EXE_NAME)-sdl$(BITS)$(EXEPOSTFIX)
#with d3d...
    #MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
    #M_CFLAGS=$(D3DCFLAGS) $(VKCFLAGS) $(GLCFLAGS) -DFTE_SDL $(CLIENTLIBFLAGS) $(DX7SDK)
#without d3d...
    MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(LTO_END) resources.o $(LTO_START)
    M_CFLAGS=$(VKCFLAGS) $(GLCFLAGS) -DFTE_SDL $(CLIENTLIBFLAGS) $(DX7SDK)

	#unsupported, video code has winmsg/input junk in it.
    D3DCL_OBJS=$(D3DQUAKE_OBJS) snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(D3DGL_OBJS) $(LTO_END) resources.o $(LTO_START)
    D3D_EXE_NAME=../$(EXE_NAME)-sdl-d3d$(BITS)$(EXEPOSTFIX)
    D3DCL_EXE_NAME=../$(EXE_NAME)-sdl-d3dcl$(BITS)$(EXEPOSTFIX)
    D3D_CFLAGS=$(D3DCFLAGS) -DFTE_SDL -DNO_XFLIP $(CLIENTLIBFLAGS) $(DX7SDK)
    D3DB_DIR=sdl_d3d_mgw$(BITS)
    D3DCL_DIR=sdl_d3dcl_mgw$(BITS)


    VKCL_OBJS=$(VKQUAKE_OBJS) gl_bloom.o gl_vidsdl.o snd_sdl.o cd_sdl.o sys_sdl.o in_sdl.o snd_directx.o $(D3DGL_OBJS) $(LTO_END) resources.o $(LTO_START)
    VK_EXE_NAME=../$(EXE_NAME)-sdl-vk$(BITS)$(EXEPOSTFIX)
    VKCL_EXE_NAME=../$(EXE_NAME)-sdl-vkcl$(BITS)$(EXEPOSTFIX)
    VK_CFLAGS=$(VKCFLAGS) -DFTE_SDL -DNO_XFLIP $(CLIENTLIBFLAGS) $(DX7SDK)
    VKB_DIR=sdl_vk_mgw$(BITS)
    VKCL_DIR=sdl_vkcl_mgw$(BITS)
	
    ifeq ($(shell echo $(FTE_TARGET)|grep -E -i -v "win32.*sdl"),)
        GL_CFLAGS+= -D_MINGW_VFPRINTF
        VK_CFLAGS+= -D_MINGW_VFPRINTF
        D3D_CFLAGS+= -D_MINGW_VFPRINTF
        M_CFLAGS+= -D_MINGW_VFPRINTF
    endif
endif

#FTE_TARGET=vc (Visual C)
ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
	#assumptions...
	ifneq ($(shell uname -o 2>&1 | grep Cygwin),)
		#we're on cygwin
		MSVCPATH?=C:/Program Files (x86)/Microsoft Visual Studio 8/VC/
		WINDOWSSDKDIR?=C:/Program Files/Microsoft SDKs/Windows/v7.1/
		FIXPATH=`cygpath -m "$(1)"`
		INVOKE="$(1)"
	else
		#we're PROBABLY on linux (invoke it via wine)
		#MSVCPATH=/mnt/win/Program Files (x86)/Microsoft Visual Studio 10.0/VC/
		MSVCPATH?=/mnt/win/Program Files (x86)/Microsoft Visual Studio 8/VC/
		WINDOWSSDKDIR?=/mnt/win/Program Files/Microsoft SDKs/Windows/v7.1/
		#functions to handle wine (or cygwin)
		FIXPATH=`winepath -w "$(1)"`
		INVOKE=WINEDEBUG=-all WINEPATH="$(WINEPATH)" wine "$(1)"
	endif

	#our msvc library naming still assumes 32bit and thus no postfix for that target (microsoft usually reuse '32' for 64bit libraries too...).
	ifeq ($(BITS),32)
		LIBBITS=
	else
		LIBBITS=$(BITS)
	endif

	#evil lameness.
	LIBS_DIR=./libs/

	ifeq ($(BITS),64)
		MSVC_ARCH=amd64/
		WINSDK_ARCH=x64/
		WINEPATH=
	else
		MSVC_ARCH=
		WINSDK_ARCH=
		WINEPATH=$(MSVCPATH)../Common7/IDE/
	endif

	LIBOPUS_STATIC=

    #-I"$(MSVCPATH)/ATLMFC/INCLUDE"
	MSVCINC+=-I"$(shell winepath -w "$(MSVCPATH)INCLUDE")"
	MSVCINC+=-I"$(shell winepath -w "$(WINDOWSSDKDIR)/Include")"
	#-I"$(MSVCPATH)PlatformSDK/include"
	#-I"$(MSVCPATH)../SDK/v2.0/include"

	MSVCLIB+=/libpath:"$(call FIXPATH,$(MSVCPATH)lib/$(MSVC_ARCH))"
	MSVCLIB+=/libpath:"$(call FIXPATH,$(WINDOWSSDKDIR)Lib/$(WINSDK_ARCH))"
	MSVCLIB+=/libpath:"$(call FIXPATH,$(LIBS_DIR))"

	ARCHLIBS:=$(call FIXPATH,$(ARCHLIBS))

	MSVCINC:=$(MSVCINC)
	DO_WINDRES=$(DO_ECHO) $(WINDRES) /nologo $(BRANDFLAGS) $(MSVCINC) -I$(call FIXPATH,$(CLIENT_DIR)) -fo $(call FIXPATH,$@) $(call FIXPATH,$<)
	WINDRES=$(call INVOKE,$(WINDOWSSDKDIR)Bin/$(WINSDK_ARCH)RC.exe)
	LINK:=$(call INVOKE,$(MSVCPATH)/bin/$(MSVC_ARCH)link.exe)
	CC:=$(call INVOKE,$(MSVCPATH)/bin/$(MSVC_ARCH)cl.exe) $(MSVCINC) -DMSVCLIBSPATH=libs/
	CXX=$(CC)

	#library stuff... we'll be breaking attempts to statically link because deps are too painful otherwise.
	BASELDFLAGS=
	CLIENTLIBFLAGS=
	SERVERLIBFLAGS=
	CLIENTLDDEPS=
	SERVERLDDEPS=

	#override various flags that expected gcc.
	DEBUG_CFLAGS = -Od $(CPUOPTIMIZATIONS) /fp:fast
	RELEASE_CFLAGS = -O2 -Gy -GS- $(CPUOPTIMIZATIONS) /fp:fast
	RELEASE_LDFLAGS = /LTCG

	#we need some command tweaks.
	DO_CC=$(DO_ECHO) $(CC) /nologo $(ALL_CFLAGS) -Fo"$(call FIXPATH,$@)" -c "$(call FIXPATH,$<)"
	DO_CXX=$(DO_ECHO) $(CXX) /nologo $(ALL_CFLAGS) -Fo"$(call FIXPATH,$@)" -c "$(call FIXPATH,$<)"
	DO_LD=$(DO_ECHO) PATH="$(MSVCPATH)/Common7/IDE:$(PATH)" $(LINK) /nologo /out:"$(call FIXPATH,$@)" /nodefaultlib:libc.lib /LARGEADDRESSAWARE /nodefaultlib:MSVCRT $(MSVCLIB) $(SDKLIB) /manifest:no /OPT:REF  wsock32.lib user32.lib kernel32.lib advapi32.lib winmm.lib libs/zlib$(LIBBITS).lib shell32.lib
	STRIP=@echo SKIP: strip
	EXEPOSTFIX=.exe

	#gotta work around a whole load of paths.
	BASE_CFLAGS:=$(WARNINGFLAGS) $(GNUC_FUNCS) -I$(call FIXPATH,$(CLIENT_DIR)) -I$(call FIXPATH,$(SERVER_DIR)) -I$(call FIXPATH,$(COMMON_DIR)) -I$(call FIXPATH,$(GL_DIR)) -I$(call FIXPATH,$(D3D_DIR)) -I$(call FIXPATH,$(PROGS_DIR)) -I. -I$(LIBS_DIR) -I$(LIBS_DIR)/dxsdk9/include -I$(LIBS_DIR)/dxsdk7/include $(SDL_INCLUDES) $(BOTLIB_CFLAGS) $(SVNREVISION)


	#this stuff isn't supported.
	PRECOMPHEADERS=
	DEPCC=
	DEPCXX=

	#dedicated server stuff
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS) -DMULTITHREAD -DMSVCLIBPATH=libs/
	SV_EXE_NAME=../$(EXE_NAME)-sv$(BITS)$(EXEPOSTFIX)
	SV_DIR=sv_vc$(BITS)
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o resources.o
	SV_LDFLAGS=ole32.lib /subsystem:console

	GL_EXE_NAME=../$(EXE_NAME)-gl$(BITS)$(EXEPOSTFIX)
	GLCL_EXE_NAME=../$(EXE_NAME)-mingl$(BITS)
	GLB_DIR=gl_vc$(BITS)
	GLCL_DIR=glcl_vc$(BITS)
	GL_LDFLAGS=$(GLLDFLAGS) $(JPEGLIB) libs/libpng$(BITS).lib uuid.lib gdi32.lib ole32.lib /subsystem:windows
	GL_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) -DMULTITHREAD -DMSVCLIBPATH=libs/
	GLCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidnt.o $(WINDOWS_OBJS)
	GL_OBJS=

	MINGL_DIR=mingl_vc$(BITS)
	MINGL_EXE_NAME=../$(EXE_NAME)-mingl$(BITS)$(EXEPOSTFIX)

	VKCL_OBJS=$(VKQUAKE_OBJS) $(D3DGL_OBJS) gl_bloom.o gl_vidnt.o $(WINDOWS_OBJS)
	VK_EXE_NAME=../$(EXE_NAME)-vk$(BITS)$(EXEPOSTFIX)
	VKCL_EXE_NAME=../$(EXE_NAME)-vkcl$(BITS)$(EXEPOSTFIX)
	VK_CFLAGS=$(VKCFLAGS) $(CLIENTLIBFLAGS) $(DX7SDK) -DMULTITHREAD -DMSVCLIBPATH=libs/
	VK_LDFLAGS=$(JPEGLIB) libs/libpng$(BITS).lib uuid.lib gdi32.lib ole32.lib /subsystem:windows
	VKB_DIR=vk_vc$(BITS)
	VKCL_DIR=vkcl_vc$(BITS)

	D3DCL_OBJS=$(D3DQUAKE_OBJS) $(D3DGL_OBJS) $(WINDOWS_OBJS)
	D3D_EXE_NAME=../$(EXE_NAME)-d3d$(BITS)$(EXEPOSTFIX)
	D3DCL_EXE_NAME=../$(EXE_NAME)-d3dcl$(BITS)$(EXEPOSTFIX)
	D3D_LDFLAGS=$(JPEGLIB) libs/libpng$(BITS).lib uuid.lib gdi32.lib ole32.lib /subsystem:windows
	D3D_CFLAGS=$(D3DCFLAGS) $(W32_CFLAGS) $(DX7SDK) -DMULTITHREAD -DMSVCLIBPATH=libs/
	D3DB_DIR=d3d_vc$(BITS)
	D3DCL_DIR=d3dcl_vc$(BITS)

	#merged client stuff
	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) gl_vidnt.o gl_videgl.o $(WINDOWS_OBJS)
	M_EXE_NAME=../$(EXE_NAME)$(BITS)$(EXEPOSTFIX)
	MCL_EXE_NAME=../$(EXE_NAME)cl$(BITS)$(EXEPOSTFIX)
	M_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(D3DCFLAGS) $(DX7SDK) $(VKCFLAGS) -DMULTITHREAD $(CLIENTLIBFLAGS)
	MB_DIR=m_vc$(BITS)
	MCL_DIR=mcl_vc$(BITS)
	M_LDFLAGS=$(GLLDFLAGS) $(JPEGLIB) libs/libpng$(LIBBITS).lib uuid.lib gdi32.lib ole32.lib /subsystem:windows
endif

#FTE_TARGET=win32 | FTE_TARGET=win64 (MinGW32 | MinGW64)
ifeq (win,$(findstring win,$(FTE_TARGET))$(findstring _SDL,$(FTE_TARGET)))
	# The extra object file called resources.o is specific for MinGW to link the icon in
#	DO_CMAKE+=-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM="NEVER"

	DO_CMAKE=cmake -DCMAKE_TOOLCHAIN_FILE=/home/spike/fteqw/fteqw-code/cmakesucks.cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER="$(firstword $(CC))" -DCMAKE_C_FLAGS="$(wordlist 2,99,$(CC))" -DCMAKE_CXX_COMPILER="$(firstword $(CXX))" -DCMAKE_CXX_FLAGS="$(wordlist 2,99,$(CXX))" 

	#cygwin's gcc requires an extra command to use mingw instead of cygwin (default paths, etc).
	ifneq ($(shell $(CC) -dumpmachine 2>&1 | grep cygwin),)
		W32_CFLAGS=-mno-cygwin
	endif
	
	ifeq ($(FTE_TARGET),win64)
		BITS=64
	endif
	QCC_DIR=winqcc$(BITS)

	BASELDFLAGS=


	# Allow 32bit FTE to access beyond the 2GB address space
	ifeq ($(FTE_TARGET),win32)
		BASELDFLAGS=-Wl,--large-address-aware
	endif
	#Note: for deterministic builds, the following line disables timestamps for import/export tables. This is UNSAFE if there are any PE files bound to the compiled PE file. Our plugin dlls are dynamically loaded so this should not be an issue for us.
	BASELDFLAGS+=-Wl,--no-insert-timestamp
	DEBUG_LDFLAGS=-Wl,--no-dynamicbase	#debug builds are useful for catching crashes. the resulting stack traces are not very useful if we don't even know what the base address should be.
	RELEASE_LDFLAGS=-Wl,--dynamicbase	#release builds should attempt to use aslr

	BASELDFLAGS+=-lcomctl32
	EXEPOSTFIX=.exe

	QTV_LDFLAGS+=-lws2_32 -lwinmm

	SV_EXE_NAME=../$(EXE_NAME)sv$(BITS)$(EXEPOSTFIX)
	SV_LDFLAGS=-lws2_32 -lwinmm -lole32
	SV_DIR=sv_mingw$(BITS)
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(WINDOWSSERVERONLY_OBJS) fs_win32.o $(LTO_END) resources.o $(LTO_START)
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS)


	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidnt.o $(WINDOWS_OBJS)
	GL_EXE_NAME=../fteglqw$(BITS)$(EXEPOSTFIX)
	GLCL_EXE_NAME=../fteglqwcl$(BITS)$(EXEPOSTFIX)
	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -Wl,--subsystem,windows
	GL_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(DX7SDK) -DMULTITHREAD $(CLIENTLIBFLAGS)
	GLB_DIR=gl_mgw$(BITS)
	GLCL_DIR=glcl_mgw$(BITS)

	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) $(D3DQUAKE_OBJS) gl_vidnt.o gl_videgl.o $(WINDOWS_OBJS)
	M_EXE_NAME=../$(EXE_NAME)$(BITS)$(EXEPOSTFIX)
	MCL_EXE_NAME=../$(EXE_NAME)cl$(BITS)$(EXEPOSTFIX)
	M_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -Wl,--subsystem,windows
	M_CFLAGS=$(GLCFLAGS) $(W32_CFLAGS) $(D3DCFLAGS) $(DX7SDK) $(VKCFLAGS) -DMULTITHREAD $(CLIENTLIBFLAGS)
	MB_DIR=m_mgw$(BITS)
	MCL_DIR=mcl_mgw$(BITS)

	D3DCL_OBJS=$(D3DQUAKE_OBJS) $(D3DGL_OBJS) $(WINDOWS_OBJS)
	D3D_EXE_NAME=../fted3dqw$(BITS)$(EXEPOSTFIX)
	D3DCL_EXE_NAME=../fted3dclqw$(BITS)$(EXEPOSTFIX)
	D3D_LDFLAGS=$(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -Wl,--subsystem,windows
	D3D_CFLAGS=$(D3DCFLAGS) $(W32_CFLAGS) $(DX7SDK) -DMULTITHREAD $(CLIENTLIBFLAGS)
	D3DB_DIR=d3d_mgw$(BITS)
	D3DCL_DIR=d3dcl_mgw$(BITS)

	VKCL_OBJS=$(GLQUAKE_OBJS) $(D3DGL_OBJS) $(WINDOWS_OBJS) gl_vidnt.o
	VK_EXE_NAME=../ftevkqw$(BITS)$(EXEPOSTFIX)
	VKCL_EXE_NAME=../ftevkclqw$(BITS)$(EXEPOSTFIX)
	VK_LDFLAGS=$(IMAGELDFLAGS) -ldxguid -lws2_32 -lwinmm -lgdi32 -lole32 -Wl,--subsystem,windows
	VK_CFLAGS=$(VKCFLAGS) $(W32_CFLAGS) $(DX7SDK) -DMULTITHREAD $(CLIENTLIBFLAGS)
	VKB_DIR=vk_mgw$(BITS)
	VKCL_DIR=vkcl_mgw$(BITS)

	MINGL_EXE_NAME=../fteminglqw$(BITS)$(EXEPOSTFIX)
	MINGL_DIR=mingl_mgw$(BITS)

	ifeq (,$(findstring NO_ZLIB,$(CFLAGS)))
		SV_LDFLAGS+=-lz
		GL_LDFLAGS+=-lz
		VK_LDFLAGS+=-lz
		M_LDFLAGS+=-lz
		QCC_LDFLAGS+=-L$(ARCHLIBS) -lz
	endif
	ifeq ($(NOCOMPAT),1)
		SV_EXE_NAME=../engine-sv$(BITS)$(EXEPOSTFIX)
		GL_EXE_NAME=../engine-gl$(BITS)$(EXEPOSTFIX)
		VK_EXE_NAME=../engine-vk$(BITS)$(EXEPOSTFIX)
		M_EXE_NAME=../engine$(BITS)$(EXEPOSTFIX)
		D3D_EXE_NAME=../engine-d3d$(BITS)$(EXEPOSTFIX)
		MINGL_EXE_NAME=../engine-mingl$(BITS)$(EXEPOSTFIX)
	endif
endif

ifeq ($(FTE_TARGET),bsd)
	#mostly uses the linux stuff.
	#oss, X, etc.
	CC=cc
	CXX=c++
	SV_DIR=sv_bsd
	SV_EXE_NAME=../$(EXE_NAME)-sv$(BITS)
	SV_LDFLAGS=-lpthread
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) -DMULTITHREAD

	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o sys_linux_threads.o
	GL_EXE_NAME=../$(EXE_NAME)-gl
	GLCL_EXE_NAME=../$(EXE_NAME)-glcl
	GL_LDFLAGS= -L/usr/local/lib $(GLLDFLAGS) $(XLDFLAGS) -lpthread
	GL_CFLAGS=$(GLCFLAGS) -I/usr/local/include -I/usr/X11R6/include -I/usr/X11R6/include/freetype2 -DMULTITHREAD -DAUDIO_OSS
	GLB_DIR=gl_bsd
	GLCL_DIR=glcl_bsd

	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o sys_linux_threads.o
	M_EXE_NAME=../$(EXE_NAME)
	MCL_EXE_NAME=../$(EXE_NAME)-cl
	M_LDFLAGS= -L/usr/local/lib -L/usr/X11R6/lib $(GLLDFLAGS) $(XLDFLAGS) -lpthread
	M_CFLAGS=$(VKCFLAGS) $(GLCFLAGS) -I/usr/local/include -I/usr/X11R6/include -I/usr/X11R6/include/freetype2 -DMULTITHREAD -DAUDIO_OSS
	MB_DIR=m_bsd
	MCL_DIR=mcl_bsd

	MINGL_EXE_NAME=../$(EXE_NAME)-mingl
	MINGL_DIR=mingl_bsd

	#openbsd has a special library for oss emulation.
	ifeq ($(shell uname -s),OpenBSD)
		GL_LDFLAGS+= -lossaudio -lfreetype
		VK_LDFLAGS+= -lossaudio -lfreetype
		M_LDFLAGS+= -lossaudio -lfreetype
		M_CFLAGS+= -DFREETYPE_STATIC
		VK_CFLAGS+= -DFREETYPE_STATIC
		GL_CFLAGS+= -DFREETYPE_STATIC
	endif

	ifeq (,$(findstring NO_ZLIB,$(CFLAGS)))
		SV_LDFLAGS+= -lz
		GL_LDFLAGS+= -lz
		VK_LDFLAGS+= -lz
		M_LDFLAGS+= -lz
	endif
endif
ifneq (,$(findstring linux,$(FTE_TARGET)))
	SV_DIR=sv_linux$(BITS)
	SV_EXE_NAME=../$(EXE_NAME)-sv$(BITS)
	SV_LDFLAGS=
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) -DMULTITHREAD

	ifneq ("$(wildcard /usr/include/wayland-client.h)","")
		HAVE_WAYLAND=-DWAYLANDQUAKE
	else
		HAVE_WAYLAND=
	endif
	ifneq ("$(wildcard /usr/include/EGL/egl.h)","")
		HAVE_EGL=-DUSE_EGL
	else
		HAVE_EGL=
	endif

	CL_CFLAGS=-DMULTITHREAD -DDYNAMIC_SDL $(HAVE_EGL) $(HAVE_WAYLAND) -DX11QUAKE -DAUDIO_PULSE -DAUDIO_ALSA -DAUDIO_OSS -DAUDIO_SDL
	BASELDFLAGS+=-Wl,--warn-common
	
	ifeq ($(BITS),32)
		CL_CFLAGS+=-D_LARGEFILE64_SOURCE
		SV_CFLAGS+=-D_LARGEFILE64_SOURCE
	endif
	
	QCC_DIR=linqcc$(BITS)

	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o gl_vidwayland.o gl_videgl.o snd_pulse.o snd_alsa.o snd_linux.o snd_sdl.o cd_linux.o sys_linux.o sys_linux_threads.o
	GL_EXE_NAME=../$(EXE_NAME)-gl$(BITS)
	GLCL_EXE_NAME=../$(EXE_NAME)-glcl$(BITS)
	GL_LDFLAGS=$(GLLDFLAGS) $(XLDFLAGS)
	GL_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include $(CL_CFLAGS) $(CLIENTLIBFLAGS)
	GLB_DIR=gl_linux$(BITS)
	GLCL_DIR=glcl_linux$(BITS)

	VKCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o gl_vidwayland.o gl_videgl.o snd_pulse.o snd_alsa.o snd_linux.o snd_sdl.o cd_linux.o sys_linux.o sys_linux_threads.o
	VK_EXE_NAME=../$(EXE_NAME)-vk$(BITS)
	VKCL_EXE_NAME=../$(EXE_NAME)-vkcl$(BITS)
	VK_LDFLAGS=$(GLLDFLAGS) $(XLDFLAGS)
	VK_CFLAGS=$(VKCFLAGS) -I/usr/X11R6/include $(CL_CFLAGS) $(CLIENTLIBFLAGS)
	VKB_DIR=vk_linux$(BITS)
	VKCL_DIR=vkcl_linux$(BITS)

	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o gl_vidwayland.o gl_videgl.o snd_linux.o snd_sdl.o snd_pulse.o snd_alsa.o cd_linux.o sys_linux.o sys_linux_threads.o
	M_EXE_NAME=../$(EXE_NAME)$(BITS)
	MCL_EXE_NAME=../$(EXE_NAME)-cl$(BITS)
	M_LDFLAGS=$(GL_LDFLAGS)
	M_CFLAGS=$(VKCFLAGS) $(GL_CFLAGS) $(CLIENTLIBFLAGS)
	MB_DIR=m_linux$(BITS)
	MCL_DIR=mcl_linux$(BITS)

	ifeq (,$(findstring NO_ZLIB,$(CFLAGS)))
		SV_LDFLAGS+= -lz
		GL_LDFLAGS+= -lz
		VK_LDFLAGS+= -lz
		M_LDFLAGS+= -lz
	endif


	MINGL_EXE_NAME=../$(EXE_NAME)-mingl$(BITS)
	MINGL_DIR=mingl_linux$(BITS)

	ifeq ($(NOCOMPAT),1)
		SV_EXE_NAME=../engine-sv$(BITS)$(EXEPOSTFIX)
		GL_EXE_NAME=../engine-gl$(BITS)$(EXEPOSTFIX)
		VK_EXE_NAME=../engine-vk$(BITS)$(EXEPOSTFIX)
		M_EXE_NAME=../engine$(BITS)$(EXEPOSTFIX)
		D3D_EXE_NAME=../engine-d3d$(BITS)$(EXEPOSTFIX)
		MINGL_EXE_NAME=../engine-mingl$(BITS)$(EXEPOSTFIX)
	endif
endif
ifneq (,$(findstring rpi,$(FTE_TARGET)))
	#These next two lines enable cross compiling. If you're compiling natively you can just kill the two.
	RPI_SYSROOT:=$(realpath $(shell echo ~)/rpi/rpi-sysroot/)
	CC=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-gcc --sysroot=$(RPI_SYSROOT)
	CXX=~/rpi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-g++ --sysroot=$(RPI_SYSROOT)
	SDLCONFIG=$(RPI_SYSROOT)/usr/bin/sdl-config --prefix=$(RPI_SYSROOT)/usr
	GL_CFLAGS+= -I$(RPI_SYSROOT)/opt/vc/include -I$(RPI_SYSROOT)/opt/vc/include/interface/vmcs_host/linux -I$(RPI_SYSROOT)/opt/vc/include/interface/vcos/pthreads -DFTE_RPI -DUSE_EGL
	GL_LDFLAGS+= -L$(RPI_SYSROOT)/opt/vc/lib -Wl,--sysroot=$(RPI_SYSROOT),-rpath=/opt/vc/lib,-rpath-link=$(RPI_SYSROOT)/opt/vc/lib -lbcm_host
	GLCL_OBJS+=gl_vidrpi.o
endif
ifneq (,$(findstring fbdev,$(FTE_TARGET)))
	GL_CFLAGS+=-DUSE_EGL
	GLCL_OBJS+=gl_vidfbdev.o
	MCL_OBJS+=gl_vidfbdev.o
endif
ifneq ($(shell echo $(FTE_TARGET)|grep macosx),)
	SV_DIR=sv_macosx$(EXTENSION)$(BITS)
	GLB_DIR=gl_macosx$(EXTENSION)$(BITS)
	GLCL_DIR=glcl_macosx$(EXTENSION)$(BITS)
	MINGL_DIR=mingl_macosx$(EXTENSION)$(BITS)
	
	GL_CFLAGS=$(GLCFLAGS) -D__MACOSX__ -L/sw/lib -I/sw/include -L/opt/local/lib -I/opt/local/include -I$(LIBS_DIR)	
	ifeq ($(FTE_TARGET),macosx_x86)
		GL_CFLAGS=$(GLCFLAGS) -D__MACOSX__ -L/sw/lib -I/sw/include -L/opt/local/lib -I/opt/local/include -I$(LIBS_DIR)
	endif

	GL_LDFLAGS=-framework AGL -framework OpenGL -framework Cocoa -framework AudioUnit
	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidcocoa.mo gl_vidmacos.o sys_linux.o cd_null.o snd_macos.o sys_linux_threads.o

	GL_EXE_NAME=../$(EXE_NAME)-macosx-gl$(EXTENSION)$(BITS)
	GLCL_EXE_NAME=../$(EXE_NAME)cl-macosx-gl$(EXTENSION)$(BITS)
	M_EXE_NAME=../$(EXE_NAME)-macosx$(EXTENSION)$(BITS)
	MCL_EXE_NAME=../$(EXE_NAME)-macosx-cl$(EXTENSION)$(BITS)
	MINGL_EXE_NAME=../$(EXE_NAME)-macosx-mingl$(EXTENSION)$(BITS)
	MINGL_DIR=mingl_macosx$(EXTENSION)$(BITS)
	
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
	SV_EXE_NAME=../$(EXE_NAME)-macosx-sv$(EXTENSION)$(BITS)
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)
	SV_LDFLAGS=-lz	

	#seems, macosx has a more limited version of strip
	STRIPFLAGS=
endif
ifeq ($(FTE_TARGET),morphos)
	#-Wno-pointer-sign unrecognised 
	WARNINGFLAGS=-Wall

	CFLAGS+=-D__MORPHOS_SHAREDLIBS

	SV_DIR=sv_morphos
	SV_LDFLAGS=-ldl -lz

	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidmorphos.o in_morphos.o snd_morphos.o cd_null.o sys_morphos.o
	GL_EXE_NAME=../$(EXE_NAME)-morphos-gl
	GLCL_EXE_NAME=../$(EXE_NAME)-morphos-glcl
	GL_LDFLAGS=$(GLLDFLAGS) -ldl $(IMAGELDFLAGS) -lz
	GL_CFLAGS=$(GLCFLAGS) -noixemul -I./
	GLB_DIR=gl_morphos
	GLCL_DIR=glcl_morphos

	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidmorphos.o vid_morphos.o in_morphos.o snd_morphos.o cd_null.o sys_morphos.o
	M_EXE_NAME=../$(EXE_NAME)-morphos
	MCL_EXE_NAME=../$(EXE_NAME)-morphos-cl
	M_LDFLAGS=$(GLLDFLAGS)
	M_CFLAGS=$(GLCFLAGS)
	MB_DIR=m_morphos
	MCL_DIR=mcl_morphos

	MINGL_EXE_NAME=../$(EXE_NAME)-morphos-mingl
	MINGL_DIR=mingl_morphos
	
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SERVERONLY_OBJS)
	SV_EXE_NAME=../$(EXE_NAME)-morphos-sv$(BITS)
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)	
endif

ifeq ($(FTE_TARGET),dos)
	EXEPOSTFIX=.exe
	SV_DIR=sv_dos
	GLB_DIR=gl_dos
	MB_DIR=m_dos
	MCL_DIR=mcl_dos
	MINGL_DIR=mingl_dos
	VKB_DIR=vk_dos
	VKCL_DIR=vkcl_dos

	IMAGELDFLAGS=

	SOFTWARE_OBJS=sw_rast.o sw_backend.o sw_image.o

	M_LDFLAGS=
	M_CFLAGS=-DSWQUAKE -DNO_ZLIB
	MCL_OBJS=$(SOFTWARE_OBJS) $(D3DGL_OBJS) sw_viddos.o cd_null.o sys_dos.o snd_sblaster.o
	M_EXE_NAME=../$(EXE_NAME)$(EXEPOSTFIX)
	SV_EXE_NAME=../$(EXE_NAME)sv$(BITS)$(EXEPOSTFIX)
	VK_EXE_NAME=../$(EXE_NAME)-vk$(BITS)$(EXEPOSTFIX)

	VKCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) cd_null.o sys_dos.o snd_sblaster.o
endif

ifeq ($(FTE_TARGET),cyg)
	SV_DIR=sv_cygwin
	SV_LDFLAGS=-lz
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)

	EXEPOSTFIX=.exe
	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o sys_linux_threads.o
	GL_EXE_NAME=../$(EXE_NAME)-cyg-gl$(EXEPOSTFIX)
	GLCL_EXE_NAME=../$(EXE_NAME)-cyg-glcl$(EXEPOSTFIX)
	GL_LDFLAGS=$(GLLDFLAGS) $(XLDFLAGS) -lz -lltdl
	GL_CFLAGS=$(GLCFLAGS) -I/usr/X11R6/include $(CLIENTLIBFLAGS) -DUSE_LIBTOOL -DAUDIO_OSS
	GLB_DIR=gl_cygwin
	GLCL_DIR=glcl_cygwin

	MCL_OBJS=$(D3DGL_OBJS) $(GLQUAKE_OBJS) $(SOFTWARE_OBJS) gl_vidlinuxglx.o snd_linux.o cd_null.o sys_linux.o sys_linux_threads.o
	M_EXE_NAME=../$(EXE_NAME)-cyg$(EXEPOSTFIX)
	MCL_EXE_NAME=../$(EXE_NAME)-cyg-cl$(EXEPOSTFIX)
	M_LDFLAGS=$(GLLDFLAGS) $(XLDFLAGS) -lz -lltdl
	M_CFLAGS=$(GLCFLAGS) $(CLIENTLIBFLAGS) -DUSE_LIBTOOL -DAUDIO_OSS
	MB_DIR=m_cygwin
	MCL_DIR=mcl_cygwin

	MINGL_EXE_NAME=../$(EXE_NAME)-cyg-mingl$(EXEPOSTFIX)
	MINGL_DIR=mingl_cygwin
endif

ifeq ($(FTE_TARGET),droid)
	BASELDFLAGS=-lz
	
	#erk! FIXME!
	CLIENTLDDEPS=
	SERVERLDDEPS=

	SYS_DROID_O=sys_droid.o sys_linux_threads.o
	GL_DROID_O=gl_viddroid.o $(SYS_DROID_O)

	SV_CFLAGS=$(SERVER_ONLY_CFLAGS) $(W32_CFLAGS)
	SV_LDFLAGS=
	SV_DIR=sv_droid-$(DROID_ARCH)
	SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS) $(SYS_DROID_O)
	SV_EXE_NAME=libftedroid.so

	GL_CFLAGS=$(GLCFLAGS)
	GL_LDFLAGS=$(GLLDFLAGS) -landroid
	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) $(GL_DROID_O) cd_null.o snd_droid.o
	GLB_DIR=gl_droid-$(DROID_ARCH)
	GL_EXE_NAME=libftedroid.so	

	M_CFLAGS=$(VKCFLAGS) $(GLCFLAGS) -DMULTITHREAD
	M_LDFLAGS=$(GLLDFLAGS) -landroid -lEGL -lOpenSLES
	MCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) $(GL_DROID_O) cd_null.o snd_opensl.o
#snd_droid.o
	MB_DIR=m_droid-$(DROID_ARCH)
	M_EXE_NAME=libftedroid.so
endif

ifeq ($(FTE_TARGET),web)
	COMMON_OBJS+=sys_web.o fs_web.o
	WEB_PREJS ?= --pre-js web/prejs.js
#	WEB_MEMORY?=402653184	#384mb
#	ASMJS_MEMORY?=16777216	#16mb
#	ASMJS_MEMORY?=33554432	#32mb
	ASMJS_MEMORY?=268435456	#256mb
#	ASMJS_MEMORY?=536870912	#512mb
#	ASMJS_MEMORY?=1073741824 #1025mb
#	ASMJS_MEMORY?=2147483648 #2048mb
	WEB_MEMORY?=$(ASMJS_MEMORY)
	JSLIBS=--js-library web/ftejslib.js
	EMCC_CFLAGS= -DFTE_TARGET_WEB
	EMCC_LDFLAGS=$(EMCC_CFLAGS) $(JSLIBS) $(WEB_PREJS)
#	EMCC_CFLAGS+= -s BINARYEN_TRAP_MODE='clamp'		#fix bigfloat->int rounding crashes
	EMCC_CFLAGS+= -s LEGACY_GL_EMULATION=0			#simplify the opengl wrappers.
	EMCC_CFLAGS+= -s NO_FILESYSTEM=1				#we have our own.
	EMCC_CFLAGS+= -s FILESYSTEM=0					#we have our own.
	EMCC_CFLAGS+= -s ALLOW_MEMORY_GROWTH=1			#reduce crashes...
	EMCC_CFLAGS+= -s MAX_WEBGL_VERSION=2			#make use of what we can.
   	EMCC_LDFLAGS+=-s ERROR_ON_UNDEFINED_SYMBOLS=1	#fairly obvious. no runtime errors please.
	RELEASE_CFLAGS=-DOMIT_QCC -DGL_STATIC $(EMCC_CFLAGS)
	DEBUG_CFLAGS=-g4 -DOMIT_QCC -DGL_STATIC $(EMCC_CFLAGS)
	RELEASE_LDFLAGS=-O3	-s TOTAL_MEMORY=$(ASMJS_MEMORY) $(EMCC_LDFLAGS)
#	RELEASE_LDFLAGS=-O1	-s TOTAL_MEMORY=$(WEB_MEMORY) $(EMCC_LDFLAGS)
	DEBUG_LDFLAGS=-O0 -g4 -s TOTAL_MEMORY=$(WEB_MEMORY) $(EMCC_LDFLAGS) -s SAFE_HEAP=1 -s ALIASING_FUNCTION_POINTERS=0 -s ASSERTIONS=2
	CC=emcc
	CXX=em++
	AR=emar
	BASELDFLAGS=-lz
	PRECOMPHEADERS=

	#mostly we inherit the sdl defaults. because we can, however emscripten does not support sdl cd code.
	GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(BOTLIB_OBJS) $(GLQUAKE_OBJS) gl_vidweb.o cd_null.o
	SDL_INCLUDES=

	SV_DIR=sv_web
	SV_LDFLAGS=
	#SV_OBJS=$(COMMON_OBJS) $(SERVER_OBJS) $(PROGS_OBJS)
	SV_EXE_NAME=../libftesv.js
	SV_CFLAGS=$(SERVER_ONLY_CFLAGS)

	#SV_LDFLAGS=

	STRIP=@echo SKIP: strip
	#GLCL_OBJS=$(GL_OBJS) $(D3DGL_OBJS) $(GLQUAKE_OBJS) cd_null.o
	#GL_LDFLAGS=$(GLLDFLAGS)
	GLB_DIR=gl_web
	GLCL_DIR=glcl_web
	GL_EXE_NAME=../ftewebgl.js
	GLCL_EXE_NAME=../ftewebglcl.js

	GL_LDFLAGS=$(GLLDFLAGS) $(IMAGELDFLAGS)
	GL_CFLAGS=$(GLCFLAGS) $(CLIENTLIBFLAGS)

	IMAGELDFLAGS=
	SERVERLDDEPS=

	#generate deps properly
	#DEPCC=
	#DEPCXX=
endif

SV_DIR?=sv_sdl
DEPCC?=$(CC)
DEPCXX?=$(CXX)
ARCH:=$(ARCH)
ifeq ($(findstring msvc,$(FTE_TARGET)),msvc)
	BASELDFLAGS:=/libpath:"$(ARCHLIBS)" $(BASELDFLAGS)
else
	BASELDFLAGS:=-L$(ARCHLIBS) $(BASELDFLAGS)
endif

default: help
-include Makefile_private

all: rel
rel: sv-rel m-rel qcc-rel
dbg: sv-dbg m-dbg qcc-dbg
relcl: glcl-rel mcl-rel
profile: sv-profile gl-profile mingl-profile

releases:
	#this is for releasing things from a linux box
	#just go through compiling absolutly everything
	-$(MAKE) FTE_TARGET=linux32 rel
	-$(MAKE) FTE_TARGET=linux64 rel
	-$(MAKE) FTE_TARGET=win32 rel
	-$(MAKE) FTE_TARGET=win64 rel
	-$(MAKE) FTE_TARGET=win32_SDL rel
	-$(MAKE) FTE_TARGET=win64_SDL rel
	-$(MAKE) FTE_TARGET=morphos rel
	-$(MAKE) FTE_TARGET=macosx rel
#	-$(MAKE) FTE_TARGET=linux32 relcl
#	-$(MAKE) FTE_TARGET=linux64 relcl
#	-$(MAKE) FTE_TARGET=win32 relcl
	-$(MAKE) droid-rel
	-$(MAKE) web-rel

autoconfig: clean
	/bin/bash makeconfig.sh y

config: clean
	/bin/bash makeconfig.sh

ifneq ($(OUT_DIR),)
-include $(OUT_DIR)/*.o.d
endif


DO_WINDRES?=$(DO_ECHO) $(WINDRES) $(BRANDFLAGS) -I$(CLIENT_DIR) -O coff $< $@

# This is for linking the FTE icon to the MinGW target
$(OUT_DIR)/resources.o : winquake.rc
	$(DO_WINDRES)
$(OUT_DIR)/fteqcc.o : fteqcc.rc
	$(DO_WINDRES)


#$(OUT_DIR)/%.d: %.c
#	@set -e; rm -f $@; \
#	$(CC) -MM $(ALL_CFLAGS) $< > $@.$$$$; \
#	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
#	rm -f $@.$$$$

$(OUT_DIR)/%.o $(OUT_DIR)/%.d : %.c
ifneq ($(DEPCC),)
	@-set -e; rm -f $@.d; \
	 $(DEPCC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.o[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
endif
	$(DO_CC) -I$(OUT_DIR)

$(OUT_DIR)/%.o $(OUT_DIR)/%.d : %.cpp
ifneq ($(DEPCXX),)
	@-set -e; rm -f $@.d; \
	$(DEPCXX) -MM $(ALL_CXXFLAGS) $< > $@.d.$$$$; \
	sed 's,\($*\)\.o[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	rm -f $@.d.$$$$
endif
	$(DO_CXX) -I$(OUT_DIR)

$(OUT_DIR)/%.o $(OUT_DIR)/%.d : %.cxx
ifneq ($(DEPCXX),)
	@-set -e; rm -f $@.d; \
	$(DEPCXX) -MM $(ALL_CXXFLAGS) $< > $@.d.$$$$; \
	sed 's,\($*\)\.o[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	rm -f $@.d.$$$$
endif
	$(DO_CXX) -I$(OUT_DIR)

$(OUT_DIR)/%.oo $(OUT_DIR)/%.d : %.c
ifneq ($(DEPCC),)
	@-set -e; rm -f $@.d; \
	 $(DEPCC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.oo[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
endif
	$(DO_CC) -I$(OUT_DIR)

$(OUT_DIR)/%.mo $(OUT_DIR)/%.d : %.m
	@-set -e; rm -f $@.d; \
	 $(DEPCC) -MM $(ALL_CFLAGS) $< > $@.d.$$$$; \
	 sed 's,\($*\)\.mo[ :]*,$@ $@.d : ,g' < $@.d.$$$$ > $@.d; \
	 sed -e 's/.*://' -e 's/\\$$//' < $@.d.$$$$ | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $@.d; \
	 rm -f $@.d.$$$$
	$(DO_CC) -I$(OUT_DIR)

#enables use of precompiled headers in gcc 3.4 onwards.
$(OUT_DIR)/quakedef.h.gch : quakedef.h
	$(CC) -x c-header $(ALL_CFLAGS) -o $@ -c $<
PRECOMPHEADERS ?= $(OUT_DIR)/quakedef.h.gch

ifneq ($(OUT_DIR),)
ALL_CFLAGS+=-I$(OUT_DIR)
endif

#addprefix is to add the ./release/server/ part of the object name
#foreach is needed as the OBJS is a list of variable names containing object lists.
#which is needed as windows sucks too much for the chaining to carry a full list.
#god knows how gcc loads the list properly.
#or at least I hope he does. It makes no sence to mortals.

LDCC ?=$(CC)
DO_LD ?= +$(DO_ECHO) $(LDCC) -o $@ $(LTO_LD) $(WCFLAGS) $(BRANDFLAGS) $(CFLAGS)
$(OUT_DIR)/$(EXE_NAME):   $(PRECOMPHEADERS) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS), $($(ol))),$(if $(findstring ltox,$(fn)),,$(OUT_DIR)/$(fn)))
	$(DO_LD) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS) $(LTO_END), $($(ol))),$(if $(findstring ltox,$(fn)),$(subst ltox,-x ,$(fn)),$(NATIVE_OUT_DIR)/$(fn)) ) $(LDFLAGS)

$(OUT_DIR)/$(EXE_NAME).db: $(PRECOMPHEADERS) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS), $($(ol))),$(if $(findstring ltox,$(fn)),,$(OUT_DIR)/$(fn)))
	$(DO_LD) $(foreach fn, $(CUSTOMOBJS) $(foreach ol, $(OBJS) $(LTO_END), $($(ol))),$(if $(findstring ltox,$(fn)),$(subst ltox,-x ,$(fn)),$(NATIVE_OUT_DIR)/$(fn)) ) $(LDFLAGS)

ifeq (,$(findstring SKIP,$(STRIP)))
#link to a .db file
#then strip its debug data to the non-.db release binary
_out-rel: $(ARCH_PREDEP)
	@$(MAKE) $(OUT_DIR)/$(EXE_NAME).db EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(RELEASE_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(RELEASE_LDFLAGS)" OBJS="$(OBJS)"
	@$(STRIP) $(STRIPFLAGS) $(OUT_DIR)/$(EXE_NAME).db -o $(OUT_DIR)/$(EXE_NAME)
else
#STRIP macro won't work, don't do the .db thing and don't expect strip -o to work.
_out-rel: $(ARCH_PREDEP)
	@$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(RELEASE_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(RELEASE_LDFLAGS)" OBJS="$(OBJS)"
	@echo not stripping $(OUT_DIR)/$(EXE_NAME)
endif

_out-dbg: $(ARCH_PREDEP)
	@$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(DEBUG_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(DEBUG_LDFLAGS)" OBJS="$(OBJS)"

_out-profile: $(ARCH_PREDEP)
	@$(MAKE) $(OUT_DIR)/$(EXE_NAME) EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(WCFLAGS) $(PROFILE_CFLAGS)" LDFLAGS="$(BASELDFLAGS) $(LDFLAGS) $(PROFILE_LDFLAGS)" OBJS="$(OBJS)"

_cl-rel: reldir
	@$(MAKE) _out-rel EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_cl-dbg: debugdir
	@$(MAKE) _out-dbg EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_cl-profile: reldir
	@$(MAKE) _out-profile EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(CLIENT_ONLY_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS"

_clsv-rel: reldir
	$(DO_ECHO) $(MAKE) _out-rel EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

_clsv-dbg: debugdir
	@$(MAKE) _out-dbg EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

_clsv-profile: reldir
	@$(MAKE) _out-profile EXE_NAME="$(EXE_NAME)" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(JOINT_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS)" SOBJS="$(SOBJS)" OBJS="SOBJS COMMON_OBJS CLIENT_OBJS PROGS_OBJS SERVER_OBJS"

sv-tmp: reldir debugdir
	@$(MAKE) $(TYPE) OUT_DIR="$(OUT_DIR)" EXE_NAME="$(SV_EXE_NAME)" WCFLAGS="$(SV_CFLAGS)" LDFLAGS="$(ARCH_LDFLAGS) $(SV_LDFLAGS) $(LDFLAGS) $(SERVERLDDEPS)" OBJS="SV_OBJS"
sv-rel:
	@$(MAKE) sv-tmp TYPE=_out-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(SV_DIR)"
sv-dbg:
	@$(MAKE) sv-tmp TYPE=_out-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(SV_DIR)"
sv-profile:
	@$(MAKE) sv-tmp TYPE=_out-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(SV_DIR)"

d3dcl-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(D3DCL_EXE_NAME)" WCFLAGS="$(D3D_CFLAGS)" LDFLAGS="$(D3D_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(D3DCL_OBJS)"
d3d-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(D3D_EXE_NAME)" WCFLAGS="$(D3D_CFLAGS)" LDFLAGS="$(D3D_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(D3DCL_OBJS)"

d3dcl-rel:
	@$(MAKE) d3dcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(D3DCL_DIR)"
d3dcl-dbg:
	@$(MAKE) d3dcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(D3DCL_DIR)"
d3dcl-profile:
	@$(MAKE) d3dcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(D3DCL_DIR)"

d3d-rel:
	@$(MAKE) d3d-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(D3DB_DIR)"
d3d-dbg:
	@$(MAKE) d3d-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(D3DB_DIR)"
d3d-profile:
	@$(MAKE) d3d-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(D3DB_DIR)"



vkcl-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(VKCL_EXE_NAME)" WCFLAGS="$(VK_CFLAGS)" LDFLAGS="$(VK_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(VKCL_OBJS)"
vk-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(VK_EXE_NAME)" WCFLAGS="$(VK_CFLAGS)" LDFLAGS="$(VK_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(VKCL_OBJS)"

vkcl-rel:
	@$(MAKE) vkcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(VKCL_DIR)"
vkcl-dbg:
	@$(MAKE) vkcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(VKCL_DIR)"
vkcl-profile:
	@$(MAKE) vkcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(VKCL_DIR)"

vk-rel:
	@$(MAKE) vk-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(VKB_DIR)"
vk-dbg:
	@$(MAKE) vk-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(VKB_DIR)"
vk-profile:
	@$(MAKE) vk-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(VKB_DIR)"


glcl-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(GLCL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS)" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(GLCL_OBJS)"
gl-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(GL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS)" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(GLCL_OBJS)"

glcl-rel:
	@$(MAKE) glcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(GLCL_DIR)"
glcl-dbg:
	@$(MAKE) glcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(GLCL_DIR)"
glcl-profile:
	@$(MAKE) glcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(GLCL_DIR)"
gl-rel:
	@$(MAKE) gl-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(GLB_DIR)"
gl-dbg:
	@$(MAKE) gl-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(GLB_DIR)"
gl-profile:
	@$(MAKE) gl-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(GLB_DIR)"

mingl-tmp: reldir
	@$(MAKE) $(TYPE) OUT_DIR="$(OUT_DIR)" EXE_NAME="$(MINGL_EXE_NAME)" WCFLAGS="$(GL_CFLAGS) -DMINIMAL" LDFLAGS="$(GL_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(GLCL_OBJS)"
mingl-rel:
	@$(MAKE) mingl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(MINGL_DIR)"
mingl-dbg:
	@$(MAKE) mingl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(MINGL_DIR)"
mingl-profile:
	@$(MAKE) mingl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(MINGL_DIR)"

mcl-tmp:
	@$(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(MCL_EXE_NAME)" WCFLAGS="$(M_CFLAGS)" LDFLAGS="$(M_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(MCL_OBJS)"
m-tmp:
	$(DO_ECHO) $(MAKE) $(TYPE)   OUT_DIR="$(OUT_DIR)" EXE_NAME="$(M_EXE_NAME)" WCFLAGS="$(M_CFLAGS)" LDFLAGS="$(M_LDFLAGS) $(LDFLAGS) $(CLIENTLDDEPS)" SOBJS="$(MCL_OBJS)"

mcl-rel:
	@$(MAKE) mcl-tmp TYPE=_cl-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(MCL_DIR)"
mcl-dbg:
	@$(MAKE) mcl-tmp TYPE=_cl-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(MCL_DIR)"
mcl-profile:
	@$(MAKE) mcl-tmp TYPE=_cl-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(MCL_DIR)"
m-rel:
	$(DO_ECHO) $(MAKE) m-tmp TYPE=_clsv-rel OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(MB_DIR)"
m-dbg:
	@$(MAKE) m-tmp TYPE=_clsv-dbg OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(MB_DIR)"
m-profile:
	@$(MAKE) m-tmp TYPE=_clsv-profile OUT_DIR="$(PROFILE_DIR)/$(NCDIRPREFIX)$(MB_DIR)"

.PHONY: m-tmp mcl-tmp mingl-tmp glcl-tmp gl-tmp sv-tmp _clsv-dbg _clsv-rel _cl-dbg _cl-rel _out-rel _out-dbg reldir debugdir makelibs wel-rel web-dbg httpserver iqmtool imgtool


_qcc-tmp: $(REQDIR)
	@$(MAKE) $(TYPE) EXE_NAME="$(EXE_NAME)$(EXEPOSTFIX)" PRECOMPHEADERS="" OUT_DIR="$(OUT_DIR)" WCFLAGS="$(QCC_CFLAGS) $(WCFLAGS)" LDFLAGS="$(LDFLAGS) $(QCC_LDFLAGS)" OBJS="QCC_OBJS SOBJS"
qcc-rel:
	@$(MAKE) _qcc-tmp TYPE=_out-rel REQDIR=reldir EXE_NAME="../fteqcc$(BITS)" OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(QCC_DIR)" SOBJS="qcctui.o packager.o decomp.o $(if $(findstring win,$(FTE_TARGET)),fteqcc.o)"
qccgui-rel:
	@$(MAKE) _qcc-tmp TYPE=_out-rel REQDIR=reldir EXE_NAME="../fteqccgui$(BITS)" LTO= OUT_DIR="$(RELEASE_DIR)/$(NCDIRPREFIX)$(QCC_DIR)gui" SOBJS="qccgui.o qccguistuff.o packager.o decomp.o fteqcc.o" LDFLAGS="$(LDFLAGS) -lole32 -lcomdlg32 -lcomctl32 -lshlwapi -mwindows"
qcc-dbg:
	@$(MAKE) _qcc-tmp TYPE=_out-dbg REQDIR=debugdir EXE_NAME="../fteqcc$(BITS)" OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(QCC_DIR)" SOBJS="qcctui.o packager.o decomp.o $(if $(findstring win,$(FTE_TARGET)),fteqcc.o)"
qccgui-dbg:
	@$(MAKE) _qcc-tmp TYPE=_out-dbg REQDIR=debugdir EXE_NAME="../fteqccgui$(BITS)" LTO= OUT_DIR="$(DEBUG_DIR)/$(NCDIRPREFIX)$(QCC_DIR)gui" SOBJS="qccgui.o qccguistuff.o packager.o decomp.o fteqcc.o" LDFLAGS="$(LDFLAGS) -lole32 -lcomdlg32 -lcomctl32 -lshlwapi -mwindows"


#scintilla is messy as fuck when building statically. but at least we can strip out the lexers we don't use this way.
#note that this is only used in the 'qccgui-scintilla' target.
SCINTILLA_FILES= AutoComplete.o CallTip.o CaseConvert.o CaseFolder.o CellBuffer.o CharacterCategory.o CharacterSet.o CharClassify.o ContractionState.o Decoration.o Document.o EditModel.o Editor.o EditView.o KeyMap.o Indicator.o LineMarker.o MarginView.o PerLine.o PlatWin.o PositionCache.o PropSetSimple.o RESearch.o RunStyles.o Selection.o Style.o UniConversion.o ViewStyle.o XPM.o ScintillaWin.o HanjaDic.o ScintillaBase.o Accessor.o Catalogue.o ExternalLexer.o LexerBase.o LexerModule.o LexerSimple.o StyleContext.o WordList.o LexCPP.o
SCINTILLA_ROOT=$(BASE_DIR)/scintilla$(SCINTILLAVER)/scintilla
SCINTILLA_DIRS=$(SCINTILLA_ROOT)/lexers:$(SCINTILLA_ROOT)/lexlib:$(SCINTILLA_ROOT)/src:$(SCINTILLA_ROOT)/win32
SCINTILLA_INC=-I$(SCINTILLA_ROOT)/include -I$(SCINTILLA_ROOT)/lexlib -I$(SCINTILLA_ROOT)/win32 -I$(SCINTILLA_ROOT)/src
$(RELEASE_DIR)/scintilla$(BITS).a: $(foreach f,$(SCINTILLA_FILES),$(OUT_DIR)/$(f))
	@$(AR) -r $@ $?
	@$(AR) -s $@
scintilla$(BITS)_static:
	@test -f scintilla$(SCINTILLAVER).tar.gz || wget http://prdownloads.sourceforge.net/scintilla/scintilla$(SCINTILLAVER).tgz?download -O scintilla$(SCINTILLAVER).tar.gz
	@-test -f $(SCINTILLA_ROOT) || (mkdir $(BASE_DIR)/scintilla$(SCINTILLAVER) && cd $(BASE_DIR)/scintilla$(SCINTILLAVER) && tar -xvzf ../scintilla$(SCINTILLAVER).tar.gz && cd scintilla && mv lexers/LexCPP.cxx . && rm lexers/Lex*.cxx && mv LexCPP.cxx lexers/ && cd scripts && python LexGen.py)
	@$(MAKE) reldir OUT_DIR=$(RELEASE_DIR)/$(QCC_DIR)scin
	@$(MAKE) $(RELEASE_DIR)/scintilla$(BITS).a VPATH="$(SCINTILLA_DIRS)" CFLAGS="$(SCINTILLA_INC) -DDISABLE_D2D -DSTATIC_BUILD -DSCI_LEXER -std=c++11" OUT_DIR=$(RELEASE_DIR)/$(QCC_DIR)scin WCFLAGS="$(WCFLAGS) -Os" WARNINGFLAGS=
qccgui-scintilla: scintilla$(BITS)_static
	#LTO bugs out on WinMain for some reason, so try to disable it for this target.
	@LTO= $(MAKE) _qcc-tmp TYPE=_out-rel REQDIR=reldir EXE_NAME="../fteqccgui$(BITS)" OUT_DIR="$(RELEASE_DIR)/$(QCC_DIR)scin" SOBJS="qccgui.o qccguistuff.o packager.o decomp.o fteqcc.o" WCFLAGS="$(WCFLAGS) -DSCISTATIC" LDFLAGS="$(LDFLAGS) $(RELEASE_DIR)/scintilla$(BITS).a -static -luuid -lole32 -limm32 -lstdc++ -loleaut32 -lcomdlg32 -lcomctl32 -lshlwapi -mwindows"

ifdef windir
outdir:
ifneq "$(OUT_DIR)" ""
	@-mkdir -p "$(subst /,\, $(OUT_DIR))"
endif
debugdir: outdir
	@-mkdir -p "$(subst /,\, $(RELEASE_DIR))"
reldir: outdir
	@-mkdir -p "$(subst /,\, $(DEBUG_DIR))"
else
outdir:
ifneq "$(OUT_DIR)" ""
	@-mkdir -p "$(OUT_DIR)"
endif
debugdir: outdir
reldir: outdir
	@-mkdir -p "$(RELEASE_DIR)"
debugdir: outdir
	@-mkdir -p "$(DEBUG_DIR)"
endif

plugins-dbg:
	@-mkdir -p $(DEBUG_DIR)
	@if test -e ../plugins/Makefile; \
	then $(MAKE) native -C ../plugins  OUT_DIR="$(DEBUG_DIR)" CC="$(CC) $(W32_CFLAGS) $(DEBUG_CFLAGS)" CXX="$(CXX) $(W32_CFLAGS) $(subst -Wno-pointer-sign,,$(DEBUG_CFLAGS))" PKGCONFIG="$(PKGCONFIG)" ARCH="$(ARCH)" BASE_CFLAGS="$(BASE_CFLAGS) $(BRANDFLAGS)" BASE_CXXFLAGS="$(subst -Wno-pointer-sign,,$(BASE_CFLAGS)) $(BRANDFLAGS)" FTE_TARGET="$(FTE_TARGET)"; \
	 else echo no plugins directory installed; \
	fi
plugins:

plugins-rel:
	@-mkdir -p $(RELEASE_DIR)
	@if test -e ../plugins/Makefile; \
	then $(MAKE) native -C ../plugins  OUT_DIR="$(RELEASE_DIR)" CC="$(CC)" CXX="$(CXX)" PKGCONFIG="$(PKGCONFIG)" ARCH="$(ARCH)" BASE_CFLAGS="$(BASE_CFLAGS) $(BRANDFLAGS)" BASE_CXXFLAGS="$(subst -Wno-pointer-sign,,$(BASE_CFLAGS)) $(BRANDFLAGS)" FTE_TARGET="$(FTE_TARGET)"; \
	 else echo no plugins directory installed; \
	fi
plugins-rel:

help:
	@-echo "Specfic targets:"
	@-echo "clean - removes all output (use make dirs afterwards)"
	@-echo "all - make all the targets possible"
	@-echo "rel - make the releases for the default system"
	@-echo "dbg - make the debug builds for the default system"
	@-echo "profile - make all the releases with profiling support for the default system"
	@-echo ""
	@-echo "Normal targets:"
	@-echo "(each of these targets must have the postfix -rel or -dbg)"
	@-echo "'sv-???' (Dedicated Server)"
	@-echo "'gl-???' (OpenGL rendering + Built-in Server)"
	@-echo "'m-???' (Merged client, OpenGL & D3D rendering + Dedicated server)"
	@-echo "'mingl-???' (Minimal featured OpenGL render)"
	@-echo "'d3d-???' (for windows builds)"
	@-echo "'mcl-???' (currently broken)"
	@-echo "'glcl-???' (currently broken)"
	@-echo "'droid-???' (cross compiles Android package)"
	@-echo "'web-???' (compiles javascript/emscripten page)"
	@-echo ""
	@-echo "Cross targets can be specified with FTE_TARGET=blah"
	@-echo "linux32, linux64 specify specific x86 archs"
	@-echo "SDL - Attempt to use sdl for the current target"
	@-echo "win32 - Mingw compile for win32"
	@-echo "vc - Attempts to use msvc8+ to compile. Note: uses profile guided optimisations. You must build+run the relevent profile target before a release target will compile properly. Debug doesn't care."
	@-echo "android targets explicitly cross compile, and should generally not be given an FTE_TARGET."

clean:
	-rm -f -r $(RELEASE_DIR)
	-rm -f -r $(DEBUG_DIR)
	-rm -f -r $(PROFILE_DIR)
	-rm -f -r droid/bin
	-rm -f -r droid/gen
	-rm -f -r droid/libs
	-rm -f droid/default.properties
	-rm -f droid/local.properties
	-rm -f droid/proguard.cfg
	-rm -f droid/build.xml

distclean: clean
	-rm -f droid/ftekeystore



#################################################
#webgl helpers

ifeq ($(FTE_TARGET),web)
$(OUT_DIR)/$(EXE_NAME): web/ftejslib.js web/prejs.js
endif
ifneq ($(shell which emcc 2> /dev/null),)
EMCC?=emcc
else
#EMCC?=/opt/emsdk_portable/emscripten/master/emcc
EMCC?=emcc.bat --em-config $(shell cygpath -m $(USERPROFILE))/.emscripten
endif
ifeq ($(EMSDK),)
	#just adds some extra paths (WINDOWS HOST ONLY)
	#assumes you installed the emscripten 1.22.0 sdk to EMSCRIPTENROOT
	#if you have a different version installed, you will need to fix up the paths yourself (or just use fte_target explicitly yourself).
	EMSCRIPTENROOT?=C:/Games/tools/Emscripten
	#EMSCRIPTENPATH=$(realpath $(EMSCRIPTENROOT)):$(realpath $(EMSCRIPTENROOT)/clang/e1.22.0_64bit):$(realpath $(EMSCRIPTENROOT)/node/0.10.17_64bit):$(realpath $(EMSCRIPTENROOT)/python/2.7.5.3_64bit):$(realpath $(EMSCRIPTENROOT)/emscripten/1.22.0):$(PATH)
	EMSCRIPTENPATH=$(realpath $(EMSCRIPTENROOT)):$(realpath $(EMSCRIPTENROOT)/clang/e1.35.0_64bit):$(realpath $(EMSCRIPTENROOT)/node/4.1.1_64bit/bin):$(realpath $(EMSCRIPTENROOT)/python/2.7.5.3_64bit):$(realpath $(EMSCRIPTENROOT)/emscripten/1.35.0):$(PATH)
else
	EMSCRIPTENPATH=$(PATH)
endif

web-rel:
	@PATH="$(EMSCRIPTENPATH)" $(MAKE) makelibs FTE_TARGET=web CC="$(EMCC)" && PATH="$(EMSCRIPTENPATH)" $(MAKE) gl-rel FTE_TARGET=web CC="$(EMCC)"
	@cp $(BASE_DIR)/web/fteshell.html $(RELEASE_DIR)/ftewebgl.html
	@gzip -kf $(RELEASE_DIR)/ftewebgl.html
	@gzip -kf $(RELEASE_DIR)/ftewebgl.js
	@gzip -kf $(RELEASE_DIR)/ftewebgl.wasm
webcl-rel:
	@PATH="$(EMSCRIPTENPATH)" $(MAKE) makelibs FTE_TARGET=web CC="$(EMCC)" && PATH="$(EMSCRIPTENPATH)" $(MAKE) glcl-rel FTE_TARGET=web CC="$(EMCC)"
	@cp $(BASE_DIR)/web/fteshell.html $(RELEASE_DIR)/ftewebglcl.html
	@sed -i 's/ftewebgl.js/ftewebglcl.js/g' release/ftewebglcl.html	#swap out the .js filename for the client-only one.
	@gzip -kf $(RELEASE_DIR)/ftewebglcl.html
	@gzip -kf $(RELEASE_DIR)/ftewebglcl.js
	@gzip -kf $(RELEASE_DIR)/ftewebglcl.wasm


web-dbg:
	@PATH="$(EMSCRIPTENPATH)" $(MAKE) gl-dbg FTE_TARGET=web CC="$(EMCC)"
	@cp $(BASE_DIR)/web/fteshell.html $(DEBUG_DIR)/ftewebgl.html
	@gzip -kf $(DEBUG_DIR)/ftewebgl.html
	@gzip -kf $(DEBUG_DIR)/ftewebgl.js
	@gzip -kf $(DEBUG_DIR)/ftewebgl.wasm

#################################################
#android

#building for android will require:
#download android sdk+ndk
#ant installed

#droid-dbg will install it on 'the current device', if you've got a device plugged in or an emulator running, it should just work.

#makes an ant project for us
droid/build.xml:
	-cd droid && PATH=$$PATH:$(realpath $(ANDROID_HOME)/tools):$(realpath $(ANDROID_NDK_ROOT)) $(ANDROID_SCRIPT) update project -t android-9 -p . -n FTEDroid

#build FTE as a library, then build the java+package (release)
droid/ftekeystore:
ifeq ($(KEYTOOLARGS),)
	@echo
	@echo In order to build a usable APK file it must be signed. That requires a private key.
	@echo Creation of a private key requries various bits of info...
	@echo You are expected to fill that stuff in now... By the way, don\'t forget the password!
	@echo Note that every time you use make droid-rel, you will be required to enter a password.
	@echo You can use \'make droid-opt\' instead if you wish to build an optimised build without signing,
	@echo  but such packages will require a rooted device \(or to be signed later\).
	@echo Just press control-c if you don\'t want to proceed.
	@echo Morality warning: never distribute droid/ftekeystore - always do make distclean before distributing.
	@echo
	$(JAVATOOL)keytool -genkey -v -keystore $@ -alias autogen -keyalg RSA -keysize 2048 -validity 10000
else
	@echo Generating keystore
	@$(JAVATOOL)keytool -genkey -keystore $@ -alias autogen -keyalg RSA -keysize 2048 -validity 10000 -noprompt $(KEYTOOLARGS)
endif

droid-rel:
	@$(MAKE) FTE_TARGET=droid droid/build.xml droid/ftekeystore
	@$(foreach a, $(DROID_ARCH), $(MAKE) FTE_TARGET=droid m-rel plugins-rel DROID_ARCH=$a NATIVE_PLUGINS="$(NATIVE_PLUGINS)"; )
	@-rm -rf droid/libs
	@$(foreach a, $(DROID_ARCH), mkdir -p droid/libs/$a; )
	-@$(foreach a, $(DROID_ARCH), cp $(RELEASE_DIR)/m_droid-$a/*.so droid/libs/$a/; )

	@cd droid && $(ANT) -q release
ifneq ($(DROID_PACKSU),)
		@echo Adding custom data files - non-compressed
		zip droid/bin/FTEDroid-release-unsigned.apk -0 -j $(DROID_PACKSU)
endif
ifneq ($(DROID_PACKSC),)
		@echo Adding custom data files - compressed
		zip droid/bin/FTEDroid-release-unsigned.apk -9 -j $(DROID_PACKSC)
endif
	@echo Signing package... I hope you remember your password.
	@$(JAVATOOL)jarsigner $(JARSIGNARGS) -digestalg SHA1 -sigalg MD5withRSA -keystore droid/ftekeystore droid/bin/FTEDroid-release-unsigned.apk autogen
	@-rm -f $(RELEASE_DIR)/FTEDroid.apk
	@$(ANDROID_ZIPALIGN) 4 droid/bin/FTEDroid-release-unsigned.apk $(NATIVE_RELEASE_DIR)/FTEDroid.apk

droid-opt:
	$(MAKE) FTE_TARGET=droid droid/build.xml droid/ftekeystore
	$(MAKE) FTE_TARGET=droid gl-rel
	mkdir -p droid/libs/armeabi
	@cp $(RELEASE_DIR)/libftedroid.so droid/libs/armeabi/
	@cd droid && $(ANT) release
	cp droid/bin/FTEDroid-unsigned.apk $(RELEASE_DIR)/FTEDroid.apk

#build FTE as a library, then build the java+package (release). also installs it onto the 'current' device.
droid-dbg:
	$(MAKE) FTE_TARGET=droid droid/build.xml
	$(foreach a, $(DROID_ARCH), $(MAKE) FTE_TARGET=droid m-dbg plugins-dbg DROID_ARCH=$a NATIVE_PLUGINS="$(NATIVE_PLUGINS)"; )
	-rm -rf droid/libs
	@$(foreach a, $(DROID_ARCH), mkdir -p droid/libs/$a; )
	-@$(foreach a, $(DROID_ARCH), cp $(DEBUG_DIR)/m_droid-$a/*.so droid/libs/$a/; )
	@cd droid && $(ANT) debug #&& $(ANT) debug install
	cp droid/bin/FTEDroid-debug.apk $(DEBUG_DIR)/FTEDroid.apk

droid-help:
	@-echo "make droid-dbg - compiles engine with debug info and signs package with debug key. Attempts to install onto emulator."
	@-echo "make droid-opt - compiles engine with optimisations, but does not sign package. Not useful."
	@-echo "make droid-rel - compiles engine with optimisations, adds custom data files, signs with private key, requires password."
	@-echo
	@-echo "Android Settings:
	@-echo "DROID_PACKSC: specifies additional pak or pk3 files to compress into the package, which avoids extra configuration. Only used in release builds. You probably shouldn't use this except for really small packages. Any file seeks will give really poor performance."
	@-echo "DROID_PACKSU: like DROID_PACKSC, but without compression. Faster loading times, but bigger. Use for anything that is already compressed (especially pk3s)."
	@-echo "ANDROID_HOME: path to the android sdk install path."
	@-echo "ANDROID_NDK_ROOT: path to the android ndk install path."
	@-echo "ANT: path and name of apache ant. Probably doesn't need to be set if you're on linux."
	@-echo "JAVA_HOME: path of your java install. Commonly already set in environment settings."
	@-echo "JAVATOOL: path to your java install's bin directory. Doesn't need to be set if its already in your path."
	@-echo "JARSIGNARGS: Additional optional arguments to java's jarsigner program. You may want to put -storepass FOO in here, but what ever you do - keep it secure. Avoid bash history snooping, etc. If its not present, you will safely be prompted as required."
	@-echo
	@-echo "Note that 'make droid-rel' will automatically generate a keystore. If you forget the password, just do a 'make dist-clean'."

$(BASE_DIR)/libs-$(ARCH)/SDL2-$(SDL2VER)/i686-w64-mingw32/bin/sdl2-config:
	wget http://www.libsdl.org/release/SDL2-devel-$(SDL2VER)-mingw.tar.gz -O $(BASE_DIR)/sdl2.tar.gz
	cd $(BASE_DIR)/libs-$(ARCH) && tar -xvzf $(BASE_DIR)/sdl2.tar.gz
	rm $(BASE_DIR)/sdl2.tar.gz
$(BASE_DIR)/libs-$(ARCH)/SDL2-$(SDL2VER)/x86_64-w64-mingw32/bin/sdl2-config: $(BASE_DIR)/libs-$(ARCH)/SDL2-$(SDL2VER)/i686-w64-mingw32/bin/sdl2-config



#makes sure the configure scripts get the right idea.
AR?=$(ARCH)-ar


CONFIGARGS+= --host=$(ARCH) --enable-shared=no CC="$(CC)"
CONFIGARGS:= $(CONFIGARGS)
#--disable-silent-rules
OPUSCONFIGARGS=$(CONFIGARGS)
SPEEXDSPCONFIGARGS=$(CONFIGARGS)

TOOLOVERRIDES+=CFLAGS="$$CFLAGS -Os"
TOOLCONGIGUREOVERRIDES=$(TOOLOVERRIDES)
TOOLMAKEOVERRIDES=$(TOOLOVERRIDES)
ifeq (web,$(FTE_TARGET))
  TOOLCONFIGUREOVERRIDES=emconfigure
  TOOLMAKEOVERRIDES=emmake
  OPUSCONFIGARGS=--disable-rtcd --disable-hardening --enable-stack-protector=no --enable-shared=no --host=none
  SPEEXDSPCONFIGARGS=--disable-neon --host=none
  CONFIGARGS=--enable-shared=no --host=none
endif


libs-$(ARCH)/libjpeg.a:
	test -f jpegsrc.v$(JPEGVER).tar.gz || wget http://www.ijg.org/files/jpegsrc.v$(JPEGVER).tar.gz
	test -f libs-$(ARCH)/libjpeg.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../jpegsrc.v$(JPEGVER).tar.gz && cd jpeg-$(JPEGVER) && $(CONFIGUREOVERRIDES) ./configure $(CONFIGARGS) && $(TOOLOVERRIDES) $(MAKE) && cp .libs/libjpeg.a ../ && $(TOOLOVERRIDES) $(AR) -s ../libjpeg.a && cp jconfig.h jerror.h jmorecfg.h jpeglib.h jversion.h ../ )

libs-$(ARCH)/libz.a libs-$(ARCH)/libz.pc:
	test -f zlib-$(ZLIBVER).tar.gz || wget http://zlib.net/fossils/zlib-$(ZLIBVER).tar.gz
	test -f libs-$(ARCH)/libz.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../zlib-$(ZLIBVER).tar.gz && cd zlib-$(ZLIBVER) && $(TOOLCONFIGUREOVERRIDES) ./configure --static && $(TOOLMAKEOVERRIDES) $(MAKE) libz.a CC="$(CC) $(W32_CFLAGS) -fPIC" && cp libz.a ../ && $(TOOLOVERRIDES) $(AR) -s ../libz.a && cp zlib.h zconf.h zutil.h zlib.pc ../ )
libs-$(ARCH)/libz9.a: libs-$(ARCH)/libz.a
	(cd libs-$(ARCH)/zlib-$(ZLIBVER) && \
		$(CC) -o contrib/infback9/infback9.o -c contrib/infback9/infback9.c -I.	&& \
		$(CC) -o contrib/infback9/inftree9.o -c contrib/infback9/inftree9.c -I.	&& \
		cp contrib/infback9/infback9.h .. && \
		$(AR) rcs ../libz9.a contrib/infback9/infback9.o contrib/infback9/inftree9.o)


libs-$(ARCH)/libpng.a libs-$(ARCH)/libpng.pc: libs-$(ARCH)/libz.a libs-$(ARCH)/libz.pc
	test -f libpng-$(PNGVER).tar.gz || wget http://prdownloads.sourceforge.net/libpng/libpng-$(PNGVER).tar.gz?download -O libpng-$(PNGVER).tar.gz
	test -f libs-$(ARCH)/libpng.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../libpng-$(PNGVER).tar.gz && cd libpng-$(PNGVER) && $(TOOLOVERRIDES) ./configure CPPFLAGS=-I$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)/ LDFLAGS=-L$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)/ $(CONFIGARGS) --enable-static && $(TOOLOVERRIDES) $(MAKE) && cp .libs/libpng16.a ../libpng.a && cp libpng.pc png*.h ../ )

libs-$(ARCH)/libogg.a:
	test -f libogg-$(OGGVER).tar.gz || wget http://downloads.xiph.org/releases/ogg/libogg-$(OGGVER).tar.gz
	test -f libs-$(ARCH)/libogg.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../libogg-$(OGGVER).tar.gz && cd libogg-$(OGGVER) && $(TOOLOVERRIDES) ./configure $(CONFIGARGS) && $(TOOLOVERRIDES) $(MAKE) && cp src/.libs/libogg.a ../ && $(TOOLOVERRIDES) $(AR) -s ../libogg.a && mkdir ../ogg && cp include/ogg/*.h ../ogg)

libs-$(ARCH)/libvorbis.a: libs-$(ARCH)/libogg.a
	test -f libvorbis-$(VORBISVER).tar.gz || wget http://downloads.xiph.org/releases/vorbis/libvorbis-$(VORBISVER).tar.gz
	test -f libs-$(ARCH)/libvorbisfile.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../libvorbis-$(VORBISVER).tar.gz && cd libvorbis-$(VORBISVER) && $(TOOLOVERRIDES) ./configure PKG_CONFIG= $(CONFIGARGS) --disable-oggtest --with-ogg-libraries=.. --with-ogg-includes=$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)/libogg-$(OGGVER)/include && $(TOOLOVERRIDES) $(MAKE) && cp lib/.libs/libvorbis.a ../ && cp lib/.libs/libvorbisfile.a ../  && mkdir ../vorbis && cp include/vorbis/*.h ../vorbis)

libs-$(ARCH)/libopus.a:
	test -f opus-$(OPUSVER).tar.gz || wget https://archive.mozilla.org/pub/opus/opus-$(OPUSVER).tar.gz
	test -f libs-$(ARCH)/libopus.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../opus-$(OPUSVER).tar.gz && cd opus-$(OPUSVER) && CFLAGS="-D_FORTIFY_SOURCE=0 $(CFLAGS) -Os" $(TOOLCONFIGUREOVERRIDES) ./configure $(OPUSCONFIGARGS) --disable-extra-programs && $(TOOLMAKEOVERRIDES) $(MAKE) && cp .libs/libopus.a ../ && cp include/opus*.h ../)

libs-$(ARCH)/libspeex.a:
	test -f speex-$(SPEEXVER).tar.gz || wget http://downloads.us.xiph.org/releases/speex/speex-$(SPEEXVER).tar.gz
	test -f libs-$(ARCH)/libspeex.a || (mkdir -p libs-$(ARCH)/speex && cd libs-$(ARCH) && tar -xvzf ../speex-$(SPEEXVER).tar.gz && cd speex-$(SPEEXVER) && CFLAGS="$(CFLAGS) -Os" $(TOOLCONFIGUREOVERRIDES) ./configure $(CONFIGARGS) --disable-binaries && $(TOOLMAKEOVERRIDES) $(MAKE) && cp libspeex/.libs/libspeex.a ../ && cp -r include/speex/*.h ../speex/)

libs-$(ARCH)/libspeexdsp.a:
	test -f speexdsp-$(SPEEXDSPVER).tar.gz || wget http://downloads.xiph.org/releases/speex/speexdsp-$(SPEEXDSPVER).tar.gz
	test -f libs-$(ARCH)/libspeexdsp.a || (mkdir -p libs-$(ARCH)/speex && cd libs-$(ARCH) && tar -xvzf ../speexdsp-$(SPEEXDSPVER).tar.gz && cd speexdsp-$(SPEEXDSPVER) && CFLAGS="$(CFLAGS) -Os" $(TOOLCONFIGUREOVERRIDES) ./configure $(SPEEXDSPCONFIGARGS) && $(TOOLMAKEOVERRIDES) $(MAKE) && cp libspeexdsp/.libs/libspeexdsp.a ../ && cp -r include/speex/*.h ../speex/)

libs-$(ARCH)/libfreetype.a libs-$(ARCH)/ft2build.h: libs-$(ARCH)/libpng.a libs-$(ARCH)/libpng.pc
	test -f freetype-$(FREETYPEVER).tar.gz || wget https://download-mirror.savannah.gnu.org/releases/freetype/freetype-$(FREETYPEVER).tar.gz
	test -f libs-$(ARCH)/libfreetype.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../freetype-$(FREETYPEVER).tar.gz && cd freetype-$(FREETYPEVER) && PKG_CONFIG_LIBDIR=$(NATIVE_ABSBASE_DIR)/libs-$(ARCH) CFLAGS="$(CFLAGS) -Os" $(TOOLOVERRIDES) ./configure CPPFLAGS=-I$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)/ LDFLAGS=-L$(NATIVE_ABSBASE_DIR)/libs-$(ARCH)/ $(CONFIGARGS) --with-zlib=yes --with-png=yes --with-bzip2=no --with-harfbuzz=no && $(TOOLOVERRIDES) $(MAKE) && cp objs/.libs/libfreetype.a ../ && cp -r include/* ../)

libs-$(ARCH)/libBulletDynamics.a:
	test -f bullet3-$(BULLETVER).tar.gz || wget https://github.com/bulletphysics/bullet3/archive/$(BULLETVER).tar.gz -O bullet3-$(BULLETVER).tar.gz
	test -f libs-$(ARCH)/libBulletDynamics.a || (mkdir -p libs-$(ARCH) && cd libs-$(ARCH) && tar -xvzf ../bullet3-$(BULLETVER).tar.gz && cd bullet3-$(BULLETVER) && CFLAGS="$(CFLAGS) -Os" $(TOOLOVERRIDES) $(DO_CMAKE) . && $(TOOLOVERRIDES) $(MAKE) LinearMath BulletDynamics BulletCollision && cp src/LinearMath/libLinearMath.a src/BulletDynamics/libBulletDynamics.a src/BulletCollision/libBulletCollision.a src/btBulletCollisionCommon.h src/btBulletDynamicsCommon.h ..)

libs-$(ARCH)/vulkan/vulkan.h:
	test -f vulkan-sdk-$(VULKANVER).tar.gz || wget https://github.com/KhronosGroup/Vulkan-Headers/archive/refs/tags/vulkan-sdk-$(VULKANVER).tar.gz  
	cd libs-$(ARCH) && tar -xvzf ../vulkan-sdk-$(VULKANVER).tar.gz --strip-components=2 Vulkan-Headers-vulkan-sdk-$(VULKANVER)/include/

ifeq ($(FTE_TARGET),web)
makelibs: libs-$(ARCH)/libz.a $(MAKELIBS)
else
MAKELIBS+=libs-$(ARCH)/vulkan/vulkan.h
makelibs: libs-$(ARCH)/libjpeg.a libs-$(ARCH)/libz9.a libs-$(ARCH)/libz.a libs-$(ARCH)/libpng.a libs-$(ARCH)/libogg.a libs-$(ARCH)/libvorbis.a libs-$(ARCH)/libopus.a libs-$(ARCH)/libspeex.a libs-$(ARCH)/libspeexdsp.a libs-$(ARCH)/libfreetype.a $(MAKELIBS)
endif

HTTP_OBJECTS=http/httpserver.c http/iwebiface.c common/fs_stdio.c http/ftpserver.c
$(RELEASE_DIR)/httpserver$(BITS)$(EXEPOSTFIX): $(HTTP_OBJECTS)
	$(CC) -o $@ -Icommon -Iclient -Iqclib -Igl -Iserver -DWEBSERVER -DWEBSVONLY -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DNO_PNG $(HTTP_OBJECTS)
httpserver: $(RELEASE_DIR)/httpserver$(BITS)$(EXEPOSTFIX)

IQM_OBJECTS=../iqm/iqm.cpp ../imgtool.c client/image.c common/json.c ../plugins/models/gltf.c
$(RELEASE_DIR)/iqmtool$(BITS)$(EXEPOSTFIX): $(IQM_OBJECTS)
ifeq (win,$(findstring win,$(FTE_TARGET)))
	$(CC) -o $@ $(IQM_OBJECTS) -Icommon -Iclient -Iqclib -Igl -Iserver $(ALL_CFLAGS) $(CLIENTLIBFLAGS) -DIQMTOOL $(BASELDFLAGS) $(CLIENTLDDEPS) --static -static-libgcc -static-libstdc++ -lstdc++ -lm -Os
else
	$(CC) -o $@ $(IQM_OBJECTS) -Icommon -Iclient -Iqclib -Igl -Iserver $(ALL_CFLAGS) -DIQMTOOL -lstdc++ -lm -ldl -Os
endif
iqm-rel: reldir $(RELEASE_DIR)/iqmtool$(BITS)$(EXEPOSTFIX)
iqmtool-rel: reldir $(RELEASE_DIR)/iqmtool$(BITS)$(EXEPOSTFIX)
iqm: iqm-rel
iqmtool: iqmtool-rel

IMGTOOL_OBJECTS=../imgtool.c client/image.c
$(RELEASE_DIR)/imgtool$(BITS)$(EXEPOSTFIX): $(IMGTOOL_OBJECTS)
	$(CC) -o $@ $(IMGTOOL_OBJECTS) -lstdc++ -lm -Os $(ALL_CFLAGS) $(CLIENTLIBFLAGS) -DIMGTOOL $(BASELDFLAGS) $(CLIENTLDDEPS)

imgtool-rel: $(RELEASE_DIR)/imgtool$(BITS)$(EXEPOSTFIX)
imgtool: imgtool-rel

MASTER_OBJECTS=server/sv_sys_unix.c common/sys_linux_threads.c common/net_ssl_gnutls.c server/sv_master.c common/net_wins.c common/net_ice.c common/cvar.c common/cmd.c common/sha1.c common/sha2.c http/httpclient.c common/log.c common/fs.c common/fs_stdio.c common/common.c common/translate.c common/zone.c qclib/hash.c
$(RELEASE_DIR)/ftemaster$(BITS)$(EXEPOSTFIX): $(MASTER_OBJECTS)
	$(CC) -o $@ $(MASTER_OBJECTS) -flto=jobserver -fvisibility=hidden -Icommon -Iclient -Iqclib -Igl -Iserver -DMASTERONLY -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -lm -ldl -lz $(RELEASE_CFLAGS) $(RELEASE_LDFLAGS)
$(DEBUG_DIR)/ftemaster$(BITS)$(EXEPOSTFIX): $(MASTER_OBJECTS)
	$(CC) -o $@ $(MASTER_OBJECTS) -Icommon -Iclient -Iqclib -Igl -Iserver -DMASTERONLY -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -lm -ldl -lz $(DEBUG_CFLAGS) $(DEBUG_LDFLAGS)
master-rel: reldir $(RELEASE_DIR)/ftemaster$(BITS)$(EXEPOSTFIX)
master-dbg: dbgdir $(DEBUG_DIR)/ftemaster$(BITS)$(EXEPOSTFIX)
master: master-rel

QTV_OBJECTS=	\
	netchan.c	\
	parse.c		\
	msg.c		\
	qw.c		\
	source.c	\
	bsp.c		\
	rcon.c		\
	mdfour.c	\
	md5.c		\
	crc.c		\
	control.c	\
	forward.c	\
	relay.c		\
	pmove.c		\
	menu.c		\
	msg.c		\
	httpsv.c	\
	sha1.c		\
	libqtvc/glibc_sucks.c
$(RELEASE_DIR)/qtv$(BITS)$(EXEPOSTFIX): $(QTV_OBJECTS)
	$(CC) -o $@ $? -lstdc++ -lm $(BASE_INCLUDES) $(QTV_LDFLAGS) $(SVNREVISION)
qtv-rel:
	@$(MAKE) $(RELEASE_DIR)/qtv$(BITS)$(EXEPOSTFIX) VPATH="$(BASE_DIR)/../fteqtv:$(VPATH)"
qtv: qtv-rel

utils: httpserver iqmtool imgtool master qtv-rel

prefix ?= /usr/local
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
sbindir ?= $(exec_prefix)/sbin
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA ?= ${INSTALL} -m 644
install: sv-rel gl-rel mingl-rel qcc-rel
	$(INSTALL_PROGRAM) $(RELEASE_DIR)/$(EXE_NAME)-gl $(DESTDIR)$(bindir)/$(EXE_NAME)-gl
	$(INSTALL_PROGRAM) $(RELEASE_DIR)/$(EXE_NAME)-mingl $(DESTDIR)$(bindir)/$(EXE_NAME)-mingl
	$(INSTALL_PROGRAM) $(RELEASE_DIR)/$(EXE_NAME)-sv $(DESTDIR)$(bindir)/$(EXE_NAME)-sv
	$(INSTALL_PROGRAM) $(RELEASE_DIR)/fteqcc $(DESTDIR)$(bindir)/fteqcc

version:
	@echo $(SVN_VERSION)
	@echo $(SVN_DATE)
	@echo $(FTE_CONFIG_EXTRA)


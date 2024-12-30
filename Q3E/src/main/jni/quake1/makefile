#####  DP_MAKE_TARGET autodetection and arch specific variables #####

ifndef DP_MAKE_TARGET

# Win32
ifdef WINDIR
	DP_MAKE_TARGET=mingw
else

# UNIXes
DP_ARCH:=$(shell uname)
ifneq ($(filter %BSD,$(DP_ARCH)),)
	DP_MAKE_TARGET=bsd
else
ifeq ($(DP_ARCH), Darwin)
	DP_MAKE_TARGET=macosx
else
ifeq ($(DP_ARCH), SunOS)
	DP_MAKE_TARGET=sunos
else
	DP_MAKE_TARGET=linux

endif  # ifeq ($(DP_ARCH), SunOS)
endif  # ifeq ($(DP_ARCH), Darwin)
endif  # ifneq ($(filter %BSD,$(DP_ARCH)),)
endif  # ifdef windir
endif  # ifndef DP_MAKE_TARGET

# If we're targeting an x86 CPU we want to enable DP_SSE (CFLAGS_SSE and SSE2)
ifeq ($(DP_MAKE_TARGET), mingw)
	DP_SSE:=1
else ifeq ($(OS),Windows_NT)
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		DP_SSE:=1
	else ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		DP_SSE:=1
	else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		DP_SSE:=1
	else
		DP_SSE:=0
	endif
else
	DP_MACHINE:=$(shell uname -m)
	ifeq ($(DP_MACHINE),x86_64)
		DP_SSE:=1
	else ifeq ($(DP_MACHINE),i686)
		DP_SSE:=1
	else ifeq ($(DP_MACHINE),i386)
		DP_SSE:=1
	else
		DP_SSE:=0
	endif
endif

# Makefile name
MAKEFILE=makefile

# Commands
ifdef windir
	CMD_RM=del
	CMD_CP=copy /y
	CMD_MKDIR=mkdir
else
	CMD_RM=$(CMD_UNIXRM)
	CMD_CP=$(CMD_UNIXCP)
	CMD_MKDIR=$(CMD_UNIXMKDIR)
endif

# default targets
TARGETS_DEBUG=sv-debug sdl-debug
TARGETS_PROFILE=sv-profile sdl-profile
TARGETS_RELEASE=sv-release sdl-release
TARGETS_RELEASE_PROFILE=sv-release-profile sdl-release-profile
TARGETS_NEXUIZ=sv-nexuiz sdl-nexuiz


###### Optional features #####
DP_VIDEO_CAPTURE?=enabled
ifeq ($(DP_VIDEO_CAPTURE), enabled)
	CFLAGS_VIDEO_CAPTURE=-DCONFIG_VIDEO_CAPTURE
	OBJ_VIDEO_CAPTURE=cap_avi.o cap_ogg.o
else
	CFLAGS_VIDEO_CAPTURE=
	OBJ_VIDEO_CAPTURE=
endif

# Linux configuration
ifeq ($(DP_MAKE_TARGET), linux)
	OBJ_ICON=
	OBJ_ICON_NEXUIZ=

	LDFLAGS_SV=$(LDFLAGS_LINUXSV)
	LDFLAGS_SDL=$(LDFLAGS_LINUXSDL)

	SDLCONFIG_CFLAGS=$(SDLCONFIG_UNIXCFLAGS) $(SDLCONFIG_UNIXCFLAGS_X11)
	SDLCONFIG_LIBS=$(SDLCONFIG_UNIXLIBS) $(SDLCONFIG_UNIXLIBS_X11)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS) $(SDLCONFIG_UNIXSTATICLIBS_X11)

	EXE_SV=$(EXE_UNIXSV)
	EXE_SDL=$(EXE_UNIXSDL)
	EXE_SVNEXUIZ=$(EXE_UNIXSVNEXUIZ)
	EXE_SDLNEXUIZ=$(EXE_UNIXSDLNEXUIZ)

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=shared
	DP_LINK_JPEG?=shared
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen
endif

ifeq ($(DP_MAKE_TARGET), wasm)
	MAKE=emmake make
# 	CFLAGS_EXTRA+=--use-port=sdl2 \
# 	              --use-port=libpng \
# 	              --use-port=libjpeg \
# 	              --use-port=zlib \
# 	              -DNOSUPPORTIPV6 \
# 	              -DUSE_GLES2
	CFLAGS_EXTRA+=-s USE_SDL=2 \
	              -s USE_LIBPNG=1 \
	              -s USE_LIBJPEG=1 \
	              -s USE_ZLIB=1 \
	              -DNOSUPPORTIPV6 \
	              -DUSE_GLES2

	SDLCONFIG_CFLAGS=$(SDLCONFIG_UNIXCFLAGS) $(SDLCONFIG_UNIXCFLAGS_X11)
	SDLCONFIG_LIBS=$(SDLCONFIG_UNIXLIBS) $(SDLCONFIG_UNIXLIBS_X11)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS) $(SDLCONFIG_UNIXSTATICLIBS_X11)
	DP_SSE=0

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=shared
	DP_LINK_JPEG?=dlopen
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen
endif

# Mac OS X configuration
ifeq ($(DP_MAKE_TARGET), macosx)
	OBJ_ICON=
	OBJ_ICON_NEXUIZ=

	LDFLAGS_SV=$(LDFLAGS_MACOSXSV)
	LDFLAGS_SDL=$(LDFLAGS_MACOSXSDL)

	SDLCONFIG_CFLAGS=$(SDLCONFIG_MACOSXCFLAGS)
	SDLCONFIG_LIBS=$(SDLCONFIG_MACOSXLIBS)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_MACOSXSTATICLIBS)

	EXE_SV=$(EXE_UNIXSV)
	EXE_SDL=$(EXE_UNIXSDL)
	EXE_SVNEXUIZ=$(EXE_UNIXSVNEXUIZ)
	EXE_SDLNEXUIZ=$(EXE_UNIXSDLNEXUIZ)

	ifeq ($(word 2, $(filter -arch, $(CC))), -arch)
		CFLAGS_MAKEDEP=
	endif

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=shared
	DP_LINK_JPEG?=dlopen
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen

	TARGETS_DEBUG=sv-debug sdl-debug
	TARGETS_PROFILE=sv-profile sdl-profile
	TARGETS_RELEASE=sv-release sdl-release
	TARGETS_RELEASE_PROFILE=sv-release-profile sdl-release-profile
	TARGETS_NEXUIZ=sv-nexuiz sdl-nexuiz
endif

# SunOS configuration (Solaris)
ifeq ($(DP_MAKE_TARGET), sunos)
	OBJ_ICON=
	OBJ_ICON_NEXUIZ=

	CFLAGS_EXTRA=$(CFLAGS_SUNOS)

	LDFLAGS_SV=$(LDFLAGS_SUNOSSV)
	LDFLAGS_SDL=$(LDFLAGS_SUNOSSDL)

	SDLCONFIG_CFLAGS=$(SDLCONFIG_UNIXCFLAGS) $(SDLCONFIG_UNIXCFLAGS_X11)
	SDLCONFIG_LIBS=$(SDLCONFIG_UNIXLIBS) $(SDLCONFIG_UNIXLIBS_X11)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS) $(SDLCONFIG_UNIXSTATICLIBS_X11)

	EXE_SV=$(EXE_UNIXSV)
	EXE_SDL=$(EXE_UNIXSDL)
	EXE_SVNEXUIZ=$(EXE_UNIXSVNEXUIZ)
	EXE_SDLNEXUIZ=$(EXE_UNIXSDLNEXUIZ)

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=shared
	DP_LINK_JPEG?=shared
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen
endif

# BSD configuration
ifeq ($(DP_MAKE_TARGET), bsd)

	OBJ_ICON=
	OBJ_ICON_NEXUIZ=

	LDFLAGS_SV=$(LDFLAGS_BSDSV)
	LDFLAGS_SDL=$(LDFLAGS_BSDSDL)

	SDLCONFIG_CFLAGS=$(SDLCONFIG_UNIXCFLAGS) $(SDLCONFIG_UNIXCFLAGS_X11)
	SDLCONFIG_LIBS=$(SDLCONFIG_UNIXLIBS) $(SDLCONFIG_UNIXLIBS_X11)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS) $(SDLCONFIG_UNIXSTATICLIBS_X11)

	EXE_SV=$(EXE_UNIXSV)
	EXE_SDL=$(EXE_UNIXSDL)
	EXE_SVNEXUIZ=$(EXE_UNIXSVNEXUIZ)
	EXE_SDLNEXUIZ=$(EXE_UNIXSDLNEXUIZ)

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=shared
	DP_LINK_JPEG?=shared
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen
endif

# Win32 configuration
ifeq ($(DP_MAKE_TARGET), mingw)
	OBJ_ICON=darkplaces.o
	OBJ_ICON_NEXUIZ=nexuiz.o

	LDFLAGS_SV=$(LDFLAGS_WINSV)
	LDFLAGS_SDL=$(LDFLAGS_WINSDL)

	SDLCONFIG_CFLAGS=$(SDLCONFIG_UNIXCFLAGS)
	SDLCONFIG_LIBS=$(SDLCONFIG_UNIXLIBS)
	SDLCONFIG_STATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS)

	EXE_SV=$(EXE_WINSV)
	EXE_SDL=$(EXE_WINSDL)
	EXE_SVNEXUIZ=$(EXE_WINSVNEXUIZ)
	EXE_SDLNEXUIZ=$(EXE_WINSDLNEXUIZ)

	DP_LINK_SDL?=shared
	DP_LINK_ZLIB?=dlopen
	DP_LINK_JPEG?=shared
	DP_LINK_ODE?=
	DP_LINK_CRYPTO?=dlopen
	DP_LINK_CRYPTO_RIJNDAEL?=dlopen
	DP_LINK_XMP?=dlopen
endif


##### Library linking #####
# SDL2
SDL_CONFIG?=sdl2-config
SDLCONFIG_UNIXCFLAGS?=`$(SDL_CONFIG) --cflags`
SDLCONFIG_UNIXCFLAGS_X11?=
SDLCONFIG_UNIXLIBS?=`$(SDL_CONFIG) --libs`
SDLCONFIG_UNIXLIBS_X11?=-lX11
SDLCONFIG_UNIXSTATICLIBS?=`$(SDL_CONFIG) --static-libs`
SDLCONFIG_UNIXSTATICLIBS_X11?=-lX11
SDLCONFIG_MACOSXCFLAGS=$(SDLCONFIG_UNIXCFLAGS)
SDLCONFIG_MACOSXLIBS=$(SDLCONFIG_UNIXLIBS)
SDLCONFIG_MACOSXSTATICLIBS=$(SDLCONFIG_UNIXSTATICLIBS)
ifeq ($(DP_LINK_SDL), shared)
	SDL_LIBS=$(SDLCONFIG_LIBS)
else ifeq ($(DP_LINK_SDL), static)
	SDL_LIBS=$(SDLCONFIG_STATICLIBS)
else ifeq ($(DP_LINK_SDL), dlopen)
  $(error libSDL2 can only be used with shared or static linking)
endif

# zlib
ifeq ($(DP_LINK_ZLIB), shared)
	CFLAGS_LIBZ=-DLINK_TO_ZLIB
	LIB_Z=-lz
else ifeq ($(DP_LINK_ZLIB), static)
	CFLAGS_LIBZ=-DLINK_TO_ZLIB
	LIB_Z=-l:libz.a
else ifeq ($(DP_LINK_ZLIB), dlopen)
	CFLAGS_LIBZ=
	LIB_Z=
endif

# jpeg
ifeq ($(DP_LINK_JPEG), shared)
	CFLAGS_LIBJPEG=-DLINK_TO_LIBJPEG
	LIB_JPEG=-ljpeg
else ifeq ($(DP_LINK_JPEG), static)
	CFLAGS_LIBJPEG=-DLINK_TO_LIBJPEG
	LIB_JPEG=-l:libjpeg.a
else ifeq ($(DP_LINK_JPEG), dlopen)
	CFLAGS_LIBJPEG=
	LIB_JPEG=
endif

# ode
ifeq ($(DP_LINK_ODE), shared)
	ODE_CONFIG?=ode-config
	LIB_ODE=`$(ODE_CONFIG) --libs`
	CFLAGS_ODE=`$(ODE_CONFIG) --cflags` -DUSEODE -DLINK_TO_LIBODE
else ifeq ($(DP_LINK_ODE), static)
	# This is the configuration from Xonotic
	ODE_CONFIG?=ode-config
	LIB_ODE=-l:libode.a -lstdc++ -pthread
	CFLAGS_ODE=-DUSEODE -DLINK_TO_LIBODE -DdDOUBLE
else ifeq ($(DP_LINK_ODE), dlopen)
	LIB_ODE=
	CFLAGS_ODE=-DUSEODE
endif

# d0_blind_id
ifeq ($(DP_LINK_CRYPTO), shared)
	LIB_CRYPTO=-ld0_blind_id
	CFLAGS_CRYPTO=-DLINK_TO_CRYPTO
else ifeq ($(DP_LINK_CRYPTO), static)
	LIB_CRYPTO=-l:libd0_blind_id.a -lgmp
	CFLAGS_CRYPTO=-DLINK_TO_CRYPTO
else ifeq ($(DP_LINK_CRYPTO), static_inc_gmp)
	LIB_CRYPTO=-l:libd0_blind_id.a -l:libgmp.a
	CFLAGS_CRYPTO=-DLINK_TO_CRYPTO
else ifeq ($(DP_LINK_CRYPTO), dlopen)
	LIB_CRYPTO=
	CFLAGS_CRYPTO=
endif

# d0_rijndael
ifeq ($(DP_LINK_CRYPTO_RIJNDAEL), shared)
	LIB_CRYPTO_RIJNDAEL=-ld0_rijndael
	CFLAGS_CRYPTO_RIJNDAEL=-DLINK_TO_CRYPTO_RIJNDAEL
else ifeq ($(DP_LINK_CRYPTO_RIJNDAEL), static)
	LIB_CRYPTO_RIJNDAEL=-l:libd0_rijndael.a
	CFLAGS_CRYPTO_RIJNDAEL=-DLINK_TO_CRYPTO_RIJNDAEL
else ifeq ($(DP_LINK_CRYPTO_RIJNDAEL), dlopen)
	LIB_CRYPTO_RIJNDAEL=
	CFLAGS_CRYPTO_RIJNDAEL=
endif

# xmp
ifeq ($(DP_LINK_XMP), shared)
	OBJ_SND_XMP=snd_xmp.o
	LIB_SND_XMP=-lxmp
	CFLAGS_SND_XMP=-DUSEXMP -DLINK_TO_LIBXMP
else ifeq ($(DP_LINK_XMP), static)
	OBJ_SND_XMP=snd_xmp.o
	LIB_SND_XMP=-l:libxmp.a
	CFLAGS_SND_XMP=-DUSEXMP -DLINK_TO_LIBXMP
else ifeq ($(DP_LINK_XMP), dlopen)
	OBJ_SND_XMP=snd_xmp.o
	LIB_SND_XMP=
	CFLAGS_SND_XMP=-DUSEXMP
endif


##### Extra CFLAGS #####
ifdef DP_FS_BASEDIR
	CFLAGS_FS=-DDP_FS_BASEDIR=\"$(DP_FS_BASEDIR)\"
else
	CFLAGS_FS=
endif

CFLAGS_PRELOAD=
ifneq ($(DP_MAKE_TARGET), mingw)
ifdef DP_PRELOAD_DEPENDENCIES
# DP_PRELOAD_DEPENDENCIES: when set, link against the libraries needed using -l
# dynamically so they won't get loaded at runtime using dlopen
	LDFLAGS_CL+=$(LDFLAGS_UNIXCL_PRELOAD)
	LDFLAGS_SV+=$(LDFLAGS_UNIXSV_PRELOAD)
	LDFLAGS_SDL+=$(LDFLAGS_UNIXSDL_PRELOAD)
	CFLAGS_PRELOAD=$(CFLAGS_UNIX_PRELOAD)
endif
endif

CFLAGS_NET=
# Systems without IPv6 support should uncomment this:
#CFLAGS_NET+=-DNOSUPPORTIPV6


##### GNU Make specific definitions #####

DO_LD=$(CC) -o ../../../$@ $^ $(LDFLAGS)


##### Definitions shared by all makefiles #####
include makefile.inc


##### Dependency files #####

-include *.d

# hack to deal with no-longer-needed .h files
%.h:
	@echo
	@echo "NOTE: file $@ mentioned in dependencies missing, continuing..."
	@echo "HINT: consider 'make clean'"
	@echo

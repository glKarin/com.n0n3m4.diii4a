#####  DP_MAKE_TARGET autodetection and arch specific variables #####

.ifndef DP_MAKE_TARGET

DP_MAKE_TARGET=bsd

.endif
DP_ARCH != uname

# Makefile name
MAKEFILE=BSDmakefile

# Commands
CMD_RM=$(CMD_UNIXRM)
CMD_CP=$(CMD_UNIXCP)
CMD_MKDIR=$(CMD_UNIXMKDIR)

# default targets
TARGETS_DEBUG=sv-debug sdl-debug
TARGETS_PROFILE=sv-profile sdl-profile
TARGETS_RELEASE=sv-release sdl-release
TARGETS_RELEASE_PROFILE=sv-release-profile sdl-release-profile
TARGETS_NEXUIZ=sv-nexuiz sdl-nexuiz

# Link options
DP_LINK_ZLIB?=shared
DP_LINK_JPEG?=shared
DP_LINK_ODE?=
DP_LINK_CRYPTO?=dlopen
DP_LINK_CRYPTO_RIJNDAEL?=dlopen
DP_LINK_XMP?=dlopen

###### Optional features #####
DP_VIDEO_CAPTURE?=enabled
.if $(DP_VIDEO_CAPTURE) == "enabled"
  CFLAGS_VIDEO_CAPTURE=-DCONFIG_VIDEO_CAPTURE
  OBJ_VIDEO_CAPTURE=cap_avi.o cap_ogg.o
.else
  CFLAGS_VIDEO_CAPTURE=
  OBJ_VIDEO_CAPTURE=
.endif

# X11 libs
UNIX_X11LIBPATH=/usr/X11R6/lib

# BSD configuration
.if $(DP_MAKE_TARGET) == "bsd"

# FreeBSD uses OSS
.if $(DP_ARCH) == "FreeBSD"
DEFAULT_SNDAPI=OSS
.else
DEFAULT_SNDAPI=BSD
.endif

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

# set these to "" if you want to use dynamic loading instead
# zlib
.if $(DP_LINK_ZLIB) == "shared"
CFLAGS_LIBZ=-DLINK_TO_ZLIB
LIB_Z=-lz
.else
CFLAGS_LIBZ=
LIB_Z=
.endif

# jpeg
.if $(DP_LINK_JPEG) == "shared"
CFLAGS_LIBJPEG=-DLINK_TO_LIBJPEG
LIB_JPEG=-ljpeg
.else
CFLAGS_LIBJPEG=
LIB_JPEG=
.endif

# ode
.if $(DP_LINK_ODE) == "shared"
ODE_CONFIG?=ode-config
LIB_ODE=`$(ODE_CONFIG) --libs`
CFLAGS_ODE=`$(ODE_CONFIG) --cflags` -DUSEODE -DLINK_TO_LIBODE
.else
LIB_ODE=
CFLAGS_ODE=-DUSEODE
.endif

# d0_blind_id
.if $(DP_LINK_CRYPTO) == "shared"
LIB_CRYPTO=-ld0_blind_id
CFLAGS_CRYPTO=-DLINK_TO_CRYPTO
.else
LIB_CRYPTO=
CFLAGS_CRYPTO=
.endif
.if $(DP_LINK_CRYPTO_RIJNDAEL) == "shared"
LIB_CRYPTO_RIJNDAEL=-ld0_rijndael
CFLAGS_CRYPTO_RIJNDAEL=-DLINK_TO_CRYPTO_RIJNDAEL
.else
LIB_CRYPTO_RIJNDAEL=
CFLAGS_CRYPTO_RIJNDAEL=
.endif

# xmp
.if $(DP_LINK_XMP) == "shared"
OBJ_SND_XMP=snd_xmp.o
LIB_SND_XMP=-lxmp
CFLAGS_SND_XMP=-DUSEXMP -DLINK_TO_LIBXMP
.else
OBJ_SND_XMP=snd_xmp.o
LIB_SND_XMP=
CFLAGS_SND_XMP=-DUSEXMP
.endif

.endif


##### Extra CFLAGS #####

CFLAGS_MAKEDEP=-MD
.ifdef DP_FS_BASEDIR
CFLAGS_FS=-DDP_FS_BASEDIR='\"$(DP_FS_BASEDIR)\"'
.else
CFLAGS_FS=
.endif

CFLAGS_PRELOAD=
.ifdef DP_PRELOAD_DEPENDENCIES
LDFLAGS_SV+=$(LDFLAGS_UNIXSV_PRELOAD)
LDFLAGS_SDL+=$(LDFLAGS_UNIXSDL_PRELOAD)
CFLAGS_PRELOAD=$(CFLAGS_UNIX_PRELOAD)
.endif

CFLAGS_NET=
# Systems without IPv6 support should uncomment this:
#CFLAGS_NET+=-DNOSUPPORTIPV6

##### BSD Make specific definitions #####

MAKE:=$(MAKE) -f BSDmakefile

DO_LD=$(CC) -o ../../../$@ $> $(LDFLAGS)


##### Definitions shared by all makefiles #####
.include "makefile.inc"

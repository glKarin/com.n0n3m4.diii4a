
#windows is special as always, but we don't support itanium, and microsoft don't support anything else (not even arm with the nt win32 api)
ifeq ($(FTE_TARGET),win32)
	PLUG_NATIVE_EXT=_x86.dll
	PLUG_LDFLAGS= -static-libgcc
	PLUG_LDFLAGS_ZLIB=-L../engine/libs/mingw-libs -lz
	BITS=32
	PLUG_LDFLAGS_DL=
	CMAKERULES=$(OUT_DIR)/toolchain_$(FTE_TARGET).cmake
endif

ifeq ($(FTE_TARGET),win64)
	PLUG_NATIVE_EXT=_x64.dll
	PLUG_LDFLAGS=-Wl,--support-old-code -static-libgcc
	PLUG_LDFLAGS_ZLIB=-L../engine/libs/mingw64-libs -lz
	BITS=64
	PLUG_LDFLAGS_DL=
	CMAKERULES=$(OUT_DIR)/toolchain_$(FTE_TARGET).cmake
endif

PLUG_PREFIX=$(OUT_DIR)/fteplug_

ifeq ($(FTE_TARGET),bsd)
	PLUG_LDFLAGS_DL?=-ldl -lc
else
	PLUG_LDFLAGS_DL?=-ldl -static-libgcc
endif

PLUG_LDFLAGS?=-L/usr/local/lib -Wl,-R/usr/local/lib -lm
PLUG_LDFLAGS_ZLIB?=-lz

ifneq ($(PLUG_NATIVE_EXT),)
	#if we're on windows, we'll put our windows-specific hacks here.
	PLUG_DEFFILE=plugin.def
	PLUG_CFLAGS=
	PLUG_CXXFLAGS=
endif

#cygwin uses dll naming.
ifeq ($(FTE_TARGET),cygwin)
	ifeq ($(BITS),64)
		PLUG_DEFFILE=plugin.def
		PLUG_NATIVE_EXT=_amd64.dll
	endif
	ifneq ($(BITS),64)
		PLUG_DEFFILE=plugin.def
		PLUG_NATIVE_EXT=_x86.dll
	endif
endif

#if they're not on windows, we'll try asking the compiler directly
#the check to see if its already set is to avoid asking msvc, which would probably break things.
ifeq ($(PLUG_NATIVE_EXT),)
	LIBRESOLV=-lresolv
	ifneq ($(shell echo|$(CC) -E -dM -|grep __amd64__),)
		#either x32 or x64 ABIs
		ifneq ($(shell echo|$(CC) -E -dM -|grep __ILP32__|grep 1),)
			PLUG_NATIVE_EXT=_x32.so
		else
			PLUG_NATIVE_EXT=_amd64.so
		endif
	endif

	ifneq ($(shell echo|$(CC) -E -dM -|grep __i386__),)
		PLUG_NATIVE_EXT=_x86.so
	endif

	ifneq ($(shell echo|$(CC) -E -dM -|grep __arm__),)
		#gnueabi[hf]
		ifneq ($(shell echo|$(CC) -E -dM -|grep __SOFTFP__),)
			PLUG_NATIVE_EXT=_arm.so
		else
			PLUG_NATIVE_EXT=_armhf.so
		endif
	endif

	ifneq ($(shell echo|$(CC) -E -dM -|grep __ppc__),)
		PLUG_NATIVE_EXT=_ppc.so		#32bit big-endian
	endif

	ifneq ($(shell echo|$(CC) -E -dM -|grep __ppc64__),)
		PLUG_NATIVE_EXT=_ppc64.so	#64bit big-endian
	endif

	ifneq ($(shell echo|$(CC) -E -dM -|grep __ppc64le__),)
		PLUG_NATIVE_EXT=_ppc64le.so	#64bit little-endian.
	endif
endif

ifeq ($(FTE_TARGET),droid)
	#plugins get written to the tmp build dir, to avoid conflicts
	PLUG_PREFIX=$(OUT_DIR)/m_droid-$(DROID_ARCH)/libplug_
	#don't bother with cpu arch postfixes. they'll be in separate directories anyway.
	PLUG_NATIVE_EXT=.so
	#libresolv has no public api on android...
	LIBRESOLV=
	#so we know our target.
	PLUG_CFLAGS=-DANDROID
	PLUG_CXXFLAGS=-DANDROID
endif
ARCHLIBS=../engine/libs-$(ARCH)

#fallback
PLUG_NATIVE_EXT?=.so

PLUG_DEFFILE?=
PLUG_CFLAGS?=-fPIC -Wl,--no-undefined -Bsymbolic -fvisibility=hidden -static-libgcc
PLUG_CXXFLAGS?=-fPIC -Wl,--no-undefined -Bsymbolic -fvisibility=hidden -static-libgcc
PLUG_CMAKE?=-DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX)
PLUG_LDFLAGS_ZLIB?=
ARCH:=$(shell $(CC) -dumpmachine)
PLUG_LDFLAGS:=-L$(ARCHLIBS) $(PLUG_LDFLAGS)
PLUG_CFLAGS:=-I$(ARCHLIBS) $(PLUG_CFLAGS)
PLUG_CXXFLAGS:=-I$(ARCHLIBS) $(PLUG_CXXFLAGS)

#$1 is the plugin name, $2 is its filename, $3 is title, $4 is description
define EMBEDMETA
	@echo "{\\n	package fteplug_$1\\n	ver \"${SVN_VERSION}\"\\n	category Plugins\\n	title \"$3\"\\n	gamedir \"\"\\n	desc \"$4\"\\n}" | zip -q -9 -fz- $2.metazip -
	@cat $2.metazip >> $2
	@zip -q -A $2
	@rm $2.metazip
endef

#legacy build rule, now equivelent to all
native: all

clean: ezscript-clean qi-clean hud-clean irc-clean

.PHONY: all native distclean clean

help:
	@-echo make a subdirectory

######################################
#small script to download+install avformat for windows cross compiles.
#linux users are expected to have the library installed locally already. If your version is too old or missing, run the following command to install it (to /usr/local), then delete the gz and directory.
#wget http://ffmpeg.org/releases/ffmpeg-1.2.tar.gz && cd tar xvfz ffmpeg-1.2.tar.gz && cd ffmpeg-1.2/ && ./configure --disable-yasm --enable-shared && make && sudo make install
#we use ffmpeg's version for some reason, as opposed to libav. not sure what the differences are meant to be, but libav seemed to have non-depricated functions defined, docs that say to use them, and these functions missing.

AV_VER=ffmpeg-4.0

ifeq ($(findstring win,$(FTE_TARGET)),win)
	AV_BASE=$(abspath $(OUT_DIR)/../fte_libav_$(AV_VER))/
else
	ifeq ($(FTE_TARGET),bsd)
		AV_BASE=/usr/local/include/
	else
		AV_BASE=/usr/include/ffmpeg/
	endif
endif

ifneq ($(AV_BASE),)
	AV_DEP=$(AV_BASE)libavformat/avformat.h
	AV_CFLAGS=-I$(AV_BASE)
	AV_LDFLAGS=-L$(AV_BASE)lib$(BITS) -lavcodec -lavformat -lavutil -lswscale
else
	AV_LDFLAGS=-lavcodec -lavformat -lavutil -lswscale
endif

ifeq ($(FTE_TARGET),bsd)
	AV_LDFLAGS+=-lc
endif

AVPLUG_OBJS= avplug/avaudio.c avplug/avencode.c avplug/avdecode.c plugin.c

ifeq ($(findstring win,$(FTE_TARGET)),win)
	ifeq (0,1)
		AV_ARCHIVEEXT=.z7
		AV_EXTRACT=7z e -y
	else
		AV_ARCHIVEEXT=.zip
		AV_EXTRACT=unzip -ju
	endif

	AV_W32_DEV=$(AV_VER)-win32-dev$(AV_ARCHIVEEXT)
	AV_W64_DEV=$(AV_VER)-win64-dev$(AV_ARCHIVEEXT)
	AV_W32_BIN=$(AV_VER)-win32-shared$(AV_ARCHIVEEXT)
	AV_W64_BIN=$(AV_VER)-win64-shared$(AV_ARCHIVEEXT)
	AV_URL32_DEV=https://archive.org/download/zeranoe/win32/dev/$(AV_W32_DEV)
	AV_URL64_DEV=https://archive.org/download/zeranoe/win64/dev/$(AV_W64_DEV)
	AV_URL32_BIN=https://archive.org/download/zeranoe/win32/shared/$(AV_W32_BIN)
	AV_URL64_BIN=https://archive.org/download/zeranoe/win64/shared/$(AV_W64_BIN)
	AV_PRE32_DEV=$(AV_VER)-win32-dev/
	AV_PRE64_DEV=$(AV_VER)-win64-dev/
	AV_PRE32_BIN=$(AV_VER)-win32-shared/
	AV_PRE64_BIN=$(AV_VER)-win64-shared/

	ifeq ($(FTE_TARGET),win32)
		FFMPEG_ZIP=$(AV_BASE)/$(AV_VER)-x86.zip
		#NATIVE_PLUGINS+=ffmpeg
	endif

	ifeq ($(FTE_TARGET),win64)
		FFMPEG_ZIP=$(AV_BASE)/$(AV_VER)-x64.zip
		#NATIVE_PLUGINS+=ffmpeg
	endif

$(AV_BASE)$(AV_VER)-win32.zip:
	mkdir -p $(AV_BASE)
	cd $(AV_BASE) && wget -N $(AV_URL32_BIN)
	mkdir -p $(AV_BASE)bin32 && cd $(AV_BASE)bin32 && $(AV_EXTRACT) ../$(AV_W32_BIN) $(AV_PRE32_BIN)bin/avcodec-*.dll $(AV_PRE32_BIN)bin/avutil-*.dll $(AV_PRE32_BIN)bin/swresample-*.dll $(AV_PRE32_BIN)bin/avformat-*.dll $(AV_PRE32_BIN)bin/swscale-*.dll && cd -
	zip -j9 $@ $(AV_BASE)bin32/*.dll

$(AV_BASE)$(AV_VER)-win64.zip:
	mkdir -p $(AV_BASE)
	cd $(AV_BASE) && wget -N $(AV_URL64_BIN)
	mkdir -p $(AV_BASE)bin64 && cd $(AV_BASE)bin64 && $(AV_EXTRACT) ../$(AV_W64_BIN) $(AV_PRE64_BIN)bin/avcodec-*.dll $(AV_PRE64_BIN)bin/avutil-*.dll $(AV_PRE64_BIN)bin/swresample-*.dll $(AV_PRE64_BIN)bin/avformat-*.dll $(AV_PRE64_BIN)bin/swscale-*.dll && cd -
	zip -j9 $@ $(AV_BASE)bin64/*.dll

	ifneq ($(FFMPEG_ZIP),)
	$(FFMPEG_ZIP): $(AV_BASE)$(AV_VER)-$(FTE_TARGET).zip
		cp $(AV_BASE)$(AV_VER)-$(FTE_TARGET).zip $@
	endif

$(AV_BASE)libavformat/avformat.h:
	mkdir -p $(AV_BASE)
	cd $(AV_BASE) && wget -N $(AV_URL32_DEV)
	mkdir -p $(AV_BASE)libavformat && cd $(AV_BASE)libavformat && $(AV_EXTRACT) ../$(AV_W32_DEV) $(AV_PRE32_DEV)include/libavformat/* && cd -
	mkdir -p $(AV_BASE)libavcodec  && cd $(AV_BASE)libavcodec  && $(AV_EXTRACT) ../$(AV_W32_DEV) $(AV_PRE32_DEV)include/libavcodec/*  && cd -
	mkdir -p $(AV_BASE)libavutil   && cd $(AV_BASE)libavutil   && $(AV_EXTRACT) ../$(AV_W32_DEV) $(AV_PRE32_DEV)include/libavutil/*   && cd -
	mkdir -p $(AV_BASE)libswscale  && cd $(AV_BASE)libswscale  && $(AV_EXTRACT) ../$(AV_W32_DEV) $(AV_PRE32_DEV)include/libswscale/*  && cd -
	mkdir -p $(AV_BASE)lib32 && cd $(AV_BASE)lib32 && $(AV_EXTRACT) ../$(AV_W32_DEV) $(AV_PRE32_DEV)lib/avformat.lib $(AV_PRE32_DEV)lib/avcodec.lib $(AV_PRE32_DEV)lib/avutil.lib $(AV_PRE32_DEV)lib/swscale.lib && cd -
	cd $(AV_BASE) && wget -N $(AV_URL64_DEV)
	mkdir -p $(AV_BASE)lib64 && cd $(AV_BASE)lib64 && $(AV_EXTRACT) ../$(AV_W64_DEV) $(AV_PRE64_DEV)lib/avformat.lib $(AV_PRE64_DEV)lib/avcodec.lib $(AV_PRE64_DEV)lib/avutil.lib $(AV_PRE64_DEV)lib/swscale.lib && cd -

distclean:
	rm $(AV_BASE)libavformat/avformat.h
	rm $(AV_BASE)$(AV_VER)-win32.zip
	rm $(AV_BASE)$(AV_VER)-win64.zip

$(PLUG_PREFIX)ffmpeg$(PLUG_NATIVE_EXT): $(AV_DEP) $(AVPLUG_OBJS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -s -o $@ -shared $(PLUG_CFLAGS) $(AV_CFLAGS) $(AVPLUG_OBJS) $(PLUG_DEFFILE) $(PLUG_LDFLAGS) $(AV_LDFLAGS)
	$(call EMBEDMETA,ffmpeg,$@,FFMPEG Video Decoding Plugin,Provides support for more audio formats as well as video playback and better capture support.)


else # NIX
$(PLUG_PREFIX)ffmpeg$(PLUG_NATIVE_EXT):
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -s -o $@ -shared $(PLUG_CFLAGS) $(AV_CFLAGS) $(AVPLUG_OBJS) $(PLUG_DEFFILE) $(PLUG_LDFLAGS) $(AV_LDFLAGS)
	$(call EMBEDMETA,ffmpeg,$@,FFMPEG Video Decoding Plugin,Provides support for more audio formats as well as video playback and better capture support.)
endif

######################################

######################################
#small script for ode
#FIXME: ode fails to compile under cygwin
#FIXME: race condition if you try compiling debug+release at the same time, as this makefile is invoked twice by the engine's one
ODE_ARCH=$(FTE_TARGET)
ifeq ($(ODE_ARCH),)
  ODE_ARCH=unknown
endif
ODE_VER=0.16.5
ODE_URL=https://bitbucket.org/odedevs/ode/downloads/ode-$(ODE_VER).tar.gz
ODE_BASE=$(ARCHLIBS)/ode-$(ODE_VER)_$(ODE_ARCH)/
ODE_LIB=$(ODE_BASE)ode-$(ODE_VER)/ode/src/.libs/libode.a
$(OUT_DIR)/../ode-$(ODE_VER).tar.gz:
	mkdir -p $(ODE_BASE)
	cd $(OUT_DIR)/.. && wget -N $(ODE_URL)
$(ODE_LIB): $(OUT_DIR)/../ode-$(ODE_VER).tar.gz
	mkdir -p $(ODE_BASE) && cd $(ODE_BASE) && tar xvfz $<
	cd $(ODE_BASE)ode-$(ODE_VER)/ && ./bootstrap && ./configure --enable-double-precision --disable-demos --without-x --with-pic CC="$(CC) $(PLUG_CXXFLAGS)" CXX="$(CC) $(PLUG_CXXFLAGS)" --host=`$(CC) -dumpmachine` && $(MAKE)

ODE_FILES=../engine/common/com_phys_ode.c ../engine/common/mathlib.c plugin.c $(ODE_LIB)
$(PLUG_PREFIX)ode$(PLUG_NATIVE_EXT): $(ODE_FILES)
	$(CC) -flto -s $(BASE_CFLAGS) $(CFLAGS) -Os -DFTEPLUGIN -DODE_STATIC -o $@ -shared $(PLUG_CFLAGS) -I$(ODE_BASE)ode-$(ODE_VER)/include $(ODE_FILES) $(PLUG_DEFFILE) $(PLUG_LDFLAGS) -static-libgcc `$(CC) -print-file-name=libstdc++.a` -lpthread
	$(call EMBEDMETA,ode,$@,ODE Physics,Provides Rigid Body Physics behaviours.)
#NATIVE_PLUGINS+=ode
######################################

######################################
ifneq ($(CMAKERULES),)
BULLET_CFLAGS+=-static-libstdc++
PLUG_CMAKE+= -DCMAKE_TOOLCHAIN_FILE="$(CMAKERULES)" 
$(CMAKERULES):
	echo "set(CMAKE_SYSTEM_NAME Windows)" > $@
	echo "set(TOOLCHAIN_PREFIX `$(CC) -dumpmachine`)" >> $@
	# cross compilers to use for C, C++ and Fortran
	echo "set(CMAKE_C_COMPILER $(CC))" >> $@
	echo "set(CMAKE_CXX_COMPILER $(CXX))" >> $@
	echo "set(CMAKE_RC_COMPILER $${TOOLCHAIN_PREFIX}-windres)" >> $@
	# target environment on the build host system
	echo "set(CMAKE_FIND_ROOT_PATH /usr/$${TOOLCHAIN_PREFIX})" >> $@
	# modify default behavior of FIND_XXX() commands
	echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> $@
	echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> $@
	echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> $@
endif

BULLET_VER=2.87
BULLET_URL=https://github.com/bulletphysics/bullet3/archive/$(BULLET_VER).tar.gz
BULLET_BASE=$(OUT_DIR)/../bullet3-$(BULLET_VER)_$(FTE_TARGET)/
BULLET_LIBS=	\
	$(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libBulletDynamics.a		\
	$(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libBulletCollision.a	\
	$(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libLinearMath.a
BULLET_CFLAGS+=-I$(BULLET_BASE)bullet3-$(BULLET_VER)/src
$(OUT_DIR)/../bullet3-$(BULLET_VER).tar.gz:
	mkdir -p $(BULLET_BASE)
	wget -N $(BULLET_URL) -O $@

BULLET_LIB=$(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libLinearMath.a
$(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libBulletDynamics.a $(BULLET_BASE)bullet3-$(BULLET_VER)/lib/libBulletCollision.a: $(BULLET_LIB)
$(BULLET_LIB): $(OUT_DIR)/../bullet3-$(BULLET_VER).tar.gz $(CMAKERULES)
	mkdir -p $(BULLET_BASE) && cd $(BULLET_BASE) && tar xvfz $<
	rm $(BULLET_BASE)bullet3-$(BULLET_VER)/build3/cmake/FindPythonLibs.cmake	#cmake is a pile of shite and fails at cross compiling. oh well, we didn't want any python stuff anyway.
	cd $(BULLET_BASE)bullet3-$(BULLET_VER)/ && cmake $(PLUG_CMAKE) -DBUILD_DEMOS:BOOL=OFF -DBUILD_EXTRAS:BOOL=OFF -DLIBRARY_OUTPUT_PATH=$(BULLET_BASE)bullet3-$(BULLET_VER)/lib . && $(MAKE) LinearMath BulletDynamics BulletCollision
#./configure --enable-double-precision --disable-demos --without-x CXX="$(CC)" CFLAGS="$(PLUG_CFLAGS)" CXXFLAGS="$(PLUG_CXXFLAGS)" --host=`$(CC) -dumpmachine` && make


$(PLUG_PREFIX)bullet$(PLUG_NATIVE_EXT): bullet/bulletplug.cpp plugin.c $(BULLET_LIBS)
	$(CXX) $(BASE_CXXFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS) $(BULLET_CFLAGS)
	$(call EMBEDMETA,bullet,$@,Bullet Physics Plugin,Provides Rigid Body Physics.)
#NATIVE_PLUGINS+=bullet
######################################

######################################
#Regarding CEF Versions: Cef 2526 is reportedly the most recent _WORKING_ version of libcef. Later versions have screwed webgl etc, and are just generally unstable.
#However, that's now impossible to get hold of since the old cefbuilds server went down. New builds are hosted by spotify and they're all randomly broken, so we might as well just use whatever seems fairly recent.

#WARNING: Changing CEF_VER requires updating downloadables.php etc
ifeq ($(FTE_TARGET),win32)
    CEF_ARCH=windows32
endif
ifeq ($(FTE_TARGET),win64)
    CEF_ARCH=windows64
endif
ifeq ($(FTE_TARGET),linux32)
    CEF_ARCH=linux32
endif
ifeq ($(FTE_TARGET),linux64)
    CEF_ARCH=linux64
endif
#ifeq ($(FTE_TARGET),macosx64)
    #CEF_ARCH=macosx64
#endif
CEF_VER=95.7.14+g9f72f35+chromium-95.0.4638.69
CEF_NAME=cef_binary_$(CEF_VER)_$(CEF_ARCH)_minimal
CEF_URL=https://cef-builds.spotifycdn.com/cef_binary_$(CEF_VER)_$(CEF_ARCH)_minimal.tar.bz2

ifneq ($(CEF_ARCH),)
cef/$(CEF_NAME)/include/cef_version.h:
	cd cef && wget -N $(CEF_URL)
	cd cef && tar -xjf $(CEF_NAME).tar.bz2
cef/$(CEF_NAME)/rel.zip: cef/$(CEF_NAME)/include/cef_version.h
	cd cef/$(CEF_NAME)/Release && zip -9 ../rel.zip *.dll *.bin
	cd cef/$(CEF_NAME)/Resources && zip -r9 ../rel.zip .
$(OUT_DIR)/cef_$(CEF_VER).zip: cef/$(CEF_NAME)/rel.zip
	cp cef/$(CEF_NAME)/rel.zip $@

CEF_SOURCES=cef/cef.c plugin.c
$(PLUG_PREFIX)cef$(PLUG_NATIVE_EXT): $(CEF_SOURCES) $(OUT_DIR)/cef_$(CEF_VER).zip cef/$(CEF_NAME)/include/cef_version.h
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $(PLUG_PREFIX)cef$(PLUG_NATIVE_EXT) -shared $(PLUG_CFLAGS) -Icef/$(CEF_NAME) $(CEF_SOURCES) $(PLUG_DEFFILE) $(PLUG_LDFLAGS) -Wl,-rpath,. $(PLUG_LDFLAGS_DL)
	$(call EMBEDMETA,cef,$@,libcef Browser Plugin,This plugin provides support for an in-game web browser.)
#NATIVE_PLUGINS+=cef
else
.PHONEY: $(PLUG_PREFIX)cef$(PLUG_NATIVE_EXT)
$(PLUG_PREFIX)cef$(PLUG_NATIVE_EXT):
	@echo cef plugin not supported on this arch - $(FTE_TARGET) - $(CEF_ARCH)
endif

######################################
#quake3

BOTLIBFILES=quake3/botlib/be_aas_bspq3.c		\
			quake3/botlib/be_aas_cluster.c	\
			quake3/botlib/be_aas_debug.c		\
			quake3/botlib/be_aas_entity.c	\
			quake3/botlib/be_aas_file.c		\
			quake3/botlib/be_aas_main.c		\
			quake3/botlib/be_aas_move.c		\
			quake3/botlib/be_aas_optimize.c	\
			quake3/botlib/be_aas_reach.c		\
			quake3/botlib/be_aas_routealt.c	\
			quake3/botlib/be_aas_route.c		\
			quake3/botlib/be_aas_sample.c	\
			quake3/botlib/be_ai_char.c		\
			quake3/botlib/be_ai_chat.c		\
			quake3/botlib/be_ai_gen.c	\
			quake3/botlib/be_ai_goal.c	\
			quake3/botlib/be_ai_move.c	\
			quake3/botlib/be_ai_weap.c	\
			quake3/botlib/be_ai_weight.c	\
			quake3/botlib/be_ea.c		\
			quake3/botlib/be_interface.c	\
			quake3/botlib/l_crc.c		\
			quake3/botlib/l_libvar.c		\
			quake3/botlib/l_log.c		\
			quake3/botlib/l_memory.c		\
			quake3/botlib/l_precomp.c	\
			quake3/botlib/l_script.c		\
			quake3/botlib/l_struct.c		\
			quake3/botlib/standalone.c
QUAKE3FILES=$(BOTLIBFILES)	\
			plugin.c		\
			quake3/clq3_cg.c		\
			quake3/clq3_ui.c		\
			quake3/clq3_parse.c	\
			quake3/svq3_game.c	\
			quake3/q3common.c
$(PLUG_PREFIX)quake3$(PLUG_NATIVE_EXT): ${QUAKE3FILES}
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -DMULTITHREAD -DBOTLIB -DBOTLIB_STATIC -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS_ZLIB) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,quake3,$@,Quake3 Compat,Quake3 Gamecode Compatibility)
NATIVE_PLUGINS+=quake3

######################################

#for custom/private plugins...
-include Makefile.private

#small plugins with simpler build rules...

######################################
#Mostly for a joke.
$(PLUG_PREFIX)mpq$(PLUG_NATIVE_EXT): mpq/fs_mpq.c mpq/blast.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Impq $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS_ZLIB) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,mpq,$@,MPQ Archives,Provides support for Blizzard's .mpq archive format used in eg Diablo 2)
#NATIVE_PLUGINS+=mpq
######################################

######################################
# XMPP aka Jabber chat-protocol support. requires manual account configuration.
$(PLUG_PREFIX)xmpp$(PLUG_NATIVE_EXT): jabber/jabberclient.c jabber/jingle.c jabber/sift.c jabber/xml.c plugin.c ../engine/common/sha1.c ../engine/common/sha2.c emailnot/md5.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Ijabber $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS) $(LIBRESOLV)
	$(call EMBEDMETA,xmpp,$@,XMPP Chat,A slightly more modern alternative to IRC. Requires manual account configuration.)
NATIVE_PLUGINS+=xmpp
######################################

######################################
# Quake Injector plugin.
$(PLUG_PREFIX)qi$(PLUG_NATIVE_EXT): qi/qi.c jabber/xml.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Ijabber $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,qi,$@,Quake-Injector Plugin,Provides easy access to the Quaddicted mod database.)
NATIVE_PLUGINS+=qi
######################################

######################################
# Internet Relay Chat (IRC) plugin.
$(PLUG_PREFIX)irc$(PLUG_NATIVE_EXT): irc/ircclient.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Iirc $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,irc,$@,IRC Chat,Internet Relay Chat plugin - requires extra server/nick/channel configuration)
NATIVE_PLUGINS+=irc
######################################

######################################
#OpenXR plugin, for use with VR headsets.
#the tar.gz has a stupid name
OPENXRVER=1.0.33
$(ARCHLIBS)/openxr/openxr.h:
	cd $(ARCHLIBS)/.. && (test -f release-$(OPENXRVER).tar.gz || wget https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-$(OPENXRVER).tar.gz)
	cd $(ARCHLIBS) && tar -xvzf ../release-$(OPENXRVER).tar.gz --strip-components=2 OpenXR-SDK-release-$(OPENXRVER)/include/openxr/
$(PLUG_PREFIX)openxr$(PLUG_NATIVE_EXT): openxr.c plugin.c $(ARCHLIBS)/openxr/openxr.h
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS) -DXR_NO_PROTOTYPES `$(PKGCONFIG) --cflags openxr` -DGLQUAKE -DVKQUAKE -DD3D11QUAKE
	$(call EMBEDMETA,openxr,$@,OpenXR Support,Enables the use of Virtual Reality headsets and inputs.)
NATIVE_PLUGINS+=openxr
######################################

######################################
#for compat with ezquake
$(PLUG_PREFIX)ezhud$(PLUG_NATIVE_EXT): ezhud/ezquakeisms.c ezhud/hud.c ezhud/hud_common.c ezhud/hud_editor.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Iezhud $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,ezhud,$@,EzHud Plugin,Provides compat with ezquake's hud scripts.)
NATIVE_PLUGINS+=ezhud
######################################

######################################
#not really relevant now that gltf was made an internal plugin
$(PLUG_PREFIX)models$(PLUG_NATIVE_EXT): models/gltf.c models/exportiqm.c models/models.c plugin.c ../engine/common/json.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) -Imodels $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,models,$@,Models Plugin,Kinda redundant now that the engine has gltf2 loading)
#NATIVE_PLUGINS+=models
######################################

######################################
#Openssl crypto plugin, to replace microsoft's shoddy dtls implementation. could also be useful on the BSDs, yay system components?

ifeq ($(FTE_TARGET),win32)
    OSSL_ARCH=mingw
endif
ifeq ($(FTE_TARGET),win64)
    OSSL_ARCH=mingw64
endif
#statically link openssl on the above systems (instead of depending on system libs)
ifneq ($(OSSL_ARCH),)
OSSL_VERSION=3.0.1
#../engine/libs-$(ARCH)/openssl-$(OSSL_VERSION).tar.gz:
../engine/openssl-$(OSSL_VERSION).tar.gz:
	wget -O $@ -N https://github.com/openssl/openssl/archive/refs/tags/openssl-$(OSSL_VERSION).tar.gz
$(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION)/libssl.a $(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION)/libcrypto.a: ../engine/openssl-$(OSSL_VERSION).tar.gz
	(cd $(ARCHLIBS) && tar xvfz ../openssl-$(OSSL_VERSION).tar.gz)
	(cd $(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION) && CFLAGS=-Os ./Configure --release no-filenames no-legacy no-shared no-stdio no-asm $(OSSL_ARCH) --cross-compile-prefix=$(ARCH)- && $(MAKE))
$(PLUG_PREFIX)openssl$(PLUG_NATIVE_EXT): $(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION)/libssl.a $(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION)/libcrypto.a

#we should be using openssl's no-sock option, but that also disables core dtls functionality (despite us using our own BIOs).
OPENSSL_LDCFLAGS=-I$(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION)/include -L$(ARCHLIBS)/openssl-openssl-$(OSSL_VERSION) -lssl -lcrypto -lws2_32
OPENSSL_AVAILABLE=1
endif

OPENSSL_LDCFLAGS?=`$(PKGCONFIG) --libs --cflags openssl`
OPENSSL_AVAILABLE?=$(shell $(OPENSSLPKGPATH) $(PKGCONFIG) --atleast-version=3.0.0 openssl && echo 1)
$(PLUG_PREFIX)openssl$(PLUG_NATIVE_EXT): net_ssl_openssl.c plugin.c
	$(CC) -s $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS) $(OPENSSL_LDCFLAGS)
	$(call EMBEDMETA,openssl,$@,OpenSSL,Provides OpenSSL support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar.)
#OpenSSL 3.0.0 is apache2 and thus GPL3-compatible, earlier versions are NOT GPL-compatible at all, so only enable it if its okay.
#(you can still force it, but don't distribute)
ifeq ($(OPENSSL_AVAILABLE),1)
NATIVE_PLUGINS+=openssl
endif
######################################

######################################
#for compat with half-life 2's file formats
$(PLUG_PREFIX)hl2$(PLUG_NATIVE_EXT): hl2/fs_vpk.c hl2/img_vtf.c hl2/mod_hl2.c hl2/mat_vmt.c hl2/mod_vbsp.c hl2/hl2.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -DMULTITHREAD -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,hl2,$@,HL2 Formats,Provides support for various formats used by Valve's Source engine.)
NATIVE_PLUGINS+=hl2
######################################

######################################
#for compat with cod's file formats
$(PLUG_PREFIX)cod$(PLUG_NATIVE_EXT): cod/codmod.c cod/codbsp.c cod/codmat.c cod/codiwi.c plugin.c
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -DFTEPLUGIN -DMULTITHREAD -o $@ -shared $(PLUG_CFLAGS) $^ $(PLUG_DEFFILE) $(PLUG_LDFLAGS)
	$(call EMBEDMETA,cod,$@,CoD Formats,Provides support for various formats used by Call of Duty.)
NATIVE_PLUGINS+=cod
######################################



all: $(foreach FOO,$(NATIVE_PLUGINS), $(PLUG_PREFIX)$(FOO)$(PLUG_NATIVE_EXT))

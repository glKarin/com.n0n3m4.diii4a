STATIC_LINKING := 0
AR             := ar
NO_STORE       := 1
HAVE_LZMA      := 0

ifneq ($(V),1)
   Q := @
endif

ifneq ($(SANITIZER),)
   CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS)
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
ifeq ($(shell uname -p),arm)
	arch = arm
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

CORE_DIR    += ../..
TARGET_NAME := ecwolf
ifneq (,$(findstring msvc,$(platform)))
LIBM		    =
else
LIBM		    = -lm
endif
DISABLE_ADLIB = 0


ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifneq (,$(findstring unix,$(platform)))
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   LIBS := -ldl -lpthread
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif

ifeq ($(UNIVERSAL),1)
ifeq ($(ARCHFLAGS),)
   ARCHFLAGS = -arch i386 -arch x86_64
ifeq ($(archs),arm)
   ARCHFLAGS = -arch arm64
endif
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
endif
endif
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif

else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   ifeq ($(IOSSDK),)
      IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
   endif
   DEFINES := -DIOS
   MINVERSION = -miphoneos-version-min=8.0
   CC = cc -arch arm64 -isysroot $(IOSSDK)
   CXX = c++ -arch arm64 -isysroot $(IOSSDK)
   CFLAGS   += $(MINVERSION)
   CXXFLAGS += $(MINVERSION)
   LDFLAGS  += $(MINVERSION)
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifeq ($(platform), tvos-arm64)
	TARGET := $(TARGET_NAME)_libretro_tvos.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
   MINVERSION = -mappletvos-version-min=11.0
	CFLAGS += -DIOS
	ifeq ($(IOSSDK),)
		IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
	endif
   CC = cc -arch arm64 -isysroot $(IOSSDK)
   CXX = c++ -arch arm64 -isysroot $(IOSSDK)
   CFLAGS += $(MINVERSION)
   CXXFLAGS += $(MINVERSION)
   LDFLAGS += $(MINVERSION)
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
   CC = qcc -Vgcc_ntoarmv7le
   CXX = QCC -Vgcc_ntoarmv7le
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -r
   STATIC_LINKING=1
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   AR = /opt/emsdk/upstream/emscripten/emar
else ifeq ($(platform), ps2)
   TARGET := $(TARGET_NAME)_libretro_ps2.a
   CC = mips64r5900el-ps2-elf-gcc
   CXX = mips64r5900el-ps2-elf-g++
   AR = mips64r5900el-ps2-elf-ar
   COMMON_CFLAGS += -G0 -DPS2
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
   DISABLE_ADLIB = 1
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_psp1.a
   CC = psp-gcc
   CXX = psp-g++
   AR = psp-ar
   COMMON_CFLAGS += -DPSP -G0
   COMMON_CFLAGS += -I$(shell psp-config --pspsdk-path)/include
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   STATIC_LINKING = 1
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_vita.a
   CC = arm-vita-eabi-gcc
   CXX = arm-vita-eabi-g++
   AR = arm-vita-eabi-ar
   DEFINES += -DVITA=1
   COMMON_CFLAGS += $(DEFINES)  -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
# Nintendo WiiU
else ifeq ($(platform), wiiu)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   PLATFORM_DEFINES := -DGEKKO -DHW_RVL -DWIIU -mcpu=750 -meabi -mhard-float
   PLATFORM_DEFINES := -ffunction-sections -fdata-sections -D__wiiu__ -D__wut__
   PLATFORM_DEFINES += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   COMMON_CFLAGS += $(PLATFORM_DEFINES)
   STATIC_LINKING=1
# DOS
else ifeq ($(platform), dos)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = i586-pc-msdosdjgpp-gcc
	AR = i586-pc-msdosdjgpp-ar
	CXX = i586-pc-msdosdjgpp-g++
	COMMON_CFLAGS += -march=i386
	STATIC_LINKING=1

else ifeq ($(platform), wii)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   COMMON_CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DGAMECUBE
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
else ifeq ($(platform), ngc)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   COMMON_CFLAGS += -DGEKKO -DHW_DOL -mogc -mcpu=750 -meabi -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   CXXFLAGS += -Wl,-q -Wall -O2
   STATIC_LINKING = 1
# Lightweight PS3 Homebrew SDK
else ifeq ($(platform), psl1ght)
   EXT=a
   TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
   CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   CXX = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
   CC_AS = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
   AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D__PSL1GHT__ -mcpu=cell
   STATIC_LINKING = 1
# Nintendo Switch (libnx)
else ifeq ($(platform), libnx)
    include $(DEVKITPRO)/libnx/switch_rules
    EXT=a
    TARGET := $(TARGET_NAME)_libretro_$(platform).$(EXT)
    DEFINES := -DSWITCH=1 -D__SWITCH__=1 -DHAVE_LIBNX=1
    COMMON_CFLAGS	:=	 $(DEFINES) -g -fPIE -I$(LIBNX)/include/ -I$(PORTLIBS)/include/ -ffunction-sections -fdata-sections -ftls-model=local-exec
    COMMON_CFLAGS += $(INCDIRS) -I$(CORE_DIR)/zlib -I$(CORE_DIR)/libogg/include -I$(CORE_DIR)/libvorbis/include \
		-I$(CORE_DIR)/jpeg-8c -fvisibility=hidden
    COMMON_CFLAGS	+= -march=armv8-a -mtune=cortex-a57 -mtp=soft -mcpu=cortex-a57+crc+fp+simd -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
    STATIC_LINKING = 1
# CTR (3DS)
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	DEFINES += -D_3DS -DARM11 -march=armv6k -mtune=mpcore -mfloat-abi=hard
	COMMON_CFLAGS += $(DEFINES) -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	STATIC_LINKING = 1
# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/lib/amd64")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

	WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')
	WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\x64")

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir);$(CORE_DIR)/src/libretro/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSDKLibDir)

	TARGET := $(TARGET_NAME)_libretro.dll
	LDFLAGS += -DLL
	LIBS = AdvAPI32.lib

# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

        WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')
        WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -io '[A-Z]:\\.*')

	WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
	WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
	WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

	INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
	export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir);$(CORE_DIR)/src/libretro/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSDKLibDir)

	TARGET := $(TARGET_NAME)_libretro.dll
	LDFLAGS += -DLL
	LIBS = AdvAPI32.lib

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

	CFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1
	CXXFLAGS += -wd4711 -wd4514 -wd4820 -wd4668 -wd4242 -wd4244 -wd4365 -DNO_VA_COPY=1

	PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
	BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

        WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\MicrosoftSDK\InstalledSDKs\8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3" -v "Install Dir" | grep -o '[A-Z]:\\.*')

        WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
        WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

        INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKAtlIncludeDir);$(WindowsSDKCrtIncludeDir);$(WindowsSDKGlIncludeDir);$(WindowsSDKMfcIncludeDir);libretro/msvc/msvc-2005
        LIB := $(LIB);$(WindowsSDKLibDir)

	export INCLUDE := $(INCLUDE);$(INETSDK)/Include;$(CORE_DIR)/src/libretro/libretro-common/include/compat/msvc
	export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL
	CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
	LIBS = AdvAPI32.lib
# RS90
else ifeq ($(platform), rs90)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/rs90-toolchain/usr/bin/mipsel-linux-gcc
   CXX = /opt/rs90-toolchain/usr/bin/mipsel-linux-g++
   AR = /opt/rs90-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,-version-script=$(CORE_DIR)/link.T
   PLATFORM_DEFINES := -DCC_RESAMPLER -DCC_RESAMPLER_NO_HIGHPASS
   COMMON_CFLAGS += -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DRS90=1
   DISABLE_ADLIB = 1

# GCW0
else ifeq ($(platform), gcw0)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
   CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
   AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,-no-undefined
   COMMON_CFLAGS += -march=mips32 -mtune=mips32r2 -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
   LIBS = -lm
# RETROFW
else ifeq ($(platform), retrofw)
	EXT ?= so
	TARGET := $(TARGET_NAME)_libretro.$(EXT)
	CC = /opt/retrofw-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/retrofw-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/retrofw-toolchain/usr/bin/mipsel-linux-ar
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
	COMMON_CFLAGS += -ffast-math -march=mips32 -mtune=mips32 -mhard-float
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
	LIBS = -lm
# MIYOO
else ifeq ($(platform), miyoo)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,-version-script=link.T
	CC = /opt/miyoo/usr/bin/arm-linux-gcc
	CXX = /opt/miyoo/usr/bin/arm-linux-g++
	AR = /opt/miyoo/usr/bin/arm-linux-ar
	COMMON_CFLAGS += -D_GNU_SOURCE -DUSE_OWN_ADDED_SIZE
	COMMON_CFLAGS += -fomit-frame-pointer -ffast-math -march=armv5te -mtune=arm926ej-s
	COMMON_CFLAGS += -fno-common -ftree-vectorize -funswitch-loops
   COMMON_CFLAGS += -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
else
   CC ?= gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   COMMON_CFLAGS += -DUSE_WINDOWS_DWORD
   SHARED := -shared -static-libgcc -static-libstdc++ -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
ifneq ($(DEBUG),1)
   SHARED += -s
endif
   LDFLAGS += -lwsock32 -lws2_32
endif

HAVE_OPENGL = 1

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   COMMON_CFLAGS += -O0 -g -DDEBUG
else
   COMMON_CFLAGS += -O2
endif

include Makefile.common

OBJECTS := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)

COMPILE_PLATFORM=$(shell uname | sed -e 's/_.*//' | tr '[:upper:]' '[:lower:]' | sed -e 's/\//_/g')
COMPILE_ARCH=$(shell uname -m | sed -e 's/i.86/x86/' | sed -e 's/^arm.*/arm/')

ifeq ($(COMPILE_PLATFORM),cygwin)
  PLATFORM=mingw32
endif

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM

ifeq ($(PLATFORM),mingw32)
  MINGW=1
endif
ifeq ($(PLATFORM),mingw64)
  MINGW=1
endif

ifeq ($(COMPILE_ARCH),i86pc)
  COMPILE_ARCH=x86
endif

ifeq ($(COMPILE_ARCH),amd64)
  COMPILE_ARCH=x86_64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=x86_64
endif

ifeq ($(COMPILE_ARCH),powerpc)
  COMPILE_ARCH=ppc
endif
ifeq ($(COMPILE_ARCH),powerpc64)
  COMPILE_ARCH=ppc64
endif

ifeq ($(COMPILE_ARCH),axp)
  COMPILE_ARCH=alpha
endif

TOPDIR ?= $(CURDIR)

COMMON_CFLAGS   += -Wall -DLIBRETRO -D__LIBRETRO__ $(fpic) -DCORE=ON -DSDL2=ON -DBASE=ON -DUSE_FILE32API -I$(TOPDIR) -I$(TOPDIR)/sys/libretro/config

ifeq (,$(findstring msvc,$(platform)))
  COMMON_CFLAGS += -ffast-math -fno-strict-aliasing -fvisibility=hidden -fomit-frame-pointer -Wno-sign-compare -Wno-switch -Wno-format-security
  CXXFLAGS += -fpermissive -std=gnu++98
ifneq ($(platform), emscripten)
  COMMON_CFLAGS += -fno-unsafe-math-optimizations
endif
endif

COMMON_CFLAGS += -DNO_STORE

CFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += $(COMMON_CFLAGS)

CFLAGS     += $(INCFLAGS)
CXXFLAGS   += $(INCFLAGS)

OBJOUT   = -o 
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
ifneq (,$(findstring msvc,$(platform)))
	link.exe -out:$@ $(OBJECTS) $(LDFLAGS) $(LIBS)
else
	$(CXX) $(fpic) $(SHARED) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
endif
endif

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(fpic) -c $(OBJOUT)$@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(fpic) -c $(OBJOUT)$@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'

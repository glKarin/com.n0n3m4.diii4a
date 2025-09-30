# ----------------------------------------------------- #
# Makefile for the Vulkan renderer lib for Quake II     #
#                                                       #
# Just type "make" to compile the                       #
#  - Vulkan renderer lib (ref_vk.so / rev_vk.dll)       #
#                                                       #
# Dependencies:                                         #
# - SDL2                                                #
# - Vulkan headers                                      #
#                                                       #
# Platforms:                                            #
# - FreeBSD                                             #
# - Linux                                               #
# - Mac OS X                                            #
# - OpenBSD                                             #
# - Windows                                             #
# ----------------------------------------------------- #

# User configurable options
# -------------------------

# Builds with SDL 3 instead of SDL 2.
WITH_SDL3:=no

# ----------

# Detect the OS
ifdef SystemRoot
YQ2_OSTYPE ?= Windows
else
YQ2_OSTYPE ?= $(shell uname -s)
endif

# Special case for MinGW
ifneq (,$(findstring MINGW,$(YQ2_OSTYPE)))
YQ2_OSTYPE := Windows
endif

# Detect the architecture
ifeq ($(YQ2_OSTYPE), Windows)
ifdef MINGW_CHOST
ifeq ($(MINGW_CHOST), x86_64-w64-mingw32)
YQ2_ARCH ?= x86_64
else # i686-w64-mingw32
YQ2_ARCH ?= i386
endif
else # windows, but MINGW_CHOST not defined
ifdef PROCESSOR_ARCHITEW6432
# 64 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITEW6432)
else
# 32 bit Windows
YQ2_ARCH ?= $(PROCESSOR_ARCHITECTURE)
endif
endif # windows but MINGW_CHOST not defined
else
ifneq ($(YQ2_OSTYPE), Darwin)
# Normalize some abiguous YQ2_ARCH strings
YQ2_ARCH ?= $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/arm64/aarch64/' -e 's/^arm.*/arm/')
else
YQ2_ARCH ?= $(shell uname -m)
endif
endif

# On Windows / MinGW $(CC) is undefined by default.
ifeq ($(YQ2_OSTYPE),Windows)
CC ?= gcc
endif

# Detect the compiler
ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
COMPILER := clang
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
else ifeq ($(shell $(CC) -v 2>&1 | grep -c -E "(gcc version|gcc-Version)"), 1)
COMPILER := gcc
COMPILERVER := $(shell $(CC)  -dumpversion | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/')
else
COMPILER := unknown
endif

# ASAN includes DEBUG
ifdef ASAN
DEBUG=1
endif

# UBSAN includes DEBUG
ifdef UBSAN
DEBUG=1
endif

# ----------

# Base CFLAGS. These may be overridden by the environment.
# Highest supported optimizations are -O2, higher levels
# will likely break this crappy code.
ifdef DEBUG
CFLAGS ?= -O0 -g -Wall -pipe -DDEBUG
ifdef ASAN
override CFLAGS += -fsanitize=address -DUSE_SANITIZER
endif
ifdef UBSAN
override CFLAGS += -fsanitize=undefined -DUSE_SANITIZER
endif
else
CFLAGS ?= -O2 -Wall -pipe -fomit-frame-pointer
endif

# Always needed are:
#  -fno-strict-aliasing since the source doesn't comply
#   with strict aliasing rules and it's next to impossible
#   to get it there...
#  -fwrapv for defined integer wrapping. MSVC6 did this
#   and the game code requires it.
#  -fvisibility=hidden to keep symbols hidden. This is
#   mostly best practice and not really necessary.
override CFLAGS += -fno-strict-aliasing -fwrapv -fvisibility=hidden

# -MMD to generate header dependencies. Unsupported by
#  the Clang shipped with OS X.
ifneq ($(YQ2_OSTYPE), Darwin)
override CFLAGS += -MMD
endif

# OS X architecture.
ifeq ($(YQ2_OSTYPE), Darwin)
override CFLAGS += -arch $(YQ2_ARCH)
endif

# ----------

# ARM needs a sane minimum architecture. We need the `yield`
# opcode, arm6k is the first iteration that supports it. arm6k
# is also the first Raspberry PI generation and older hardware
# is likely too slow to run the game. We're not enforcing the
# minimum architecture, but if you're build for something older
# like arm5 the `yield` opcode isn't compiled in and the game
# (especially q2ded) will consume more CPU time than necessary.
ifeq ($(YQ2_ARCH), arm)
CFLAGS += -march=armv6k
endif

# ----------

# Switch of some annoying warnings.
ifeq ($(COMPILER), clang)
	# -Wno-missing-braces because otherwise clang complains
	#  about totally valid 'vec3_t bla = {0}' constructs.
	override CFLAGS += -Wno-missing-braces
else ifeq ($(COMPILER), gcc)
	# GCC 8.0 or higher.
	ifeq ($(shell test $(COMPILERVER) -ge 80000; echo $$?),0)
	    # -Wno-format-truncation and -Wno-format-overflow
		# because GCC spams about 50 false positives.
		override CFLAGS += -Wno-format-truncation -Wno-format-overflow
	endif
endif

# ----------

# Defines the operating system and architecture
override CFLAGS += -DYQ2OSTYPE=\"$(YQ2_OSTYPE)\" -DYQ2ARCH=\"$(YQ2_ARCH)\"

# ----------

# For reproduceable builds, look here for details:
# https://reproducible-builds.org/specs/source-date-epoch/
ifdef SOURCE_DATE_EPOCH
override CFLAGS += -DBUILD_DATE=\"$(shell date --utc --date="@${SOURCE_DATE_EPOCH}" +"%b %_d %Y" | sed -e 's/ /\\ /g')\"
endif

# ----------

# Using the default x87 float math on 32bit x86 causes rounding trouble
# -ffloat-store could work around that, but the better solution is to
# just enforce SSE - every x86 CPU since Pentium3 supports that
# and this should even improve the performance on old CPUs
ifeq ($(YQ2_ARCH), i386)
override CFLAGS += -msse -mfpmath=sse
endif

# Force SSE math on x86_64. All sane compilers should do this
# anyway, just to protect us from broken Linux distros.
ifeq ($(YQ2_ARCH), x86_64)
override CFLAGS += -mfpmath=sse
endif

# Disable floating-point expression contraction. While this shouldn't be
# a problem for C (only for C++) better be safe than sorry. See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100839 for details.
ifeq ($(COMPILER), gcc)
override CFLAGS += -ffp-contract=off
endif

# ----------

# Extra CFLAGS for SDL.
ifeq ($(WITH_SDL3),yes)
SDLCFLAGS := $(shell pkgconf --cflags sdl3)
SDLCFLAGS += -DUSE_SDL3
else
SDLCFLAGS := $(shell sdl2-config --cflags)
endif

# ----------

# Base include path.
ifeq ($(YQ2_OSTYPE),Linux)
INCLUDE ?= -I/usr/include
else ifeq ($(YQ2_OSTYPE),FreeBSD)
INCLUDE ?= -I/usr/local/include
else ifeq ($(YQ2_OSTYPE),NetBSD)
INCLUDE ?= -I/usr/X11R7/include -I/usr/pkg/include
else ifeq ($(YQ2_OSTYPE),OpenBSD)
INCLUDE ?= -I/usr/local/include
else ifeq ($(YQ2_OSTYPE),Windows)
INCLUDE ?= -I/usr/include
else ifeq ($(YQ2_OSTYPE),Darwin)
MOLTENVK_PATH ?= $(shell brew --prefix molten-vk)
INCLUDE ?= -I$(MOLTENVK_PATH)/libexec/include -I$(MOLTENVK_PATH)/include
endif

# ----------

# Base LDFLAGS. This is just the library path.
ifeq ($(YQ2_OSTYPE),Linux)
LDFLAGS ?= -L/usr/lib
else ifeq ($(YQ2_OSTYPE),FreeBSD)
LDFLAGS ?= -L/usr/local/lib
else ifeq ($(YQ2_OSTYPE),NetBSD)
LDFLAGS ?= -L/usr/X11R7/lib -Wl,-R/usr/X11R7/lib -L/usr/pkg/lib -Wl,-R/usr/pkg/lib
else ifeq ($(YQ2_OSTYPE),OpenBSD)
LDFLAGS ?= -L/usr/local/lib
else ifeq ($(YQ2_OSTYPE),Windows)
LDFLAGS ?= -L/usr/lib
else ifeq ($(YQ2_OSTYPE),Darwin)
LDFLAGS ?= -L/usr/local/lib -L/opt/homebrew/lib
endif

# Link address sanitizer if requested.
ifdef ASAN
override LDFLAGS += -fsanitize=address
endif

# Link undefined behavior sanitizer if requested.
ifdef UBSAN
override LDFLAGS += -fsanitize=undefined
endif

# Required libraries.
ifeq ($(YQ2_OSTYPE),Linux)
LDLIBS ?= -lm -ldl -rdynamic
else ifeq ($(YQ2_OSTYPE),FreeBSD)
LDLIBS ?= -lm
else ifeq ($(YQ2_OSTYPE),NetBSD)
LDLIBS ?= -lm
else ifeq ($(YQ2_OSTYPE),OpenBSD)
LDLIBS ?= -lm
else ifeq ($(YQ2_OSTYPE),Windows)
LDLIBS ?= -lws2_32 -lwinmm -static-libgcc
else ifeq ($(YQ2_OSTYPE), Darwin)
LDLIBS ?= -arch $(YQ2_ARCH)
else ifeq ($(YQ2_OSTYPE), Haiku)
LDLIBS ?= -lm
else ifeq ($(YQ2_OSTYPE), SunOS)
LDLIBS ?= -lm
endif

# ASAN and UBSAN must not be linked
# with --no-undefined. OSX and OpenBSD
# don't support it at all.
ifndef ASAN
ifndef UBSAN
ifneq ($(YQ2_OSTYPE), Darwin)
ifneq ($(YQ2_OSTYPE), OpenBSD)
override LDFLAGS += -Wl,--no-undefined
endif
endif
endif
endif

# ----------

# Extra LDFLAGS for SDL
ifeq ($(WITH_SDL3),yes)
ifeq ($(YQ2_OSTYPE), Darwin)
SDLLDFLAGS := -lSDL3
else
SDLLDFLAGS := $(shell pkgconf --libs sdl3)
endif
else
ifeq ($(YQ2_OSTYPE), Darwin)
SDLLDFLAGS := -lSDL2
else
SDLLDFLAGS := $(shell sdl2-config --libs)
endif
endif

ifeq ($(WITH_SDL3),yes)
# The renderer libs don't need libSDL3main, libmingw32 or -mwindows.
ifeq ($(YQ2_OSTYPE), Windows)
DLL_SDLLDFLAGS = $(subst -mwindows,,$(subst -lmingw32,,$(subst -lSDL3main,,$(SDLLDFLAGS))))
endif
else
# The renderer libs don't need libSDL2main, libmingw32 or -mwindows.
ifeq ($(YQ2_OSTYPE), Windows)
DLL_SDLLDFLAGS = $(subst -mwindows,,$(subst -lmingw32,,$(subst -lSDL2main,,$(SDLLDFLAGS))))
endif
endif

# ----------

# When make is invoked by "make VERBOSE=1" print
# the compiler and linker commands.
ifdef VERBOSE
Q :=
else
Q := @
endif

# ----------

# Phony targets
.PHONY : all

# ----------

# Builds everything
all: ref_vk

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release/*

cleanall:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
ref_vk:
	@echo "===> Building ref_vk.dll"
	${Q}mkdir -p release
	$(MAKE) release/ref_vk.dll

release/ref_vk.dll : LDFLAGS += -shared

else ifeq ($(YQ2_OSTYPE), Darwin)

ref_vk:
	@echo "===> Building ref_vk.dylib"
	${Q}mkdir -p release
	$(MAKE) release/ref_vk.dylib

release/ref_vk.dylib : LDFLAGS += -shared

else # not Windows or Darwin

ref_vk:
	@echo "===> Building ref_vk.so"
	${Q}mkdir -p release
	$(MAKE) release/ref_vk.so

release/ref_vk.so : CFLAGS += -fPIC
release/ref_vk.so : LDFLAGS += -shared

endif # OS specific ref_vk stuff

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) $(SDLCFLAGS) $(INCLUDE) -o $@ $<

# ----------

REFVK_OBJS_ := \
	src/vk/vk_buffer.o \
	src/vk/vk_cmd.o \
	src/vk/vk_common.o \
	src/vk/vk_device.o \
	src/vk/vk_draw.o \
	src/vk/vk_image.o \
	src/vk/vk_light.o \
	src/vk/vk_mesh.o \
	src/vk/vk_model.o \
	src/vk/vk_pipeline.o \
	src/vk/vk_main.o \
	src/vk/vk_misc.o \
	src/vk/vk_surf.o \
	src/vk/vk_shaders.o \
	src/vk/vk_swapchain.o \
	src/vk/vk_validation.o \
	src/vk/vk_warp.o \
	src/vk/vk_util.o \
	src/vk/volk/volk.o \
	src/files/surf.o \
	src/files/models.o \
	src/files/pcx.o \
	src/files/stb.o \
	src/files/wal.o \
	src/files/pvs.o \
	src/common/shared.o \
	src/common/utils.o \
	src/common/md4.o

ifeq ($(YQ2_OSTYPE), Windows)
REFVK_OBJS_ += \
	src/backends/hunk_windows.o
else # not Windows
REFVK_OBJS_ += \
	src/backends/hunk_unix.o
endif

# ----------

# Rewrite pathes to our object directory
REFVK_OBJS = $(patsubst %,build/%,$(REFVK_OBJS_))

# ----------

# Generate header dependencies
REFVK_DEPS= $(REFVK_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(REFVK_DEPS)

# ----------

# release/ref_vk.so
ifeq ($(YQ2_OSTYPE), Windows)
release/ref_vk.dll : $(REFVK_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) $(REFVK_OBJS) $(LDLIBS) $(DLL_SDLLDFLAGS) -o $@
else ifeq ($(YQ2_OSTYPE), Darwin)
release/ref_vk.dylib : $(REFVK_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) $(REFVK_OBJS) $(LDLIBS) $(SDLLDFLAGS) -o $@
else
release/ref_vk.so : $(REFVK_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) $(REFVK_OBJS) $(LDLIBS) $(SDLLDFLAGS) -o $@
endif

# ----------

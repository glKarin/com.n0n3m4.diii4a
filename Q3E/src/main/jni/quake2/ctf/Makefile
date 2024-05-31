# ----------------------------------------------------- #
# Makefile for the CTF game module for Quake II         #
#                                                       #
# Just type "make" to compile the                       #
#  - CTF Game (game.so / game.dll)                      #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theory every one should work              #
#   Yamagi Quake II is recommended.                    #
#                                                       #
# Platforms:                                            #
# - FreeBSD                                             #
# - Linux                                               #
# - Mac OS X                                            #
# - OpenBSD                                             #
# - Windows                                             #
# ----------------------------------------------------- #

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

# ----------

# Base CFLAGS. These may be overridden by the environment.
# Highest supported optimizations are -O2, higher levels
# will likely break this crappy code.
ifdef DEBUG
CFLAGS ?= -O0 -g -Wall -pipe
else
CFLAGS ?= -O2 -Wall -pipe -fomit-frame-pointer
endif

# Always needed are:
#  -fno-strict-aliasing since the source doesn't comply
#   with strict aliasing rules and it's next to impossible
#   to get it there...
#  -fwrapv for defined integer wrapping. MSVC6 did this
#   and the game code requires it.
override CFLAGS += -std=gnu99 -fno-strict-aliasing -fwrapv

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

# Switch off some annoying warnings.
ifeq ($(COMPILER), clang)
	# -Wno-missing-braces because otherwise clang complains
	#  about totally valid 'vec3_t bla = {0}' constructs.
	CFLAGS += -Wno-missing-braces
else ifeq ($(COMPILER), gcc)
	# GCC 8.0 or higher.
	ifeq ($(shell test $(COMPILERVER) -ge 80000; echo $$?),0)
	    # -Wno-format-truncation and -Wno-format-overflow
		# because GCC spams about 50 false positives.
    	CFLAGS += -Wno-format-truncation -Wno-format-overflow
	endif
endif

# ----------

# Defines the operating system and architecture
override CFLAGS += -DYQ2OSTYPE=\"$(YQ2_OSTYPE)\" -DYQ2ARCH=\"$(YQ2_ARCH)\"

# ----------

# For reproducible builds, look here for details:
# https://reproducible-builds.org/specs/source-date-epoch/
ifdef SOURCE_DATE_EPOCH
CFLAGS += -DBUILD_DATE=\"$(shell date --utc --date="@${SOURCE_DATE_EPOCH}" +"%b %_d %Y" | sed -e 's/ /\\ /g')\"
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

# ----------

# Base LDFLAGS.
LDFLAGS ?=

# It's a shared library.
override LDFLAGS += -shared

# Required libraries
ifeq ($(YQ2_OSTYPE), Darwin)
override LDFLAGS += -arch $(YQ2_ARCH)
else ifeq ($(YQ2_OSTYPE), Windows)
override LDFLAGS += -static-libgcc
else
override LDFLAGS += -lm
endif

# ----------

# Builds everything
all: ctf

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
.PHONY : all clean ctf

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The ctf game
ifeq ($(YQ2_OSTYPE), Windows)
ctf:
	@echo "===> Building game.dll"
	$(Q)mkdir -p release
	$(MAKE) release/game.dll
else ifeq ($(YQ2_OSTYPE), Darwin)
ctf:
	@echo "===> Building game.dylib"
	${Q}mkdir -p release
	$(MAKE) release/game.dylib
else
ctf:
	@echo "===> Building game.so"
	$(Q)mkdir -p release
	$(MAKE) release/game.so

release/game.so : CFLAGS += -fPIC
endif

build/%.o: %.c
	@echo "===> CC $<"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

# ----------

CTF_OBJS_ = \
	src/g_ai.o \
	src/g_chase.o \
	src/g_cmds.o \
	src/g_combat.o \
	src/g_ctf.o \
	src/g_func.o \
	src/g_items.o \
	src/g_main.o \
	src/g_misc.o \
	src/g_monster.o \
	src/g_phys.o \
	src/g_save.o \
	src/g_spawn.o \
	src/g_svcmds.o \
	src/g_target.o \
	src/g_trigger.o \
	src/g_utils.o \
	src/g_weapon.o \
	src/menu/menu.o \
	src/monster/move.o \
	src/player/client.o \
	src/player/hud.o \
	src/player/trail.o \
	src/player/view.o \
	src/player/weapon.o \
	src/shared/shared.o

# ----------

# Rewrite pathes to our object directory
CTF_OBJS = $(patsubst %,build/%,$(CTF_OBJS_))

# ----------

# Generate header dependencies
CTF_DEPS= $(CTF_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(CTF_DEPS)

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
release/game.dll : $(CTF_OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) -o $@ $(CTF_OBJS) $(LDFLAGS)
else ifeq ($(YQ2_OSTYPE), Darwin)
release/game.dylib : $(CTF_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) -o $@ $(CTF_OBJS) $(LDFLAGS)
else
release/game.so : $(CTF_OBJS)
	@echo "===> LD $@"
	$(Q)$(CC) -o $@ $(CTF_OBJS) $(LDFLAGS)
endif

# ----------

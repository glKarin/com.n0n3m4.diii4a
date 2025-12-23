# ----------------------------------------------------- #
# Makefile for the 3zb2 game module for Quake II       #
#                                                       #
# Just type "make" to compile the                       #
#  - 3rd. Zigock Bot Game (game.so)                         #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theorie every client should work           #
#   Yamagi Quake II ist recommended.                    #
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
YQ2_OSTYPE := Windows
else
YQ2_OSTYPE := $(shell uname -s)
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
# Normalize some abiguous YQ2_ARCH strings
YQ2_ARCH ?= $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/^arm.*/arm/')
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

# Base CFLAGS. 
#
# -O2 are enough optimizations.
# 
# -fno-strict-aliasing since the source doesn't comply
#  with strict aliasing rules and it's next to impossible
#  to get it there...
#
# -fomit-frame-pointer since the framepointer is mostly
#  useless for debugging Quake II and slows things down.
#
# -g to build allways with debug symbols. Please do not
#  change this, since it's our only chance to debug this
#  crap when random crashes happen!
#
# -fPIC for position independend code.
#
# -MMD to generate header dependencies.
ifeq ($(YQ2_OSTYPE), Darwin)
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -fwrapv -arch i386 -arch x86_64
else
CFLAGS := -O2 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -g -MMD -fwrapv
endif

# ----------

# Switch of some annoying warnings.
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

# Using the default x87 float math on 32bit x86 causes rounding trouble
# -ffloat-store could work around that, but the better solution is to
# just enforce SSE - every x86 CPU since Pentium3 supports that
# and this should even improve the performance on old CPUs
ifeq ($(YQ2_ARCH), i386)
override CFLAGS += -msse -mfpmath=sse
endif

# ----------

# Defines the operating system and architecture
CFLAGS += -DOSTYPE=\"$(YQ2_OSTYPE)\" -DARCH=\"$(YQ2_ARCH)\"

# ----------

# Base LDFLAGS.
ifeq ($(YQ2_OSTYPE), Darwin)
LDFLAGS := -shared -arch i386 -arch x86_64 
else ifeq ($(YQ2_OSTYPE), Windows)
LDFLAGS := -shared -static-libgcc
else
LDFLAGS := -shared -lm
endif

# ----------

# Builds everything
all: 3zb2

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
.PHONY : all clean 3zb2

# ----------
 
# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The 3zb2 game
ifeq ($(YQ2_OSTYPE), Windows)
3zb2:
	@echo "===> Building game.dll"
	${Q}mkdir -p release
	$(MAKE) release/game.dll
else ifeq ($(YQ2_OSTYPE), Darwin)
3zb2:
	@echo "===> Building game.dylib"
	${Q}mkdir -p release
	$(MAKE) release/game.dylib
else
3zb2:
	@echo "===> Building game.so"
	${Q}mkdir -p release
	$(MAKE) release/game.so

release/game.so : CFLAGS += -fPIC
endif

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<

# ----------

3ZB2_OBJS_ = \
	src/bot/bot.o \
	src/bot/fire.o \
	src/bot/func.o \
	src/bot/za.o \
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
	src/monster/move.o \
	src/player/client.o \
	src/player/hud.o \
	src/player/menu.o \
	src/player/trail.o \
	src/player/view.o \
	src/player/weapon.o \
	src/shared/shared.o

# ----------

# Rewrite pathes to our object directory
3ZB2_OBJS = $(patsubst %,build/%,$(3ZB2_OBJS_))

# ----------

# Generate header dependencies
3ZB2_DEPS= $(3ZB2_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(3ZB2_DEPS)

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
release/game.dll : $(3ZB2_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(3ZB2_OBJS)
else ifeq ($(YQ2_OSTYPE), Darwin)
release/game.dylib : $(3ZB2_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(3ZB2_OBJS)
else
release/game.so : $(3ZB2_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(3ZB2_OBJS)
endif

# ----------

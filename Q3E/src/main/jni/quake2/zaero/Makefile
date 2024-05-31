# ----------------------------------------------------- #
# Makefile for the zaero game module for Quake II       #
#                                                       #
# Just type "make" to compile the                       #
#  - Zaero Game (game.so / game.dll)                    #
#                                                       #
# Dependencies:                                         #
# - None, but you need a Quake II to play.              #
#   While in theorie every one should work              #
#   Yamagi Quake II ist recommended.                    #
#                                                       #
# Platforms:                                            #
# - Linux                                               #
# - FreeBSD                                             #
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
ifneq ($(YQ2_OSTYPE), Darwin)
# Normalize some abiguous YQ2_ARCH strings
YQ2_ARCH ?= $(shell uname -m | sed -e 's/i.86/i386/' -e 's/amd64/x86_64/' -e 's/arm64/aarch64/' -e 's/^arm.*/arm/')
else
YQ2_ARCH ?= $(shell uname -m)
endif
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
		  -Wall -pipe -g -fwrapv -arch $(YQ2_ARCH)
else
CFLAGS := -O0 -fno-strict-aliasing -fomit-frame-pointer \
		  -Wall -pipe -ggdb -MMD -fwrapv
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
override CFLAGS += -DYQ2OSTYPE=\"$(YQ2_OSTYPE)\" -DYQ2ARCH=\"$(YQ2_ARCH)\"

# ----------

# Base LDFLAGS.
ifeq ($(YQ2_OSTYPE), Darwin)
override LDFLAGS := -shared -arch $(YQ2_ARCH)
else
override LDFLAGS := -shared
endif

# ----------

# Builds everything
all: zaero

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
.PHONY : all clean zaero

# ----------

# Cleanup
clean:
	@echo "===> CLEAN"
	${Q}rm -Rf build release

# ----------

# The zaero game
ifeq ($(YQ2_OSTYPE), Windows)
zaero:
	@echo "===> Building game.dll"
	${Q}mkdir -p release
	${MAKE} release/game.dll

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<
else ifeq ($(YQ2_OSTYPE), Darwin)
zaero:
	@echo "===> Building game.dylib"
	${Q}mkdir -p release
	$(MAKE) release/game.dylib

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<
else
zaero:
	@echo "===> Building game.so"
	${Q}mkdir -p release
	$(MAKE) release/game.so

build/%.o: %.c
	@echo "===> CC $<"
	${Q}mkdir -p $(@D)
	${Q}$(CC) -c $(CFLAGS) -o $@ $<

release/game.so : CFLAGS += -fPIC
endif 

# ----------

ZAERO_OBJS_ = \
	src/g_ai.o \
	src/g_cmds.o \
	src/g_combat.o \
	src/g_func.o \
	src/g_items.o \
	src/g_main.o \
	src/g_misc.o \
	src/g_monster.o \
	src/g_phys.o \
	src/g_spawn.o \
	src/g_svcmds.o \
	src/g_target.o \
	src/g_trigger.o \
	src/g_turret.o \
	src/g_utils.o \
	src/g_weapon.o \
	src/monster/actor/actor.o \
	src/monster/berserker/berserker.o \
	src/monster/boss/boss.o \
	src/monster/boss2/boss2.o \
	src/monster/boss3/boss3.o \
	src/monster/boss3/boss31.o \
	src/monster/boss3/boss32.o \
	src/monster/brain/brain.o \
	src/monster/chick/chick.o \
	src/monster/flipper/flipper.o \
	src/monster/float/float.o \
	src/monster/flyer/flyer.o \
	src/monster/gladiator/gladiator.o \
	src/monster/gunner/gunner.o \
	src/monster/handler/handler.o \
	src/monster/hound/hound.o \
	src/monster/hover/hover.o \
	src/monster/infantry/infantry.o \
	src/monster/insane/insane.o \
	src/monster/medic/medic.o \
	src/monster/misc/move.o \
	src/monster/mutant/mutant.o \
	src/monster/parasite/parasite.o \
	src/monster/sentien/sentien.o \
	src/monster/soldier/soldier.o \
	src/monster/supertank/supertank.o \
	src/monster/tank/tank.o \
	src/player/client.o \
	src/player/hud.o \
	src/player/trail.o \
	src/player/view.o \
	src/player/weapon.o \
	src/savegame/savegame.o \
	src/shared/flash.o \
	src/shared/shared.o \
    src/zaero/acannon.o \
    src/zaero/ai.o \
    src/zaero/anim.o \
    src/zaero/camera.o \
    src/zaero/frames.o \
    src/zaero/item.o \
    src/zaero/mtest.o \
    src/zaero/spawn.o \
    src/zaero/trigger.o \
    src/zaero/weapon.o

# ----------

# Rewrite pathes to our object directory
ZAERO_OBJS = $(patsubst %,build/%,$(ZAERO_OBJS_))

# ----------

# Generate header dependencies
ZAERO_DEPS= $(ZAERO_OBJS:.o=.d)

# ----------

# Suck header dependencies in
-include $(ZAERO_DEPS)

# ----------

ifeq ($(YQ2_OSTYPE), Windows)
release/game.dll : $(ZAERO_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(ZAERO_OBJS)
else ifeq ($(YQ2_OSTYPE), Darwin)
release/game.dylib : $(ZAERO_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(ZAERO_OBJS)
else
release/game.so : $(ZAERO_OBJS)
	@echo "===> LD $@"
	${Q}$(CC) $(LDFLAGS) -o $@ $(ZAERO_OBJS)
endif

# ----------

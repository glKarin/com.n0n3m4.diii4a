COMMON_OBJS=comprout.o hash.o qcc_cmdlib.o qcd_main.o
QCC_OBJS=qccmain.o qcc_pr_comp.o qcc_pr_lex.o packager.o decomp.o
VM_OBJS=pr_exec.o pr_edict.o pr_multi.o initlib.o qcdecomp.o
GTKGUI_OBJS=qcc_gtk.o qccguistuff.o
WIN32GUI_OBJS=qccgui.o qccguistuff.o packager.o
TUI_OBJS=qcctui.o
LIB_OBJS=

CC?=gcc
CFLAGS?=-Wall

all: help qcc
help:
	@echo for fteqccgui: win or nocyg
	@echo for commandline: qcc
	@echo for debug builds, add: DEBUG=1
	@echo 

USEGUI_CFLAGS=
# set to -DUSEGUI when compiling the GUI
WARNING_CFLAGS=-Wno-pointer-sign
BASE_CFLAGS+=$(WARNING_CFLAGS)
BASE_CFLAGS+=$(USEGUI_CFLAGS)

ifneq ($(DEBUG),)
	BASE_CFLAGS+=-ggdb
else
	BASE_LDFLAGS+=-s
endif
BASE_LDFLAGS+=-lz
# set to "" for debugging

DO_CC?=$(CC) $(BASE_CFLAGS) -o $@ -c $< $(CFLAGS)

lib: 

R_win_nocyg: $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS)
	$(CC) $(BASE_CFLAGS) -o fteqcc.exe -O3 $(BASE_LDFLAGS) $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS) -mno-cygwin -mwindows -lcomctl32 -lole32 -lshlwapi
R_nocyg: $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS)
	$(CC) $(BASE_CFLAGS) -o fteqcc.exe -O3 $(BASE_LDFLAGS) $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS) -mno-cygwin -lcomctl32 -lole32 -lshlwapi
R_win: $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS)
	$(CC) $(BASE_CFLAGS) -o fteqcc.exe -O3 $(BASE_LDFLAGS) $(QCC_OBJS) $(COMMON_OBJS) $(WIN32GUI_OBJS) -mwindows -lcomctl32 -lole32 -lshlwapi

win_nocyg:
	$(MAKE) USEGUI_CFLAGS="-DUSEGUI -DQCCONLY" R_win_nocyg
nocyg:
	$(MAKE) USEGUI_CFLAGS="-DUSEGUI -DQCCONLY" R_nocyg
win:
	$(MAKE) USEGUI_CFLAGS="-DUSEGUI -DQCCONLY" R_win

R_qcc: $(QCC_OBJS) $(COMMON_OBJS) $(TUI_OBJS)
	$(CC) $(BASE_CFLAGS) -o fteqcc.bin -O3 $(QCC_OBJS) $(TUI_OBJS) $(COMMON_OBJS) $(BASE_LDFLAGS) -lm
qcc:
	$(MAKE) USEGUI_CFLAGS="" R_qcc

qccmain.o: qccmain.c qcc.h
	$(DO_CC)

qcc_cmdlib.o: qcc_cmdlib.c qcc.h
	$(DO_CC)

qcc_pr_comp.o: qcc_pr_comp.c qcc.h
	$(DO_CC)

qcc_pr_lex.o: qcc_pr_lex.c qcc.h
	$(DO_CC)

comprout.o: comprout.c qcc.h
	$(DO_CC)

hash.o: hash.c qcc.h
	$(DO_CC)

qcd_main.o: qcd_main.c qcc.h
	$(DO_CC)

qccguistuff.o: qccguistuff.c qcc.h
	$(DO_CC)

packager.o: packager.c qcc.h
	$(DO_CC)

%.o: %.c
	$(DO_CC)

qcc_gtk.o: qcc_gtk.c qcc.h
	$(DO_CC) `pkg-config --cflags gtk+-2.0`

R_gtkgui: $(QCC_OBJS) $(COMMON_OBJS) $(GTKGUI_OBJS)
	$(CC) $(BASE_CFLAGS) $(USEGUI_CFLAGS) -o fteqccgui.bin -O3 $(GTKGUI_OBJS) $(QCC_OBJS) $(COMMON_OBJS) `pkg-config --libs gtk+-2.0`
gtkgui:
	$(MAKE) USEGUI_CFLAGS="-DUSEGUI -DQCCONLY" R_gtkgui

clean:
	$(RM) fteqcc.bin fteqcc.exe $(QCC_OBJS) $(COMMON_OBJS) $(VM_OBJS) $(GTKGUI_OBJS) $(WIN32GUI_OBJS) $(TUI_OBJS)

qcvm.so: $(QCC_OBJS) $(VM_OBJS) $(COMMON_OBJS)
	$(CC) $(BASE_CFLAGS) -o $@ -O3 $(BASE_LDFLAGS) $(QCC_OBJS) $(VM_OBJS) $(COMMON_OBJS) -shared
qcvm.a: $(QCC_OBJS) $(VM_OBJS) $(COMMON_OBJS)
	ar r $@ $^

test.o: test.c
	$(DO_CC)

qcvm: test.o qcvm.a
	$(CC) $(BASE_CFLAGS) $(CFLAGS) -o qcvm -O3 $(BASE_LDFLAGS) $^ -lm -lz -ggdb

tests: qcvm
	@echo Running Tests...
	@$(foreach a,$(wildcard tests/*.src), echo TEST: $a; rm progs.dat; ./testapp.bin progs.dat -srcfile $a; echo; echo)
	@echo Tests run.

.PHONY: tests

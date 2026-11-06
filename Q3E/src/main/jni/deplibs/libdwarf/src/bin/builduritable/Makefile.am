###Copyright (C) 2021 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

### This does not build the entire file
### dd_uri.c, it just creates the array entries
### for that file.  There is no reason
### to do anything with newuritable.
### This documents its content and how
### it was created.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = newuritable
rebuild: builduritable
	./builuritable > newuritable

noinst_PROGRAMS = builduritable
builduritable_SOURCES = uritablebuild.c
builduritable_CFLAGS = $(DWARF_CFLAGS_WARN)

EXTRA_DIST = \
meson.build

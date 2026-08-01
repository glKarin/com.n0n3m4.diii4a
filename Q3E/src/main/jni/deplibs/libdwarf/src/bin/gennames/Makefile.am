###Copyright (C) 2021 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

# This also creates dwarf_names.h which one should check against
# libdwarf.h (which has these and much else).

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = dwarf_names.h dwarf_names.c
rebuild: gennames
	./gennames -o . -i $(top_srcdir)/src/lib/libdwarf
	mv dwarf_names.c $(top_srcdir)/src/lib/libdwarf/dwarf_names.c

noinst_PROGRAMS = gennames
gennames_SOURCES = gennames.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_getopt.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.c
gennames_CPPFLAGS = \
  -I$(top_srcdir)/src/bin/dwarfdump \
  -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir) \
  -I$(top_builddir)/src/lib/libdwarf
gennames_CFLAGS = $(DWARF_CFLAGS_WARN)

EXTRA_DIST = \
meson.build


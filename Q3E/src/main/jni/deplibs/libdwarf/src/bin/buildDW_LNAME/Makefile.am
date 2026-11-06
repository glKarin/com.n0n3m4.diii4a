###Copyright (C) 2025 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = 
rebuild: buildlname
	./buildlname -f $(top_srcdir) 
noinst_PROGRAMS = buildlname
buildlname_SOURCES = buildlname.c \
$(top_srcdir)/src/lib/libdwarf/dwarf_string.c \
$(top_srcdir)/src/lib/libdwarf/dwarf_string.h
buildlname_CPPFLAGS = \
  -I. -I$(top_srcdir)/src/lib/libdwarf 
buildlname_CFLAGS = $(DWARF_CFLAGS_WARN)

EXTRA_DIST = meson.build dwarf_lname_data.h

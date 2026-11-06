###Copyright (C) 2025 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = 
rebuild: checklangname
	./checklangname -f $(top_srcdir) 
noinst_PROGRAMS = checklangname
checklangname_SOURCES = checklangname.c
checklangname_CPPFLAGS = \
  -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
checklangname_CFLAGS = $(DWARF_CFLAGS_WARN)

EXTRA_DIST = meson.build

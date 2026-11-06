###Copyright (C) 2021 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
CLEANFILES = dd_opscounttab.c
rebuild: buildopstab
	./buildopstab -f $(top_srcdir) >dd_opscounttab.c
	cp dd_opscounttab.c $(top_srcdir)/src/bin/dwarfdump/dd_opscounttab.c

noinst_PROGRAMS = buildopstab
buildopstab_SOURCES = buildopscounttab.c
buildopstab_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_srcdir)/src/bin/dwarfdump \
  -I$(top_builddir)/src/lib/libdwarf
buildopstab_CFLAGS = $(DWARF_CFLAGS_WARN)
buildopstab_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la

EXTRA_DIST = \
meson.build

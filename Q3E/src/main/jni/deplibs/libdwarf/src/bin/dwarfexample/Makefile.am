###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = simplereader frame1 frame2 findfuncbypc \
    dwdebuglink  jitreader showsectiongroups
dwarfbigend=@DWARF_BIGENDIAN@

simplereader_SOURCES = simplereader.c
simplereader_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
simplereader_CFLAGS = $(DWARF_CFLAGS_WARN)
simplereader_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

frame1_SOURCES = frame1.c
frame1_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
frame1_CFLAGS = $(DWARF_CFLAGS_WARN)
frame1_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

frame2_SOURCES = frame2.c
frame2_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
frame2_CFLAGS = $(DWARF_CFLAGS_WARN)
frame2_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

findfuncbypc_SOURCES = findfuncbypc.c
findfuncbypc_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
findfuncbypc_CFLAGS = $(DWARF_CFLAGS_WARN)
findfuncbypc_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

dwdebuglink_SOURCES = dwdebuglink.c
dwdebuglink_CPPFLAGS = \
  -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/libdwarf
dwdebuglink_CFLAGS = $(DWARF_CFLAGS_WARN)
dwdebuglink_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

jitreader_SOURCES = jitreader.c
jitreader_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
jitreader_CFLAGS = $(DWARF_CFLAGS_WARN)
jitreader_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

showsectiongroups_SOURCES = showsectiongroups.c
showsectiongroups_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarf \
  -I$(top_builddir)/src/lib/libdwarf
showsectiongroups_CFLAGS = $(DWARF_CFLAGS_WARN)
showsectiongroups_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la \
$(DWARF_LIBS)

EXTRA_DIST = \
ChangeLog \
ChangeLog2009 \
ChangeLog2010 \
ChangeLog2011 \
ChangeLog2012 \
ChangeLog2013 \
ChangeLog2014 \
ChangeLog2015 \
ChangeLog2016 \
ChangeLog2017 \
ChangeLog2018 \
ChangeLog2019 \
ChangeLog2020 \
CMakeLists.txt \
meson.build \
simplecrc.c \
NEWS \
$(dwarfexample_DATA)

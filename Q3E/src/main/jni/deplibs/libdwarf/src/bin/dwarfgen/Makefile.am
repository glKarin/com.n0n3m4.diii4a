###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = dwarfgen

dwarfgen_SOURCES = \
createirepformfrombinary.cc \
createirepfrombinary.h \
createirepfrombinary.cc \
dwarfgen.cc \
dwarf_elf_defines.h  \
dwarf_elfstructs.h \
general.h \
irepattrtodbg.cc \
irepattrtodbg.h \
irepdie.h \
irepform.h \
irepframe.h \
irepline.h \
irepmacro.h \
ireppubnames.h \
irepresentation.h \
ireptodbg.cc \
ireptodbg.h \
dg_getopt.cc \
dg_getopt.h \
strtabdata.h

# dwarfdump here is so we find dwarf_getopt.h
dwarfgen_CPPFLAGS = -I$(top_srcdir)/src/lib/libdwarfp \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf \
-I$(top_builddir)/src/lib/libdwarf \
-I$(top_builddir)/src/lib/libdwarfp

dwarfgen_LDADD = \
$(top_builddir)/src/lib/libdwarfp/libdwarfp.la \
$(top_builddir)/src/lib/libdwarf/libdwarf.la \
@DWARF_LIBS@ @DWARFGEN_LIBS@

EXTRA_DIST = \
COPYING \
ChangeLog \
CMakeLists.txt \
meson.build \
NEWS \
README \
$(dwarfgen_DATA) \
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
dwarf-generator.txt \
meson.build

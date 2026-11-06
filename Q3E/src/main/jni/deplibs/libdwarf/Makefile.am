###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS=subdir-objects
SUBDIRS = src/lib/libdwarf src/bin/dwarfdump

if HAVE_DWARFGEN
  SUBDIRS += src/lib/libdwarfp
  SUBDIRS += src/bin/dwarfgen
endif

if HAVE_DWARFEXAMPLE
SUBDIRS += src/bin/dwarfexample
endif

SUBDIRS += test
SUBDIRS += doc
SUBDIRS += fuzz
SUBDIRS += src/bin/gennames
SUBDIRS += src/bin/tag_tree
SUBDIRS += src/bin/tag_attr
SUBDIRS += src/bin/attr_form
SUBDIRS += src/bin/buildopstab
SUBDIRS += src/bin/builduritable
SUBDIRS += src/bin/checkDW_LANG
SUBDIRS += src/bin/buildDW_LNAME

### rebuild is for maintainers to update C source
### that depends on certain (C and/or text) Files.
### Very rarely necessary.
### Harmless if nothing needed update.
### Rebuild all source after doing 'make rebuild'
### Also do 'make check' as it checks correctness
### of certain libdwarf source files.
rebuild:
	make -C src/bin/gennames    rebuild
	make -C src/bin/tag_tree    rebuild
	make -C src/bin/tag_attr    rebuild
	make -C src/bin/attr_form   rebuild
	make -C src/bin/buildopstab rebuild
	make -C src/bin/checkDW_LANG rebuild
	make -C src/bin/buildDW_LNAME rebuild
doc:
	make -C doc doc

### Avoids README complaints from earlier versions of automake.
README: README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdwarf.pc
if HAVE_DWARFGEN
pkgconfig_DATA +=  libdwarfp.pc
endif

DISTCLEANFILES =

MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
ar-lib \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
m4/libtool.m4 \
m4/ltoptions.m4 \
m4/ltsugar.m4 \
m4/ltversion.m4 \
m4/lt~obsolete.m4 \
missing  \
test-driver \
$(distdir).tar.gz  \
$(distdir).tar.xz

EXTRA_DIST = \
appveyor.yml \
autogen.sh \
bugxml \
ChangeLog \
ChangeLog2018 \
ChangeLog2019 \
ChangeLog2020 \
ChangeLog2021 \
ChangeLog2022 \
ChangeLog2023 \
ChangeLog2024 \
ChangeLog2025 \
cmake \
CMakeLists.txt \
CODINGSTYLE.md \
CONTRIBUTING.md \
cmake/config.h.in \
README.md \
READMEmacos.md \
READMEcmake.md \
READMEwin-msys2.md \
libdwarf.pc.in \
libdwarfp.pc.in \
meson.build \
meson_options.txt \
scripts/allsimplebuilds.sh \
scripts/buildandreleasetest.sh \
scripts/run-all-tests.sh \
scripts/ChangeLog \
scripts/ChangeLog2018 \
scripts/ChangeLog2019 \
scripts/ChangeLog2020 \
scripts/CLEANUP \
tools/makerelease.sh	\
tools/README	\
tools/updatesemanticversion.py \
.travis.yml

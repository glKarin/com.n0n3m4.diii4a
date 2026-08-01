# Copyright 2021 David Anderson
# This Makefile.am is hereby placed in the public domain
### Any error in thie file will lead to:
### error: Something went wrong bootstrapping makefile fragments
### for automatic dependency tracking
###Entering directory '/var/tmp/bld/src/lib/libdwarf'
###CC       libdwarf_la-dwarf_abbrev.lo
###/bin/bash: ../../../libtool: No such file or directory
### with no hint as to what the problem really is..

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in\
  dwarfdump.conf \
  getnametest.log \
  getnametest.trs \
  junk.debuglink1 \
  junk.debuglink1a \
  junk.debuglink1b \
  junk.debuglink1sed1 \
  junk.debuglink1sed2 \
  junk.debuglink2 \
  junk.debuglink2a \
  junk.debuglink2b \
  junk.jitreader.new \
  test_dwarfstring.log \
  test_dwarfstring.trs \
  test_dwgetopt.log \
  test_dwgetopt.trs \
  test_errmsglist.log \
  test_errmsglist.trs \
  test_extra_flag_strings.log \
  test_extra_flag_strings.trs \
  test_helpertree.log  \
  test_helpertree.trs \
  test_ignoresec.trs \
  test_linkedtopath.log \
  test_linkedtopath.trs \
  test_lname.trs\
  test_lname.log\
  test_macrocheck.log \
  test_macrocheck.trs \
  test_makenametest.log \
  test_makenametest.trs \
  test_objectaccess.log \
  test_objectaccess.trs \
  test_safe_strcpy.log \
  test_safe_strcpy.trs \
  test_setupsections.trs \
  test_setupsections.log \
  test_sanitized.log \
  test_sanitized.trs \
  test_testesb.log \
  test_testesb.trs

clean-local:
	-rm -f junk.*
	-rm -f dwarfdump.conf
	-rm -f test_setupsections.exe.manifest

TESTS = test_canonical  \
  test_dwarflebtest \
  test_dwarfstring \
  test_dwgetopt \
  test_errmsglist \
  test_extra_flag_strings \
  test_getnametest \
  test_helpertree \
  test_ignoresec \
  test_int64_test \
  test_linkedtopath \
  test_lname \
  test_macrocheck \
  test_makenametest \
  test_regex \
  test_safe_strcpy \
  test_setupsections \
  test_testesb \
  test_sanitized \
  test_tied

check_PROGRAMS = test_canonical \
  test_dwarflebtest  \
  test_dwarfstring \
  test_dwgetopt \
  test_errmsglist \
  test_extra_flag_strings \
  test_getnametest \
  test_helpertree \
  test_ignoresec \
  test_int64_test \
  test_linkedtopath \
  test_lname \
  test_macrocheck \
  test_makenametest \
  test_regex \
  test_safe_strcpy \
  test_setupsections \
  test_testesb \
  test_sanitized \
  test_tied

test_canonical_SOURCES = test_canonical.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_canonical_append.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_safe_strcpy.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_esb.c
test_canonical_CFLAGS = $(DWARF_CFLAGS_WARN)
test_canonical_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_dwarflebtest_SOURCES = test_dwarf_leb.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_leb.c
test_dwarflebtest_CFLAGS = $(DWARF_CFLAGS_WARN)
test_dwarflebtest_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf


test_int64_test_SOURCES = test_int64_test.c
test_int64_test_CFLAGS = $(DWARF_CFLAGS_WARN)
test_int64_test_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_dwarfstring_SOURCES = test_dwarfstring.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.h \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.c
test_dwarfstring_CFLAGS = $(CFLAGS_WARN) $(DWARF_CFLAGS_WARN)
test_dwarfstring_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_dwgetopt_SOURCES = test_getopt.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_getopt.h \
    $(top_srcdir)/src/bin/dwarfdump/dd_getopt.c
test_dwgetopt_CFLAGS = $(DWARF_CFLAGS_WARN)
test_dwgetopt_CPPFLAGS =  -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump

test_errmsglist_SOURCES = test_errmsglist.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.c
test_errmsglist_CFLAGS = $(DWARF_CFLAGS_WARN)
test_errmsglist_CPPFLAGS = -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_extra_flag_strings_SOURCES = test_extra_flag_strings.c \
   $(top_srcdir)/src/lib/libdwarfp/dwarf_pro_log_extra_flag_strings.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.h \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.c
test_extra_flag_strings_CFLAGS = $(CFLAGS_WARN) $(DWARF_CFLAGS_WARN)
test_extra_flag_strings_CPPFLAGS =  -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarfp \
-I$(top_srcdir)/src/lib/libdwarf

test_getnametest_SOURCES = test_getname.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_names.c
test_getnametest_CFLAGS = $(DWARF_CFLAGS_WARN)
test_getnametest_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_helpertree_SOURCES = test_helpertree.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_helpertree.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_tsearchbal.c
test_helpertree_CFLAGS = $(DWARF_CFLAGS_WARN)
test_helpertree_CPPFLAGS =  -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_ignoresec_SOURCES = test_ignoresec.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_secname_ck.c
test_ignoresec_CFLAGS = $(DWARF_CFLAGS_WARN)
test_ignoresec_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_linkedtopath_SOURCES = test_linkedtopath.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.h \
   $(top_srcdir)/src/lib/libdwarf/dwarf_string.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.h \
   $(top_srcdir)/src/lib/libdwarf/dwarf_debuglink.h \
   $(top_srcdir)/src/lib/libdwarf/dwarf_debuglink.c \
   $(top_srcdir)/src/lib/libdwarf/dwarf_error.h
test_linkedtopath_CFLAGS =  $(DWARF_CFLAGS_WARN) -DTESTING
test_linkedtopath_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) \
-I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_lname_SOURCES = test_lname.c 
test_lname_CFLAGS =  $(DWARF_CFLAGS_WARN) -DTESTING
test_lname_CPPFLAGS = -DTESTING \
-DLIBDWARF_BUILD \
-I$(top_srcdir) \
-I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf


test_macrocheck_SOURCES = test_macrocheck.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_esb.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_tsearchbal.c
test_macrocheck_CFLAGS = $(DWARF_CFLAGS_WARN)
test_macrocheck_CPPFLAGS =  -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_makenametest_SOURCES = test_makename.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_esb.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_makename.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_tsearchbal.c
test_makenametest_CFLAGS = $(DWARF_CFLAGS_WARN)
test_makenametest_CPPFLAGS =  -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_regex_SOURCES = test_regex.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_regex.c
test_regex_CFLAGS = $(DWARF_CFLAGS_WARN)
test_regex_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_testesb_SOURCES = test_esb.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_esb.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_tsearchbal.c
test_testesb_CFLAGS = $(DWARF_CFLAGS_WARN)
test_testesb_CPPFLAGS = -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_sanitized_SOURCES = test_sanitized.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_esb.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_sanitized.c \
    $(top_srcdir)/src/bin/dwarfdump/dd_utf8.c
test_sanitized_CFLAGS = $(DWARF_CFLAGS_WARN)
test_sanitized_CPPFLAGS = -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/bin/dwarfdump \
-I$(top_srcdir)/src/lib/libdwarf

test_safe_strcpy_SOURCES = test_safe_strcpy.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_safe_strcpy.h
test_safe_strcpy_CFLAGS = $(DWARF_CFLAGS_WARN)
test_safe_strcpy_CPPFLAGS = \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_setupsections_SOURCES = test_setupsections.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_setup_sections.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_setup_sections.h
test_setupsections_CFLAGS = $(DWARF_CFLAGS_WARN)
test_setupsections_CPPFLAGS = -DTESTING \
-I$(top_srcdir) -I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

test_tied_SOURCES = test_dwarf_tied.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_tied.c \
    $(top_srcdir)/src/lib/libdwarf/dwarf_tsearchhash.c
test_tied_CFLAGS = $(DWARF_CFLAGS_WARN)
test_tied_CPPFLAGS = -DTESTING \
-I$(top_srcdir) \
-I$(top_srcdir)/src/lib/libdwarf

### debuglink tests are difficult to support in Windows/mingw
if HAVE_DEBUGLINK
if HAVE_DWARFEXAMPLE
  TESTS += test_debuglink-a.sh test_debuglink-b.sh
endif
endif
TESTS += test_dwarfdumpLinux.sh  test_dwarfdumpPE.sh test_dwarfdumpMacos.sh
if HAVE_DWARFEXAMPLE
TESTS += test_jitreaderdiff.sh
endif

### no dwarfgen tests available.

AM_TESTS_ENVIRONMENT = DWTOPSRCDIR='$(top_srcdir)'; \
    export DWTOPSRCDIR ; \
    DWCOMPILERFLAGS='$(DWARF_CFLAGS_WARN)';\
    export DWCOMPILERFLAGS ;

### dummysource ignore is to be kept, but not used.
### See buildingdummy.sh which is also not to be used.
EXTRA_DIST= \
buildingdummy.sh \
CMakeLists.txt \
debuglink2.base \
debuglink.base \
canonicalpath.py \
test_debuglink-a.sh \
test_debuglink-b.sh \
dummyexecutable \
dummyexecutable.debug \
dummysourceignore \
test_dwarfdumpLinux.sh  test_dwarfdumpMacos.sh \
test_dwarfdumpPE.sh  test_dwarfdumpsetup.sh \
test_dwarfdump.py \
test_dwarf_leb.c \
test_dwarf_tied.c \
test_dwdiff.py \
test_getname.c \
test_getopt.c \
test_helpertree.c \
test_ignoresec.c \
jitreader.base \
test_jitreaderdiff.sh \
test_makename.c \
meson.build \
README.testcases \
test_dwarfstring.c \
test_errmsglist.c \
test_esb.c \
test_safe_strcpy.c \
test_sanitized.c \
test_setupsections.c \
test_extra_flag_strings.c \
test_linkedtopath.c \
test-mach-o-32.base \
test-mach-o-32.dSYM \
testobjLE32PE.base \
testobjLE32PE.exe \
testobjLE32PE.test.c \
testuriLE64ELf.base \
testuriLE64ELfsource.c \
testuriLE64ELf.testme \
test_transformpath.py


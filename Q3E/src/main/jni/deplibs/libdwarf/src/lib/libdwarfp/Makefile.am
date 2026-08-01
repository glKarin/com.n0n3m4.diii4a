###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in

### libdwarfp

lib_LTLIBRARIES = libdwarfp.la

libdwarfp_la_SOURCES = \
$(top_srcdir)/src/lib/libdwarf/dwarf_tsearchhash.c \
$(top_srcdir)/src/lib/libdwarf/dwarf_memcpy_swap.c \
$(top_srcdir)/src/lib/libdwarf/dwarf_string.c \
libdwarfp.h \
dwarf_pro_alloc.c \
dwarf_pro_alloc.h \
dwarf_pro_arange.c \
dwarf_pro_arange.h \
dwarf_pro_debug_sup.c \
dwarf_pro_die.c \
dwarf_pro_die.h \
dwarf_pro_dnames.h \
dwarf_pro_dnames.c \
dwarf_pro_error.c \
dwarf_pro_error.h \
dwarf_pro_expr.c \
dwarf_pro_expr.h \
dwarf_pro_finish.c \
dwarf_pro_forms.c \
dwarf_pro_frame.c \
dwarf_pro_frame.h \
dwarf_pro_funcs.c \
dwarf_pro_incl.h \
dwarf_pro_init.c \
dwarf_pro_line.c \
dwarf_pro_line.h \
dwarf_pro_log_extra_flag_strings.c \
dwarf_pro_macinfo.c \
dwarf_pro_macinfo.h \
dwarf_pro_opaque.h \
dwarf_pro_pubnames.c \
dwarf_pro_reloc.c \
dwarf_pro_reloc.h \
dwarf_pro_reloc_symbolic.c \
dwarf_pro_reloc_symbolic.h \
dwarf_pro_reloc_stream.h \
dwarf_pro_reloc_stream.c \
dwarf_pro_section.c \
dwarf_pro_section.h \
dwarf_pro_types.c \
dwarf_pro_types.h \
dwarf_pro_util.h \
dwarf_pro_vars.c \
dwarf_pro_weaks.c


### -DLIBDWARFP_BUILD this is not used anywhere Old mistake.
libdwarfp_la_CFLAGS = $(DWARF_CFLAGS_WARN)
libdwarfp_la_CPPFLAGS = \
-DLIBDWARF_BUILD \
-I$(top_builddir) \
-I$(top_srcdir)/src/lib/libdwarf

libdwarfp_la_LIBADD = \
@DWARF_LIBS@ \
$(top_builddir)/src/lib/libdwarf/libdwarf.la

libdwarfp_la_LDFLAGS = -fPIC -no-undefined -version-info @version_info@ @release_info@

libdwarfppkgincludedir = $(pkgincludedir)-@VMAJ@
libdwarfppkginclude_DATA = \
$(top_srcdir)/src/lib/libdwarf/dwarf.h  \
$(top_srcdir)/src/lib/libdwarfp/libdwarfp.h \
$(top_srcdir)/src/lib/libdwarf/libdwarf.h

EXTRA_DIST = \
COPYING \
ChangeLog \
CMakeLists.txt \
libdwarfp.pc.in \
cmake/libdwarfp-config.cmake \
meson.build \
NEWS \
README

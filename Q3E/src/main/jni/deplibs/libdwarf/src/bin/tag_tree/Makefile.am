###Copyright (C) 2021 David Anderson <davea42 at linuxmail dot org>
###This code is public domain and can be freely used or copied.

AUTOMAKE_OPTIONS=subdir-objects
MAINTAINERCLEANFILES = Makefile.in
rebuild: tagtree
	cp $(top_srcdir)/src/bin/tag_tree/tag_tree.list tmp-t2.c
	$(CC) -DTRIVIAL_NAMING -I$(top_srcdir)/src/lib/libdwarf -E tmp-t2.c >tmp-tag-tree-build2.tmp
	./tagtree -s -i tmp-tag-tree-build2.tmp -o dwarfdump-tt-table.h
	cp dwarfdump-tt-table.h $(top_srcdir)/src/bin/dwarfdump/
	rm -f tmp-tag-tree-build2.tmp
	rm -f tmp-t2.c
	rm -f tmp-t3.c
	cp $(top_srcdir)/src/bin/tag_tree/tag_tree_ext.list tmp-t3.c
	$(CC) -DTRIVIAL_NAMING -I$(top_srcdir)/src/lib/libdwarf -E tmp-t3.c > tmp-tag-tree-build3.tmp
	./tagtree -e -i tmp-tag-tree-build3.tmp -o dwarfdump-tt-ext-table.h
	cp dwarfdump-tt-ext-table.h $(top_srcdir)/src/bin/dwarfdump/
	rm -f dwarfdump-tt-ext-table.h
	rm -f dwarfdump-tt-table.h
	rm -f tmp-t2.c
	rm -f tmp-tag-tree-build2.tmp
	rm -f tmp-t3.c
	rm -f tmp-tag-tree-build3.tmp

noinst_PROGRAMS = tagtree
tagtree_SOURCES = tag_tree.c \
$(top_srcdir)/src/bin/dwarfdump/dd_common.c \
$(top_srcdir)/src/bin/dwarfdump/dd_tsearchbal.c \
$(top_srcdir)/src/bin/dwarfdump/dd_getopt.c \
$(top_srcdir)/src/bin/dwarfdump/dd_esb.c \
$(top_srcdir)/src/bin/dwarfdump/dd_makename.c \
$(top_srcdir)/src/bin/dwarfdump/dd_naming.c \
$(top_srcdir)/src/bin/dwarfdump/dd_glflags.c \
$(top_srcdir)/src/bin/dwarfdump/dd_sanitized.c \
$(top_srcdir)/src/bin/dwarfdump/dd_utf8.c \
$(top_srcdir)/src/bin/dwarfdump/dd_safe_strcpy.c \
$(top_srcdir)/src/bin/tag_tree/tag_common.c
tagtree_CPPFLAGS = -DTRIVIAL_NAMING \
-DBUILD_NONLIB_SOURCE \
-I$(top_srcdir)/src/lib/libdwarf \
-I$(top_srcdir)/src/bin/dwarfdump
tagtree_CFLAGS = $(DWARF_CFLAGS_WARN)
tagtree_LDADD = $(top_builddir)/src/lib/libdwarf/libdwarf.la

EXTRA_DIST = \
meson.build \
tag_tree.list \
tag_tree_ext.list

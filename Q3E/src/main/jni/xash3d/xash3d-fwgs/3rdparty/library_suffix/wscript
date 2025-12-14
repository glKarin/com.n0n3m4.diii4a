#!/usr/bin/env python

def options(opt):
	pass

def configure(conf):
	pass

def build(bld):
	# for those who only want to use build.h
	bld(name = 'library_suffix_includes',
		export_includes = 'include/')

	bld.stlib(source = 'src/library_suffix.c',
		target = 'library_suffix',
		use = 'library_suffix_includes werror')

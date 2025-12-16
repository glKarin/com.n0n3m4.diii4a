#! /usr/bin/env python
# encoding: utf-8
# mittorn, 2018

from waflib import Logs
import os

top = '.'

def options(opt):
	# stub
	return

def configure(conf):
	if conf.env.DEST_OS2 == 'android':
		conf.check_cc(lib='log')
	conf.define('__MULTITEXTURE_SUPPORT__', 1)
	conf.define('NANOGL_MANGLE_PREPEND', 1)
	conf.define('REF_DLL', 1)
	# stub
	return

def build(bld):
	source = bld.path.ant_glob(['*.cpp'])
	libs = ['werror']
	if bld.env.DEST_OS2 == 'android':
		libs += ['LOG']
	includes = [ '.', 'GL/' ]

	bld.stlib(
		source   = source,
		target   = 'nanogl',
		includes = includes,
		use      = libs
	)

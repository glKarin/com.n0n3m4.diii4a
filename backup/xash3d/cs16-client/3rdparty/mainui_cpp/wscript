#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from waflib import Logs, Configure
import os

top = '.'

FT2_CHECK='''extern "C" {
#include <ft2build.h>
#include FT_FREETYPE_H
}

int main() { return FT_Init_FreeType( NULL ); }
'''

def options(opt):
	grp = opt.add_option_group('MainUI C++ options')
	grp.add_option('--enable-stbtt', action = 'store_true', dest = 'USE_STBTT', default = False,
		help = 'prefer stb_truetype.h over freetype [default: %(default)s]')

	return

def configure(conf):
	conf.load('fwgslib cxx11')
	conf.env.append_unique('DEFINES', 'STDINT_H=<cstdint>')

	if not conf.check_std('cxx11'):
		conf.define('MY_COMPILER_SUCKS', 1)

	if conf.env.DEST_OS in ['android', 'darwin', 'nswitch', 'psvita', 'emscripten'] or conf.env.MAGX:
		conf.options.USE_STBTT = True

	conf.define('MAINUI_USE_CUSTOM_FONT_RENDER', 1)

	nortti = {
		'msvc': ['/GR-'],
		'default': ['-fno-rtti']
	}

	conf.env.append_unique('CXXFLAGS', conf.get_flags_by_compiler(nortti, conf.env.COMPILER_CC))

	conf.define_cond('MAINUI_USE_STB', conf.options.USE_STBTT)

	if conf.env.DEST_OS == 'android':
		conf.define('NO_STL', 1)
		conf.env.append_unique('CXXFLAGS', '-fno-exceptions')

	if conf.env.DEST_OS != 'win32' and conf.env.DEST_OS != 'dos':
		if not conf.options.USE_STBTT and not conf.options.LOW_MEMORY:
			conf.check_pkg('freetype2', 'FT2', FT2_CHECK)
			conf.define('MAINUI_USE_FREETYPE', 1)

def build(bld):
	source = bld.path.ant_glob([
		'*.cpp',
		'miniutl/*.cpp',
		'font/*.cpp',
		'menus/*.cpp',
		'menus/dynamic/*.cpp',
		'model/*.cpp',
		'controls/*.cpp'
	])

	includes = [
		'.',
		'miniutl/',
		'font/',
		'controls/',
		'menus/',
		'model/',
		'sdk_includes/common',
		'sdk_includes/engine',
		'sdk_includes/public',
		'sdk_includes/pm_shared'
	]

	bld.shlib(
		source   = source,
		target   = 'menu',
		includes = includes,
		use      = 'werror FT2 GDI32 USER32',
		install_path = bld.env.LIBDIR,
		cmake_skip = True
	)

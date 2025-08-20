#! /usr/bin/env python
# Copyright 2019 (C) a1batross

from waflib import Configure, Errors, Utils

# TODO: make generic
CHECK_SYMBOL_EXISTS_FRAGMENT = '''
#include "build.h"

int main(int argc, char** argv)
{
  (void)argv;
#ifndef %s
  return ((int*)(&%s))[argc];
#else
  (void)argc;
  return 0;
#endif
}
'''

# generated(see comments in public/build.h)
# cat build.h | grep '^#undef XASH' | awk '{ print "'\''" $2 "'\''," }'
DEFINES = [
'XASH_64BIT',
'XASH_AMD64',
'XASH_ANDROID',
'XASH_APPLE',
'XASH_ARM',
'XASH_ARM_HARDFP',
'XASH_ARM_SOFTFP',
'XASH_ARMv4',
'XASH_ARMv5',
'XASH_ARMv6',
'XASH_ARMv7',
'XASH_ARMv8',
'XASH_BIG_ENDIAN',
'XASH_DOS4GW',
'XASH_E2K',
'XASH_EMSCRIPTEN',
'XASH_FREEBSD',
'XASH_HAIKU',
'XASH_IOS',
'XASH_IRIX',
'XASH_JS',
'XASH_LINUX',
'XASH_LITTLE_ENDIAN',
'XASH_MIPS',
'XASH_MOBILE_PLATFORM',
'XASH_NETBSD',
'XASH_OPENBSD',
'XASH_POSIX',
'XASH_PPC',
'XASH_RISCV',
'XASH_RISCV_DOUBLEFP',
'XASH_RISCV_SINGLEFP',
'XASH_RISCV_SOFTFP',
'XASH_SERENITY',
'XASH_TERMUX',
'XASH_WIN32',
'XASH_X86',
'XASH_NSWITCH',
'XASH_PSVITA',
'XASH_WASI',
'XASH_WASM',
'XASH_SUNOS',
'XASH_HURD',
]

def configure(conf):
	conf.env.stash()
	conf.start_msg('Determining library postfix')
	tests = map(lambda x: {
		'fragment': CHECK_SYMBOL_EXISTS_FRAGMENT % (x, x),
		'includes': [conf.path.find_node('public/').abspath()],
		'define_name': x }, DEFINES )

	conf.multicheck(*tests, msg = '', mandatory = False, quiet = True)

	# engine/common/build.c
	if conf.env.XASH_ANDROID:
		buildos = "android"
	elif conf.env.XASH_WIN32:
		buildos = "win32"
	elif conf.env.XASH_LINUX:
		buildos = "linux"
	elif conf.env.XASH_APPLE:
		buildos = "apple"
	elif conf.env.XASH_FREEBSD:
		buildos = "freebsd"
	elif conf.env.XASH_NETBSD:
		buildos = "netbsd"
	elif conf.env.XASH_OPENBSD:
		buildos = "openbsd"
	elif conf.env.XASH_EMSCRIPTEN:
		buildos = "emscripten"
	elif conf.env.XASH_DOS4GW:
		buildos = "dos4gw" # unused, just in case
	elif conf.env.XASH_HAIKU:
		buildos = "haiku"
	elif conf.env.XASH_SERENITY:
		buildos = "serenityos"
	elif conf.env.XASH_NSWITCH:
		buildos = "nswitch"
	elif conf.env.XASH_PSVITA:
		buildos = "psvita"
	elif conf.env.XASH_IRIX:
		buildos = "irix"
	elif conf.env.XASH_WASI:
		buildos = "wasi"
	elif conf.env.XASH_SUNOS:
		buildos = "sunos"
	elif conf.env.XASH_HURD:
		buildos = "hurd"
	else:
		conf.fatal("Place your operating system name in build.h and library_naming.py!\n"
			"If this is a mistake, try to fix conditions above and report a bug")

	if conf.env.XASH_AMD64:
		buildarch = "amd64"
	elif conf.env.XASH_X86:
		buildarch = "i386"
	elif conf.env.XASH_ARM and conf.env.XASH_64BIT:
		buildarch = "arm64"
	elif conf.env.XASH_ARM:
		buildarch = "armv"
		if conf.env.XASH_ARMv8:
			buildarch += "8_32"
		elif conf.env.XASH_ARMv7:
			buildarch += "7"
		elif conf.env.XASH_ARMv6:
			buildarch += "6"
		elif conf.env.XASH_ARMv5:
			buildarch += "5"
		elif conf.env.XASH_ARMv4:
			buildarch += "4"
		else:
			raise conf.fatal('Unknown ARM')

		if conf.env.XASH_ARM_HARDFP:
			buildarch += "hf"
		else:
			buildarch += "l"
	elif conf.env.XASH_MIPS:
		buildarch = "mips"
		if conf.env.XASH_64BIT:
			buildarch += "64"
		if conf.env.XASH_LITTLE_ENDIAN:
			buildarch += "el"
	elif conf.env.XASH_RISCV:
		buildarch = "riscv"
		if conf.env.XASH_64BIT:
			buildarch += "64"
		else:
			buildarch += "32"

		if conf.env.XASH_RISCV_DOUBLEFP:
			buildarch += "d"
		elif conf.env.XASH_RISCV_SINGLEFP:
			buildarch += "f"
	elif conf.env.XASH_JS:
		buildarch = "javascript"
	elif conf.env.XASH_E2K:
		buildarch = "e2k"
	elif conf.env.XASH_PPC:
		buildarch = "ppc"
		if conf.env.XASH_64BIT:
			buildarch += "64"
		if conf.env.XASH_LITTLE_ENDIAN:
			buildarch += "el"
	elif conf.env.XASH_WASM:
		buildarch = "wasm"
		if conf.env.XASH_64BIT:
			buildarch += "64"
		else:
			buildarch += "32"
	else:
		raise conf.fatal("Place your architecture name in build.h and library_naming.py!\n"
			"If this is a mistake, try to fix conditions above and report a bug")

	node = conf.bldnode.make_node('true_postfix.txt')
	node.write('%s-%s' % (buildos, buildarch))

	if not conf.env.XASH_ANDROID and (conf.env.XASH_WIN32 or conf.env.XASH_LINUX or conf.env.XASH_APPLE):
		buildos = ''
		if conf.env.XASH_X86:
			buildarch = ''

	if buildos != '' and buildarch != '':
		postfix = '_%s_%s' % (buildos,buildarch)
	elif buildarch != '':
		postfix = '_%s' % buildarch
	else:
		postfix = ''

	conf.env.revert()
	conf.env.POSTFIX = postfix
	conf.end_msg(conf.env.POSTFIX)

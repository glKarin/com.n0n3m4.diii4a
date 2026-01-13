# -*- mode: python -*-
# DOOM build script
# TTimo <ttimo@idsoftware.com>
# http://scons.sourceforge.net

import sys, os, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile, string
import SCons

sys.path.append( 'sys/scons' )
import scons_utils

conf_filename='site.conf'
# choose configuration variables which should be saved between runs
# ( we handle all those as strings )
serialized=['CC', 'CXX', 'JOBS', 'BUILD', 'IDNET_HOST', 'DEDICATED',
	'DEBUG_MEMORY', 'LIBC_MALLOC', 'ID_NOLANADDRESS', 'ID_MCHECK', 'ALSA',
	'TARGET_CORE', 'TARGET_GAME', 'TARGET_D3XP', 'TARGET_MONO', 'TARGET_DEMO', 'NOCURL',
	'TARGET_CDOOM',
	'TARGET_D3LE',
	'TARGET_RIVENSIN',
	'TARGET_HARDCORPS',
	'TARGET_RAVEN',
	'TARGET_QUAKE4',
	'TARGET_HUMANHEAD',
	'TARGET_PREY',
	'BUILD_ROOT', 'BUILD_GAMEPAK', 'BASEFLAGS', 'SILENT', 'TARGET_OPENGL',
	'TARGET_ANDROID', 'NDK',
	'LIB_PATH',
	'MULTITHREAD', 'OPENSLES'
    ]

# help -------------------------------------------

help_string = """
Usage: scons [OPTIONS] [TARGET] [CONFIG]

[OPTIONS] and [TARGET] are covered in command line options, use scons -H

[CONFIG]: KEY="VALUE" [...]
a number of configuration options saved between runs in the """ + conf_filename + """ file
erase """ + conf_filename + """ to start with default settings again

CC (default gcc)
CXX (default g++)
	Specify C and C++ compilers (defaults gcc and g++)
	ex: CC="gcc-3.3"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc" CXX="ccache distcc g++"

JOBS (default 1)
	Parallel build

BUILD (default debug)
	Use debug-all/debug/release to select build settings
	ex: BUILD="release"
	debug-all: no optimisations, debugging symbols
	debug: -O -g
	release: all optimisations, including CPU target etc.

BUILD_ROOT (default 'build')
	change the build root directory

TARGET_GAME (default 1)
	Build the base game code

TARGET_D3XP (default 1)
	Build the d3xp game code

TARGET_CDOOM (default 0)
	Build the classic doom game code

TARGET_D3LE (default 0)
	Build the lost mission game code

TARGET_RIVENSIN (default 0)
	Build rivensin game code

TARGET_HARDCORPS (default 0)
	Build hardcorps game code

TARGET_QUAKE4 (default 0)
	Build quake4 game code

TARGET_RAVEN (default 0)
	Build the core(Raven)

TARGET_PREY (default 0)
	Build prey game code

TARGET_HUMANHEAD (default 0)
	Build the core(HumanHead)

MULTITHREAD (default 1)
	Multi-threading support

OPENSLES (default 1)
	OpenSLES support

BUILD_GAMEPAK (default 0)
	Build a game pak

BASEFLAGS (default '')
	Add compile flags

NOCONF (default 0, not saved)
	ignore site configuration and use defaults + command line only
	
SILENT ( default 0, saved )
	hide the compiler output, unless error

DEDICATED (default 0)
	Control regular / dedicated type of build:
	0 - client
	1 - dedicated server
	2 - both

TARGET_CORE (default 1)
	Build the core

TARGET_MONO (default 0)
	Build a monolithic binary

TARGET_DEMO (default 0)
	Build demo client ( both a core and game, no mono )
	NOTE: if you *only* want the demo client, set TARGET_CORE and TARGET_GAME to 0

TARGET_OPENGL (default 0)
	Build an OpenGL renderer instead of OpenGL ES2.0.
	GLSL shaders are compatible.

TARGET_ANDROID (default 0)
	Build the Android version.

NDK (default '')
	Directory containing the Android NDK (Native Development Kit)

IDNET_HOST (default to source hardcoded)
	Override builtin IDNET_HOST with your own settings

DEBUG_MEMORY (default 0)
	Enables memory logging to file
	
LIBC_MALLOC (default 1)
	Toggle idHeap memory / libc malloc usage
	When libc malloc is on, memory size statistics are wrong ( no _msize )

ID_NOLANADDRESS (default 0)
	Don't recognize any IP as LAN address. This is useful when debugging network
	code where LAN / not LAN influences application behaviour
	
ID_MCHECK (default 2)
	Perform heap consistency checking
	0: on in Debug / off in Release
	1 forces on, 2 forces off
	note that Doom has it's own block allocator/checking
	this should not be considered a replacement, but an additional tool

ALSA (default 1)
	enable ALSA sound backend support
	
NOCURL (default 0)
	set to 1 to disable usage of libcurl and http/ftp downloads feature
"""

Help( help_string )

# end help ---------------------------------------

# sanity -----------------------------------------

EnsureSConsVersion( 0, 96 )

# end sanity -------------------------------------

# system detection -------------------------------

# CPU type
cpu = commands.getoutput('uname -m')
exp = re.compile('i?86')
if exp.match(cpu):
	cpu = 'x86'
else:
	if (commands.getoutput('uname -p') == 'powerpc'):
		cpu = 'ppc'
g_os = 'Linux'

if ARGUMENTS.has_key( 'ARCH' ):
	cpu = ARGUMENTS['ARCH']

# end system detection ---------------------------

# default settings -------------------------------

CC = 'gcc'
CXX = 'g++'
JOBS = '16'
BUILD = 'debug'
DEDICATED = '0'
TARGET_CORE = '1'
TARGET_GAME = '1'
TARGET_D3XP = '1'
TARGET_CDOOM = '0'
TARGET_D3LE = '0'
TARGET_RIVENSIN = '0'
TARGET_HARDCORPS = '0'
TARGET_QUAKE4 = '0'
TARGET_RAVEN = '0'
TARGET_PREY = '0'
TARGET_HUMANHEAD = '0'
TARGET_MONO = '0'
TARGET_DEMO = '0'
TARGET_OPENGL = '0'
TARGET_ANDROID = '0'
NDK = ''
IDNET_HOST = ''
DEBUG_MEMORY = '0'
LIBC_MALLOC = '1'
ID_NOLANADDRESS = '0'
ID_MCHECK = '2'
BUILD_ROOT = 'build'
ALSA = '1'
NOCONF = '0'
NOCURL = '0'
BUILD_GAMEPAK = '0'
BASEFLAGS = ''
SILENT = '0'
LIB_PATH = ''
MULTITHREAD = '1'
OPENSLES = '1'

# end default settings ---------------------------

# site settings ----------------------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	site_dict = {}
	if (os.path.exists(conf_filename)):
		site_file = open(conf_filename, 'r')
		p = pickle.Unpickler(site_file)
		site_dict = p.load()
		print('Loading build configuration from ' + conf_filename + ':')
		for k, v in site_dict.items():
			exec_cmd = k + '=\'' + v + '\''
			print('  ' + exec_cmd)
			exec(exec_cmd)
else:
	print('Site settings ignored')

# end site settings ------------------------------

# command line settings --------------------------

for k in ARGUMENTS.keys():
	exec_cmd = k + '=\'' + ARGUMENTS[k] + '\''
	print('Command line: ' + exec_cmd)
	exec( exec_cmd )

# end command line settings ----------------------

# save site configuration ----------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	for k in serialized:
		exec_cmd = 'site_dict[\'' + k + '\'] = ' + k
		exec(exec_cmd)

	site_file = open(conf_filename, 'w')
	p = pickle.Pickler(site_file)
	p.dump(site_dict)
	site_file.close()

# end save site configuration ------------------

# general configuration, target selection --------

g_build = BUILD_ROOT + '/' + BUILD

SConsignFile( 'scons.signatures' )

if ( DEBUG_MEMORY != '0' ):
	g_build += '-debugmem'
	
if ( LIBC_MALLOC != '1' ):
	g_build += '-nolibcmalloc'

SetOption('num_jobs', JOBS)

LINK = CXX

# common flags
# BASE + CORE + OPT for engine
# BASE + GAME + OPT for game
# _noopt versions of the environements are built without the OPT

BASECPPFLAGS = [ ]
CORECPPPATH = [ ]
CORELIBPATH = [ ]
CORECPPFLAGS = [ ]
GAMECPPFLAGS = [ ]
BASELINKFLAGS = [ '-Wl,--no-undefined' ]
CORELINKFLAGS = [ '-Wl,--no-undefined' ]
#//k BASELINKFLAGS = [ '-Wl,--no-undefined', '-static-libstdc++' ]
#//k CORELINKFLAGS = [ '-Wl,--no-undefined', '-static-libstdc++' ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

BASECPPFLAGS.append( BASEFLAGS.split() )
BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' ) # -w
#BASECPPFLAGS.append( '-Wextra' )
BASECPPFLAGS.append( '-Wno-sign-compare' )
BASECPPFLAGS.append( '-Wno-unknown-pragmas' )
BASECPPFLAGS.append( '-Wno-psabi' )
# this define is necessary to make sure threading support is enabled in X
CORECPPFLAGS.append( '-DXTHREADS' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )
# gcc 4.0
BASECPPFLAGS.append( '-fpermissive' )

if ( g_os == 'Linux' ):
	# gcc 4.x option only - only export what we mean to from the game SO
	BASECPPFLAGS.append( '-fvisibility=hidden' )
# //k
#BASECPPFLAGS.append('-D_K_CLANG')
	#BASECPPFLAGS.append('-D__arm__')
# //k

if ( TARGET_ANDROID == '1' ):
	BASECPPFLAGS.append( '-D__ANDROID__')
	g_os = 'Android'
	if ( LIB_PATH != '' ):
		CORELIBPATH.append( LIB_PATH.split() )

if ( BUILD == 'debug-all' ):
	OPTCPPFLAGS = [ '-g', '-D_DEBUG' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'debug' ):
#//k OPTCPPFLAGS = [ '-g', '-O1', '-D_DEBUG' ]
	OPTCPPFLAGS = [ '-g', '-O0' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'release' ):
	# -fomit-frame-pointer: "-O also turns on -fomit-frame-pointer on machines where doing so does not interfere with debugging."
	#   on x86 have to set it explicitely
	# -finline-functions: implicit at -O3
	# -fschedule-insns2: implicit at -O2
	# no-unsafe-math-optimizations: that should be on by default really. hit some wonko bugs in physics code because of that
	if ( g_os == 'Android' ):
		OPTCPPFLAGS = [ '-O3', '-Wl,--no-undefined', '-ffast-math', '-fno-unsafe-math-optimizations', '-fomit-frame-pointer' ]
	else:
		OPTCPPFLAGS = [ '-O3', '-march=native', '-ffast-math', '-fno-unsafe-math-optimizations', '-fomit-frame-pointer' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '2'
else:
	print('Unknown build configuration ' + BUILD)
	sys.exit(0)

if ( DEBUG_MEMORY != '0' ):
	BASECPPFLAGS += [ '-DID_DEBUG_MEMORY', '-DID_REDIRECT_NEWDELETE' ]
	
if ( LIBC_MALLOC != '1' ):
	BASECPPFLAGS.append( '-DUSE_LIBC_MALLOC=0' )

if ( len( IDNET_HOST ) ):
	CORECPPFLAGS.append( '-DIDNET_HOST=\\"%s\\"' % IDNET_HOST)

if ( ID_NOLANADDRESS != '0' ):
	CORECPPFLAGS.append( '-DID_NOLANADDRESS' )
	
if ( ID_MCHECK == '1' ):
	BASECPPFLAGS.append( '-DID_MCHECK' )
	
# create the build environements
g_base_env = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH, NDK = NDK, MULTITHREAD = MULTITHREAD, OPENSLES = OPENSLES )
scons_utils.SetupUtils( g_base_env ) 

g_env = g_base_env.Clone()

g_env['CPPFLAGS'] += OPTCPPFLAGS
g_env['CPPFLAGS'] += CORECPPFLAGS
g_env['LINKFLAGS'] += CORELINKFLAGS

g_env_noopt = g_base_env.Clone()
g_env_noopt['CPPFLAGS'] += CORECPPFLAGS

g_game_env = g_base_env.Clone()
g_game_env['CPPFLAGS'] += OPTCPPFLAGS
g_game_env['CPPFLAGS'] += GAMECPPFLAGS

# maintain this dangerous optimization off at all times
g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_env_noopt.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_game_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

if ( int(JOBS) > 1 ):
	print('Using buffered process output')
	silent = False
	if ( SILENT == '1' ):
		silent = True
	scons_utils.SetupBufferedOutput( g_env, silent )
	scons_utils.SetupBufferedOutput( g_game_env, silent )

# mark the globals

local_dedicated = 0
# 0 for monolithic build
local_gamedll = 1
# carry around rather than using .a, avoids binutils bugs
idlib_objects = []
game_objects = []
local_demo = 0
# curl usage. there is a global toggle flag
local_curl = 0
# if idlib should produce PIC objects ( depending on core or game inclusion )
local_idlibpic = 0
# switch between base game build and d3xp game build
local_d3xp = 0
local_cdoom = 0
local_d3le = 0
local_rivensin = 0
local_hardcorps = 0
local_quake4 = 0
local_prey = 0
local_raven = 0
local_humanhead = 0
# OpenGL
local_opengl = 0

GLOBALS = 'g_env g_env_noopt g_game_env g_os ID_MCHECK ALSA idlib_objects game_objects local_dedicated local_gamedll local_demo local_idlibpic local_curl local_d3xp local_cdoom local_d3le local_rivensin local_hardcorps local_quake4 local_raven local_prey local_humanhead local_opengl OPTCPPFLAGS NDK MULTITHREAD OPENSLES'

# end general configuration ----------------------

# targets ----------------------------------------

Export( 'GLOBALS ' + GLOBALS )

doom = None
doomded = None
game = None
doom_mono = None
doom_demo = None
game_demo = None

if ( TARGET_OPENGL != '0' ):
	local_opengl = 1
	Export( 'GLOBALS ' + GLOBALS )

if ( TARGET_ANDROID != '0' ):
	Export( 'GLOBALS ' + GLOBALS )

if ( TARGET_ANDROID != '' ):
	Export( 'GLOBALS ' + GLOBALS )

# Raven Quake4 / Humanhead Prey
if ( TARGET_QUAKE4 == '1' ):
	local_quake4 = 1
	local_raven = 1
	Export( 'GLOBALS ' + GLOBALS )
elif ( TARGET_RAVEN == '1' ):
	local_raven = 1
	Export( 'GLOBALS ' + GLOBALS )
if ( TARGET_PREY == '1' ):
	local_humanhead = 1
	local_prey = 1
	Export( 'GLOBALS ' + GLOBALS )
elif ( TARGET_HUMANHEAD == '1' ):
	local_humanhead = 1
	Export( 'GLOBALS ' + GLOBALS )

# build curl if needed
if ( NOCURL == '0' and ( TARGET_CORE == '1' or TARGET_MONO == '1' or TARGET_RAVEN == '1' or TARGET_HUMANHEAD == '1' ) ):
	local_curl = 1
	Export( 'GLOBALS ' + GLOBALS )

if ( TARGET_CORE == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_idlibpic = 0
	local_raven = 0
	local_humanhead = 0
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
	
        VariantDir( g_build + '/core', '.', duplicate = 0 )
        idlib_objects = SConscript( g_build + '/core/sys/scons/SConscript.idlib' )
        Export( 'GLOBALS ' + GLOBALS ) # update idlib_objects
        doom = SConscript( g_build + '/core/sys/scons/SConscript.core' )

        InstallAs( '#doom.' + cpu, doom )
        InstallAs( g_build + '/package/libidtech4.so', doom )
		
	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		
		VariantDir( g_build + '/dedicated', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/dedicated/sys/scons/SConscript.idlib' )
		Export( 'GLOBALS ' + GLOBALS )
		doomded = SConscript( g_build + '/dedicated/sys/scons/SConscript.core' )

		InstallAs( '#doomded.' + cpu, doomded )

if ( TARGET_RAVEN == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_idlibpic = 0
	local_raven = 1
	local_humanhead = 0
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
	
        VariantDir( g_build + '/raven', '.', duplicate = 0 )
        idlib_objects = SConscript( g_build + '/raven/sys/scons/SConscript.idlib' )
        Export( 'GLOBALS ' + GLOBALS ) # update idlib_objects
        doom_raven = SConscript( g_build + '/raven/sys/scons/SConscript.core' )

        InstallAs( '#doom_raven.' + cpu, doom_raven )
        InstallAs( g_build + '/package/libidtech4_raven.so', doom_raven )
		
	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		
		VariantDir( g_build + '/raven_dedicated', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/raven_dedicated/sys/scons/SConscript.idlib' )
		Export( 'GLOBALS ' + GLOBALS )
		doomded = SConscript( g_build + '/raven_dedicated/sys/scons/SConscript.core' )

		InstallAs( '#doomded_raven.' + cpu, doomded )

if ( TARGET_HUMANHEAD == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_idlibpic = 0
	local_raven = 0
	local_humanhead = 1
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
	
        VariantDir( g_build + '/humanhead', '.', duplicate = 0 )
        idlib_objects = SConscript( g_build + '/humanhead/sys/scons/SConscript.idlib' )
        Export( 'GLOBALS ' + GLOBALS ) # update idlib_objects
        doom_humanhead = SConscript( g_build + '/humanhead/sys/scons/SConscript.core' )

        InstallAs( '#doom_humanhead.' + cpu, doom_humanhead )
        InstallAs( g_build + '/package/libidtech4_humanhead.so', doom_humanhead )
		
	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		
		VariantDir( g_build + '/humanhead_dedicated', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/raven_dedicated/sys/scons/SConscript.idlib' )
		Export( 'GLOBALS ' + GLOBALS )
		doomded = SConscript( g_build + '/humanhead_dedicated/sys/scons/SConscript.core' )

		InstallAs( '#doomded_humanhead.' + cpu, doomded )


if ( TARGET_GAME == '1' or TARGET_D3XP == '1' or TARGET_CDOOM == '1' or TARGET_D3LE == '1' or TARGET_RIVENSIN == '1' or TARGET_HARDCORPS == '1' or TARGET_QUAKE4 == '1' or TARGET_PREY == '1' ):
	local_gamedll = 1
	local_demo = 0
	local_dedicated = 0
	local_idlibpic = 1
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/game', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/game/sys/scons/SConscript.idlib' )
	if ( TARGET_GAME == '1' ):
		local_d3xp = 0
		local_cdoom = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		Export( 'GLOBALS ' + GLOBALS )
		game = SConscript( g_build + '/game/sys/scons/SConscript.game' )
		game_base = InstallAs( '#game%s-base.so' % cpu, game )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-base.pk4', [ game_base, game ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libgame.so', game )
	if ( TARGET_D3XP == '1' ):
		# uses idlib as compiled for game/
		local_d3xp = 1
		local_cdoom = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/d3xp', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		d3xp = SConscript( g_build + '/d3xp/sys/scons/SConscript.game' )
		game_d3xp = InstallAs( '#game%s-d3xp.so' % cpu, d3xp )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-d3xp.pk4', [ game_d3xp, d3xp ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libd3xp.so', d3xp )
	if ( TARGET_CDOOM == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 1
		local_d3xp = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/cdoom', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		cdoom = SConscript( g_build + '/cdoom/sys/scons/SConscript.game' )
		game_cdoom = InstallAs( '#game%s-cdoom.so' % cpu, cdoom )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-cdoom.pk4', [ game_cdoom, cdoom ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libcdoom.so', cdoom )
	if ( TARGET_D3LE == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 0
		local_d3xp = 0
		local_d3le = 1
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/d3le', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		d3le = SConscript( g_build + '/d3le/sys/scons/SConscript.game' )
		game_d3le = InstallAs( '#game%s-d3le.so' % cpu, d3le )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-d3le.pk4', [ game_d3le, d3le ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libd3le.so', d3le )
	if ( TARGET_RIVENSIN == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 0
		local_d3xp = 0
		local_d3le = 0
		local_rivensin = 1
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/rivensin', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		rivensin = SConscript( g_build + '/rivensin/sys/scons/SConscript.game' )
		game_rivensin = InstallAs( '#game%s-rivensin.so' % cpu, rivensin )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-rivensin.pk4', [ game_rivensin, rivensin ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/librivensin.so', rivensin )
	if ( TARGET_HARDCORPS == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 0
		local_d3xp = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 1
		local_quake4 = 0
		local_raven = 0
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/hardcorps', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		hardcorps = SConscript( g_build + '/hardcorps/sys/scons/SConscript.game' )
		game_hardcorps = InstallAs( '#game%s-hardcorps.so' % cpu, hardcorps )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-hardcorps.pk4', [ game_hardcorps, hardcorps ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libhardcorps.so', hardcorps )
	if ( TARGET_QUAKE4 == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 0
		local_d3xp = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 1
		local_raven = 1
		local_prey = 0
		local_humanhead = 0
		VariantDir( g_build + '/quake4', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		quake4 = SConscript( g_build + '/quake4/sys/scons/SConscript.game' )
		game_quake4 = InstallAs( '#game%s-q4game.so' % cpu, quake4 )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-quake4.pk4', [ game_quake4, quake4 ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libq4game.so', quake4 )
	if ( TARGET_PREY == '1' ):
		# uses idlib as compiled for game/
		local_cdoom = 0
		local_d3xp = 0
		local_d3le = 0
		local_rivensin = 0
		local_hardcorps = 0
		local_quake4 = 0
		local_raven = 0
		local_prey = 1
		local_humanhead = 1
		VariantDir( g_build + '/prey', '.', duplicate = 0 )
		Export( 'GLOBALS ' + GLOBALS )
		prey = SConscript( g_build + '/prey/sys/scons/SConscript.game' )
		game_prey = InstallAs( '#game%s-preygame.so' % cpu, prey )
		if ( BUILD_GAMEPAK == '1' ):
			Command( '#game01-prey.pk4', [ game_prey, prey ], Action( g_env.BuildGamePak ) )
		InstallAs( g_build + '/package/libpreygame.so', prey )
	
	
if ( TARGET_MONO == '1' ):
	# NOTE: no D3XP atm. add a TARGET_MONO_D3XP
	local_gamedll = 0
	local_dedicated = 0
	local_demo = 0
	local_idlibpic = 0
	local_d3xp = 0
	local_cdoom = 0
	local_d3le = 0
	local_rivensin = 0
	local_hardcorps = 0
	local_quake4 = 0
	local_raven = 0
	local_prey = 0
	local_humanhead = 0
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/mono', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/mono/sys/scons/SConscript.idlib' )
	game_objects = SConscript( g_build + '/mono/sys/scons/SConscript.game' )
	Export( 'GLOBALS ' + GLOBALS )
	doom_mono = SConscript( g_build + '/mono/sys/scons/SConscript.core' )
	InstallAs( '#doom-mon.' + cpu, doom_mono )

if ( TARGET_DEMO == '1' ):
	# NOTE: no D3XP atm. add a TARGET_DEMO_D3XP
	local_demo = 1
	local_dedicated = 0
	local_gamedll = 1
	local_idlibpic = 0
	local_curl = 0
	local_d3xp = 0
	local_cdoom = 0
	local_d3le = 0
	local_rivensin = 0
	local_hardcorps = 0
	local_quake4 = 0
	local_raven = 0
	local_prey = 0
	local_humanhead = 0
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/demo', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/demo/sys/scons/SConscript.idlib' )
	Export( 'GLOBALS ' + GLOBALS )
	doom_demo = SConscript( g_build + '/demo/sys/scons/SConscript.core' )

	InstallAs( '#doom-demo.' + cpu, doom_demo )
	
	local_idlibpic = 1
	Export( 'GLOBALS ' + GLOBALS )
	VariantDir( g_build + '/demo/game', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/demo/game/sys/scons/SConscript.idlib' )
	Export( 'GLOBALS ' + GLOBALS )
	game_demo = SConscript( g_build + '/demo/game/sys/scons/SConscript.game' )

	InstallAs( '#game%s-demo.so' % cpu, game_demo )

# end targets ------------------------------------

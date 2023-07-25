# -*- mode: python -*-
# ETQW build script
# TTimo <ttimo@idsoftware.com>
# http://scons.org

import sys, os, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile, string
import SCons

sys.path.append( 'sys/scons' )
import scons_utils

conf_filename='site.conf'
# choose configuration variables which should be saved between runs
# ( we handle all those as strings )
serialized=[ 'CC', 'CXX', 'JOBS', 'BUILD', 'GL_HARDLINK', 'DEDICATED',
	'ID_NOLANADDRESS', 'ID_MCHECK',
	'TARGET_CORE', 'TARGET_GAME', 'TARGET_MONO', 'NOCURL',
	'BUILD_ROOT', 'SILENT', 'BETA', 'DEMO', 'RANKED', 'TARGET_SCRIPT', 'SCRIPT_SOURCE', 'SCRIPT_FOLDER' ]

# global build mode ------------------------------

g_sdk = not os.path.exists( 'sys/scons/SConscript.core' )

# ------------------------------------------------

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
	ex: CC="gcc-4.1"
	You can use ccache and distcc, for instance:
	CC="ccache distcc gcc-4.1" CXX="ccache distcc g++-4.1"

JOBS (default 1)
	Parallel build

BUILD (default debug)
	Use debug-all/debug/release to select build settings
	ex: BUILD="release"
	debug-all: no optimisations, debugging symbols
	debug: -O -g
	release: all optimisations, including CPU target etc.
	release-test: mostly release, plus debugging and various things to help crash report analysis

BUILD_ROOT (default 'build')
	change the build root directory

TARGET_SCRIPT (default 0, saved)
	Produce a compiled scripts shared object

SCRIPT_SOURCE (default '~/.etqw/base/src', saved)
	Point to the directory in which the compiled scripts source resides
	You can point to a tarball, which will get expanded, and subsequent zips inside it will be expanded as well

SCRIPT_FOLDER (saved)
	Give a folder name in the expanded tarball so we can have a single tarball with compiled script source for multiple versions (branches support)

NOCONF (default 0, not saved)
	ignore site configuration and use defaults + command line only

SILENT ( default 0, saved )
	hide the compiler output, unless error
"""

if ( not g_sdk ):
	help_string += """
DEDICATED (default 0)
	Control regular / dedicated type of build:
	0 - client
	1 - dedicated server
	2 - both

TARGET_CORE (default 1)
	Build the core

TARGET_GAME (default 1)
	Build the game code

TARGET_MONO (default 0)
	Build a monolithic binary

GL_HARDLINK (default 0)
	Instead of dynamically loading the OpenGL libraries, use implicit dependencies
	NOTE: no GL logging capability and no r_glDriver with GL_HARDLINK 1

ID_NOLANADDRESS (default 0)
	Don't recognize any IP as LAN address. This is useful when debugging network
	code where LAN / not LAN influences application behaviour
	
ID_MCHECK (default 2)
	Perform heap consistency checking
	0: on in Debug / off in Release
	1 forces on, 2 forces off

SETUP (default 0, not saved)
    package release configuration binaries into the bin/ directory
	( can use BUILD=release and BUILD=release-test )
	if TARGET_GAME == 1, compile and create the game pak
	if TARGET_CORE == 1, compile and process the core binaries
	if SETUP == 2, assemble the data into a setup (wether we compile anything new depends on above settings)

SDK (default 0, not saved)
	build an SDK release

NOCURL (default 0)
	set to 1 to disable usage of libcurl and http/ftp downloads feature

BETA (default 0)
	1 or public: enable public beta build
	2 or private: enable private beta build

DEMO (default 0)
	set to 1 for demo build (implies BETA=0 RANKED=0)

RANKED (default 0)
	set to 1 to enable ranked dedicated server build
"""

Help( help_string )

# end help ---------------------------------------

# sanity -----------------------------------------
# end sanity -------------------------------------

# system detection -------------------------------

# OS and CPU
OS = commands.getoutput( 'uname -s' )
if ( OS == 'Linux' ):
	cpu = commands.getoutput( 'uname -m' )
	# when building in a 32 bit x86_64 chroot, make sure to use linux32 to remap properly
	if ( cpu == 'i686' ):
		cpu = 'x86'
	elif ( cpu == 'x86_64' ):
		cpu = 'x64'
	else:
		cpu = 'cpu'
elif ( OS == 'Darwin' ):
	cpu = commands.getoutput( 'uname -m' )
	if ( cpu == 'Power Macintosh' ):
		cpu = 'ppc'
	else:
		cpu = 'cpu'

# end system detection ---------------------------

# default settings -------------------------------

CC = 'gcc-4.1'
CXX = 'g++-4.1'
JOBS = '1'
if ( not g_sdk ):
	BUILD = 'debug'
else:
	BUILD = 'release'
DEDICATED = '0'
if ( not g_sdk ):
	TARGET_CORE = '1'
else:
	TARGET_CORE = '0'
TARGET_GAME = '1'
TARGET_MONO = '0'
TARGET_SCRIPT = '0'
SCRIPT_SOURCE = os.path.expanduser( '~/.etqw/base/src' )
SCRIPT_FOLDER = ''
GL_HARDLINK = '0'
ID_NOLANADDRESS = '0'
ID_MCHECK = '2'
BUILD_ROOT = 'build'
SETUP = '0'
SDK = '0'
NOCONF = '0'
NOCURL = '0'
SILENT = '0'
BETA = '0'
DEMO = '0'
RANKED = '0'

# end default settings ---------------------------

# site settings ----------------------------------

if ( not ARGUMENTS.has_key( 'NOCONF' ) or ARGUMENTS['NOCONF'] != '1' ):
	site_dict = {}
	if (os.path.exists(conf_filename)):
		site_file = open(conf_filename, 'r')
		p = pickle.Unpickler(site_file)
		site_dict = p.load()
		print 'Loading build configuration from ' + conf_filename + ':'
		for k, v in site_dict.items():
			exec_cmd = k + '=\'' + v + '\''
			print '  ' + exec_cmd
			exec(exec_cmd)
else:
	print 'Site settings ignored'

# end site settings ------------------------------

# command line settings --------------------------

for k in ARGUMENTS.keys():
	exec_cmd = k + '=\'' + ARGUMENTS[k] + '\''
	print 'Command line: ' + exec_cmd
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

# configuration rules --------------------------

if ( SETUP != '0' ):
	# allow either release or release-test
	if ( BUILD.find( 'release' ) == -1 ):
		BUILD = 'release'

if ( g_sdk or SDK != '0' ):
	TARGET_CORE = '0'
	TARGET_GAME = '1'
	TARGET_MONO = '0'

if ( BETA.lower() == 'public' ):
	BETA = '1'
if ( BETA.lower() == 'private' ):
	BETA = '2'

# end configuration rules ----------------------

# general configuration, target selection --------

g_build = BUILD_ROOT + '/' + BUILD

SConsignFile( 'scons.signatures' )

if ( GL_HARDLINK != '0' ):
	g_build += '-hardlink'

if ( DEMO != '0' ):
	BETA = '0'
	RANKED = '0'
	g_build += '-beta'

if ( BETA != '0' ):
	if ( BETA == '1' ):
		g_build += '-pubbeta'
	else:
		g_build += '-privbeta'

if ( RANKED != '0' ):
	g_build += '-ranked'

SetOption('num_jobs', JOBS)

LINK = CXX

# common flags
# BASE + CORE + OPT for engine
# BASE + GAME + OPT for game
# _noopt versions of the environements are built without the OPT

BASECPPFLAGS = [ ]
BASECXXFLAGS = [ ]
CORECPPPATH = [ ]
CORELIBPATH = [ ]
CORECPPFLAGS = [ ]
GAMECPPFLAGS = [ ]
BASELINKFLAGS = [ ]
CORELINKFLAGS = [ ]

# for release build, further optimisations that may not work on all files
OPTCPPFLAGS = [ ]

BASECPPFLAGS.append( '-pipe' )
# warn all
BASECPPFLAGS.append( '-Wall' )
# this define is necessary to make sure threading support is enabled in X
CORECPPFLAGS.append( '-DXTHREADS' )
# don't wrap gcc messages
BASECPPFLAGS.append( '-fmessage-length=0' )
# gcc 4.0
BASECXXFLAGS.append( '-fpermissive' )
BASECXXFLAGS.append( '-fvisibility=hidden' )
BASECXXFLAGS.append( '-fvisibility-inlines-hidden' )

# so I can have 64 bit machines in the distcc array
BASECPPFLAGS.append( '-m32' )

if ( g_sdk ):
	BASECPPFLAGS.append( '-DSD_SDK_BUILD' )
	BASECPPFLAGS.append( '-Igame' )

if ( BETA == '1' ):
	CORECPPFLAGS.append( '-DSD_PUBLIC_BETA_BUILD' )
	
if ( BETA == '2' ):
	CORECPPFLAGS.append( '-DSD_PRIVATE_BETA_BUILD' )

if ( DEMO == '1' ):
	CORECPPFLAGS.append( '-DSD_DEMO_BUILD' )

if ( BUILD == 'debug-all' ):
	BASECPPFLAGS.append( '-g' )
	BASELINKFLAGS.append( '-g' )
	BASECPPFLAGS.append( '-D_DEBUG' )
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'debug' ):
	BASECPPFLAGS.append( '-g' )
	BASELINKFLAGS.append( '-g' )
	BASECPPFLAGS.append( '-O1' )
	BASECPPFLAGS.append( '-D_DEBUG' )
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '1'
elif ( BUILD == 'release' or BUILD == 'release-test' ):
	OPTCPPFLAGS = [ '-O3', '-march=pentium3', '-Winline', '-ffast-math', '-fno-unsafe-math-optimizations', '-fomit-frame-pointer' ]
	if ( ID_MCHECK == '0' ):
		ID_MCHECK = '2'
	if ( BUILD == 'release-test' ):
		# a release configuration that makes it slightly easier to track down crashes and get backtraces
		# and put debugging symbols in still (we strip before distributing if needed)
		BASECPPFLAGS.append( '-g' )
		# some extra sanity code may be enabled - for instance the WriteBits overflow nastyness
		BASECPPFLAGS.append( '-DID_RELEASE_TEST' )
		# this affects ability to obtain proper return adresses
		OPTCPPFLAGS.remove( '-fomit-frame-pointer' )
	else:
		# TMP - let's skip this on release builds for now so we can get better crash reports
		OPTCPPFLAGS.remove( '-fomit-frame-pointer' )
else:
	print 'Unknown build configuration ' + BUILD
	sys.exit(0)

if ( GL_HARDLINK != '0' ):
	CORECPPFLAGS.append( '-DID_GL_HARDLINK' )

if ( ID_NOLANADDRESS != '0' ):
	CORECPPFLAGS.append( '-DID_NOLANADDRESS' )
	
if ( ID_MCHECK == '1' ):
	BASECPPFLAGS.append( '-DID_MCHECK' )
	
# create the build environements
g_base_env = Environment( ENV = os.environ, CC = CC, CXX = CXX, LINK = LINK, CPPFLAGS = BASECPPFLAGS, CXXFLAGS = BASECXXFLAGS, LINKFLAGS = BASELINKFLAGS, CPPPATH = CORECPPPATH, LIBPATH = CORELIBPATH )
scons_utils.SetupUtils( g_base_env )

g_env = g_base_env.Clone()

g_env['CPPFLAGS'] += OPTCPPFLAGS
g_env['CPPFLAGS'] += CORECPPFLAGS
g_env['LINKFLAGS'] += CORELINKFLAGS

g_game_env = g_base_env.Clone()
g_game_env['CPPFLAGS'] += OPTCPPFLAGS
g_game_env['CPPFLAGS'] += GAMECPPFLAGS

# maintain this dangerous optimization off at all times
g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )
g_game_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

if ( int(JOBS) > 1 ):
	print 'Using buffered process output'
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
glimp_objects = []
qgllib_objects = []
imagelib_objects = []
game_objects = []
# curl usage. there is a global toggle flag
local_curl = 0
curl_lib = []
# freetype
freetype_root = 'libs/freetype/freetype-2.2.1'
freetype_lib = []
# speex
speex_lib = []
# if idlib should produce PIC objects ( depending on core or game inclusion )
local_idlibpic = 0
version_file = []
version = None
CPPPATH_SPEEX = ''
# demonware (include paths, libraries)
# need a path value even when we're not building it for includes
demonware_basepath = 'libs/DemonWare/DemonWare-1.80'
demonware_info = [ Dir( demonware_basepath ) ]
# conf the ogg library is done once at toplevel
ogg_config = None

GLOBALS = 'g_env g_game_env OS ID_MCHECK idlib_objects glimp_objects qgllib_objects game_objects local_dedicated local_gamedll local_idlibpic curl_lib local_curl speex_lib imagelib_objects version_file version CPPPATH_SPEEX BETA RANKED demonware_info ogg_config SCRIPT_SOURCE SCRIPT_FOLDER freetype_root freetype_lib g_sdk'

# end general configuration ----------------------

# targets ----------------------------------------

Export( 'GLOBALS ' + GLOBALS )

etqw = None
etqwded = None
game = None
etqw_mono = None
etqw_demo = None
game_demo = None

if ( TARGET_SCRIPT == '1' ):
	BuildDir( os.path.join( g_build, 'compiledscript' ), '.', duplicate = 0 )
	compiledscript = SConscript( g_build + '/compiledscript/sys/scons/SConscript.compiledscript' )
	InstallAs( '#compiledscriptx86.so', compiledscript )

# build various pieces used by the core if one is going to be built
if ( TARGET_CORE == '1' or TARGET_MONO == '1' ):
	# curl
	if ( NOCURL == '0' ):
		if ( BUILD.find( 'release' ) != -1 ):
			local_curl = 2
		else:
			local_curl = 1
		Export( 'GLOBALS ' + GLOBALS )
		curl_lib = SConscript( 'sys/scons/SConscript.curl' )
	# freetype
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		freetype_lib = SConscript( 'sys/scons/SConscript.freetype' )
	# speex
	Export( 'GLOBALS ' + GLOBALS )	
	( speex_lib, CPPPATH_SPEEX ) = SConscript( 'sys/scons/SConscript.speex' )
	speex_lib = [ speex_lib ]
	# image lib
	BuildDir( os.path.join( g_build, 'imagelib' ), '.', duplicate = 0 )
	imagelib_objects = SConscript( g_build + '/imagelib/sys/scons/SConscript.imagelib' )
	# demonware
	Export( 'CC CXX BUILD' )
	BuildDir( os.path.join( g_build, 'demonware' ), '.', duplicate = 0 )
	demonware_info = SConscript( os.path.join( g_build, 'demonware', demonware_basepath, 'SConscript' ) )
	# ogg configuration (the objects are still compiled with each target's settings)
	ogg_config = SConscript( 'sys/scons/SConscript.oggconfig' )

# version file - compile it once at the top
BuildDir( g_build, '.', duplicate = 0 )
if ( g_sdk ):
	version_file = [ [ 'framework/BuildVersion.h' ], [ '#framework/BuildVersion.cpp' ] ] # not generated in SDK builds
	version = '-1.-1'
else:
	( version_file, version ) = SConscript( 'sys/scons/SConscript.version' )

if ( TARGET_CORE == '1' ):
	local_gamedll = 1
	local_idlibpic = 0
	if ( DEDICATED == '0' or DEDICATED == '2' ):
		local_dedicated = 0
		Export( 'GLOBALS ' + GLOBALS )
		
		BuildDir( g_build + '/core/glimp', '.', duplicate = 1 )
		glimp_objects = SConscript( g_build + '/core/glimp/sys/scons/SConscript.gl' )
		BuildDir( g_build + '/core', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/core/sys/scons/SConscript.idlib' )
		qgllib_objects  = SConscript( g_build + '/core/sys/scons/SConscript.qgllib' )
		Export( 'GLOBALS ' + GLOBALS ) # update global objects
		etqw = SConscript( g_build + '/core/sys/scons/SConscript.core' )

		InstallAs( '#etqw.' + cpu, etqw )

		
	if ( DEDICATED == '1' or DEDICATED == '2' ):
		local_dedicated = 1
		Export( 'GLOBALS ' + GLOBALS )
		
		BuildDir( g_build + '/dedicated/glimp', '.', duplicate = 1 )
		glimp_objects = SConscript( g_build + '/dedicated/glimp/sys/scons/SConscript.gl' )
		BuildDir( g_build + '/dedicated', '.', duplicate = 0 )
		idlib_objects = SConscript( g_build + '/dedicated/sys/scons/SConscript.idlib' )
		qgllib_objects  = SConscript( g_build + '/dedicated/sys/scons/SConscript.qgllib' )
		Export( 'GLOBALS ' + GLOBALS )
		etqwded = SConscript( g_build + '/dedicated/sys/scons/SConscript.core' )
		
		InstallAs( '#etqwded.' + cpu, etqwded )

if ( TARGET_GAME == '1' ):
	local_gamedll = 1
	local_dedicated = 0
	local_idlibpic = 1
	Export( 'GLOBALS ' + GLOBALS )
	dupe = 0
	if ( SDK == '1' ):
		# building an SDK, use scons for dependencies walking
		# clear the build directory to be safe
		g_env.PreBuildSDK( g_build + '/game' )
		dupe = 1
	BuildDir( g_build + '/game', '.', duplicate = dupe )
	idlib_objects = SConscript( g_build + '/game/sys/scons/SConscript.idlib' )
	Export( 'GLOBALS ' + GLOBALS )
	game = SConscript( g_build + '/game/sys/scons/SConscript.game' )

	InstallAs( '#game%s.so' % cpu, game )
	
if ( TARGET_MONO == '1' ):
	# the game in a single piece
	local_gamedll = 0
	local_dedicated = 0
	local_idlibpic = 0
	Export( 'GLOBALS ' + GLOBALS )
	BuildDir( g_build + '/mono/glimp', '.', duplicate = 1 )
	glimp_objects = SConscript( g_build + '/mono/glimp/sys/scons/SConscript.gl' )
	BuildDir( g_build + '/mono', '.', duplicate = 0 )
	idlib_objects = SConscript( g_build + '/mono/sys/scons/SConscript.idlib' )
	qgllib_objects  = SConscript( g_build + '/mono/sys/scons/SConscript.qgllib' )
	game_objects = SConscript( g_build + '/mono/sys/scons/SConscript.game' )
	Export( 'GLOBALS ' + GLOBALS )
	etqw_mono = SConscript( g_build + '/mono/sys/scons/SConscript.core' )

	InstallAs( '#etqw-mon.' + cpu, etqw_mono )

if ( SETUP != '0' ):
	# lacking a better way
	g_env.setup.version = version
	g_env.setup.build = BUILD
	g_env.setup.beta = ( BETA != '0' )
	g_env.setup.ranked = ( RANKED != '0' )
	g_env.setup.dedicated = int( DEDICATED )
	brandelf = Program( 'brandelf', 'sys/linux/setup/brandelf.c' )
	# compile the scripts?
	if ( TARGET_SCRIPT == '1' ):
		compiled_assembly = Command( 'compiled_assembly', [ compiledscript ], Action( g_env.setup.DoCompiledScript ) )
		Default( compiled_assembly )
	else:
		compiled_assembly = None
	# build a game pak?
	if ( TARGET_GAME == '1' ):
		deps = [ game ]
		# the compiled scripts are put in the game pak
		if ( not compiled_assembly is None ):
			deps.append( compiled_assembly )
		gamepak = Command( 'gamepak', deps, Action( g_env.setup.BuildGamePak ) )
		Default( gamepak )
	else:
		gamepak = None
	# build the core? (dedicated, and/or client)
	if ( TARGET_CORE == '1' ):
		deps = []
		if ( DEDICATED == '1' or DEDICATED == '2' ):
			deps.append( etqwded )
		if ( DEDICATED == '0' or DEDICATED == '2' ):
			deps.append( etqw )
		if ( gamepak != None ):
			# those two steps are actually independent, but it wouldn't make sense to paralellize them
			deps.append( gamepak )
		corefiles = Command( 'corefiles', deps, Action( g_env.setup.PrepareCoreFiles ) )
		Default( corefiles )
	else:
		corefiles = None
	# assemble a setup?
	if ( SETUP == '2' ):
		deps = []
		# make sure this step always comes last if some things have to be compiled
		if ( gamepak != None ):
			deps.append( gamepak )
		if ( corefiles != None ):
			deps.append( corefiles )
		setup = Command( 'setup', deps, Action( g_env.setup.BuildSetup ) )
		Default( setup )

if ( SDK != '0' ):
	setup_sdk = Command( 'sdk', [ ], Action( g_env.BuildSDK ) )
	g_env.Depends( setup_sdk, game ) 
	
# end targets ------------------------------------

# -*- mode: python -*-
import sys, os, string, time, commands, re, pickle, StringIO, popen2, commands, pdb, zipfile
import SCons

# need an Environment and a matching buffered_spawn API .. encapsulate
class idBuffering:
	silent = False

	def buffered_spawn( self, sh, escape, cmd, args, env ):
		stderr = StringIO.StringIO()
		stdout = StringIO.StringIO()
		command_string = ''
		for i in args:
			if ( len( command_string ) ):
				command_string += ' '
			command_string += i
		try:
			retval = self.env['PSPAWN']( sh, escape, cmd, args, env, stdout, stderr )
		except OSError, x:
			if x.errno != 10:
				raise x
			print 'OSError ignored on command: %s' % command_string
			retval = 0
		print command_string
		if ( retval != 0 or not self.silent ):
			sys.stdout.write( stdout.getvalue() )
			sys.stderr.write( stderr.getvalue() )
		return retval		

class idSetupBase:
	
	def SimpleCommand( self, cmd ):
		print cmd
		ret = commands.getstatusoutput( cmd )
		if ( len( ret[ 1 ] ) ):
			sys.stdout.write( ret[ 1 ] )
			sys.stdout.write( '\n' )
		if ( ret[ 0 ] != 0 ):
			raise 'command failed'
		return ret[ 1 ]

	def TrySimpleCommand( self, cmd ):
		print cmd
		ret = commands.getstatusoutput( cmd )
		sys.stdout.write( ret[ 1 ] )

	def M4Processing( self, file, d ):
		file_out = file[:-3]
		cmd = 'm4 '
		for ( key, val ) in d.items():
			cmd += '--define=%s="%s" ' % ( key, val )
		cmd += '%s > %s' % ( file, file_out )
		self.SimpleCommand( cmd )	

	def ExtractProtocolVersion( self ):
		f = open( 'framework/Licensee.h' )
		l = f.readlines()
		f.close()

		major = 'X'
		p = re.compile( '^#define ASYNC_PROTOCOL_MAJOR\t*(.*)' )
		for i in l:
			if ( p.match( i ) ):
				major = p.match( i ).group(1)
				break
	
		f = open( 'framework/async/AsyncNetwork.h' )
		l = f.readlines()
		f.close()

		minor = 'X'
		p = re.compile( '^const int ASYNC_PROTOCOL_MINOR\t*= (.*);' )
		for i in l:
			if ( p.match( i ) ):
				minor = p.match( i ).group(1)
				break	
	
		return '%s.%s' % ( major, minor )

	def ExtractEngineVersion( self ):
		f = open( 'framework/Licensee.h' )
		l = f.readlines()
		f.close()

		version = 'X'
		p = re.compile( '^#define.*ENGINE_VERSION\t*"DOOM (.*)"' )
		for i in l:
			if ( p.match( i ) ):
				version = p.match( i ).group(1)
				break
	
		return version

	def ExtractBuildVersion( self ):
		f = open( 'framework/BuildVersion.h' )
		l = f.readlines()[ 4 ]
		f.close()
		pat = re.compile( '.* = (.*);\n' )
		return pat.split( l )[ 1 ]

def checkLDD( target, source, env ):
	file = target[0]
	if (not os.path.isfile(file.abspath)):
		print('ERROR: CheckLDD: target %s not found\n' % target[0])
		Exit(1)
	( status, output ) = commands.getstatusoutput( 'ldd -r %s' % file )
	if ( status != 0 ):
		print 'ERROR: ldd command returned with exit code %d' % ldd_ret
		os.system( 'rm %s' % target[ 0 ] )
		sys.exit(1)
	lines = string.split( output, '\n' )
	have_undef = 0
	for i_line in lines:
		#print repr(i_line)
		regex = re.compile('undefined symbol: (.*)\t\\((.*)\\)')
		if ( regex.match(i_line) ):
			symbol = regex.sub('\\1', i_line)
			try:
				env['ALLOWED_SYMBOLS'].index(symbol)
			except:
				have_undef = 1
	if ( have_undef ):
		print output
		print "ERROR: undefined symbols"
		os.system('rm %s' % target[0])
		sys.exit(1)

def SharedLibrarySafe( env, target, source ):
	ret = env.SharedLibrary( target, source )
	env.AddPostAction( ret, checkLDD )
	return ret

def NotImplementedStub( *whatever ):
	print 'Not Implemented'
	sys.exit( 1 )

# --------------------------------------------------------------------

# get a clean error output when running multiple jobs
def SetupBufferedOutput( env, silent ):
	buf = idBuffering()
	buf.silent = silent
	buf.env = env
	env['SPAWN'] = buf.buffered_spawn

# setup utilities on an environement
def SetupUtils( env ):
	env.SharedLibrarySafe = SharedLibrarySafe
	if ( os.path.exists( 'sys/scons/SDK.py' ) ):
		import SDK
		sdk = SDK.idSDK()
		env.PreBuildSDK = sdk.PreBuildSDK
		env.BuildSDK = sdk.BuildSDK
	else:
		env.PreBuildSDK = NotImplementedStub
		env.BuildSDK = NotImplementedStub
	if ( os.path.exists( 'sys/scons/Setup.py' ) ):
		import Setup
		setup = Setup.idSetup()
		env.setup = setup
	else:
		env.BuildSetup = NotImplementedStub
		env.BuildGamePak = NotImplementedStub

	if ( os.path.exists( 'sys/scons/OSX.py' ) ):
		import OSX
		OSX = OSX.idOSX()
		env.BuildBundle = OSX.BuildBundle
	else:
		env.BuildBundle = NotImplementedStub

def BuildList( s_prefix, s_string ):
	s_list = string.split( s_string )
	for i in range( len( s_list ) ):
		s_list[ i ] = s_prefix + '/' + s_list[ i ]
	return s_list

def ExtractSource( file ):
	from xml.dom.minidom import parse
	dom = parse( file )
	files = dom.getElementsByTagName( 'File' )
	l = []
	for i in files:
		s = i.getAttribute( 'RelativePath' )
		s = s.encode('ascii', 'ignore')
		s = re.sub( '\\\\', '/', s )
		s = re.sub( '^\./', '', s )
		if ( string.lower( s[-4:] ) == '.cpp' or string.lower( s[-2:] ) == '.c' ):
			l.append( s )
	return l

def SplitPattern( list, pat ):
	pat = re.compile( pat )
	yes = []
	nope = []
	for i in list:
		if ( pat.search( i ) ):
			yes.append( i )
		else:
			nope.append( i )
	return ( yes, nope )

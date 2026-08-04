/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#define DOCTEST_THREAD_LOCAL	//stgatilov: initialization of threadlocal g_oss crashes because it happens before CRT starts
#define DOCTEST_CONFIG_IMPLEMENT
#include "testing.h"
#include "TestRun.h"
#include "framework/CmdSystem.h"

class GameConsoleStreamBuf : public std::stringbuf {
	idList<char> buffer;
	int sync() override {
		common->Printf( "%s", str().c_str() );
		str( "" );
		return 0;
	}
};
class RedirectStdoutToGameConsole {
	GameConsoleStreamBuf gameBuf;
	std::streambuf *oldBuf;
public:
	RedirectStdoutToGameConsole() { oldBuf = std::cout.rdbuf( &gameBuf ); }
	~RedirectStdoutToGameConsole() { std::cout.rdbuf( oldBuf ); }
};

static void RunTests( const idCmdArgs &args ) {
	//most of additional keys in doctest look like this:
	//	-tc=testname
	//	-tce="some name with spaces, * and ::"
	//	--help
	//D3 parser splits them into separate tokens, here we merge them back
	//note: long parameter names like --test-case are NOT supported
	idCmdArgs fixedArgs;
	for ( int i = 0; i < args.Argc(); ) {
		if ( idStr::Cmp( args.Argv( i ), "-" ) == 0 || idStr::Cmp( args.Argv( i ), "--" ) == 0 ) {
			//run from game console:
			//  '-', 'tc', '=', 'test name'
			//  '--', 'help'
			int len = 2;
			if ( idStr::Cmp( args.Argv( i + len ), "=" ) == 0 )
				len += 2;
			idStr param;
			for ( int d = 0; d < len; d++ )
				param += args.Argv( i++ );
			fixedArgs.AppendArg( param.c_str() );
		}
		else if ( idStr::Cmpn( args.Argv( i ), "-", 1 ) == 0 || idStr::Cmpn( args.Argv( i ), "--", 2 ) == 0 ) {
			//run from OS command line:
			//  '-tc', '=', 'test name'
			//  '--help'
			int len = 1;
			if ( idStr::Cmp( args.Argv( i + len ), "=" ) == 0 )
				len += 2;
			idStr param;
			for ( int d = 0; d < len; d++ )
				param += args.Argv( i++ );
			fixedArgs.AppendArg( param.c_str() );
		}
		else {
			fixedArgs.AppendArg( args.Argv( i++ ) );
		}
	}

	doctest::Context context;

	//pass command line parameters
	int argc = 0;
	const char **argv = fixedArgs.GetArgs( &argc );
	context.applyCommandLine( argc, argv );

	//set output stream to write text to
	RedirectStdoutToGameConsole redirect;

	//run tests
	int res = context.run();
	std::cout << std::endl;
}

void ArgCompletion_DoctestArg( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	idStr start = args.Args();
	char buffer[1024];
	auto allTests = doctest::getRegisteredTests();

	if ( start.IcmpPrefix( "- tc =" ) == 0 ) {
		for ( const auto &test : allTests ) {
			idStr::snPrintf( buffer, sizeof(buffer), "runTests -tc=\"%s\"", test.m_name );
			callback( buffer );
		}
	} else if ( start.IcmpPrefix( "- tce =" ) == 0 ) {
		for ( const auto &test : allTests ) {
			idStr::snPrintf( buffer, sizeof(buffer), "runTests -tce=\"%s\"", test.m_name );
			callback( buffer );
		}
	} else {
		callback( "runTests --help" );
		callback( "runTests -tc=\"" );
		callback( "runTests -tce=\"" );
	}
}

void TestsInit() {
	cmdSystem->AddCommand( "runTests", RunTests, CMD_FL_SYSTEM, "runs unit tests (powered by doctest)", ArgCompletion_DoctestArg );

	//doctest tries to popularize verbose test names with special characters in them
	//but it causes issues with command line parsers, and breaks even its own features like -tc=name:
	//  https://github.com/onqtam/doctest/issues/297
	//to keep away of confusion, let's forbid bad characters in test names
	auto allTests = doctest::getRegisteredTests();
	for ( const auto &test : allTests ) {
		if (
			idStr::CountChar( test.m_name, ',' ) ||	//breaks -tc="name"
		0) {
			throw idException( va( "Detected test with wrong name: %s", test.m_name ) );
		}
	}
}

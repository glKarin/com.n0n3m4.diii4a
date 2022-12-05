#ifndef __LEXERFACTORY_H__
#define __LEXERFACTORY_H__

class LexerFactory
{
public:

	static Lexer *MakeLexer( int flags );
	static Lexer *MakeLexer( char const * const filename, int flags = 0, bool OSPath = false );
	static Lexer *MakeLexer( char const * const ptr, int length, char const * const name, int flags = 0 );

private:
	static int GetReadBinary();
	static int GetWriteBinary();

	// disallow default constructor
	LexerFactory();
	~LexerFactory();
};

#endif

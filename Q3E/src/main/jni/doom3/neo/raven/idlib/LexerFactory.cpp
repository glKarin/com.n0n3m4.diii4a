
#include "../../idlib/precompiled.h"
#pragma hdrstop

LexerFactory::~LexerFactory()
{
}

Lexer *LexerFactory::MakeLexer(char const * const filename, int flags, bool OSPath)
{
	return new Lexer(filename, flags | GetReadBinary() | GetWriteBinary(), OSPath);
}

Lexer *LexerFactory::MakeLexer(int flags)
{
	return new Lexer(flags | GetReadBinary() | GetWriteBinary());
}

Lexer *LexerFactory::MakeLexer( char const * const ptr, int length, char const * const name, int flags)
{
	return new Lexer(ptr, length, name, flags | GetWriteBinary() | GetReadBinary());
}

int LexerFactory::GetReadBinary() 
{ 
	if(cvarSystem->GetCVarBool("com_binaryRead")) 
	{
		return LEXFL_READBINARY; 
	}
	else 
	{
		return 0; 
	}
}

int LexerFactory::GetWriteBinary() 
{ 
	int ret=0;
	int writeBinary = cvarSystem->GetCVarInteger("com_binaryWrite");
	switch(writeBinary)
	{
	case 0:
		break;
	case 1:
		ret = LEXFL_WRITEBINARY;
		break;
	case 2:
		ret = LEXFL_WRITEBINARY | LEXFL_BYTESWAP;
		break;
	}

	return ret;
}

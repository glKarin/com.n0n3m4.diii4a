#include "PrecompCommon.h"
#include "gmParseSourceFile.h"
#include "obParser.h"

struct ArgMap
{
	const char *	argName;
	ArgumentType	argType;
};

static const ArgMap ArgMapping[] =
{
	{ "__argVoid", ArgVoid },
	{ "__argInt", ArgInt },
	{ "__argFloat", ArgFloat },
	{ "__argNum", ArgNumber },
	{ "__argString", ArgString },
	{ "__argFunc", ArgFunction },
	{ "__argEntity", ArgEntity },

	{ "const char*", ArgString },
	{ "const std::string&", ArgString },
	{ "const String&", ArgString },
};

static bool ToArgType(const String &type, ArgumentType &argtype)
{
	for(int i = 0; i < sizeof(ArgMapping)/sizeof(ArgMapping[0]); ++i)
	{
		if(ArgMapping[i].argName == type)
		{
			argtype = ArgMapping[i].argType;
			return true;
		}
	}
	return false;
}

Function::Function()
{
	ReturnType = ArgVoid;
	NumArguments = 0;
	for(int i = 0; i < MaxArguments; ++i)
	{
		Argument[i] = ArgVoid;
	}
}

bool FileParser::ParseFunction(const String &str, Function &fn)
{
	obParser parser(str);
	if(!parser.ExpectNextToken("__scriptfunc"))
		return false;

	obToken retType = parser.GetNextToken("(");
	if(!retType)
		return false;
	{
		// parse the return type from the type
		obParser retParser(retType.GetString());
		obToken ret = retParser.GetLastToken(": *&");
		if(!ret)
		{
			Printf("%s: expected return token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, retParser.GetInputString().c_str());
			return false;
		}
		fn.Name = ret.GetString();

		retParser.MinimizeWhiteSpace();
		if(retParser.ExpectLastChar(":") && retParser.ExpectLastChar(":"))
		{
			fn.ClassName = retParser.GetLastToken(" \t*&").GetString();
		}

		if(!ToArgType(retParser.GetInputString(),fn.ReturnType))
		{
			Printf("%s: invalid return arg type '%s' at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), retType.GetString().c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}
	}

	/*obToken funcName = parser.GetNextToken("(");
	if(!funcName)
		return false;
	fn.Name = funcName.GetString();	*/
	
	if(!parser.ExpectNextChar("("))
	{
		Printf("%s: expected '(' token at line %d\n\t\t%s\n", 
			CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
		return false;
	}

	// parse arguments
	fn.NumArguments = 0;
	while(true)
	{
		obToken arg = parser.GetNextToken(",)");
		if(!arg)
		{
			Printf("%s: expected arg at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}

		// parse the arg name from the type
		obParser argParser(arg.GetString());
		obToken argName = argParser.GetLastToken(" \t*&");
		if(!argName)
		{
			Printf("%s: expected arg name token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, argParser.GetInputString().c_str());
			return false;
		}

		argParser.MinimizeWhiteSpace();
		obToken argType = obToken(argParser.GetInputString());
		if(!argType)
		{
			Printf("%s: expected arg type token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, argParser.GetInputString().c_str());
			return false;
		}

		if(!ToArgType(argType.GetString(),fn.Argument[fn.NumArguments]))
		{
			Printf("%s: invalid arg type '%s' at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), argType.GetString().c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}

		/*obToken argType = parser.GetNextToken(" ");
		if(!argType)
		{
			Printf("%s: expected arg type token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}

		if(!ToArgType(argType.GetString(),fn.Argument[fn.NumArguments]))
		{
			Printf("%s: invalid arg type '%s' at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), argType.GetString().c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}

		obToken argName = parser.GetNextToken(",)");
		if(!argName)
		{
			Printf("%s: expected arg name token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}*/

		fn.ArgumentName[fn.NumArguments] = argName.GetString();
		fn.NumArguments++;

		// check for end of arguments.
		if(parser.ExpectNextChar(")"))
			return true;

		if(parser.Finished())
		{
			Printf("%s: expected ')' token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}

		if(!parser.ExpectNextChar(","))
		{
			Printf("%s: expected ',' token at line %d\n\t\t%s\n", 
				CurrentFile.c_str(), CurrentLine, parser.GetInputString().c_str());
			return false;
		}
	}
	return true;
}

void FileParser::ParseLine(String line)
{
	Function fn;
	if(ParseFunction(line,fn))
	{
		Functions.push_back(fn);
	}
}

void FileParser::ParseFile()
{
	File f;
	if(f.OpenForRead(CurrentFile.c_str(),File::Text))
	{
		String line;
		while(f.ReadLine(line))
		{
			ParseLine(line);
			CurrentLine++;
		}
	}
}

void FileParser::Printf(const char* _msg, ...)
{
	static char buffer[8192] = {0};
	va_list list;
	va_start(list, _msg);
#ifdef WIN32
	_vsnprintf(buffer, 8192, _msg, list);	
#else
	vsnprintf(buffer, 8192, _msg, list);
#endif
	va_end(list);

	Info.push_back(buffer);
}

FileParser::FileParser(const String &filename) : CurrentFile(filename), CurrentLine(0)
{
}

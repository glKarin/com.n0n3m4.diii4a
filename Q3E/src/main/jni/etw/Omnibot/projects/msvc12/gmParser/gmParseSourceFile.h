#ifndef _GMPARSESOURCEFILE_H_
#define _GMPARSESOURCEFILE_H_

enum ArgumentType
{
	ArgVoid,
	ArgInt,
	ArgFloat,
	ArgNumber,
	ArgString,
	ArgFunction,
	ArgEntity,

	ArgTypeNum
};

struct Range
{
	union
	{
		float rangeF[2];
		float rangeI[2];
	};
};

struct Default
{
	union
	{
		float		defaultF;
		int			defaultI;
		const char *defaultS;
	};
};

struct Function
{
	enum { MaxArguments=32 };
	String			Name;
	String			ClassName;
	ArgumentType	ReturnType;
	ArgumentType	Argument[MaxArguments];
	String			ArgumentName[MaxArguments];
	Range			ArgumentRange[MaxArguments];
	Default			ArgumentDefaults[MaxArguments];
	int				NumArguments;
	Function();
};

typedef std::vector<Function> FunctionList;

struct FileParser
{
	void ParseFile();
	void ParseLine(String line);

	bool ParseFunction(const String &str, Function &fn);

	void Printf(const char* _msg, ...);

	FunctionList Functions;

	StringVector Info;

	FileParser(const String &filename);
private:
	String	CurrentFile;
	int		CurrentLine;

	
};



#endif

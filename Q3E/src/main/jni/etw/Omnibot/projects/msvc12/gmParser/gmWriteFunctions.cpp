#include "PrecompCommon.h"
#include "gmParseSourceFile.h"
#include "gmWriteFunctions.h"

struct NameSpaces
{
	String			ShortName;
	String			UserVar;
	String			FileName;
	FunctionList	Functions;

	NameSpaces(const String &str) : ShortName(str), FileName(String("Script"+str)) {}
};

//typedef std::set<String> StringSet;
typedef std::map<String,NameSpaces> OutFileMap;

static void WriteUserTypeVars(File &file, NameSpaces &ns)
{
	ns.UserVar = "GM_" + ns.ShortName;
	std::transform(ns.UserVar.begin(), ns.UserVar.end(), ns.UserVar.begin(), std::toupper);

	file.Printf("static gmType %s = GM_NULL;\n",ns.UserVar.c_str());
	file.Printf("\n\n");
}

static void WriteFunctionImplementations(File &file, const NameSpaces &ns)
{
	for(FunctionList::const_iterator fl = ns.Functions.begin();
		fl != ns.Functions.end();
		++fl)
	{
		const Function &fn = *fl;
		
		file.Printf("static int GM_CDECL gmf%s%s(gmThread *a_thread)\n",
			fn.ClassName.c_str(),fn.Name.c_str());
		file.Printf("{\n");
		file.Printf("\tGM_CHECK_NUM_PARAMS(%d);\n",fn.NumArguments);

		// handle the 'this' if necessary
		if(!fn.ClassName.empty())
		{
			file.Printf("\t%s *thisVar = static_cast<%s *>(a_thread->ThisUserCheckType(%s));\n", 
				fn.ClassName.c_str(),
				fn.ClassName.c_str(),
				ns.UserVar.c_str());
		}

		// write argument handling
		for(int i = 0; i < fn.NumArguments; ++i)
		{
			switch(fn.Argument[i])
			{
			case ArgInt:
				file.Printf("\tGM_CHECK_INT_PARAM(arg%d, %d);\n",i,i);
				break;
			case ArgFloat:
				file.Printf("\tGM_CHECK_FLOAT_PARAM(arg%d, %d);\n",i,i);
				break;
			case ArgNumber:
				file.Printf("\tGM_CHECK_FLOAT_OR_INT_PARAM(arg%d, %d);\n",i,i);
				break;
			case ArgString:
				file.Printf("\tGM_CHECK_STRING_PARAM(arg%d, %d);\n",i,i);
				break;
			case ArgFunction:
				file.Printf("\tGM_CHECK_FUNCTION_PARAM(arg%d, %d);\n",i,i);
				break;
			case ArgEntity:
				file.Printf("\tGM_CHECK_ENTITY_PARAM(arg%d, %d);\n",i,i);
				break;
			default:
			case ArgVoid:
				file.Printf("\t#error Unsupported argument type\n");
				break;
			}
		}

		switch(fn.ReturnType)
		{
		case ArgInt:
			file.Printf("\tconst int ret = ");
			break;
		case ArgFloat:
			file.Printf("\tconst float ret = ");
			break;
		case ArgNumber:
			file.Printf("\tconst float ret = ");
			break;
		case ArgString:
			file.Printf("\tconst String ret = ");
			break;
		case ArgFunction:
			file.Printf("\tconst gmFunctionObject* ret = ");
			break;
		case ArgEntity:
			file.Printf("\tconst int ret = ");
			break;
		default:
			file.Printf("\t");
		}

		// write out the function call
		file.Printf("%s(",fn.Name.c_str());
		for(int i = 0; i < fn.NumArguments; ++i)
		{
			file.Printf("%sarg%d",i>0?",":"",i);
		}
		file.Printf(");\n");

		// push the return value
		switch(fn.ReturnType)
		{
		case ArgInt:
			file.Printf("\ta_thread->PushInt(ret);\n");
			break;
		case ArgFloat:
			file.Printf("\ta_thread->PushFloat(ret);\n");
			break;
		case ArgNumber:
			file.Printf("\ta_thread->PushFloat(ret);\n");
			break;
		case ArgString:
			file.Printf("\ta_thread->PushNewString(ret);\n");
			break;
		case ArgFunction:
			file.Printf("\ta_thread->PushFunction(ret);\n");
			break;
		case ArgEntity:
			file.Printf("\ta_thread->PushEntity(ret);\n");
			break;
		default:
			break;
		}

		file.Printf("\treturn GM_OK;\n");
		file.Printf("}\n");
	}
}

static void WriteFunctionBindBlock(File &file, const NameSpaces &ns)
{
	const String libVarName = Utils::VA("s_%sLib",ns.FileName.c_str());

	file.Printf("\n\n\n");
	file.Printf("static gmFunctionEntry %s[] = \n",libVarName.c_str());
	file.Printf("{\n");

	for(FunctionList::const_iterator fl = ns.Functions.begin();
		fl != ns.Functions.end();
		++fl)
	{
		file.Printf("\t{\"%s\",\t\t\t\tgmf%s},\n",fl->Name.c_str(),fl->Name.c_str());
	}

	file.Printf("};\n\n");

	file.Printf("void gmBind%sLib(gmMachine * a_machine)\n",ns.FileName.c_str());
	file.Printf("{\n");
	if(ns.ShortName != "Globals")
	{
		String upperName = ns.ShortName;
		std::transform(upperName.begin(), upperName.end(), upperName.begin(), std::toupper);
		file.Printf("\tGM_%s = a_machine->CreateUserType(\"%s\");\n",upperName.c_str(),upperName.c_str());
		file.Printf("\ta_machine->RegisterTypeLibrary(GM_%s,%s, sizeof(%s) / sizeof(%s[0]));\n",
			upperName.c_str(),libVarName.c_str(),libVarName.c_str(),libVarName.c_str());
	}
	else
	{
		file.Printf("\ta_machine->RegisterLibrary(%s, sizeof(%s) / sizeof(%s[0]));\n",
			libVarName.c_str(),libVarName.c_str(),libVarName.c_str());
	}
	file.Printf("}\n");
}

int WriteFunctions(const FunctionList &functions)
{
	// separate the master function list into namespaces
	OutFileMap fileMap;
	fileMap.insert(std::make_pair("","Globals"));

	for(obuint32 i = 0; i < functions.size(); ++i)
	{
		OutFileMap::iterator it = fileMap.find(functions[i].ClassName);
		if(it == fileMap.end())
		{
			OutFileMap::_Pairib ins = fileMap.insert(std::make_pair(functions[i].ClassName,NameSpaces(functions[i].ClassName)));
			it = ins.first;
		}
		it->second.Functions.push_back(functions[i]);
	}

	// write out each file.
	for(OutFileMap::iterator it = fileMap.begin();
		it != fileMap.end();
		++it)
	{
		File outFile;

		// write out each function
		const String FileName = Utils::VA("%s.cpp", it->second.FileName.c_str());
		if(outFile.OpenForWrite(FileName.c_str(),File::Text,false))
		{
			outFile.Printf("#include \"gmThread.h\"\n\n");

			WriteUserTypeVars(outFile,it->second);

			// write the function implementations
			WriteFunctionImplementations(outFile,it->second);

			// write the binding block
			WriteFunctionBindBlock(outFile,it->second);
		}
	}

	// write table of contents file, where all bindings are registered.
	File tocFile;
	if(tocFile.OpenForWrite("ScriptBindAll.cpp",File::Text))
	{
		tocFile.Printf("#include \"gmThread.h\"\n\n");

		tocFile.Printf("// declarations\n");
		for(OutFileMap::iterator it = fileMap.begin();
			it != fileMap.end();
			++it)
		{
			if(!it->second.Functions.empty())
			{
				tocFile.Printf("void gmBind%sLib(gmMachine * a_machine);\n",it->second.FileName.c_str());
			}
		}

		tocFile.Printf("\n\n// bind libraries\n");
		tocFile.Printf("void gmBindAllAutogenLibs(gmMachine * a_machine)\n");
		tocFile.Printf("{\n");

		for(OutFileMap::iterator it = fileMap.begin();
			it != fileMap.end();
			++it)
		{
			if(!it->second.Functions.empty())
			{
				tocFile.Printf("\tgmBind%sLib(a_machine);\n",it->second.FileName.c_str());
			}
		}
		tocFile.Printf("}\n");
	}

	return fileMap.size();
}

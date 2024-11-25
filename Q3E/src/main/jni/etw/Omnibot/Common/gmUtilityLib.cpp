#include "PrecompCommon.h"

#include "gmUtilityLib.h"
#include "ScriptManager.h"

#undef GetObject

#define GM_PATHSTRING		  "_GM_PATH"
#define GM_REQUIRETABLE		  "_GM_REQUIRE"

//////////////////////////////////////////////////////////////////////////

// script: UtilityLibrary
//		Binds various useful utility functions to the scripting system.
namespace gmUtility
{
	struct TableInfo_t
	{
		String			name;
		gmTableNode *	node;
	};
	typedef std::vector<TableInfo_t> TableInfoList;
	bool _TableNodeAlphabetical(const TableInfo_t &_n1, const TableInfo_t &_n2)
	{
		return _n1.name < _n2.name;
	}
	//////////////////////////////////////////////////////////////////////////

	void DumpTableInfo(gmMachine *_machine, const int _flags, gmTableObject *_table, char *_buffer, int _buflen, int _lvl, File &_file)
	{
		TableInfoList nodeList;
		nodeList.reserve(_table->Count());
		
		// copy the nodes to a list, so we can sort them by default
		{
			gmTableIterator tIt;
			gmTableNode *pNode = _table->GetFirst(tIt);
			while(pNode)
			{
				char tmpName[256] = {};
				const char * str = pNode->m_key.AsString(_machine,tmpName,256);
				OBASSERT(str && *str,"Error getting key string of gm var");
				if(str != NULL)
				{
					TableInfo_t ti;
					ti.node = pNode;
					ti.name = str;
					nodeList.push_back(ti);
				}
				pNode = _table->GetNext(tIt);
			}
		}

		std::sort(nodeList.begin(), nodeList.end(), _TableNodeAlphabetical);

		/*gmTableIterator tIt;
		gmTableNode *pNode = _table->GetFirst(tIt);
		while(pNode)*/
		for(TableInfoList::iterator it = nodeList.begin();
			it != nodeList.end();
			++it)
		{
			gmTableNode * pNode = (*it).node;

			// Indent
			for(int i = 0; i < _lvl; ++i)
				_file.WriteString("\t");

			//////////////////////////////////////////////////////////////////////////
			switch(pNode->m_key.m_type)
			{
			case GM_INT:
				{
					_file.WriteString("[");
					_file.WriteInt32(pNode->m_key.GetInt(),false);
					_file.WriteString("]");
					_file.WriteString(" = ");
					break;
				}
			case GM_FUNCTION:
				{
					gmFunctionObject *func = pNode->m_key.GetFunctionObjectSafe();
					_file.WriteString("// ");
					_file.WriteString(func->GetDebugName());
					_file.WriteString(" = ");
					break;
				}
			default:
				{
					_file.WriteString(pNode->m_key.AsString(_machine, _buffer, _buflen));
					_file.WriteString(" = ");
					break;
				}
			}

			//////////////////////////////////////////////////////////////////////////
			switch(pNode->m_value.m_type)
			{
			case GM_NULL:
				{
					break;
				}
			case GM_INT:
			case GM_FLOAT:
				{
					_file.WriteString(pNode->m_value.AsString(_machine, _buffer, _buflen));
					_file.WriteString(_lvl != 0 ? "," : ";");
					_file.WriteNewLine();
					break;
				}
			case GM_VEC3:
				{
					_file.WriteString("Vec3");
					_file.WriteString(pNode->m_value.AsString(_machine, _buffer, _buflen));
					_file.WriteString(_lvl != 0 ? "," : ";");
					_file.WriteNewLine();
					break;
				}
			case GM_STRING:
				{
					_file.WriteString("\"");
					_file.WriteString(pNode->m_value.AsString(_machine, _buffer, _buflen));
					_file.WriteString("\"");
					_file.WriteString(_lvl != 0 ? "," : ";");
					_file.WriteNewLine();
					break;
				}
			case GM_TABLE:
				{
					if(_flags & gmUtility::DUMP_RECURSE)
					{
						_file.WriteNewLine();

						for(int i = 0; i < _lvl; ++i)
							_file.WriteString("\t");

						_file.WriteString("{");
						_file.WriteNewLine();

						gmTableObject *pTable = pNode->m_value.GetTableObjectSafe();
						DumpTableInfo(_machine, _flags, pTable, _buffer, _buflen, _lvl+1, _file);

						for(int i = 0; i < _lvl; ++i)
							_file.WriteString("\t");

						_file.WriteString("}");
						_file.WriteString(_lvl != 0 ? "," : ";");
						_file.WriteNewLine();
					}				
					break;
				}
			case GM_FUNCTION:
				{
					if(_flags & gmUtility::DUMP_FUNCTIONS)
					{
						gmFunctionObject *fn = pNode->m_value.GetFunctionObjectSafe();
						if(fn)
						{
							_file.WriteString(" <function> ");
							
							// write function header
							//_file.WriteString(" = function(");
							//for(int p = 0; p < fn->GetNumParams(); ++p)
							//{
							//	if(p!=0)
							//	{
							//		_file.WriteString(",");
							//	}
							//	_file.WriteString(fn->GetSymbol(p));
							//}
							//_file.WriteString(")");
							//_file.WriteString("\n");
							//_file.WriteString("{");
							//_file.WriteString("\n");
							//// source code.
							////////////////////////////////////////////////////////////////////////////
							//const char *SourceCode = 0;
							//const char *SourceFile = 0;
							//_machine->GetSourceCode(fn->GetSourceId(),SourceCode,SourceFile);
							//if(SourceCode)
							//{
							//	//_file.WriteString(SourceCode);

							//	int FuncSourceLine = 0;

							//	int line;
							//	while((line = fn->GetFunctionSourceLine(FuncSourceLine++)) >= 0)
							//	{
							//		enum { BufferSize = 1024 };
							//		char buffer[BufferSize] = {};
							//		gmGetLineFromString(SourceCode, line, buffer, BufferSize);
							//		_file.WriteString(buffer);
							//		_file.WriteString("\n");
							//	}
							//}
							//else if(SourceFile)
							//{
							//	_file.WriteString(" <file> ");
							//	_file.WriteString(SourceFile);
							//}
							////////////////////////////////////////////////////////////////////////////
							//_file.WriteString("\n");
							//_file.WriteString("}");
							//_file.WriteString("\n");
							
							_file.WriteNewLine();
						}
					}
					break;
				}
			default:
				{
					// All other user types.
					if(_flags & gmUtility::DUMP_REFERENCES)
					{
						_file.WriteString("// ");
						_file.WriteString(pNode->m_key.AsString(_machine, _buffer, _buflen));
						_file.WriteString(" : ");
						_file.WriteString(pNode->m_value.AsString(_machine, _buffer, _buflen));
						_file.WriteString(" <user> ");
						_file.WriteNewLine();
					}
					break;
				}
			}

			//pNode = _table->GetNext(tIt);
		}
	}

	bool DumpGlobals(gmMachine *_machine, const String &_file, int _flags)
	{
		char strBuffer[1024] = {};
		sprintf(strBuffer, "user/%s", _file.c_str());

		File outFile;
		outFile.OpenForWrite(strBuffer, File::Text);
		if(outFile.IsOpen())
		{
			const int BUF_SIZE = 512;
			char buffer[BUF_SIZE] = {0};

			gmTableObject *pGlobalTable = _machine->GetGlobals();

			DumpTableInfo(_machine, _flags, pGlobalTable, buffer, BUF_SIZE, 0, outFile);

			if(_flags & gmUtility::DUMP_TYPEFUNCTIONS)
			{
				_flags |= gmUtility::DUMP_FUNCTIONS;

				gmTableObject *pTable = 0;

				gmType type = GM_USER;
				while((pTable = _machine->GetTypeTable(type)))
				{
					const char *pTypeName = _machine->GetTypeName(type);
					if(pTypeName)
					{
						outFile.WriteString("// Type: ");
						outFile.WriteString(pTypeName);
						outFile.WriteString(", Functions ");
						outFile.WriteNewLine();
						outFile.WriteString("// {");
						outFile.WriteNewLine();						
						DumpTableInfo(_machine, _flags, pTable, buffer, BUF_SIZE, 1, outFile);
						outFile.WriteString("// }");
						outFile.WriteNewLine();
					}			
					++type;
				}
			}
			return true;
		}
		return false;
	}

	bool DumpTable(gmMachine *_machine, const String &_file, const String &_name, int _flags)
	{
		char strBuffer[1024] = {};
		sprintf(strBuffer, "user/%s", _file.c_str());

		File outFile;
		outFile.OpenForWrite(strBuffer, File::Text);

		if(outFile.IsOpen())
		{
			const int BUF_SIZE = 512;
			char buffer[BUF_SIZE] = {0};

			gmVariable vTable = _machine->GetGlobals()->Get(_machine, _name.c_str());
			gmTableObject *pTable = vTable.GetTableObjectSafe();
			if(pTable)
			{
				outFile.WriteString("global ");
				outFile.WriteString(_name.c_str());
				outFile.WriteString(" = ");
				outFile.WriteNewLine();
				outFile.WriteString("{");
				outFile.WriteNewLine();
				DumpTableInfo(_machine, _flags, pTable, buffer, BUF_SIZE, 1, outFile);
				outFile.WriteString("};");
			}
			return true;
		}

		return false;
	}

	bool DumpTable(gmMachine *_machine, File &outFile, const String &_name, gmTableObject *_tbl, int _flags)
	{
		if(outFile.IsOpen())
		{
			const int BUF_SIZE = 512;
			char buffer[BUF_SIZE] = {0};

			if(_tbl)
			{
				outFile.WriteString("global ");
				outFile.WriteString(_name.c_str());
				outFile.WriteString(" = ");
				outFile.WriteNewLine();
				outFile.WriteString("{");
				outFile.WriteNewLine();
				DumpTableInfo(_machine, _flags, _tbl, buffer, BUF_SIZE, 1, outFile);
				outFile.WriteString("};");
			}
			return true;
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////////

	// function: dumpGlobals
	//		This function will recursively dump all the global variables, types & functions 
	//		to the game console and optionally a file.
	//
	// Parameters:
	//
	//		string - The filename to dump to
	//		int - The bitflags of things to dump. See the global DUMP table.
	//
	// Returns:
	//		None

	static int GM_CDECL gmfDumpGlobals(gmThread *a_thread)
	{
		if(GM_NUM_PARAMS > 2)
		{
			GM_EXCEPTION_MSG("expecting 1 - 2 parameters");
			return GM_EXCEPTION;
		}

		GM_CHECK_STRING_PARAM(filename, 0);	
		int iFlags = a_thread->ParamInt(1, gmUtility::DUMP_ALL);
		DumpGlobals(a_thread->GetMachine(), filename, iFlags);

		return GM_OK;
	}

	//////////////////////////////////////////////////////////////////////////

	// function: dumpTable
	//		This function will dump a specific table.
	//
	// Parameters:
	//
	//		string - The filename to dump to
	//		string - The table to dump
	//		int - The bitflags of things to dump. See the global DUMP table.
	//
	// Returns:
	//		None
	static int GM_CDECL gmfDumpTable(gmThread *a_thread)
	{
		if(GM_NUM_PARAMS > 3)
		{
			GM_EXCEPTION_MSG("expecting 2 - 3 parameters");
			return GM_EXCEPTION;
		}

		GM_CHECK_STRING_PARAM(filename, 0);
		GM_CHECK_STRING_PARAM(tablename, 1);
		int iFlags = a_thread->ParamInt(2, gmUtility::DUMP_ALL);
		DumpTable(a_thread->GetMachine(), filename, tablename, iFlags);

		return GM_OK;
	}

	//////////////////////////////////////////////////////////////////////////

	static int GM_CDECL gmfEchoTable(gmThread *a_thread)
	{
		GM_CHECK_NUM_PARAMS(1);
		GM_CHECK_STRING_PARAM(tablename, 0);

		const int BUF_SIZE = 512;
		char buffer[BUF_SIZE] = {0};
		char buffer2[BUF_SIZE] = {0};

		gmMachine *pMachine = a_thread->GetMachine();
		gmVariable vTable = pMachine->GetGlobals()->Get(pMachine, tablename);
		gmTableObject *pTable = vTable.GetTableObjectSafe();
		if(pTable)
		{
			gmTableIterator tIt;
			gmTableNode *pNode = pTable->GetFirst(tIt);
			while(pNode)
			{
				EngineFuncs::ConsoleMessage(
					va("%s = %s",
					pNode->m_key.AsString(pMachine, buffer, BUF_SIZE),
					pNode->m_value.AsString(pMachine, buffer2, BUF_SIZE)));
				pNode = pTable->GetNext(tIt);
			}
		}
		return GM_OK;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// package: Global Bot Library Functions
static gmFunctionEntry s_UtilityLib[] = 
{
	/*gm
	\function dumpGlobals
	\brief dump global variables based on bit masks
	\param string
	*/
	{"dumpGlobals", gmUtility::gmfDumpGlobals},
	/*gm
	\function dumpTable
	\brief dump the contents of a single table
	\param string
	\param string
	*/
	{"dumpTable", gmUtility::gmfDumpTable},
	/*gm
	\function echoTable
	\brief echo the contents of a single table
	\param string table name
	*/
	{"echoTable", gmUtility::gmfEchoTable},
	/*gm
	\function unitTest
	\brief runs a function in another thread for testing
	\param function
	*/
	//{"unitTest", gmUtility::gmfUnitTest},
	/*gm
	\function require
	\brief functions like the lua version
	\param string
	*/
	//{"require", gmUtility::gmRequire},
	/*gm
	\function abort
	\brief throws an exception for the current thread
	\param string error message
	*/
	//{"abort", gmUtility::gmfAbort},
};

//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmStringTokenize(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(delim, 0);

	DisableGCInScope gcEn(a_thread->GetMachine());

	const gmVariable * var = a_thread->GetThis();
	gmStringObject * strObj = var->GetStringObjectSafe();

	StringVector tokens;
	Utils::Tokenize(strObj->GetString(), delim, tokens);
	gmTableObject *pTbl = a_thread->GetMachine()->AllocTableObject();
	for(obuint32 i = 0; i < tokens.size(); ++i)
		pTbl->Set(a_thread->GetMachine(), i, gmVariable(a_thread->GetMachine()->AllocStringObject(tokens[i].c_str())));
	a_thread->PushTable(pTbl);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_stringLib[] =  
{
	{"Tokenize", gmStringTokenize},
};

//////////////////////////////////////////////////////////////////////////

void gmBindUtilityLib(gmMachine * a_machine)
{
	DisableGCInScope gcEn(a_machine);

	a_machine->GetGlobals()->Set(a_machine, GM_PATHSTRING, gmVariable(a_machine->AllocStringObject("?;?.gm")));
	a_machine->GetGlobals()->Set(a_machine, GM_REQUIRETABLE, gmVariable(a_machine->AllocTableObject()));

	// Register the bot functions.
	a_machine->RegisterLibrary(s_UtilityLib, sizeof(s_UtilityLib) / sizeof(s_UtilityLib[0]));	

	// extra string utilities
	a_machine->RegisterTypeLibrary(GM_STRING, s_stringLib, sizeof(s_stringLib) / sizeof(s_stringLib[0]));

	// Create the dump flag table.
	gmTableObject *pDumpTable = a_machine->AllocTableObject();
	pDumpTable->Set(a_machine, gmVariable(a_machine->AllocStringObject("RECURSE")), gmVariable(gmUtility::DUMP_RECURSE));
	pDumpTable->Set(a_machine, gmVariable(a_machine->AllocStringObject("FUNCTIONS")), gmVariable(gmUtility::DUMP_FUNCTIONS));
	pDumpTable->Set(a_machine, gmVariable(a_machine->AllocStringObject("REFERENCES")), gmVariable(gmUtility::DUMP_REFERENCES));
	pDumpTable->Set(a_machine, gmVariable(a_machine->AllocStringObject("TYPEFUNCTIONS")), gmVariable(gmUtility::DUMP_TYPEFUNCTIONS));
	pDumpTable->Set(a_machine, gmVariable(a_machine->AllocStringObject("ALL")), gmVariable(gmUtility::DUMP_ALL));
	a_machine->GetGlobals()->Set(a_machine, gmVariable(a_machine->AllocStringObject("DUMP")), gmVariable(pDumpTable));
}

//////////////////////////////////////////////////////////////////////////

#include "PrecompCommon.h"
#include "gmConfig.h"
#include "gmSystemLibApp.h"
#include "gmThread.h"
#include "gmMachine.h"
#include "gmHelpers.h"
#include "FileSystem.h"
#include "ScriptManager.h"

#include "physfs.h"

#undef GetObject

const int GM_SYSTEM_LIB_MAX_LINE = 1024;
gmType	GM_NEWLINE = GM_NULL;

// script: SystemLibrary
//		Binds various useful functionality to the scripting system.

// type: File
//		File IO binding for GM script.
GMBIND_INIT_TYPE( gmFile, "File" );

GMBIND_FUNCTION_MAP_BEGIN( gmFile )
	GMBIND_FUNCTION( "Open", gmfOpen )
	GMBIND_FUNCTION( "Close", gmfClose )
	GMBIND_FUNCTION( "IsOpen", gmfIsOpen )
	GMBIND_FUNCTION( "Seek", gmfSeek )
	GMBIND_FUNCTION( "Tell", gmfTell )
	GMBIND_FUNCTION( "EndOfFile", gmfEndOfFile )
	GMBIND_FUNCTION( "FileSize", gmfFileSize )
	GMBIND_FUNCTION( "Flush", gmfFlush )

	GMBIND_FUNCTION( "ReadInt", gmfReadInt32 )
	GMBIND_FUNCTION( "ReadShort", gmfReadInt16 )
	GMBIND_FUNCTION( "ReadByte", gmfReadInt8 )
	GMBIND_FUNCTION( "ReadFloat", gmfReadFloat )
	GMBIND_FUNCTION( "ReadString", gmfReadString )
	GMBIND_FUNCTION( "ReadLine", gmfReadLine )	

	GMBIND_FUNCTION( "Write", gmfWrite )	
GMBIND_FUNCTION_MAP_END()

GMBIND_PROPERTY_MAP_BEGIN( gmFile )
GMBIND_PROPERTY_MAP_END();

//////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

File *gmFile::Constructor(gmThread *a_thread)
{
	File *pNewFile = new File;	
	return pNewFile;
}

void gmFile::Destructor(File *_native)
{
	if(_native)
	{
		delete _native;
		_native = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

int gmfMount(gmThread *a_thread)
{
	// TODO:
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

int gmfUnMount(gmThread *a_thread)
{
	// TODO:
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FileExists
//		This function checks if a file exists. Note: File IO functions restricted to a scratch folder.
//
// Parameters:
//
//		<string> - Filename to look for.
//
// Returns:
//		<int> - True if file exists, False if not.
static int GM_CDECL gmfFileExists(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(filename, 0);

	bool bGood = false;
	try
	{
		fs::path filepath("user/");
		filepath /= fs::path(filename);
		bGood = FileSystem::FileExists(filepath.string().c_str());
	}
	catch ( const std::exception & ex )
	{
		GM_EXCEPTION_MSG("Filesystem Exception: %s",ex.what());
		return GM_EXCEPTION;
	}
	a_thread->PushInt(bGood ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FileDelete
//		This function deletes a file.
//
// Parameters:
//
//		<string> - Filename to delete.
//
// Returns:
//		<int> - True if success, False if not.
static int GM_CDECL gmfFileDelete(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(filename, 0);

	bool bGood = false;
	try
	{
		filePath file( "user/%s", filename );
		bGood = FileSystem::FileDelete(file);
	}
	catch ( const std::exception & ex )
	{
		GM_EXCEPTION_MSG("Filesystem Exception: %s",ex.what());
		return GM_EXCEPTION;
	}
	a_thread->PushInt(bGood ? 1 : 0);
	return GM_OK;
}

void ScriptEnumerateCallback(void *data, const char *origdir, const char *str)
{
	gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
	gmGCRoot<gmFunctionObject> pFn(static_cast<gmFunctionObject*>(data), pMachine);

	try
	{
		char fullname[1024] = {};
		sprintf(fullname, "%s/%s", origdir, str);
		const char *pDir = PHYSFS_getRealDir(fullname);
		if(pDir)
		{
			fs::path filepath(pDir);
			filepath /= origdir;
			filepath /= str;

			if(!fs::is_directory(filepath))
			{
				gmCall call;
				if(call.BeginFunction(pMachine, pFn))
				{
					call.AddParamString(pMachine->AllocStringObject(fullname));
					call.End();
				}
			}
		}
	}
	catch(const std::exception & ex)
	{
		LOGCRIT("Filesystem Exception: "<<ex.what());
		SOFTASSERTALWAYS(0, "Filesystem: ",ex.what());
	}
}

//////////////////////////////////////////////////////////////////////////

// function: FileEnumerate
//		This function performs a script callback for every file that is enumerated.
//
// Parameters:
//
//		<string> - Directory to enumerate.
//		<function> - Script function to call back.
//
// Returns:
//		<int> - True if success, False if not.
static int GM_CDECL gmfFileEnumerate(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(dir, 0);
	GM_CHECK_FUNCTION_PARAM(callback, 1);

	bool bGood = false;
	try
	{
		fs::path filepath("user/");
		filepath /= fs::path(dir);		
		PHYSFS_enumerateFilesCallback(filepath.string().c_str(), ScriptEnumerateCallback, callback);
	}
	catch ( const std::exception & ex )
	{
		ex;
		LOGCRIT("Filesystem Exception: "<<ex.what());
		return GM_EXCEPTION;
	}
	a_thread->PushInt(bGood ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FindFiles
//		Find the first file matching some search parameters.
//
// Parameters:
//
//		<string> - OPTIONAL - relative path to look for file in.
//		<string> - OPTIONAL - file extension to look for.
//
// Returns:
//		<string> - File path + name.
//static int GM_CDECL gmfFindFiles(gmThread * a_thread)
//{
//	GM_STRING_PARAM(fileextension, 0, "*.*");
//	GM_STRING_PARAM(relpath, 1, "");
//
//	try
//	{
//		fs::path filepath = Utils::GetScratchFolder();
//		filepath /= relpath;
//
//		DirectoryList lst;
//		FileSystem::FindAllFiles(filepath, lst, Utils::RegexMatch(fileextension), true);
//
//		gmMachine *pMachine = a_thread->GetMachine();
//		pMachine->EnableGC(false);
//
//		gmTableObject *pTable = pMachine->AllocTableObject();
//
//		for(obuint32 i = 0; i < lst.size(); ++i)
//		{
//			pTable->Set(pMachine, i, gmVariable(pMachine->AllocStringObject(lst[i].string().c_str())));
//		}
//		a_thread->PushTable(pTable);
//		pMachine->EnableGC(true);
//		return GM_OK;
//	}
//	catch ( const std::exception & ex )
//	{
//		GM_EXCEPTION_MSG((Format("Filesystem Exception: %1%") % ex.what()).str().c_str());
//		return GM_EXCEPTION;
//	}
//}

//////////////////////////////////////////////////////////////////////////

// function: Open
//		Open a file for reading/writing/binary access.
//
// Parameters:
//
//		<string> - Filename to open.
//		<int> - OPTIONAL - readonly mode, default true
//		<int> - OPTIONAL - append mode, default false, only valid for write mode
//
// Returns:
//		<int> - True if file opened, False if not.
int gmFile::gmfOpen(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(filename, 0);
	GM_CHECK_STRING_PARAM(filemode, 1);
	GM_INT_PARAM(readonly, 2, 1);
	GM_INT_PARAM(append, 3, 0);

	File::FileMode m = File::Binary;
	if(!Utils::StringCompareNoCase(filemode, "text"))
		m = File::Text;
	else if(!Utils::StringCompareNoCase(filemode, "binary"))
		m = File::Binary;
	else
	{
		GM_EXCEPTION_MSG("Invalid File Mode");
		return GM_EXCEPTION;
	}

	if(!filename || !filename[0])
	{
		GM_EXCEPTION_MSG("Invalid File Name");
		return GM_EXCEPTION;
	}

	bool bSuccess = false;
	File *pNative = gmFile::GetThisObject( a_thread );

	char strBuffer[1024] = {};
	sprintf(strBuffer, "user/%s", filename);

	// Close the previous file if there is one.
	if(pNative->IsOpen())
		pNative->Close();

	if(readonly)
		bSuccess = pNative->OpenForRead(strBuffer, m);
	else
		bSuccess = pNative->OpenForWrite(strBuffer, m, append!=0);

	GM_THREAD_ARG->PushInt(bSuccess ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Close
//		Close a file.
//
// Parameters:
//
//		none
//
// Returns:
//		none
int gmFile::gmfClose(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	pNative->Close();
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: IsOpen
//		Checks if a file is currently open or not.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - True if file opened, False if not.
int gmFile::gmfIsOpen(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	GM_THREAD_ARG->PushInt(pNative->IsOpen() ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Seek
//		Checks if a file is currently open or not.
//
// Parameters:
//
//		<int> - Offset to seek to.
//
// Returns:
//		<int> - True if success, false if not.
int gmFile::gmfSeek(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(offset, 0);
	File *pNative = gmFile::GetThisObject( a_thread );
	pNative->Seek(offset);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Tell
//		Gets the current read offset into the file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - Location in the file.
int gmFile::gmfTell(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	a_thread->PushInt((obuint32)pNative->Tell());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: EndOfFile
//		Checks if the read position is at the end of the file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - true if end of file, false if not.
int gmFile::gmfEndOfFile(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	a_thread->PushInt(pNative->EndOfFile() ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: FileSize
//		Gets the size of the file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - size of the file, in bytes.
int gmFile::gmfFileSize(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	a_thread->PushInt((int)pNative->FileLength());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: Flush
//		Flushes the file buffer.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - true if successful false if not.
int gmFile::gmfFlush(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	a_thread->PushInt(pNative->Flush() ? 1 : 0);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: GetLastError
//		Gets a string describing the last error.
//
// Parameters:
//
//		none
//
// Returns:
//		<string> - Last error string.
int gmFile::gmfGetLastError(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );
	a_thread->PushNewString(pNative->GetLastError().c_str());
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadInt
//		Reads a 4 byte integer value from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - Integer value read from file.
int gmFile::gmfReadInt32(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	obuint32 out;
	if(pNative->ReadInt32(out))
		a_thread->PushInt(out);	
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadShort
//		Reads a 2 byte integer value from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - Integer value read from file.
int gmFile::gmfReadInt16(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	obuint16 out;
	if(pNative->ReadInt16(out))
		a_thread->PushInt(out);	
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadByte
//		Reads a 1 byte integer value from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<int> - Integer value read from file.
int gmFile::gmfReadInt8(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	obuint8 out;
	if(pNative->ReadInt8(out))
		a_thread->PushInt(out);	
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadFloat
//		Reads a floating point value from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<float> - Float value read from file.
int gmFile::gmfReadFloat(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	float out;
	if(pNative->ReadFloat(out))
		a_thread->PushFloat(out);	
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadString
//		Reads a string value from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<string> - String read from file.
int gmFile::gmfReadString(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	String out;
	if(pNative->ReadString(out))
		a_thread->PushNewString(out.c_str());	
	else
		a_thread->PushNull();

	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////

// function: ReadLine
//		Reads a line from a file.
//
// Parameters:
//
//		none
//
// Returns:
//		<string> - Line read from file.
int gmFile::gmfReadLine(gmThread *a_thread)
{
	File *pNative = gmFile::GetThisObject( a_thread );

	String out;
	if(pNative->ReadLine(out))
		a_thread->PushNewString(out.c_str());	
	else
		a_thread->PushNull();

	return GM_OK;
}
//////////////////////////////////////////////////////////////////////////

// function: Write
//		Write a value to a file.
//
// Parameters:
//
//		<ANY> - Variable to read from the file.
//
// Returns:
//		<int> - true if successful, false if not.
int gmFile::gmfWrite(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	File *pNative = gmFile::GetThisObject( a_thread );
	
	int nParams = a_thread->GetNumParams();
	for(obint32 i = 0; i < nParams; ++i)
	{
		gmVariable v = a_thread->Param(i);
		switch(v.m_type)
		{
		case GM_INT:
			{
				pNative->WriteInt32(v.m_value.m_int);
				break;
			}
		case GM_FLOAT:
			{
				pNative->WriteFloat(v.m_value.m_float);
				break;
			}
		case GM_STRING:
			{
				gmStringObject *pStr = v.GetStringObjectSafe();
				pNative->WriteString(pStr->GetString());
				break;
			}
		default:
			if(v.m_type == GM_NEWLINE && pNative->GetFileMode() == File::Text)
			{
				pNative->WriteNewLine();
				break;
			}
			GM_EXCEPTION_MSG("Expected int, float, or string");
			return GM_EXCEPTION;
		}
	}
	a_thread->PushInt(1);	
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static int GM_CDECL gmfSystemTime(gmThread * a_thread)
{
	GM_STRING_PARAM(format, 0, "%A %d %B %Y, %I:%M:%S %p");
	char buffer[256];
	time_t lt;
	time(&lt);
	struct tm * ct = localtime(&lt);
	strftime(buffer, 256, format, ct);
	a_thread->PushNewString(buffer);
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static gmFunctionEntry s_systemLib[] = 
{
	{"FileExists", gmfFileExists},
	{"FileDelete", gmfFileDelete},
	{"FileEnumerate", gmfFileEnumerate},

	{"Time", gmfSystemTime},

	//{"Mount", gmfMount},
	//{"UnMount", gmfUnMount},
};

void gmBindSystemLib(gmMachine * a_machine)
{
	// system
	GM_NEWLINE = a_machine->CreateUserType("NewLine");
	a_machine->RegisterLibrary(s_systemLib, sizeof(s_systemLib) / sizeof(s_systemLib[0]), "System");

	//
	a_machine->Lookup("System").GetTableObjectSafe()->Set(
		a_machine,"NewLine",
		gmVariable( a_machine->AllocUserObject(0,GM_NEWLINE)));
	gmFile::Initialise(a_machine, false);
}


#include "PrecompCommon.h"
#include "gmParseSourceFile.h"
#include "gmWriteFunctions.h"

#include <iostream>

void PrintUsage()
{
	std::cout << "gmParser.exe inputfolder outputfolder." << std::endl;
}

#define ERROR_RETURN g_Logger.Stop(); return 1;

int main(int argc,const char **argv)
{
	int returnVal = 0;

	g_Logger.Start("gmParser.log", true);
	LOG("gmParser");

	if(argc < 2)
	{
		PrintUsage();
		ERROR_RETURN;
	}	

	Timer tme, totaltime;
	

	const char *FolderIn = argv[1];
	//const char *FolderOut = argv[2];

	if(!FileSystem::InitRawFileSystem(FolderIn))
	{
		std::cout << "Unable to Initialize File System." << std::endl;
		ERROR_RETURN;
	}

	if(!FileSystem::Mount(FolderIn,""))
	{
		std::cout << "Unable to Mount Root Folder." << std::endl;
		ERROR_RETURN;
	}

	if(!FileSystem::SetWriteDirectory(fs::path(FolderIn,fs::native)))
	{
		std::cout << "Unable to Set Write Folder." << std::endl;
		ERROR_RETURN;
	}

	// find all files to parse
	tme.Reset();
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
	std::regex ex("test.*.cpp");
#else
	boost::regex ex("test.*.cpp");
#endif
	DirectoryList dl;
	FileSystem::FindAllFiles("",dl,ex,true);
	std::cout << "Found " << dl.size() << " files in " << tme.GetElapsedSeconds() << " seconds." << std::endl;;

	// parse files
	tme.Reset();
	typedef std::vector<FileParser> FileParserList;
	FileParserList parsers;
	parsers.reserve(dl.size());
	for(obuint32 i = 0; i < dl.size(); ++i)
	{
		parsers.push_back(FileParser(dl[i].string()));
	}

	const int numParsers = parsers.size();

#pragma omp parallel for
	for(int i = 0; i < numParsers; ++i)
	{
		parsers[i].ParseFile();
		std::cout << "*";
	}

	std::cout << std::endl;
	std::cout << "Parsed " << numParsers << " files in " << tme.GetElapsedSeconds() << " seconds." << std::endl;

	// merge all to a master function list
	FunctionList allFunctions;
	allFunctions.reserve(2048);
	for(obuint32 i = 0; i < (obuint32)numParsers; ++i)
	{
		allFunctions.insert(allFunctions.end(),parsers[i].Functions.begin(),parsers[i].Functions.end());

		for(obuint32 e = 0; e < parsers[i].Info.size(); ++e)
		{
			std::cout << parsers[i].Info[e] << std::endl;
		}
	}
	std::cout << "Found " << allFunctions.size() << " functions..." << std::endl;

	// write the function bindings out to files.
	tme.Reset();
	const int numFiles = WriteFunctions(allFunctions);
	std::cout << "Wrote " << numFiles << " files in " << tme.GetElapsedSeconds() << " seconds." << std::endl;

	std::cout << "Finished in " << totaltime.GetElapsedSeconds() << " seconds." << std::endl;

	return 0;
}

//////////////////////////////////////////////////////////////////////////

IGame *CreateGameInstance()
{
	return NULL;
}

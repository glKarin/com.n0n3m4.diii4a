#include <fstream>
#include <iostream>

#include "Timer.h"
#include "gmMachine.h"

enum ErrorCodes { Success,Error };

#define ERROR_RETURN return 1;

static void GM_CDECL Callback_Print(gmMachine* a_machine, const char* a_string)
{
	std::cout << a_string << std::endl;
}

//static bool GM_CDECL Callback_Machine(gmMachine* a_machine, gmMachineCommand a_command, const void* a_context)
//{
//}

int main(int argc,const char **argv)
{
	if(argc < 2)
	{
		std::cout << "gmCompiler.exe inputfile - expected 1 arg" << std::endl;
		return Error;
	}	
	
	const char *CompileFile = argv[1];
	if(!CompileFile)
	{
		std::cout << "gmCompiler.exe inputfile - no inputfile defined" << std::endl;
		return Error;
	}

	gmMachine machine;
	machine.SetDebugMode(true);
	//gmMachine::s_machineCallback = Callback_Machine;
	gmMachine::s_printCallback = Callback_Print;

	// bind libs

	std::fstream fileIn;
	fileIn.open(CompileFile,std::ios_base::in);
	if(!fileIn.is_open())
	{
		std::cout << "gmCompiler.exe inputfile - file not found" << std::endl;
		return Error;
	}

	// get the file size
	fileIn.seekg(0, std::ios::end);
	const int fileSize = fileIn.tellg();
	fileIn.seekg(0, std::ios::beg);
	char * buffer = new char[fileSize+1];
	memset(buffer,0,sizeof(char)*(fileSize+1));

	// read the file into buffer
	fileIn.read(buffer,fileSize);
	buffer[fileSize] = 0;

	const int errors = machine.ExecuteString(buffer,NULL,true,CompileFile);
	if(errors)
	{
		std::cout << "Error Compiling " << CompileFile << std::endl;

		bool bFirst = true;
		const char *pMessage = 0;
		while((pMessage =  machine.GetLog().GetEntry(bFirst)))
		{
			
			std::cout << pMessage;
		}
		machine.GetLog().Reset();
		return Error;
	}

	Timer tme;
	while( machine.GetNumThreads() > 0 )
	{
		while(tme.GetElapsedSeconds() < 0.1f) {}
		machine.Execute((gmuint32)(tme.GetElapsedSeconds() * 1000.f));
		tme.Reset();
	}

	std::cout << CompileFile << "compiled successfully!" << std::endl;
	
	return Success;
}

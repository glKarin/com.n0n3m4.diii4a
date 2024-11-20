#include <iostream>

#include "gmmachine.h"
#include "gmthread.h"
#include "gmdebug.h"
#include "gmstreambuffer.h"

// libs

#include "gmmathlib.h"
#include "gmstringlib.h"
#include "gmarraylib.h"
#include "gmsystemlib.h"
#include "gmvector3lib.h"

#include "timer.h"
#include <math.h>

#undef GetObject

#include "gmbinder2.h"
#include "gmbinder2_class.h"

#define GM_DEBUGGER_PORT  49001

//
// globals
//

#undef GetObject

gmMachine * g_machine = NULL;
Timer g_timer;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// gmMachine exception handler
//

bool GM_CDECL gmeMachineCallback(gmMachine * a_machine, gmMachineCommand a_command, const void * a_context)
{
	if(a_command == MC_THREAD_EXCEPTION)
	{
		bool first = true;
		const char * entry;
		while((entry = a_machine->GetLog().GetEntry(first)))
		{
			fprintf(stderr, "%s", entry);
		}
		a_machine->GetLog().Reset();
	}
	return false;
}

class BindClass
{
public:
	enum Flags
	{
		Flag0,
		Flag1,
		Flag2,
		Flag3,
	};
	void FuncNoRetNoArg() 
	{
		std::cout << __FUNCTION__ << std::endl; 
	}
	bool FuncBoolNoArg()
	{
		std::cout << __FUNCTION__ << std::endl; return true; 
	}
	std::string FuncStringNoArg()
	{
		std::cout << __FUNCTION__ << std::endl; 
		return "success!"; 
	}
	std::string FuncStringArg(std::string str)
	{
		std::cout << __FUNCTION__ << std::endl;
		return str;
	}
	static int FuncRaw(gmThread *a_thread)
	{
		a_thread->PushNewString("RawFunc!");
		return GM_OK;
	}

	static void Bind(gmMachine *a_machine)
	{
		gmBind2::Class<BindClass>("BindClass",a_machine)
			.constructor()
			.constructor((const char *)0,"Under")
			.func(&BindClass::FuncNoRetNoArg,"FuncNoRetNoArg")
			.func(&BindClass::FuncBoolNoArg,"FuncBoolNoArg")
			.func(&BindClass::FuncStringNoArg,"FuncStringNoArg")
			.func(&BindClass::FuncStringArg,"FuncStringArg")
			.func(&BindClass::FuncRaw,"FuncRaw")
			.var(&BindClass::TestInt,"TestInt")
			.var(&BindClass::TestFloat,"TestFloat")
			//.var(&BindClass::TestVector,"TestVector")
			.var_readonly(&BindClass::TestFloatReadOnly,"TestFloatReadOnly")
			.var(&BindClass::TestStdString,"TestStdString")
			.var(&BindClass::TestGCTable,"TestGCTable")
			.var(&BindClass::TestGCFunction,"TestGCFunction")
			.var_bitfield(&BindClass::BitField,Flag0,"BitField0")
			.var_bitfield(&BindClass::BitField,Flag1,"BitField1")
			.var_bitfield(&BindClass::BitField,Flag2,"BitField2")
			;
	}
	BindClass() : TestInt(1), BitField(0), TestFloat(2.f), TestStdString("foo"), TestFloatReadOnly(50.f)
	{
		TestVector[0] = 1;
		TestVector[1] = 2;
		TestVector[2] = 3;
	}
private:
	int							TestInt;
	int							BitField;
	float						TestFloat;
	float						TestFloatReadOnly;
	float						TestVector[3];
	std::string					TestStdString;
	gmGCRoot<gmTableObject>		TestGCTable;
	gmGCRoot<gmFunctionObject>	TestGCFunction;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// main
//
void main(int argc, char * argv[], char * envp[])
{  
	//
	// start the machine
	//

	gmMachine::s_machineCallback = gmeMachineCallback;

	g_machine = new gmMachine;
	g_machine->SetDesiredByteMemoryUsageHard(128*1024);
	g_machine->SetDesiredByteMemoryUsageSoft(g_machine->GetDesiredByteMemoryUsageHard() * 9 / 10);
	g_machine->SetAutoMemoryUsage(true);
	g_machine->SetDebugMode(true);

	std::cout << "gmMachine Created" << std::endl;

	//
	// bind the default libs
	//

	gmBindMathLib(g_machine);
	std::cout << "Lib Bound: Math" << std::endl;
	gmBindStringLib(g_machine);
	std::cout << "Lib Bound: String" << std::endl;
	gmBindArrayLib(g_machine);
	std::cout << "Lib Bound: Array" << std::endl;
#if GM_SYSTEM_LIB
	gmBindSystemLib(g_machine);
	std::cout << "Lib Bound: System" << std::endl;
#endif
	//gmBindVector3Lib(g_machine);
	//std::cout << "Lib Bound: Vector3" << std::endl;

	BindClass::Bind(g_machine);

	//
	// execute loop
	//

	std::cout << "Running Console. Waiting for Input." << std::endl;
	g_timer.Init();

	float timeAcc = 0.0f;
	gmuint32 idt = 0;
	while(1)
	{
		enum { BufferSize=512 };
		char buffer[BufferSize] ={};

		std::cin.getline(buffer,BufferSize);

		//////////////////////////////////////////////////////////////////////////
		char *Script = 0;
		if(strstr(buffer,"runfile"))
		{			
			const char *FileName = buffer+strlen("runfile");
			while(*FileName==' ') ++FileName;
			FILE * fp = fopen(FileName, "rb");
			if(fp)
			{
				fseek(fp, 0, SEEK_END);
				int size = ftell(fp);
				rewind(fp);
				Script = new char[size + 1];
				fread(Script, 1, size, fp);
				Script[size] = 0;
				fclose(fp);
			}
		}
		if(strstr(buffer,"testfunc"))
		{
			{
				gmBind2::Function callFn(g_machine,"TestFunc");
				gmVariable returnVal = callFn << 10 << 20 << 30 << gmBind2::Null() << 40 << gmBind2::Call();

				char buffer[256];
				std::cout << returnVal.AsString(g_machine,buffer,256) << std::endl;
			}
			
			{
				gmBind2::Function callFn(g_machine,"InTable.TestFunc");
				callFn << 1 << 2 << 3 << gmBind2::Null() << 4 << gmBind2::CallAsync();
			}
			continue;
		}
		//////////////////////////////////////////////////////////////////////////
		int errors = g_machine->ExecuteString(Script?Script:buffer, NULL, false);
		if(errors)
		{
			bool first = true;
			const char * message;
			while((message = g_machine->GetLog().GetEntry(first)))
				std::cout << message;
			g_machine->GetLog().Reset();
		}
		delete [] Script;
		Script = 0;

		g_machine->Execute(idt);
		//
		// update time
		//
		timeAcc += (g_timer.Tick() * 1000.0f);
		if(timeAcc > 1.0f)
		{
			idt = (gmuint32) floorf(timeAcc);
			timeAcc -= (float) idt;
		}
		else idt = 0;

	}
#ifdef WIN32
	// Just give the OS a chance to update and run more smoothly.
	Sleep(0);
#endif //WIN32
}


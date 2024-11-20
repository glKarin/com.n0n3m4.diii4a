//  See Copyright Notice in gmMachine.h

#include "gmMachine.h"
#include "gmThread.h"
#include "gmDebug.h"
#include "gmstreambuffer.h"

#include "timer.h"
#include <math.h>

#define GM_DEBUGGER_PORT  49001

//
// globals
//

#undef GetObject

gmTimer g_timer, g_timer1;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// gmeLib
//

static POINT s_screenSize;
static HANDLE s_hConsole;

static int GM_CDECL gmfCXY(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(x, 0);
	GM_CHECK_INT_PARAM(y, 1);
	COORD point;
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > s_screenSize.x) x = s_screenSize.x;
	if(y > s_screenSize.y) y = s_screenSize.y;
	point.X = (short) x; point.Y = (short) y;
	SetConsoleCursorPosition(s_hConsole, point);
	return GM_OK;
}

static int GM_CDECL gmfCXYTEXT(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(3);
	GM_CHECK_INT_PARAM(x, 0);
	GM_CHECK_INT_PARAM(y, 1);
	GM_CHECK_STRING_PARAM(str, 2);
	COORD point;
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x > s_screenSize.x) x = s_screenSize.x;
	if(y > s_screenSize.y) y = s_screenSize.y;
	point.X = (short) x; point.Y = (short) y;
	SetConsoleCursorPosition(s_hConsole, point);
	printf("%s", str);
	return GM_OK;
}

static int GM_CDECL gmfCCLR(gmThread * a_thread)
{
	COORD coordScreen = { 0, 0 }; 
	DWORD cCharsWritten; 
	CONSOLE_SCREEN_BUFFER_INFO csbi; 
	DWORD dwConSize; 
	GetConsoleScreenBufferInfo(s_hConsole, &csbi); 
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y; 
	FillConsoleOutputCharacter(s_hConsole, TEXT(' '), dwConSize, coordScreen, &cCharsWritten); 
	GetConsoleScreenBufferInfo(s_hConsole, &csbi); 
	FillConsoleOutputAttribute(s_hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten); 
	SetConsoleCursorPosition(s_hConsole, coordScreen); 
	return GM_OK;
}

static int GM_CDECL gmfCURSOR(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(state, 0);
	GM_CHECK_INT_PARAM(size, 1);
	CONSOLE_CURSOR_INFO info;
	if(size < 1) size = 1; if(size > 100) size = 100;
	info.bVisible = state;
	info.dwSize = size;
	SetConsoleCursorInfo(s_hConsole, &info);
	return GM_OK;
}

static int GM_CDECL gmfCATTRIB(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(param, 0);
	SetConsoleTextAttribute(s_hConsole, (short) param);
	return GM_OK;
}

int GM_CDECL gmfTimer(gmThread * a_thread)
{
	a_thread->PushFloat((float)g_timer1.GetElapsedSeconds());
	return GM_OK;
}

int GM_CDECL gmfIsPressed(gmThread * a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(key, 0);
	int k = GetAsyncKeyState(key);
	a_thread->PushInt(k);
	return GM_OK;
}

void ClearConsole()
{
	gmfCCLR(0);
}

static gmFunctionEntry s_gmeLib[] = 
{ 
	{"TICK", gmfTimer},
	{"CLS", gmfCCLR},
	{"XY", gmfCXY},
	{"XYTEXT", gmfCXYTEXT},
	{"CURSOR", gmfCURSOR},
	{"CATTRIB", gmfCATTRIB},
	{"ISPRESSED", gmfIsPressed},
};


///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// debug callbacks
//

//#if GMDEBUG_SUPPORT
//
//
//void SendDebuggerMessage(gmDebugSession * a_session, const void * a_command, int a_len)
//{
//  nClient * client = (nClient *) a_session->m_user;
//  client->SendMessage((const char *) a_command, a_len);
//}
//
//
//const void * PumpDebuggerMessage(gmDebugSession * a_session, int &a_len)
//{
//  nClient * client = (nClient *) a_session->m_user;
//  return client->PumpMessage(a_len);
//}
//
//
//#endif 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
//
// gmeSetEnv
//
void GM_CDECL gmdSetEnv(gmMachine * a_machine, const char * a_env, gmTableObject * a_vars)
{
	char * t = (char *) alloca(strlen(a_env) + 1);
	strcpy(t, a_env);
	char * r = t;
	while(*r != '\0' && *r != '=') ++r;
	if(*r == '=')
	{
		*r = '\0';
		++r;
	}
	else r = "";

	a_vars->Set(a_machine, gmVariable(GM_STRING, a_machine->AllocStringObject(t)->GetRef()),
		gmVariable(GM_STRING, a_machine->AllocStringObject(r)->GetRef()));
}

void gmeInit(gmMachine * a_machine)
{
	AllocConsole();
	freopen("conin$", "r", stdin); 
	freopen("conout$", "w", stdout); 
	freopen("conout$", "w", stderr);

	CONSOLE_SCREEN_BUFFER_INFO csbi; 
	DWORD dwConSize; 

	s_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(s_hConsole, &csbi); 
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y; 
	s_screenSize.x = csbi.dwSize.X;
	s_screenSize.y = csbi.dwSize.Y;

	gmTableObject * table = a_machine->AllocTableObject();
	a_machine->GetGlobals()->Set(a_machine, "CA", gmVariable(GM_TABLE, table->GetRef()));

	table->Set(a_machine, "F_BLUE", gmVariable(GM_INT, FOREGROUND_BLUE));
	table->Set(a_machine, "F_GREEN", gmVariable(GM_INT, FOREGROUND_GREEN));
	table->Set(a_machine, "F_RED", gmVariable(GM_INT, FOREGROUND_RED));
	table->Set(a_machine, "F_INTENSITY", gmVariable(GM_INT, FOREGROUND_INTENSITY));
	table->Set(a_machine, "B_BLUE", gmVariable(GM_INT, BACKGROUND_BLUE));
	table->Set(a_machine, "B_GREEN", gmVariable(GM_INT, BACKGROUND_GREEN));
	table->Set(a_machine, "B_RED", gmVariable(GM_INT, BACKGROUND_RED));
	table->Set(a_machine, "B_INTENSITY", gmVariable(GM_INT, BACKGROUND_INTENSITY));

	table->Set(a_machine, "LEADING_BYTE", gmVariable(GM_INT, COMMON_LVB_LEADING_BYTE));
	table->Set(a_machine, "TRAILING_BYTE", gmVariable(GM_INT, COMMON_LVB_TRAILING_BYTE));
	table->Set(a_machine, "GRID_HORIZONTAL", gmVariable(GM_INT, COMMON_LVB_GRID_HORIZONTAL));
	table->Set(a_machine, "GRID_LVERTICAL", gmVariable(GM_INT, COMMON_LVB_GRID_LVERTICAL));
	table->Set(a_machine, "GRID_RVERTICAL", gmVariable(GM_INT, COMMON_LVB_GRID_RVERTICAL));
	table->Set(a_machine, "REVERSE_VIDEO", gmVariable(GM_INT, COMMON_LVB_REVERSE_VIDEO));
	table->Set(a_machine, "UNDERSCORE", gmVariable(GM_INT, COMMON_LVB_UNDERSCORE));

	a_machine->RegisterLibrary(s_gmeLib, sizeof(s_gmeLib) / sizeof(s_gmeLib[0]));
}

/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop
#include "sys_local.h"

const char *sysLanguageNames[] = {
	"english", "spanish", "italian", "german", "french", "russian",
	"polish", "korean", "japanese", "chinese", NULL
};

idCVar sys_lang("sys_lang", "english", CVAR_SYSTEM | CVAR_ARCHIVE,  "", sysLanguageNames, idCmdSystem::ArgCompletion_String<sysLanguageNames>);

idSysLocal			sysLocal;
idSys 				*sys = &sysLocal;

float analogx=0.0f;
float analogy=0.0f;
int analogenabled=0;

void idSysLocal::DebugPrintf(const char *fmt, ...)
{
	va_list argptr;

	va_start(argptr, fmt);
	Sys_DebugVPrintf(fmt, argptr);
	va_end(argptr);
}

void idSysLocal::DebugVPrintf(const char *fmt, va_list arg)
{
	Sys_DebugVPrintf(fmt, arg);
}

double idSysLocal::GetClockTicks(void)
{
	return Sys_GetClockTicks();
}

double idSysLocal::ClockTicksPerSecond(void)
{
	return Sys_ClockTicksPerSecond();
}

int idSysLocal::GetProcessorId(void)
{
	return Sys_GetProcessorId();
}

const char *idSysLocal::GetProcessorString(void)
{
	return Sys_GetProcessorString();
}

const char *idSysLocal::FPU_GetState(void)
{
	return Sys_FPU_GetState();
}

bool idSysLocal::FPU_StackIsEmpty(void)
{
	return Sys_FPU_StackIsEmpty();
}

void idSysLocal::FPU_SetFTZ(bool enable)
{
	Sys_FPU_SetFTZ(enable);
}

void idSysLocal::FPU_SetDAZ(bool enable)
{
	Sys_FPU_SetDAZ(enable);
}

bool idSysLocal::LockMemory(void *ptr, int bytes)
{
	return Sys_LockMemory(ptr, bytes);
}

bool idSysLocal::UnlockMemory(void *ptr, int bytes)
{
	return Sys_UnlockMemory(ptr, bytes);
}

void idSysLocal::GetCallStack(address_t *callStack, const int callStackSize)
{
	Sys_GetCallStack(callStack, callStackSize);
}

const char *idSysLocal::GetCallStackStr(const address_t *callStack, const int callStackSize)
{
	return Sys_GetCallStackStr(callStack, callStackSize);
}

const char *idSysLocal::GetCallStackCurStr(int depth)
{
	return Sys_GetCallStackCurStr(depth);
}

void idSysLocal::ShutdownSymbols(void)
{
	Sys_ShutdownSymbols();
}

uintptr_t idSysLocal::DLL_Load(const char *dllName)
{
	DebugPrintf("idSysLocal::DLL_Load(%s)\n", dllName);
	return Sys_DLL_Load(dllName);
}

void *idSysLocal::DLL_GetProcAddress(uintptr_t dllHandle, const char *procName)
{
	return Sys_DLL_GetProcAddress(dllHandle, procName);
}

void idSysLocal::DLL_Unload(uintptr_t dllHandle)
{
	Sys_DLL_Unload(dllHandle);
}

void idSysLocal::DLL_GetFileName(const char *baseName, char *dllName, int maxLength)
{
#ifdef _WIN32
	idStr::snPrintf(dllName, maxLength, "%s" CPUSTRING ".dll", baseName);
#elif defined( __linux__ )
	idStr::snPrintf(dllName, maxLength, "%s" CPUSTRING ".so", baseName);
#elif defined( MACOS_X )
	idStr::snPrintf(dllName, maxLength, "%s" ".dylib", baseName);
#else
#error OS define is required
#endif
}

#ifdef _SPLASHDAMAGE //karin: return a event heap pointer
const sdSysEvent* idSysLocal::GenerateMouseButtonEvent(int button, bool down)
{
	sysEvent_t *ev = new sysEvent_t;
	ev->Memset();
	ev->evType = SE_KEY;
	ev->evValue = K_MOUSE1 + button - 1;
	ev->evValue2 = down;
	return ev;
}

const sdSysEvent* idSysLocal::GenerateMouseMoveEvent(int deltax, int deltay)
{
	sysEvent_t *ev = new sysEvent_t;
	ev->Memset();
	ev->evType = SE_MOUSE;
	ev->evValue = deltax;
	ev->evValue2 = deltay;
	return ev;
}
#else
sysEvent_t idSysLocal::GenerateMouseButtonEvent(int button, bool down)
{
	sysEvent_t ev;
	ev.evType = SE_KEY;
	ev.evValue = K_MOUSE1 + button - 1;
	ev.evValue2 = down;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}

sysEvent_t idSysLocal::GenerateMouseMoveEvent(int deltax, int deltay)
{
	sysEvent_t ev;
	ev.evType = SE_MOUSE;
	ev.evValue = deltax;
	ev.evValue2 = deltay;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;
	return ev;
}
#endif

void idSysLocal::FPU_EnableExceptions(int exceptions)
{
	Sys_FPU_EnableExceptions(exceptions);
}

/*
=================
Sys_TimeStampToStr
=================
*/
const char *Sys_TimeStampToStr(ID_TIME_T timeStamp)
{
	static char timeString[MAX_STRING_CHARS];
	timeString[0] = '\0';

	tm	*time = localtime(&timeStamp);
	idStr out;

	idStr lang = cvarSystem->GetCVarString("sys_lang");

	if (lang.Icmp("english") == 0) {
		// english gets "month/day/year  hour:min" + "am" or "pm"
		out = va("%02d", time->tm_mon + 1);
		out += "/";
		out += va("%02d", time->tm_mday);
		out += "/";
		out += va("%d", time->tm_year + 1900);
		out += "\t";

		if (time->tm_hour > 12) {
			out += va("%02d", time->tm_hour - 12);
		} else if (time->tm_hour == 0) {
			out += "12";
		} else {
			out += va("%02d", time->tm_hour);
		}

		out += ":";
		out +=va("%02d", time->tm_min);

		if (time->tm_hour >= 12) {
			out += "pm";
		} else {
			out += "am";
		}
	} else {
		// europeans get "day/month/year  24hour:min"
		out = va("%02d", time->tm_mday);
		out += "/";
		out += va("%02d", time->tm_mon + 1);
		out += "/";
		out += va("%d", time->tm_year + 1900);
		out += "\t";
		out += va("%02d", time->tm_hour);
		out += ":";
		out += va("%02d", time->tm_min);
	}

	idStr::Copynz(timeString, out, sizeof(timeString));

	return timeString;
}

#ifdef _SPLASHDAMAGE //karin: sysEvent
#if 0 //karin: make as POD
sysEvent_s::~sysEvent_s( void ) {
	//Mem_Free( evPtr ); //karin: as return value or copy, so using FreeData/FreeEvent to manual delete, or declare operator=/constructor
}

sdSysEvent& sdSysEvent::operator=( const sdSysEvent& rhs ) {
	if (&rhs == this)
		return *this;

	evType = rhs.evType;
	evValue = rhs.evValue;
	evValue2 = rhs.evValue2;

	FreeData();
	if ( rhs.evPtr && rhs.evPtrLength > 0 ) {
		evPtrLength = rhs.evPtrLength;
		evPtr = Mem_Alloc( evPtrLength );
		::memcpy( evPtr, rhs.evPtr, rhs.evPtrLength );
	}

	return *this;
}
#endif


void sysEvent_s::Memset() {
	evType = SE_NONE;
	evPtrLength = 0;
	evPtr = NULL;
	evValue = evValue2 = 0;
}

void sysEvent_s::FreeData( void ) {
	if (evPtr) {
		Mem_Free( evPtr );
		evPtr = NULL;
	}
	evPtrLength = 0;
}

void sysEvent_s::Init( sysEventType_t _type, int _value, int _value2, int _ptrLength, void* _ptr ) {
	evType = _type;
	evValue = _value;
	evValue2 = _value2;
	evPtrLength = _ptrLength;
	evPtr = _ptr;
}

void sdSysEvent::Save( idFile* file ) {
	file->WriteInt( evType );
	file->WriteInt( evValue );
	file->WriteInt( evValue2 );
	file->WriteInt( evPtrLength );
	if ( evPtrLength ) {
		assert( evPtr );
		file->Write( evPtr, evPtrLength );
	}
}

void sdSysEvent::Restore( idFile* file ) {
	int type;
	file->ReadInt( type );
	evType = (sysEventType_t)type;
	file->ReadInt( evValue );
	file->ReadInt( evValue2 );
	int _evPtrLength;
	file->ReadInt( _evPtrLength );

	FreeData();
	evPtrLength = _evPtrLength;
	if ( _evPtrLength ) {
		evPtr = Mem_Alloc( _evPtrLength );
		file->Read( evPtr, _evPtrLength );
	}
}
#endif

#ifdef _SPLASHDAMAGE
#include <locale.h>
#include "sys/sys_keyboard.h"
#include "sys/sys_ime.h"

ID_INLINE static void Sys_ConvertSysTimeToStdTm(struct tm &t, const sysTime_t &src) {
	t.tm_sec = src.tm_sec;
	t.tm_min = src.tm_min;
	t.tm_hour = src.tm_hour;
	t.tm_mday = src.tm_mday;
	t.tm_mon = src.tm_mon;
	t.tm_year = src.tm_year;
	t.tm_wday = src.tm_wday;
	t.tm_yday = src.tm_yday;
	t.tm_isdst = src.tm_isdst;
}

ID_INLINE static void Sys_ConvertStdTmToSysTime(sysTime_t &t, const struct tm &src) {
	t.tm_sec = src.tm_sec;
	t.tm_min = src.tm_min;
	t.tm_hour = src.tm_hour;
	t.tm_mday = src.tm_mday;
	t.tm_mon = src.tm_mon;
	t.tm_year = src.tm_year;
	t.tm_wday = src.tm_wday;
	t.tm_yday = src.tm_yday;
	t.tm_isdst = src.tm_isdst;
}

void idSysLocal::GetCPUInfo( cpuInfo_t& info ) {
	int packageNum = 0;
	Sys_CPUCount( info.logicalNum, info.physicalNum, packageNum );
	info.hyperThreadedStatus = 0;
}

int idSysLocal::Milliseconds() {
	return Sys_Milliseconds();
}

time_t idSysLocal::RealTime( sysTime_t* sysTime ) {
	time_t t = time(NULL);
	SecondsToTime(t, *sysTime);
	return t;
}

const char* idSysLocal::TimeToSystemStr( const sysTime_t& sysTime ) {
	static char ret[64];
	struct tm t;
	Sys_ConvertSysTimeToStdTm(t, sysTime);
	strftime(ret, sizeof(ret), "%H:%M:%S", &t);
	return ret;
}

const char* idSysLocal::TimeAndDateToSystemStr( const sysTime_t& sysTime ) {
	static char ret[64];
	struct tm t;
	Sys_ConvertSysTimeToStdTm(t, sysTime);
	strftime(ret, sizeof(ret), "%Y-%m-%d %H:%M:%S", &t);
	return ret;
}

time_t idSysLocal::TimeDiff( const sysTime_t& from, const sysTime_t& to ) {
	struct tm a, b;
	Sys_ConvertSysTimeToStdTm(a, from);
	Sys_ConvertSysTimeToStdTm(b, to);
	time_t at = mktime(&a);
	time_t bt = mktime(&b);
	return bt - at;
}

void idSysLocal::SecondsToTime( const time_t t, sysTime_t& out, bool localTime ) {
	struct tm res;
	struct tm *tmp;
	if (localTime) {
		tmp = localtime(&t);
	} else {
		tmp = gmtime(&t);
	}
	memcpy(&res, tmp, sizeof(res));
	Sys_ConvertStdTmToSysTime(out, res);
}

const char * idSysLocal::GetCurCallStackStr( int depth ) {
	return "";
}

void * idSysLocal::DLL_Load( const char *dllName, bool checkFullPathMatch ) {
	return (void *)DLL_Load(dllName);
}

void * idSysLocal::DLL_GetProcAddress( void* dllHandle, const char *procName ) {
	return (void *)DLL_GetProcAddress((uintptr_t)dllHandle, procName);
}

void idSysLocal::DLL_Unload( void* dllHandle ) {
	DLL_Unload((uintptr_t)dllHandle);
}

void idSysLocal::ProcessOSEvents() {
}

const sdSysEvent* idSysLocal::GenerateGuiEvent( int value ) {
	sysEvent_t *ev = new sysEvent_t;
	ev->Memset();
	ev->evType = SE_GUI;
	ev->evValue = value;
	return ev;
}

void idSysLocal::FreeEvent( const sdSysEvent* event ) {
	if (event) {
		if (event->evPtr)
			Mem_Free(event->evPtr);
		delete event;
	}
}

idWStr idSysLocal::GetClipboardData( void ) {
	return StrToWStr(Sys_GetClipboardData());
}

void idSysLocal::SetClipboardData( const wchar_t *string ) {
	idStr text = WStrToStr(string);
	Sys_SetClipboardData(text);
}

idKeyboard& idSysLocal::Keyboard() {
	return *globalKeyboard;
}

sdIME& idSysLocal::IME() {
	return *globalIME;
}

void idSysLocal::SetSystemLocale() {
	//setlocale(LC_ALL, "");
	//setlocale(LC_ALL, "C");
}

void idSysLocal::SetDefaultLocale() {
	//setlocale(LC_ALL, "");
}

const char * idSysLocal::NetAdrToString( const netadr_t& a ) const {
	return Sys_NetAdrToString(a);
}

bool idSysLocal::StringToNetAdr( const char *s, netadr_t *a, bool doDNSResolve ) const {
	return Sys_StringToNetAdr(s, a, doDNSResolve);
}

#include "../framework/Session_local.h"
void idSysLocal::SetServerInfo( const char* key, const char* value ) {
	sessLocal.mapSpawnData.serverInfo.Set(key, value);
}

void idSysLocal::FlushServerInfo( void ) {
	sessLocal.mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict(CVAR_SERVERINFO);
}

#endif

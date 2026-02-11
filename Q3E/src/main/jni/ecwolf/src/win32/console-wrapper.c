/*
** console-wrapper.c
**
**---------------------------------------------------------------------------
** Copyright 2025 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** Runs the main game engine executable with an additional parameter passed
** to run under the console SUBSYSTEM. This program exists to improve the
** user experience of running the game engine under a command prompt.
**
** This program is written to not depend on the C runtime enabling a tiny
** executable size.
**
*/
#define WIN32_LEAN_AND_MEAN
#define STRSAFE_NO_DEPRECATE
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>

#ifdef _M_IX86
// Relevant compatibility functions for Windows 98/98SE/ME (performs similar function to unicows)
#undef CreateProcess
#undef GetFileAttributes
#undef GetModuleFileName
#undef WriteConsole
#define CreateProcess CreateProcessCompat
#define GetFileAttributes GetFileAttributesCompat
#define GetModuleFileName GetModuleFileNameCompat
#define WriteConsole WriteConsoleCompat

static char* ConvertString(const wchar_t* str, int inLen)
{
	int len = WideCharToMultiByte(CP_ACP, 0, str, inLen, NULL, 0, NULL, NULL);
	if(len == 0)
		return NULL;
	char* out = (char*)LocalAlloc(0, len);
	if(!out)
		return NULL;
	if(WideCharToMultiByte(CP_ACP, 0, str, inLen, out, len, NULL, NULL) == 0)
	{
		*out = 0;
		return out;
	}
	return out;
}

static BOOL CreateProcess(const wchar_t* lpApplicationName, wchar_t* lpCommandLine, SECURITY_ATTRIBUTES* lpProcessAttributes, SECURITY_ATTRIBUTES* lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, void* lpEnvironment, const wchar_t* lpCurrentDirectory, STARTUPINFO* lpStartupInfo, PROCESS_INFORMATION* lpProcessInformation)
{
	BOOL ret = CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	if(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{
		STARTUPINFOA aStartupInfo = {
			sizeof(STARTUPINFOA),
			NULL,
			lpStartupInfo->lpDesktop ? ConvertString(lpStartupInfo->lpDesktop, lstrlen(lpStartupInfo->lpDesktop)+1) : NULL,
			lpStartupInfo->lpTitle ? ConvertString(lpStartupInfo->lpTitle, lstrlen(lpStartupInfo->lpTitle)+1) : NULL,
			lpStartupInfo->dwX,
			lpStartupInfo->dwY,
			lpStartupInfo->dwXSize,
			lpStartupInfo->dwYSize,
			lpStartupInfo->dwXCountChars,
			lpStartupInfo->dwYCountChars,
			lpStartupInfo->dwFillAttribute,
			lpStartupInfo->dwFlags,
			lpStartupInfo->wShowWindow,
			lpStartupInfo->cbReserved2,
			lpStartupInfo->lpReserved2,
			lpStartupInfo->hStdInput,
			lpStartupInfo->hStdOutput,
			lpStartupInfo->hStdError,
		};
		const char* aApplicationName = lpApplicationName ? ConvertString(lpApplicationName, lstrlen(lpApplicationName)+1) : NULL;
		char* aCommandLine = lpCommandLine ? ConvertString(lpCommandLine, lstrlen(lpCommandLine)+1) : NULL;
		const char* aCurrentDirectory = lpCurrentDirectory ? ConvertString(lpCurrentDirectory, lstrlen(lpCurrentDirectory)+1) : NULL;
		ret = CreateProcessA(aApplicationName, aCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, aCurrentDirectory, &aStartupInfo, lpProcessInformation);
		LocalFree((void*)aCurrentDirectory);
		LocalFree(aCommandLine);
		LocalFree((void*)aApplicationName);
		LocalFree(aStartupInfo.lpTitle);
		LocalFree(aStartupInfo.lpDesktop);
	}
	return ret;
}

static DWORD GetFileAttributes(const wchar_t* lpFilename)
{
	DWORD ret = GetFileAttributesW(lpFilename);
	if(ret == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{
		const char* aFilename = ConvertString(lpFilename, lstrlen(lpFilename)+1);
		if(!aFilename)
			return INVALID_FILE_ATTRIBUTES;
		ret = GetFileAttributesA(aFilename);
		LocalFree((void*)aFilename);
	}
	return ret;
}

static DWORD GetModuleFileName(HMODULE hModule, wchar_t* lpFilename, DWORD nSize)
{
	DWORD ret = GetModuleFileNameW(hModule, lpFilename, nSize);
	if(ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{
		char* aFilename = (char*)LocalAlloc(0, nSize);
		if(aFilename == NULL)
			return 0;
		ret = GetModuleFileNameA(hModule, aFilename, nSize);
		ret = MultiByteToWideChar(CP_ACP, 0, aFilename, ret, lpFilename, nSize);
		LocalFree(aFilename);
	}
	return ret;
}

static BOOL WriteConsole(HANDLE hConsoleOutput, const wchar_t* lpBuffer, DWORD nNumberOfCharsToWrite, DWORD* lpNumberOfCharsWritten, void* lpReserved)
{
	BOOL ret = WriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	if(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
	{
		const char* aBuffer = ConvertString(lpBuffer, nNumberOfCharsToWrite);
		DWORD dummy = 0; // This seems to have been required on old Windows?
		ret = WriteConsoleA(hConsoleOutput, aBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten ? lpNumberOfCharsWritten : &dummy, NULL);
		LocalFree((void*)aBuffer);
		ret = TRUE;
	}
	return ret;
}
#endif

#define ECWOLF_EXTRA_COMMAND_LINE L" --console"
#define GZDOOM_EXTRA_COMMAND_LINE L" -stdout"

#define MAX_CANDIDATE_LENGTH 10
static const wchar_t* const EXECUTABLE_CANDIDATES[] = {
	L"ecwolf.exe",
	L"gzdoom.exe",
	L"lzwolf.exe",
	L"raze.exe",
	L"vkdoom.exe",
	NULL
};

static void PrintString(const wchar_t* str)
{
	WriteConsole(GetStdHandle(STD_ERROR_HANDLE), str, lstrlen(str), NULL, NULL);
}

static void PrintHex(DWORD number)
{
	wchar_t buffer[9];
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4995)
#endif
	wsprintf(buffer, L"%X", number);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	PrintString(buffer);
}

static const wchar_t* GetExecutablePath()
{
	wchar_t* filePath = (wchar_t*)LocalAlloc(0, MAX_PATH * sizeof(wchar_t));
	if(!filePath)
		return NULL;
	DWORD nSize = GetModuleFileName(NULL, filePath, MAX_PATH);

	if(nSize == MAX_PATH && StrCmpN(filePath, L"\\\\?\\", 4) == 0)
	{
		wchar_t* newFilePath = (wchar_t*)LocalReAlloc(filePath, 0x7FFF * sizeof(wchar_t), 0);
		if(!newFilePath)
		{
			LocalFree(filePath);
			return NULL;
		}
		filePath = newFilePath;

		nSize = GetModuleFileName(NULL, filePath, 0x7FFF);
	}

	if(nSize == 0 && GetLastError() != ERROR_SUCCESS)
	{
		LocalFree(filePath);
		return NULL;
	}

	for(DWORD pos = nSize; pos-- > 0;)
	{
		if(filePath[pos] == L'\\')
		{
			filePath[pos] = 0;
			break;
		}
	}

	return filePath;
}

static const wchar_t* GetEngineExecutable()
{
	const wchar_t* exePath = GetExecutablePath();
	size_t exePathLen = lstrlen(exePath);
	size_t enginePathMax = exePathLen + MAX_CANDIDATE_LENGTH + 2;
	wchar_t* enginePath = (wchar_t*)LocalAlloc(0, enginePathMax * sizeof(wchar_t));
	if(!enginePath || FAILED(StringCchCopy(enginePath, enginePathMax, exePath)))
	{
		PrintString(L"Failed to copy executable path ");
		PrintString(exePath);
		PrintString(L"\n");
		LocalFree(enginePath);
		LocalFree((void*)exePath);
		return NULL;
	}
	LocalFree((void*)exePath);
	enginePath[exePathLen] = L'\\';

	for(const wchar_t* const* candidate = EXECUTABLE_CANDIDATES; *candidate; ++candidate)
	{
		enginePath[exePathLen+1] = 0;
		if(FAILED(StringCchCat(enginePath, enginePathMax, *candidate)))
		{
			PrintString(L"Failed to form candidate path for ");
			PrintString(*candidate);
			PrintString(L"\n");
			continue;
		}

		if(GetFileAttributes(enginePath) != INVALID_FILE_ATTRIBUTES)
			return enginePath;
	}

	LocalFree(enginePath);
	return NULL;
}

static const wchar_t* GetExtraCommandLineForExecutable(const wchar_t* path)
{
	const wchar_t* fileName = StrRChr(path, NULL, L'\\');
	if(fileName == NULL)
		fileName = path;

	if(StrStr(fileName, L"wolf"))
		return ECWOLF_EXTRA_COMMAND_LINE;
	return GZDOOM_EXTRA_COMMAND_LINE;
}

static const wchar_t* StripArg0(const wchar_t* commandLine)
{
	size_t length = lstrlen(commandLine);
	if(length == 0)
		return commandLine;

	if(commandLine[0] != L'"')
	{
		for(const wchar_t* out = commandLine + 1; *out; ++out)
		{
			if(*out == L' ' || *out == L'\t')
				return out;
		}
		return commandLine;
	}

	for(const wchar_t* out = commandLine + 1; *out; ++out)
	{
		if(*out == '"' && *(out-1) != L'"')
			return out+1;
	}
	return commandLine + 1;
}

static const wchar_t* MakeArg0(const wchar_t* arg0)
{
	size_t length = lstrlen(arg0);
	if(length == 0) {
		wchar_t* out = (wchar_t*)LocalAlloc(0, 3 * sizeof(wchar_t));
		if(!out)
			return NULL;
		out[0] = L'"';
		out[1] = L'"';
		out[2] = 0;
		return out;
	}

	size_t extraNeeded = 2;
	for(const wchar_t* c = arg0; *c; ++c)
	{
		if(*c == L'"')
			++extraNeeded;
	}

	size_t outLength = length + extraNeeded + 1;
	wchar_t* out = (wchar_t*)LocalAlloc(0, outLength * sizeof(wchar_t));
	if(!out)
		return NULL;
	out[0] = L'"';
	wchar_t* oc = out+1;
	for(const wchar_t* c = arg0; *c; ++c)
	{
		if(*c == L'"')
			*oc++ = L'\\';
		*oc++ = *c;
	}
	*oc++ = L'"';
	*oc = 0;

	return out;
}

static wchar_t* GetNewCommandLine(const wchar_t* engineExecutablePath)
{
	const wchar_t* inputCommandLine = StripArg0(GetCommandLine());
	const wchar_t* extraCommandLine = GetExtraCommandLineForExecutable(engineExecutablePath);
	const wchar_t* arg0 = MakeArg0(engineExecutablePath);
	if(!arg0)
		return NULL;

	size_t newCommandLineMax = lstrlen(arg0) + lstrlen(inputCommandLine) + lstrlen(extraCommandLine) + 1;
	wchar_t* newCommandLine = (wchar_t*)LocalAlloc(0, newCommandLineMax * sizeof(wchar_t));
	if(!newCommandLine || FAILED(StringCchCopy(newCommandLine, newCommandLineMax, arg0)))
	{
		PrintString(L"Failed to copy arg0: ");
		PrintString(arg0);
		PrintString(L"\n");
		LocalFree(newCommandLine);
		LocalFree((void*)arg0);
		return NULL;
	}
	LocalFree((void*)arg0);
	if(FAILED(StringCchCat(newCommandLine, newCommandLineMax, inputCommandLine)))
	{
		PrintString(L"Failed to copy command line: ");
		PrintString(inputCommandLine);
		PrintString(L"\n");
		LocalFree(newCommandLine);
		return NULL;
	}
	if(FAILED(StringCchCat(newCommandLine, newCommandLineMax, extraCommandLine)))
	{
		PrintString(L"Failed to append extra command line: ");
		PrintString(extraCommandLine);
		PrintString(L"\n");
		LocalFree(newCommandLine);
		return NULL;
	}
	return newCommandLine;
}

int wmain()
{
	const wchar_t* application = GetEngineExecutable();
	if(!application)
	{
		PrintString(L"Failed to obtain engine executable\n");
		return EXIT_FAILURE;
	}

	wchar_t* commandLine = GetNewCommandLine(application);
	if(!commandLine)
	{
		PrintString(L"Failed to create new command line\n");
		return EXIT_FAILURE;
	}

	STARTUPINFO startupInfo;
	SecureZeroMemory(&startupInfo, sizeof(STARTUPINFO)); // Avoids possible memset
	startupInfo.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION procInfo;
	if(!CreateProcess(application, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &procInfo))
	{
		PrintString(L"Failed to create process for");
		PrintString(application);
		PrintString(L": ");
		PrintHex(GetLastError());
		return EXIT_FAILURE;
	}
	WaitForSingleObject(procInfo.hProcess, INFINITE);
	DWORD exitCode = 0;
	GetExitCodeProcess(procInfo.hProcess, &exitCode);
	CloseHandle(procInfo.hThread);
	CloseHandle(procInfo.hProcess);
	return exitCode;
}

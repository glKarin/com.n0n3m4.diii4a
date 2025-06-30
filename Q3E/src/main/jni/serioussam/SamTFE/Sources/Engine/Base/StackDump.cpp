/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "Engine/StdH.h"

#include <Engine/Base/Console_Internal.h>

#include <Engine/Build.h>

#if (!defined PLATFORM_WIN32)
#error You probably should not try to compile this.
#endif

extern ULONG _ulEngineBuildMajor;
extern ULONG _ulEngineBuildMinor;

//==========================================
// Matt Pietrek
// Microsoft Systems Journal, May 1997
// FILE: MSJEXHND.CPP

class MSJExceptionHandler
{
      public:

      MSJExceptionHandler( );
      ~MSJExceptionHandler( );

      void SetLogFileName(const char* pszLogFileName );

      private:

      // entry point where control comes on an unhandled exception
      static LONG WINAPI MSJUnhandledExceptionFilter(
                                           PEXCEPTION_POINTERS pExceptionInfo );

      // where report info is extracted and generated
      static void GenerateExceptionReport( PEXCEPTION_POINTERS pExceptionInfo );

      // Helper functions
      static const char* GetExceptionString( DWORD dwCode );
      static BOOL GetLogicalAddress(PVOID addr, char* szModule, DWORD len,
                                    DWORD& section, DWORD& offset );
      static void IntelStackWalk( PCONTEXT pContext );
      static int __cdecl _tprintf(const char * format, ...);

      // Variables used by the class
      static char m_szLogFileName[MAX_PATH];
      static LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter;
      static HANDLE m_hReportFile;

};

//extern MSJExceptionHandler g_MSJExceptionHandler;  // global instance of class

MSJExceptionHandler g_MSJExceptionHandler;  // Declare global instance of class

//============================== Global Variables =============================

//
// Declare the static variables of the MSJExceptionHandler class
//
char MSJExceptionHandler::m_szLogFileName[MAX_PATH];
LPTOP_LEVEL_EXCEPTION_FILTER MSJExceptionHandler::m_previousFilter;
HANDLE MSJExceptionHandler::m_hReportFile;

//============================== Class Methods =============================

//=============
// Constructor
//=============
MSJExceptionHandler::MSJExceptionHandler( )
{
    // Install the unhandled exception filter function
    m_previousFilter = SetUnhandledExceptionFilter(MSJUnhandledExceptionFilter);

    // Figure out what the report file will be named, and store it away
    GetModuleFileNameA( 0, m_szLogFileName, MAX_PATH );

    // Look for the '.' before the "EXE" extension.  Replace the extension
    // with "RPT"
    char* pszDot = strrchr( m_szLogFileName, '.' );
    if ( pszDot )
    {
        pszDot++;   // Advance past the '.'
        if ( strlen(pszDot) >= 3 )
            strcpy( pszDot, "RPT" );   // "RPT" -> "Report"
    }
}

//============
// Destructor
//============
MSJExceptionHandler::~MSJExceptionHandler( )
{
    SetUnhandledExceptionFilter( m_previousFilter );
}

//==============================================================
// Lets user change the name of the report file to be generated
//==============================================================
void MSJExceptionHandler::SetLogFileName(const char* pszLogFileName )
{
    strcpy( m_szLogFileName, pszLogFileName );
}

//===========================================================
// Entry point where control comes on an unhandled exception
//===========================================================
LONG WINAPI MSJExceptionHandler::MSJUnhandledExceptionFilter(
                                             PEXCEPTION_POINTERS pExceptionInfo )
{
    m_hReportFile = CreateFileA( m_szLogFileName,
                                GENERIC_WRITE,
                                0,
                                0,
                                OPEN_ALWAYS,
                                FILE_FLAG_WRITE_THROUGH,
                                0 );

    if ( m_hReportFile )
    {
        SetFilePointer( m_hReportFile, 0, 0, FILE_END );

        GenerateExceptionReport( pExceptionInfo );

        CloseHandle( m_hReportFile );
        m_hReportFile = 0;
    }

    // make sure the console log was written safely
    if (_pConsole!=NULL) {
      _pConsole->CloseLog();
    }
    extern void EnableWindowsKeys(void);
    EnableWindowsKeys();

    if ( m_previousFilter )
        return m_previousFilter( pExceptionInfo );
    else
        return EXCEPTION_CONTINUE_SEARCH;
}

//===========================================================================
// Open the report file, and write the desired information to it.  Called by
// MSJUnhandledExceptionFilter
//===========================================================================
void MSJExceptionHandler::GenerateExceptionReport(
    PEXCEPTION_POINTERS pExceptionInfo )
{
    // Start out with a banner
    _tprintf( "//=====================================================\n" );
    char strTime[80];
    _strtime(strTime);
    char strDate[80];
    _strdate(strDate);
    _tprintf( "Crashed at: %s %s\n", strDate, strTime);
    _tprintf( "Version: %d.%d%s%s\n", _ulEngineBuildMajor, _ulEngineBuildMinor, _SE_BUILD_EXTRA, _SE_DEMO?"-demo":"");

    PEXCEPTION_RECORD pExceptionRecord = pExceptionInfo->ExceptionRecord;

    // First print information about the type of fault
    _tprintf(   "Exception code: %08X %s\n",
                pExceptionRecord->ExceptionCode,
                GetExceptionString(pExceptionRecord->ExceptionCode) );

    // Now print information about where the fault occured
    char szFaultingModule[MAX_PATH];
    DWORD section, offset;
    GetLogicalAddress(  pExceptionRecord->ExceptionAddress,
                        szFaultingModule,
                        sizeof( szFaultingModule ),
                        section, offset );

    _tprintf( "Fault address:  %08X %02X:%08X %s\n",
              pExceptionRecord->ExceptionAddress,
              section, offset, szFaultingModule );

    PCONTEXT pCtx = pExceptionInfo->ContextRecord;

    // Show the registers
    #ifdef _M_IX86  // Intel Only!
    _tprintf( "\nRegisters:\n" );

    _tprintf("EAX:%08X\nEBX:%08X\nECX:%08X\nEDX:%08X\nESI:%08X\nEDI:%08X\n",
             pCtx->Eax, pCtx->Ebx, pCtx->Ecx, pCtx->Edx, pCtx->Esi, pCtx->Edi );

    _tprintf( "CS:EIP:%04X:%08X\n", pCtx->SegCs, pCtx->Eip );
    _tprintf( "SS:ESP:%04X:%08X  EBP:%08X\n",
              pCtx->SegSs, pCtx->Esp, pCtx->Ebp );
    _tprintf( "DS:%04X  ES:%04X  FS:%04X  GS:%04X\n",
              pCtx->SegDs, pCtx->SegEs, pCtx->SegFs, pCtx->SegGs );
    _tprintf( "Flags:%08X\n", pCtx->EFlags );

    #endif

    #ifdef _M_IX86  // Intel Only!
    // Walk the stack using x86 specific code
    IntelStackWalk( pCtx );
    #endif

    _tprintf( "\n" );
}

//======================================================================
// Given an exception code, returns a pointer to a static string with a
// description of the exception
//======================================================================
const char* MSJExceptionHandler::GetExceptionString( DWORD dwCode )
{
    #define EXCEPTION( x ) case EXCEPTION_##x: return #x;

    switch ( dwCode )
    {
        EXCEPTION( ACCESS_VIOLATION )
        EXCEPTION( DATATYPE_MISALIGNMENT )
    case EXCEPTION_BREAKPOINT: return "BREAKPOINT";
        //EXCEPTION( BREAKPOINT )
        EXCEPTION( SINGLE_STEP )
        EXCEPTION( ARRAY_BOUNDS_EXCEEDED )
        EXCEPTION( FLT_DENORMAL_OPERAND )
        EXCEPTION( FLT_DIVIDE_BY_ZERO )
        EXCEPTION( FLT_INEXACT_RESULT )
        EXCEPTION( FLT_INVALID_OPERATION )
        EXCEPTION( FLT_OVERFLOW )
        EXCEPTION( FLT_STACK_CHECK )
        EXCEPTION( FLT_UNDERFLOW )
        EXCEPTION( INT_DIVIDE_BY_ZERO )
        EXCEPTION( INT_OVERFLOW )
        EXCEPTION( PRIV_INSTRUCTION )
        EXCEPTION( IN_PAGE_ERROR )
        EXCEPTION( ILLEGAL_INSTRUCTION )
        EXCEPTION( NONCONTINUABLE_EXCEPTION )
        EXCEPTION( STACK_OVERFLOW )
        EXCEPTION( INVALID_DISPOSITION )
        EXCEPTION( GUARD_PAGE )
        EXCEPTION( INVALID_HANDLE )
    }

    // If not one of the "known" exceptions, try to get the string
    // from NTDLL.DLL's message table.

    static char szBuffer[512] = { 0 };

    FormatMessageA(  FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandleA( "NTDLL.DLL" ),
                    dwCode, 0, szBuffer, sizeof( szBuffer ), 0 );

    return szBuffer;
}

//==============================================================================
// Given a linear address, locates the module, section, and offset containing
// that address.
//
// Note: the szModule paramater buffer is an output buffer of length specified
// by the len parameter (in characters!)
//==============================================================================
BOOL MSJExceptionHandler::GetLogicalAddress(
        PVOID addr, char* szModule, DWORD len, DWORD& section, DWORD& offset )
{
    MEMORY_BASIC_INFORMATION mbi;

    if ( !VirtualQuery( addr, &mbi, sizeof(mbi) ) )
        return FALSE;

    DWORD hMod = (DWORD)mbi.AllocationBase;

    if ( !GetModuleFileNameA( (HMODULE)hMod, szModule, len ) )
        return FALSE;

    // Point to the DOS header in memory
    PIMAGE_DOS_HEADER pDosHdr = (PIMAGE_DOS_HEADER)hMod;

    // From the DOS header, find the NT (PE) header
    PIMAGE_NT_HEADERS pNtHdr = (PIMAGE_NT_HEADERS)(hMod + pDosHdr->e_lfanew);

    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION( pNtHdr );

    DWORD rva = (DWORD)addr - hMod; // RVA is offset from module load address

    // Iterate through the section table, looking for the one that encompasses
    // the linear address.
    for (   unsigned i = 0;
            i < pNtHdr->FileHeader.NumberOfSections;
            i++, pSection++ )
    {
        DWORD sectionStart = pSection->VirtualAddress;
        DWORD sectionEnd = sectionStart
                    + max(pSection->SizeOfRawData, pSection->Misc.VirtualSize);

        // Is the address in this section???
        if ( (rva >= sectionStart) && (rva <= sectionEnd) )
        {
            // Yes, address is in the section.  Calculate section and offset,
            // and store in the "section" & "offset" params, which were
            // passed by reference.
            section = i+1;
            offset = rva - sectionStart;
            return TRUE;
        }
    }

    return FALSE;   // Should never get here!
}

//============================================================
// Walks the stack, and writes the results to the report file
//============================================================
void MSJExceptionHandler::IntelStackWalk( PCONTEXT pContext )
{
    _tprintf( "\nmanual stack frame walk begin:\n" );

    _tprintf( "Address   Frame     Logical addr  Module\n" );

    DWORD pc = pContext->Eip;
    PDWORD pFrame, pPrevFrame;

    pFrame = (PDWORD)pContext->Ebp;


    for(INDEX iDepth=0; iDepth<100; iDepth++)
    {
        char szModule[MAX_PATH] = "";
        DWORD section = 0, offset = 0;

        _tprintf( "%08X %08X ", pc, pFrame);

        DWORD symDisplacement = 0;  // Displacement of the input address,
                                    // relative to the start of the symbol

        GetLogicalAddress((PVOID)pc, szModule,sizeof(szModule),section,offset );

        _tprintf( "$adr: %s %04X:%08X", szModule, section, offset);


        _tprintf( "\n");

        pc = pFrame[1];

        pPrevFrame = pFrame;

        pFrame = (PDWORD)pFrame[0]; // proceed to next higher frame on stack

        if ( (DWORD)pFrame & 3 )    // Frame pointer must be aligned on a
            break;                  // DWORD boundary.  Bail if not so.

        if ( pFrame <= pPrevFrame )
            break;

        // Can two DWORDs be read from the supposed frame address?
        if ( IsBadWritePtr(pFrame, sizeof(PVOID)*2) )
            break;

    };
    _tprintf( "\nmanual stack frame walk end:\n" );
}

//============================================================================
// Helper function that writes to the report file, and allows the user to use
// printf style formating
//============================================================================
int __cdecl MSJExceptionHandler::_tprintf(const char * format, ...)
{
    char szBuff[1024];
    int retValue;
    DWORD cbWritten;
    va_list argptr;

    va_start( argptr, format );
    retValue = wvsprintfA( szBuff, format, argptr );
    va_end( argptr );

    WriteFile( m_hReportFile, szBuff, retValue * sizeof(char), &cbWritten, 0 );

    return retValue;
}

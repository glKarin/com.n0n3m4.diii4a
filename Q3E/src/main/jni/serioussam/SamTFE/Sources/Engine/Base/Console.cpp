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

#include <Engine/Base/Console.h>
#include <Engine/Base/Console_internal.h>

#include <Engine/Base/Timer.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/CTString.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Memory.h>

#include <Engine/Math/Functions.h>

__extern CConsole *_pConsole = NULL;

extern INDEX con_iLastLines;
__extern BOOL con_bCapture = FALSE;
__extern CTString con_strCapture = "";


// Constructor.
CConsole::CConsole(void)
{
  con_strBuffer  = NULL;
  con_strLineBuffer = NULL;
  con_atmLines = NULL;
  con_fLog = NULL;
}
// Destructor.
CConsole::~CConsole(void)
{
  ASSERT(this!=NULL);
  if (con_fLog!=NULL) {
    fclose(con_fLog);
    con_fLog = NULL;
  }
  if (con_strBuffer!=NULL) {
    FreeMemory(con_strBuffer);
  }
  if (con_strLineBuffer!=NULL) {
    FreeMemory(con_strLineBuffer);
  }
  if (con_atmLines!=NULL) {
    FreeMemory(con_atmLines);
  }
}

// Initialize the console.
void CConsole::Initialize(const CTFileName &fnmLog, INDEX ctCharsPerLine, INDEX ctLines)
{
  con_csConsole.cs_iIndex = -1;
  // synchronize access to console
  CTSingleLock slConsole(&con_csConsole, TRUE);

  // allocate the buffer
  con_ctCharsPerLine = ctCharsPerLine;
  con_ctLines        = ctLines;
  con_ctLinesPrinted = 0;
  // note: we add +1 for '\n' perline and +1 '\0' at the end of buffer
  con_strBuffer = (char *)AllocMemory((ctCharsPerLine+1)*ctLines+1);
  con_strLineBuffer = (char *)AllocMemory(ctCharsPerLine+2); // includes '\n' and '\0'
  con_atmLines = (TIME*)AllocMemory((ctLines+1)*sizeof(TIME));
  // make it empty
  for(INDEX iLine=0; iLine<ctLines; iLine++) {
    ClearLine(iLine);
  }
  // add string terminator at the end
  con_strBuffer[(ctCharsPerLine+1)*ctLines] = 0;

  // start printing in last line
  con_strLastLine = con_strBuffer+(ctCharsPerLine+1)*(ctLines-1);
  con_strCurrent = con_strLastLine;

  // open console file
  con_fLog = fopen(fnmLog, "wt");

  if (con_fLog==NULL) {
    FatalError("%s", strerror(errno));
  }

  // print one dummy line on start
  CPrintF("\n");
}

// Get current console buffer.
const char *CConsole::GetBuffer(void)
{
  ASSERT(this!=NULL);
  return con_strBuffer+(con_ctLines-con_ctLinesPrinted)*(con_ctCharsPerLine+1);
}
INDEX CConsole::GetBufferSize(void)
{
  ASSERT(this!=NULL);
  return (con_ctCharsPerLine+1)*con_ctLines+1;
}

// Discard timing info for last lines
void CConsole::DiscardLastLineTimes(void)
{
  ASSERT(this!=NULL);
  for(INDEX i=0; i<con_ctLines; i++) {
    con_atmLines[i] = -10000.0f;
  }
}

// Get number of lines newer than given time
INDEX CConsole::NumberOfLinesAfter(TIME tmLast)
{
  ASSERT(this!=NULL);
  // clamp console variable
  con_iLastLines = Clamp( con_iLastLines, (INDEX)0, (INDEX)CONSOLE_MAXLASTLINES);
  // find number of last console lines to be displayed on screen
  for(INDEX i=0; i<con_iLastLines; i++) {
    if (con_atmLines[con_ctLines-1-i]<tmLast) {
      return i;
    }
  }
  return con_iLastLines;
}

// Get one of last lines
CTString CConsole::GetLastLine(INDEX iLine)
{
  ASSERT(this!=NULL);
  if (iLine>=con_ctLinesPrinted) {
    return "";
  }
  ASSERT(iLine>=0 && iLine<con_ctLines);
  // get line number from the start of buffer
  iLine = con_ctLines-1-iLine;
  // copy line
  memcpy(con_strLineBuffer, con_strBuffer+iLine*(con_ctCharsPerLine+1), con_ctCharsPerLine);
  // put terminator at the end
  con_strLineBuffer[con_ctCharsPerLine] = 0;
  // return it
  return con_strLineBuffer;
}

// clear one given line in buffer
void CConsole::ClearLine(INDEX iLine)
{
  ASSERT(this!=NULL);
  // line must be valid
  ASSERT(iLine>=0 && iLine<con_ctLines);
  // get start of line
  char *pchLine = con_strBuffer+iLine*(con_ctCharsPerLine+1);
  // fill it with spaces
  memset(pchLine, ' ', con_ctCharsPerLine);
  // add return at the end of line
  pchLine[con_ctCharsPerLine] = '\n';
  con_atmLines[iLine] = _pTimer!=NULL?_pTimer->GetRealTimeTick():0.0f;
}

// scroll buffer up, discarding lines at the start
void CConsole::ScrollBufferUp(INDEX ctLines)
{
  ASSERT(this!=NULL);
  ASSERT(ctLines>0 && ctLines<con_ctLines);
  // move buffer up
  memmove(
    con_strBuffer, 
    con_strBuffer+ctLines*(con_ctCharsPerLine+1),
    (con_ctLines-ctLines)*(con_ctCharsPerLine+1));
  // move buffer up
  memmove(
    con_atmLines, 
    con_atmLines+ctLines,
    (con_ctLines-ctLines)*sizeof(TIME));
  con_ctLinesPrinted = ClampUp(con_ctLinesPrinted+1, con_ctLines);
  // clear lines at the end
  for(INDEX iLine=con_ctLines-ctLines; iLine<con_ctLines; iLine++) {
    ClearLine(iLine);
  }
}

// Add a line of text to console
void CConsole::PutString(const char *strString)
{
  ASSERT(this!=NULL);
  // synchronize access to console
  CTSingleLock slConsole(&con_csConsole, TRUE);

  // if in debug version, report it to output window
  _RPT1(_CRT_WARN, "%s", strString);
  // first append that string to the console output file
  if (con_fLog!=NULL) {
    fprintf(con_fLog, "%s", strString);
    fflush(con_fLog);
  }
  // if needed, append to capture string
  if (con_bCapture) {
    con_strCapture+=strString;
  }

  // if dedicated server
  extern BOOL _bDedicatedServer;
#if !defined(_DIII4A)
  if (_bDedicatedServer)
#endif
  {
    // print to output
    printf("%s", strString);
  }

  // start at the beginning of the string
  const char *pch=strString;
  // while not end of string
  while(*pch!=0) {
    // if line buffer full
    if (con_strCurrent==con_strLastLine+con_ctCharsPerLine) {
      // move buffer up
      ScrollBufferUp(1);
      // restart new line
      con_strCurrent=con_strLastLine;
    }
    // get char
    char c = *pch++;
    // skip cr
    if (c=='\r') {
      continue;
    }
    // if it is end of line
    if (c=='\n') {
      // move buffer up
      ScrollBufferUp(1);
      // restart new line
      con_strCurrent=con_strLastLine;
      continue;
    }
    // otherwise, add the char to buffer
    *con_strCurrent++ = c;
  }
}

// Close console log file buffers (call only when force-exiting!)
void CConsole::CloseLog(void)
{
  ASSERT(this!=NULL);
  if (con_fLog!=NULL) {
    fclose(con_fLog);
  }
  con_fLog = NULL;
}

// Print formated text to the main console.
__extern void CPrintF(const char *strFormat, ...)
{
  if (_pConsole==NULL) {
    return;
  }
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat);
  CTString strBuffer;
  strBuffer.VPrintF(strFormat, arg);
  va_end(arg);

  // print it to the main console
  _pConsole->PutString(strBuffer);
}

// Add a string of text to console
void CPutString(const char *strString)
{
  if (_pConsole==NULL) {
    return;
  }
  _pConsole->PutString(strString);
}

// Get number of lines newer than given time
INDEX CON_NumberOfLinesAfter(TIME tmLast)
{
  if (_pConsole==NULL) {
    return 0;
  }
  return _pConsole->NumberOfLinesAfter(tmLast);
}
// Get one of last lines
CTString CON_GetLastLine(INDEX iLine)
{
  if (_pConsole==NULL) {
    return "";
  }
  return _pConsole->GetLastLine(iLine);
}
// Discard timing info for last lines
void CON_DiscardLastLineTimes(void)
{
  if (_pConsole==NULL) {
    return;
  }
  _pConsole->DiscardLastLineTimes();
}
// Get current console buffer.
const char *CON_GetBuffer(void)
{
  if (_pConsole==NULL) {
    return "";
  }
  return _pConsole->GetBuffer();
}
INDEX CON_GetBufferSize(void)
{
  if (_pConsole==NULL) {
    return 1;
  }
  return _pConsole->GetBufferSize();
}

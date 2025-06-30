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

#ifndef SE_INCL_CONSOLE_INTERNAL_H
#define SE_INCL_CONSOLE_INTERNAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Synchronization.h>

// Object that takes care of game console.
#define CONSOLE_MAXLASTLINES 15 // how many last-line times to remember
class CConsole {
public:
// implementation:
  CTCriticalSection con_csConsole; // critical section for access to console data
  char *con_strBuffer;        // the allocated buffer
  char *con_strCurrent;       // next char to print
  char *con_strLastLine;      // start of last line in buffer
  char *con_strLineBuffer;    // one-line-sized buffer for temp usage
  INDEX con_ctCharsPerLine;   // number of characters per line
  INDEX con_ctLines;          // number of total lines
  TIME *con_atmLines;         // time stamp for each line
  INDEX con_ctLinesPrinted;   // number of lines printed
  FILE *con_fLog;   // log file for streaming the console to

  // clear line buffer
  void ClearLineBuffer();
  // clear one given line in buffer
  void ClearLine(INDEX iLine);
  // scroll buffer up, discarding lines at the start
  void ScrollBufferUp(INDEX ctBytesToFree);
  // Move last line times one place back and add new time
  void NewLastTime(TIME tmNew);

// interface:

  // Constructor.
  CConsole(void);
  // Destructor.
  ~CConsole(void);

  // Initialize the console.
  void Initialize(const CTFileName &fnmLog, INDEX ctCharsPerLine, INDEX ctLines);

  // Get current console buffer.
  ENGINE_API const char *GetBuffer(void);
  ENGINE_API INDEX GetBufferSize(void);
  // Add a string of text to console
  void PutString(const char *strString);
  // Close console log file buffers (call only when force-exiting!)
  void CloseLog(void);

  // Get number of lines newer than given time
  INDEX NumberOfLinesAfter(TIME tmLast);
  // Get one of last lines
  CTString GetLastLine(INDEX iLine);
  // Discard timing info for last lines
  void DiscardLastLineTimes(void);
};

ENGINE_API extern CConsole *_pConsole;


#endif  /* include-once check. */


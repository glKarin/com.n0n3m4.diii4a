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

#ifndef SE_INCL_CONSOLE_H
#define SE_INCL_CONSOLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Print formated text to the main console.
ENGINE_API extern void CPrintF(const char *strFormat, ...);
// Add a string of text to console
ENGINE_API void CPutString(const char *strString);

// Get number of lines newer than given time
ENGINE_API INDEX CON_NumberOfLinesAfter(TIME tmLast);
// Get one of last lines
ENGINE_API CTString CON_GetLastLine(INDEX iLine);
// Discard timing info for last lines
ENGINE_API void CON_DiscardLastLineTimes(void);
// Get current console buffer.
ENGINE_API const char *CON_GetBuffer(void);
ENGINE_API INDEX CON_GetBufferSize(void);


#endif  /* include-once check. */


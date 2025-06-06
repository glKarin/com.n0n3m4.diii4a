/* Copyright (c) 2022-2024 Dreamy Cecil
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

#ifndef XGIZMO_INCL_DATAINTERFACE_H
#define XGIZMO_INCL_DATAINTERFACE_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include "Render.h"

// Expandable array of strings
typedef CStaticStackArray<CTString> CStringStack;

// Interface of useful methods for data manipulation
namespace IData {

// Replace characters in a string
inline void ReplaceChar(char *str, char chOld, char chNew) {
  while (*str != '\0') {
    if (*str == chOld) {
      *str = chNew;
    }
    ++str;
  }
};

// Find character in a string
inline ULONG FindChar(const char *str, char ch, ULONG ulFrom = 0) {
  // Iteration position
  const char *pData = str + ulFrom;

  // Go until the string end
  while (*pData != '\0') {
    // If found the character
    if (*pData == ch) {
      // Return current position relative to the beginning
      return (pData - str);
    }
    ++pData;
  }

  // None found
  return -1;
};

// Count character in a string
inline ULONG CountChar(const char *str, char ch, ULONG ulFrom = 0) {
  ULONG ctReturn = 0;

  // Iteration position
  const char *pData = str + ulFrom;

  // Go until the string end
  while (*pData != '\0') {
    // Count found character
    if (*pData == ch) {
      ++ctReturn;
    }
    ++pData;
  }

  return ctReturn;
};

// Extract substring at a specific position
inline CTString ExtractSubstr(const char *str, ULONG ulFrom, ULONG ulChars) {
  // Limit character amount
  ulChars = ClampUp(ulChars, ULONG(strlen(str)) - ulFrom);

  // Copy substring into a null-terminated buffer
  char *strBuffer = new char[ulChars + 1];
  memcpy(strBuffer, str + ulFrom, ulChars);
  strBuffer[ulChars] = '\0';

  // Set a new string and discard the buffer
  CTString strSubstr(strBuffer);
  delete[] strBuffer;

  return strSubstr;
};

// Fill a list of strings separated by a single delimiter from text
inline void GetStrings(CStringStack &astrOut, const CTString &strIn, char chDelimiter) {
  if (strIn == "") return;

  const char *pch = strIn.str_String;
  ULONG ulSep = 0;

  while (ulSep != (ULONG)-1) {
    // Extract substring until a delimiter or the end (-1)
    ulSep = IData::FindChar(pch, chDelimiter);

    CTString &strNew = astrOut.Push();
    strNew = IData::ExtractSubstr(pch, 0, ulSep);

    pch += strNew.Length() + 1;
  }
};

// Get position of a decorated character in a decorated string (doesn't count color tags)
inline INDEX GetDecoratedChar(const CTString &str, INDEX iChar)
{
  const INDEX ct = str.Length();
  INDEX iNonTagChar = 0;

  for (INDEX iPos = 0; iPos < ct;) {
    // Parse a tag
    if (str[iPos] == '^') {
      // Count tag characters
      UBYTE *pubTag = (UBYTE *)str.str_String + iPos + 2;
      INDEX ctTag = -1; // Non-tag character

      switch (str[iPos + 1]) {
        case 'c': ctTag = 2 + FindZero(pubTag, 6); break;
        case 'a': ctTag = 2 + FindZero(pubTag, 2); break;
        case 'f': ctTag = 2 + FindZero(pubTag, 1); break;

        case 'b': case 'i': case 'r': case 'o':
        case 'C': case 'A': case 'F': case 'B': case 'I':
          ctTag = 2;
          break;

        case '^':
          ctTag = 1;
          break;
      }

      // Skip tag characters
      if (ctTag != -1) {
        iPos += ctTag;
        continue;
      }
    }

    // Reached needed position of the non-tag character
    if (iNonTagChar == iChar) return iPos;

    // Next non-tag character
    iPos++;
    iNonTagChar++;
  }

  // End of the string
  return ct;
};

// Return position of the last character that fits within some width in pixels
inline INDEX TextFitsInWidth(CDrawPort *pdp, PIX pixMaxWidth, const CTString &str) {
  // No width to fit in
  if (pixMaxWidth <= 0) return 0;

  // Go through characters themselves without decorations
  CTString strCheck = str.Undecorated();
  INDEX ctLastLength = strCheck.Length();

  // Keep decreasing amount of characters in a string until it fits
  while (IRender::GetTextWidth(pdp, strCheck) > pixMaxWidth) {
    strCheck.TrimRight(--ctLastLength);
  }

  return GetDecoratedChar(str, ctLastLength);
};

// Fixed function for reading lines from a stream without a delimiter at the last line
inline void GetLineFromStream_t(CTStream &strm, char *strBuffer, SLONG slBufferSize, char cDelimiter = '\n') {
  // Check parameters and that the stream can be read
  ASSERT(strBuffer != NULL && slBufferSize > 0 && strm.IsReadable());

  INDEX iLetters = 0;

  // Check if already at the end
  if (strm.AtEOF()) ThrowF_t((char *)TRANSV("EOF reached, file %s"), (const char *)strm.strm_strStreamDescription);

  FOREVER {
    char c;
    strm.Read_t(&c, 1);

    // [Cecil] Just skip instead of entering the block when it's not that
    if (c == '\r') continue;

    strBuffer[iLetters] = c;

    // Stop reading after the delimiter
    if (strBuffer[iLetters] == cDelimiter) {
      strBuffer[iLetters] = '\0';
      return;
    }

    // Go to the next destination letter
    iLetters++;

    // [Cecil] Cut off after actually setting the character
    if (strm.AtEOF()) {
      strBuffer[iLetters] = '\0';
      return;
    }

    // Check if reached the maximum length
    if (iLetters == slBufferSize) {
      return;
    }
  }
};

// Check if a string matches any line of the string mask
inline BOOL MatchesMask(const CTString &strString, CTString strMask) {
  CTString strLine;

  // If there's still something in the mask
  while (strMask != "") {
    // Get first line of the mask
    strLine = strMask;
    strLine.OnlyFirstLine();

    // Remove this line from the mask including the line break
    strMask.TrimLeft(strMask.Length() - strLine.Length() + 1);

    // Check if the string matches the line
    if (strString.Matches(strLine)) {
      return TRUE;
    }
  }

  // No matching lines found
  return FALSE;
};

// Print out specific time in details (years, days, hours, minutes, seconds)
inline void PrintDetailedTime(CTString &strOut, CTimerValue tvTime) {
  // Get precise seconds
  __int64 iSeconds = (tvTime.tv_llValue / _pTimer->tm_llPerformanceCounterFrequency);

  // Limit down to 0 seconds
  iSeconds = ClampDn(iSeconds, __int64(0));

  // Timeout
  if (iSeconds == 0) {
    strOut = "0s";
    return;
  }

  // Display seconds
  const ULONG ulSec = iSeconds % 60;

  if (ulSec > 0) {
    strOut.PrintF("%us", ulSec);
  }

  // Display minutes
  const ULONG ulMin = (iSeconds / 60) % 60;

  if (ulMin > 0) {
    strOut.PrintF("%umin %s", ulMin, (const char *)strOut);
  }

  // Display hours
  const ULONG ulHours = (iSeconds / 3600) % 24;

  if (ulHours > 0) {
    strOut.PrintF("%uh %s", ulHours, (const char *)strOut);
  }

  // Display days
  const ULONG ulDaysTotal = iSeconds / 3600 / 24;
  const ULONG ulDays = ulDaysTotal % 365;

  if (ulDays > 0) {
    strOut.PrintF("%ud %s", ulDays, (const char *)strOut);
  }

  const ULONG ulYearsTotal = (ulDaysTotal / 365);

  if (ulYearsTotal > 0) {
    strOut.PrintF("%uyrs %s", ulYearsTotal, (const char *)strOut);
  }
};

}; // namespace

#endif

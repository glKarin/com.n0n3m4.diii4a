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

#ifndef SE_INCL_CTSTRING_INL
#define SE_INCL_CTSTRING_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Memory.h>
#include <Engine/Base/Assert.h>

/*
 * Default constructor.
 */
ENGINE_API CTString::CTString(void)
{
  str_String = StringDuplicate("");
}

/*
 * Copy constructor.
 */
ENGINE_API CTString::CTString(const CTString &strOriginal)
{
  ASSERT(strOriginal.IsValid());

  // make string duplicate
  str_String = StringDuplicate(strOriginal.str_String);
}

/*
 * Constructor from character string.
 */
ENGINE_API CTString::CTString( const char *strCharString)
{
  ASSERT(strCharString!=NULL);

  // make string duplicate
  str_String = StringDuplicate( strCharString);
}

/* Constructor with formatting. */
ENGINE_API CTString::CTString(INDEX iDummy, const char *strFormat, ...)
{
  str_String = StringDuplicate("");
  va_list arg;
  va_start(arg, strFormat);
  VPrintF(strFormat, arg);
  va_end(arg);
}

/*
 * Destructor.
 */
ENGINE_API CTString::~CTString()
{
  // check that it is valid
  ASSERT(IsValid());
  // free memory
  FreeMemory(str_String);
}

/*
 * Clear the object.
 */
ENGINE_API void CTString::Clear(void)
{
  operator=("");
}

/*
 * Conversion into character string.
 */
ENGINE_API CTString::operator const char*() const
{
  ASSERT(IsValid());

  return str_String;
}

/*
 * Assignment.
 */
ENGINE_API CTString &CTString::operator=(const char *strCharString)
{
  ASSERT(IsValid());
  ASSERT(strCharString!=NULL);

  /* The other string must be copied _before_ this memory is freed, since it could be same
     pointer!
   */
  // make a copy of character string
  char *strCopy = StringDuplicate(strCharString);
  // empty this string
  FreeMemory(str_String);
  // assign it the copy of the character string
  str_String = strCopy;

  return *this;
}
ENGINE_API CTString &CTString::operator=(const CTString &strOther)
{
  ASSERT(IsValid());
  ASSERT(strOther.IsValid());

  /* The other string must be copied _before_ this memory is freed, since it could be same
     pointer!
   */
  // make a copy of character string
  char *strCopy = StringDuplicate(strOther.str_String);
  // empty this string
  FreeMemory(str_String);
  // assign it the copy of the character string
  str_String = strCopy;

  return *this;
}


#endif  /* include-once check. */


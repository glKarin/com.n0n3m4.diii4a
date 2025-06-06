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

#include <Engine/Base/Serial.h>

#include <Engine/Base/Stream.h>
#include <Engine/Base/CRCTable.h>

/*
 * Default constructor.
 */
CSerial::CSerial( void) : ser_ctUsed(0) // not used initially
{
}

/*
 * Destructor.
 */
CSerial::~CSerial( void)
{
  // must not be used at all
  ASSERT(ser_ctUsed == 0);    // look at _strLastCleared for possible name
}

/*
 * Clear the object.
 */
void CSerial::Clear(void)
{
  // mark that you have changed
  MarkChanged();

  // clear the filename
  ser_FileName.Clear();
}

/* Get the description of this object. */
CTString CSerial::GetDescription(void)
{
  return "<no description>";
}

/*
 * Load from file.
 */
void CSerial::Load_t(const CTFileName fnFileName)  // throw char *
{
  ASSERT(!IsUsed());
  // mark that you have changed
  MarkChanged();

  // open a stream
  CTFileStream istrFile;
  istrFile.Open_t(fnFileName);
  // read object from stream
  Read_t(&istrFile);
  // if still here (no exceptions raised)
  // remember filename
  ser_FileName = fnFileName;
}

/*
 * Reload from file.
 */
void CSerial::Reload(void)
{
  // mark that you have changed
  MarkChanged();

  CTFileName fnmOldName = ser_FileName;
  Clear();
  // try to
  //try {
    // open a stream
    CTFileStream istrFile;
    istrFile.Open_t(fnmOldName);
    // read object from stream
    Read_t(&istrFile);

  // if there is some error while reloading
  //} catch (const char *strError) {
    // quit the application with error explanation
    //FatalError(TRANS("Cannot reload file '%s':\n%s"), (const char *) (CTString&)fnmOldName, strError);
  //}

  // if still here (no exceptions raised)
  // remember filename
  ser_FileName = fnmOldName;
}

/*
 * Save to file.
 */
void CSerial::Save_t(const CTFileName fnFileName)  // throw char *
{
  // open a stream
  CTFileStream ostrFile;
  ostrFile.Create_t(fnFileName);
  // write object to stream
  Write_t(&ostrFile);
  // if still here (no exceptions raised)
  // remember new filename
  ser_FileName = fnFileName;
}

/*
 * Mark that object is used once more.
 */
void CSerial::MarkUsed(void)
{
  // use count must not have dropped below zero
  ASSERT(ser_ctUsed>=0);
  // increment use count
  ser_ctUsed++;
}

/*
 * Mark that object is used once less.
 */
void CSerial::MarkUnused(void)
{
  // decrement use count
  ser_ctUsed--;
  // use count must not have dropped below zero
  ASSERT(ser_ctUsed>=0);
}

/*
 * Check if object is used at least once.
 */
BOOL CSerial::IsUsed(void)
{
  // use count must not have dropped below zero
  ASSERT(ser_ctUsed>=0);

  return ser_ctUsed>0;
}
INDEX CSerial::GetUsedCount(void)
{
  return ser_ctUsed;
}

// gather the CRC of the file
void CSerial::AddToCRCTable(void)
{
  // add the file to CRC table
  CRCT_AddFile_t(ser_FileName);
}

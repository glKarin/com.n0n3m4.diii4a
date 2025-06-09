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

#ifndef SE_INCL_SERIAL_H
#define SE_INCL_SERIAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Changeable.h>
#include <Engine/Base/FileName.h>

 /*
 * Abstract base class for objects that can be saved and loaded.
 */
class ENGINE_API CSerial : public CChangeable {
public:
  INDEX ser_ctUsed;         // use count
  CTFileName ser_FileName;  // last file name loaded

public:
  /* Default constructor. */
  CSerial(void);
  /* Destructor. */
  virtual ~CSerial( void);

  /* Get the file name of this object. */
  inline const CTFileName &GetName(void) { return ser_FileName; };
  /* Get the description of this object. */
  virtual CTString GetDescription(void);
  /* Load from file. */
  void Load_t( const CTFileName fnFileName); // throw char *
  /* Save to file. */
  void Save_t( const CTFileName fnFileName); // throw char *
  /* Reload from file. */
  void Reload(void);
  /* Mark that object is used once more. */
  void MarkUsed(void);
  /* Mark that object is used once less. */
  void MarkUnused(void);
  /* Check if object is used at least once. */
  BOOL IsUsed(void);
  INDEX GetUsedCount(void);

  /* Clear the object. */
  virtual void Clear(void);
  /* Read from stream. */
  virtual void Read_t( CTStream *istrFile)=0; // throw char *
  /* Write to stream. */
  virtual void Write_t( CTStream *ostrFile)=0; // throw char *
  // check if this kind of objects is auto-freed
  virtual BOOL IsAutoFreed(void) { return TRUE; };
  // get amount of memory used by this object
  virtual SLONG GetUsedMemory(void) { return -1; };
  // gather the CRC of the file
  virtual void AddToCRCTable(void);
};


#endif  /* include-once check. */


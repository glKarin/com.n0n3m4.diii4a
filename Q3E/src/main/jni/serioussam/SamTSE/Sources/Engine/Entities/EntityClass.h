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

#ifndef SE_INCL_ENTITYCLASS_H
#define SE_INCL_ENTITYCLASS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Serial.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Entities/EntityProperties.h> // rcg10042001
#include <Engine/Base/DynamicLoader.h> // rcg10112001

/*
 *  General structure of an entity class.
 */
class ENGINE_API CEntityClass : public CSerial {
public:
  /* Obtain all components from component table. */
  void ObtainComponents_t(void);  // throw char *
  /* Release all components from component table. */
  void ReleaseComponents(void);
public:
  CTFileName ec_fnmClassDLL;              // filename of the DLL with the class
  CDynamicLoader *ec_hiClassDLL;
  class CDLLEntityClass *ec_pdecDLLClass; // pointer to DLL class in the DLL

  /* Default constructor. */
  CEntityClass(void);
  /* Constructor for a fixed class. */
  CEntityClass(class CDLLEntityClass *pdecDLLClass);
  /* Destructor. */
  ~CEntityClass(void);
  /* Clear the object. */
  void Clear(void);

  // reference counting functions
  void AddReference(void);
  void RemReference(void);

  /* Check that all properties have been properly declared. */
  void CheckClassProperties(void);

  /* Construct a new member of the class. */
  class CEntity *New(void);

  /* Get pointer to entity property from its name. */
  class CEntityProperty *PropertyForName(const CTString &strPropertyName);
  /* Get pointer to entity property from its packed identifier. */
  class CEntityProperty *PropertyForTypeAndID(ULONG ulType, ULONG ulID);
  /* Get event handler for given state and event code. */
  CEntity::pEventHandler HandlerForStateAndEvent(SLONG slState, SLONG slEvent);
  /* Get pointer to component from its type and identifier. */
  class CEntityComponent *ComponentForTypeAndID(
    enum EntityComponentType ectType, SLONG slID);
  /* Get pointer to component from the component. */
  inline class CEntityComponent *ComponentForPointer(void *pv);

  // overrides from CSerial
  /* Read from stream. */
  virtual void Read_t( CTStream *istr);  // throw char *
  /* Write to stream. */
  virtual void Write_t( CTStream *ostr); // throw char *

  // get amount of memory used by this object
  SLONG GetUsedMemory(void);
  // check if this kind of objects is auto-freed
  BOOL IsAutoFreed(void);
  // gather the CRC of the file
  void AddToCRCTable(void);
};


#endif  /* include-once check. */


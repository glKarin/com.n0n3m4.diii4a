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

#ifndef SE_INCL_ENTITYPROPERTIES_H
#define SE_INCL_ENTITYPROPERTIES_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/FileName.h>
#include <Engine/Entities/Entity.h>

/////////////////////////////////////////////////////////////////////
// Classes and macros for defining enums for entity properties

class CEntityPropertyEnumValue {
public:
  INDEX epev_iValue;      // value
  const char *epev_strName;     // descriptive name of the enum value (for editor)
};
class CEntityPropertyEnumType {
public:
  CEntityPropertyEnumValue *epet_aepevValues;     // array of values
  INDEX epet_ctValues;                            // number of values
  // convert value of an enum to its name
  ENGINE_API const char *NameForValue(INDEX iValue);
};

/* rcg10072001 */
#ifdef _MSC_VER
  #define ENUMEXTERN extern _declspec (dllexport)
#else
  #define ENUMEXTERN
#endif

#define EP_ENUMBEG(typename) ENUMEXTERN CEntityPropertyEnumValue typename##_values[] = {
#define EP_ENUMVALUE(value, name) {value, name}
#define EP_ENUMEND(typename) }; CEntityPropertyEnumType typename##_enum = { \
  typename##_values, sizeof(typename##_values)/sizeof(CEntityPropertyEnumValue ) }

// types that are used only for properties.
// range, edited visually as sphere around entity
typedef FLOAT RANGE;
// type of illumination (for lights that illuminate through polygons)
typedef INDEX ILLUMINATIONTYPE;
// type of light animation
typedef INDEX ANIMATION;
// CTString browsed like a file - doesn't cause dependencies
typedef CTString CTFileNameNoDep;
// CTString that gets translated to different languages
typedef CTString CTStringTrans;

/////////////////////////////////////////////////////////////////////
// Classes and macros for defining entity properties

#define EPROPF_HIDEINPERSPECTIVE    (1UL<<0)  // not visualized in perspective view (for ranges)

class ENGINE_API CEntityProperty {
public:
  enum PropertyType {
  /* IMPORTANT: Take care not to renumber the property types, since it would ruin loading
   * of previously saved data like levels etc.! Always give a new type new number, and
   * never change existing number, nor reuse old ones!
   * Update 'next free number' comment when adding new type, but not when removing some!
   */
    EPT_PARENT = 9998,          // used internaly in WED for setting entity's parents
    EPT_SPAWNFLAGS = 9999,      // used internaly in WED for spawn flags property
    EPT_ENUM = 1,               // enum xxx, INDEX with descriptions
    EPT_BOOL = 2,               // BOOL
    EPT_FLOAT = 3,              // FLOAT
    EPT_COLOR = 4,              // COLOR
    EPT_STRING = 5,             // CTString
    EPT_RANGE = 6,              // RANGE FLOAT edited as a sphere around the entity
    EPT_ENTITYPTR = 7,          // CEntityPointer
    EPT_FILENAME = 8,           // CTFileName
    EPT_INDEX = 9,              // INDEX
    EPT_ANIMATION = 10,         // ANIMATION, INDEX edited by animation names
    EPT_ILLUMINATIONTYPE = 11,  // ILLUMINATIONTYPE, INDEX edited by illumination names
    EPT_FLOATAABBOX3D = 12,     // FLOATaabbox3D
    EPT_ANGLE = 13,             // ANGLE, INDEX edited in degrees
    EPT_FLOAT3D = 14,           // FLOAT3D
    EPT_ANGLE3D = 15,           // ANGLE3D
    EPT_FLOATplane3D = 16,      // FLOATplane3D
    EPT_MODELOBJECT = 17,       // CModelObject
    EPT_PLACEMENT3D = 18,       // PLACEMENT3D
    EPT_ANIMOBJECT = 19,        // CAnimObject
    EPT_FILENAMENODEP = 20,     // CTString browsed like a file - doesn't cause dependencies
    EPT_SOUNDOBJECT = 21,       // CSoundObject
    EPT_STRINGTRANS = 22,       // CTString that gets translated to different languages
    EPT_FLOATQUAT3D = 23,       // FLOATquat3D
    EPT_FLOATMATRIX3D = 24,     // FLOATmatrix3D
    EPT_FLAGS = 25,             // flags - ULONG bitfield with names for each field
    EPT_MODELINSTANCE = 26,
    // next free number: 27
  } ep_eptType;               // type of property
  CEntityPropertyEnumType *ep_pepetEnumType;   // if the type is EPT_ENUM or EPT_FLAGS

  ULONG ep_ulID;         // property ID for this class
  SLONG ep_slOffset;     // offset of the property in the class
  const char *ep_strName;      // descriptive name of the property (for editor)
  ULONG ep_ulFlags;      // additional flags for the property
  char  ep_chShortcut;   // shortcut key for selecting the property in editor (0 for none)
  COLOR ep_colColor;     // property color, for various wed purposes (like target arrows)

  CEntityProperty(PropertyType eptType, CEntityPropertyEnumType *pepetEnumType,
    ULONG ulID, SLONG slOffset, const char *strName, char chShortcut, COLOR colColor, ULONG ulFlags)
    : ep_eptType         (eptType      )
    , ep_pepetEnumType   (pepetEnumType)
    , ep_ulID            (ulID         )
    , ep_slOffset        (slOffset     )
    , ep_strName         (strName      )
    , ep_ulFlags         (ulFlags      )
    , ep_chShortcut      (chShortcut   )
    , ep_colColor        (colColor     )
  {};
  CEntityProperty(void) {};
};

// macro for accessing property inside an entity
#define ENTITYPROPERTY(entityptr, offset, type) (*((type *)(((UBYTE *)entityptr)+offset)))

/////////////////////////////////////////////////////////////////////
// Classes and macros for defining entity event handler functions

// one entry in function table
class CEventHandlerEntry {
public:
  SLONG ehe_slState;                    // code of the automaton state
  SLONG ehe_slBaseState;                // code of the base state if overridden
  CEntity::pEventHandler ehe_pEventHandler;  // handler function
  const char *ehe_strName;              // symbolic name of handler for debugging
};

/////////////////////////////////////////////////////////////////////
// Classes and macros for defining entity components

enum EntityComponentType {  // DO NOT RENUMBER!
  ECT_TEXTURE   = 1,    // texture data
  ECT_MODEL     = 2,    // model data
  ECT_CLASS     = 3,    // entity class
  ECT_SOUND     = 4,    // sound data
};

class ENGINE_API CEntityComponent {
public:
  /* Obtain the component. */
  void Obtain_t(void);  // throw char *
  void ObtainWithCheck(void);
  // add component to crc table
  void AddToCRCTable(void);
  /* Release the component. */
  void Release(void);
public:
  EntityComponentType ec_ectType;       // type of component
  ULONG ec_slID;      // component ID in this class

  CTFileName ec_fnmComponent;  // component file name

  // this one is auto-filled by SCape on DLL initialization
  union { // pointer to component
    CTextureData *ec_ptdTexture;      // for textures
    CModelData   *ec_pmdModel;        // for models
    CSoundData   *ec_psdSound;        // for sounds
    CEntityClass *ec_pecEntityClass;  // for entity classes
    void *ec_pvPointer;   // for comparison needs
  };

  // NOTE: This uses special EFNM initialization for CTFileName class!
  CEntityComponent(EntityComponentType ectType,
    ULONG ulID, const char *strEFNMComponent)
    : ec_ectType(ectType)
    , ec_slID(ulID)
    , ec_fnmComponent(strEFNMComponent, 4) { ec_pvPointer = NULL; };
  CEntityComponent(void) { ec_slID = (ULONG) -1; ec_pvPointer = NULL; };
};

/////////////////////////////////////////////////////////////////////
// The class defining an entity class DLL itself

class ENGINE_API CDLLEntityClass {
public:
  CEntityProperty *dec_aepProperties; // array of properties
  INDEX dec_ctProperties;             // number of properties

  CEventHandlerEntry *dec_aeheHandlers;  // array of event handlers
  INDEX dec_ctHandlers;                  // number of handlers

  CEntityComponent *dec_aecComponents;// array of components
  INDEX dec_ctComponents;             // number of components

  const char *dec_strName;                  // descriptive name of the class
  const char *dec_strIconFileName;          // filename of texture or thumbnail
  INDEX dec_iID;                      // class ID

  CDLLEntityClass *dec_pdecBase;      // pointer to the base class

  CEntity *(*dec_New)(void);          // pointer to function that creates a member of class
  void (*dec_OnInitClass)(void);      // pointer to function for connecting DLL
  void (*dec_OnEndClass)(void);       // pointer to function for disconnecting DLL
  void (*dec_OnPrecache)(CDLLEntityClass *pdec, INDEX iUser);       // function for precaching data
  void (*dec_OnWorldInit)(CWorld *pwoWorld);    // function called on world initialization
  void (*dec_OnWorldTick)(CWorld *pwoWorld);    // function called for each tick
  void (*dec_OnWorldRender)(CWorld *pwoWorld);  // function called for each rendering
  void (*dec_OnWorldEnd)(CWorld *pwoWorld);     // function called on world cleanup

  /* Get pointer to entity property from its name. */
  class CEntityProperty *PropertyForName(const CTString &strPropertyName);
  /* Get pointer to entity property from its packed identifier. */
  class CEntityProperty *PropertyForTypeAndID(CEntityProperty::PropertyType eptType, ULONG ulID);
  /* Get event handler given state and event code. */
  CEntity::pEventHandler HandlerForStateAndEvent(SLONG slState, SLONG slEvent);
  /* Get event handler name for given state. */
  const char *HandlerNameForState(SLONG slState);
  /* Get derived class override for given state. */
  SLONG GetOverridenState(SLONG slState);
  /* Get pointer to component from its type and identifier. */
  class CEntityComponent *ComponentForTypeAndID(EntityComponentType ectType,
    SLONG slID);
  /* Get pointer to component from the component. */
  class CEntityComponent *ComponentForPointer(void *pv);
  // precache given component
  void PrecacheModel(SLONG slID);
  void PrecacheTexture(SLONG slID);
  void PrecacheSound(SLONG slID);
  void PrecacheClass(SLONG slID, INDEX iUser = -1);
};

/* rcg10062001 */
#if (defined _MSC_VER)
 #define DECLSPEC_DLLEXPORT _declspec (dllexport)
#else
 #define DECLSPEC_DLLEXPORT
#endif

// macro for defining entity class DLL structures
#define ENTITY_CLASSDEFINITION(classname, basename, descriptivename, iconfilename, id)\
  extern "C" DECLSPEC_DLLEXPORT CDLLEntityClass classname##_DLLClass; \
  CDLLEntityClass classname##_DLLClass = {                            \
    classname##_properties,                                           \
    classname##_propertiesct,                                         \
    classname##_handlers,                                             \
    classname##_handlersct,                                           \
    classname##_components,                                           \
    classname##_componentsct,                                         \
    descriptivename,                                                  \
    iconfilename,                                                     \
    id,                                                               \
    &basename##_DLLClass,                                             \
    &classname##_New,                                                 \
    &classname##_OnInitClass,                                         \
    &classname##_OnEndClass,                                          \
    &classname##_OnPrecache,                                          \
    &classname##_OnWorldInit,                                         \
    &classname##_OnWorldTick,                                         \
    &classname##_OnWorldRender,                                       \
    &classname##_OnWorldEnd                                           \
  }

#define ENTITY_CLASSDEFINITION_BASE(classname, id)                    \
  extern "C" DECLSPEC_DLLEXPORT CDLLEntityClass classname##_DLLClass; \
  CDLLEntityClass classname##_DLLClass = {                            \
    NULL,0, NULL,0, NULL,0, "", "", id,                               \
    NULL, NULL,NULL,NULL,NULL, NULL,NULL,NULL,NULL                    \
  }

inline ENGINE_API void ClearToDefault(FLOAT &f) { f = 0.0f; };
inline ENGINE_API void ClearToDefault(INDEX &i) { i = 0; };
#if (defined _MSC_VER)
inline ENGINE_API void ClearToDefault(BOOL &b) { b = FALSE; };
#endif
inline ENGINE_API void ClearToDefault(CEntityPointer &pen) { pen = NULL; };
inline ENGINE_API void ClearToDefault(CTString &str) { str = ""; };
inline ENGINE_API void ClearToDefault(FLOATplane3D &pl) { pl = FLOATplane3D(FLOAT3D(0,1,0), 0); };
inline ENGINE_API void ClearToDefault(FLOAT3D &v) { v = FLOAT3D(0,1,0); };
inline ENGINE_API void ClearToDefault(COLOR &c) { c = 0xFFFFFFFF; };
inline ENGINE_API void ClearToDefault(CModelData *&pmd) { pmd = NULL; };
inline ENGINE_API void ClearToDefault(CTextureData *&pmt) { pmt = NULL; };


#endif  /* include-once check. */


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

#include <Engine/Entities/EntityProperties.h>
#include <Engine/Entities/Precaching.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/CRCTable.h>
#include <Engine/Base/Console.h>
#include <Engine/World/World.h>
#include <Engine/Base/ReplaceFile.h>
#include <Engine/Sound/SoundObject.h>
#include <Engine/Math/Quaternion.h>

#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CEntityClass.h>
#include <Engine/Templates/StaticArray.cpp>

#define FILTER_ALL            "All files (*.*)\0*.*\0"
#define FILTER_END            "\0"

#define PROPERTY(offset, type) ENTITYPROPERTY(this, offset, type)

/////////////////////////////////////////////////////////////////////
// Property management functions


/*
 * Set all properties to default values.
 */
void CEntity::SetDefaultProperties(void)
{
  // no properties to set in base class
}

/*
 * Helpers for writing/reading entity pointers.
 */
void CEntity::ReadEntityPointer_t(CTStream *istrm, CEntityPointer &pen)
{
  CEntity *penPointed;
  // read index
  INDEX iPointedEntity;
  *istrm>>iPointedEntity;
  // if there is no entity pointed to
  if (iPointedEntity == -1) {
    // set NULL pointer
    penPointed = NULL;
  // if there is some entity
  } else {
    // get the entity in this world with that index
    extern BOOL _bReadEntitiesByID;
    if (_bReadEntitiesByID) {
      penPointed = en_pwoWorld->EntityFromID(iPointedEntity);
    } else {
      penPointed = en_pwoWorld->wo_cenAllEntities.Pointer(iPointedEntity);
    }
  }
  // return the entity pointer
  pen = penPointed;
}
void CEntity::WriteEntityPointer_t(CTStream *ostrm, CEntityPointer pen)
{
  // if there is no entity pointed to
  if (pen==NULL) {
    // write -1 index
    *ostrm<<(INDEX)-1;
  // if there is some entity
  } else {
    // the entity must be in the same world as this one
    ASSERT(pen->en_pwoWorld == en_pwoWorld);
    // write index of the entity in this world
    *ostrm<<(pen->en_ulID);
  }
}

/*
 * Read all properties from a stream.
 */
void CEntity::ReadProperties_t(CTStream &istrm) // throw char *
{
  istrm.ExpectID_t("PRPS");  // 'properties'
  CDLLEntityClass *pdecDLLClass = en_pecClass->ec_pdecDLLClass;
  INDEX ctProperties;
  // read number of properties (note that this doesn't have to be same as number
  // of properties in the class (class might have changed))
  istrm>>ctProperties;

  // for all saved properties
  for(INDEX iProperty=0; iProperty<ctProperties; iProperty++) {
    pdecDLLClass->dec_ctProperties;
    // read packed identifier
    ULONG ulIDAndType;
    istrm>>ulIDAndType;
    // unpack property ID and property type from the identifier
    ULONG ulID;
    CEntityProperty::PropertyType eptType;
    ulID = ulIDAndType>>8;
    eptType = (CEntityProperty::PropertyType )(ulIDAndType&0x000000FFUL);

    // get the property with that ID and type
    CEntityProperty *pepProperty = PropertyForTypeAndID(eptType, ulID);
    // if not found, but it is a string
    if (pepProperty == NULL && eptType==CEntityProperty::EPT_STRING) {
      // maybe that became translatable string try that
      pepProperty = PropertyForTypeAndID(CEntityProperty::EPT_STRINGTRANS, ulID);
      // NOTE: it is still loaded as string, without translation chunk,
      // we just find it in properties table as a translatable string.
    }

    // if it was not found
    if (pepProperty == NULL) {
      // depending on the property type
      switch (eptType) {
      // if it is BOOL
      case CEntityProperty::EPT_BOOL: {
        // skip BOOL
        BOOL bDummy;
        istrm>>(INDEX &)bDummy;
        break;
                                      }
      // if it is INDEX
      case CEntityProperty::EPT_INDEX:
      case CEntityProperty::EPT_ENUM:
      case CEntityProperty::EPT_FLAGS:
      case CEntityProperty::EPT_ANIMATION:
      case CEntityProperty::EPT_ILLUMINATIONTYPE:
      case CEntityProperty::EPT_COLOR:
      case CEntityProperty::EPT_ANGLE: {
        // skip INDEX
        INDEX iDummy;
        istrm>>iDummy;

      } break;
      // if it is FLOAT
      case CEntityProperty::EPT_FLOAT:
      case CEntityProperty::EPT_RANGE: {
        // skip FLOAT
        FLOAT fDummy;
        istrm>>fDummy;
                                       }
        break;
      // if it is STRING
      case CEntityProperty::EPT_STRING: {
        // skip STRING
        CTString strDummy;
        istrm>>strDummy;
        break;
                                        }
      // if it is STRINGTRANS
      case CEntityProperty::EPT_STRINGTRANS: {
        // skip STRINGTRANS
        istrm.ExpectID_t("DTRS");
        CTString strDummy;
        istrm>>strDummy;
        break;
                                        }
      // if it is FILENAME
      case CEntityProperty::EPT_FILENAME: {
        // skip FILENAME
        CTFileName fnmDummy;
        istrm>>fnmDummy;
        break;
                                          }
      // if it is FILENAMENODEP
      case CEntityProperty::EPT_FILENAMENODEP: {
        // skip FILENAMENODEP
        CTFileNameNoDep fnmDummy;
        istrm>>fnmDummy;
        break;
                                        }
      // if it is ENTITYPTR
      case CEntityProperty::EPT_ENTITYPTR: {
        // skip index
        INDEX iDummy;
        istrm>>iDummy;
                                           }
        break;
      // if it is FLOATAABBOX3D
      case CEntityProperty::EPT_FLOATAABBOX3D: {
        // skip FLOATAABBOX3D
        FLOATaabbox3D boxDummy;
        istrm.Read_t(&boxDummy, sizeof(FLOATaabbox3D));
                                               }
        break;
      // if it is FLOATMATRIX3D
      case CEntityProperty::EPT_FLOATMATRIX3D: {
        // skip FLOATMATRIX3D
        FLOATmatrix3D boxDummy;
        istrm.Read_t(&boxDummy, sizeof(FLOATmatrix3D));
                                               }
        break;
      // if it is EPT_FLOATQUAT3D
      case CEntityProperty::EPT_FLOATQUAT3D: {
        // skip EPT_FLOATQUAT3D
        FLOATquat3D qDummy;
        istrm.Read_t(&qDummy, sizeof(FLOATquat3D));
                                               }
        break;
      // if it is FLOAT3D
      case CEntityProperty::EPT_FLOAT3D: {
        // skip FLOAT3D
        FLOAT3D vDummy;
        istrm>>vDummy;
                                         }
        break;
      // if it is ANGLE3D
      case CEntityProperty::EPT_ANGLE3D: {
        // skip ANGLE3D
        ANGLE3D vDummy;
        istrm>>vDummy;
                                         }
        break;
      // if it is FLOATplane3D
      case CEntityProperty::EPT_FLOATplane3D: {
        // skip FLOATplane3D
        FLOATplane3D plDummy;
        istrm.Read_t(&plDummy, sizeof(plDummy));
                                              }
        break;
      // if it is MODELOBJECT
      case CEntityProperty::EPT_MODELOBJECT:
        // skip CModelObject
        SkipModelObject_t(istrm);
        break;
      // if it is MODELINSTANCE
      case CEntityProperty::EPT_MODELINSTANCE:
        SkipModelInstance_t(istrm);
        break;
      // if it is ANIMOBJECT
      case CEntityProperty::EPT_ANIMOBJECT:
        // skip CAnimObject
        SkipAnimObject_t(istrm);
        break;
      // if it is SOUNDOBJECT
      case CEntityProperty::EPT_SOUNDOBJECT:
        // skip CSoundObject
        SkipSoundObject_t(istrm);
        break;
      default:
        ASSERTALWAYS("Unknown property type");
      }

    // if it was found
    } else {

      // fixup for loading old strings as translatable strings
      CEntityProperty::PropertyType eptLoad = pepProperty->ep_eptType;
      if (eptType==CEntityProperty::EPT_STRING &&
          eptLoad==CEntityProperty::EPT_STRINGTRANS) {
        eptLoad = CEntityProperty::EPT_STRING;
      }

      // depending on the property type
      switch (eptLoad) {
      // if it is BOOL
      case CEntityProperty::EPT_BOOL:
        // read BOOL
        istrm>>(INDEX &)PROPERTY(pepProperty->ep_slOffset, BOOL);
        break;
      // if it is INDEX
      case CEntityProperty::EPT_INDEX:
      case CEntityProperty::EPT_ENUM:
      case CEntityProperty::EPT_FLAGS:
      case CEntityProperty::EPT_ANIMATION:
      case CEntityProperty::EPT_ILLUMINATIONTYPE:
      case CEntityProperty::EPT_COLOR:
      case CEntityProperty::EPT_ANGLE:
        // read INDEX
        istrm>>PROPERTY(pepProperty->ep_slOffset, INDEX);
        break;
      // if it is FLOAT
      case CEntityProperty::EPT_FLOAT:
      case CEntityProperty::EPT_RANGE:
        // read FLOAT
        istrm>>PROPERTY(pepProperty->ep_slOffset, FLOAT);
        break;
      // if it is STRING
      case CEntityProperty::EPT_STRING:
        // read STRING
        istrm>>PROPERTY(pepProperty->ep_slOffset, CTString);
        break;
      // if it is STRINGTRANS
      case CEntityProperty::EPT_STRINGTRANS:
        // read STRINGTRANS
        istrm.ExpectID_t("DTRS");
        istrm>>PROPERTY(pepProperty->ep_slOffset, CTString);
        break;
      // if it is FILENAME
      case CEntityProperty::EPT_FILENAME:
        // read FILENAME
        istrm>>PROPERTY(pepProperty->ep_slOffset, CTFileName);
        if (PROPERTY(pepProperty->ep_slOffset, CTFileName)=="") {
          break;
        }
        // try to replace file name if it doesn't exist
        for(;;)
        {
          if( !FileExists( PROPERTY(pepProperty->ep_slOffset, CTFileName)))
          {
            // if file was not found, ask for replacing file
            CTFileName fnReplacingFile;
            if( GetReplacingFile( PROPERTY(pepProperty->ep_slOffset, CTFileName),
                                  fnReplacingFile, FILTER_ALL FILTER_END))
            {
              // replacing file was provided
              PROPERTY(pepProperty->ep_slOffset, CTFileName) = fnReplacingFile;
            } else {
              ThrowF_t(TRANS("File '%s' does not exist"), (const char*)PROPERTY(pepProperty->ep_slOffset, CTFileName));
            }
          }
          else
          {
            break;
          }
        }
        break;
      // if it is FILENAMENODEP
      case CEntityProperty::EPT_FILENAMENODEP:
        // read FILENAMENODEP
        istrm>>PROPERTY(pepProperty->ep_slOffset, CTFileNameNoDep);
        break;
      // if it is ENTITYPTR
      case CEntityProperty::EPT_ENTITYPTR:
        // read the entity pointer
        ReadEntityPointer_t(&istrm, PROPERTY(pepProperty->ep_slOffset, CEntityPointer));
        break;
      // if it is FLOATAABBOX3D
      case CEntityProperty::EPT_FLOATAABBOX3D:
        // read FLOATAABBOX3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, FLOATaabbox3D), sizeof(FLOATaabbox3D));
        break;
      // if it is FLOATMATRIX3D
      case CEntityProperty::EPT_FLOATMATRIX3D:
        // read FLOATMATRIX3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, FLOATmatrix3D), sizeof(FLOATmatrix3D));
        break;
      // if it is FLOATQUAT3D
      case CEntityProperty::EPT_FLOATQUAT3D:
        // read FLOATQUAT3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, FLOATquat3D), sizeof(FLOATquat3D));
        break;
      // if it is FLOAT3D
      case CEntityProperty::EPT_FLOAT3D:
        // read FLOAT3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, FLOAT3D), sizeof(FLOAT3D));
        break;
      // if it is ANGLE3D
      case CEntityProperty::EPT_ANGLE3D:
        // read ANGLE3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, ANGLE3D), sizeof(ANGLE3D));
        break;
      // if it is FLOATplane3D
      case CEntityProperty::EPT_FLOATplane3D:
        // read FLOATplane3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, FLOATplane3D), sizeof(FLOATplane3D));
        break;
      // if it is MODELOBJECT
      case CEntityProperty::EPT_MODELOBJECT:
        // read CModelObject
        ReadModelObject_t(istrm, PROPERTY(pepProperty->ep_slOffset, CModelObject));
        break;
      // if it is MODELINSTANCE
      case CEntityProperty::EPT_MODELINSTANCE:
        // read CModelObject
        ReadModelInstance_t(istrm, PROPERTY(pepProperty->ep_slOffset, CModelInstance));
        break;
      // if it is ANIMOBJECT
      case CEntityProperty::EPT_ANIMOBJECT:
        // read CAnimObject
        ReadAnimObject_t(istrm, PROPERTY(pepProperty->ep_slOffset, CAnimObject));
        break;
      // if it is SOUNDOBJECT
      case CEntityProperty::EPT_SOUNDOBJECT:
        // read CSoundObject
        {
          CSoundObject &so = PROPERTY(pepProperty->ep_slOffset, CSoundObject);
          ReadSoundObject_t(istrm, so);
          so.so_penEntity = this;
        }
        break;
      // if it is CPlacement3D
      case CEntityProperty::EPT_PLACEMENT3D:
        // read CPlacement3D
        istrm.Read_t(&PROPERTY(pepProperty->ep_slOffset, CPlacement3D), sizeof(CPlacement3D));
        break;
      default:
        ASSERTALWAYS("Unknown property type");
      }
    }
  }
}

/*
 * Write all properties to a stream.
 */
void CEntity::WriteProperties_t(CTStream &ostrm) // throw char *
{
  INDEX ctProperties = 0;
  // for all classes in hierarchy of this entity
  {for(CDLLEntityClass *pdecDLLClass = en_pecClass->ec_pdecDLLClass;
      pdecDLLClass!=NULL;
      pdecDLLClass = pdecDLLClass->dec_pdecBase) {
    // count the properties
    ctProperties+=pdecDLLClass->dec_ctProperties;
  }}

  ostrm.WriteID_t("PRPS");  // 'properties'
  // write number of properties
  ostrm<<ctProperties;

  // for all classes in hierarchy of this entity
  {for(CDLLEntityClass *pdecDLLClass = en_pecClass->ec_pdecDLLClass;
      pdecDLLClass!=NULL;
      pdecDLLClass = pdecDLLClass->dec_pdecBase) {
    // for all properties
    for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
      CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];

      // pack property ID and property type together
      ULONG ulID = epProperty.ep_ulID;
      ULONG ulType = (ULONG)epProperty.ep_eptType;
      ULONG ulIDAndType = (ulID<<8)|ulType;
      // write the packed identifier
      ostrm<<ulIDAndType;

      // depending on the property type
      switch (epProperty.ep_eptType) {
      // if it is BOOL
      case CEntityProperty::EPT_BOOL:
        // write BOOL
        ostrm<<(INDEX &)PROPERTY(epProperty.ep_slOffset, BOOL);
        break;
      // if it is INDEX
      case CEntityProperty::EPT_INDEX:
      case CEntityProperty::EPT_ENUM:
      case CEntityProperty::EPT_FLAGS:
      case CEntityProperty::EPT_ANIMATION:
      case CEntityProperty::EPT_ILLUMINATIONTYPE:
      case CEntityProperty::EPT_COLOR:
      case CEntityProperty::EPT_ANGLE:
        // write INDEX
        ostrm<<PROPERTY(epProperty.ep_slOffset, INDEX);
        break;
      // if it is FLOAT
      case CEntityProperty::EPT_FLOAT:
      case CEntityProperty::EPT_RANGE:
        // write FLOAT
        ostrm<<PROPERTY(epProperty.ep_slOffset, FLOAT);
        break;
      // if it is STRING
      case CEntityProperty::EPT_STRING:
        // write STRING
        ostrm<<PROPERTY(epProperty.ep_slOffset, CTString);
        break;
      // if it is STRINGTRANS
      case CEntityProperty::EPT_STRINGTRANS:
        // write STRINGTRANS
        ostrm.WriteID_t("DTRS");
        ostrm<<PROPERTY(epProperty.ep_slOffset, CTString);
        break;
      // if it is FILENAME
      case CEntityProperty::EPT_FILENAME:
        // write FILENAME
        ostrm<<PROPERTY(epProperty.ep_slOffset, CTFileName);
        break;
      // if it is FILENAMENODEP
      case CEntityProperty::EPT_FILENAMENODEP:
        // write FILENAMENODEP
        ostrm<<PROPERTY(epProperty.ep_slOffset, CTFileNameNoDep);
        break;
      // if it is FLOATAABBOX3D
      case CEntityProperty::EPT_FLOATAABBOX3D:
        // write FLOATAABBOX3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, FLOATaabbox3D), sizeof(FLOATaabbox3D));
        break;
      // if it is FLOATMATRIX3D
      case CEntityProperty::EPT_FLOATMATRIX3D:
        // write FLOATMATRIX3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, FLOATmatrix3D), sizeof(FLOATmatrix3D));
        break;
      // if it is FLOATQUAT3D
      case CEntityProperty::EPT_FLOATQUAT3D:
        // write FLOATQUAT3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, FLOATquat3D), sizeof(FLOATquat3D));
        break;
      // if it is ANGLE3D
      case CEntityProperty::EPT_ANGLE3D:
        // write ANGLE3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, ANGLE3D), sizeof(ANGLE3D));
        break;
      // if it is FLOAT3D
      case CEntityProperty::EPT_FLOAT3D:
        // write FLOAT3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, FLOAT3D), sizeof(FLOAT3D));
        break;
      // if it is FLOATplane3D
      case CEntityProperty::EPT_FLOATplane3D:
        // write FLOATplane3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, FLOATplane3D), sizeof(FLOATplane3D));
        break;
      // if it is ENTITYPTR
      case CEntityProperty::EPT_ENTITYPTR:
        // write entity pointer
        WriteEntityPointer_t(&ostrm, PROPERTY(epProperty.ep_slOffset, CEntityPointer));
        break;
      // if it is MODELOBJECT
      case CEntityProperty::EPT_MODELOBJECT:
        // write CModelObject
        WriteModelObject_t(ostrm, PROPERTY(epProperty.ep_slOffset, CModelObject));
        break;
      // if it is MODELINSTANCE
      case CEntityProperty::EPT_MODELINSTANCE:
        // write CModelInstance
        WriteModelInstance_t(ostrm, PROPERTY(epProperty.ep_slOffset, CModelInstance));
        break;
      // if it is ANIMOBJECT
      case CEntityProperty::EPT_ANIMOBJECT:
        // write CAnimObject
        WriteAnimObject_t(ostrm, PROPERTY(epProperty.ep_slOffset, CAnimObject));
        break;
      // if it is SOUNDOBJECT
      case CEntityProperty::EPT_SOUNDOBJECT:
        // write CSoundObject
        WriteSoundObject_t(ostrm, PROPERTY(epProperty.ep_slOffset, CSoundObject));
        break;
      // if it is CPlacement3D
      case CEntityProperty::EPT_PLACEMENT3D:
        // write CPlacement3D
        ostrm.Write_t(&PROPERTY(epProperty.ep_slOffset, CPlacement3D), sizeof(CPlacement3D));
        break;
      default:
        ASSERTALWAYS("Unknown property type");
      }
    }
  }}
}

/////////////////////////////////////////////////////////////////////
// Component management functions

/*
 * Obtain the component.
 */
void CEntityComponent::Obtain_t(void)  // throw char *
{
  // if obtained
  if (ec_pvPointer!=NULL) {
    // just add to CRC
    AddToCRCTable();
    // do not obtain again
    return;
  }

  INDEX ctUsed = 0;
  // check the component type
  switch(ec_ectType) {
    // if texture
    case ECT_TEXTURE:
      // obtain texture data
      ec_ptdTexture = _pTextureStock->Obtain_t(ec_fnmComponent);
      ctUsed = ec_ptdTexture->GetUsedCount();
      break;
    // if model
    case ECT_MODEL:
      // obtain model data
      ec_pmdModel = _pModelStock->Obtain_t(ec_fnmComponent);
      ctUsed = ec_pmdModel->GetUsedCount();
      break;
    // if sound
    case ECT_SOUND:
      // obtain sound data
      ec_psdSound = _pSoundStock->Obtain_t(ec_fnmComponent);
      ctUsed = ec_psdSound->GetUsedCount();
      break;
    // if class
    case ECT_CLASS:
      // obtain entity class
      ec_pecEntityClass = _pEntityClassStock->Obtain_t(ec_fnmComponent);
      ctUsed = ec_pecEntityClass->GetUsedCount();
      break;
    // if something else
    default:
      // error
      ThrowF_t(TRANS("Component '%s'(%d) is of unknown type!"), (const char *) (CTString&)ec_fnmComponent, ec_slID);
  }

  // if not already loaded and should not be precaching now
  if( ctUsed<=1 && !_precache_bNowPrecaching) {
    // report warning
    CPrintF(TRANSV("Not precached: (0x%08X)'%s'\n"), this->ec_slID, (const char *) ec_fnmComponent);
  }
  //CPrintF(TRANSV("Precaching NOW: (0x%08X)'%s'\n"), this->ec_slID, (const char *) ec_fnmComponent);

  // add to CRC
  AddToCRCTable();
}
void CEntityComponent::ObtainWithCheck(void)
{
  try {
    Obtain_t();
  } catch (const char *strError) {
    FatalError("%s", strError);
  }
}

// add component to crc table
void CEntityComponent::AddToCRCTable(void)
{
  // if not obtained
  if (ec_pvPointer==NULL) {
    // do nothing
    return;
  }

  // add it
  switch(ec_ectType) {
    case ECT_TEXTURE: ec_ptdTexture->AddToCRCTable(); break;
    case ECT_MODEL:   ec_pmdModel->AddToCRCTable(); break;
    case ECT_SOUND:   ec_psdSound->AddToCRCTable(); break;
    case ECT_CLASS:   ec_pecEntityClass->AddToCRCTable(); break;
  }
}

/*
 * Release the component.
 */
void CEntityComponent::Release(void)
{
  // if the component is not obtained
  if (ec_pvPointer==NULL) {
    // don't release it
    return;
  }
  // check the component type
  switch(ec_ectType) {
    // if texture
    case ECT_TEXTURE:
      // release texture data
      _pTextureStock->Release(ec_ptdTexture);
      break;
    // if model
    case ECT_MODEL:
      // release model data
      _pModelStock->Release(ec_pmdModel);
      break;
    // if sound
    case ECT_SOUND:
      // release sound data
      _pSoundStock->Release(ec_psdSound);
      break;
    // if class
    case ECT_CLASS:
      // release entity class
      _pEntityClassStock->Release(ec_pecEntityClass);
      break;
    // if something else
    default:
      // error
      ThrowF_t(TRANS("Component '%s'(%d) is of unknown type!"), (const char *) (CTString&)ec_fnmComponent, ec_slID);
  }

  // released
  ec_pvPointer=NULL;
}

// these entity classes are bases, here stop all recursive searches
ENTITY_CLASSDEFINITION_BASE(CEntity, 32000);
ENTITY_CLASSDEFINITION_BASE(CLiveEntity, 32001);
ENTITY_CLASSDEFINITION_BASE(CRationalEntity, 32002);

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

/*
 * Model entity that can move.
 */

2
%{
#include <Engine/StdH.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Console.h>
#include <Engine/Models/ModelObject.h>
%}

class export CMovableModelEntity : CMovableEntity {
name      "MovableModelEntity";
thumbnail "";
properties:
  1 INDEX en_iCollisionBox = 0,   // current collision box for model entities
  2 INDEX en_iWantedCollisionBox = 0, // collision box to change to


components:


functions:

  // create a checksum value for sync-check
  export void ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck)
  {
    CMovableEntity::ChecksumForSync(ulCRC, iExtensiveSyncCheck);
    if (iExtensiveSyncCheck>0) {
      CRC_AddLONG(ulCRC, en_iCollisionBox);
      CRC_AddLONG(ulCRC, en_iWantedCollisionBox);
    }
  }
  // dump sync data to text file
  export void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
  {
    CMovableEntity::DumpSync_t(strm, iExtensiveSyncCheck);
    if (iExtensiveSyncCheck>0) {
      strm.FPrintF_t("collision box: %d(%d)\n", en_iCollisionBox, en_iWantedCollisionBox);
    }
  }

  // prepare parameters for moving in this tick
  export void PreMoving(void) // override from CMovableEntity
  {
    // if collision box should be changed
    if (en_iCollisionBox != en_iWantedCollisionBox) {
      // change if possible
      ChangeCollisionBoxIndexNow(en_iWantedCollisionBox);
    }

    CMovableEntity::PreMoving();
  }
  export void DoMoving(void)  // override from CMovableEntity
  {
    CMovableEntity::DoMoving();
  }

  /* Get current collision box index for this entity. */
  export INDEX GetCollisionBoxIndex(void)
  {
    return en_iCollisionBox;
  }

  /* Check if collision box touches any brush near. */
  export BOOL CheckForCollisionNow(INDEX iNewCollisionBox, CEntity **ppenObstacle)
  {
    // test if an entity can change to the new collision box without intersecting anything
    extern BOOL CanEntityChangeCollisionBox(CEntity *pen, INDEX iNewCollisionBox, CEntity **ppenObstacle);
    return !CanEntityChangeCollisionBox(this, en_iCollisionBox, ppenObstacle);
  }

  /* Change current collision box. */
  export BOOL ChangeCollisionBoxIndexNow(INDEX iNewCollisionBox, CEntity **ppenObstacle)
  {
    // if same as current
    if (iNewCollisionBox == en_iCollisionBox) {
      // do nothing
      return TRUE;
    }

    //CPrintF("changing box %d-%d...", en_iCollisionBox, iNewCollisionBox);
    // test if an entity can change to the new collision box without intersecting anything
    extern BOOL CanEntityChangeCollisionBox(CEntity *pen, INDEX iNewCollisionBox, CEntity **ppenObstacle);
    BOOL bCanChange = CanEntityChangeCollisionBox(this, iNewCollisionBox, ppenObstacle);
    
    // if it cannot
    if (!bCanChange) {
      // fail
      //CPrintF("failed\n");
      return FALSE;
    }

    // if this is ska model
    if(en_RenderType == CEntity::RT_SKAMODEL || en_RenderType == CEntity::RT_SKAEDITORMODEL) {
      if(GetModelInstance()!=NULL) {
        // change his colision box index
        GetModelInstance()->mi_iCurentBBox = iNewCollisionBox;
      }
    }
    // remember new collision box
    en_iCollisionBox = iNewCollisionBox;
    en_iWantedCollisionBox = iNewCollisionBox;

    // recalculate collision info
    ModelChangeNotify();

    //CPrintF("succeeded\n");
    return TRUE;
  }

  /* Change current collision box. */
  export BOOL ChangeCollisionBoxIndexNow(INDEX iNewCollisionBox)
  {
    CEntity *penDummy;
    return ChangeCollisionBoxIndexNow(iNewCollisionBox, &penDummy);
  }

  /* Force immediate changing of collision box. */
  export void ForceCollisionBoxIndexChange(INDEX iNewCollisionBox)
  {
    // if this is ska model
    if(en_RenderType == CEntity::RT_SKAMODEL || en_RenderType == CEntity::RT_SKAEDITORMODEL) {
      if(GetModelInstance()!=NULL) {
        // change his colision box index
        GetModelInstance()->mi_iCurentBBox = iNewCollisionBox;
      }
    }
    // remember new collision box
    en_iCollisionBox = iNewCollisionBox;
    en_iWantedCollisionBox = iNewCollisionBox;

    // recalculate collision info
    ModelChangeNotify();
  }

  /* Change current collision box next time when possible. */
  export void ChangeCollisionBoxIndexWhenPossible(INDEX iNewCollisionBox)
  {
    en_iWantedCollisionBox = iNewCollisionBox;
  }

  /* Copy entity from another entity of same class. */
  /*CMovableModelEntity &operator=(CMovableModelEntity &enOther)
  {
    CMovableEntity::operator=(enOther);
    return *this;
  } */
  /* Read from stream. */
  export void Read_t( CTStream *istr) // throw char *
  {
    CMovableEntity::Read_t(istr);
  }
  /* Write to stream. */
  export void Write_t( CTStream *ostr) // throw char *
  {
    CMovableEntity::Write_t(ostr);
  }

  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    return( sizeof(CMovableModelEntity) - sizeof(CMovableEntity) + CMovableEntity::GetUsedMemory());
  }


procedures:


  // must have at least one procedure per class
  Dummy() {};

  // wait here until scheduled animation starts
  WaitUntilScheduledAnimStarts(EVoid)
  {
    ASSERT(en_RenderType == CEntity::RT_MODEL || en_RenderType == CEntity::RT_EDITORMODEL);
    FLOAT fToWait = GetModelObject()->ao_tmAnimStart-_pTimer->CurrentTick();
    if( fToWait>0)
    {
      autowait(fToWait+_pTimer->TickQuantum);
    }
    return EReturn();
  }
};

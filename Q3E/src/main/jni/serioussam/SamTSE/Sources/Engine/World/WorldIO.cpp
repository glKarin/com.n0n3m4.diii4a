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

#include <Engine/StdH.h>

#include <Engine/Base/Stream.h>
#include <Engine/Math/Float.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/Entities/EntityClass.h>
#include <Engine/Entities/Precaching.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Terrain/TerrainArchive.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Network/Network.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Terrain/Terrain.h>

#define WORLDSTATEVERSION_NOCLASSCONTAINER 9
#define WORLDSTATEVERSION_MULTITEXTURING 8
#define WORLDSTATEVERSION_SHADOWSPERMIP 7
#define WORLDSTATEVERSION_CURRENT WORLDSTATEVERSION_NOCLASSCONTAINER

#ifdef PLATFORM_WIN32
extern CWorld *_pwoCurrentLoading = NULL;  // world that is currently loading
extern BOOL _bReadEntitiesByID = FALSE;
#else
CWorld *_pwoCurrentLoading = NULL;  // world that is currently loading
BOOL _bReadEntitiesByID = FALSE;
#endif

extern BOOL _bPortalSectorLinksPreLoaded;
extern BOOL _bEntitySectorLinksPreLoaded;
extern BOOL _bFileReplacingApplied;

/*
 * Save entire world (both brushes  current state).
 */
void CWorld::Save_t(const CTFileName &fnmWorld) // throw char *
{
  // create the file
  CTFileStream strmFile;
  strmFile.Create_t(fnmWorld);

  // save engine build
  _pNetwork->WriteVersion_t(strmFile);

  // write the world to the file
  Write_t(&strmFile);
}

/*
 * Load entire world (both brushes and current state).
 */
void CWorld::Load_t(const CTFileName &fnmWorld) // throw char *
{
  // remember the file
  wo_fnmFileName = fnmWorld;
  // open the file
  CTFileStream strmFile;
  strmFile.Open_t(fnmWorld);

  // check engine build allowing reinit
  BOOL bNeedsReinit;
  _pNetwork->CheckVersion_t(strmFile, TRUE, bNeedsReinit);

  // read the world from the file
  Read_t(&strmFile);

  // close the file
  strmFile.Close();

  // if reinit is needed
  if (bNeedsReinit) {
    // reinitialize
    SetProgressDescription(TRANS("converting from old version"));
    CallProgressHook_t(0.0f);
    ReinitializeEntities();
    CallProgressHook_t(1.0f);
    // reinitialize
    SetProgressDescription(TRANS("saving converted file"));
    CallProgressHook_t(0.0f);
    Save_t(fnmWorld);
    CallProgressHook_t(1.0f);
  }
}

/*
 * Write entire world (both brushes and current state).
 */
void CWorld::Write_t(CTStream *postrm) // throw char *
{
  // need high FPU precision
  CSetFPUPrecision FPUPrecision(FPT_53BIT);

  // delete all predictor entities before saving
  UnmarkForPrediction();
  DeletePredictors();

  // lock all arrays and containers
  LockAll();

  postrm->WriteID_t("WRLD"); // 'world'
  // write the world brushes to the file
  WriteBrushes_t(postrm);
  // write current world state to the file
  WriteState_t(postrm);
  postrm->WriteID_t("WEND"); // 'world end'

  // unlock all arrays and containers
  UnlockAll();
}


/*
 * Read entire world (both brushes and current state).
 */
void CWorld::Read_t(CTStream *pistrm) // throw char *
{
  _pfWorldEditingProfile.IncrementAveragingCounter();
  _bFileReplacingApplied = FALSE;

  // need high FPU precision
  CSetFPUPrecision FPUPrecision(FPT_53BIT);

  // clear eventual old data in the world
  Clear();

  // lock all arrays and containers
  LockAll();

  pistrm->ExpectID_t("WRLD"); // 'world'
  // read the world brushes from the file
  ReadBrushes_t(pistrm);
  // read current world state from the file
  ReadState_t(pistrm);
  pistrm->ExpectID_t("WEND"); // 'world end'

  // unlock all arrays and containers
  UnlockAll();

  if( _bFileReplacingApplied)
    WarningMessage("Some of files needed to load world have been replaced while loading");
}

/*
 * Load just world brushes from a file with entire world information.
 */
void CWorld::LoadBrushes_t(const CTFileName &fnmWorld) // throw char *
{
  // remember the file
  wo_fnmFileName = fnmWorld;
  // open the file
  CTFileStream strmFile;
  strmFile.Open_t(fnmWorld);

  // check engine build disallowing reinit
  BOOL bNeedsReinit;
  _pNetwork->CheckVersion_t(strmFile, FALSE, bNeedsReinit);
  ASSERT(!bNeedsReinit);

  strmFile.ExpectID_t("WRLD"); // 'world'
  // read the world brushes from the file
  ReadBrushes_t(&strmFile);
}

/*
 * Read world brushes from stream.
 */
void CWorld::ReadBrushes_t( CTStream *istrm)// throw char *
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_READBRUSHES);

  // must be in 53bit mode when managing brushes
  CSetFPUPrecision FPUPrecision(FPT_53BIT);
  
  ReadInfo_t(istrm, FALSE);

  SetProgressDescription(TRANS("loading world textures"));
  CallProgressHook_t(0.0f);
  // read the brushes from the file
  _pwoCurrentLoading = this;
  istrm->DictionaryReadBegin_t();
  istrm->DictionaryPreload_t();
  CallProgressHook_t(1.0f);
  SetProgressDescription(TRANS("loading brushes"));
  CallProgressHook_t(0.0f);
  wo_baBrushes.Read_t(istrm);
  CallProgressHook_t(1.0f);

  // if there are some terrais in world
  if(istrm->PeekID_t()==CChunkID("TRAR")) { // 'terrain archive'
    SetProgressDescription(TRANS("loading terrains"));
    CallProgressHook_t(0.0f);
    wo_taTerrains.Read_t(istrm);
    CallProgressHook_t(1.0f);
  }

  istrm->DictionaryReadEnd_t();
  _pwoCurrentLoading = NULL;
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READBRUSHES);
}

/*
 * Write world brushes to stream.
 */
void CWorld::WriteBrushes_t( CTStream *ostrm) // throw char *
{
  WriteInfo_t(ostrm);

  // write the brushes to the file
  ostrm->DictionaryWriteBegin_t(CTString(""), 0);
  wo_baBrushes.Write_t(ostrm);
  wo_taTerrains.Write_t(ostrm);
  ostrm->DictionaryWriteEnd_t();
}

/*
 * Read current world state from stream.
 */
void CWorld::ReadState_t( CTStream *istr) // throw char *
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_READSTATE);
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  CTmpPrecachingNow tpn;
  _bReadEntitiesByID = FALSE;

  SetProgressDescription(TRANS("loading models"));
  CallProgressHook_t(0.0f);
  wo_slStateDictionaryOffset = istr->DictionaryReadBegin_t();
  istr->DictionaryPreload_t();
  CallProgressHook_t(1.0f);
  istr->ExpectID_t("WSTA"); // world state

  // read the version number
  INDEX iSavedVersion;
  (*istr)>>iSavedVersion;
  // if the version number is the newest
  if(iSavedVersion==WORLDSTATEVERSION_CURRENT) {
    // read current version
    ReadState_new_t(istr);

  // if the version number is not the newest
  } else {

    // if the version can be converted
    if(iSavedVersion==WORLDSTATEVERSION_CURRENT-1) {
      // show warning
//      WarningMessage(
//        "World state version was %d (old).\n"
//        "Auto-converting to version %d.",
//        iSavedVersion, WORLDSTATEVERSION_CURRENT);
      // read previous version
      ReadState_old_t(istr);
    // if the version can be converted
    } else if(iSavedVersion==WORLDSTATEVERSION_CURRENT-2) {
      // show warning
      WarningMessage(
        TRANS("World state version was %d (very old).\n"
        "Auto-converting to version %d."),
        iSavedVersion, WORLDSTATEVERSION_CURRENT);
      // read previous version
      ReadState_veryold_t(istr);
    } else {
      // report error
      ThrowF_t(
        TRANS("World state version is %d (unsupported).\n"
        "Current supported version is %d."),
        iSavedVersion, WORLDSTATEVERSION_CURRENT);
    }
  }
  istr->DictionaryReadEnd_t();

  SetProgressDescription(TRANS("precaching"));
  CallProgressHook_t(0.0f);
  // precache data needed by entities
  if( gam_iPrecachePolicy==PRECACHE_SMART) {
    PrecacheEntities_t();
  }
  CallProgressHook_t(1.0f);

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_READSTATE);
}

/*
 * Read current world state from stream -- preprevious version.
 */
void CWorld::ReadState_veryold_t( CTStream *istr) // throw char *
{
  // read the world description
  (*istr)>>wo_strDescription;
  // read the world background color
  (*istr)>>wo_colBackground;

  INDEX ienBackgroundViewer = -1;
  // if background viewer entity is saved here
  if (istr->PeekID_t() == CChunkID("BGVW")) {
    // read background viewer entity index
    istr->ExpectID_t("BGVW"); // background viewer
    (*istr)>>ienBackgroundViewer;
  } else {
    BOOL bUseBackgroundTexture;
    (*istr)>>(SLONG &)bUseBackgroundTexture;

    // read the world background texture name
    CTString strBackgroundTexture;
    (*istr)>>strBackgroundTexture;   // saved as string to bypass dependency catcher
    // skip the 6 dummy texture names used for dependencies
    CTFileName fnmDummy;
    (*istr)>>fnmDummy>>fnmDummy>>fnmDummy
           >>fnmDummy>>fnmDummy>>fnmDummy;
  }

  // if backdrop image data is saved here
  if (istr->PeekID_t() == CChunkID("BRDP")) {
    // read backdrop image data
    istr->ExpectID_t("BRDP"); // backdrop
    (*istr)>>wo_strBackdropUp;
    (*istr)>>wo_strBackdropFt;
    (*istr)>>wo_strBackdropRt;
    (*istr)>>wo_fUpW>>wo_fUpL>>wo_fUpCX>>wo_fUpCZ;
    (*istr)>>wo_fFtW>>wo_fFtH>>wo_fFtCX>>wo_fFtCY;
    (*istr)>>wo_fRtL>>wo_fRtH>>wo_fRtCZ>>wo_fRtCY;
  }

  // if backdrop object name is saved here
  if (istr->PeekID_t() == CChunkID("BDRO")) {
    // read backdrop object name
    istr->ExpectID_t("BDRO"); // backdrop object
    (*istr)>>wo_strBackdropObject;
  }

  // if viewer position should be loaded
  if (istr->PeekID_t() == CChunkID("VWPS")) {
    istr->ExpectID_t("VWPS"); // viewer position
    (*istr)>>wo_plFocus;
    (*istr)>>wo_fTargetDistance;
  }

  istr->ExpectID_t("SHAN"); // shadow animations
  // for all anim objects
  {for(INDEX iao=0; iao<256; iao++) {
    // skip animation object
    CAnimObject ao;
    ao.Read_t(istr);
  }}

  istr->ExpectID_t("ECLs"); // entity classes
  // read number of entity classes
  INDEX ctEntityClasses;
  (*istr)>>ctEntityClasses;

  CStaticArray<CTFileName> cecClasses;
  cecClasses.New(ctEntityClasses);
  // for each entity class
  {for(INDEX iEntityClass=0; iEntityClass<ctEntityClasses; iEntityClass++) {
    // load filename
    (*istr)>>cecClasses[iEntityClass];
  }}

  /* NOTE: Entities must be loaded in two passes, since all entities must be created
   * before any entity pointer properties can be loaded.
   */
  istr->ExpectID_t("ENTs"); // entities
  // read number of entities
  INDEX ctEntities;
  (*istr)>>ctEntities;

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    INDEX iEntityClass;
    CPlacement3D plPlacement;
    // read entity class index and entity placement
    (*istr)>>iEntityClass>>plPlacement;
    // create an entity of that class
    /* CEntity *penNew = */ CreateEntity_t(plPlacement, cecClasses[iEntityClass]);
  }}

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    // deserialize entity from stream
    wo_cenAllEntities[iEntity].Read_t(istr);
  }}

  // after all entities have been read, set the background viewer entity
  if (ienBackgroundViewer==-1) {
    SetBackgroundViewer(NULL);
  } else {
    SetBackgroundViewer(wo_cenAllEntities.Pointer(ienBackgroundViewer));
  }

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    CEntity &en = wo_cenAllEntities[iEntity];
    // if the entity is destroyed
    if (en.en_ulFlags&ENF_DELETED) {
      // remove the reference made by itself
      ASSERT(en.en_ctReferences>1); // must be referenced by someone else too
      en.RemReference();
      wo_cenEntities.Remove(&en);
    }
  }}

  // after all entities have been read and brushes are connected to entities,
  // calculate bounding boxes of all brushes
  wo_baBrushes.CalculateBoundingBoxes();
  // after all bounding boxes and BSP trees are created,
  // create links between portals and sectors on their other side
  wo_baBrushes.LinkPortalsAndSectors();
  wo_bPortalLinksUpToDate = TRUE;
  // create links between sectors and non-zoning entities in sectors
  LinkEntitiesToSectors();
}

/*
 * Read current world state from stream -- previous version.
 */
void CWorld::ReadState_old_t( CTStream *istr) // throw char *
{
  // read the world description
  (*istr)>>wo_strDescription;
  // read the world background color
  (*istr)>>wo_colBackground;

  INDEX ienBackgroundViewer = -1;
  // read background viewer entity index
  istr->ExpectID_t("BGVW"); // background viewer
  (*istr)>>ienBackgroundViewer;

  // read backdrop image data
  istr->ExpectID_t("BRDP"); // backdrop
  (*istr)>>wo_strBackdropUp;
  (*istr)>>wo_strBackdropFt;
  (*istr)>>wo_strBackdropRt;
  (*istr)>>wo_fUpW>>wo_fUpL>>wo_fUpCX>>wo_fUpCZ;
  (*istr)>>wo_fFtW>>wo_fFtH>>wo_fFtCX>>wo_fFtCY;
  (*istr)>>wo_fRtL>>wo_fRtH>>wo_fRtCZ>>wo_fRtCY;

  // read backdrop object name
  istr->ExpectID_t("BDRO"); // backdrop object
  (*istr)>>wo_strBackdropObject;

  istr->ExpectID_t("VWPS"); // viewer position
  (*istr)>>wo_plFocus;
  (*istr)>>wo_fTargetDistance;

  // if thumbnail saving position should be loaded
  if (istr->PeekID_t() == CChunkID("TBPS")) {
    istr->ExpectID_t("TBPS"); // thumbnail position
    (*istr)>>wo_plThumbnailFocus;
    (*istr)>>wo_fThumbnailTargetDistance;
  }

  istr->ExpectID_t("SHAN"); // shadow animations
  // for all anim objects
  {for(INDEX iao=0; iao<256; iao++) {
    // skip animation object
    CAnimObject ao;
    ao.Read_t(istr);
  }}

  istr->ExpectID_t("ECLs"); // entity classes
  // read number of entity classes
  INDEX ctEntityClasses;
  (*istr)>>ctEntityClasses;

  CStaticArray<CTFileName> cecClasses;
  cecClasses.New(ctEntityClasses);
  // for each entity class
  {for(INDEX iEntityClass=0; iEntityClass<ctEntityClasses; iEntityClass++) {
    // load filename
    (*istr)>>cecClasses[iEntityClass];
  }}

  /* NOTE: Entities must be loaded in two passes, since all entities must be created
   * before any entity pointer properties can be loaded.
   */
  istr->ExpectID_t("ENTs"); // entities
  // read number of entities
  INDEX ctEntities;
  (*istr)>>ctEntities;

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    INDEX iEntityClass;
    CPlacement3D plPlacement;
    // read entity class index and entity placement
    (*istr)>>iEntityClass>>plPlacement;
    // create an entity of that class
    /* CEntity *penNew = */ CreateEntity_t(plPlacement, cecClasses[iEntityClass]);
  }}

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    // deserialize entity from stream
    wo_cenAllEntities[iEntity].Read_t(istr);
  }}

  // after all entities have been read, set the background viewer entity
  if (ienBackgroundViewer==-1) {
    SetBackgroundViewer(NULL);
  } else {
    SetBackgroundViewer(wo_cenAllEntities.Pointer(ienBackgroundViewer));
  }

  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    CEntity &en = wo_cenAllEntities[iEntity];
    // if the entity is destroyed
    if (en.en_ulFlags&ENF_DELETED) {
      // remove the reference made by itself
      ASSERT(en.en_ctReferences>1); // must be referenced by someone else too
      en.RemReference();
      wo_cenEntities.Remove(&en);
    }
  }}

  // after all entities have been read and brushes are connected to entities,
  // calculate bounding boxes of all brushes
  wo_baBrushes.CalculateBoundingBoxes();
  // after all bounding boxes and BSP trees are created,
  // create links between portals and sectors on their other side if needed
  if (!_bPortalSectorLinksPreLoaded) {
    wo_baBrushes.LinkPortalsAndSectors();
  }
  wo_bPortalLinksUpToDate = TRUE;
  _bPortalSectorLinksPreLoaded = FALSE;
  // create links between sectors and non-zoning entities in sectors
  LinkEntitiesToSectors();
}

/*
 * Read current world state from stream -- current version.
 */
void CWorld::ReadState_new_t( CTStream *istr) // throw char *
{
  // read the world info
  ReadInfo_t(istr, TRUE);

  // read the world background color
  (*istr)>>wo_colBackground;

  // read entity ID counter
  if (istr->PeekID_t()==CChunkID("NFID")) {
    istr->ExpectID_t("NFID");
    (*istr)>>wo_ulNextEntityID;
  } else {
    wo_ulNextEntityID = 1;
  }

  INDEX ienBackgroundViewer = -1;
  // read background viewer entity index
  istr->ExpectID_t("BGVW"); // background viewer
  (*istr)>>ienBackgroundViewer;

  // read backdrop image data
  istr->ExpectID_t("BRDP"); // backdrop
  (*istr)>>wo_strBackdropUp;
  (*istr)>>wo_strBackdropFt;
  (*istr)>>wo_strBackdropRt;
  (*istr)>>wo_fUpW>>wo_fUpL>>wo_fUpCX>>wo_fUpCZ;
  (*istr)>>wo_fFtW>>wo_fFtH>>wo_fFtCX>>wo_fFtCY;
  (*istr)>>wo_fRtL>>wo_fRtH>>wo_fRtCZ>>wo_fRtCY;

  // read backdrop object name
  istr->ExpectID_t("BDRO"); // backdrop object
  (*istr)>>wo_strBackdropObject;

  istr->ExpectID_t("VWPS"); // viewer position
  (*istr)>>wo_plFocus;
  (*istr)>>wo_fTargetDistance;

  // if thumbnail saving position should be loaded
  if (istr->PeekID_t() == CChunkID("TBPS")) {
    istr->ExpectID_t("TBPS"); // thumbnail position
    (*istr)>>wo_plThumbnailFocus;
    (*istr)>>wo_fThumbnailTargetDistance;
  }

  /* NOTE: Entities must be loaded in two passes, since all entities must be created
   * before any entity pointer properties can be loaded.
   */
  if (istr->PeekID_t()== CChunkID("ENTs")) {
    istr->ExpectID_t("ENTs"); // entities
  } else {
    istr->ExpectID_t("ENs2"); // entities v2
    _bReadEntitiesByID = TRUE;
  }
  // read number of entities
  INDEX ctEntities;
  (*istr)>>ctEntities;

  SetProgressDescription(TRANS("creating entities"));
  CallProgressHook_t(0.0f);
  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    // read entity id if needed
    ULONG ulID = 0;
    if (_bReadEntitiesByID) {
      (*istr)>>ulID;
    }
    // read entity class and entity placement
    CTFileName fnmClass;
    CPlacement3D plPlacement;
    (*istr)>>fnmClass>>plPlacement;
    // create an entity of that class
    CEntity *penNew = CreateEntity_t(plPlacement, fnmClass);
    // adjust id if needed
    if (_bReadEntitiesByID) {
      wo_ulNextEntityID--;
      penNew->en_ulID = ulID;
    }
    CallProgressHook_t(FLOAT(iEntity)/ctEntities);
  }}
  CallProgressHook_t(1.0f);

  SetProgressDescription(TRANS("loading entities"));
  CallProgressHook_t(0.0f);
  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    // deserialize entity from stream
    wo_cenAllEntities[iEntity].Read_t(istr);
    CallProgressHook_t(FLOAT(iEntity)/ctEntities);
  }}
  CallProgressHook_t(1.0f);

  // after all entities have been read, set the background viewer entity
  if (ienBackgroundViewer==-1) {
    SetBackgroundViewer(NULL);
  } else {
    CEntity *penBcg;
    if (_bReadEntitiesByID) {
      penBcg = EntityFromID(ienBackgroundViewer);
    } else {
      penBcg = wo_cenAllEntities.Pointer(ienBackgroundViewer);
    }
    SetBackgroundViewer(penBcg);
  }

  wo_cenEntities.Unlock();
  // for each entity
  {for(INDEX iEntity=0; iEntity<ctEntities; iEntity++) {
    CEntity &en = wo_cenAllEntities[iEntity];
    // if the entity is destroyed
    if (en.en_ulFlags&ENF_DELETED) {
      // remove the reference made by itself
      ASSERT(en.en_ctReferences>1); // must be referenced by someone else too
      en.RemReference();
      wo_cenEntities.Remove(&en);
    }
  }}
  wo_cenEntities.Lock();

  // if version with entity order
  if (istr->PeekID_t()==CChunkID("ENOR")) { // entity order
    istr->ExpectID_t(CChunkID("ENOR")); // entity order
    INDEX ctEntities;
    *istr>>ctEntities;
    wo_cenEntities.Clear();
    // for each non-deleted entity
    for(INDEX i=0; i<ctEntities; i++) {
      ULONG ulID;
      *istr>>ulID;
      wo_cenEntities.Add(EntityFromID(ulID));
    }
  }

  // some shadow layers might not have light sources, remove such to prevent crashes
  wo_baBrushes.RemoveDummyLayers();

  SetProgressDescription(TRANS("preparing world"));
  CallProgressHook_t(0.0f);
  // after all entities have been read and brushes are connected to entities,
  // calculate bounding boxes of all brushes
  wo_baBrushes.CalculateBoundingBoxes();
  CallProgressHook_t(0.3f);
  // after all bounding boxes and BSP trees are created,
  // create links between portals and sectors on their other side if needed
  if (!_bPortalSectorLinksPreLoaded) {
    wo_baBrushes.LinkPortalsAndSectors();
  }
  wo_bPortalLinksUpToDate = TRUE;
  // create links between sectors and non-zoning entities in sectors
  wo_baBrushes.ReadEntitySectorLinks_t(*istr);
  CallProgressHook_t(0.6f);
  LinkEntitiesToSectors();
  CallProgressHook_t(1.0f);
  _bPortalSectorLinksPreLoaded = FALSE;
  _bEntitySectorLinksPreLoaded = FALSE;
}

/*
 * Write current world state to stream.
 */
void CWorld::WriteState_t( CTStream *ostr, BOOL bImportDictionary /* = FALSE */) // throw char *
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // all predictors must be deleted
  ASSERT(
    wo_cenPredicted.Count()==0 && 
    wo_cenPredictor.Count()==0 && 
    wo_cenWillBePredicted.Count()==0);

  if (bImportDictionary) {
    ostr->DictionaryWriteBegin_t(wo_fnmFileName, wo_slStateDictionaryOffset);
  } else {
    ostr->DictionaryWriteBegin_t(CTString(""), 0);
  }
  (*ostr).WriteID_t("WSTA"); // world state
  // write the world version
  (*ostr)<<INDEX(WORLDSTATEVERSION_CURRENT);

  // write the world description
  WriteInfo_t(ostr);

  // write the world background color
  (*ostr)<<wo_colBackground;

  // write entity ID counter
  ostr->WriteID_t("NFID");
  (*ostr)<<wo_ulNextEntityID;

  // write background viewer entity if any
  ostr->WriteID_t("BGVW"); // background viewer
  CEntity *penBackgroundViewer = GetBackgroundViewer();
  if (penBackgroundViewer==NULL) {
    (*ostr)<<INDEX(-1);
  } else {
    (*ostr)<<penBackgroundViewer->en_ulID;
  }

  // write backdrop image data
  ostr->WriteID_t("BRDP"); // backdrop
  (*ostr)<<wo_strBackdropUp;
  (*ostr)<<wo_strBackdropFt;
  (*ostr)<<wo_strBackdropRt;
  (*ostr)<<wo_fUpW<<wo_fUpL<<wo_fUpCX<<wo_fUpCZ;
  (*ostr)<<wo_fFtW<<wo_fFtH<<wo_fFtCX<<wo_fFtCY;
  (*ostr)<<wo_fRtL<<wo_fRtH<<wo_fRtCZ<<wo_fRtCY;

  // write backdrop object name
  ostr->WriteID_t("BDRO"); // backdrop object
  (*ostr)<<wo_strBackdropObject;

  // write viewer position
  ostr->WriteID_t("VWPS"); // viewer placement
  (*ostr)<<wo_plFocus;
  (*ostr)<<wo_fTargetDistance;
  // write thumbnail position
  ostr->WriteID_t("TBPS"); // thumbnail placement
  (*ostr)<<wo_plThumbnailFocus;
  (*ostr)<<wo_fThumbnailTargetDistance;

  /* NOTE: Entities must be saved in two passes, so that they can be loaded in two passes,
   * since all entities must be created before any entity pointer properties can be loaded.
   */
  ostr->WriteID_t("ENs2"); // entities
  // write number of entities
  (*ostr)<<wo_cenAllEntities.Count();
  // for each entity
  {FOREACHINDYNAMICCONTAINER(wo_cenAllEntities, CEntity, iten) {
    CEntity &en = *iten;
    // write the id, class and its placement
    (*ostr)<<en.en_ulID<<en.en_pecClass->GetName()<<en.en_plPlacement;
  }}
  // for each entity
  {FOREACHINDYNAMICCONTAINER(wo_cenAllEntities, CEntity, iten) {
    // remember stream position
    SLONG slOffset = ostr->GetPos_t();
    // serialize entity into stream
    iten->Write_t(ostr);
    // save the size of data in start chunk, after chunkid and entity id
    SLONG slOffsetAfter = ostr->GetPos_t();
    ostr->SetPos_t(slOffset+2*sizeof(SLONG));
    *ostr<<SLONG(slOffsetAfter-slOffset-3*sizeof(SLONG));
    ostr->SetPos_t(slOffsetAfter);
  }}

  ostr->WriteID_t(CChunkID("ENOR")); // entity order
  *ostr<<wo_cenEntities.Count();
  // for each non-deleted entity
  {FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // write its id
    *ostr<<iten->en_ulID;
  }}

  wo_baBrushes.WriteEntitySectorLinks_t(*ostr);

  ostr->DictionaryWriteEnd_t();
}

// read/write world information (description, name, flags...)
void CWorld::ReadInfo_t(CTStream *strm, BOOL bMaybeDescription) // throw char *
{
  // if version with world info
  if (strm->PeekID_t()==CChunkID("WLIF")) { // world info
    strm->ExpectID_t(CChunkID("WLIF")); // world info

    // skip eventual translation chunk
    if (strm->PeekID_t()==CChunkID("DTRS")) {
      strm->ExpectID_t("DTRS");
    }
    // read the name
    (*strm)>>wo_strName;
    // read the flags
    (*strm)>>wo_ulSpawnFlags;
    // read the world description
    (*strm)>>wo_strDescription;

  // if version with description only
  } else if (bMaybeDescription) {
    // read the world description
    (*strm)>>wo_strDescription;
  }
}

void CWorld::WriteInfo_t(CTStream *strm) // throw char *
{
  strm->WriteID_t(CChunkID("WLIF"));  // world info
  // write the name
  strm->WriteID_t(CChunkID("DTRS"));
  (*strm)<<wo_strName;
  // write the flags
  (*strm)<<wo_ulSpawnFlags;
  // write the world description
  (*strm)<<wo_strDescription;
}

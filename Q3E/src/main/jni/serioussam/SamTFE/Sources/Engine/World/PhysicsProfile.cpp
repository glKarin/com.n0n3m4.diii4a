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

#include <Engine/World/PhysicsProfile.h>

// profile form for profiling world physics
CPhysicsProfile ppPhysicsProfile;
CProfileForm &_pfPhysicsProfile = ppPhysicsProfile;

CPhysicsProfile::CPhysicsProfile(void)
 : CProfileForm ("Physics", "frames",
    CPhysicsProfile::PCI_COUNT, CPhysicsProfile::PTI_COUNT)
{
  SETTIMERNAME(PTI_PROCESSGAMETICK, "ProcessGameTick()", "");
  SETTIMERNAME(PTI_APPLYACTIONS,    " applying client actions", "");
  SETTIMERNAME(PTI_HANDLETIMERS,    " handling timers", "");
  SETTIMERNAME(PTI_HANDLEMOVERS,    " handling movers", "");
  SETTIMERNAME(PTI_WORLDBASETICK,   " WorldBase tick", "");

  SETTIMERNAME(PTI_PREMOVING,       "PreMoving()", "move");
  SETTIMERNAME(PTI_POSTMOVING,      "PostMoving()", "move");
  SETTIMERNAME(PTI_DOMOVING,        "DoMoving()", "move");
  SETTIMERNAME(PTI_ISSTANDINGONPOLYGON, " IsStandingOnPolygon()", "");
  SETTIMERNAME(PTI_TRYTOTRANSLATE,  " TryToTranslate()", "try");
  SETTIMERNAME(PTI_TRYTOROTATE,     " TryToRotate()", "try");
  SETTIMERNAME(PTI_TRYTOGOUPSTAIRS, " TryToGoUpstairs()", "try");
  SETTIMERNAME(PTI_SETPLACEMENTFROMNEXTPOSITION, "SetPlacementFromNextPosition()", "setting");
  SETTIMERNAME(PTI_SETPLACEMENT,    " SetPlacement()", "setting");

  SETTIMERNAME(PTI_SETPLACEMENT_COORDSUPDATE,    "  coords updating", "");
  SETTIMERNAME(PTI_SETPLACEMENT_LIGHTUPDATE,     "  light updating", "");
  SETTIMERNAME(PTI_SETPLACEMENT_BRUSHUPDATE,     "  brush updating", "");
  SETTIMERNAME(PTI_SETPLACEMENT_SPATIALUPDATE,   "  spatial updating", "");
  SETTIMERNAME(PTI_SETPLACEMENT_COLLISIONUPDATE, "  collision updating", "");


  SETTIMERNAME(PTI_PREPARECLIPMOVE,           "CClipMove::CClipMove()", "");
  SETTIMERNAME(PTI_CLIPMOVETOWORLD,           "ClipMoveToWorld()", "clip");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES,         "ClipMoveToBrushes()", "clip");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES_ADDINITIAL,        " addinitial", "sector");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES_MAINLOOP,          " mainloop", "sector");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES_FINDNONZONING,     "   findnonzoning", "");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES_ADDNONZONING,      "     addnonzoning", "");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHES_CLEANUP,           " cleanup", "");
  SETTIMERNAME(PTI_CLIPMOVETOMODELS,          "ClipMoveToModels()", "");

  SETTIMERNAME(PTI_CACHENEARPOLYGONS,     "CacheNearPolygons()", "caching");
  SETTIMERNAME(PTI_CACHENEARPOLYGONS_ADDINITIAL,    " addinitial", "");
  SETTIMERNAME(PTI_CACHENEARPOLYGONS_MAINLOOP,      "   mainloop", "sector");
  SETTIMERNAME(PTI_CACHENEARPOLYGONS_MAINLOOPFOUND, "     found", "polygon");
  SETTIMERNAME(PTI_CACHENEARPOLYGONS_CLEANUP,       " cleanup", "");

  SETTIMERNAME(PTI_CLIPTONONZONINGSECTOR, "ClipToNonZoningSector()", "polygon");
  SETTIMERNAME(PTI_CLIPTOZONINGSECTOR,    "ClipToZoningSector()", "polygon");

  SETTIMERNAME(PTI_CLIPMOVETOMODEL,               " ClipMoveToModel()", "");
  SETTIMERNAME(PTI_CLIPMOVETOMODELNONTRIVIAL,     " ClipMoveToModel()-nontrivial", "");
  SETTIMERNAME(PTI_CLIPMOVETOBRUSHPOLYGON,        " ClipMoveToBrushPolygon()", "");
  SETTIMERNAME(PTI_CLIPMODELMOVETOMODEL,          " ClipModelMoveToModel()", "");
  SETTIMERNAME(PTI_PREPAREPROJECTIONSANDSPHERES,  " PrepareProjectionsAndSpheres()", "");
  SETTIMERNAME(PTI_PROJECTASPHERESTOB,            "  ProjectASpheresToB()", "");
  SETTIMERNAME(PTI_GETPOSITIONSOFENTITY,          " GetPositionsOfEntity()", "");

  SETTIMERNAME(PTI_FINDENTITIESNEARBOX,           " FindEntitiesNearBox()", "");
  SETTIMERNAME(PTI_ADDENTITYTOGRID,               " AddEntityToCollisionGrid()", "");
  SETTIMERNAME(PTI_REMENTITYFROMGRID,             " RemoveEntityFromCollisionGrid()", "");
  SETTIMERNAME(PTI_MOVEENTITYINGRID,              " MoveEntityInCollisionGrid()", "");

  SETCOUNTERNAME(PCI_GRAVITY_NONTRIVIAL,  "non-trivial gravity moves");
  SETCOUNTERNAME(PCI_GRAVITY_TRIVIAL,     "trivial gravity moves");

  SETCOUNTERNAME(PCI_CLIPMOVES, "tested movements");
  SETCOUNTERNAME(PCI_XXTESTS,          "x-x tests");
  SETCOUNTERNAME(PCI_MODELXTESTS,      "model-x tests");
  SETCOUNTERNAME(PCI_BRUSHXTESTS,      "brush-x tests");
  SETCOUNTERNAME(PCI_MODELMODELTESTS,  "model-model tests");
  SETCOUNTERNAME(PCI_MODELBRUSHTESTS,  "model-brush tests");
  SETCOUNTERNAME(PCI_SPHERETOPOLYGONTESTS, "sphere-polygon tests");
  SETCOUNTERNAME(PCI_SPHERETOSPHERETESTS, "sphere-sphere tests");
  SETCOUNTERNAME(PCI_SPHERETOSPHEREHITS,  "sphere-sphere hits");

  SETCOUNTERNAME(PCI_DOMOVING,                "do moving");
  SETCOUNTERNAME(PCI_DOMOVING_SYNC,           " sync");
  SETCOUNTERNAME(PCI_DOMOVING_ASYNC,          " async");
  SETCOUNTERNAME(PCI_DOMOVING_ASYNC_SYNCTRY,  "  sync try");
  SETCOUNTERNAME(PCI_DOMOVING_ASYNC_SYNCPASS, "  sync pass");
  SETCOUNTERNAME(PCI_DOMOVING_ASYNC_TRANSLATE,"  translate");
  SETCOUNTERNAME(PCI_DOMOVING_ASYNC_ROTATE,   "  rotate");
  SETCOUNTERNAME(PCI_TRYTOMOVE,     "try to move");
  SETCOUNTERNAME(PCI_TRYTOMOVE_FAST," fast");
  SETCOUNTERNAME(PCI_TRYTOMOVE_PASS," pass");
  SETCOUNTERNAME(PCI_TRYTOMOVE_CLIP," clip");

  SETCOUNTERNAME(PCI_FINDINGNEARENTITIES, "how many times FindEntitiesNearBox() was called");
  SETCOUNTERNAME(PCI_NEARCELLSFOUND,  "cells found in FindEntitiesNearBox()");
  SETCOUNTERNAME(PCI_NEAROCCUPIEDCELLSFOUND, "occupied cells found in FindEntitiesNearBox()");
  SETCOUNTERNAME(PCI_NEARENTITIESFOUND,  "entities found in FindEntitiesNearBox()");
}


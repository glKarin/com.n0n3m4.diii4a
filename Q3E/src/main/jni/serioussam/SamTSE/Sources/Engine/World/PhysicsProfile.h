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
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 

S-Cape3D 3D game engine library
Copyright (c) 1997-1998, CroTeam. */

#ifndef SE_INCL_PHYSICSPROFILE_H
#define SE_INCL_PHYSICSPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#ifndef __ENGINE_BASE_PROFILING_H__
#include <Engine/Base/Profiling.h>
#endif

/* Class for holding profiling information for physics. */
class CPhysicsProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_PROCESSGAMETICK,
    PTI_APPLYACTIONS,
    PTI_HANDLETIMERS,
    PTI_HANDLEMOVERS,
    PTI_WORLDBASETICK,

    PTI_DUMMY1,

    PTI_PREMOVING,
    PTI_POSTMOVING,
    PTI_DOMOVING,
    PTI_ISSTANDINGONPOLYGON,
    PTI_TRYTOTRANSLATE,
    PTI_TRYTOROTATE,
    PTI_TRYTOGOUPSTAIRS,
    PTI_SETPLACEMENTFROMNEXTPOSITION,
    PTI_SETPLACEMENT,
    PTI_SETPLACEMENT_COORDSUPDATE,
    PTI_SETPLACEMENT_LIGHTUPDATE,
    PTI_SETPLACEMENT_BRUSHUPDATE,
    PTI_SETPLACEMENT_SPATIALUPDATE,
    PTI_SETPLACEMENT_COLLISIONUPDATE,

    PTI_DUMMY2,

    PTI_PREPARECLIPMOVE,
    PTI_CLIPMOVETOWORLD,
    PTI_CLIPMOVETOBRUSHES,
      PTI_CLIPMOVETOBRUSHES_ADDINITIAL,
      PTI_CLIPMOVETOBRUSHES_MAINLOOP,
        PTI_CLIPMOVETOBRUSHES_FINDNONZONING,
          PTI_CLIPMOVETOBRUSHES_ADDNONZONING,
      PTI_CLIPMOVETOBRUSHES_CLEANUP,

    PTI_CACHENEARPOLYGONS,
      PTI_CACHENEARPOLYGONS_ADDINITIAL,
      PTI_CACHENEARPOLYGONS_MAINLOOP,
        PTI_CACHENEARPOLYGONS_MAINLOOPFOUND,
      PTI_CACHENEARPOLYGONS_CLEANUP,

    PTI_CLIPTONONZONINGSECTOR,
    PTI_CLIPTOZONINGSECTOR,

    PTI_CLIPMOVETOMODELS,
    PTI_CLIPMOVETOMODEL,
    PTI_CLIPMOVETOMODELNONTRIVIAL,
    PTI_CLIPBRUSHMOVETOMODEL,
    PTI_CLIPMOVETOBRUSHPOLYGON,
    PTI_CLIPMODELMOVETOMODEL,
    PTI_PREPAREPROJECTIONSANDSPHERES,
    PTI_PROJECTASPHERESTOB,
    PTI_GETPOSITIONSOFENTITY,

    PTI_DUMMY3,

    PTI_FINDENTITIESNEARBOX,
    PTI_ADDENTITYTOGRID,
    PTI_REMENTITYFROMGRID,
    PTI_MOVEENTITYINGRID,
    PTI_COUNT
  };
  enum ProfileCounterIndex {
    PCI_GRAVITY_NONTRIVIAL,    // non-trivial gravity moves
    PCI_GRAVITY_TRIVIAL,       // trivial gravity moves

    PCI_CLIPMOVES,                // number of tested movements
    PCI_XXTESTS,                  // number of x-x tests
    PCI_MODELXTESTS,              // number of model-x tests
    PCI_BRUSHXTESTS,              // number of brush-x tests
    PCI_MODELMODELTESTS,          // number of model-model tests
    PCI_MODELBRUSHTESTS,          // number of model-brush tests
    PCI_SPHERETOPOLYGONTESTS,     // number of sphere-polygon tests
    PCI_SPHERETOSPHERETESTS,      // number of sphere-sphere tests
    PCI_SPHERETOSPHEREHITS,       // number of sphere-sphere hits

    PCI_DOMOVING,
    PCI_DOMOVING_SYNC,
    PCI_DOMOVING_ASYNC,
    PCI_DOMOVING_ASYNC_SYNCTRY,
    PCI_DOMOVING_ASYNC_SYNCPASS,
    PCI_DOMOVING_ASYNC_TRANSLATE,
    PCI_DOMOVING_ASYNC_ROTATE,
    PCI_TRYTOMOVE,
    PCI_TRYTOMOVE_FAST,
    PCI_TRYTOMOVE_PASS,
    PCI_TRYTOMOVE_CLIP,

    PCI_FINDINGNEARENTITIES,      // how many times FindEntitiesNearBox() was called
    PCI_NEARCELLSFOUND,           // cells found in FindEntitiesNearBox()
    PCI_NEAROCCUPIEDCELLSFOUND,   // occupied cells found in FindEntitiesNearBox()
    PCI_NEARENTITIESFOUND,        // near entities found in FindEntitiesNearBox()
    PCI_COUNT
  };
  // constructor
  CPhysicsProfile(void);
};

#endif /* include-once wrapper. */



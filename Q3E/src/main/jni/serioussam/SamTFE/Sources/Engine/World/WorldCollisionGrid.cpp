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
//#pragma GCC optimize 0
#include <Engine/World/World.h>
#include <Engine/World/PhysicsProfile.h>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/AllocationArray.h>
#include <Engine/Templates/AllocationArray.cpp>
#include <cmath> //karin: for isinf

#define DEBUG_COLLIDEWITHALL 0

#if DEBUG_COLLIDEWITHALL
  #include <Engine/Templates/DynamicContainer.h>
  #include <Engine/Templates/DynamicContainer.cpp>
#endif

// allowed grid dimensions (meters)
#define GRID_MIN (-32000)
#define GRID_MAX (+32000)

#define GRID_CELLSIZE  2.0 // size of one grid cell (meters)
// number of hash table entries for grid cells
#define GRID_HASHTABLESIZE_LOG2  12 // must be even for bit-shuffling
#define GRID_HASHTABLESIZE (1<<GRID_HASHTABLESIZE_LOG2)

//#pragma inline_depth(0)

// find grid box from float coordinates
static inline void BoxToGrid(
  const FLOATaabbox3D &boxEntity, INDEX &iMinX, INDEX &iMaxX, INDEX &iMinZ, INDEX &iMaxZ)
{
  FLOAT fMinX = boxEntity.Min()(1);
  FLOAT fMinZ = boxEntity.Min()(3);
  FLOAT fMaxX = boxEntity.Max()(1);
  FLOAT fMaxZ = boxEntity.Max()(3);
#ifdef __arm__
#if defined(PLATFORM_PANDORA) || defined(PLATFORM_PYRA)
  #define Isinf(a) (((*(unsigned int*)&a)&0x7fffffff)==0x7f800000)
#elif defined(ANDROID)
  #define Isinf std::isinf
#else
  #define Isinf isinff
#endif
  iMinX = (Isinf(fMinX))?INDEX(GRID_MIN):Clamp(INDEX(floor(fMinX/GRID_CELLSIZE)), (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMinZ = (Isinf(fMinZ))?INDEX(GRID_MIN):Clamp(INDEX(floor(fMinZ/GRID_CELLSIZE)), (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMaxX = (Isinf(fMaxX))?INDEX(GRID_MIN):Clamp(INDEX(ceil(fMaxX/GRID_CELLSIZE)), (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMaxZ = (Isinf(fMaxZ))?INDEX(GRID_MIN):Clamp(INDEX(ceil(fMaxZ/GRID_CELLSIZE)), (INDEX)GRID_MIN, (INDEX)GRID_MAX);
#else
#if 0 //karin: must check is inf, it cause block on intro
  iMinX = INDEX(floor(fMinX/GRID_CELLSIZE));
  iMinZ = INDEX(floor(fMinZ/GRID_CELLSIZE));
  iMaxX = INDEX(ceil(fMaxX/GRID_CELLSIZE));
  iMaxZ = INDEX(ceil(fMaxZ/GRID_CELLSIZE));
#else
		iMinX = std::isinf(fMinX) ? GRID_MIN : INDEX(floor(fMinX/GRID_CELLSIZE));
		iMinZ = std::isinf(fMinZ) ? GRID_MIN : INDEX(floor(fMinZ/GRID_CELLSIZE));
  iMaxX = std::isinf(fMaxX) ? GRID_MIN : INDEX(ceil(fMaxX/GRID_CELLSIZE));
  iMaxZ = std::isinf(fMaxZ) ? GRID_MIN : INDEX(ceil(fMaxZ/GRID_CELLSIZE));
#endif

  iMinX = Clamp(iMinX, (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMinZ = Clamp(iMinZ, (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMaxX = Clamp(iMaxX, (INDEX)GRID_MIN, (INDEX)GRID_MAX);
  iMaxZ = Clamp(iMaxZ, (INDEX)GRID_MIN, (INDEX)GRID_MAX);
#endif
}

// key calculations
static inline ULONG MakeCode(INDEX iX, INDEX iZ)
{
  return (iX<<16)|(iZ&0xffff);
}

static inline INDEX MakeKey(INDEX iX, INDEX iZ)
{
  //INDEX iKey = (iX+iZ)&(GRID_HASHTABLESIZE-1);  // x+z
  // use absolute x and z, swap upper and lower bits in z, xor x and z
  INDEX iZ2 = abs(iZ);
  INDEX iKey = (iZ2>>(GRID_HASHTABLESIZE_LOG2/2));
  iKey |= ((iZ2&(GRID_HASHTABLESIZE/2-1))<<(GRID_HASHTABLESIZE_LOG2/2));
  iKey ^= abs(iX);
  iKey &= (GRID_HASHTABLESIZE-1);
  return iKey;
}

static inline INDEX MakeKeyFromCode(ULONG ulCode)
{
  INDEX iX = SLONG(ulCode)>>16;
  INDEX iZ = SLONG(SWORD(ulCode&0xffff));
  return MakeKey(iX, iZ);
}

// collision grid classes
class CGridCell {
public:
  ULONG gc_ulCode;      // 32 bit uid of the cell (from its coordinates in grid)
  INDEX gc_iNextCell;   // next cell with this hash code
  INDEX gc_iFirstEntry; // first entry in this cell
};
class CGridEntry {
public:
  CEntity *ge_penEntity;    // entity pointed to
  INDEX ge_iNextEntry;      // next entry in same cell
};

class CCollisionGrid {
public:
  CStaticArray<INDEX> cg_aiFirstCells;     // first cell for each hash entry
  CAllocationArray<CGridCell> cg_agcCells;     // all cells
  CAllocationArray<CGridEntry> cg_ageEntries;  // all entries

  CCollisionGrid(void);
  ~CCollisionGrid(void);
  void Clear(void);
  // create a new grid cell in given hash table entry
  INDEX CreateCell(INDEX iKey, ULONG ulCode);
  // remove a cell
  void RemoveCell(INDEX igc);
  // get grid cell for its coordinates
  INDEX FindCell(INDEX iX, INDEX iZ, BOOL bCreate);
  // add entry to a given cell
  void AddEntry(INDEX igc, CEntity *pen);
  // remove entry from a given cell
  void RemoveEntry(INDEX igc, CEntity *pen);
};


// collision grid class implementation

CCollisionGrid::CCollisionGrid(void)
{
  Clear();
}

CCollisionGrid::~CCollisionGrid(void)
{
  Clear();
}

void CCollisionGrid::Clear(void)
{
  cg_aiFirstCells.Clear();
  cg_agcCells.Clear();
  cg_ageEntries.Clear();

  cg_aiFirstCells.New(GRID_HASHTABLESIZE);
  cg_agcCells.SetAllocationStep(1024);
  cg_ageEntries.SetAllocationStep(1024);

  // mark all cells as unused
  for(INDEX iKey=0; iKey<GRID_HASHTABLESIZE; iKey++) {
    cg_aiFirstCells[iKey] = -1;
  }
}

// create a new grid cell in given hash table entry
INDEX CCollisionGrid::CreateCell(INDEX iKey, ULONG ulCode)
{
  // find an empty cell
  INDEX igc = cg_agcCells.Allocate();
  CGridCell &gc = cg_agcCells[igc];

  // set up the cell
  gc.gc_ulCode = ulCode;
  gc.gc_iFirstEntry = -1;

  // link it by hash key
  gc.gc_iNextCell = cg_aiFirstCells[iKey];
  cg_aiFirstCells[iKey] = igc;

  return igc;
}

// remove a cell
void CCollisionGrid::RemoveCell(INDEX igc)
{
  // get key of the cell
  CGridCell &gc = cg_agcCells[igc];
  INDEX iKey = MakeKeyFromCode(gc.gc_ulCode);

  // find the cell's index pointer
  INDEX *pigc = &cg_aiFirstCells[iKey];
  ASSERT(*pigc>=0);
  while(*pigc>=0) {
    CGridCell &gc = cg_agcCells[*pigc];
    if (*pigc==igc) {
      *pigc = gc.gc_iNextCell;
      gc.gc_iNextCell = -2;
      gc.gc_iFirstEntry = -1;
      gc.gc_ulCode = 0x12345678;
      cg_agcCells.Free(igc);
      return;
    }
    pigc = &gc.gc_iNextCell;
  }
  ASSERT(FALSE);
}

// get grid cell for its coordinates
INDEX CCollisionGrid::FindCell(INDEX iX, INDEX iZ, BOOL bCreate)
{
  // make uid of the cell
  ASSERT(iX>=GRID_MIN && iX<=GRID_MAX);
  ASSERT(iZ>=GRID_MIN && iZ<=GRID_MAX);
  ULONG ulCode = MakeCode(iX, iZ);
  // get the hash key for the cell
  INDEX iKey = MakeKey(iX, iZ);  // x+z, use lower bits
  ASSERT(iKey==MakeKeyFromCode(ulCode));
  // find the cell in list of cells with that key
  INDEX igcFound = -1;
  for (INDEX igc=cg_aiFirstCells[iKey]; igc>=0; igc = cg_agcCells[igc].gc_iNextCell) {
    if (cg_agcCells[igc].gc_ulCode==ulCode) {
      igcFound = igc;
      break;
    }
  }

  // if the cell is found
  if (igcFound>=0) {
    // use existing one
    return igcFound;
  // if the cell is not found
  } else {
    // if new one may be created
    if (bCreate) {
      // create a new one
      return CreateCell(iKey, ulCode);
    // if new one may not be created
    } else {
      // return nothing
      return -1;
    }
  }
}

// add entry to a given cell
void CCollisionGrid::AddEntry(INDEX igc, CEntity *pen)
{
  // find an empty entry
  INDEX ige = cg_ageEntries.Allocate();
  CGridEntry &ge = cg_ageEntries[ige];

  // init the entry and link it in its cell
  ge.ge_penEntity = pen;
  CGridCell &gc = cg_agcCells[igc];
  ge.ge_iNextEntry = gc.gc_iFirstEntry;
  gc.gc_iFirstEntry = ige;
}

// remove entry from a given cell
void CCollisionGrid::RemoveEntry(INDEX igc, CEntity *pen)
{
  CGridCell &gc = cg_agcCells[igc];

  // find the entry's index pointer
  INDEX *pige = &gc.gc_iFirstEntry;
  ASSERT(*pige>=0);
  while(*pige>=0) {
    CGridEntry &ge = cg_ageEntries[*pige];
    if (ge.ge_penEntity==pen) {
      // remove the entry from the list
      cg_ageEntries.Free(*pige);
      *pige = ge.ge_iNextEntry;
      ge.ge_iNextEntry = -2;
      ge.ge_penEntity = NULL;
      // if the cell becomes empty
      if (gc.gc_iFirstEntry<0) {
        // remove the cell
        RemoveCell(igc);
      }
      return;
    }
    pige = &ge.ge_iNextEntry;
  }
  ASSERT(FALSE);
}



/* Initialize collision grid. */
void CWorld::InitCollisionGrid(void)
{
  wo_pcgCollisionGrid = new CCollisionGrid;
}
/* Destroy collision grid. */
void CWorld::DestroyCollisionGrid(void)
{
  delete wo_pcgCollisionGrid;
  wo_pcgCollisionGrid = NULL;
}
// clear collision grid
void CWorld::ClearCollisionGrid(void)
{
  wo_pcgCollisionGrid->Clear();
}


/* Add an entity to cell(s) in collision grid. */
void CWorld::AddEntityToCollisionGrid(CEntity *pen, const FLOATaabbox3D &boxEntity)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_ADDENTITYTOGRID);
  // find grid coordinates
  INDEX iMinX, iMaxX, iMinZ, iMaxZ;
  BoxToGrid(boxEntity, iMinX, iMaxX, iMinZ, iMaxZ);
  // for each cell spanned by the entity
  for(INDEX iX=iMinX; iX<=iMaxX; iX++) {
    for(INDEX iZ=iMinZ; iZ<=iMaxZ; iZ++) {
      // find that cell
      INDEX igc = wo_pcgCollisionGrid->FindCell(iX, iZ, TRUE);
      // add the entity to the cell
      wo_pcgCollisionGrid->AddEntry(igc, pen);
    }
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_ADDENTITYTOGRID);
}


/* Remove an entity from cell(s) in collision grid. */
void CWorld::RemoveEntityFromCollisionGrid(CEntity *pen, const FLOATaabbox3D &boxEntity)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_REMENTITYFROMGRID);
  // find grid coordinates
  INDEX iMinX, iMaxX, iMinZ, iMaxZ;
  BoxToGrid(boxEntity, iMinX, iMaxX, iMinZ, iMaxZ);
  // for each cell spanned by the entity
  for(INDEX iX=iMinX; iX<=iMaxX; iX++) {
    for(INDEX iZ=iMinZ; iZ<=iMaxZ; iZ++) {
      // find that cell
      INDEX igc = wo_pcgCollisionGrid->FindCell(iX, iZ, FALSE);
      ASSERT(igc>=0);
      // remove the entity from the cell
      if (igc>=0) {
        wo_pcgCollisionGrid->RemoveEntry(igc, pen);
      }
    }
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_REMENTITYFROMGRID);
}

/* Move an entity inside cell(s) in collision grid. */
void CWorld::MoveEntityInCollisionGrid(CEntity *pen,
  const FLOATaabbox3D &boxOld, const FLOATaabbox3D &boxNew)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_MOVEENTITYINGRID);

  // find grid coordinates
  INDEX iOldMinX, iOldMaxX, iOldMinZ, iOldMaxZ;
  BoxToGrid(boxOld, iOldMinX, iOldMaxX, iOldMinZ, iOldMaxZ);
  INDEX iNewMinX, iNewMaxX, iNewMinZ, iNewMaxZ;
  BoxToGrid(boxNew, iNewMinX, iNewMaxX, iNewMinZ, iNewMaxZ);

  // for each cell spanned by the entity before moving but not after moving
  {for(INDEX iX=iOldMinX; iX<=iOldMaxX; iX++) {
    for(INDEX iZ=iOldMinZ; iZ<=iOldMaxZ; iZ++) {
      if (iX>=iNewMinX && iX<=iNewMaxX
        &&iZ>=iNewMinZ && iZ<=iNewMaxZ) {
        continue;
      }
      // find that cell
      INDEX igc = wo_pcgCollisionGrid->FindCell(iX, iZ, FALSE);
      ASSERT(igc>=0);
      // remove the entity from the cell
      if (igc>=0) {
        wo_pcgCollisionGrid->RemoveEntry(igc, pen);
      }
    }
  }}

  // for each cell spanned by the entity after moving but not before moving
  {for(INDEX iX=iNewMinX; iX<=iNewMaxX; iX++) {
    for(INDEX iZ=iNewMinZ; iZ<=iNewMaxZ; iZ++) {
      if (iX>=iOldMinX && iX<=iOldMaxX
        &&iZ>=iOldMinZ && iZ<=iOldMaxZ) {
        continue;
      }
      // find that cell
      INDEX igc = wo_pcgCollisionGrid->FindCell(iX, iZ, TRUE);
      wo_pcgCollisionGrid->AddEntry(igc, pen);
    }
  }}
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_MOVEENTITYINGRID);
}


/* Find all entities in collision grid near given box. */
void CWorld::FindEntitiesNearBox(const FLOATaabbox3D &boxNear,
  CStaticStackArray<CEntity*> &apenNearEntities)
{

#if DEBUG_COLLIDEWITHALL
  apenNearEntities.PopAll();
  // for each entity
  {FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    CEntity &en = *iten;
    if (en.GetRenderType()==CEntity::RT_MODEL && en.en_pciCollisionInfo!=NULL) {
      apenNearEntities.Push() = &en;
    }
  }}

  return;
#endif

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_FINDENTITIESNEARBOX);
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_FINDINGNEARENTITIES);

  // find grid coordinates
  INDEX iMinX, iMaxX, iMinZ, iMaxZ;
  BoxToGrid(boxNear, iMinX, iMaxX, iMinZ, iMaxZ);
  apenNearEntities.PopAll();

  // for each cell spanned by the box
  {for(INDEX iX=iMinX; iX<=iMaxX; iX++) {
    for(INDEX iZ=iMinZ; iZ<=iMaxZ; iZ++) {
      _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_NEARCELLSFOUND);
      // find that cell
      INDEX igc = wo_pcgCollisionGrid->FindCell(iX, iZ, FALSE);
      // if the cell is empty
      if (igc<0) {
        // skip it
        continue;
      }
      _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_NEAROCCUPIEDCELLSFOUND);
      // for each entity in the cell
      for(INDEX iEntry = wo_pcgCollisionGrid->cg_agcCells[igc].gc_iFirstEntry;
          iEntry>=0;
          iEntry = wo_pcgCollisionGrid->cg_ageEntries[iEntry].ge_iNextEntry) {
        CEntity *penEntity = wo_pcgCollisionGrid->cg_ageEntries[iEntry].ge_penEntity;
        // if it is not already found
        if (!(penEntity->en_ulFlags&ENF_FOUNDINGRIDSEARCH)) {
          // add it
          apenNearEntities.Push() = penEntity;
          // mark it as found
          penEntity->en_ulFlags|=ENF_FOUNDINGRIDSEARCH;
        }
      }
    }
  }}


  _pfPhysicsProfile.IncrementCounter(
    CPhysicsProfile::PCI_NEARENTITIESFOUND, apenNearEntities.Count());
  // for each of the found entities
  for(INDEX ienFound=0; ienFound<apenNearEntities.Count(); ienFound++) {
    // clear found flag
    apenNearEntities[ienFound]->en_ulFlags&=~ENF_FOUNDINGRIDSEARCH;
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_FINDENTITIESNEARBOX);
}



// get amount of memory used by this object
extern SLONG GetCollisionGridMemory( CCollisionGrid *pcg)
{
  // no collision grid?
  if( pcg==NULL) return 0;

  // phew, it's here!
  SLONG slUsedMemory = pcg->cg_aiFirstCells.Count() * sizeof(INDEX);
  slUsedMemory += pcg->cg_agcCells.Count()   * sizeof(CGridCell);
  slUsedMemory += pcg->cg_ageEntries.Count() * sizeof(CGridEntry);
  slUsedMemory += pcg->cg_agcCells.aa_aiFreeElements.sa_Count   * sizeof(INDEX);
  slUsedMemory += pcg->cg_ageEntries.aa_aiFreeElements.sa_Count * sizeof(INDEX);
  return slUsedMemory;
}

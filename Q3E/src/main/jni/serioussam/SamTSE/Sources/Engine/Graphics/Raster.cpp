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

#include <Engine/Graphics/Raster.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/GfxLibrary.h>

/*
 *  Raster functions
 */

CRaster::CRaster(PIX ulWidth, PIX ulHeight, ULONG ulFlags) : ra_MainDrawPort()
{
  // remember width and height
  ra_Width  = ulWidth;
  ra_Height = ulHeight;

  // clear uninitialized fields
  ra_LockCount = 0;

  // set flags
  ra_Flags = ulFlags;

  // add main drawport to list
  ASSERT( ra_DrawPortList.IsEmpty());
  ra_DrawPortList.AddTail(ra_MainDrawPort.dp_NodeInRaster);
  ra_MainDrawPort.dp_Raster = this;
  ra_pvpViewPort = NULL;

	// when all is initialized correct the drawport dimensions
	RecalculateDrawPortsDimensions();
}


CRaster::~CRaster(void)
{
	// remove main drawport from list of drawports
	ra_MainDrawPort.dp_NodeInRaster.Remove();
  // remove all other drawports in this raster
  FORDELETELIST(CDrawPort, dp_NodeInRaster, ra_DrawPortList, litdp) {
		// and delete each one
    delete &litdp.Current();
  }

  // raster must be unlocked before destroying it
	ASSERT(ra_LockCount==0);
}


/* Recalculate dimensions for all drawports. */
void CRaster::RecalculateDrawPortsDimensions(void)
{
  // for all drawports in this raster
  FOREACHINLIST(CDrawPort, dp_NodeInRaster, ra_DrawPortList, litdp) {
    // recalculate dimensions to fit new size of raster
    litdp.Current().RecalculateDimensions();
  }
}


/*
 * Lock for drawing.
 */
BOOL CRaster::Lock()
{
  ASSERT( this!=NULL);
  ASSERT( ra_LockCount>=0);

  // if raster size is too small in some axis
  if( ra_Width<1 || ra_Height<1) {
    // do not allow locking
    ASSERTALWAYS( "Raster size to small to be locked!");
    return FALSE;
  }

  // if allready locked
  if( ra_LockCount>0) {
    // just increment counter
    ra_LockCount++;
    return TRUE;
  }
  // if not already locked
  else {
    // try to lock with driver
    BOOL bLocked = _pGfx->LockRaster(this);
    // if succeeded in locking
    if( bLocked) {
      // set the counter to 1
      ra_LockCount = 1;
      return TRUE;
    }
    // lock not ok
    return FALSE;
  }
}


/*
 * Unlock after drawing.
 */
void CRaster::Unlock()
{
  ASSERT( this!=NULL);
  ASSERT( ra_LockCount>0);

  // decrement counter
  ra_LockCount--;
  // if reached zero
  if( ra_LockCount==0 ) {
    // unlock it with driver
    _pGfx->UnlockRaster(this);
  }
}


/*****
 * Change Raster size.
 */
void CRaster::Resize( PIX pixWidth, PIX pixHeight)
{
  ASSERT( pixWidth>0 && pixHeight>0);
  if( pixWidth <=0) pixWidth  = 1;
  if( pixHeight<=0) pixHeight = 1;
  ra_Width  = pixWidth;
  ra_Height = pixHeight;
  RecalculateDrawPortsDimensions();
}

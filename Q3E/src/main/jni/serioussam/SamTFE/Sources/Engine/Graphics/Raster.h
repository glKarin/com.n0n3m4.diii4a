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

#ifndef SE_INCL_RASTER_H
#define SE_INCL_RASTER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Graphics/DrawPort.h>

/*
 *  Raster
 */

class CRaster {
public:
  CViewPort *ra_pvpViewPort;        // viewport if existing
  CDrawPort ra_MainDrawPort;		    // initial drawport for entire raster
  CListHead ra_DrawPortList;	      // list of drawports

  PIX    ra_Width;								  // number of pixels in one row
  PIX    ra_Height;							    // number of pixels in one column
  SLONG  ra_LockCount;							// counter for memory locking
  ULONG  ra_Flags;									// special flags

  /* Recalculate dimensions for all drawports. */
	void RecalculateDrawPortsDimensions(void);

  /* Constructor for given size. */
  CRaster( PIX pixWidth, PIX pixHeight, ULONG ulFlags);
	/* Destructor. */
  virtual ~CRaster(void);
  /* Change size of this raster and all it's drawports. */
  void Resize(PIX pixNewWidth, PIX pixNewHeight);

public:
  /* Lock for drawing. */
  virtual BOOL Lock();
  /* Unlock after drawing. */
  virtual void Unlock();
};


#endif  /* include-once check. */


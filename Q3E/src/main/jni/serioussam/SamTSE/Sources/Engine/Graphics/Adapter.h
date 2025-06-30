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

#ifndef SE_INCL_ADAPTER_H
#define SE_INCL_ADAPTER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Graphics/DisplayMode.h>


// display adapter flags
#define DAF_ONEWINDOW       (1L<<0)   // supports only one window
#define DAF_FULLSCREENONLY  (1L<<1)   // don't add windowed modes
#define DAF_USEGDIFUNCTIONS (1L<<2)   // don't use wgl functions instead gdi functions
#define DAF_16BITONLY       (1L<<3)   // supports only 16-bit color depth


class CDisplayAdapter
{
public:
  CDisplayMode da_admDisplayModes[25]; // 25 should be just enough
  INDEX da_ctDisplayModes;       // number of display modes with hardware acceleration (>=1)
  INDEX da_iCurrentDisplayMode;  // currently active display mode (-1 if none)
  ULONG da_ulFlags;              // various flags (DAF_ ...) 
  CTString da_strVendor;         // OpenGL will fill this upon initialization of adapter ...
  CTString da_strRenderer;       //  ... till then, it will be set to unknown/OpenGL ICD/1.1 or ...
  CTString da_strVersion;        //  ... 3Dfx/OpenGL/1.1
};


class CGfxAPI
{
public:
  CDisplayAdapter ga_adaAdapter[4];
  INDEX ga_ctAdapters;         // min=0, max=4
  INDEX ga_iCurrentAdapter;    // currently active display adapter
};


// get list of all modes avaliable through CDS -- do not modify/free the returned list
CListHead &CDS_GetModes(void);

// set given display mode
BOOL CDS_SetMode( PIX pixSizeI, PIX pixSizeJ, enum DisplayDepth dd);
// reset windows to mode chosen by user within windows diplay properties
void CDS_ResetMode(void);

ULONG DetermineDesktopWidth(void);

#endif  /* include-once check. */


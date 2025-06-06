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

#ifndef SE_INCL_DISPLAYMODE_H
#define SE_INCL_DISPLAYMODE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Base/CTString.h>

// display mode bit-depth
enum DisplayDepth
{
  DD_NODEPTH = -1,
  DD_DEFAULT =  0,
  DD_16BIT   =  1,
  DD_32BIT   =  2,
  DD_24BIT   =  3, // for z-buffer
};

/*
 *  Structure that holds display mode description
 */

class ENGINE_API CDisplayMode {
public:
  PIX dm_pixSizeI; // size of screen in pixels
  PIX dm_pixSizeJ;
  enum DisplayDepth dm_ddDepth;  // bits per pixel for color

  /* Default constructor. */
  CDisplayMode(void);

  // get depth string
  CTString DepthString(void) const;
  // check if mode is dualhead
  BOOL IsDualHead(void);
  // check if mode is widescreen
  BOOL IsWideScreen(void);
  // check if mode is fullscreen
  BOOL IsFullScreen(void);
};


#endif  /* include-once check. */


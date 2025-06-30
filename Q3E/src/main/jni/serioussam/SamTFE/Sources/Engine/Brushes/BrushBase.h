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

#ifndef SE_INCL_BRUSH_BASE_H
#define SE_INCL_BRUSH_BASE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// this is base class for brushes and terrains 
class ENGINE_API CBrushBase {
public:
  virtual INDEX GetBrushType() {
    ASSERT(FALSE);
    return BT_NONE;
  };

  enum BrushType {
    BT_NONE        = 0,     // none 
    BT_BRUSH3D     = 1,     // this is Brush3D
    BT_TERRAIN     = 2,     // this is Terrain
  };
};

#endif

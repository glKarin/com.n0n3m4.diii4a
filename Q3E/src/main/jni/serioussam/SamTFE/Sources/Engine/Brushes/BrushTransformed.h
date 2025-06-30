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

#ifndef SE_INCL_BRUSHTRANSFORMED_H
#define SE_INCL_BRUSHTRANSFORMED_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#undef ALIGNED_NEW_AND_DELETE
#ifdef NDEBUG
#define ALIGNED_NEW_AND_DELETE(align) \
  void *operator new[] (size_t size) { return AllocMemoryAligned(size, align); }; \
  void operator delete[] (void* ptr) { FreeMemoryAligned(ptr); };
#else 
#define ALIGNED_NEW_AND_DELETE(align)
#endif

#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/TextureMapping.h>

/*
 * Cached transformed vertex.
 */
class CWorkingVertex {
public:
  FLOAT3D wvx_vRelative;  // coordinates in brush space
  ALIGNED_NEW_AND_DELETE(32);
  /* Clear the object. */
  inline void Clear(void) {};
};

/*
 * Cached transformed plane.
 */
class CWorkingPlane {
public:
  FLOATplane3D wpl_plRelative;  // coordinates in brush space
  CMappingVectors wpl_mvRelative;  // mapping vectors in brush space
  FLOATplane3D wpl_plView;      // coordinates in view space
  CMappingVectors wpl_mvView;   // mapping vectors in view space
  CPlanarGradients wpl_pgOoK;   // plane gradients in default mapping
  BOOL wpl_bVisible;            // set if the plane is visible
  /* Clear the object. */
  inline void Clear(void) {};
};

/*
 * Cached transformed edge.
 */
class CWorkingEdge {
public:
  INDEX wed_iwvx0;    // working vertices in sector
  INDEX wed_iwvx1;
  /* Clear the object. */
  inline void Clear(void) {};
};


#endif  /* include-once check. */


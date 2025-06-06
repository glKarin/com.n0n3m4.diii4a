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

#ifndef SE_INCL_STOCK_CSKELETON_H
#define SE_INCL_STOCK_CSKELETON_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Ska/Skeleton.h>

#define TYPE CSkeleton
#define CStock_TYPE CStock_CSkeleton
#define CNameTable_TYPE CNameTable_CSkeleton
#define CNameTableSlot_TYPE CNameTableSlot_CSkeleton

#include <Engine/Templates/NameTable.h>
#include <Engine/Templates/Stock.h>

#undef CStock_TYPE
#undef CNameTableSlot_TYPE
#undef CNameTable_TYPE
#undef TYPE

ENGINE_API extern CStock_CSkeleton *_pSkeletonStock;


#endif  /* include-once check. */


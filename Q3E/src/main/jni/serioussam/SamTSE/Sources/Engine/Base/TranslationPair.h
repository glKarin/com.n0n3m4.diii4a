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

#ifndef SE_INCL_TRANSLATIONPAIR_H
#define SE_INCL_TRANSLATIONPAIR_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>

class CTranslationPair {
public:
  BOOL m_bUsed;             // for internal use while building the table
  CTString tp_strSrc;       // original string
  CTString tp_strDst;       // translated string

  CTranslationPair(void) : m_bUsed(FALSE) {};

  inline void Clear(void) {
    tp_strSrc.Clear();
    tp_strDst.Clear();
  };

  // getname function for addinf to nametable
  inline const CTString &GetName(void) const { return tp_strSrc; };
};



#endif  /* include-once check. */


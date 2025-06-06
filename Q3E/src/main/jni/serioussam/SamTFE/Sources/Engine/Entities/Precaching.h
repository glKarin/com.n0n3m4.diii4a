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

#ifndef SE_INCL_PRECACHING_H
#define SE_INCL_PRECACHING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#define PRECACHE_NONE      0
#define PRECACHE_SMART     1
#define PRECACHE_ALL       2
#define PRECACHE_PARANOIA  3

extern ENGINE_API INDEX gam_iPrecachePolicy;
extern ENGINE_API INDEX _precache_bNowPrecaching;

class CTmpPrecachingNow {
public:
  BOOL m_bOldPrecaching;

  inline CTmpPrecachingNow(void) {
    m_bOldPrecaching = _precache_bNowPrecaching;
    _precache_bNowPrecaching = TRUE;
  }
  ~CTmpPrecachingNow(void)
  {
    _precache_bNowPrecaching = m_bOldPrecaching;
  }
};

class ENGINE_API CAutoPrecacheSound {
public:
  CSoundData *apc_psd;
  CAutoPrecacheSound();
  ~CAutoPrecacheSound();
  void Precache(const CTFileName &fnm);
};
class ENGINE_API CAutoPrecacheModel {
public:
  CModelData *apc_pmd;
  CAutoPrecacheModel();
  ~CAutoPrecacheModel();
  void Precache(const CTFileName &fnm);
};
class ENGINE_API CAutoPrecacheTexture {
public:
  CTextureData *apc_ptd;
  CAutoPrecacheTexture();
  ~CAutoPrecacheTexture();
  void Precache(const CTFileName &fnm);
};


#endif  /* include-once check. */


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

#ifndef SE_INCL_VARLIST_H
#define SE_INCL_VARLIST_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CVarSetting {
public:
  CListNode vs_lnNode;
  BOOL  vs_bSeparator;
  BOOL  vs_bCanChangeInGame;
  INDEX vs_iSlider;
  CTString vs_strName;
  CTString vs_strTip;
  CTString vs_strVar;
  CTString vs_strFilter;
  CTFileName vs_strSchedule;
  INDEX vs_iValue;
  INDEX vs_ctValues;
  INDEX vs_iOrgValue;
  BOOL  vs_bCustom;
  CStaticStackArray<CTString> vs_astrTexts;
  CStaticStackArray<CTString> vs_astrValues;
  CVarSetting();
  void Clear(void);
  BOOL Validate(void);
};


extern CListHead _lhVarSettings;

void LoadVarSettings(const CTFileName &fnmCfg);
void FlushVarSettings(BOOL bApply);


#endif  /* include-once check. */


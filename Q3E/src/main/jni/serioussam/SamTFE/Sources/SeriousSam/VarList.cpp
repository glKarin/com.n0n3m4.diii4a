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

#include "SeriousSam/StdH.h"

#include "SeriousSam/VarList.h"

CListHead _lhVarSettings;

static CTString _strFile;
static INDEX _ctLines;

static CTString GetNonEmptyLine_t(CTStream &strm)
{
  FOREVER {
   if(strm.AtEOF()) {
     ThrowF_t(TRANS("Unexpected end of file"));
   }
   CTString str;
   _ctLines++;
   strm.GetLine_t(str);
   str.TrimSpacesLeft();
   if (str.RemovePrefix("//")) {  // skip comments
     continue;
   }
   if (str!="") {
     str.TrimSpacesRight();
     return str;
   }
  }
}

void TranslateLine(CTString &str)
{
  str.TrimSpacesLeft();
  if (str.RemovePrefix("TTRS")) {
    str.TrimSpacesLeft();
    str = TranslateConst(str, 0);
  }
  str.TrimSpacesLeft();
}

static void FixupFileName_t(CTString &strFnm)
{
  strFnm.TrimSpacesLeft();
  strFnm.TrimSpacesRight();
  if (!strFnm.RemovePrefix(CTString("TF") +"NM ")) {  // must not directly have ids in code
    ThrowF_t(TRANS("Expected %s%s before filename"), "TF", "NM");
  }
}

void CheckPVS_t(CVarSetting *pvs)
{
  if (pvs==NULL) {
    ThrowF_t("Gadget expected");
  }
}

void ParseCFG_t(CTStream &strm)
{
  CVarSetting *pvs = NULL;

  // repeat
  FOREVER {
    // read one line
    CTString strLine = GetNonEmptyLine_t(strm);

    if (strLine.RemovePrefix("MenuEnd")) {
      return;
    } else if (strLine.RemovePrefix("Gadget:")) {
      pvs = new CVarSetting;
      _lhVarSettings.AddTail(pvs->vs_lnNode);
      TranslateLine(strLine);
      strLine.TrimSpacesLeft();
      pvs->vs_strName = strLine;
    } else if (strLine.RemovePrefix("Type:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      if (strLine=="Toggle") {
        pvs->vs_bSeparator = FALSE;
      } else if (strLine=="Separator") {
        pvs->vs_bSeparator = TRUE;
      }
    } else if (strLine.RemovePrefix("Schedule:")) {
      CheckPVS_t(pvs);
      FixupFileName_t(strLine);
      pvs->vs_strSchedule = strLine;
    } else if (strLine.RemovePrefix("Tip:")) {
      CheckPVS_t(pvs);
      TranslateLine(strLine);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      pvs->vs_strTip = strLine;
    } else if (strLine.RemovePrefix("Var:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      pvs->vs_strVar = strLine;
    } else if (strLine.RemovePrefix("Filter:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      pvs->vs_strFilter = strLine;
    } else if (strLine.RemovePrefix("Slider:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      if (strLine=="Fill") {
        pvs->vs_iSlider = 1;
      } else if (strLine=="Ratio") {
        pvs->vs_iSlider = 2;
      } else {
        pvs->vs_iSlider = 0;
      }
    } else if (strLine.RemovePrefix("InGame:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      if( strLine=="No") {
        pvs->vs_bCanChangeInGame = FALSE;
      } else {
        ASSERT( strLine=="Yes");
        pvs->vs_bCanChangeInGame = TRUE;
      }
    } else if (strLine.RemovePrefix("String:")) {
      CheckPVS_t(pvs);
      TranslateLine(strLine);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      pvs->vs_astrTexts.Push() = strLine;
    } else if (strLine.RemovePrefix("Value:")) {
      CheckPVS_t(pvs);
      strLine.TrimSpacesLeft();
      strLine.TrimSpacesRight();
      pvs->vs_astrValues.Push() = strLine;
    } else {
      ThrowF_t(TRANS("unknown keyword"));
    }
  }
}


void LoadVarSettings(const CTFileName &fnmCfg)
{
  FlushVarSettings(FALSE);

  try {
    CTFileStream strm;
    strm.Open_t(fnmCfg);
    _ctLines = 0;
    _strFile = fnmCfg;
    ParseCFG_t(strm);

  } catch (const char* strError) {
    CPrintF("%s (%d) : %s\n", (const char*)_strFile, _ctLines, (const char *)strError);
  }

  FOREACHINLIST(CVarSetting, vs_lnNode, _lhVarSettings, itvs) {
    CVarSetting &vs = *itvs;
    if (!vs.Validate() || vs.vs_bSeparator) {
      continue;
    }
    INDEX ctValues = vs.vs_ctValues;
    CTString strValue = _pShell->GetValue(vs.vs_strVar);
    vs.vs_bCustom = TRUE;
    vs.vs_iOrgValue = vs.vs_iValue = -1;
    for(INDEX iValue=0; iValue<ctValues; iValue++) {
      if (strValue == vs.vs_astrValues[iValue]) {
        vs.vs_iOrgValue = vs.vs_iValue = iValue;
        vs.vs_bCustom = FALSE;
        break;
      }
    }
  }
}

void FlushVarSettings(BOOL bApply)
{
  CStaticStackArray<CTString> astrScheduled;

  if (bApply) {
    FOREACHINLIST(CVarSetting, vs_lnNode, _lhVarSettings, itvs) {
      CVarSetting &vs = *itvs;
      if (vs.vs_iValue!=vs.vs_iOrgValue) {
        CTString strCmd;
        _pShell->SetValue(vs.vs_strVar, vs.vs_astrValues[vs.vs_iValue]);

        if (vs.vs_strSchedule!="") {
          BOOL bSheduled = FALSE;
          for(INDEX i=0; i<astrScheduled.Count(); i++) {
            if (astrScheduled[i]==vs.vs_strSchedule) {
              bSheduled = TRUE;
              break;
            }
          }
          if (!bSheduled) {
            astrScheduled.Push() = vs.vs_strSchedule;
          }
        }
      }
    }
  }

  {FORDELETELIST(CVarSetting, vs_lnNode, _lhVarSettings, itvs) {
    delete &*itvs;
  }}

  for(INDEX i=0; i<astrScheduled.Count(); i++) {
    CTString strCmd;
    strCmd.PrintF("include \"%s\"", (const char *) astrScheduled[i]);
    _pShell->Execute(strCmd);
  }
}

CVarSetting::CVarSetting()
{
  Clear();
}

void CVarSetting::Clear()
{
  vs_iOrgValue = 0;
  vs_iValue = 0;
  vs_ctValues = 0;
  vs_bSeparator = FALSE;
  vs_bCanChangeInGame = TRUE;
  vs_iSlider = 0;
  vs_strName.Clear();
  vs_strTip.Clear();
  vs_strVar.Clear();
  vs_strFilter.Clear();
  vs_strSchedule.Clear();
  vs_bCustom = FALSE;
}

BOOL CVarSetting::Validate(void)
{
  if (vs_bSeparator) {
    return TRUE;
  }

  vs_ctValues = Min(vs_astrValues.Count(), vs_astrTexts.Count());
  if (vs_ctValues<=0) {
    ASSERT(FALSE);
    return FALSE;
  }
  if (!vs_bCustom) {
    vs_iValue = Clamp(vs_iValue, (INDEX)0, vs_ctValues-1);
  }
  return TRUE;
}

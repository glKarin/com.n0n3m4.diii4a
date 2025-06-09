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

// list of settings data
static CListHead _lhSettings;
extern INDEX sam_iVideoSetup;  // 0==speed, 1==normal, 2==quality, 3==custom

class CSettingsEntry {
public:
   CListNode se_lnNode;
    CTString se_strRenderer;
    CTString se_strDescription;
  CTFileName se_fnmScript;
  // check if this entry matches given info
  BOOL Matches( const CTString &strRenderer) const;
};

// last valid settings info
static CTString _strLastRenderer;
CTString _strPreferencesDescription = "";
INDEX    _iLastPreferences = 1;

// check if this entry matches given info
BOOL CSettingsEntry::Matches( const CTString &strRenderer) const
{
  return strRenderer.Matches(se_strRenderer);
}


const char *RenderingPreferencesDescription(int iMode)
{
  if (iMode==0) {
    return TRANS("speed");
  } else if (iMode==1) {
    return TRANS("normal");
  } else if (iMode==2) {
    return TRANS("quality");
  } else if (iMode==3) {
    return TRANS("custom");
  } else {
    ASSERT(FALSE);
    return TRANS("normal");
  }
}

void InitGLSettings(void)
{
  ASSERT(_lhSettings.IsEmpty());

  char achrLine    [1025];
  char achrRenderer[1025];
  char achrDesc    [1025];
  char achrScript  [1025];

  CTFileStream strmFile;
  try
  {
    strmFile.Open_t( CTString("Scripts\\GLSettings\\GLSettings.lst"), CTStream::OM_READ);
    //INDEX iIndex = 0;
	  do
    {
      achrLine    [0] = 0;
      achrRenderer[0] = 0;
      achrDesc    [0] = 0;
      achrScript  [0] = 0;
      strmFile.GetLine_t( achrLine, 1024);
      sscanf( achrLine,
        "\"%1024[^\"]\"%*[^\"]\"%1024[^\"]\"%*[^\"]\"%1024[^\"]\"", achrRenderer, achrDesc, achrScript);
      if( achrRenderer[0]==0) continue;

      CSettingsEntry &se = *new CSettingsEntry;
      se.se_strRenderer    = achrRenderer;
      se.se_strDescription = achrDesc;
      se.se_fnmScript      = CTString(achrScript);
      _lhSettings.AddTail( se.se_lnNode);
    }
  	while( !strmFile.AtEOF());
  }

  // ignore errors
  catch (const char *strError) { 
    WarningMessage(TRANS("unable to setup OpenGL settings list: %s"), (const char *) strError);
  }

  _strLastRenderer= "none";
  _iLastPreferences = 1;
  _pShell->DeclareSymbol("persistent CTString sam_strLastRenderer;", (void *) &_strLastRenderer);
  _pShell->DeclareSymbol("persistent INDEX    sam_iLastSetup;",      (void *) &_iLastPreferences);
}


CSettingsEntry *GetGLSettings( const CTString &strRenderer)
{
  // for each setting
  FOREACHINLIST(CSettingsEntry, se_lnNode, _lhSettings, itse)
  { // return the one that matches
    CSettingsEntry &se = *itse;
    if( se.Matches(strRenderer)) return &se;
  }
  // none found
  return NULL;
}


extern void ApplyGLSettings(BOOL bForce)
{
  CPrintF( TRANS("\nAutomatic 3D-board preferences adjustment...\n"));
  CDisplayAdapter &da = _pGfx->gl_gaAPI[_pGfx->gl_eCurrentAPI].ga_adaAdapter[_pGfx->gl_iCurrentAdapter];
  CPrintF( TRANS("Detected: %s - %s - %s\n"), (const char *) da.da_strVendor, (const char *) da.da_strRenderer, (const char *) da.da_strVersion);

  // get new settings
  CSettingsEntry *pse = GetGLSettings( da.da_strRenderer);

  // if none found
  if (pse==NULL) {
    // error
    CPrintF(TRANSV("No matching preferences found! Automatic adjustment disabled!\n"));
    return;
  }

  // report
  CPrintF(TRANSV("Matching: %s (%s)\n"),
            (const char *) pse->se_strRenderer,
            (const char *) pse->se_strDescription);

  _strPreferencesDescription = pse->se_strDescription;

  if (!bForce) {
    // if same as last
    if( pse->se_strDescription==_strLastRenderer && sam_iVideoSetup==_iLastPreferences) {
      // do nothing
      CPrintF(TRANSV("Similar to last, keeping same preferences.\n"));
      return;
    }
    CPrintF(TRANSV("Different than last, applying new preferences.\n"));
  } else {
    CPrintF(TRANSV("Applying new preferences.\n"));
  } 

  // clamp rendering preferences (just to be on the safe side)
  sam_iVideoSetup = Clamp( sam_iVideoSetup, (INDEX)0, (INDEX)3);
  CPrintF(TRANSV("Mode: %s\n"), (const char *) RenderingPreferencesDescription(sam_iVideoSetup));
  // if not in custom mode
  if (sam_iVideoSetup<3) {
    // execute the script
    CTString strCmd;
    strCmd.PrintF("include \"Scripts\\GLSettings\\%s\"", (const char *) (CTString(pse->se_fnmScript)));
    _pShell->Execute(strCmd);
    // refresh textures
    _pShell->Execute("RefreshTextures();");
  }
  // done
  CPrintF(TRANSV("Done.\n\n"));

  // remember settings
  _strLastRenderer = pse->se_strDescription; 
  _iLastPreferences = sam_iVideoSetup;
}

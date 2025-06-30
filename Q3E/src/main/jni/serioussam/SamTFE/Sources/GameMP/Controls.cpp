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

/*
 *  Game library
 *  Copyright (c) 1997-1999, CroTeam. 
 */

#include "StdAfx.h"

extern CGame *_pGame;

/*
 * Default constructor
 */
CControls::CControls(void)
{
  // buttons are all none anyway (empty list)
  // switch axes actions to defaults
  SwitchAxesToDefaults();
}

/*
 * Default destructor
 */
CControls::~CControls(void)
{
  // for each action in the original list
  FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction)
  {
    // delete button action
    delete( &*itButtonAction);
  }
}

// Assignment operator.
CControls &CControls::operator=(CControls &ctrlOriginal)
{
  // remove old button actions
  {FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction) {
    delete &*itButtonAction;
  }}
  // for each action in the original list
  {FOREACHINLIST(CButtonAction, ba_lnNode, ctrlOriginal.ctrl_lhButtonActions, itButtonAction) {
    // create and copy button action
    AddButtonAction() = *itButtonAction;
  }}

  // for all axis-type actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++) {
    // copy it
    ctrl_aaAxisActions[iAxisAction] = ctrlOriginal.ctrl_aaAxisActions[iAxisAction];
  }

  // copy global settings
  ctrl_fSensitivity  = ctrlOriginal.ctrl_fSensitivity;
  ctrl_bInvertLook   = ctrlOriginal.ctrl_bInvertLook;
  ctrl_bSmoothAxes   = ctrlOriginal.ctrl_bSmoothAxes;

  return *this;
}

/*
 * Switches button and axis action mounters to defaults
 */
void CControls::SwitchAxesToDefaults(void)
{
  // for all axis-type actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++)
  {
    // set axis action none
    ctrl_aaAxisActions[ iAxisAction].aa_iAxisAction = AXIS_NONE;
    ctrl_aaAxisActions[ iAxisAction].aa_fSensitivity = 50;
    ctrl_aaAxisActions[ iAxisAction].aa_fDeadZone = 0;
    ctrl_aaAxisActions[ iAxisAction].aa_bInvert = FALSE;
    ctrl_aaAxisActions[ iAxisAction].aa_bRelativeControler = TRUE;
    ctrl_aaAxisActions[ iAxisAction].aa_bSmooth = FALSE;
    ctrl_aaAxisActions[ iAxisAction].aa_fLastReading = 0.0f;
  }
  // set default controlers for some axis-type actions
  // mouse left/right motion
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_iAxisAction = MOUSE_X_AXIS;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_fSensitivity = 45;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_bInvert = FALSE;
  ctrl_aaAxisActions[ AXIS_TURN_LR].aa_bRelativeControler = TRUE;
  // mouse up/down motion
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_iAxisAction = MOUSE_Y_AXIS;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_fSensitivity = 45;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_bInvert = TRUE;
  ctrl_aaAxisActions[ AXIS_TURN_UD].aa_bRelativeControler = TRUE;

  ctrl_fSensitivity = 50;
  ctrl_bInvertLook = FALSE;
  ctrl_bSmoothAxes = TRUE;
}

void CControls::SwitchToDefaults(void)
{
  // copy controls from initial player
  try
  {
    CControls ctrlDefaultControls;
    ctrlDefaultControls.Load_t( CTString("Data\\Defaults\\InitialControls.ctl"));
    *this = ctrlDefaultControls;
  }
  catch (const char *strError)
  {
    (void) strError;
  }
}

/*
 * Depending on axis attributes and type (rotation or translation),
 * calculates axis influence factors for all axis actions
 */
void CControls::CalculateInfluencesForAllAxis(void)
{
  FLOAT fSensitivityGlobal = (FLOAT)pow(1.2, (ctrl_fSensitivity-50.0)*1.0/5.0);
  FLOAT fBaseDelta; // different for rotations and translations
  // for all axis actions
  for( INDEX iAxisAction=0; iAxisAction<AXIS_ACTIONS_CT; iAxisAction++)
  {
    fBaseDelta = 1.0f;
    // apply invert factor
    if( ctrl_aaAxisActions[ iAxisAction].aa_bInvert || 
      ( (iAxisAction==AXIS_TURN_UD||iAxisAction==AXIS_LOOK_UD) && ctrl_bInvertLook) ) {
      // negative factor
      fBaseDelta = -fBaseDelta;
    }

    FLOAT fSensitivityLocal = (FLOAT)pow(2.0, (ctrl_aaAxisActions[ iAxisAction].aa_fSensitivity-50.0)*1.0/5.0);

    // calculate influence for current axis
    ctrl_aaAxisActions[ iAxisAction].aa_fAxisInfluence = fBaseDelta * fSensitivityGlobal * fSensitivityLocal;
  }
}

INDEX DIKForName( CTString strKeyName)
{
  if( strKeyName == "None") return KID_NONE;
  for( INDEX iButton=0; iButton<MAX_OVERALL_BUTTONS; iButton++)
  {
    if( _pInput->GetButtonName( iButton) == strKeyName) return iButton;
  }
  return KID_NONE;
}

CTString ReadTextLine(CTStream &strm, const CTString &strKeyword, BOOL bTranslate)
{
  CTString strLine;
  strm.GetLine_t(strLine);
  strLine.TrimSpacesLeft();
  if (!strLine.RemovePrefix(strKeyword)) {
    return "???";
  }
  strLine.TrimSpacesLeft();
  if (bTranslate) {
    strLine.RemovePrefix("TTRS");
  }
  strLine.TrimSpacesLeft();
  strLine.TrimSpacesRight();

  return strLine;
}

void CControls::Load_t( CTFileName fnFile)
{
  char achrLine[ 1025];
  char achrName[ 1025];
  char achrID[ 1025];
  char achrActionName[ 1025];
  
  // open script file for reading
  CTFileStream strmFile;
  strmFile.Open_t( fnFile);				

  // if file can be opened for reading remove old button actions
  {FORDELETELIST(CButtonAction, ba_lnNode, ctrl_lhButtonActions, itButtonAction) {
    delete &*itButtonAction;
  }}

	do
  {
    achrLine[0] = 0;
    achrID[0] = 0;
    strmFile.GetLine_t( achrLine, 1024);
    sscanf( achrLine, "%s", achrID);
    // if name
    if( CTString( achrID) == "Name") {
      // name is obsolete, just skip it
      sscanf( achrLine, "%*[^\"]\"%1024[^\"]\"", achrName);
    
    // if this is button action
    } else if( CTString( achrID) == "Button") {
      // create and read button action
      CButtonAction &baNew = AddButtonAction();
      baNew.ba_strName = ReadTextLine(strmFile, "Name:", TRUE);

      baNew.ba_iFirstKey = DIKForName( ReadTextLine(strmFile, "Key1:", FALSE));
      baNew.ba_iSecondKey = DIKForName( ReadTextLine(strmFile, "Key2:", FALSE));
      
      baNew.ba_strCommandLineWhenPressed = ReadTextLine(strmFile, "Pressed:", FALSE);
      baNew.ba_strCommandLineWhenReleased = ReadTextLine(strmFile, "Released:", FALSE);

    // if this is axis action
    } else if( CTString( achrID) == "Axis") {
      char achrAxis[ 1025];
      achrAxis[ 0] = 0;
      char achrIfInverted[ 1025];
      achrIfInverted[ 0] = 0;
      char achrIfRelative[ 1025];
      achrIfRelative[ 0] = 0;
      //char achrIfSmooth[ 1025];
      //achrIfSmooth[ 0] = 0;
      achrActionName[ 0] = 0;
      FLOAT fSensitivity = 50;
      FLOAT fDeadZone = 0;
      // FIXME DG: if achrIfSmooth should be read, add another %1024s - but it seems like achrIfSmooth
      //           is unused - below achrIfRelative is compared to "Smooth"?!
      sscanf( achrLine, "%*[^\"]\"%1024[^\"]\"%*[^\"]\"%1024[^\"]\" %g %g %1024s %1024s",
              achrActionName, achrAxis, &fSensitivity, &fDeadZone, achrIfInverted, achrIfRelative /*, achrIfSmooth*/);
      // find action axis
      INDEX iActionAxisNo = -1;
      {for( INDEX iAxis=0; iAxis<AXIS_ACTIONS_CT; iAxis++){
        if( CTString(_pGame->gm_astrAxisNames[iAxis]) == achrActionName)
        {
          iActionAxisNo = iAxis;
          break;
        }
      }}
      // find controller axis
      INDEX iCtrlAxisNo = -1;
      {for( INDEX iAxis=0; iAxis<MAX_OVERALL_AXES; iAxis++) {
        if( _pInput->GetAxisName( iAxis) == achrAxis)
        {
          iCtrlAxisNo = iAxis;
          break;
        }
      }}
      // if valid axis found
      if( iActionAxisNo!=-1 && iCtrlAxisNo!=-1) {
        // set it
        ctrl_aaAxisActions[ iActionAxisNo].aa_iAxisAction = iCtrlAxisNo;
        ctrl_aaAxisActions[ iActionAxisNo].aa_fSensitivity = fSensitivity;
        ctrl_aaAxisActions[ iActionAxisNo].aa_fDeadZone = fDeadZone;
        ctrl_aaAxisActions[ iActionAxisNo].aa_bInvert = ( CTString( "Inverted") == achrIfInverted);
        ctrl_aaAxisActions[ iActionAxisNo].aa_bRelativeControler = ( CTString( "Relative") == achrIfRelative);
        ctrl_aaAxisActions[ iActionAxisNo].aa_bSmooth = ( CTString( "Smooth") == achrIfRelative);
      }
    // read global parameters
    } else if( CTString( achrID) == "GlobalInvertLook") {
      ctrl_bInvertLook = TRUE;
    } else if( CTString( achrID) == "GlobalDontInvertLook") {
      ctrl_bInvertLook = FALSE;
    } else if( CTString( achrID) == "GlobalSmoothAxes") {
      ctrl_bSmoothAxes = TRUE;
    } else if( CTString( achrID) == "GlobalDontSmoothAxes") {
      ctrl_bSmoothAxes = FALSE;
    } else if( CTString( achrID) == "GlobalSensitivity") {
      sscanf( achrLine, "GlobalSensitivity %g", &ctrl_fSensitivity);
    }
  }
	while( !strmFile.AtEOF());

/*
  // search for talk button
  BOOL bHasTalk = FALSE;
  BOOL bHasT = FALSE;
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;
    if (ba.ba_strName=="Talk") {
      bHasTalk = TRUE;
    }
    if (ba.ba_iFirstKey==KID_T) {
      bHasT = TRUE;
    }
    if (ba.ba_iSecondKey==KID_T) {
      bHasT = TRUE;
    }
  }
  // if talk button not found
  if (!bHasTalk) {
    // add it
    CButtonAction &baNew = AddButtonAction();
    baNew.ba_strName = "Talk";
    baNew.ba_iFirstKey = KID_NONE;
    baNew.ba_iSecondKey = KID_NONE;
    baNew.ba_strCommandLineWhenPressed = "  con_bTalk=1;";
    baNew.ba_strCommandLineWhenReleased = "";
    // if T key is not bound to anything
    if (!bHasT) {
      // bind it to talk
      baNew.ba_iFirstKey = KID_T;
    // if we couldn't bind it
    } else {
      // put it to the top of the list
      baNew.ba_lnNode.Remove();
      ctrl_lhButtonActions.AddHead(baNew.ba_lnNode);
    }
  }
  */

  CalculateInfluencesForAllAxis();
}

void CControls::Save_t( CTFileName fnFile)
{
  CTString strLine;
  // create file
  CTFileStream strmFile;
  strmFile.Create_t( fnFile, CTStream::CM_TEXT);

  // write button actions
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba)
  {
    strLine.PrintF("Button\n Name: TTRS %s\n Key1: %s\n Key2: %s",
      (const char *) itba->ba_strName,
      (const char *) _pInput->GetButtonName( itba->ba_iFirstKey),
      (const char *) _pInput->GetButtonName( itba->ba_iSecondKey) );
    strmFile.PutLine_t( strLine);

    // export pressed command
    strLine.PrintF(" Pressed:  %s", (const char *) itba->ba_strCommandLineWhenPressed);
    {for( INDEX iLetter = 0; strLine[ iLetter] != 0; iLetter++)
    {
      // delete EOL-s
      if( (strLine[ iLetter] == 0x0d) || (strLine[ iLetter] == 0x0a) )
      {
        ((char*)(const char*)strLine)[ iLetter] = ' ';
      }
    }}
    strmFile.PutLine_t( strLine);

    // export released command
    strLine.PrintF(" Released: %s", (const char *) itba->ba_strCommandLineWhenReleased);
    {for( INDEX iLetter = 0; strLine[ iLetter] != 0; iLetter++)
    {
      // delete EOL-s
      if( (strLine[ iLetter] == 0x0d) || (strLine[ iLetter] == 0x0a) )
      {
        ((char*)(const char*)strLine)[ iLetter] = ' ';
      }
    }}
    strmFile.PutLine_t( strLine);
  }
  // write axis actions
  for( INDEX iAxis=0; iAxis<AXIS_ACTIONS_CT; iAxis++)
  {
    CTString strIfInverted;
    CTString strIfRelative;
    CTString strIfSmooth;

    if( ctrl_aaAxisActions[iAxis].aa_bInvert) {
      strIfInverted = "Inverted";
    } else {
      strIfInverted = "NotInverted";
    }
    if( ctrl_aaAxisActions[iAxis].aa_bRelativeControler) {
      strIfRelative = "Relative";
    } else {
      strIfRelative = "Absolute";
    }
    if( ctrl_aaAxisActions[iAxis].aa_bSmooth) {
      strIfSmooth = "Smooth";
    } else {
      strIfSmooth = "NotSmooth";
    }
    

    strLine.PrintF("Axis \"%s\" \"%s\" %g %g %s %s %s",
      (const char *) _pGame->gm_astrAxisNames[iAxis], 
      (const char *) _pInput->GetAxisName(ctrl_aaAxisActions[iAxis].aa_iAxisAction),
      ctrl_aaAxisActions[ iAxis].aa_fSensitivity,
      ctrl_aaAxisActions[ iAxis].aa_fDeadZone,
      (const char *) strIfInverted,
      (const char *) strIfRelative,
      (const char *) strIfSmooth);
    strmFile.PutLine_t( strLine);
  }

  // write global parameters
  if (ctrl_bInvertLook) {
    strmFile.PutLine_t( "GlobalInvertLook");
  } else {
    strmFile.PutLine_t( "GlobalDontInvertLook");
  }
  if (ctrl_bSmoothAxes) {
    strmFile.PutLine_t( "GlobalSmoothAxes");
  } else {
    strmFile.PutLine_t( "GlobalDontSmoothAxes");
  }
  strmFile.FPrintF_t("GlobalSensitivity %g\n", ctrl_fSensitivity);
}

// check if these controls use any joystick
BOOL CControls::UsesJoystick(void)
{
  // for each button
  FOREACHINLIST( CButtonAction, ba_lnNode, ctrl_lhButtonActions, itba) {
    CButtonAction &ba = *itba;
    if (ba.ba_iFirstKey>=FIRST_JOYBUTTON || ba.ba_iSecondKey>=FIRST_JOYBUTTON) {
      return TRUE;
    }
  }

    // write axis actions
  for( INDEX iAxis=0; iAxis<AXIS_ACTIONS_CT; iAxis++)
  {
    if (ctrl_aaAxisActions[iAxis].aa_iAxisAction>=FIRST_JOYAXIS) {
      return TRUE;
    }
  }

  return FALSE;
}


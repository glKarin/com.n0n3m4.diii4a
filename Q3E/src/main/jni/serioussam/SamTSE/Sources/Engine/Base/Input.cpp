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

#include "Engine/StdH.h"

#include <Engine/Base/Timer.h>
#include <Engine/Base/Input.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/KeyNames.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Synchronization.h>

#include <Engine/Base/ErrorReporting.h>

/*
 * rcg10072001
 * The bulk of CInput's implementation is in the platform-specific subdirs.
 *  For example, if you want the win32 implementation, look in
 *  Engine\Base\win32\Win32Input.cpp ...
 */

INDEX inp_ctJoysticksAllowed = 8;

static CTString inp_astrAxisTran[MAX_OVERALL_AXES];// translated names for axis

// pointer to global input object
CInput *_pInput = NULL;

// deafult constructor
CInput::CInput(void)
{
  // disable control scaning
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;
  inp_bLastPrescan = FALSE;
  // clear key buffer
  for( INDEX iButton=0; iButton<MAX_OVERALL_BUTTONS; iButton++)
  {
    inp_ubButtonsBuffer[ iButton] = 0;
  }
  // clear axis relative and absolute values
  for( INDEX iAxis=0; iAxis<MAX_OVERALL_AXES; iAxis++)
  {
    inp_caiAllAxisInfo[ iAxis].cai_fReading  = 0.0f;
    inp_caiAllAxisInfo[ iAxis].cai_bExisting = FALSE;
  }

  PlatformInit();
}


void CInput::SetJoyPolling(BOOL bPoll)
{
  inp_bPollJoysticks = bPoll;
}

/*
 * Sets names of keys on keyboard
 */
void CInput::SetKeyNames( void)
{
  // set name "None" for all keys, known keys will override this default name
  for( INDEX iKey=0; iKey<static_cast<INDEX>(ARRAYCOUNT(inp_strButtonNames)); iKey++) {
    inp_strButtonNames[iKey] = "None";
    inp_strButtonNamesTra[iKey] = TRANS("None");
  }

  PlatformSetKeyNames();

  // -------- Enumerate known axis -------------
  // no axis as axis type 0
  inp_caiAllAxisInfo[0].cai_strAxisName = "None";
  inp_astrAxisTran[  0] = TRANS("None");
  // mouse axis occupy types from 1 up to 3
  inp_caiAllAxisInfo[1].cai_strAxisName = "mouse X";
  inp_astrAxisTran[  1] = TRANS("mouse X");
  inp_caiAllAxisInfo[2].cai_strAxisName = "mouse Y";
  inp_astrAxisTran[  2] = TRANS("mouse Y");
  inp_caiAllAxisInfo[3].cai_strAxisName = "mouse Z";
  inp_astrAxisTran[  3] = TRANS("mouse Z");
  inp_caiAllAxisInfo[4].cai_strAxisName = "2nd mouse X";
  inp_astrAxisTran[  4] = TRANS("2nd mouse X");
  inp_caiAllAxisInfo[5].cai_strAxisName = "2nd mouse Y";
  inp_astrAxisTran[  5] = TRANS("2nd mouse Y");

  // -------- Get number of joysticks ----------
  // get number of joystics
  INDEX ctJoysticksPresent = PlatformGetJoystickCount();
  CPrintF(TRANSV("  joysticks found: %d\n"), ctJoysticksPresent);
  ctJoysticksPresent = Min(ctJoysticksPresent, inp_ctJoysticksAllowed);
  CPrintF(TRANSV("  joysticks allowed: %d\n"), ctJoysticksPresent);

  // -------- Enumerate axis and buttons for joysticks ----------
  for (INDEX iJoy=0; iJoy<MAX_JOYSTICKS; iJoy++) {
    inp_abJoystickOn[iJoy] = FALSE;
    if (iJoy<ctJoysticksPresent && CheckJoystick(iJoy)) {
      inp_abJoystickOn[iJoy] = TRUE;
    }
    AddJoystickAbbilities(iJoy);
  }
}

#ifdef PLATFORM_WIN32
// check if a joystick exists
BOOL CInput::CheckJoystick(INDEX iJoy)
{
  CPrintF(TRANS("  joy %d:"), iJoy+1);

  JOYCAPS jc;
  // seek for capabilities of requested joystick
  MMRESULT mmResult = joyGetDevCaps( JOYSTICKID1+iJoy,	&jc, sizeof(JOYCAPS));
  // report possible errors
  if( mmResult == MMSYSERR_NODRIVER) {
    CPrintF(TRANS(" joystick driver is not present\n"));
    return FALSE;
  } else if( mmResult == MMSYSERR_INVALPARAM) {
    CPrintF(TRANS(" invalid parameter\n"));
    return FALSE;
  } else if( mmResult != JOYERR_NOERROR) {
    CPrintF(TRANS("  error 0x%08x\n"), mmResult);
    return FALSE;
  }
  CPrintF(" '%s'\n", jc.szPname);

  CPrintF(TRANS("    %d axes\n"), jc.wNumAxes);
  CPrintF(TRANS("    %d buttons\n"), jc.wNumButtons);
  if (jc.wCaps&JOYCAPS_HASPOV) {
    CPrintF(TRANS("    POV hat present\n"));
    inp_abJoystickHasPOV[iJoy] = TRUE;
  } else {
    inp_abJoystickHasPOV[iJoy] = FALSE;
  }

  // read joystick state
  JOYINFOEX ji;
  ji.dwFlags = JOY_RETURNBUTTONS|JOY_RETURNCENTERED|JOY_RETURNPOV|JOY_RETURNR|
    JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_RETURNU|JOY_RETURNV;
  ji.dwSize = sizeof( JOYINFOEX);
  mmResult = joyGetPosEx( JOYSTICKID1+iJoy, &ji);

  // if some error
  if( mmResult != JOYERR_NOERROR) {
    // fail
    CPrintF(TRANS("    Cannot read the joystick!\n"));
    return FALSE;
  }

  // for each axis
  for(INDEX iAxis=0; iAxis<MAX_AXES_PER_JOYSTICK; iAxis++) {
    ControlAxisInfo &cai= inp_caiAllAxisInfo[ FIRST_JOYAXIS+iJoy*MAX_AXES_PER_JOYSTICK+iAxis];
    // remember min/max info
    switch( iAxis) {
    case 0: 
      cai.cai_slMin = jc.wXmin; cai.cai_slMax = jc.wXmax; 
      cai.cai_bExisting = TRUE;
      break;
    case 1: 
      cai.cai_slMin = jc.wYmin; cai.cai_slMax = jc.wYmax; 
      cai.cai_bExisting = TRUE;
      break;
    case 2: 
      cai.cai_slMin = jc.wZmin; cai.cai_slMax = jc.wZmax; 
      cai.cai_bExisting = jc.wCaps&JOYCAPS_HASZ;
      break;
    case 3: 
      cai.cai_slMin = jc.wRmin; cai.cai_slMax = jc.wRmax; 
      cai.cai_bExisting = jc.wCaps&JOYCAPS_HASR;
      break;
    case 4: 
      cai.cai_slMin = jc.wUmin; cai.cai_slMax = jc.wUmax; 
      cai.cai_bExisting = jc.wCaps&JOYCAPS_HASU;
      break;
    case 5: 
      cai.cai_slMin = jc.wVmin; cai.cai_slMax = jc.wVmax; 
      cai.cai_bExisting = jc.wCaps&JOYCAPS_HASV;
      break;
    }
  }

  return TRUE;
}
#endif

// adds axis and buttons for given joystick
void CInput::AddJoystickAbbilities(INDEX iJoy)
{
  CTString strJoystickName;
  strJoystickName.PrintF("Joy %d", iJoy+1);
  CTString strJoystickNameTra;
  strJoystickNameTra.PrintF(TRANSV("Joy %d"), iJoy+1);

  // for each axis
  for( UINT iAxis=0; iAxis<6; iAxis++) {
    INDEX iAxisTotal = FIRST_JOYAXIS+iJoy*MAX_AXES_PER_JOYSTICK+iAxis;
    ControlAxisInfo &cai= inp_caiAllAxisInfo[iAxisTotal];
    CTString &strTran = inp_astrAxisTran[iAxisTotal];
    // set axis name
    switch( iAxis) {
    case 0: cai.cai_strAxisName = strJoystickName + " Axis X"; strTran = strJoystickNameTra + TRANS(" Axis X"); break;
    case 1: cai.cai_strAxisName = strJoystickName + " Axis Y"; strTran = strJoystickNameTra + TRANS(" Axis Y"); break;
    case 2: cai.cai_strAxisName = strJoystickName + " Axis Z"; strTran = strJoystickNameTra + TRANS(" Axis Z"); break;
    case 3: cai.cai_strAxisName = strJoystickName + " Axis R"; strTran = strJoystickNameTra + TRANS(" Axis R"); break;
    case 4: cai.cai_strAxisName = strJoystickName + " Axis U"; strTran = strJoystickNameTra + TRANS(" Axis U"); break;
    case 5: cai.cai_strAxisName = strJoystickName + " Axis V"; strTran = strJoystickNameTra + TRANS(" Axis V"); break;
    }
  }

  INDEX iButtonTotal = FIRST_JOYBUTTON+iJoy*MAX_BUTTONS_PER_JOYSTICK;
  // add buttons that the joystick supports
  for( UINT iButton=0; iButton<32; iButton++) {
    CTString strButtonName;
    CTString strButtonNameTra;
    // create name for n-th button
    strButtonName.PrintF( " Button %d", iButton);
    strButtonNameTra.PrintF( TRANS(" Button %d"), iButton);
    // set n-th button name
    inp_strButtonNames[iButtonTotal]    = strJoystickName + strButtonName;
    inp_strButtonNamesTra[iButtonTotal] = strJoystickNameTra + strButtonNameTra;
    iButtonTotal++;
  }

  // add the four POV buttons
  inp_strButtonNames   [ iButtonTotal  ] = strJoystickName    +      (" POV N");
  inp_strButtonNamesTra[ iButtonTotal++] = strJoystickNameTra + TRANS(" POV N");
  inp_strButtonNames   [ iButtonTotal  ] = strJoystickName    +      (" POV E");
  inp_strButtonNamesTra[ iButtonTotal++] = strJoystickNameTra + TRANS(" POV E");
  inp_strButtonNames   [ iButtonTotal  ] = strJoystickName    +      (" POV S");
  inp_strButtonNamesTra[ iButtonTotal++] = strJoystickNameTra + TRANS(" POV S");
  inp_strButtonNames   [ iButtonTotal  ] = strJoystickName    +      (" POV W");
  inp_strButtonNamesTra[ iButtonTotal++] = strJoystickNameTra + TRANS(" POV W");
}

/*
 * Initializes all available devices and enumerates available controls
 */
void CInput::Initialize( void )
{
  CPrintF(TRANSV("Detecting input devices...\n"));
  SetKeyNames();
  CPrintF("\n");
}


/*
 * Enable direct input
 */

void CInput::EnableInput(CViewPort *pvp)
{
  EnableInput(pvp->vp_hWnd);
}



// Clear all input states (keys become not pressed, axes are reset to zero)
void CInput::ClearInput( void)
{
  // clear button's buffer
  memset( inp_ubButtonsBuffer, 0, sizeof( inp_ubButtonsBuffer));
  // clear axis values
  for (INDEX i=0; i<MAX_OVERALL_AXES; i++) {
    inp_caiAllAxisInfo[i].cai_fReading = 0;
  }
}

const CTString &CInput::GetAxisTransName( INDEX iAxisNo) const
{
  return inp_astrAxisTran[iAxisNo];
}

// end of Input.cpp ...



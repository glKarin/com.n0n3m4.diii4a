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

#ifndef SE_INCL_INPUT_H
#define SE_INCL_INPUT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/CTString.h>

// number of key ids reserved (in KeyNames.h)
#define KID_TOTALCOUNT 256

// defines for offsets of empty axis ("NONE" key) and mouse axis
#define AXIS_NONE    0
#define MOUSE_X_AXIS 1
#define MOUSE_Y_AXIS 2

#define MAX_JOYSTICKS 8
#define MAX_AXES_PER_JOYSTICK 6 // (XYZRUV)
#define FIRST_JOYAXIS (1+3+2)   // one dummy, 3 axis for windows mouse (3rd is scroller), 2 axis for serial mouse
#define MAX_OVERALL_AXES (FIRST_JOYAXIS+MAX_JOYSTICKS*MAX_AXES_PER_JOYSTICK)
#define MAX_BUTTONS_PER_JOYSTICK (32+4) // 32 buttons and 4 POV directions
#define FIRST_JOYBUTTON (KID_TOTALCOUNT)
#define MAX_OVERALL_BUTTONS (KID_TOTALCOUNT+MAX_JOYSTICKS*MAX_BUTTONS_PER_JOYSTICK)


/*
 *  Mouse speed control structure
 */
struct MouseSpeedControl
{
  int msc_iThresholdX;
  int msc_iThresholdY;
  int msc_iSpeed;
};

/*
 * One axis descriptive information
 */
struct ENGINE_API ControlAxisInfo
{
  CTString cai_strAxisName;           // name of this axis
  FLOAT cai_fReading;                 // current reading of this axis
  BOOL  cai_bExisting; // set if the axis exists (for joystick axes)
  SLONG cai_slMax;    // max/min info for joysticks
  SLONG cai_slMin;
};

/*
 * Class responsible for dealing with DirectInput
 */
class ENGINE_API CInput {
public:
// Attributes

  BOOL inp_bLastPrescan;
  BOOL inp_bInputEnabled;
  BOOL inp_bPollJoysticks;
  struct ControlAxisInfo inp_caiAllAxisInfo[ MAX_OVERALL_AXES];// info for all available axis
  CTString inp_strButtonNames[ MAX_OVERALL_BUTTONS];// individual button names
  CTString inp_strButtonNamesTra[ MAX_OVERALL_BUTTONS];// individual button names (translated)
  UBYTE inp_ubButtonsBuffer[ MAX_OVERALL_BUTTONS];  // statuses for all buttons (KEY & 128 !=0)
  BOOL inp_abJoystickOn[MAX_JOYSTICKS];  // set if a joystick is valid for reading
  BOOL inp_abJoystickHasPOV[MAX_JOYSTICKS];  // set if a joystick has a POV hat

  SLONG inp_slScreenCenterX;                        // screen center X in pixels
  SLONG inp_slScreenCenterY;                        // screen center Y in pixels
  RECT inp_rectOldClip;                             // old cursor clip rectangle in pixels
  POINT inp_ptOldMousePos;                          // old mouse position
  struct MouseSpeedControl inp_mscMouseSettings;    // system mouse settings

  void SetKeyNames( void);                          // sets name for every key
  // check if a joystick exists
  BOOL CheckJoystick(INDEX iJoy);
  // adds axis and buttons for given joystick
  void AddJoystickAbbilities(INDEX iJoy);
  BOOL ScanJoystick(INDEX iJoyNo, BOOL bPreScan);// scans axis and buttons for given joystick
public:
// Operations
  CInput();
  ~CInput();
  // Initializes all available devices and enumerates available controls
  void Initialize(void);
  // Enable input inside one viewport, or window
  void EnableInput(CViewPort *pvp);
  void EnableInput(HWND hWnd);
  // Disable input
  void DisableInput(void);
  // enable/disable joystick polling (it can be slow to poll if user doesn't realy use the joystick)
  void SetJoyPolling(BOOL bPoll);
  // Test input activity
  BOOL IsInputEnabled( void) const { return inp_bInputEnabled; };
  // Scan states of all available input sources
  void GetInput(BOOL bPreScan);
  // Clear all input states (keys become not pressed, axes are reset to zero)
  void ClearInput( void);
  // Get count of available axis
  inline const INDEX GetAvailableAxisCount( void) const {
    return MAX_OVERALL_AXES;};
  // Get count of available buttons
  inline const INDEX GetAvailableButtonsCount( void) const {
    return MAX_OVERALL_BUTTONS;};
  // Get name of given axis
  inline const CTString &GetAxisName( INDEX iAxisNo) const {
    return inp_caiAllAxisInfo[ iAxisNo].cai_strAxisName;};
  const CTString &GetAxisTransName( INDEX iAxisNo) const;
  // Get current position of given axis
  inline float GetAxisValue( INDEX iAxisNo) const {
    return inp_caiAllAxisInfo[ iAxisNo].cai_fReading;};
  // Get given button's name
  inline const CTString &GetButtonName( INDEX iButtonNo) const {
    return inp_strButtonNames[ iButtonNo];};
  // Get given button's name translated
  inline const CTString &GetButtonTransName( INDEX iButtonNo) const {
    return inp_strButtonNamesTra[ iButtonNo];};
  // Get given button's current state
  inline BOOL GetButtonState( INDEX iButtonNo) const {
    return (inp_ubButtonsBuffer[ iButtonNo] & 128) != 0;};

  // rcg02042003 hack for SDL vs. Win32.
  void ClearRelativeMouseMotion(void);

protected:
  BOOL PlatformInit(void); /* rcg10072001 platform-specific construction */
  BOOL PlatformSetKeyNames(void); /* rcg10072001 platform-specific code */
  LONG PlatformGetJoystickCount(void); /* rcg11242001 platform-specific code */
};

// pointer to global input object
ENGINE_API extern CInput *_pInput;


#endif  /* include-once check. */


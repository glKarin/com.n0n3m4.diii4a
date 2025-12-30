//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//


#include "wl_def.h"
#include "c_cvars.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "config.h"
#include "wl_play.h"


/*
=============================================================================

					GLOBAL VARIABLES

=============================================================================
*/


//
// configuration variables
//
bool MousePresent;
bool MouseWheel[4];

// 	Global variables
unsigned short Paused;
char LastASCII;
ScanCode LastScan;

JoystickSens *JoySensitivity;
int JoyNumButtons;
int JoyNumAxes;

void IN_GetJoyDelta(int *dx,int *dy)
{
}

int IN_GetJoyAxis(int axis)
{
  return 0;
}


int IN_JoyButtons()
{
  return 0;
}

int IN_JoyAxes()
{
    return 0;
}

bool IN_JoyPresent()
{
  return 1;
}

void IN_WaitAndProcessEvents()
{
}

void IN_ProcessEvents()
{
}


void
IN_Startup(void)
{
}

void
IN_Shutdown(void)
{
}
void
IN_ClearKeysDown(void)
{
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears only mouse wheel state
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearWheel()
{
	memset ((void *) MouseWheel,0,sizeof(MouseWheel));
}


void
IN_ReadControl(int player,ControlInfo *info)
{
}

ScanCode
IN_WaitForKey(void)
{
    return 0;
}

char
IN_WaitForASCII(void)
{
    return 0;
}

bool	btnstate[NUMBUTTONS];

void IN_StartAck(void)
{
}


bool IN_CheckAck (void)
{
    return 0;
}


void IN_Ack (void)
{
}


bool IN_UserInput(longword delay)
{
    return 0;
}

//===========================================================================

/*
===================
=
= IN_MouseButtons
=
===================
*/
int IN_MouseButtons (void)
{
  return 0;
}

void IN_ReleaseMouse()
{
}

void IN_GrabMouse()
{
}

void IN_AdjustMouse()
{
}

bool IN_IsInputGrabbed()
{
	return 1;
}

void IN_CenterMouse()
{
}

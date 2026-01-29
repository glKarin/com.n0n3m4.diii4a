/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// input.h -- external (non-keyboard) input devices

void IN_ReInit (void);

void IN_Init (void);
float IN_DetermineMouseRate(void);

void IN_Shutdown (void);

void IN_Commands (void);
// oportunity for devices to stick commands on the script buffer

void IN_Touch_BlockGestures(unsigned int devid);	//prevents any gestures from being generated from the same touch event.
int IN_Touch_Fallback(unsigned int devid);		//decides whether a tap should be attack/jump according to m_touchstrafe
qboolean IN_Touch_MouseIsAbs(unsigned int devid);

void IN_Move (float *nudgemovements, float *absmovements, int pnum, float frametime);
// add additional movement on top of the keyboard move cmd

extern cvar_t in_xflip;

extern float mousecursor_x, mousecursor_y;
extern float mousemove_x, mousemove_y;

void IN_ActivateMouse(void);
void IN_DeactivateMouse(void);

int CL_TargettedSplit(qboolean nowrap);

//specific events for the system-specific input code to call. may be called outside the main thread (so long as you don't call these simultaneously - ie: use a mutex or only one input thread).
void IN_KeyEvent(unsigned int devid, int down, int keycode, int unicode);		//don't use IN_KeyEvent for mice if you ever use abs mice...
void IN_MouseMove(unsigned int devid, int abs, float x, float y, float z, float size);
void IN_JoystickAxisEvent(unsigned int devid, int axis, float value);
void IN_Accelerometer(unsigned int devid, float x, float y, float z);
void IN_Gyroscope(unsigned int devid, float pitch, float yaw, float roll);
qboolean IN_SetHandPosition(const char *devname, vec3_t org, vec3_t ang, vec3_t vel, vec3_t avel);

//system-specific functions
void INS_Move (void);
void INS_Accumulate (void);
void INS_ClearStates (void);
void INS_ReInit (void);
void INS_Init (void);
void INS_Shutdown (void);
void INS_Commands (void);	//final chance to call IN_MouseMove/IN_KeyEvent each frame
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid));
void INS_SetupControllerAudioDevices(qboolean enabled);	//creates audio devices for each controller (where controllers have their own audio devices)

enum controllertype_e
{
	CONTROLLER_NONE,
	CONTROLLER_UNKNOWN,		//its all just numbers. could be anything
	CONTROLLER_XBOX,		//ABXY
	CONTROLLER_PLAYSTATION,	//XOSqareTriangle
	CONTROLLER_NINTENDO,	//BAYX...
	CONTROLLER_VIRTUAL		//its fucked. no idea what mappings they're using at all. could be keyboard.
};
enum controllertype_e INS_GetControllerType(int id);
void INS_Rumble(int joy, quint16_t amp_low, quint16_t amp_high, quint32_t duration);
void INS_RumbleTriggers(int joy, quint16_t left, quint16_t right, quint32_t duration);
void INS_SetLEDColor(int id, vec3_t color);
void INS_SetTriggerFX(int id, const void *data, size_t size);
qboolean INS_KeyToLocalName(int qkey, char *buf, size_t bufsize);	//returns a name for the key, according to their keyboard layout AND system language(hopefully), or false on unsupported/error. result may change at any time (eg: tap alt+shift on windows)

#define DEVID_UNSET ~0u

extern cvar_t	cl_splitscreen;
extern cvar_t	cl_nodelta;
extern cvar_t	cl_c2spps;
extern cvar_t	cl_c2sImpulseBackup;
extern cvar_t	cl_netfps;
extern cvar_t	cl_queueimpulses;
extern cvar_t	cl_smartjump;
extern cvar_t	cl_run;
extern cvar_t	cl_fastaccel;
extern cvar_t	cl_rollspeed;
extern cvar_t	cl_prydoncursor;
extern cvar_t	cl_instantrotate;
extern cvar_t	in_xflip;
extern cvar_t	in_windowed_mouse;	//if 0, uses absolute mouse coords allowing the mouse to be used for other programs too. ignored when fullscreen (and reliable).
extern cvar_t	prox_inmenu;
extern cvar_t	cl_forceseat;

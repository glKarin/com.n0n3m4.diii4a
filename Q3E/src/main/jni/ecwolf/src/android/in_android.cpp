
#include "wl_def.h"
#include "wl_play.h"

extern "C"
{

#include "in_android.h"

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "SDL.h"
#include "SDL_keycode.h"
#include "SDL_main.h"


#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"JNI", __VA_ARGS__))
//#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "JNI", __VA_ARGS__))
//#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"JNI", __VA_ARGS__))



// FIFO STUFF ////////////////////
// Copied from FTEQW, I don't know if this is thread safe, but it's safe enough for a game :)
#define EVENTQUEUELENGTH 128
struct eventlist_s
{

	int scancode, unicode,state;

} eventlist[EVENTQUEUELENGTH];

volatile int events_avail; /*volatile to make sure the cc doesn't try leaving these cached in a register*/
volatile int events_used;

static struct eventlist_s *in_newevent(void)
{
	if (events_avail >= events_used + EVENTQUEUELENGTH)
		return 0;
	return &eventlist[events_avail & (EVENTQUEUELENGTH-1)];
}

static void in_finishevent(void)
{
	events_avail++;
}
///////////////////////


extern int SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode);

int PortableKeyEvent(int state, int code, int unicode){
	if (state)
		SDL_SendKeyboardKey(SDL_PRESSED, (SDL_Scancode)code);
	else
		SDL_SendKeyboardKey(SDL_RELEASED, (SDL_Scancode) code);

	return 0;
}



const char* postedCommand = 0;
void postCommand(const char * cmd)
{
	postedCommand = cmd;
}


bool my_buttonstate[NUMBUTTONS];

static bool alwaysrun = false;

void PortableAction(int state, int action)
{
	int key = -1;

	switch (action)
	{
	case PORT_ACT_LEFT:
		key = bt_turnleft;
		break;
	case PORT_ACT_RIGHT:
		key = bt_turnright;
		break;
	case PORT_ACT_FWD:
		key = bt_moveforward;
		break;
	case PORT_ACT_BACK:
		key = bt_movebackward;
		break;

	case PORT_ACT_MOVE_LEFT:
		key = bt_strafeleft;
		break;
	case PORT_ACT_MOVE_RIGHT:
		key = bt_straferight;
		break;


	case PORT_ACT_USE:
		key = bt_use;
		break;
	case PORT_ACT_ATTACK:
		key = bt_attack;
		break;
	case PORT_ACT_MAP:
		key = bt_automap;
		break;
	case PORT_ACT_MAP_ZOOM_IN:
		PortableKeyEvent(state,SDL_SCANCODE_EQUALS,0);
		break;
	case PORT_ACT_MAP_ZOOM_OUT:
		PortableKeyEvent(state,SDL_SCANCODE_MINUS,0);
		break;
	case PORT_ACT_NEXT_WEP:
		key = bt_nextweapon;
		break;
	case PORT_ACT_PREV_WEP:
		key = bt_prevweapon;
		break;
	case PORT_ACT_ZOOM_IN:
		key = bt_zoom;
		break;
	case PORT_ACT_ALT_FIRE:
		key =  bt_altattack;
		break;
	case PORT_ACT_QUICKSAVE:
		PortableKeyEvent(state,SDL_SCANCODE_F8,0);
		break;
	case PORT_ACT_QUICKLOAD:
		PortableKeyEvent(state,SDL_SCANCODE_F9,0);
		break;
	case PORT_ACT_ALWAYS_RUN:
		if (state)
			alwaysrun = !alwaysrun;
		break;
	}

	if (key != -1)
		my_buttonstate[key] = state;

}

int mdx=0,mdy=0;
void PortableMouse(float dx,float dy)
{
	dx *= 1500;
	dy *= 1200;

	mdx += dx;
	mdy += dy;
}

int absx=0,absy=0;
void PortableMouseAbs(float x,float y)
{
	absx = x;
	absy = y;
}


// =================== FORWARD and SIDE MOVMENT ==============

float forwardmove, sidemove; //Joystick mode

void PortableMoveFwd(float fwd)
{
	if (fwd > 1)
		fwd = 1;
	else if (fwd < -1)
		fwd = -1;

	forwardmove = fwd;
}

void PortableMoveSide(float strafe)
{
	if (strafe > 1)
		strafe = 1;
	else if (strafe < -1)
		strafe = -1;

	sidemove = strafe;
}

void PortableMove(float fwd, float strafe)
{
	PortableMoveFwd(fwd);
	PortableMoveSide(strafe);
}

//======================================================================

//Look up and down
int look_pitch_mode;
float look_pitch_mouse,look_pitch_abs,look_pitch_joy;
void PortableLookPitch(int mode, float pitch)
{
	look_pitch_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_pitch_mouse += pitch;
		break;
	case LOOK_MODE_ABSOLUTE:
		look_pitch_abs = pitch;
		break;
	case LOOK_MODE_JOYSTICK:
		look_pitch_joy = pitch;
		break;
	}
}

//left right
int look_yaw_mode;
float look_yaw_mouse,look_yaw_joy;
void PortableLookYaw(int mode, float yaw)
{
	look_yaw_mode = mode;
	switch(mode)
	{
	case LOOK_MODE_MOUSE:
		look_yaw_mouse += yaw;
		break;
	case LOOK_MODE_JOYSTICK:
		look_yaw_joy = yaw;
		break;
	}
}



void PortableCommand(const char * cmd){
	postCommand(cmd);
}

void PortableFrame(void){


}

extern bool	ingame;
extern  bool menusAreFaded;
extern int inConversation;

bool inConfirm = false;;
int PortableInMenu(void){
	//return (ingame)?0:1;
	return (!menusAreFaded) || !ingame || inConfirm || inConversation;
}

int PortableInAutomap(void)
{
	return 0;
}

int PortableShowKeyboard(void){

	return 0;
}
}

#define BASEMOVE                35
#define RUNMOVE                 70
#define BASETURN                35
#define RUNTURN                 70

//NOTE this is cpp
void pollAndroidControls()
{
	control[ConsolePlayer].controly  -= forwardmove     * (alwaysrun ? RUNMOVE:BASEMOVE);
	control[ConsolePlayer].controlstrafe  += sidemove   * (alwaysrun ? RUNMOVE:BASEMOVE);


	switch(look_yaw_mode)
	{
	case LOOK_MODE_MOUSE:
		control[ConsolePlayer].controlx += -look_yaw_mouse * 8000;
		look_yaw_mouse = 0;
		break;
	case LOOK_MODE_JOYSTICK:
		control[ConsolePlayer].controlx += -look_yaw_joy * 80;
		break;
	}

	for (int n=0;n<NUMBUTTONS;n++)
	{
		if (my_buttonstate[n])
			control[ConsolePlayer].buttonstate[n] = 1;
	}
}



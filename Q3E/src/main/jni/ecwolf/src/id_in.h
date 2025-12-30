//
//	ID Engine
//	ID_IN.h - Header file for Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

#ifndef	__ID_IN__
#define	__ID_IN__

#ifdef	__DEBUG__
#define	__DEBUG_InputMgr__
#endif

typedef	int		ScanCode;

#ifndef LIBRETRO

#include <SDL.h>

#if !SDL_VERSION_ATLEAST(1,3,0)
#define SDLK_A SDLK_a
#define SDLK_B SDLK_b
#define SDLK_C SDLK_c
#define SDLK_D SDLK_d
#define SDLK_E SDLK_e
#define SDLK_F SDLK_f
#define SDLK_G SDLK_g
#define SDLK_H SDLK_h
#define SDLK_I SDLK_i
#define SDLK_J SDLK_j
#define SDLK_K SDLK_k
#define SDLK_L SDLK_l
#define SDLK_M SDLK_m
#define SDLK_N SDLK_n
#define SDLK_O SDLK_o
#define SDLK_P SDLK_p
#define SDLK_Q SDLK_q
#define SDLK_R SDLK_r
#define SDLK_S SDLK_s
#define SDLK_T SDLK_t
#define SDLK_U SDLK_u
#define SDLK_V SDLK_v
#define SDLK_W SDLK_w
#define SDLK_X SDLK_x
#define SDLK_Y SDLK_y
#define SDLK_Z SDLK_z
#define SDLK_GRAVE SDLK_BACKQUOTE
#define SDLx_SCANCODE(x) SDLK_##x
#else
#define SDLx_SCANCODE(x) SDL_SCANCODE_##x
#endif

#define	sc_None			0
#define	sc_Bad			0xff
#define	sc_Return		SDLx_SCANCODE(RETURN)
#define	sc_Enter		sc_Return
#define	sc_Escape		SDLx_SCANCODE(ESCAPE)
#define	sc_Space		SDLx_SCANCODE(SPACE)
#define	sc_BackSpace	SDLx_SCANCODE(BACKSPACE)
#define	sc_Tab			SDLx_SCANCODE(TAB)
#define	sc_Alt			SDLx_SCANCODE(LALT)
#define	sc_Control		SDLx_SCANCODE(LCTRL)
#define	sc_CapsLock		SDLx_SCANCODE(CAPSLOCK)
#define	sc_LShift		SDLx_SCANCODE(LSHIFT)
#define	sc_RShift		SDLx_SCANCODE(RSHIFT)
#define	sc_UpArrow		SDLx_SCANCODE(UP)
#define	sc_DownArrow	SDLx_SCANCODE(DOWN)
#define	sc_LeftArrow	SDLx_SCANCODE(LEFT)
#define	sc_RightArrow	SDLx_SCANCODE(RIGHT)
#define	sc_Insert		SDLx_SCANCODE(INSERT)
#define	sc_Delete		SDLx_SCANCODE(DELETE)
#define	sc_Home			SDLx_SCANCODE(HOME)
#define	sc_End			SDLx_SCANCODE(END)
#define	sc_PgUp			SDLx_SCANCODE(PAGEUP)
#define	sc_PgDn			SDLx_SCANCODE(PAGEDOWN)
#define	sc_F1			SDLx_SCANCODE(F1)
#define	sc_F2			SDLx_SCANCODE(F2)
#define	sc_F3			SDLx_SCANCODE(F3)
#define	sc_F4			SDLx_SCANCODE(F4)
#define	sc_F5			SDLx_SCANCODE(F5)
#define	sc_F6			SDLx_SCANCODE(F6)
#define	sc_F7			SDLx_SCANCODE(F7)
#define	sc_F8			SDLx_SCANCODE(F8)
#define	sc_F9			SDLx_SCANCODE(F9)
#define	sc_F10			SDLx_SCANCODE(F10)
#define	sc_F11			SDLx_SCANCODE(F11)
#define	sc_F12			SDLx_SCANCODE(F12)

#define sc_ScrollLock		SDLx_SCANCODE(SCROLLOCK)
#define sc_PrintScreen		SDLx_SCANCODE(PRINT)

#define	sc_1			SDLx_SCANCODE(1)
#define	sc_2			SDLx_SCANCODE(2)
#define	sc_3			SDLx_SCANCODE(3)
#define	sc_4			SDLx_SCANCODE(4)
#define	sc_5			SDLx_SCANCODE(5)
#define	sc_6			SDLx_SCANCODE(6)
#define	sc_7			SDLx_SCANCODE(7)
#define	sc_8			SDLx_SCANCODE(8)
#define	sc_9			SDLx_SCANCODE(9)
#define	sc_0			SDLx_SCANCODE(0)

#define	sc_A			SDLx_SCANCODE(A)
#define	sc_B			SDLx_SCANCODE(B)
#define	sc_C			SDLx_SCANCODE(C)
#define	sc_D			SDLx_SCANCODE(D)
#define	sc_E			SDLx_SCANCODE(E)
#define	sc_F			SDLx_SCANCODE(F)
#define	sc_G			SDLx_SCANCODE(G)
#define	sc_H			SDLx_SCANCODE(H)
#define	sc_I			SDLx_SCANCODE(I)
#define	sc_J			SDLx_SCANCODE(J)
#define	sc_K			SDLx_SCANCODE(K)
#define	sc_L			SDLx_SCANCODE(L)
#define	sc_M			SDLx_SCANCODE(M)
#define	sc_N			SDLx_SCANCODE(N)
#define	sc_O			SDLx_SCANCODE(O)
#define	sc_P			SDLx_SCANCODE(P)
#define	sc_Q			SDLx_SCANCODE(Q)
#define	sc_R			SDLx_SCANCODE(R)
#define	sc_S			SDLx_SCANCODE(S)
#define	sc_T			SDLx_SCANCODE(T)
#define	sc_U			SDLx_SCANCODE(U)
#define	sc_V			SDLx_SCANCODE(V)
#define	sc_W			SDLx_SCANCODE(W)
#define	sc_X			SDLx_SCANCODE(X)
#define	sc_Y			SDLx_SCANCODE(Y)
#define	sc_Z			SDLx_SCANCODE(Z)

#define sc_Equals		SDLx_SCANCODE(EQUALS)
#define sc_Minus		SDLx_SCANCODE(MINUS)

#define sc_Comma		SDLx_SCANCODE(COMMA)
#define sc_Peroid		SDLx_SCANCODE(PERIOD)

#define sc_Grave		SDLx_SCANCODE(GRAVE)
#endif

#define	key_None		0

enum Demo {
	demo_Off,demo_Record,demo_Playback,demo_PlayDone
};
enum ControlType {
	ctrl_Keyboard,
	ctrl_Keyboard1 = ctrl_Keyboard,ctrl_Keyboard2,
	ctrl_Joystick,
	ctrl_Joystick1 = ctrl_Joystick,ctrl_Joystick2,
	ctrl_Mouse
};
enum Motion {
	motion_Left = -1,motion_Up = -1,
	motion_None = 0,
	motion_Right = 1,motion_Down = 1
};
enum Direction {
	dir_North,dir_NorthEast,
	dir_East,dir_SouthEast,
	dir_South,dir_SouthWest,
	dir_West,dir_NorthWest,
	dir_None
};
struct CursorInfo {
	bool		button0,button1,button2,button3;
	short		x,y;
	Motion		xaxis,yaxis;
	Direction	dir;
};
typedef	CursorInfo	ControlInfo;
struct KeyboardDef {
	ScanCode	button0,button1,
				upleft,		up,		upright,
				left,				right,
				downleft,	down,	downright;
};

struct JoystickSens
{
	int sensitivity;
	int deadzone;
};
extern JoystickSens *JoySensitivity;

// Global variables
extern bool Keyboard[];
extern bool MousePresent;
extern bool MouseWheel[4];
extern unsigned short Paused;
extern char LastASCII;
extern ScanCode LastScan;
extern int JoyNumButtons;
extern int JoyNumAxes;


// Function prototypes

void IN_Startup();
void IN_Shutdown();
void IN_ClearKeysDown();
void IN_ClearWheel();
void IN_ReadControl(int,ControlInfo *);
void IN_GetJoyAbs(word joy,word *xp,word *yp);
void IN_SetupJoy(word joy,word minx,word maxx,word miny,word maxy);
void IN_StopDemo();
void IN_FreeDemoBuffer();
void IN_Ack();
bool IN_UserInput(longword delay);
char IN_WaitForASCII();
ScanCode IN_WaitForKey();
word IN_GetJoyButtonsDB(word joy);
const char *IN_GetScanName(ScanCode);

void IN_WaitAndProcessEvents();
void IN_ProcessEvents();

int IN_MouseButtons (void);
void IN_ReleaseMouse();
void IN_GrabMouse();
void IN_AdjustMouse();

bool IN_JoyPresent();
void IN_SetJoyCurrent(int joyIndex);
int IN_JoyButtons (void);
int IN_JoyAxes (void);
void IN_GetJoyDelta(int *dx,int *dy);
int IN_GetJoyAxis(int axis);

void IN_StartAck(void);
bool IN_CheckAck (void);
bool IN_IsInputGrabbed();
void IN_CenterMouse();

#endif

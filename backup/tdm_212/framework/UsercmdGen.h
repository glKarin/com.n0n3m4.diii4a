/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __USERCMDGEN_H__
#define __USERCMDGEN_H__

/*
===============================================================================

	Samples a set of user commands from player input.

===============================================================================
*/

const int USERCMD_HZ			= 60;			// 60 frames per second
const int USERCMD_MSEC			= 1000 / USERCMD_HZ;

// ButtonState inputs; originally from UsercmdGen.cpp, left out of SDK by accident
// sourced from http://www.doom3world.org/phpbb2/viewtopic.php?f=26&t=18587&p=170143
typedef enum {
   UB_NONE,

   UB_UP,
   UB_DOWN,
   UB_LEFT,
   UB_RIGHT,
   UB_FORWARD,
   UB_BACK,
   UB_LOOKUP,
   UB_LOOKDOWN,
   UB_STRAFE,
   UB_MOVELEFT,
   UB_MOVERIGHT,

   UB_BUTTON0,
   UB_BUTTON1,
   UB_BUTTON2,
   UB_BUTTON3,
   UB_BUTTON4,
   UB_BUTTON5,
   UB_BUTTON6,
   UB_BUTTON7,

   UB_ATTACK,
   UB_SPEED,
   UB_PARRY_MANIPULATE,
   UB_SHOWSCORES,
   UB_MLOOK,

#ifdef QUAKE4
        UB_INGAMESTATS,
        UB_VOICECHAT,
        UB_TOURNEY,
#endif//QUAKE4

   UB_WEAPON0,
   UB_WEAPON1,
   UB_WEAPON2,
   UB_WEAPON3,
   UB_WEAPON4,
   UB_WEAPON5,
   UB_WEAPON6,
   UB_WEAPON7,
   UB_WEAPON8,
   UB_WEAPON9,
   UB_WEAPON10,
   UB_WEAPON11,
   UB_WEAPON12,
   UB_RELOAD,
   UB_WEAPON_NEXT,
   UB_WEAPON_PREV,
   UB_IMPULSE16,
   UB_READY,
   UB_CENTER_VIEW,
   UB_OBJECTIVES,
   UB_IMPULSE20,
   UB_IMPULSE21,
   UB_IMPULSE22,
   UB_CROUCH,
   UB_MANTLE,
   UB_IMPULSE25,
   UB_IMPULSE26,
   UB_IMPULSE27,
   UB_IMPULSE28,
   UB_IMPULSE29,
   UB_INVENTORY_GRID,
   UB_IMPULSE31,
   UB_IMPULSE32,
   UB_IMPULSE33,
   UB_IMPULSE34,
   UB_IMPULSE35,
   UB_IMPULSE36,
   UB_IMPULSE37,
   UB_IMPULSE38,
   UB_IMPULSE39,
   UB_IMPULSE40,
   UB_FROB,
   UB_IMPULSE42,
   UB_IMPULSE43,
   UB_LEAN_FORWARD,
   UB_LEAN_LEFT,
   UB_LEAN_RIGHT,
   UB_INVENTORY_PREV,
   UB_INVENTORY_NEXT,
   UB_INVENTORY_GROUP_PREV,
   UB_INVENTORY_GROUP_NEXT,
   UB_INVENTORY_USE,
   UB_INVENTORY_DROP,
   UB_CREEP,
   UB_IMPULSE54,
   UB_IMPULSE55,
   UB_IMPULSE56,
   UB_IMPULSE57,
   UB_IMPULSE58,
   UB_IMPULSE59,
   UB_IMPULSE60,
   UB_IMPULSE61,
   UB_IMPULSE62,
   UB_IMPULSE63,

   UB_MAX_BUTTONS
} UserCmdButton;


// usercmd_t->button bits
const int BUTTON_ATTACK			= BIT(0);
const int BUTTON_RUN			= BIT(1);
const int BUTTON_ZOOM			= BIT(2);
const int BUTTON_SCORES			= BIT(3);
const int BUTTON_MLOOK			= BIT(4);
const int BUTTON_5				= BIT(5);
const int BUTTON_6				= BIT(6);
const int BUTTON_7				= BIT(7);

// usercmd_t->impulse commands
enum {
	IMPULSE_WEAPON0,
	IMPULSE_WEAPON1,
	IMPULSE_WEAPON2,
	IMPULSE_WEAPON3,
	IMPULSE_WEAPON4,
	IMPULSE_WEAPON5,
	IMPULSE_WEAPON6,
	IMPULSE_WEAPON7,
	IMPULSE_WEAPON8,
	IMPULSE_WEAPON9,
	IMPULSE_WEAPON10,
	IMPULSE_WEAPON11,
	IMPULSE_WEAPON12,
	IMPULSE_RELOAD,
	IMPULSE_WEAPON_NEXT,
	IMPULSE_WEAPON_PREV,
	IMPULSE_16,
	IMPULSE_READY,
	IMPULSE_CENTER_VIEW,
	IMPULSE_OBJECTIVES,
	IMPULSE_20,
	IMPULSE_21,
	IMPULSE_22,
	IMPULSE_CROUCH,
	IMPULSE_MANTLE,
	IMPULSE_25,
	IMPULSE_26,
	IMPULSE_27,
	IMPULSE_28,
	IMPULSE_29,
	IMPULSE_INVENTORY_GRID,	// #4286
	IMPULSE_40 = 40,
	IMPULSE_FROB,
	IMPULSE_42,
	IMPULSE_43,
	IMPULSE_LEAN_FORWARD,
	IMPULSE_LEAN_LEFT,
	IMPULSE_LEAN_RIGHT,
	IMPULSE_INVENTORY_PREV,
	IMPULSE_INVENTORY_NEXT,
	IMPULSE_INVENTORY_GROUP_PREV,
	IMPULSE_INVENTORY_GROUP_NEXT,
	IMPULSE_INVENTORY_USE,
	IMPULSE_INVENTORY_DROP,
	IMPULSE_CREEP,
	IMPULSE_MAX
};

// Darkmod: Added as a baseoffset for the impulse keys, when used with ButtonState.
// This function requires an int as input which defines the key that should be used,
// and it looks as if the first impulse starts with the number 25.
const int IMPULSE_BUTTON_BASE	= 25;
#define KEY_FROM_IMPULSE(n)		(n + IMPULSE_BUTTON_BASE)

// usercmd_t->flags
const int UCF_IMPULSE_SEQUENCE	= 0x0001;		// toggled every time an impulse command is sent

class usercmd_t {
public:
	int			gameFrame;						// frame number
	int			gameTime;						// game time
	int			duplicateCount;					// duplication count for networking
	byte		buttons;						// buttons
	signed char	forwardmove;					// forward/backward movement
	signed char	rightmove;						// left/right movement
	signed char	upmove;							// up/down movement
	short		angles[3];						// view angles
	short		mx;								// mouse delta x
	short		my;								// mouse delta y
	signed char impulse;						// impulse command
	byte		flags;							// additional flags
	int			sequence;						// just for debugging
	short		jx;								// joystick x
	short		jy;								// joystick y

public:
	void		ByteSwap();						// on big endian systems, byte swap the shorts and ints
	bool		operator==( const usercmd_t &rhs ) const;
};

typedef enum {
	INHIBIT_SESSION = 0,
	INHIBIT_ASYNC
} inhibit_t;

const int MAX_BUFFERED_USERCMD = 64;

class idUsercmdGen {
public:
	virtual				~idUsercmdGen( void ) {}

	// Sets up all the cvars and console commands.
	virtual	void		Init( void ) = 0;

	// Prepares for a new map.
	virtual void		InitForNewMap( void ) = 0;

	// Shut down.
	virtual void		Shutdown( void ) = 0;

	// Clears all key states and face straight.
	virtual	void		Clear( void ) = 0;

	// Clears view angles.
	virtual void		ClearAngles( void ) = 0;

	// When the console is down or the menu is up, only emit default usercmd, so the player isn't moving around.
	// Each subsystem (session and game) may want an inhibit will OR the requests.
	virtual void		InhibitUsercmd( inhibit_t subsystem, bool inhibit ) = 0;

	// Returns a buffered command for the given game tic.
	virtual usercmd_t	TicCmd( int ticNumber ) = 0;

	// Called async at regular intervals.
	virtual	void		UsercmdInterrupt( void ) = 0;

	// Set a value that can safely be referenced by UsercmdInterrupt() for each key binding.
	virtual	int			CommandStringUsercmdData( const char *cmdString ) = 0;

	// Returns the number of user commands.
	virtual int			GetNumUserCommands( void ) = 0;

	// Returns the name of a user command via index.
	virtual const char *GetUserCommandName( int index ) = 0;

	// Continuously modified, never reset. For full screen guis.
	virtual void		MouseState( int *x, int *y, int *button, bool *down ) = 0;

	// Directly sample a button.
	virtual int			ButtonState( int key ) = 0;

	// Directly sample a keystate.
	virtual int			KeyState( int key ) = 0;

	virtual int 		GetToggledRunState( void ) = 0;
	virtual void		SetToggledRunState( int on ) = 0;

	// Directly sample a usercmd.
	virtual usercmd_t	GetDirectUsercmd( void ) = 0;

	// stgatilov: needed for automation only!!!
	// Allows to toggle impulse flag properly.
	virtual int&		hack_Flags() = 0;
};

extern idUsercmdGen	*usercmdGen;

#endif /* !__USERCMDGEN_H__ */

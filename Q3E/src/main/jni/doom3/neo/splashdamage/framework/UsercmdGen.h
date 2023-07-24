// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __USERCMDGEN_H__
#define __USERCMDGEN_H__

/*
===============================================================================

	Samples a set of user commands from player input.

===============================================================================
*/

const int USERCMD_HZ			= 30;			// 30 frames per second
const int USERCMD_MSEC			= 1000 / USERCMD_HZ;

struct userButtons_t {
#ifdef _XENON
	bool			pad0		: 1; // Nerve: Padding is used to align the buttons
	bool			pad1		: 1; // to the least signifigant part of the short
	bool			pad2		: 1; // That this struct is unioned with
	bool			pad3		: 1;
	bool			pad4		: 1;
	bool			pad5		: 1;
#endif
	bool			attack		: 1;
	bool			run			: 1;
	bool			modeSwitch	: 1;
	bool			mLookOff	: 1;
	bool			sprint		: 1;
	bool			activate	: 1;
	bool			altAttack	: 1;
	bool			leanLeft	: 1;
	bool			leanRight	: 1;
	bool			tophat		: 1;
}; // update the define below whenever you change this struct, all fields MUST be single bits

#define USERCMD_NUM_BUTTONS 10

struct clientButtons_t {
	bool			showScores		: 1;
	bool			voice			: 1;
	bool			teamVoice		: 1;
	bool			fireteamVoice	: 1;
};

enum usercmdbuttonType_t {
	B_BUTTON,
	B_IMPULSE,
	B_LOCAL_IMPULSE,
	B_COMMAND,
};

enum usercmdButton_t {
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

	UB_ATTACK,
	UB_SPEED,
	UB_MODESWITCH,
	UB_SPRINT,
	UB_ACTIVATE,
	UB_SHOWSCORES,
	UB_VOICE,
	UB_TEAMVOICE,
	UB_FIRETEAMVOICE,
	UB_MLOOK,
	UB_ALTATTACK,
	UB_TOPHAT,
	UB_LEANLEFT,
	UB_LEANRIGHT,

	UB_MAX_BUTTONS
};

// usercmd_t->flags
const int UCF_IMPULSE_SEQUENCE	= 0x0001;		// toggled every time an impulse command is sent

union userButtonsUnion_t {
	short				btnValue;	// this has to be at least as wide as the btn struct
	userButtons_t		btn;
};

class usercmd_t {
public:
	int						gameFrame;						// frame number
	int						gameTime;						// game time
	int						duplicateCount;					// duplication count for networking
	userButtonsUnion_t		buttons;						// buttons
	signed char				forwardmove;					// forward/backward movement
	signed char				rightmove;						// left/right movement
	signed char				upmove;							// up/down movement
	short					angles[ 3 ];					// view angles
	signed char				impulse;						// impulse command
	byte					flags;
	clientButtons_t			clientButtons;					// new sets of buttons ( client only )

public:
	void				ByteSwap();						// on big endian systems, byte swap the shorts and ints
	bool				operator==( const usercmd_t &rhs ) const;
};

enum inhibit_t {
	INHIBIT_SESSION = 0,
	INHIBIT_ASYNC
};

const int MAX_BUFFERED_USERCMD = 64;

class sdKeyCommand;
class idKey;

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

	// Returns a buffered command for the given game tic.
	virtual usercmd_t	TicCmd( int ticNumber ) = 0;

	// Called async at regular intervals.
	virtual	void		UsercmdInterrupt( void ) = 0;

	// Directly sample a usercmd.
	virtual usercmd_t	GetDirectUsercmd( bool doGameCallback = true ) = 0;

	virtual void		HandleCommand( const sdKeyCommand* cmd, bool down ) = 0;

	virtual bool		ProcessEvent( const sdSysEvent& evt ) = 0;
};

extern idUsercmdGen	*usercmdGen;

#endif /* !__USERCMDGEN_H__ */

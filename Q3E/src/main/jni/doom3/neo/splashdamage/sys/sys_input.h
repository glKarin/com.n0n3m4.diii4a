// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_INPUT__
#define __SYS_INPUT__

#include "../framework/KeyInput.h"
#include "keynum.h"

/*
==============================================================

	Input

==============================================================
*/

/*
======================================================================

	Keyboard

======================================================================
*/

class idKeyboard {
public:
	virtual						~idKeyboard() { }

	virtual bool				Init() = 0;
	virtual void				Shutdown() = 0;

	virtual void				Activate() = 0;
	virtual void				Deactivate() = 0;

	virtual int					PollInputEvents( bool postEvents ) = 0;
	virtual int					ReturnInputEvent( const int n, keyNum_t& key, bool& isDown ) = 0;
	virtual void				EndInputEvents() = 0;

	virtual keyNum_t			ConvertScanToKey( unsigned int scanCode ) const = 0;
	virtual keyNum_t			ConvertCharToKey( char ch ) const = 0;
	virtual char				ConvertScanToChar( unsigned int scanCode ) const = 0;
	virtual unsigned int		ConvertCharToScan( char ch ) const = 0;
	virtual char				ConvertKeyToChar( const keyNum_t keyNum ) const = 0;
	virtual bool				IsConsoleKey( const sdSysEvent& event ) const = 0;

	static void					AllocateKeys();
	static unsigned int			StringToScanCode( const char* str );
	static keyNum_t				StringToKeyNum( const char* str );
	static void					KeyNumToString( const keyNum_t keyNum, idWStr& fixedText, idStr& locName );

	static idKey&				GetStandardKey( const keyNum_t key );
};

/*
======================================================================

	IME

======================================================================
*/

typedef enum imeEvent_e {
	IMEV_COMPOSITION_START,
	IMEV_COMPOSITION_END,
	IMEV_COMPOSITION_UPDATE,
	IMEV_COMPOSITION_COMMIT,
} imeEvent_t;

class sdIME {
public:
	static const int MAX_CANDLIST = 9;
	static const int MAX_CANDIDATE_LENGTH = 256;

	enum state_e {
		IME_STATE_OFF,
		IME_STATE_ON,
		IME_STATE_ENGLISH
	};

	enum lang_e {
		IME_LANG_NEUTRAL,
		IME_LANG_CHINESE,
		IME_LANG_CHINESE_SIMPLIFIED,	// sub lang
		IME_LANG_CHINESE_TRADITIONAL,	// sub lang
		IME_LANG_KOREAN,
		IME_LANG_JAPANESE
	};

	virtual bool			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual bool			LangSupportsIME() const = 0;

	virtual void			Enable( bool enable ) = 0;
	virtual bool			IsEnabled() const = 0;
	virtual void			FinalizeString( bool send = false ) = 0;
	virtual int				GetCursorChars() const = 0;

	virtual bool			IsReadingWindowActive() const = 0;
	virtual bool			IsHorizontalReading() const = 0;
	virtual bool			VerticalCandidateLine() const = 0;

	virtual state_e			GetState() const = 0;
	virtual const wchar_t*	GetIndicator() const = 0;

	virtual bool			IsCandidateListActive() const = 0;
	virtual const wchar_t*	GetCandidate( const unsigned int index ) const = 0;
	virtual int				GetCandidateCount() const = 0;
	virtual int				GetCandidateSelection() const = 0;

	virtual const wchar_t*	GetCompositionString() const = 0;
	virtual const byte*		GetCompositionStringAttributes() const = 0;

	virtual const lang_e	GetLanguage() const = 0;
	virtual const lang_e	GetPrimaryLanguage() const = 0;
};

/*
======================================================================

	Mouse

======================================================================
*/

typedef enum mouseButton_e {
	M_INVALID = 0,

	M_MOUSE1 = 1,
	M_MOUSE2,
	M_MOUSE3,
	M_MOUSE4,
	M_MOUSE5,
	M_MOUSE6,
	M_MOUSE7,
	M_MOUSE8,
	M_MOUSE9,
	M_MOUSE10,
	M_MOUSE11,
	M_MOUSE12,

	M_MWHEELDOWN,
	M_MWHEELUP,

	M_NUM_MOUSEBUTTONS,
} mouseButton_t;

class idMouse {
public:
	virtual					~idMouse() { }

	virtual bool			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual bool			IsActive() const = 0;
	virtual void			Activate() = 0;
	virtual void			Deactivate() = 0;

	virtual void			GrabCursor( bool grab ) = 0;

	virtual int				PollInputEvents( bool postEvents ) = 0;
	virtual int				ReturnInputEvent( const int n, int& action, int& value ) = 0;
	virtual void			EndInputEvents() = 0;

	static void				AllocateMouseButtons();
	static mouseButton_t	StringToMouseButton( const char* str );
	static const wchar_t*	MouseButtonToString( const mouseButton_t button, bool localized );

	static idKey&			GetMouseButton( const mouseButton_t button );
};

/*
======================================================================

	Controllers

======================================================================
*/

#include "../framework/KeyInput.h"

typedef enum {
	C_BUTTON1,
	C_BUTTON2,
	C_BUTTON3,
	C_BUTTON4,
	C_BUTTON5,
	C_BUTTON6,
	C_BUTTON7,
	C_BUTTON8,
	C_BUTTON9,
	C_BUTTON10,
	C_BUTTON11,
	C_BUTTON12,
	C_BUTTON13,
	C_BUTTON14,
	C_BUTTON15,
	C_BUTTON16,
	C_BUTTON17,
	C_BUTTON18,
	C_BUTTON19,
	C_BUTTON20,
	C_BUTTON21,
	C_BUTTON22,
	C_BUTTON23,
	C_BUTTON24,
	C_BUTTON25,
	C_BUTTON26,
	C_BUTTON27,
	C_BUTTON28,
	C_BUTTON29,
	C_BUTTON30,
	C_BUTTON31,
	C_BUTTON32,
	C_NUMBERED_BUTTON_MAX = C_BUTTON32,
	C_LEFT_TRIGGER,
	C_RIGHT_TRIGGER,
	C_DPAD_UP,
	C_DPAD_DOWN,
	C_DPAD_LEFT,
	C_DPAD_RIGHT,
	C_BUTTON_MAX = C_DPAD_RIGHT,
	C_AXIS1,
	C_AXIS2,
	C_AXIS3,
	C_AXIS4,
	C_AXIS5,
	C_AXIS6,
	C_AXIS7,
	C_AXIS8,
	C_AXIS_MAX = C_AXIS8,
	MAX_CONTROLLER_EVENTS
} sys_cEvents;

const int MAX_CONTROLLER_BUTTONS = C_BUTTON_MAX + 1 - C_BUTTON1;
const int MAX_CONTROLLER_NUMBERED_BUTTONS = C_NUMBERED_BUTTON_MAX + 1 - C_BUTTON1;
const int MAX_CONTROLLER_AXES = C_AXIS_MAX + 1 - C_AXIS1;

class sdController {
public:
	typedef enum {
		CS_NOT_CONNECTED,	// someone tripped over the cable
		CS_OK,				// all fine
	} controllerState_e;

							sdController();
	virtual					~sdController() {};

	void					SetAPITypeIndex( const int index ) { apiTypeIndex = index; }

	const char*				GetName() const { return name; }
	int						GetHash() const { return hash; }

	virtual void			SetIndex( const int index ) { this->index = index; InitButtons(); }
	controllerState_e		GetState() const { return state; }

	virtual void			UpdateState() = 0;	// update controller state without generating any input events

	virtual int				PollInputEvents() = 0;
	virtual int				GetNumEvents() = 0;
	virtual int				ReturnInputEvent( const int n, int &action, int &value ) = 0;
	virtual void			EndInputEvents() = 0;

	void					SetAxis( int axis, float value ) { this->axis[ axis ] = value; }
	const float*			GetAxisArray() const { return axis; }

	idKey&					GetButton( int index ) { return idKeyInput::GetKeyByIndex( buttons[ index ] ); }
	void					InitButtons( void );

protected:
	int						apiTypeIndex;

	char					name[ MAX_PATH ];
	int						hash;

	int						index;
	controllerState_e		state;
	bool					mapped;

	float					axis[MAX_CONTROLLER_AXIS];	// set by controller events
	int						buttons[ MAX_CONTROLLER_BUTTONS ];
};

class sdControllerAPI {
public:
	typedef enum {
		CAS_BAD_API,
		CAS_UNINITIALIZED,
		CAS_SUPPORTED,
		CAS_INIT_FAILED
	} controllerApiState_e;

							sdControllerAPI();
	virtual					~sdControllerAPI() {};

	virtual void			Init( const int apiIndex ) = 0;
	virtual void			Shutdown();

	virtual const char*		GetName() const = 0;

	controllerApiState_e	GetState() const { return state; }

protected:
	controllerApiState_e	state;
};

class sdDeviceMappingCallback : public idCVarCallback {
	virtual void OnChanged( void );
};

class sdControllerManager {
public:
	virtual											~sdControllerManager() {};

	virtual void									Init() = 0;
	virtual void									Shutdown() = 0;

	virtual sdControllerAPI::controllerApiState_e	GetAPIState( const char* APIName ) = 0;

	int												GetMaxControllers() const { return controllers.Num(); }	// maximum amount of supported controllers
	virtual int										GetNumConnectedControllers() const = 0;					// amount of connected controllers

	void											AddController( sdController& controller ) { controllers.Append( &controller ); }
	sdController&									GetController( const int index ) { return *controllers[ index ]; }

	sdController*									GetControllerByHash( int hash );

	void											OnDeviceMappingChanged( void );

	// remapping controller hashes to joystick slots
	static idCVar									in_joy1_device;
	static idCVar									in_joy2_device;
	static idCVar									in_joy3_device;
	static idCVar									in_joy4_device;

	sdController*									GetControllerByJoySlot( int slot );
	int												GetJoySlotByController( const sdController& controller );
	static int										GetJoySlotByHash( int hash );
	static void										GetKeyNameForSlotButton( int slot, int button, idStr& name, idStr& locName );
	static void										AllocateControllerButtons( void );

public:
	struct buttonMap_t {
		const char*	name;
		sys_cEvents	event;
	};

	static sdDeviceMappingCallback					deviceMappingCallback;
	static buttonMap_t								specialButtons[];
	static const int								numSpecialButtons;

protected:
	idList< sdController* >							controllers;
};

#endif /* !__SYS_INPUT__ */

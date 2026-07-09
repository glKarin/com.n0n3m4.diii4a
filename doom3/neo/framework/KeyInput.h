/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __KEYINPUT_H__
#define __KEYINPUT_H__

/*
===============================================================================

	Key Input

===============================================================================
*/

// these are the key numbers that are used by the key system
// normal keys should be passed as lowercased ascii
// Some high ascii (> 127) characters that are mapped directly to keys on
// western european keyboards are inserted in this table so that those keys
// are bindable (otherwise they get bound as one of the special keys in this
// table)
#if !defined(_SPLASHDAMAGE)
typedef enum {
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE = 127,

	K_COMMAND = 128,
	K_CAPSLOCK,
	K_SCROLL,
	K_POWER,
	K_PAUSE,

	K_UPARROW = 133,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	// The 3 windows keys
	K_LWIN = 137,
	K_RWIN,
	K_MENU,

	K_ALT = 140,
	K_CTRL,
	K_SHIFT,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1 = 149,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_INVERTED_EXCLAMATION = 161,	// upside down !
	K_F13,
	K_F14,
	K_F15,

	K_KP_HOME = 165,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_SUPERSCRIPT_TWO = 178,		// superscript 2
	K_KP_MINUS,
	K_ACUTE_ACCENT = 180,			// accute accent
	K_KP_PLUS,
	K_KP_NUMLOCK,
	K_KP_STAR,
	K_KP_EQUALS,

	K_MASCULINE_ORDINATOR = 186,
	// K_MOUSE enums must be contiguous (no char codes in the middle)
	K_MOUSE1 = 187,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,

	K_MWHEELDOWN = 195,
	K_MWHEELUP,

	K_JOY1 = 197,
	K_JOY2,
	K_JOY3,
	K_JOY4,
	K_JOY5,
	K_JOY6,
	K_JOY7,
	K_JOY8,
	K_JOY9,
	K_JOY10,
	K_JOY11,
	K_JOY12,
	K_JOY13,
	K_JOY14,
	K_JOY15,
	K_JOY16,
	K_JOY17,
	K_JOY18,
	K_JOY19,
	K_JOY20,
	K_JOY21,
	K_JOY22,
	K_JOY23,
	K_JOY24,
	K_JOY25,
	K_JOY26,
	K_JOY27,
	K_GRAVE_A = 224,	// lowercase a with grave accent
	K_JOY28,
	K_JOY29,
	K_JOY30,
	K_JOY31,
	K_JOY32,

	K_AUX1 = 230,
	K_CEDILLA_C = 231,	// lowercase c with Cedilla
	K_GRAVE_E = 232,	// lowercase e with grave accent
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_GRAVE_I = 236,	// lowercase i with grave accent
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_TILDE_N = 241,	// lowercase n with tilde
	K_GRAVE_O = 242,	// lowercase o with grave accent
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_GRAVE_U = 249,	// lowercase u with grave accent
	K_AUX15,
	K_AUX16,

	K_PRINT_SCR	= 252,	// SysRq / PrintScr
	K_RIGHT_ALT = 253,	// used by some languages as "Alt-Gr"
	K_LAST_KEY  = 254	// this better be < 256!
} keyNum_t;
#else
#include "sys/keynum.h"
#endif


class idKeyInput
{
	public:
		static void			Init(void);
		static void			Shutdown(void);

		static void			ArgCompletion_KeyName(const idCmdArgs &args, void(*callback)(const char *s));
		static void			PreliminaryKeyEvent(int keyNum, bool down);
		static bool			IsDown(int keyNum);
		static int			GetUsercmdAction(int keyNum);
		static bool			GetOverstrikeMode(void);
		static void			SetOverstrikeMode(bool state);
		static void			ClearStates(void);
		static int			StringToKeyNum(const char *str);
		static const char 	*KeyNumToString(int keyNum, bool localized);

		static void			SetBinding(int keyNum, const char *binding);
		static const char 	*GetBinding(int keyNum);
		static bool			UnbindBinding(const char *bind);
		static int			NumBinds(const char *binding);
		static bool			ExecKeyBinding(int keyNum);
		static const char 	*KeysFromBinding(const char *bind);
		static const char 	*BindingFromKey(const char *key);
		static bool			KeyIsBoundTo(int keyNum, const char *binding);
		static void			WriteBindings(idFile *f);

		class Modifier {
			static bool			Shift(void) {
				return IsDown(K_SHIFT);
			}
			static bool			Ctrl(void) {
				return IsDown(K_CTRL);
			}
			static bool			Alt(void) {
				return IsDown(K_ALT);
			}
		};
};

#ifdef _SPLASHDAMAGE
extern const int MAX_KEYS;
extern idKey *keys;

class sdKeyCommand
{
public:
    sdKeyCommand( void );

    void					Set( const char* _binding );
    int						GetAction( void ) const
    {
        return action;
    }
    const char*				GetBinding( void ) const
    {
        return binding.c_str();
    }
    usercmdbuttonType_t		GetType( void ) const
    {
        return type;
    }
    void					FixupBind( void );

private:
    idStr					binding;
    int						action;
    usercmdbuttonType_t		type;
};

enum
{
	KM_CTRL,
	KM_SHIFT,
	KM_ALT,
	KM_COMMAND,
	KM_RCTRL,
	KM_RSHIFT,
	KM_RALT,
	KM_RCOMMAND,
	KM_TOTAL
};

class sdKeyBind
{
public:
    static const int MAX_MODIFIERS = 8;
    typedef sdPair< int, sdKeyCommand > pair_t;

    void					ClearCommand( int modifier );
    void					SetCommand( int modifier, const char* command );
    sdKeyCommand&			GetCommand( void );
    sdKeyCommand&			GetCommand( int modifier );
    void					Write( idFile* f, const char* context, const char* keyName );
    void					UnBindBinding( const char* binding );
    void					SetupBinds( void );

private:
    sdKeyCommand							defaultCommand;
    idStaticList< pair_t, MAX_MODIFIERS >	modifierCommands;
};

class sdBindContext
{
public:
    typedef sdPair< int, sdKeyBind* > pair_t;

    sdBindContext( const char* _name )
    {
        name = _name;
		//karin: add HACK: ignore menu, bindmenu, radialmenu
		isMenu = !idStr::Icmp(_name, "menu") || !idStr::Icmp(_name, "radialmenu") || !idStr::Icmp(_name, "bindmenu");
    }
    ~sdBindContext( void )
    {
        UnBindAll();
    }

    sdKeyBind*				AllocBind( int key );
    sdKeyBind*				GetBind( int key );
    const char*				GetName( void ) const
    {
        return name.c_str();
    }
    sdKeyCommand*			GetCommand( int key );
    void					WriteBindings( idFile* f );
    void					Bind( int key, int modifierKey, const char* binding );
    void					UnBind( int key, int modifierKey );
    void					UnBindAll( void );
    void					UnBindBinding( const char* binding );
    void					SetupBinds( void );

	//karin: add HACK: save is handle command event
	bool					IsMenu(void) const {
		return isMenu;
	}

private:
    idList< pair_t >		keys;
    idHashIndex				keyHash;
    idStr					name;
	//karin: add HACK: ignore key command in menu
	bool					isMenu;
};

class idKey
{
public:
    idKey( int _id, const char* _name, const char* _locName, const wchar_t* _fixedText ) : down( false ), id( _id ), activeCommand( NULL )
    {
        name = _name;
        if ( _locName != NULL ) {
            locName = _locName;
        }
        if ( _fixedText != NULL ) {
            fixedText = _fixedText;
        }

    	//karin: compat for DOOM3
    	repeats = 0;
    	usercmdAction = UB_NONE;
    }

    void					SetDown( bool _down );
    bool					IsDown( void ) const
    {
        return down;
    }

    int						GetId( void ) const
    {
        return id;
    }

    void					GetLocalizedText( idWStr& text )
    {
        if ( locName.Length() > 0 ) {
            text = common->LocalizeText( locName.c_str() );
            return;
        }
        text = fixedText;
    }

    void					SetActiveCommand( sdKeyCommand* cmd )
    {
        activeCommand = cmd;
    }
    sdKeyCommand*			GetActiveCommand( void ) const
    {
        return activeCommand;
    }

    const char*				GetName( void ) const
    {
        return name.c_str();
    }

    bool					down;
    int						id;
    idStr					name;
    idStr					locName;
    idWStr					fixedText;
    sdKeyCommand*			activeCommand;

	//karin: compat for DOOM3
	int				repeats;		// if > 1, it is autorepeating
	idStr			binding;
	int				usercmdAction;	// for testing by the asyncronous usercmd generation
    usercmdbuttonType_t		type;

	sdKeyCommand			command;

	idKey() : down( false ), id( 0 ), activeCommand( NULL ), repeats(0), usercmdAction(UB_NONE), type(B_COMMAND)
	{}
};

class sdKeyInputManager
{
public:
    virtual ~sdKeyInputManager() {}

    virtual void			SetBinding( sdBindContext* context, idKey& key, const char* binding, idKey* modifierKey ) = 0;
    virtual const char*		GetBinding( sdBindContext* context, idKey& key, idKey* modifierKey ) = 0;

    virtual void			UnbindBinding( sdBindContext* context, const char *bind ) = 0;
    virtual void			KeysFromBinding( sdBindContext* context, const char* binding, bool useBindStrWhenEmpty, idWStr& keyName ) = 0;

    // pass NULL for keys to find the number of keys to allocate
    virtual void			KeysFromBinding( sdBindContext* context, const char* binding, int& numKeys, idKey** keys ) = 0;

    virtual bool			IsDown( const idKey& key ) = 0;
    virtual bool			IsDown( keyNum_e key ) = 0;
    virtual idKey*			GetKey( const char* name ) = 0;
    virtual idKey*			GetKeyForEvent( const sdSysEvent& evt, bool& down ) = 0;

    virtual void			ProcessUserCmdEvent( const sdSysEvent& event ) = 0;

    virtual sdKeyCommand*	GetCommand( sdBindContext* context, const idKey& key ) = 0;

    virtual sdBindContext*	AllocBindContext( const char* context ) = 0;

    virtual void			UnbindKey(  sdBindContext* context, idKey& key, idKey* modifier = NULL ) = 0;

    virtual bool			AnyKeysDown( void ) = 0;
};

extern sdKeyInputManager* keyInputManager;
#endif

#endif /* !__KEYINPUT_H__ */

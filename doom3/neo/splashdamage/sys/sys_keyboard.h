// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_KEYBOARD__
#define __SYS_KEYBOARD__

#include "sys_input.h"

/*
======================================================================

	Keyboard

======================================================================
*/

class idKeyboardGeneric : public idKeyboard {
public:
	idKeyboardGeneric();
	virtual						~idKeyboardGeneric();

	virtual bool				Init();
	virtual void				Shutdown();

	virtual void				Activate();
	virtual void				Deactivate();

	virtual int					PollInputEvents( bool postEvents );
	virtual int					ReturnInputEvent( const int n, keyNum_t& key, bool& isDown );
	virtual void				EndInputEvents();

	virtual keyNum_t			ConvertScanToKey( unsigned int scanCode ) const;
	virtual keyNum_t			ConvertCharToKey( char ch ) const;
	virtual char				ConvertScanToChar( unsigned int scanCode ) const;
	virtual unsigned int		ConvertCharToScan( char ch ) const;
	virtual char				ConvertKeyToChar( const keyNum_t keyNum ) const;
	virtual bool				IsConsoleKey( const sdSysEvent& event ) const;

	static void					AllocateKeys();
	static unsigned int			StringToScanCode( const char* str );
	static keyNum_t				StringToKeyNum( const char* str );
	static void					KeyNumToString( const keyNum_t keyNum, idWStr& fixedText, idStr& locName );

	static idKey&				GetStandardKey( const keyNum_t key );
};

extern idKeyboard *globalKeyboard;

#endif /* !__SYS_KEYBOARD__ */

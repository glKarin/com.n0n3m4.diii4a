// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __KEYINPUT_H__
#define __KEYINPUT_H__

#include "UsercmdGen.h"
#include "../decllib/declLocStr.h"

/*
===============================================================================

	Key Input

===============================================================================
*/

class sdKeyCommand {
public:
							sdKeyCommand( void );

	void					Set( const char* _binding );
	int						GetAction( void ) const { return action; }
	const char*				GetBinding( void ) const { return binding.c_str(); }
	usercmdbuttonType_t		GetType( void ) const { return type; }
	void					FixupBind( void );

private:
	idStr					binding;
	int						action;
	usercmdbuttonType_t		type;
};

class sdKeyBind {
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

class sdBindContext {
public:
	typedef sdPair< int, sdKeyBind* > pair_t;

							sdBindContext( const char* _name ) { name = _name; }
							~sdBindContext( void ) { UnBindAll(); }

	sdKeyBind*				AllocBind( int key );
	sdKeyBind*				GetBind( int key );
	const char*				GetName( void ) const { return name.c_str();  }
	sdKeyCommand*			GetCommand( int key );
	void					WriteBindings( idFile* f );
	void					Bind( int key, int modifierKey, const char* binding );
	void					UnBind( int key, int modifierKey );
	void					UnBindAll( void );
	void					UnBindBinding( const char* binding );
	void					SetupBinds( void );

private:
	idList< pair_t >		keys;
	idHashIndex				keyHash;
	idStr					name;
};

class idKey {
public:
							idKey( int _id, const char* _name, const char* _locName, const wchar_t* _fixedText ) : down( false ), id( _id ), activeCommand( NULL ) {
								name = _name;
								if ( _locName != NULL ) {
									locName = _locName;
								}
								if ( _fixedText != NULL ) {
									fixedText = _fixedText;
								}
							}

	void					SetDown( bool _down );
	bool					IsDown( void ) const { return down; }

	int						GetId( void ) const { return id; }

	void					GetLocalizedText( idWStr& text ) {
								if ( locName.Length() > 0 ) {
									text = common->LocalizeText( locName.c_str() );
									return;
								}
								text = fixedText;
							}

	void					SetActiveCommand( sdKeyCommand* cmd ) { activeCommand = cmd; }
	sdKeyCommand*			GetActiveCommand( void ) const { return activeCommand; }

	const char*				GetName( void ) const { return name.c_str(); }

protected:
	bool					down;
	int						id;
	idStr					name;
	idStr					locName;
	idWStr					fixedText;
	sdKeyCommand*			activeCommand;
};

class sdController;

class idKeyInput {
public:
	static void				Init( void );
	static void				Shutdown( void );

	static int				GetContextIndex( const char* name );
	static sdBindContext*	GetContext( const char* name );
	static sdBindContext*	AllocContext( const char* name );

	static void				ArgCompletion_KeyName( const idCmdArgs &args, void( *callback )( const char *s ) );

	static bool				IsDown( keyNum_e keyNum );
	static bool				GetOverstrikeMode( void );
	static void				SetOverstrikeMode( bool state );

	static void				ClearStates( void );
	
	static void				SetBinding( idKey& key, const char* binding, idKey* modifier, sdBindContext* context, bool doPrint );
	static const char*		GetBinding( sdBindContext* context, idKey& key, idKey* modifier );
	static void				KeysFromBinding( sdBindContext* context, const char* bind, bool useBindStrWhenEmpty, idWStr& keyName );
	static void				KeysFromBinding( sdBindContext* context, const char* binding, int& numKeys, idKey** keys );
	static void				UnbindKey( sdBindContext* context, idKey& key, idKey* modifier = NULL );

	static void				ListBinds( sdBindContext* context );

	static void				ExecKeyBinding( const sdKeyCommand* cmd );
	static void				WriteBindings( idFile *f );

	static void				SetupBinds( void );

	static void				UnbindAll( void );

	static int				AllocKey( const char* name, const char* locName, const wchar_t* fixedText );
	static idKey&			GetKeyByIndex( int index ) { return *keys[ index ]; }
	static idKey*			GetKey( const char* name );
	static int				GetKeyIndex( const char* name );

	static bool				AnyKeysDown( void );

private:
	static idHashIndex					keysHash;
	static idList< idKey* >				keys;

	static idList< sdBindContext* >		bindContexts;
	static idHashIndex					bindContextsHash;

	static bool							overStrikeMode;

//	static idBlockAlloc< idKey, 128 >	keyAllocator;
};

class sdKeyInputManager {
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

#endif /* !__KEYINPUT_H__ */

// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __KEY_INPUT_MANAGER_LOCAL_H__
#define __KEY_INPUT_MANAGER_LOCAL_H__

class sdKeyInputManagerLocal : public sdKeyInputManager
{
public:
	sdKeyInputManagerLocal();
	virtual ~sdKeyInputManagerLocal();

	virtual void			SetBinding( sdBindContext* context, idKey& key, const char* binding, idKey* modifierKey );
	virtual const char*		GetBinding( sdBindContext* context, idKey& key, idKey* modifierKey );

	virtual void			UnbindBinding( sdBindContext* context, const char *bind );
	virtual void			KeysFromBinding( sdBindContext* context, const char* binding, bool useBindStrWhenEmpty, idWStr& keyName );

	// pass NULL for keys to find the number of keys to allocate
	virtual void			KeysFromBinding( sdBindContext* context, const char* binding, int& numKeys, idKey** keys );

	virtual bool			IsDown( const idKey& key );
	virtual bool			IsDown( keyNum_e key );
	virtual idKey*			GetKey( const char* name );
	virtual idKey*			GetKeyForEvent( const sdSysEvent& evt, bool& down );

	virtual void			ProcessUserCmdEvent( const sdSysEvent& event );

	virtual sdKeyCommand*	GetCommand( sdBindContext* context, const idKey& key );

	virtual sdBindContext*	AllocBindContext( const char* context );

	virtual void			UnbindKey(  sdBindContext* context, idKey& key, idKey* modifier = NULL );

	virtual bool			AnyKeysDown( void );

	void					Init(void);
    const idKey&			GetKeyByNum( int keynum ) const;

private:
	idList<sdBindContext *>	bindContexts;
};

extern sdKeyInputManagerLocal keyInputManagerLocal;

#endif

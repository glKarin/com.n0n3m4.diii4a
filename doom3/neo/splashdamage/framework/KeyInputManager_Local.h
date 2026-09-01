// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __KEY_INPUT_MANAGER_LOCAL_H__
#define __KEY_INPUT_MANAGER_LOCAL_H__

class sdKeyInputManagerLocal : public sdKeyInputManager
{
public:
							sdKeyInputManagerLocal(void);
	virtual					~sdKeyInputManagerLocal(void);

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
	void					BindDefault(void);
    const idKey&			GetKeyByNum( int keynum ) const;
	sdBindContext *			GetBindContext(const char *name);
	void					Write(idFile *f, bool unbindall = false) const;
	void					UnBindAll(void);

	static bool				IsDefaultContext(const char *name) {
		return !name || !name[0] || idStr::Icmp(name, "default") == 0;
	}

private:
	enum bindContextFlag {
		BCF_DEFAULT = 1,
		BCF_MENU = 1 << 1,
	};
	bool					IsMenu(sdBindContext *context) const;
	bool					IsDefault(sdBindContext *context) const {
		return defaultContext == context || !context;
	}
	bool					DefaultBindsFilePath(idStr &ret, const char *lang = NULL) const;

private:
	idList<sdBindContext *>	bindContexts;
	//karin: add HACK: ignore key command in menu
	idList<int>				contextFlags;
	sdBindContext *			defaultContext;
	sdBindContext *			currentContext;
};

extern sdKeyInputManagerLocal keyInputManagerLocal;

#endif

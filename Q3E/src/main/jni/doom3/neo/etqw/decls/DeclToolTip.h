// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLTOOLTIP_H__
#define __DECLTOOLTIP_H__

#undef GetMessage

class sdToolTipParms {
public:
							sdToolTipParms( void ) { }
							~sdToolTipParms( void ) { }

	void					Add( const wchar_t* parm ) { parms.Alloc() = parm; }
	int						Num( void ) const { return parms.Num(); }
	const wchar_t*			Get( int index ) const { return parms[ index ].c_str(); }

private:
	idWStrList				parms;
};

class sdDeclToolTipOption {
public:
	virtual					~sdDeclToolTipOption() { ; }

	virtual const wchar_t*	GetText( sdToolTipParms* formatting ) const = 0;
};

class sdDeclToolTipOptionParm : public sdDeclToolTipOption {
public:
							sdDeclToolTipOptionParm( int _index ) : index( _index ) { ; }
	virtual					~sdDeclToolTipOptionParm() { ; }

	virtual const wchar_t*	GetText( sdToolTipParms* formatting ) const {
		if ( index >= formatting->Num() ) {
			return L"<EMPTY>";
		}
		return formatting->Get( index );
	}

private:
	int						index;
};

class sdDeclToolTipOptionText : public sdDeclToolTipOption {
public:
							sdDeclToolTipOptionText( const wchar_t* _text ) : text( _text ) { ; }
	virtual					~sdDeclToolTipOptionText() { ; }

	virtual const wchar_t*	GetText( sdToolTipParms* formatting ) const { return text.c_str(); }

private:
	idWStr					text;
};

class sdDeclToolTipOptionKey : public sdDeclToolTipOption {
public:
							sdDeclToolTipOptionKey() {}
							sdDeclToolTipOptionKey( const char* _key ) : key( _key ) { ; }
	virtual					~sdDeclToolTipOptionKey() { ; }

	virtual void			SetKey( const char* _key ) { key = _key; }
	virtual const wchar_t*	GetText( sdToolTipParms* formatting ) const;

private:
	idStr					key;
	mutable idWStr			cache;
};

class sdDeclToolTip : public idDecl {
public:
	enum tlEventType_e {
		TL_GUIEVENT,
		TL_PAUSE,
		TL_UNPAUSE,
		TL_SHOWINVENTORY,
		TL_HIDEINVENTORY,
		TL_WAYPOINTHIGHLIGHT,
		TL_LOOKATTASK,
	};

	class timelineEvent_t {
	public:
		tlEventType_e eventType;
		idStr arg1;
	};

	enum tlTime_e {
		TLTIME_END = -1,
	};

	// int == time in percent
	typedef sdPair< int, timelineEvent_t > timelinePair_t;

							sdDeclToolTip( void );
	virtual					~sdDeclToolTip( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	void					GetMessage( sdToolTipParms* formatting, idWStr& text ) const;
	int						GetLength( void ) const { return ( useSoundLength && sound != NULL ) ? sound->GetTimeLength() : length; }

	bool					AddMessage( const wchar_t *text );

	void					SetLastTimeUsed( void ) const;
	int						GetLastTimeUsed( void ) const { return lastTimeUsed; }

	const idSoundShader*	GetSoundShader( void ) const { return sound; }
	int						GetMaxPlayCount( void ) const { return maxPlayCount; }
	int						GetCurrentPlayCount( void ) const;
	int						GetCurrentSinglePlayerPlayCount( void ) const;
	int						GetNextShowDelay( void ) const { return nextShowDelay; }
	const idMaterial*		GetIcon( void ) const { return icon; }
	int						GetLocationIndex( void ) const { return locationIndex; }
	bool					GetSinglePlayerToolTip( void ) const { return singlePlayerToolTip; }
	bool					UsingSoundLength( void ) const { return ( useSoundLength && sound != NULL ) ? true : false; };
	int						GetUnpauseWeaponSlot( void ) const { return unpauseWeaponSlot; }
	const wchar_t*			GetUnpauseKeyString( void ) const { return unpauseKeyString.GetText( NULL ); }

	void					ClearCookies( void ) const;

	static void				CacheFromDict( const idDict& dict );

	void					DumpToFile( idFile* file ) const;

	static void				Cmd_DumpTooltips_f( const idCmdArgs& args );
	static void				Cmd_ClearCookies_f( const idCmdArgs& args );

	const idList< timelinePair_t >& GetTimeline() const { return timeline; }

	bool					GetLookAtObjective( void ) const { return lookAtObjective; }

protected:
	virtual bool			ParseTimeline( idParser& src );

	class message_t {
	public:
							~message_t( void ) { blurbs.DeleteContents( true ); }

		idList< sdDeclToolTipOption* > blurbs;
	};

	idList< message_t* >	messages;
	int						length;
	mutable int				lastTimeUsed;
	const idSoundShader*	sound;
	idStr					category;
	int						locationIndex;
	int						maxPlayCount;
	int						nextShowDelay;
	const idMaterial*		icon;
	bool					useSoundLength;
	idList< timelinePair_t > timeline;
	bool					singlePlayerToolTip;
	int						unpauseWeaponSlot;
	sdDeclToolTipOptionKey	unpauseKeyString;
	bool					lookAtObjective;
};

#endif // __DECLTOOLTIP_H__

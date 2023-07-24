// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLPLAYERCLASS_H__
#define __DECLPLAYERCLASS_H__

class sdDeclGUI;
class sdDeclRadialMenu;
class sdDeclLocStr;
class sdDeclPlayerClass;

class sdPlayerClassContextCallback : public idCVarCallback {
public:
									sdPlayerClassContextCallback( void ) : cvar( NULL ), playerClass( NULL ) { }

	void							Init( const sdDeclPlayerClass* _playerClass ) { playerClass = _playerClass; }
	void							SetCVar( idCVar* _cvar );
	const char*						GetValue( void ) const { return cvar == NULL ? "" : cvar->GetString(); }

	virtual void					OnChanged( void );

private:
	idCVar*							cvar;
	const sdDeclPlayerClass*		playerClass;
};

class sdDeclPlayerClass : public idDecl {
public:
	struct proficiencyUpgrade_t {
		const sdDeclLocStr* text;
		const sdDeclLocStr* title;
		idStr				materialInfo;
		int					level;
		const sdDeclToolTip* toolTip;
		idStr				sound;
	};

	struct proficiencyCategory_t {
		const sdDeclLocStr*					text;
		int									index;
		idList< proficiencyUpgrade_t >		upgrades;
	};

	struct stats_t {
		sdPlayerStatEntry*				timePlayed;
		sdPlayerStatEntry*				deaths;
		sdPlayerStatEntry*				suicides;
		sdPlayerStatEntry*				revived;
		sdPlayerStatEntry*				tapouts;
		sdPlayerStatEntry*				respawns;
	};

	typedef idList< const sdDeclItemPackage* > optionList_t;

								sdDeclPlayerClass( void );
	virtual						~sdDeclPlayerClass( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	sdTeamInfo*					GetTeam( void ) const					{ return team; }
	const sdDeclItemPackage*	GetPackage( void ) const				{ return package; }
	const sdDeclItemPackage*	GetDisguisePackage( void ) const		{ return disguisePackage; }
	const sdDeclLocStr*			GetTitle( void ) const 					{ return this != NULL ? title : NULL; }
	const idDeclModelDef*		GetModel( void ) const 					{ return playerModel; }
	const idMaterial*			GetCommandmapIcon( void ) const			{ return cmIcon; }
	const idMaterial*			GetCommandmapIconClass( void ) const	{ return cmIconClass; }
	const idMaterial*			GetCommandmapIconUnknown( void ) const	{ return cmIconUnknown; }
	const idMaterial*			GetIconClass( void ) const 				{ return iconClass; }
	const idMaterial*			GetIconOffScreen( void ) const 			{ return iconOffScreen; }
	const idMaterial*			GetIconFriendlyArrow( void ) const 		{ return iconFriendlyArrow; }
	const idMaterial*			GetIconEnemyArrow( void ) const 		{ return iconEnemyArrow; }
	const idDict&				GetModelData( void ) const 				{ return modelData; }
	const idDict&				GetClassData( void ) const 				{ return classData; }
	int							GetAmmoLimit( ammoType_t type ) const	{ return ammoLimits[ type ]; }
	int							GetTotalAmmoLimit( void ) const			{ return totalAmmoLimit; }
	int							GetNumOptions( void ) const				{ return options.Num(); }
	const optionList_t&			GetOption( int index ) const			{ return options[ index ]; }
	const sdDeclGUI*			GetHUDOverlay( void ) const				{ return classOverlay; }
	const char*					GetClassThreadName( void ) const		{ return classThreadName; }
	const char*					GetClimateSkinKey( void ) const			{ return climateSkinKey; }
	idCVar*						GetLimitCVar( void ) const 				{ return limitCVar; }
	int							GetMaxHealth( void ) const 				{ return maxHealth; }
	const idDeclEntityDef*		GetBodyDef( void ) const				{ return deadBodyDef; }
	const playerClassTypes_t&	GetPlayerClassNum( void ) const			{ return playerClassNum; }
	sdBindContext*				GetBindContext( void ) const			{ return bindContext; }

	void						OnContextCVarChanged( void ) const;
	void						OnInputInit( void ) const;
	void						OnInputShutdown( void ) const;

	int							GetNumProficiencies( void ) const	{ return proficiencies.Num(); }
	const proficiencyCategory_t&GetProficiency( int index ) const	{ return proficiencies[ index ]; }

	static void					CacheFromDict( const idDict& dict );

	bool						HasAbility( qhandle_t handle ) const { return abilities.HasAbility( handle ); }

	const char*					BuildQuickChatDeclName( const char* qc ) const;

	const stats_t&				GetStats( void ) const { return stats; }

private:
	void						ReadFromDict( const idDict& info );
	bool						ParseOption( idParser& src );
	bool						ParseProficiency( proficiencyCategory_t& category, idParser& src );

private:
	idList< proficiencyCategory_t > proficiencies;

	const sdDeclItemPackage*	package;
	const sdDeclItemPackage*	disguisePackage;
	const idDeclModelDef*		playerModel;
	sdAbilityProvider			abilities;
	const idMaterial*			cmIcon;
	const idMaterial*			cmIconClass;
	const idMaterial*			cmIconUnknown;
	const idMaterial*			iconClass;
	const idMaterial*			iconFriendlyArrow;
	const idMaterial*			iconEnemyArrow;
	const idMaterial*			iconOffScreen;
	const sdDeclLocStr*			title;
	sdTeamInfo*					team;
	idDict						modelData;
	idDict						classData;
	idList< int >				ammoLimits;
	int							totalAmmoLimit;
	int							maxHealth;
	idList< optionList_t >		options;
	const sdDeclGUI*			classOverlay;
	idStr						classThreadName;
	idStr						climateSkinKey;
	idCVar*						limitCVar;
	stats_t						stats;
	const idDeclEntityDef*		deadBodyDef;
	playerClassTypes_t			playerClassNum;

	sdPlayerClassContextCallback	contextCallback;
	mutable sdBindContext*			bindContext;
};

#endif // __DECLPLAYERCLASS_H__

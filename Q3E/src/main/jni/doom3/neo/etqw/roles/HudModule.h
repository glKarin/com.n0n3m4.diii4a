// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ROLES_HUDMODULE_H__
#define __GAME_ROLES_HUDMODULE_H__

#include "ObjectiveManager.h"

class sdUserInterfaceLocal;

class sdHudModule : public sdUIPropertyHolder {
	friend class sdPlayerProperties;
public:
										sdHudModule( void );
										~sdHudModule( void );

	void								InitGui( const char* guiName, bool permanent = true );

	void								Enable( bool enable, bool urgent = false, bool timedOut = false );

	bool								Active( void ) const { return _activated; }
	bool								Enabled( void ) const { return _enabled; }	

	bool								ManualDraw( void ) const { return _manualDraw; }
	bool								AllowInhibit( void ) const { return _allowInhibit; }
	bool								InhibitGuiFocus( void ) const { return _inhibitGuiFocus; }
	bool								InhibitUserCommands( void ) const { return _inhibitUserCommands; }
	bool								InhibitControllerMovement( void ) const { return _inhibitControllerMovement; }
	bool								HideWeapon( void ) const { return _hideWeapon; }

	virtual bool						GetSensitivity( float& x, float& y ) { return false; }
	virtual bool						HandleGuiEvent( const sdSysEvent* event );
	virtual void						HandleInput( void ) { ; }
	virtual void						UsercommandCallback( usercmd_t& cmd ) { }
	virtual void						OnActivate( void ) { ; }
	virtual void						OnCancel( void ) { ; }
	virtual void						Update( void );
	virtual bool						DoWeaponLock( void ) const { return true; }

	virtual void						Draw( void );

	virtual bool						GetFov( float& fov ) const { return false; }

	void								SetSort( int value ) { _sort = value; }
	int									GetSort() const { return _sort; }

	void								SetAllowInhibit( bool set ) { _allowInhibit = set; }

	sdUserInterfaceLocal*				GetGui( void ) const;
	guiHandle_t							GetGuiHandle( void ) const { return _guiHandle; }
	idLinkList< sdHudModule >&			GetNode( void ) { return _node; }
	idLinkList< sdHudModule >&			GetDrawNode( void ) { return _drawNode; }
	
	virtual sdUserInterfaceScope*		GetScope( void ) { return this; }

	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdProperty*			GetProperty( const char* name );
	virtual sdProperties::sdPropertyHandler&	GetProperties() { return _properties; }
	virtual const char*							GetName( void ) const { return "module"; }
	virtual const char*							FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return _properties.NameForProperty( property ); }

private:
	void								Activate( void );

protected:
	bool								_manualDraw;
	bool								_allowInhibit;
	bool								_inhibitUserCommands;
	bool								_inhibitControllerMovement;
	bool								_inhibitGuiFocus;
	bool								_passive;
	bool								_hideWeapon;

	sdProperties::sdPropertyHandler		_properties;

private:
	int									_sort;
	bool								_enabled;
	bool								_activated;
	idLinkList< sdHudModule >			_node;
	idLinkList< sdHudModule >			_drawNode;
	guiHandle_t							_guiHandle;

};

class sdLimboMenu : public sdHudModule {
public:
										sdLimboMenu( void );
	virtual								~sdLimboMenu( void ) { ; }

	virtual bool						HandleGuiEvent( const sdSysEvent* event );
	virtual void						OnCancel( void );
	virtual void						OnActivate( void );
	virtual void						Update( void );
};

class sdDeployMenu : public sdHudModule {
public:
										sdDeployMenu( void );
	virtual								~sdDeployMenu( void ) { FreeDecals(); }

	virtual void						HandleInput( void );
	virtual void						UsercommandCallback( usercmd_t& cmd );
	virtual void						OnCancel( void );
	virtual void						OnActivate( void );
	virtual void						Update( void );
	virtual bool						HandleGuiEvent( const sdSysEvent* event );

	void								SetObject( const sdDeclDeployableObject* object );
	void								SetState( deployResult_t state ) { deployableState = state; }

	void								FreeDecals( void );

	void								SetDeployMode( bool mode );
	bool								GetDeployMode( void ) const { return modeToggle; }
	float								GetRotation( void ) const { return rotation; }
	const idVec3&						GetPosition( void ) const { return position; }
	bool								AllowRotation( void ) const;

private:
	renderEntity_t						deployableRenderEntity;
	qhandle_t							deployableRenderEntityHandle;

	const sdDeclDeployableObject*		deployableObject;
	deployResult_t						deployableState;

	const idMaterial*					decalMaterial;
	const idMaterial*					decalMaterialOuter;
	const idMaterial*					decalMaterialArrows;

	int									decalHandle;
	int									decalHandleOuter;
	int									decalHandleArrows;

	short								lockedAngles[ 3 ];
	float								rotation;
	idVec3								position;
	bool								modeToggle;

	deployMaskExtents_t					lastExpandedExtents;
	bool								lastTerritory[ 10 * 10 ];

};

class sdFireTeamMenu : public sdHudModule {
public:
										sdFireTeamMenu( void ) { _inhibitUserCommands = false; }
	virtual								~sdFireTeamMenu( void ) { }
	virtual bool						HandleGuiEvent( const sdSysEvent* event );

private:	
};

class sdRadialMenuModule : public sdHudModule {
public:
										sdRadialMenuModule( void ) { }
	virtual								~sdRadialMenuModule( void ) { }
	virtual bool						HandleGuiEvent( const sdSysEvent* event );

	virtual bool						DoWeaponLock( void ) const;

	virtual bool						GetSensitivity( float& x, float& y );

	virtual void						UsercommandCallback( usercmd_t& cmd );

private:	
};

class sdQuickChatMenu : public sdRadialMenuModule {
public:
										sdQuickChatMenu( void ) { _inhibitControllerMovement = true; }
	virtual								~sdQuickChatMenu( void ) { }
	
	virtual void						OnActivate( void );
	virtual void						OnCancel( void );

};

class sdChatMenu : public sdHudModule {
public:
										sdChatMenu( void ) { _inhibitUserCommands = true; }
	virtual								~sdChatMenu( void ) { }
	virtual bool						HandleGuiEvent( const sdSysEvent* event );

private:
};

class sdTakeViewNoteMenu : public sdHudModule {
public:
										sdTakeViewNoteMenu( void ) { _inhibitUserCommands = true; }
	virtual								~sdTakeViewNoteMenu( void ) { }
	virtual bool						HandleGuiEvent( const sdSysEvent* event );

private:
};

class sdPassiveHudModule : public sdHudModule {
public:
										sdPassiveHudModule( void ) { _passive = true; _allowInhibit = true; }

	virtual bool						DoWeaponLock( void ) const { return false; }
	

private:
};

class sdPlayerHud : public sdPassiveHudModule {
public:
	sdPlayerHud( void ) { ; }
	~sdPlayerHud( void ) { ; }

private:
};

class sdPostProcess : public sdPassiveHudModule {
public:
	sdPostProcess( void ) { _allowInhibit = false; _manualDraw = true; }
	~sdPostProcess( void ) { ; }

	virtual void						Draw( void ) {}
	void								DrawPost( void );
private:
};

class sdWeaponSelectionMenu : public sdPassiveHudModule {
public:
	enum eActivationType { AT_DISABLED, AT_ENABLED, AT_DIRECT_SELECTION };
										sdWeaponSelectionMenu( void ) { switchActive = AT_DISABLED; _allowInhibit = false; }
	virtual								~sdWeaponSelectionMenu( void ) { ; }

	virtual bool						DoWeaponLock( void ) const;

	void								SetSwitchActive( eActivationType set );
	bool								IsMenuVisible( void ) const { return switchActive != AT_DISABLED; }
	bool								IsPassiveSwitchActive( void ) const { return switchActive == AT_ENABLED; }
	virtual void						UsercommandCallback( usercmd_t& cmd );

private:
	void								UpdateGui( void );

private:
	eActivationType switchActive;
};

#endif // __GAME_ROLES_HUDMODULE_H__

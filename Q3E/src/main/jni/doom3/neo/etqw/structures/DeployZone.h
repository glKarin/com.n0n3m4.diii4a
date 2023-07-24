// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_STRUCTURES_DEPLOYZONE_H__
#define __GAME_STRUCTURES_DEPLOYZONE_H__

#include "../ScriptEntity.h"

class sdDeployZoneBroadcastData : public sdScriptEntityBroadcastData {
public:
										sdDeployZoneBroadcastData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	bool								active;
};

class sdDeployZone : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( sdDeployZone );

										sdDeployZone( void );
										~sdDeployZone( void );

	void								Spawn( void );

	bool								IsActive( void ) const { return _active; }
	void								SetActive( bool active ) { _active = active; }

	void								Event_SetActive( bool active ) { SetActive( active != 0.f ); }

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

private:
	bool								_active;
};

class sdPlayZoneEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdPlayZoneEntity );

										sdPlayZoneEntity( void );
										~sdPlayZoneEntity( void );

	void								Spawn( void );

private:
};

class sdPlayZoneMarker : public idEntity {
public:
	CLASS_PROTOTYPE( sdPlayZoneMarker );

										sdPlayZoneMarker( void );
										~sdPlayZoneMarker( void );

	void								Spawn( void );

private:
};


#endif // __GAME_STRUCTURES_DEPLOYZONE_H__

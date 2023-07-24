// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_STRUCTURES_DEPLOYREQUEST_H__
#define __GAME_STRUCTURES_DEPLOYREQUEST_H__

class sdDeployRequest {
public:
	typedef sdDeployRequest*		deployRequestPtr_t;

									sdDeployRequest( idFile* file );
									sdDeployRequest( const sdDeclDeployableObject* _object, idPlayer* _owner, const idVec3& _position, float _rotation, sdTeamInfo* _team, int delayMS );
									~sdDeployRequest( void );

	static void						UpdateRenderEntity( renderEntity_t& renderEntity, const idVec4& color, const idVec3& position );
	void							SetupModel( void );
	bool							CheckBlock( const idBounds& bounds );

	const sdDeclDeployableObject*	GetObject( void ) const { return object; }

	bool							Update( idPlayer* player );

	void							Show( void );
	void							Hide( void );

	void							WriteCreateEvent( int index, const sdReliableMessageClientInfoBase& info ) const;
	void							WriteDestroyEvent( int index ) const;

	void							Write( idFile* file ) const;

private:
	bool							CallForDropOff( void );

	idEntityPtr< idPlayer >			owner;
	sdTeamInfo*						team;
	const sdDeclDeployableObject*	object;

	renderEntity_t					renderEntity;
	qhandle_t						renderEntityHandle;
	idVec3							position;
	idBounds						bounds;
	int								callTime;
	float							rotation;
};

#endif // __GAME_STRUCTURES_DEPLOYREQUEST_H__

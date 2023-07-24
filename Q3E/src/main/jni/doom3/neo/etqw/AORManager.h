// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __AORMANAGER_H__
#define __AORMANAGER_H__

class idEntity;

class sdAORManagerLocal {
public:
							sdAORManagerLocal( void );
							~sdAORManagerLocal( void );

	void					Init( void );
	void					OnMapLoad( void );
	void					Shutdown( void );
	void					FreePVS( void );
	void					Setup( void );

	void					UpdateEntityAORFlags( idEntity* ent, const idVec3& otherPos, bool& shouldUpdate );
	int						GetFlagsForPoint( const sdDeclAOR* aorLayout, const idVec3& otherPos );

	void					DebugDraw( int clientNum );
	void					DebugDrawEntities( int clientNum );

	void					SetClient( idPlayer* client );
	void					SetPosition( const idVec3& position );

	void					SetSnapShotPlayer( idPlayer* player );

protected:
	float					GetDistSqForPoint( const idVec3& pos );
	bool					CheckEntity( idEntity* ent, int spread );

	idVec2					cachedViewOrg;
	idVec2					cachedViewDir;
	float					ringScale;
	float					ringScaleFactor;
	float					priorityScale;
	float					priorityDot;

	int						snapShotPlayerIndex;

	pvsHandle_t				pvsHandle;
};

#endif // __AORMANAGER_H__


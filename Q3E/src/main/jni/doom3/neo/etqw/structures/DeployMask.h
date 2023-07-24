// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_STRUCTURES_DEPLOYMASK_H__
#define __GAME_STRUCTURES_DEPLOYMASK_H__

struct deployMaskExtents_t;

class sdDeployMaskEditSession : public idClass {
public:
	CLASS_PROTOTYPE( sdDeployMaskEditSession );

									sdDeployMaskEditSession( void );
									~sdDeployMaskEditSession( void );

	void							Event_OpenMask( const char* maskName );
	void							Event_UpdateProjection( const idVec3& position );
	void							Event_SetDecalMaterial( const char* materialName );
	void							Event_SetStampSize( int size );
	void							Event_Stamp( const idVec3& position, bool save, bool state );
	void							Event_SaveAll( void );

	void							FreeDecals( void );
	bool							SetMaskState( const idVec3& position, bool save, bool state );
	void							UpdateProjection( const idVec3& position );
	void							GetExtents( const idVec3& position, const sdDeployMaskInstance& mask, deployMaskExtents_t& extents );
	sdDeployMaskInstance*			GetMask( const idVec3& position );
	const sdHeightMapInstance*		GetHeightMap( const idVec3& position );

	virtual idScriptObject*			GetScriptObject( void ) const { return scriptObject; }

private:
	qhandle_t						maskHandle;
	idScriptObject*					scriptObject;
	int								decalHandle;
	const idMaterial*				decalMaterial;
	int								stampSize;
};

#endif // __GAME_STRUCTURES_DEPLOYMASK_H__

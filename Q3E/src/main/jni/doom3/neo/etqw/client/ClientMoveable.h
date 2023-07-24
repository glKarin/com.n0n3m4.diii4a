// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientMoveable.h
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_MOVEABLE_H__
#define __GAME_CLIENT_MOVEABLE_H__

#include "ClientEffect.h"
#include "../physics/Physics_RigidBody.h"

struct trace_t;

class rvClientMoveable : public rvClientEntity {
public:

	CLASS_PROTOTYPE( rvClientMoveable );

	rvClientMoveable ( void );
	~rvClientMoveable ( void );
	
	virtual void			Spawn			( const idDict* args, int effectSet = 0 );
	virtual void			Think			( void );
	virtual idPhysics*		GetPhysics		( void ) const;	
	virtual bool			Collide			( const trace_t& collision, const idVec3 &velocity );
	
	renderEntity_t*			GetRenderEntity	( void );
	
	virtual const idDict*	GetSpawnArgs( void ) const { return spawnArgs; }

protected:

	void					FreeEntityDef	 ( void );

	renderEntity_t			renderEntity;
	int						entityDefHandle;

	rvClientEffectPtr		trailEffect;
	float					trailAttenuateSpeed;
		
	idPhysics_RigidBody		physicsObj;
	
	int						bounceSoundTime;
	const idSoundShader*	bounceSoundShader;
	bool					firstBounce;

	const sdDeclAOR*		aorLayout;

	const idDict*			spawnArgs;

	int						effectSet;
	
private:
	
	void					Event_FadeOut			( int duration );
};

ID_INLINE renderEntity_t* rvClientMoveable::GetRenderEntity ( void ) {
	return &renderEntity;
}

extern const idEventDefInternal CL_FadeOut;

#endif // __GAME_CLIENT_MOVEABLE_H__

// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WATEREFFECTS_H__
#define __GAME_WATEREFFECTS_H__

#include "Effects.h"
#include "Wakes.h"

//===============================================================
//
//	sdWaterEffects
//		just add this to any object to get splashes and wakes going
//
//===============================================================


class sdWaterEffects {
public:

	sdWaterEffects();
	~sdWaterEffects( void );

	// Both can be Null if not wanted, making sure they are precached is up to the user.
	void Init( const char *splashEffectName, const char *wakeEffectName, idVec3 &offset, const idDict &wakeArgs );

	// Check the object against the water collision model, updating effects and splashes if needed
	void CheckWater( idEntity *ent, const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel, bool showWake = true );

	// Skips some checks if the user already has this data
	void CheckWater( idEntity *ent, bool newWaterState, float waterlevel, bool submerged, bool showWake = true );

	// Update the effects only
	void CheckWaterEffectsOnly( void );

	// Set the origin where the effects need to be spawned
	void SetOrigin( const idVec3 &origin );

	// Set the axis of the spawning object
	void SetAxis( const idMat3 &axis );

	void SetVelocity( const idVec3 &velocity );

	// This velocity will cause the atten of the effects to go to 1
	void SetMaxVelocity( float max) { 
		maxVelocity = max;
	}

	// This velocity will cause the atten of the effects to go to 1
	void SetAtten( float atten) { 
		this->atten = atten;
	}

	// Reset water effect state
	void ResetWaterState() { inWater = false; }

	// Convenience 
	static sdWaterEffects *SetupFromSpawnArgs( const idDict &args  );

protected:
	sdEffect	splash;//Damage!
	sdEffect	wake;
	idVec3		origin;
	idVec3		offset;	// Allows a constant offset (in object local space) from the vehicle origin (for stuff like boats that float high in the water)
	idMat3		axis;
	bool		inWater;
	float		atten;
	float		maxVelocity;
	bool		wakeStopped;
	idVec3		velocity;
	int			wakeHandle;
	sdWakeParms	wakeParms;
};


#endif //__GAME_WATEREFFECTS_H__

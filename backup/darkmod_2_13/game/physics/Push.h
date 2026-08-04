/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __PUSH_H__
#define __PUSH_H__

/*
===============================================================================

  Allows physics objects to be pushed geometrically.

===============================================================================
*/

#define PUSHFL_ONLYMOVEABLE			1		// only push moveable entities
#define PUSHFL_NOGROUNDENTITIES		2		// don't push entities the clip model rests upon
#define PUSHFL_CLIP					4		// also clip against all non-moveable entities
#define PUSHFL_CRUSH				8		// kill blocking entities
#define PUSHFL_APPLYIMPULSE			16		// apply impulse to pushed entities
#define PUSHFL_NOPLAYER				32		// greebo: Don't push player entities

//#define NEW_PUSH

class idPush {
public:
					// Try to push other entities by moving the given entity.
					// If results.fraction < 1.0 the move was blocked by results.c.entityNum
					// Returns total mass of all pushed entities.
	float			ClipTranslationalPush( trace_t &results, idEntity *pusher, const int flags,
											const idVec3 &newOrigin, const idVec3 &move, float ImpulseMod = 1.0f );

	float			ClipRotationalPush( trace_t &results, idEntity *pusher, const int flags,
											const idMat3 &newAxis, const idRotation &rotation );

	float			ClipPush( trace_t &results, idEntity *pusher, const int flags,
											const idVec3 &oldOrigin, const idMat3 &oldAxis,
												idVec3 &newOrigin, idMat3 &newAxis );

					// initialize saving the positions of entities being pushed
	void			InitSavingPushedEntityPositions( void );
					// move all pushed entities back to their previous position
	void			RestorePushedEntityPositions( void );
					// returns the number of pushed entities
	int				GetNumPushedEntities( void ) const { return pushed.Num(); }
					// get the ith pushed entity
	idEntity *		GetPushedEntity( int i ) const { assert( i >= 0 && i < pushed.Num() ); return pushed[i].ent; }

private:
	static const int PUSHED_AUTOSIZE = 128;
	struct pushed_s {
		idEntity *	ent;					// pushed entity
		idAngles	deltaViewAngles;		// actor delta view angles
	};
	idFlexList<pushed_s, PUSHED_AUTOSIZE> pushed;	// pushed entities

	struct pushedGroup_s {
		idEntity *	ent;
		float		fraction;
		bool		groundContact;
		bool		test;
	};
	idFlexList<pushedGroup_s, PUSHED_AUTOSIZE> pushedGroup;

private:
	void			SaveEntityPosition( idEntity *ent );
	bool			RotateEntityToAxial( idEntity *ent, idVec3 rotationPoint );
#ifdef NEW_PUSH
	bool			CanPushEntity( idEntity *ent, idEntity *pusher, idEntity *initialPusher, const int flags );
	void			AddEntityToPushedGroup( idEntity *ent, float fraction, bool groundContact );
	bool			IsFullyPushed( idEntity *ent );
	bool			ClipTranslationAgainstPusher( trace_t &results, idEntity *ent, idEntity *pusher, const idVec3 &translation );
	int				GetPushableEntitiesForTranslation( idEntity *pusher, idEntity *initialPusher, const int flags,
											const idVec3 &translation, idEntity *entityList[], int maxEntities );
	bool			ClipRotationAgainstPusher( trace_t &results, idEntity *ent, idEntity *pusher, const idRotation &rotation );
	int				GetPushableEntitiesForRotation( idEntity *pusher, idEntity *initialPusher, const int flags,
											const idRotation &rotation, idEntity *entityList[], int maxEntities );
#else
	void			ClipEntityRotation( trace_t &trace, const idEntity *ent, const idClipModel *clipModel,
										idClipModel *skip, const idRotation &rotation );
	void			ClipEntityTranslation( trace_t &trace, const idEntity *ent, const idClipModel *clipModel,
										idClipModel *skip, const idVec3 &translation );
	int				TryTranslatePushEntity( trace_t &results, idEntity *check, idClipModel *clipModel, const int flags,
												const idVec3 &newOrigin, const idVec3 &move );
	int				TryRotatePushEntity( trace_t &results, idEntity *check, idClipModel *clipModel, const int flags,
												const idMat3 &newAxis, const idRotation &rotation );
	int				DiscardEntities( idClip_EntityList &entityList, int flags, idEntity *pusher );
#endif
};

#endif /* !__PUSH_H__ */

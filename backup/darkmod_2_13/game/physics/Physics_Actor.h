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

#ifndef __PHYSICS_ACTOR_H__
#define __PHYSICS_ACTOR_H__

/*
===================================================================================

	Actor physics base class

	An actor typically uses one collision model which is aligned with the gravity
	direction. The collision model is usually a simple box with the origin at the
	bottom center.

===================================================================================
*/

#ifdef MOD_WATERPHYSICS
typedef enum {					// MOD_WATERPHYSICS
	WATERLEVEL_NONE = 0,		// MOD_WATERPHYSICS
	WATERLEVEL_FEET,			// MOD_WATERPHYSICS
	WATERLEVEL_WAIST,			// MOD_WATERPHYSICS
	WATERLEVEL_HEAD				// MOD_WATERPHYSICS
} waterLevel_t;					// MOD_WATERPHYSICS
// As of #4159, TDM 2.04, the water levels are used by the player script object too. 
// Any future changes need to be reflected there. Also added an explicit =0 to
// WATERLEVEL_NONE above, because scripts will want to use AI_INWATER as a boolean. 
#endif // MOD_WATERPHYSICS

class idPhysics_Actor : public idPhysics_Base {

public:
	CLASS_PROTOTYPE( idPhysics_Actor );

							idPhysics_Actor( void );
	virtual					~idPhysics_Actor( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// get delta yaw of master
	float					GetMasterDeltaYaw( void ) const;
							// returns the ground entity
	idEntity *				GetGroundEntity( void ) const;
							// align the clip model with the gravity direction
	void					SetClipModelAxis( void );

#ifdef MOD_WATERPHYSICS
	virtual waterLevel_t	GetWaterLevel( void ) const; 	// MOD_WATERPHYSICS
	virtual int				GetWaterType( void ) const; 	// MOD_WATERPHYSICS

	// greebo: returns the time we (last) submersed into water (above HEAD)
	int						GetSubmerseTime() const;
#endif

public:	// common physics interface
	virtual void			SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true ) override;
	virtual idClipModel *	GetClipModel( int id = 0 ) const override;
	virtual int				GetNumClipModels( void ) const override;

	virtual void			SetMass( float mass, int id = -1 ) override;
	virtual float			GetMass( int id = -1 ) const override;

	virtual void			SetContents( int contents, int id = -1 ) override;
	virtual int				GetContents( int id = -1 ) const override;

	virtual const idBounds &GetBounds( int id = -1 ) const override;
	virtual const idBounds &GetAbsBounds( int id = -1 ) const override;

	virtual bool			IsPushable( void ) const override;

	virtual const idVec3 &	GetOrigin( int id = 0 ) const override;
	virtual const idMat3 &	GetAxis( int id = 0 ) const override;

	virtual void			SetGravity( const idVec3 &newGravity ) override;
	const idMat3 &			GetGravityAxis( void ) const;

	virtual void			ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const override;
	virtual void			ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const override;
	virtual int				ClipContents( const idClipModel *model ) const override;

	virtual void			DisableClip( void ) override;
	virtual void			EnableClip( void ) override;

	virtual void			UnlinkClip( void ) override;
	virtual void			LinkClip( void ) override;

	virtual bool			EvaluateContacts( void ) override;

protected:
#ifdef MOD_WATERPHYSICS
	virtual void		SetWaterLevel( bool updateWaterLevelChanged );		// MOD_WATERPHYSICS
	waterLevel_t		waterLevel;					// MOD_WATERPHYSICS
	waterLevel_t		previousWaterLevel;			// greebo: The water level of the previous frame
	int					waterType;					// MOD_WATERPHYSICS
	int					submerseFrame;				// greebo: The frame in which we submersed (above WATERLEVEL_HEAD)
	int					submerseTime;				// greebo: The time we submersed (above WATERLEVEL_HEAD)

	// greebo: This is TRUE if the water level has changed since the last physics evaluation (frame)
	bool				waterLevelChanged;
#endif

	idClipModel *			clipModel;			// clip model used for collision detection
	idMat3					clipModelAxis;		// axis of clip model aligned with gravity direction

	// derived properties
	float					mass;
	float					invMass;

	// master
	idEntity *				masterEntity;
	float					masterYaw;
	float					masterDeltaYaw;

	// results of last evaluate
	idEntityPtr<idEntity>	groundEntityPtr;
};

#endif /* !__PHYSICS_ACTOR_H__ */

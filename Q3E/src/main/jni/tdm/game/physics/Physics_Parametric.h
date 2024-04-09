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

#ifndef __PHYSICS_PARAMETRIC_H__
#define __PHYSICS_PARAMETRIC_H__

/*
===================================================================================

	Parametric physics

	Used for predefined or scripted motion. The motion of an object is completely
	parametrized. By adjusting the parameters an object is forced to follow a
	predefined path. The parametric physics is typically used for doors, bridges,
	rotating fans etc.

===================================================================================
*/

typedef struct parametricPState_s {
	int										time;					// physics time
	int										atRest;					// set when simulation is suspended
	idVec3									origin;					// world origin
	idAngles								angles;					// world angles
	idMat3									axis;					// world axis
	idVec3									localOrigin;			// local origin
	idAngles								localAngles;			// local angles
	idExtrapolate<idVec3>					linearExtrapolation;	// extrapolation based description of the position over time
	idExtrapolate<idAngles>					angularExtrapolation;	// extrapolation based description of the orientation over time
	idInterpolateAccelDecelLinear<idVec3>	linearInterpolation;	// interpolation based description of the position over time
	idInterpolateAccelDecelLinear<idAngles>	angularInterpolation;	// interpolation based description of the orientation over time
	idCurve_Spline<idVec3> *				spline;					// spline based description of the position over time
	idInterpolateAccelDecelLinear<float>	splineInterpolate;		// position along the spline over time
	bool									useSplineAngles;		// set the orientation using the spline
} parametricPState_t;

class idPhysics_Parametric : public idPhysics_Base {

public:
	CLASS_PROTOTYPE( idPhysics_Parametric );

							idPhysics_Parametric( void );
	virtual					~idPhysics_Parametric( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					SetPusher( int flags );
	bool					IsPusher( void ) const;

	void					SetLinearExtrapolation( extrapolation_t type, int time, int duration, const idVec3 &base, const idVec3 &speed, const idVec3 &baseSpeed );
	void					SetAngularExtrapolation( extrapolation_t type, int time, int duration, const idAngles &base, const idAngles &speed, const idAngles &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType( void ) const;
	extrapolation_t			GetAngularExtrapolationType( void ) const;

	void					SetLinearInterpolation( int time, int accelTime, int decelTime, int duration, const idVec3 &startPos, const idVec3 &endPos );
	void					SetAngularInterpolation( int time, int accelTime, int decelTime, int duration, const idAngles &startAng, const idAngles &endAng );

	void					SetSpline( idCurve_Spline<idVec3> *spline, int accelTime, int decelTime, bool useSplineAngles );
	idCurve_Spline<idVec3> *GetSpline( void ) const;
	int						GetSplineAcceleration( void ) const;
	int						GetSplineDeceleration( void ) const;
	bool					UsingSplineAngles( void ) const;

	void					GetLocalOrigin( idVec3 &curOrigin ) const;
	const idVec3&			GetLocalOrigin() const;
	void					SetLocalOrigin( const idVec3 &newOrigin);
	void					GetLocalAngles( idAngles &curAngles ) const;
	const idAngles&			GetLocalAngles() const;
	void					SetLocalAngles(idAngles curAngles);

	void					GetAngles( idAngles &curAngles ) const;

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

	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec ) override;
	virtual void			UpdateTime( int endTimeMSec ) override;
	virtual int				GetTime( void ) const override;

	virtual void			Activate( void ) override;
	virtual bool			IsAtRest( void ) const override;
	virtual int				GetRestStartTime( void ) const override;
	virtual bool			IsPushable( void ) const override;

	virtual void			SaveState( void ) override;
	virtual void			RestoreState( void ) override;

	virtual void			SetOrigin( const idVec3 &newOrigin, int id = -1 ) override;
	virtual void			SetAxis( const idMat3 &newAxis, int id = -1 ) override;

	virtual void			Translate( const idVec3 &translation, int id = -1 ) override;
	virtual void			Rotate( const idRotation &rotation, int id = -1 ) override;

	virtual const idVec3 &	GetOrigin( int id = 0 ) const override;
	virtual const idMat3 &	GetAxis( int id = 0 ) const override;

	virtual void			SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 ) override;
	virtual void			SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 ) override;

	virtual const idVec3 &	GetLinearVelocity( int id = 0 ) const override;
	virtual const idVec3 &	GetAngularVelocity( int id = 0 ) const override;

	virtual void			DisableClip( void ) override;
	virtual void			EnableClip( void ) override;

	virtual void			UnlinkClip( void ) override;
	virtual void			LinkClip( void ) override;

	virtual void			SetMaster( idEntity *master, const bool orientated = true ) override;

	virtual const trace_t *	GetBlockingInfo( void ) const override;
	virtual idEntity *		GetBlockingEntity( void ) const override;

	virtual int				GetLinearEndTime( void ) const override;
	virtual int				GetAngularEndTime( void ) const override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	// SteveL #3712
	bool					pushFlagOverride( int flag, bool forcedValue ); // return old state of flag
private:
	// parametric physics state
	parametricPState_t		current;
	parametricPState_t		saved;

	// pusher
	bool					isPusher;
	idClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;

private:
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
};

#endif /* !__PHYSICS_PARAMETRIC_H__ */

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

#ifndef __PHYSICS_BASE_H__
#define __PHYSICS_BASE_H__

/*
===============================================================================

	Physics base for a moving object using one or more collision models.

===============================================================================
*/

#define contactEntity_t		idEntityPtr<idEntity>

class idPhysics_Base : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_Base );

							idPhysics_Base( void );
	virtual					~idPhysics_Base( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

public:	// common physics interface

	virtual void			SetSelf( idEntity *e ) override;
#ifdef MOD_WATERPHYSICS
	virtual idEntity		*GetSelf() override { return self; } // MOD_WATERPHYSICS
#endif		// MOD_WATERPHYSICS

	virtual void			SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true ) override;
	virtual idClipModel *	GetClipModel( int id = 0 ) const override;
	virtual int				GetNumClipModels( void ) const override;

	virtual void			SetMass( float mass, int id = -1 ) override;
	virtual float			GetMass( int id = -1 ) const override;

	virtual void			SetContents( int contents, int id = -1 ) override;
	virtual int				GetContents( int id = -1 ) const override;

	virtual void			SetClipMask( int mask, int id = -1 ) override;
	virtual int				GetClipMask( int id = -1 ) const override;

	virtual const idBounds &GetBounds( int id = -1 ) const override;
	virtual const idBounds &GetAbsBounds( int id = -1 ) const override;

	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec ) override;
	virtual void			UpdateTime( int endTimeMSec ) override;
	virtual int				GetTime( void ) const override;

	virtual void			GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const override;
	virtual void			ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) override;
	virtual bool			PropagateImpulse( const int id, const idVec3& point, const idVec3& impulse ) override;

	virtual void			Activate( void ) override;
	virtual void			PutToRest( void ) override;
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

	virtual void			SetGravity( const idVec3 &newGravity ) override;
	virtual const idVec3 &	GetGravity( void ) const override;
	virtual const idVec3 &	GetGravityNormal( void ) const override;

	virtual void			ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const override;
	virtual void			ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const override;
	virtual int				ClipContents( const idClipModel *model ) const override;

	virtual void			DisableClip( void ) override;
	virtual void			EnableClip( void ) override;

	virtual void			UnlinkClip( void ) override;
	virtual void			LinkClip( void ) override;

	virtual bool			EvaluateContacts( void ) override;
	virtual int				GetNumContacts( void ) const override;
	virtual const contactInfo_t &GetContact( int num ) const override;
	virtual void			ClearContacts( void ) override;
	virtual void			AddContactEntity( idEntity *e ) override;
	virtual void			RemoveContactEntity( idEntity *e ) override;

	virtual bool			HasGroundContacts( void ) const override;
	virtual bool			IsGroundEntity( int entityNum ) const override;
	virtual bool			IsGroundClipModel( int entityNum, int id ) const override;

	virtual void			SetPushed( int deltaTime ) override;
	virtual const idVec3 &	GetPushedLinearVelocity( const int id = 0 ) const override;
	virtual const idVec3 &	GetPushedAngularVelocity( const int id = 0 ) const override;

	virtual void			SetMaster( idEntity *master, const bool orientated = true ) override;

	virtual const trace_t *	GetBlockingInfo( void ) const override;
	virtual idEntity *		GetBlockingEntity( void ) const override;

	virtual int				GetLinearEndTime( void ) const override;
	virtual int				GetAngularEndTime( void ) const override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

#ifdef MOD_WATERPHYSICS
	virtual idPhysics_Liquid *GetWater() override;												// MOD_WATERPHYSICS
	float					GetWaterMurkiness() const;								// TDM Tels
	virtual void			SetWater( idPhysics_Liquid *e, const float murkiness ) override;	// MOD_WATERPHYSICS
	float					SetWaterLevelf();										// MOD_WATERPHYSICS
	float					GetWaterLevelf() const;									// MOD_WATERPHYSICS
#endif	// MOD_WATERPHYSICS

protected:
	idEntity *				self;					// entity using this physics object
	int						clipMask;				// contents the physics object collides with
	idVec3					gravityVector;			// direction and magnitude of gravity
	idVec3					gravityNormal;			// normalized direction of gravity
	idList<contactInfo_t>	contacts;				// contacts with other physics objects
	idList<contactEntity_t>	contactEntities;		// entities touching this physics object

#ifdef MOD_WATERPHYSICS
// the water object the object is in, we use this to check density/viscosity
	idPhysics_Liquid		*water;					// MOD_WATERPHYSICS
// TDM Tels: The murkiness of this water, 0 => clear as crystal, 1 => opaque
	float					m_fWaterMurkiness;
#endif

protected:
							// add ground contacts for the clip model
	void					AddGroundContacts( const idClipModel *clipModel );
							// add contact entity links to contact entities
	void					AddContactEntitiesForContacts( void );
							// active all contact entities
	void					ActivateContactEntities( void );
							// returns true if the whole physics object is outside the world bounds
	bool					IsOutsideWorld( void ) const;
							// draw linear and angular velocity
	void					DrawVelocity( int id, float linearScale, float angularScale ) const;
};

#endif /* !__PHYSICS_BASE_H__ */

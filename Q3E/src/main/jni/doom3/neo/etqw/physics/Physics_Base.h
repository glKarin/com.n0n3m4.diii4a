// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_BASE_H__
#define __PHYSICS_BASE_H__

/*
===============================================================================

	Physics base for a moving object using one or more collision models.

===============================================================================
*/

#include "Physics.h"
#include "../../cm/CollisionModel.h"

typedef idEntityPtr< idEntity > contactEntity_t;
typedef idList< contactInfo_t > contactList_t;

struct trace_t;

class sdClipModelCollection {
public:
	sdClipModelCollection() {
		lastUpdate = -1;
	}

	void	Update( const idBounds& bounds, const idVec3& origin, const idMat3& axis, 
					const idVec3& linearVelocity, const idVec3& angularVelocity, 
					int clipMask, idEntity* passEntity );

	void	RemoveEntitiesOfCollection( const char* collection );

	void	ForceNextUpdate();

	void	SetSelf( idEntity* e );

	void	Clear() { collection.AssureSize( MAX_GENTITIES ); collection.SetNum( 0, false ); }
	void	AddClipModel( const idClipModel* model ) { collection.Append( model ); }
	void	RemoveClipModel( const idClipModel* model ) { collection.RemoveFast( model ); }
	bool	ContainsClipModel( const idClipModel* model ) const { return collection.FindIndex( model ) != -1; }
	idList< const idClipModel* >&	GetCollection() { return collection; }

	// tracing functions to trace against the collection
	bool	TracePoint( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask ) const;

	bool	Translation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idVec3 &end,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const;

	bool	Rotation( CLIP_DEBUG_PARMS_DECLARATION trace_t &results, const idVec3 &start, const idRotation &rotation,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const;

	int		Contacts( CLIP_DEBUG_PARMS_DECLARATION contactInfo_t *contacts, const int maxContacts, 
						const idVec3 &start, const idVec3 *dir, const float depth,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const;

	int		Contents( CLIP_DEBUG_PARMS_DECLARATION const idVec3 &start,
						const idClipModel *mdl, const idMat3 &trmAxis, int contentMask ) const;

protected:
	idList< const idClipModel* >	collection;
	int								lastUpdate;
	idEntity*						self;
};

class idPhysics_Base : public idPhysics {
public:
	CLASS_PROTOTYPE( idPhysics_Base );

							idPhysics_Base( void );
							~idPhysics_Base( void );

public:	// common physics interface

	void					SetSelf( idEntity *e );

	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	virtual const idMat3&	GetInertiaTensor( int id = -1 ) const	{ return mat3_zero; }
	virtual const idVec3&	GetCenterOfMass( void ) const			{ return vec3_origin; }

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	void					SetClipMask( int mask, int id = -1 );
	int						GetClipMask( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( const int id, const idVec3 &point, const idVec3 &force );
	void					AddForce( const idVec3& force );
	void					AddTorque( const idVec3& torque );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	virtual void			EnableImpact( void ) { }
	virtual void			DisableImpact( void ) { }

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SetGravity( const idVec3 &newGravity );
	const idVec3 &			GetGravity( void ) const;
	const idVec3 &			GetGravityNormal( void ) const;

	void					ClipTranslation( trace_t& results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t& results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );
	int						GetNumContacts( void ) const;
	const contactInfo_t&	GetContact( int num ) const;
	void					ClearContacts( void );
	void					AddContactEntity( idEntity *e );
	void					RemoveContactEntity( idEntity *e );

	bool					HasGroundContacts( void ) const;
	bool					IsGroundEntity( int entityNum ) const;
	bool					IsGroundClipModel( int entityNum, int id ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const idVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true );

	virtual const trace_t*	GetBlockingInfo( void ) const;
	virtual idEntity*		GetBlockingEntity( void ) const;

	virtual void			SetComeToRest( bool value ) { }

	virtual void			DrawDebugInfo( void ) { }

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	virtual float			InWater( void ) const { return 0.0f; }

	virtual bool			AllowInhibit( void ) const { return true; }

	const sdClipModelCollection&		GetTraceCollection( void );

protected:
	idEntity *				self;					// entity using this physics object
	int						clipMask;				// contents the physics object collides with
	idVec3					gravityVector;			// direction and magnitude of gravity
	idVec3					gravityNormal;			// normalized direction of gravity
	contactList_t			contacts;				// contacts with other physics objects
	idList<contactEntity_t>	contactEntities;		// entities touching this physics object

	sdClipModelCollection	traceCollection;		// collection of clip models that this object might hit this frame

protected:
							// add ground contacts for the clip model
	void					AddGroundContacts( CLIP_DEBUG_PARMS_DECLARATION const idClipModel *clipModel, unsigned int max = 4 );
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

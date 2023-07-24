// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLE_COMPONENTS_H__
#define __GAME_VEHICLE_COMPONENTS_H__

#include "../effects/Effects.h"
#include "../effects/WaterEffects.h"
#include "VehicleSuspension.h"
#include "Transport.h"


class sdVehicle;
class sdVehicleInput;
class sdDeclVehiclePart;
class sdTransport_AF;
class sdDeclAFPart;
class sdDeclRigidBodyPart;
class sdDeclVehiclePartSimple;
class sdDeclRigidBodyWheel;
class sdDeclRigidBodyHover;
class sdDeclRigidBodyRotor;
class sdTransport;
class sdTransport_RB;
class sdVehicle_RigidBody;
class sdVehicleSuspension;
class sdVehicleRigidBodyVtol;
class sdVehicleRigidBodyAntiGrav;
class sdPhysics_RigidBodyMultiple;

class sdVehicleDriveObject : public idClass {
public:
	CLASS_PROTOTYPE( sdVehicleDriveObject );

									sdVehicleDriveObject( void );
	virtual							~sdVehicleDriveObject( void );
	virtual int						GetBodyId( void ) = 0;
	virtual void					GetBounds( idBounds& bounds ) = 0;
	virtual void					UpdatePrePhysics( const sdVehicleInput& input ) = 0;
	virtual void					UpdatePostPhysics( const sdVehicleInput& input ) = 0;
	virtual void					GetWorldOrigin( idVec3& vec ) = 0;
	virtual void					GetWorldAxis( idMat3& axis ) = 0;
	virtual void					GetWorldPhysicsOrigin( idVec3& vec ) { GetWorldOrigin( vec ); }
	virtual void					GetWorldPhysicsAxis( idMat3& axis ) { GetWorldAxis( axis ); }
	virtual void					Detach( bool createDebris, bool decay ) = 0;
	virtual void					Reattach( void ) = 0;
	virtual void					Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) = 0;
	virtual void					Decay( void ) = 0;
	virtual int						Repair( int repair ) = 0;
	virtual bool					CanDamage( void ) = 0;
	virtual const partDamageInfo_t*	GetDamageInfo( void ) const = 0;
	virtual int						EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max ) { return 0; }
	virtual int						AddCustomConstraints( constraintInfo_t* list, int max ) { return 0; }
	virtual void					PostInit( void );
	virtual bool					UpdateSuspensionIK( void ) { return false; }
	virtual void					ClearSuspensionIK( void ) { ; }

	virtual bool					Mask( int mask ) const { return false; }
	virtual bool					DestroyFirst( void ) const { return false; }
	virtual void					CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) { ; }
	virtual const sdDeclSurfaceType* GetSurfaceType( void ) const { return NULL; }

	virtual sdTransport*			GetParent( void ) const = 0;

	virtual bool					HasPhysics( void ) const { return false; }

	bool							IsHidden( void ) const { return hidden; }
	void							Hide( void );
	void							Show( void );

	const char*						Name( void ) const { return name; }
	virtual idScriptObject*			GetScriptObject( void ) const { return scriptObject; }

	virtual void					SetThrust( float thrust ) {};


	virtual bool					IsNetworked() { return false; }
	virtual void					ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) { ; }
	virtual void					ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const { ; }
	virtual void					WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const { ; }
	virtual bool					CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const { return false; }
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const { return NULL; }


	virtual bool					GoesInPartList() { return true; }

protected:
	virtual void					CreateExplosionDebris( void ) = 0;
	virtual void					CreateDecayDebris( void	) = 0;

	bool							hidden;
	idScriptObject*					scriptObject;
	idStr							name;




public:
	// static stuff for scaling the amount of debris that can exist
	enum debrisPriority_t {
		PRIORITY_LOW = 0,
		PRIORITY_MEDIUM,
		PRIORITY_HIGH,
		PRIORITY_EXTRA_HIGH
	};

	static bool						CanAddDebris( debrisPriority_t priority, const idVec3& origin );
	static void						AddDebris( rvClientMoveable* debris, debrisPriority_t priority );

protected:
	static const int				MAX_DEBRIS	= 128;
	typedef idStaticList< rvClientEntityPtr< rvClientMoveable >, MAX_DEBRIS >	debrisList_t;

	static debrisList_t				extraHighDebris;
	static debrisList_t				highDebris;
	static debrisList_t				mediumDebris;
	static debrisList_t				lowDebris;
};

class sdVehiclePart : public sdVehicleDriveObject {
public:
	CLASS_PROTOTYPE( sdVehiclePart );

							sdVehiclePart( void );
	virtual					~sdVehiclePart( void );

	void					Init( const sdDeclVehiclePart& part );
	virtual void			OnKilled( void ) { }

	void					SetIndex( int _index ) { partIndex = _index; }
	int						GetIndex( void ) { return partIndex; }
	void					AddSurface( const char* surfaceName );

	void					HideSurfaces( void );
	void					ShowSurfaces( void );

	virtual int				GetBodyId( void ) { return bodyId; }
	virtual void			GetBounds( idBounds& bounds );
	virtual void			GetWorldOrigin( idVec3& vec );
	virtual void			GetWorldAxis( idMat3& axis );
	virtual void			UpdatePrePhysics( const sdVehicleInput& input ) { assert( false ); }
	virtual void			UpdatePostPhysics( const sdVehicleInput& input ) { }
	virtual void			Detach( bool createDebris, bool decay );
	virtual void			Reattach( void );
	virtual void			Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision );
	virtual void			Decay( void );
	virtual int				Repair( int repair );
	virtual	jointHandle_t	GetJoint( void ) const { return INVALID_JOINT; }
	virtual bool			CanDamage( void ) { return health > 0; }
	virtual const partDamageInfo_t*	GetDamageInfo( void ) const { return &damageInfo; }

	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );
	virtual idBounds		CalcSurfaceBounds( jointHandle_t joint );
	virtual bool			DestroyFirst( void ) const { return flipMaster; }

	// Callable by scripts
	void					Event_GetHealth( void );
	void					Event_GetOrigin( void );
	void					Event_GetAngles( void );
	void					Event_GetParent( void );
	void					Event_GetJoint( void );
protected:
	virtual void			CreateExplosionDebris( void );
	virtual void			CreateDecayDebris( void	);

	int						health;
	int						maxHealth;
	int						oldcontents;
	int						oldclipmask;
	int						bodyId;
	idList< int >			surfaces;
	int						partIndex;
	partDamageInfo_t		damageInfo;
	const idDeclEntityDef*	brokenPart;
	idBounds				partBounds;
	sdWaterEffects*			waterEffects;
	bool					noAutoHide;

	float					flipPower;
	bool					flipMaster;
	int						reattachTime;
};

class sdVehiclePartSimple : public sdVehiclePart {
public:
	CLASS_PROTOTYPE( sdVehiclePartSimple );

	virtual sdTransport*			GetParent( void ) const { return parent; }
	virtual jointHandle_t			GetJoint( void ) const { return joint; }
	void							Init( const sdDeclVehiclePart& part, sdTransport* _parent );
	virtual void					GetWorldAxis( idMat3& axis );
	virtual void					GetWorldOrigin( idVec3& vec );
	virtual void					GetWorldPhysicsOrigin( idVec3& vec );
	virtual void					GetWorldPhysicsAxis( idMat3& axis );
	bool							CanDamage( void ) { return health > 0; }
	bool							ShouldDisplayDebugInfo() const;

protected:
	jointHandle_t					joint;
	sdTransport*					parent;
};

class sdVehiclePartScripted : public sdVehiclePartSimple {
public:
	CLASS_PROTOTYPE( sdVehiclePartScripted );
	virtual void		Init( const sdDeclVehiclePart& part, sdTransport* _parent );
	virtual void		OnKilled( void );
	virtual void		Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision );
	virtual void		Decay( void ) { }

protected:
	const sdProgram::sdFunction *onKilled;
	const sdProgram::sdFunction *onPostDamage;
};

class sdVehicleRigidBodyPart : public sdVehiclePart {
public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyPart );

	sdTransport_RB*					GetRBParent( void ) { return parent; }
	virtual sdTransport*			GetParent( void ) const;
	virtual jointHandle_t			GetJoint( void ) const { return joint; }
	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					GetWorldOrigin( idVec3& vec );

	bool							ShouldDisplayDebugInfo() const;

protected:
	jointHandle_t					joint;
	sdTransport_RB*					parent;
};

class sdVehicleRigidBodyPartSimple : public sdVehiclePart {
public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyPartSimple );

	sdTransport_RB*					GetRBParent( void ) { return parent; }
	virtual sdTransport*			GetParent( void ) const;
	virtual jointHandle_t			GetJoint( void ) const { return joint; }
	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					GetWorldOrigin( idVec3& vec );
	virtual void					GetWorldPhysicsOrigin( idVec3& vec );
	bool							CanDamage( void ) { return health > 0; }

	bool							ShouldDisplayDebugInfo() const;

protected:
	jointHandle_t					joint;
	sdTransport_RB*					parent;
};

class sdVehicleRigidBodyWheel : public sdVehicleRigidBodyPartSimple {
private:
	class sdSuspension : public sdVehicleSuspensionInterface {
	public:
									sdSuspension( void ) { }
		void						Init( sdVehicleRigidBodyWheel* owner ) { _owner = owner; }

		virtual sdTransport*		GetParent( void ) const { return _owner->GetParent(); }
		virtual float				GetOffset( void ) const { return _owner->wheelOffset; }
		virtual jointHandle_t		GetJoint( void ) const { return _owner->GetWheelJoint(); }

		sdVehicleRigidBodyWheel*	_owner;
	};

public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyWheel );

									sdVehicleRigidBodyWheel( void );
	virtual							~sdVehicleRigidBodyWheel( void );

	void							Init( const sdDeclVehiclePart& wheel, sdTransport_RB* _parent );
	void							TrackWheelInit( const sdDeclVehiclePart& track, int index, sdTransport_RB* _parent );

	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );
	float							GetLinearSpeed( void );
	virtual int						EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max );
	void							CalcForces( float& maxForce, float& velocity );

	virtual bool					Mask( int mask ) const		{ return mask & VPT_WHEEL ? true : false; }

	virtual bool					HasPhysics( void ) const { return true; }

	bool							HasDrive( void )			{ return wheelFlags.hasDrive; }
	bool							HasSteering( void )			{ return wheelFlags.hasSteering; }
	bool							HasInverseSteering( void )	{ return wheelFlags.inverseSteering; }

	bool							SlowsOnLeft( void )			{ return wheelFlags.slowsOnLeft; }
	bool							SlowsOnRight( void )		{ return wheelFlags.slowsOnRight; }

	bool							IsLeftWheel( void )			{ return wheelFlags.isLeftWheel; }
	bool							IsRightWheel( void )		{ return !wheelFlags.isLeftWheel; }

	const trace_t&					GetGroundTrace( void ) const { return groundTrace; }

	float							GetInputSpeed( const sdVehicleInput& input );

	const idVec3&					GetRotationAxis( void ) const { return rotationAxis; }

	jointHandle_t					GetWheelJoint( void ) const	{ return joint; }
	float							GetWheelAngle( void ) const { return angle; }
	bool							HasVisualStateChanged( void ) const { return state.changed; }
	void							ResetVisualState( void ) { state.changed = false; }
	const idMat3&					GetFrictionAxes( void ) { return frictionAxes; }
	virtual bool					UpdateSuspensionIK( void );
	virtual void					ClearSuspensionIK( void );

	idVec3							GetBaseWorldOrg( void ) const;
	const idMat3&					GetBaseAxes( void ) const { return baseAxes; }
	void							UpdateRotation( float speed );
	bool							IsGrounded( void ) const { return state.grounded; }
	void							DisableSuspension( bool disable ) { state.suspensionDisabled = disable; }
	virtual void					CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );
	void							UpdateFriction( const sdVehicleInput& input );
	void							UpdateSkidding();

	virtual const sdDeclSurfaceType* GetSurfaceType( void ) const { return state.grounded ? groundTrace.c.surfaceType : NULL; }


	virtual bool					IsNetworked() { return false; }

	void							UpdateSuspension( const sdVehicleInput& input );
	void							UpdateMotor( const sdVehicleInput& input, float motorForce );

	virtual bool					GoesInPartList() { return !wheelFlags.partOfTrack; }

protected:
	void							CommonInit( const sdDeclVehiclePart& part );
	virtual void					CreateDecayDebris( void	);

	typedef struct wheelFlags_s {
		bool						hasDrive		: 1;
		bool						hasSteering		: 1;
		bool						inverseSteering : 1;
		bool						slowsOnLeft		: 1;
		bool						slowsOnRight	: 1;
		bool						isLeftWheel		: 1;
		bool						isFrontWheel	: 1;
		bool						noRotation		: 1;
		bool						noPhysics		: 1;
		bool						hasHandBrake	: 1;
		bool						partOfTrack		: 1;
	} wheelFlags_t;


	typedef struct wheelState_s {
		bool						moving				: 1;
		bool						steeringChanged		: 1;
		bool						changed				: 1;
		bool						grounded			: 1;
		bool						rested				: 1;
		bool						suspensionDisabled	: 1;
		bool						spinning			: 1;
		bool						setSteering			: 1;
		bool						skidding			: 1;
	} wheelState_t;

	wheelFlags_t					wheelFlags;
	float							steerScale;
	float							steerAngle;
	float							idealSteerAngle;
	float							angle;
	float							rotationspeed;
	float							radius;
	float							velocityScale;
	idVec3							currentFriction;
	idVec3							normalFriction;
	wheelState_t					state;
	idMat3							frictionAxes;
	float							motorForce;
	float							motorSpeed;
	idClipModel*					wheelModel;
	idVec3							rotationAxis;

	float							brakingForce;
	float							handBrakeSlipScale;
	float							maxSlip;


	idList< float >					traction;
	idLinkList< sdEffect >			activeEffects;

	float							wheelSpinForceThreshold;
	float							wheelSkidVelocityThreshold;

	sdSuspension					suspensionInterface;
	sdVehicleSuspension*			suspension;
	float							wheelOffset;

	typedef struct suspensionInfo_s {
		float						upTrace;
		float						downTrace;
		float						totalDist;
		float						kCompress;
		float						damping;
		float						velocityScale;
		float						maxRestVelocity;
		float						base;
		float						range;
		bool						aggressiveDampening;
		float						slowScale;
		float						slowScaleSpeed;
		float						hardStopScale;
		bool						alternateSuspensionModel;
	} suspensionInfo_t;

	suspensionInfo_t				suspensionInfo;

	idVec3							baseOrg;
	idVec3							baseOrgOffset;
	idMat3							baseAxes;
	trace_t							groundTrace;
	float							suspensionForce;
	float							suspensionVelocity;

	vehicleEffectList_t				dustEffects;
	vehicleEffectList_t				spinEffects;
	vehicleEffectList_t				skidEffects;
	int								numSurfaceTypesAtSpawn;

	unsigned int					treadId;
	bool							stroggTread;

	// Stuff to allow wheel traces to be throttled instead of all run every frame
	int								traceIndex;
	int								totalWheels;

	// 16 is sufficient to cover some unplayable pings - 500+
	const static int				MAX_WHEEL_MEMORY = 16;
	idStaticList< float, MAX_WHEEL_MEMORY >		wheelFractionMemory;
	int								currentMemoryFrame;
	int								currentMemoryIndex;

protected:
	void							UpdateRotation( const sdVehicleInput& input );
	void							UpdateParticles( const sdVehicleInput& input );
};

class sdVehicleTrack : public sdVehicleDriveObject {
public :
	CLASS_PROTOTYPE( sdVehicleTrack );

	virtual							~sdVehicleTrack( void );

	void							Init( const sdDeclVehiclePart& track, sdTransport_RB* _parent );

	virtual int						GetBodyId( void ) { return -1; }
	virtual int						GetSurfaceId( void ) { return -1; }
	virtual void					GetBounds( idBounds& bounds ) { }
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );
	virtual void					GetWorldOrigin( idVec3& vec ) { vec = vec3_origin; }
	virtual void					GetWorldAxis( idMat3& axis ) { axis = mat3_identity; }
	virtual void					Detach( bool createDebris, bool decay ) { }
	virtual void					Reattach( void ) { }
	virtual void					Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) { }
	virtual void					Decay( void ) { };
	virtual int						Repair( int repair ) { return 0; }
	virtual bool					CanDamage( void ) { return false; }
	virtual const partDamageInfo_t*	GetDamageInfo( void ) const { return NULL; }
	virtual void					PostInit( void );
	float							GetInputSpeed( const sdVehicleInput& input ) const;
	bool							IsLeftTrack( void ) const { return leftTrack; }

	virtual int						EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max );
	virtual void					CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );
	virtual const sdDeclSurfaceType* GetSurfaceType( void ) const;

	virtual sdTransport*			GetParent( void ) const;

	virtual bool					HasPhysics( void ) const { return true; }

	virtual bool					UpdateSuspensionIK( void );
	virtual void					ClearSuspensionIK( void );

	bool							IsGrounded( void ) const;

protected:
	const static int	TRACK_MAX_WHEELS = 8;

	virtual void					CreateExplosionDebris( void ) { }
	virtual void					CreateDecayDebris( void	) { }

	sdTransport_RB*								parent;
	jointHandle_t								joint;
	int											shaderParmIndex;
	idVec3										direction;
	idStaticList< sdVehicleRigidBodyWheel*, TRACK_MAX_WHEELS >	wheels;
	bool										leftTrack;

	const sdDeclVehiclePart*			spawnPart;
};

class sdVehicleThruster : public sdVehicleDriveObject {
public :
	CLASS_PROTOTYPE( sdVehicleThruster );

	void								Init( const sdDeclVehiclePart& thruster, sdTransport_RB* _parent );

	virtual bool						HasPhysics( void ) const { return true; }

	virtual int							GetBodyId( void ) { return -1; }
	virtual int							GetSurfaceId( void ) { return -1; }
	virtual void						GetBounds( idBounds& bounds ) { }
	virtual void						UpdatePrePhysics( const sdVehicleInput& input );
	virtual void						UpdatePostPhysics( const sdVehicleInput& input ) { }
	virtual void						GetWorldOrigin( idVec3& vec ) { vec = vec3_origin; }
	virtual void						GetWorldAxis( idMat3& axis ) { axis = mat3_identity; }
	virtual void						Detach( bool createDebris, bool decay ) { }
	virtual void						Reattach( void ) { }
	virtual void						Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) { }
	virtual void						Decay( void ) { };
	virtual int							Repair( int repair ) { return 0; }
	virtual bool						CanDamage( void ) { return false; }
	virtual const partDamageInfo_t*		GetDamageInfo( void ) const { return NULL; }
	virtual void						CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

	virtual sdTransport*				GetParent( void ) const;

	void								CalcPos( idVec3& pos );

	bool								IsInWater( void ) const { return inWater; }
	float								GetThrust( void ) const { return thrustScale; }
	
	virtual void						SetThrust( float thrust );
	void								Event_SetThrust( float thrust );

protected:
	virtual void						CreateExplosionDebris( void ) { }
	virtual void						CreateDecayDebris( void	) { }

	sdTransport_RB*						parent;
	idVec3								direction;
	idVec3								fixedDirection;
	idVec3								origin;
	bool								needWater;
	bool								inWater;
	float								thrustScale;
	float								reverseScale;
};

class sdVehicleAirBrake : public sdVehicleDriveObject {
public :
	CLASS_PROTOTYPE( sdVehicleAirBrake );

	void								Init( const sdDeclVehiclePart& airBrake, sdTransport_RB* _parent );

	virtual bool						HasPhysics( void ) const { return true; }

	virtual int							GetBodyId( void ) { return -1; }
	virtual int							GetSurfaceId( void ) { return -1; }
	virtual void						GetBounds( idBounds& bounds ) { }
	virtual void						UpdatePrePhysics( const sdVehicleInput& input );
	virtual void						UpdatePostPhysics( const sdVehicleInput& input ) { }
	virtual void						GetWorldOrigin( idVec3& vec ) { vec = vec3_origin; }
	virtual void						GetWorldAxis( idMat3& axis ) { axis = mat3_identity; }
	virtual void						Detach( bool createDebris, bool decay ) { }
	virtual void						Reattach( void ) { }
	virtual void						Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) { }
	virtual void						Decay( void ) { }
	virtual int							Repair( int repair ) { return 0; }
	virtual bool						CanDamage( void ) { return false; }
	virtual const partDamageInfo_t*		GetDamageInfo( void ) const { return NULL; }

	virtual sdTransport*				GetParent( void ) const { return parent; }

	void								Enable( void ) { enabled = true; }
	void								Disable( void ) { enabled = false; }

protected:
	virtual void						CreateExplosionDebris( void ) { }
	virtual void						CreateDecayDebris( void	) { }

	sdTransport*						parent;
	bool								enabled;
	float								factor;
	float								maxSpeed;
};

class sdVehicleSuspensionPoint : public sdVehicleDriveObject {
	class sdSuspension : public sdVehicleSuspensionInterface {
	public:
									sdSuspension( void ) { }
		void						Init( sdVehicleSuspensionPoint* owner ) { _owner = owner; }

		virtual sdTransport*		GetParent( void ) const { return _owner->GetParent(); }
		virtual float				GetOffset( void ) const { return _owner->offset; }
		virtual jointHandle_t		GetJoint( void ) const { return _owner->joint; }

		sdVehicleSuspensionPoint*	_owner;
	};

public :
	CLASS_PROTOTYPE( sdVehicleSuspensionPoint );

										sdVehicleSuspensionPoint( void );
										~sdVehicleSuspensionPoint( void );

	void								Init( const sdDeclVehiclePart& point, sdTransport_RB* _parent );

	virtual bool						HasPhysics( void ) const { return true; }

	virtual int							GetBodyId( void ) { return -1; }
	virtual int							GetSurfaceId( void ) { return -1; }
	virtual void						GetBounds( idBounds& bounds ) { }
	virtual void						UpdatePrePhysics( const sdVehicleInput& input );
	virtual void						UpdatePostPhysics( const sdVehicleInput& input ) { }
	virtual void						GetWorldOrigin( idVec3& vec ) { vec = vec3_origin; }
	virtual void						GetWorldAxis( idMat3& axis ) { axis = mat3_identity; }
	virtual void						Detach( bool createDebris, bool decay ) { }
	virtual void						Reattach( void ) { }
	virtual void						Damage( int damage, idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const trace_t* collision ) { }
	virtual void						Decay( void ) { }
	virtual int							Repair( int repair ) { return 0; }
	virtual bool						CanDamage( void ) { return false; }
	virtual const partDamageInfo_t*		GetDamageInfo( void ) const { return NULL; }
	virtual int							EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max );

	virtual sdTransport*				GetParent( void ) const;
	void								CalcForces( float& maxForce, float& velocity, const idVec3& traceDir );

	virtual bool						UpdateSuspensionIK( void );
	virtual void						ClearSuspensionIK( void );

protected:
	virtual void						CreateExplosionDebris( void ) { }
	virtual void						CreateDecayDebris( void	) { }

	typedef struct suspensionState_s {
		bool							grounded	: 1;
		bool							rested		: 1;
	} suspensionState_t;

	typedef struct suspensionInfo_s {
		float							totalDist;
		float							kCompress;
		float							damping;
		float							velocityScale;
	} suspensionInfo_t;

	suspensionInfo_t					suspensionInfo;
	sdTransport_RB*						parent;
	float								offset;
	float								radius;
	jointHandle_t						joint;
	jointHandle_t						startJoint;
	idVec3								baseOrg;
	idVec3								baseStartOrg;

	trace_t								groundTrace;
	float								suspensionForce;
	float								suspensionVelocity;

	suspensionState_t					state;

	sdSuspension						suspensionInterface;
	sdVehicleSuspension*				suspension;

	idVec3								contactFriction;
	idMat3								frictionAxes;
	bool								aggressiveDampening;
};

// FIXME: Split main and tail rotor into two seperate classes
class sdVehicleRigidBodyRotor : public sdVehicleRigidBodyPartSimple {
public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyRotor );

									sdVehicleRigidBodyRotor( void );
	virtual							~sdVehicleRigidBodyRotor( void );

	void							Init( const sdDeclVehiclePart& rotor, sdTransport_RB* _parent );

	virtual bool					HasPhysics( void ) const { return true; }

	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );

	void							UpdatePrePhysics_Main( const sdVehicleInput& input );
	void							UpdatePrePhysics_Tail( const sdVehicleInput& input );

	void							UpdatePostPhysics_Tail( const sdVehicleInput& input );
	void							UpdatePostPhysics_Main( const sdVehicleInput& input );

	float							GetSpeed( void ) const { return speed; }
	float							GetTopGoalSpeed( void ) const;
	void							ResetCollective( void );

	float							GetCollective( void ) const { return advanced.collective; }
	void							SetCollective( float value ) { advanced.collective = value; }

protected:
	typedef struct advancedControls_s {
		float						cyclicBank;
		float						cyclicPitch;
		float						cyclicPitchRate;
		float						cyclicBankRate;

		float						collective;
	} advancedControls_t;

	typedef struct rotorJoint_s {
		jointHandle_t				joint;
		idMat3						jointAxes;
		float						speedScale;
		float						angle;
		bool						isYaw;
	} rotorJoint_t;

	advancedControls_t				advanced;

	float							liftCoefficient;

	idList< rotorJoint_t >			animJoints;

	rotorType_t						type;
	float							maxPitchDeflect;
	float							maxYawDeflect;

	float							sideScale;

	float							speed;

	float							oldPitch;
	float							oldYaw;

	int								lastYawChange;
	int								lastPitchChange;

	float							zOffset;
};

class sdVehicleRigidBodyHoverPad : public sdVehicleRigidBodyPartSimple {
public:
									CLASS_PROTOTYPE( sdVehicleRigidBodyHoverPad );

									sdVehicleRigidBodyHoverPad( void );
	virtual							~sdVehicleRigidBodyHoverPad( void );

	virtual bool					HasPhysics( void ) const { return false; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	
	virtual int						EvaluateContacts( contactInfo_t* list, contactInfoExt_t* listExt, int max );

protected:
	float							maxTraceLength;
	idVec3							traceDir;

	// Initial positions of the joint
	idVec3							baseOrg;
	idMat3							baseAxes;

	// Rot limits so they don't stick into the body
	idAngles						minAngles;
	idAngles						maxAngles;

	// Movement info
	idQuat							currentAxes;
	float							adaptSpeed;
	idVec3							lastVelocity;

	// Lightning related
	sdEffect						engineEffect;
	int								nextBeamTime;
	const sdDeclTargetInfo			*beamTargetInfo;
	int								shaderParmIndex;
};

#define PSEUDO_HOVER_MAX_CASTS		8

class sdPseudoHoverBroadcastData : public sdEntityStateNetworkData {
public:
							sdPseudoHoverBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	bool					parkMode;
	bool					foundPark;
	bool					lockedPark;
	int						startParkTime;
	int						endParkTime;
	int						lastParkUpdateTime;
	idVec3					chosenParkOrigin;
	idMat3					chosenParkAxis;
};

class sdPseudoHoverNetworkData : public sdEntityStateNetworkData {
public:
							sdPseudoHoverNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	float					lastFrictionScale;
};

class sdVehicleRigidBodyPseudoHover : public sdVehiclePart {
public:
									CLASS_PROTOTYPE( sdVehicleRigidBodyPseudoHover );

									sdVehicleRigidBodyPseudoHover( void );
	virtual							~sdVehicleRigidBodyPseudoHover( void );

	virtual bool					HasPhysics( void ) const { return true; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	
	virtual int						AddCustomConstraints( constraintInfo_t* list, int max );

	virtual sdTransport*			GetParent( void ) const { return parent; }

	virtual bool					IsNetworked() { return true; }
	virtual void					ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void					ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void					WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool					CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

protected:

	// hoverEvalState_t is just used to pass around to the various evaluation functions
	// note that it DOES NOT store anything across frames.
	typedef struct {
		sdClipModelCollection	clipLocale;

		float		timeStep;

		// position
		idVec3		origin;
		idVec3		linVelocity;

		// physical properties
		float		mass;
		idVec3		gravity;
		idMat3		inertiaTensor;

		// orientation
		idMat3		axis;
		idVec3		angVelocity;

		// game properties
		sdPhysics_RigidBodyMultiple* physics;
		idPlayer*	driver;

		// evaluation
		idVec3		surfaceNormal;
		idQuat		surfaceQuat;
		idMat3		surfaceAxis;
		idVec3		hoverForce;
		idVec3		drivingForce;
		idVec3		frictionForce;

		idQuat		surfaceMatchingQuat;
		idVec3		steeringAngVel;
	} hoverEvalState_t;

	void							DoRepulsors();
	void							DoRepulsionForCast( const idVec3& start, const idVec3& end, float desiredFraction, const trace_t& trace, float& height, int numForces );
	void							CalculateSurfaceAxis();
	void							CalculateDrivingForce( const sdVehicleInput& input );
	void							CalculateFrictionForce( const sdVehicleInput& input );
	void							CalculateTilting( const sdVehicleInput& input );
	void							CalculateYaw( const sdVehicleInput& input );

	void							ChooseParkPosition();
	void							DoParkRepulsors();


	// state
	bool							grounded;
	sdTransport*					parent;
	idPlayer*						oldDriver;

	trace_t							groundTrace;

	float							lastFrictionScale;		// sync this!

	hoverEvalState_t				evalState;

	idVec3							targetVelocity;
	idQuat							targetQuat;

	//
	// "park" mode
	//

	bool							parkMode;				// sync this!
	int								startParkTime;			// sync this!
	int								endParkTime;			// sync this!
	int								lastParkUpdateTime;		// sync this!
	idVec3							chosenParkOrigin;		// sync this!
	idMat3							chosenParkAxis;			// sync this!
	bool							foundPark;				// sync this!
	bool							lockedPark;				// sync this!

	int								lastParkEffectTime;
	int								lastUnparkEffectTime;

	// Physics tuning parameters
	float							hoverHeight;
	float							parkHeight;

	float							repulsionSpeedCoeff;
	float							repulsionSpeedHeight;
	float							repulsionSpeedMax;

	float							yawCoeff;

	float							fwdCoeff;
	float							fwdSpeedDampCoeff;
	float							fwdSpeedDampPower;
	float							fwdSpeedDampMax;

	float							frontCastPos;
	float							backCastPos;

	float							castOffset;
	float							maxSlope;
	float							slopeDropoff;

	float							parkTime;

	idBounds						mainBounds;
	idClipModel*					mainClipModel;

	jointHandle_t					effectJoint;

	// casts
	idStaticList< idVec3, PSEUDO_HOVER_MAX_CASTS >		castStarts;
	idStaticList< idVec3, PSEUDO_HOVER_MAX_CASTS >		castDirections;
};

class sdVehicleRigidBodyVtol : public sdVehicleRigidBodyPartSimple {
public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyVtol );

	sdVehicleRigidBodyVtol( void );
	virtual							~sdVehicleRigidBodyVtol( void );

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	

protected:

	// Initial positions of the joints
	int								shoulderAxis;
	int								elbowAxis;
	jointHandle_t					elbowJoint;
	jointHandle_t					effectJoint;
	idMat3							shoulderBaseAxes;
	idMat3							elbowBaseAxes;

	float							oldShoulderAngle;
	float							oldElbowAngle;

	// Parameters
	float							shoulderAngleScale, elbowAngleScale;
	idVec2							shoulderAnglesBounds;
	idVec2							elbowAnglesBounds;
	sdEffect						engineEffect;

	// Radom movements of the hover
	float							noisePhase;
	float							noiseFreq;
	float							noiseAmplitude;
};

class sdVehicleRigidBodyAntiGrav : public sdVehiclePartSimple {
public:
	CLASS_PROTOTYPE( sdVehicleRigidBodyAntiGrav );

	sdVehicleRigidBodyAntiGrav( void );
	virtual							~sdVehicleRigidBodyAntiGrav( void );

	void							Init( const sdDeclVehiclePart& part, sdTransport* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	

	void							UpdateEffect();	

	void							SetClientParent( rvClientEntity* p );

protected:
	void							SetupJoints( idAnimator* targetAnimator );

	// Initial positions of the joints
	idMat3							baseAxes;

	// Parameters
	idVec2							mountAnglesBounds;
	sdEffect						engineMainEffect;
	sdEffect						engineBoostEffect;

	idVec3							oldVelocity;
	float							oldAngle;

	// Axes and stuff
	int								rotAxis;	// The engines will rotate around this axis
	int								tailUpAxis;
	int								tailSideAxis;

	float							fanRotation;
	float							targetAngle;

	jointHandle_t					fanJoint;
	jointHandle_t					tailJoint;
	idStr							fanJointName;
	idStr							tailJointName;

	rvClientEntity*					clientParent;

	int								lastGroundEffectsTime;

	// fan speed stuff
	float							fanSpeedMultiplier;
	float							fanSpeedOffset;
	float							fanSpeedMax;
	float							fanSpeedRampRate;
	float							lastFanSpeed;
};

class sdVehicleRigidBodyDragPlane : public sdVehiclePart {
public:
									CLASS_PROTOTYPE( sdVehicleRigidBodyDragPlane );

									sdVehicleRigidBodyDragPlane( void );
	virtual							~sdVehicleRigidBodyDragPlane( void );

	virtual bool					HasPhysics( void ) const { return true; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	

	virtual sdTransport*			GetParent( void ) const { return parent; }

protected:
	sdTransport*					parent;

	idVec3							normal;
	idVec3							origin;		// point of application of the drag force
	float							coefficient;
	float							maxForce;
	float							minForce;
	bool							doubleSided;
	bool							useAngleScale;
};

class sdVehicleRigidBodyRudder : public sdVehicleRigidBodyDragPlane {
public:
									CLASS_PROTOTYPE( sdVehicleRigidBodyRudder );

	virtual bool					HasPhysics( void ) const { return true; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );

protected:
};

class sdVehicleRigidBodyHurtZone : public sdVehicleRigidBodyPart {
public:
									CLASS_PROTOTYPE( sdVehicleRigidBodyHurtZone );

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
};

class sdVehicleAntiRoll : public sdVehiclePart {
public:
									CLASS_PROTOTYPE( sdVehicleAntiRoll );

									sdVehicleAntiRoll( void );
	virtual							~sdVehicleAntiRoll( void );

	virtual bool					HasPhysics( void ) const { return true; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	
	virtual int						AddCustomConstraints( constraintInfo_t* list, int max );

	virtual sdTransport*			GetParent( void ) const { return parent; }

protected:
	sdTransport_RB*					parent;

	float							currentStrength;
	bool							active;

	float							startAngle;
	float							endAngle;
	float							strength;
	bool							needsGround;
};

class sdVehicleAntiPitch : public sdVehiclePart {
public:
									CLASS_PROTOTYPE( sdVehicleAntiPitch );

									sdVehicleAntiPitch( void );
	virtual							~sdVehicleAntiPitch( void );

	virtual bool					HasPhysics( void ) const { return true; }

	void							Init( const sdDeclVehiclePart& part, sdTransport_RB* _parent );
	virtual void					UpdatePrePhysics( const sdVehicleInput& input );
	virtual void					UpdatePostPhysics( const sdVehicleInput& input );	
	virtual int						AddCustomConstraints( constraintInfo_t* list, int max );

	virtual sdTransport*			GetParent( void ) const { return parent; }

protected:
	sdTransport_RB*					parent;

	float							currentStrength;
	bool							active;

	float							startAngle;
	float							endAngle;
	float							strength;
	bool							needsGround;
};

#endif // __GAME_VEHICLE_COMPONENTS_H__

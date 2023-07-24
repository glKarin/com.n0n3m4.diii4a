// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientEntity.h
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_ENTITY_H__
#define __GAME_CLIENT_ENTITY_H__

#include "../Game.h"
#include "../Entity.h"
#include "../anim/Anim.h"
#include "../script/Script_Interface.h"
#include "../physics/Physics_Parabola.h"

class idSoundShader;
class rvClientPhysics;
class sdDeclAOR;

struct trace_t;

//============================================================================

template< class type >
class rvClientEntityPtr {
public:
							rvClientEntityPtr();

	rvClientEntityPtr<type> &	operator=( type* cent );

	void					SetEntity		( const type* cent );

	int						GetSpawnId		( void ) const { return spawnId; }
	bool					SetSpawnId		( int id );
	void					ForceSpawnId	( int id ) { spawnId = id; }
	bool					UpdateSpawnId	( void );

	bool					IsValid			( void ) const;
	type *					GetEntity		( void ) const;
	int						GetEntityNum	( void ) const;

	type *					operator->		( void ) const { return GetEntity ( ); }			
	operator				type*			( void ) const { return GetEntity ( ); }

private:
	int						spawnId;
};


class rvClientEntity : public idClass {
public:

	ABSTRACT_PROTOTYPE( rvClientEntity );

								rvClientEntity ( void );
	virtual						~rvClientEntity ( void );

	void						Dispose( void );
	virtual void				CleanUp( void );

	virtual idLinkList< idClass >* GetInstanceNode( void ) { return &instanceNode; }

	virtual void				Present				( void );
	virtual void				Think				( void );
	virtual class idPhysics*	GetPhysics			( void ) const;
	virtual bool				Collide				( const trace_t& collision, const idVec3& velocity );

	virtual bool				UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime ) { return false; }
	virtual void				ClientUpdateView( void );

	void						SetOrigin			( const idVec3& origin );
	void						SetAxis				( const idMat3& axis );
	const idVec3&				GetOrigin			( void );
	const idMat3&				GetAxis				( void );
	
	rvClientEffect*				PlayEffect( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop = false, const idVec3& endOrigin = vec3_origin );
	virtual rvClientEffect*		PlayEffect( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t joint, bool loop = false, const idVec3& endOrigin = vec3_origin );

	void						Bind				( idEntity* master, jointHandle_t joint = INVALID_JOINT );
	void						Bind				( rvClientEntity* master, jointHandle_t joint = INVALID_JOINT );
	void						Unbind				( void );
	bool						IsBound				( void );
	void						EnableAxisBind		( bool enable ) { axisBind = enable; }

	virtual void				DrawDebugInfo		( void ) const;

	int							StartSoundShader	( const idSoundShader *shader, const soundChannel_t channel, int soundShaderFlags );

	static rvClientPhysics*		GetClientMaster		( void );
	virtual idEntity*			GetOwner			( int index = 0 ) { return NULL; }
	virtual int					GetNumOwners		() { return 0; }
	virtual idScriptObject*		GetScriptObject		( void ) { return NULL; }
	virtual void				OnSetState			( const char* name ) { ; }

	virtual idAnimator*			GetAnimator( void ) { return NULL; }
	virtual renderEntity_t*		GetRenderEntity	( void ) { return NULL; }
	virtual int					GetRenderEntityHandle( void ) { return -1; }

	virtual bool				CanCollide			( const idEntity* other, int traceId ) const { return true; }

	virtual bool				IsHidden			( void ) const { return false; }

	void						MakeCurrent			( void );
	void						ResetCurrent		( void );

	virtual const char*			GetName( void ) const { return "Client Entity"; }

	virtual const idDict*		GetSpawnArgs( void ) const { return NULL; }

protected:

	void						RunPhysics			( void );

	virtual void				UpdateBind			( bool skipModelUpdate );
	void						UpdateSound			( void );

	void						RemoveBinds( void );
	void						RemoveClientEntities ( void );		// deletes any client entities bound to this object

public:
	
	int								entityNumber;

	idLinkList<rvClientEntity>		spawnNode;
	idLinkList<rvClientEntity>		bindNode;

	idLinkList< rvClientEntity >	clientEntities;

	idLinkList< idClass >			instanceNode;

protected:

	idVec3						worldOrigin;
	idMat3						worldAxis;

	idEntityPtr< idEntity >				bindMaster;
	rvClientEntityPtr< rvClientEntity >	bindMasterClient;
	idVec3								bindOrigin;
	idMat3								bindAxis;
	jointHandle_t						bindJoint;
	
	refSound_t					refSound;

	bool						axisBind;
};

ID_INLINE const idVec3& rvClientEntity::GetOrigin( void ) {
	return worldOrigin;
}

ID_INLINE const idMat3& rvClientEntity::GetAxis( void ) {
	return worldAxis;
}

/*
===============================================================================

	rvClientPhysics

===============================================================================
*/

struct trace_t;

class rvClientPhysics : public idEntity {
public:

	CLASS_PROTOTYPE( rvClientPhysics );

							~rvClientPhysics( void );

	void					Spawn();

	virtual bool			Collide( const trace_t& collision, const idVec3 &velocity, int bodyId );
	virtual bool			CanCollide( const idEntity* other, int traceId ) const;

	void					PushCurrentClientEntity( int entityNumber ) { activeEntities.Insert( entityNumber ); }
	void					PopCurrentClientEntity( void ) { activeEntities.RemoveIndex( 0 ); }

	rvClientEntity*			GetCurrentClientEntity( void ) const;

private:
	idList< int >			activeEntities;
};

/*
===============================================================================

	sdClientScriptEntity

===============================================================================
*/

struct trace_t;

class sdClientScriptEntity : public rvClientEntity {
public:

	CLASS_PROTOTYPE( sdClientScriptEntity );

							sdClientScriptEntity( void );
							~sdClientScriptEntity( void );

	void					CreateByName( const idDict* _spawnArgs, const char* scriptObjectName );
	virtual void			Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type );

	virtual void			CleanUp( void );

	void					UpdateScript( void );
	bool					SetState( const sdProgram::sdFunction* newState );
	sdProgramThread*		ConstructScriptObject( void );
	void					DeconstructScriptObject( void );
	void					CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper );

	virtual idScriptObject* GetScriptObject( void ) const { return scriptObject; }

	virtual void			Think( void );

	void					OnSetState( const char* name );

	virtual const idDict*	GetSpawnArgs( void ) const { return &spawnArgs; }

	void					Event_GetKey( const char* key );
	void					Event_GetIntKey( const char* key );
	void					Event_GetFloatKey( const char* key );
	void					Event_GetVectorKey( const char* key );
	void					Event_GetEntityKey( const char* key );
	void					Event_GetWorldOrigin( void );
	void					Event_GetDamagePower( void );
	void					Event_GetWorldAxis( int index );
	void					Event_GetOwner( void );
	void					Event_IsOwner( idEntity* other );
	void					Event_SetState( const char* name );
	void					Event_SetOrigin( const idVec3& org );
	void					Event_SetAngles( const idAngles& ang );
	void					Event_SetWorldAxis( const idVec3& fwd, const idVec3& right, const idVec3& up );
	void					Event_PlayMaterialEffect( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop );
	void					Event_PlayEffect( const char* effectName, const char* boneName, bool loop );
	void					Event_StopEffect( const char* effectName );
	void					Event_StopEffectHandle( int handle );
	void					Event_KillEffect( const char *effectName );
	void					Event_Bind( idEntity *bindMaster );
	void					Event_BindToJoint( idEntity *bindMaster, const char *boneName, float rotateWithMaster ); 
	void					Event_UnBind( void );
	void					Event_AddCheapDecal( idEntity *attachTo, idVec3 &origin, idVec3 &normal, const char* decalName, const char* materialName );
	void					Event_GetJointHandle( const char* jointName );
	void					Event_Dispose( void );
	void					Event_EnableAxisBind( bool enabled );
	void					Event_SetGravity( const idVec3& gravity );

protected:
	// state variables
	const sdProgram::sdFunction*	scriptState;
	const sdProgram::sdFunction*	scriptIdealState;
	sdProgramThread*				baseScriptThread;
	idScriptObject*					scriptObject;
	idDict							spawnArgs;
};

class sdClientAnimated : public sdClientScriptEntity {
public:
	CLASS_PROTOTYPE( sdClientAnimated );

							sdClientAnimated( void );
							~sdClientAnimated( void );

	virtual void			CleanUp( void );

	virtual void			Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type );

	void					SetStaticModel( const char* modelName );
	void					SetModel( const char* modelName );
	void					FreeModelDef( void );
	void					UpdateModel( void );
	virtual void			Present( void );
	void					Hide( void );
	void					Show( void );
	virtual idAnimator*		GetAnimator( void ) { return &animator; }
	virtual renderEntity_t*	GetRenderEntity( void ) { return &renderEntity; }
	virtual int				GetRenderEntityHandle( void ) { return renderEntityHandle; }
	virtual void			Think( void );
	void					SetSkin( const idDeclSkin* skin );

	void					UpdateAnimation( void );

	virtual bool			IsHidden( void ) const { return animatedFlags.hidden; }

	virtual void			BecomeActive( int flags, bool force );
	virtual void			BecomeInactive( int flags, bool force );

	virtual bool			UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime );
	virtual rvClientEffect* PlayEffect( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t jointHandle, bool loop, const idVec3& endOrigin );

	static bool				ModelCallback( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime );

	void					Event_SetAnimFrame( const char* anim, animChannel_t channel, float frame );
	void					Event_GetNumFrames( const char* animName );
	void					Event_IsHidden( void );
	void					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void					Event_SetJointAngle( jointHandle_t joint, jointModTransform_t transformType, const idAngles& angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void					Event_GetShaderParm( int parm );
	void					Event_SetShaderParm( int parm, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void 					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor();
	void					Event_PlayAnim( animChannel_t channel, const char *animname );
	void					Event_PlayCycle( animChannel_t channel, const char *animname );
	void					Event_PlayAnimBlended( animChannel_t channel, const char *animname, float blendTime );
	void					Event_GetAnimatingOnChannel( animChannel_t channel );
	void					Event_SetSkin( const char* skinName );
	void					Event_SetModel( const char* modelName );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );

	virtual const char*		GetName( void ) const;
protected:
	idAnimator				animator;
	renderEntity_t			renderEntity;
	qhandle_t				renderEntityHandle;
	
	struct animatedFlags_t {
		bool				hidden;
	};

	animatedFlags_t			animatedFlags;
	int						lastServiceTime;

	int						thinkFlags;
};

class sdClientProjectile : public sdClientAnimated {
public:

	CLASS_PROTOTYPE( sdClientProjectile );

							sdClientProjectile( void );

	void					AddOwner( idEntity* owner ) { _owners.Alloc() = owner; }
	void					Launch( idEntity* owner, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis );
	virtual idEntity*		GetOwner( int index ) { return _owners[ index ]; }
	virtual int				GetNumOwners() { return _owners.Num(); }
	virtual bool			CanCollide( const idEntity* other, int traceId ) const { return _owners.FindIndex( idEntityPtr< idEntity >( other )) == -1; }

private:
	idList< idEntityPtr< idEntity > > _owners;
};

class sdClientProjectile_Parabolic : public sdClientProjectile {
public:

	CLASS_PROTOTYPE( sdClientProjectile_Parabolic );

	virtual void			Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type );

	void					InitPhysics( void );

	virtual void			Think( void );

	virtual class idPhysics*	GetPhysics( void ) const { return ( idPhysics* )&physicsObj; }

	void					Event_Launch( const idVec3& velocity );
	void					Event_AddOwner( idEntity* other );
	void					Event_SetOwner( idEntity* other );
	void					Event_SetGameTeam( idScriptObject* object );

	virtual bool			Collide( const trace_t& collision, const idVec3 &velocity );

private:
	sdPhysics_Parabola		physicsObj;

	renderEntity_t			renderEntity;
	qhandle_t				renderEntityHandle;

};

class sdClientLight : public sdClientScriptEntity {
public:

	CLASS_PROTOTYPE( sdClientLight );

						sdClientLight( void );
						~sdClientLight( void );
	virtual void		Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type );
	virtual void		Present( void );

	void				Event_On( void );
	void				Event_Off( void );

private:
	bool				on;
	renderLight_t		renderLight;
	qhandle_t			lightDefHandle;
};

#endif // __GAME_CLIENT_ENTITY_H__

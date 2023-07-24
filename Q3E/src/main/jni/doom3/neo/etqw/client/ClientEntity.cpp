// Copyright (C) 2007 Id Software, Inc.
//

//----------------------------------------------------------------
// ClientEntity.cpp
//----------------------------------------------------------------

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ClientEntity.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "ClientEffect.h"
#include "../../decllib/declTypeHolder.h"
#include "../guis/UserInterfaceLocal.h"
#include "../guis/GuiSurface.h"
#include "../ContentMask.h"

ABSTRACT_DECLARATION( idClass, rvClientEntity )
END_CLASS

/*
================
rvClientEntity::rvClientEntity
================
*/
rvClientEntity::rvClientEntity( void ) {
	entityNumber = -1;

	worldOrigin.Zero();
	worldAxis.Identity();

	spawnNode.SetOwner( this );
	bindNode.SetOwner( this );
	instanceNode.SetOwner( this );

	memset ( &refSound, 0, sizeof(refSound) );

	gameLocal.RegisterClientEntity ( this );

	axisBind = true;
}

/*
================
rvClientEntity::~rvClientEntity
================
*/
rvClientEntity::~rvClientEntity( void ) {
	CleanUp();

	gameLocal.UnregisterClientEntity( this );
}

/*
================
rvClientEntity::CleanUp
================
*/
void rvClientEntity::CleanUp( void ) {
	Unbind();

	RemoveClientEntities();

	// Free sound emitter
	if ( refSound.referenceSound != NULL ) {
		refSound.referenceSound->Free( false );
		refSound.referenceSound = NULL;
	}
}

/*
=====================
rvClientEntity::Dispose
=====================
*/
void rvClientEntity::Dispose( void ) {
	CleanUp();
	PostEventMS( &EV_Remove, 0 );
}

/*
================
rvClientEntity::Present
================
*/
void rvClientEntity::Present( void ) {
}

/*
================
rvClientEntity::Think
================
*/
void rvClientEntity::Think( void ) {
	if ( bindMaster.IsValid() && bindMaster->IsInterpolated() ) {
		UpdateBind( true );
	} else {
		UpdateBind( false );
	}

	UpdateSound();
	Present();
}

/*
================
rvClientEntity::ClientUpdateView
================
*/
void rvClientEntity::ClientUpdateView( void ) {
	UpdateBind( true );
	Present();
}

/*
================
rvClientEntity::Bind
================
*/
void rvClientEntity::Bind( idEntity* master, jointHandle_t joint ) {
	Unbind();

	if ( joint != INVALID_JOINT && !master->GetAnimator() ) {
		gameLocal.Warning( "rvClientEntity::Bind: entity '%s' cannot support skeletal models.", master->GetName() );
		joint = INVALID_JOINT;
	}

	bindMaster = master;
	bindJoint  = joint;
	bindOrigin = worldOrigin;
	bindAxis   = worldAxis;

	bindNode.AddToEnd( bindMaster->clientEntities );

	UpdateBind( false );
}

/*
================
rvClientEntity::Bind
================
*/
void rvClientEntity::Bind( rvClientEntity* master, jointHandle_t joint ) {
	Unbind();

	if ( joint != INVALID_JOINT && !master->GetAnimator() ) {
		gameLocal.Warning( "rvClientEntity::Bind: entity '%s' cannot support skeletal models.", master->GetName() );
		joint = INVALID_JOINT;
	}

	bindMasterClient = master;
	bindJoint = joint;
	bindOrigin = worldOrigin;
	bindAxis = worldAxis;

	bindNode.AddToEnd( bindMasterClient->clientEntities );

	UpdateBind( false );
}

/*
=====================
rvClientEntity::PlayEffect
=====================
*/
rvClientEffect* rvClientEntity::PlayEffect( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t jointHandle, bool loop, const idVec3& endOrigin ) {
	return NULL;
}

/*
================
rvClientEntity::PlayEffect
================
*/
rvClientEffect* rvClientEntity::PlayEffect( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop, const idVec3& endOrigin ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return NULL;
	}

//	assert ( joint != INVALID_JOINT );

	if ( effectHandle < 0 ) {
		return NULL;
	}

	rvClientEffect* effect = new rvClientEffect( effectHandle );
	effect->SetOrigin( vec3_origin );
	effect->SetAxis( mat3_identity );
	effect->Bind( this, joint );
	effect->SetGravity( gameLocal.GetGravity() );
	effect->SetMaterialColor( color );

	if ( !effect->Play ( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}

	return effect;
}

/*
================
rvClientEntity::Unbind
================
*/
void rvClientEntity::Unbind	( void ) {
	bindMaster = NULL;
	bindMasterClient = NULL;
	bindNode.Remove();
}

/*
================
rvClientEntity::SetOrigin
================
*/
void rvClientEntity::SetOrigin( const idVec3& origin ) {
	if ( IsBound() ) {
		bindOrigin = origin;
	} else {
		worldOrigin = origin;
	}
}

/*
================
rvClientEntity::SetAxis
================
*/
void rvClientEntity::SetAxis ( const idMat3& axis ) {
	if ( IsBound() && axisBind ) {
		bindAxis = axis;
	} else {
		worldAxis = axis;
	}
}

/*
================
rvClientEntity::IsBound
================
*/
bool rvClientEntity::IsBound( void ) {
	return bindMaster != NULL || bindMasterClient != NULL;
}

/*
================
rvClientEntity::UpdateBind
================
*/
void rvClientEntity::UpdateBind( bool skipModelUpdate ) {
	if ( !IsBound() ) {
		return;
	}

	idMat3 tempWorldAxis;
	if ( bindJoint != INVALID_JOINT ) {
		if ( bindMaster.IsValid() ) {
			if ( skipModelUpdate ) {
				bindMaster->GetWorldOriginAxisNoUpdate( bindJoint, worldOrigin, tempWorldAxis );
			} else {
				bindMaster->GetAnimator()->GetJointTransform( bindJoint, gameLocal.time, worldOrigin, tempWorldAxis );
				worldOrigin = bindMaster->GetLastPushedOrigin() + ( worldOrigin * bindMaster->GetLastPushedAxis() );
				tempWorldAxis *= bindMaster->GetLastPushedAxis();
			}
		} else {
			rvClientEntity* clientEnt = bindMasterClient;

			clientEnt->GetAnimator()->GetJointTransform( bindJoint, gameLocal.time, worldOrigin, tempWorldAxis );

			renderEntity_t* renderEnt = clientEnt->GetRenderEntity();

			worldOrigin = renderEnt->origin + ( worldOrigin * renderEnt->axis );
			tempWorldAxis	*= renderEnt->axis;
		}
	} else {
		if ( bindMaster.IsValid() ) {
			worldOrigin	= bindMaster->GetLastPushedOrigin();
			tempWorldAxis	= bindMaster->GetLastPushedAxis();
		} else {
			renderEntity_t* renderEnt = bindMasterClient->GetRenderEntity();

			worldOrigin	= renderEnt->origin;
			tempWorldAxis = renderEnt->axis;
		}
	}

	worldOrigin += (bindOrigin * tempWorldAxis);
	if ( axisBind ) {
		worldAxis    = bindAxis * tempWorldAxis;
	}
}

/*
================
rvClientEntity::DrawDebugInfo
================
*/
void rvClientEntity::DrawDebugInfo ( void ) const {
	idBounds bounds( idVec3(-8,-8,-8), idVec3(8,8,8) );

	gameRenderWorld->DebugBounds( colorGreen, bounds, worldOrigin );

	if ( gameLocal.GetLocalPlayer() ) {
		gameRenderWorld->DrawText( GetClassname(), worldOrigin, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->firstPersonViewAxis, 1 );
	}
}

/*
================
rvClientEntity::UpdateSound
================
*/
void rvClientEntity::UpdateSound( void ) {
	if ( refSound.referenceSound ) {
		refSound.origin = worldOrigin;
		refSound.referenceSound->UpdateEmitter( refSound.origin, refSound.listenerId, &refSound.parms );
	}
}

/*
================
rvClientEntity::StartSoundShader
================
*/
int rvClientEntity::StartSoundShader( const idSoundShader* shader, const soundChannel_t channel, int soundShaderFlags )  {
	if ( !shader ) {
		return 0;
	}

	if ( !refSound.referenceSound ) {
		refSound.referenceSound = gameSoundWorld->AllocSoundEmitter();
	}

	UpdateSound();

	return refSound.referenceSound->StartSound( shader, channel, channel, gameLocal.random.RandomFloat(), soundShaderFlags  );
}

/*
================
rvClientEntity::GetClientMaster
================
*/
rvClientPhysics* rvClientEntity::GetClientMaster( void ) {
	return gameLocal.entities[ ENTITYNUM_CLIENT ]->Cast< rvClientPhysics >();
}

/*
================
rvClientEntity::MakeCurrent
================
*/
void rvClientEntity::MakeCurrent( void ) {
	rvClientPhysics* clientEnt = GetClientMaster();
	if ( clientEnt ) {
		clientEnt->PushCurrentClientEntity( entityNumber );
	} else {
		assert( false );
	}
}

/*
================
rvClientEntity::ResetCurrent
================
*/
void rvClientEntity::ResetCurrent( void ) {
	rvClientPhysics* clientEnt = GetClientMaster();
	if ( clientEnt ) {
		clientEnt->PopCurrentClientEntity();
	} else {
		assert( false );
	}
}

/*
================
rvClientEntity::RunPhysics
================
*/
void rvClientEntity::RunPhysics( void ) {
	idPhysics* physics = GetPhysics();
	if ( physics ) {
		MakeCurrent();
		physics->Evaluate( gameLocal.time - gameLocal.previousTime, gameLocal.time );
		worldOrigin = physics->GetOrigin();
		worldAxis = physics->GetAxis();
		ResetCurrent();
	}
}

/*
================
rvClientEntity::GetPhysics
================
*/
idPhysics* rvClientEntity::GetPhysics( void ) const {
	return NULL;
}

/*
================
rvClientEntity::Collide
================
*/
bool rvClientEntity::Collide( const trace_t &collision, const idVec3 &velocity ) {
	return false;
}

/*
================
rvClientEntity::RemoveClientEntities
================
*/
void rvClientEntity::RemoveClientEntities( void ) {
	rvClientEntity* cent;
	rvClientEntity* next;

	// jrad - keep client effect entities around so they can finish thinking/evaluating
	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect ) {
			effect->Stop();
			continue;
		}

		cent->Dispose();
	}
	clientEntities.Clear();
}

/*
===============================================================================

	rvClientPhysics

===============================================================================
*/

extern const idEventDef EV_GetWorldOrigin;

CLASS_DECLARATION( idEntity, rvClientPhysics )
END_CLASS

/*
=====================
rvClientPhysics::~rvClientPhysics
=====================
*/
rvClientPhysics::~rvClientPhysics( void ) {
}

/*
=====================
rvClientPhysics::Spawn
=====================
*/
void rvClientPhysics::Spawn( void ) {
}

/*
=====================
rvClientPhysics::CanCollide
=====================
*/
bool rvClientPhysics::CanCollide( const idEntity* other, int traceId ) const {
	return GetCurrentClientEntity()->CanCollide( other, traceId );
}

/*
=====================
rvClientPhysics::Collide
=====================
*/
bool rvClientPhysics::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	return GetCurrentClientEntity()->Collide( collision, velocity );
}

/*
=====================
rvClientPhysics::GetCurrentClientEntity
=====================
*/
rvClientEntity* rvClientPhysics::GetCurrentClientEntity( void ) const {
	assert( activeEntities.Num() && activeEntities[ 0 ] >= 0 && activeEntities[ 0 ] < MAX_CENTITIES );
	assert( gameLocal.clientEntities[ activeEntities[ 0 ] ] );

	return gameLocal.clientEntities[ activeEntities[ 0 ] ];
}




/*
===============================================================================

	sdClientScriptEntity

===============================================================================
*/
extern const idEventDef EV_GetJointHandle;
extern const idEventDef EV_SetState;

const idEventDef EV_Dispose( "dispose", '\0', DOC_TEXT( "Removes all allocated resources associated with the entity, and schedules it for deletion at the end of the frame." ), 0, NULL );
const idEventDef EV_EnableAxisBind( "enableAxisBind", '\0', DOC_TEXT( "Enables/disables orientation locked bind mode." ), 1, NULL, "b", "state", "Whether to enable or disable the mode." );

CLASS_DECLARATION( rvClientEntity, sdClientScriptEntity )
	EVENT( EV_GetWorldOrigin,				sdClientScriptEntity::Event_GetWorldOrigin )
	EVENT( EV_GetWorldAxis,					sdClientScriptEntity::Event_GetWorldAxis )
	EVENT( EV_GetOwner,						sdClientScriptEntity::Event_GetOwner )
	EVENT( EV_IsOwner,						sdClientScriptEntity::Event_IsOwner )
	EVENT( EV_GetDamagePower,				sdClientScriptEntity::Event_GetDamagePower )
	EVENT( EV_SetState,						sdClientScriptEntity::Event_SetState )
	EVENT( EV_GetKey,						sdClientScriptEntity::Event_GetKey )
	EVENT( EV_GetIntKey,					sdClientScriptEntity::Event_GetIntKey )
	EVENT( EV_GetFloatKey,					sdClientScriptEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,					sdClientScriptEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,					sdClientScriptEntity::Event_GetEntityKey )
	EVENT( EV_PlayEffect,					sdClientScriptEntity::Event_PlayEffect )
	EVENT( EV_StopEffect,					sdClientScriptEntity::Event_StopEffect )
	EVENT( EV_StopEffectHandle,				sdClientScriptEntity::Event_StopEffectHandle )
	EVENT( EV_PlayMaterialEffect,			sdClientScriptEntity::Event_PlayMaterialEffect )
	EVENT( EV_KillEffect,					sdClientScriptEntity::Event_KillEffect )
	EVENT( EV_SetOrigin,					sdClientScriptEntity::Event_SetOrigin )
	EVENT( EV_SetAngles,					sdClientScriptEntity::Event_SetAngles )
	EVENT( EV_SetGravity,					sdClientScriptEntity::Event_SetGravity )
	EVENT( EV_SetWorldAxis,					sdClientScriptEntity::Event_SetWorldAxis )
	EVENT( EV_Bind,							sdClientScriptEntity::Event_Bind )
	EVENT( EV_BindToJoint,					sdClientScriptEntity::Event_BindToJoint )
	EVENT( EV_Unbind,						sdClientScriptEntity::Event_UnBind )
	EVENT( EV_AddCheapDecal,				sdClientScriptEntity::Event_AddCheapDecal )
	EVENT( EV_GetJointHandle,				sdClientScriptEntity::Event_GetJointHandle )
	EVENT( EV_Dispose,						sdClientScriptEntity::Event_Dispose )
	EVENT( EV_EnableAxisBind,				sdClientScriptEntity::Event_EnableAxisBind )
END_CLASS

/*
=====================
sdClientScriptEntity::sdClientScriptEntity
=====================
*/
sdClientScriptEntity::sdClientScriptEntity( void ) {
	scriptState			= NULL;
	scriptIdealState	= NULL;
	baseScriptThread	= NULL;
	scriptObject		= NULL;
}

/*
=====================
sdClientScriptEntity::~sdClientScriptEntity
=====================
*/
sdClientScriptEntity::~sdClientScriptEntity( void ) {
	CleanUp();
}

/*
=====================
sdClientScriptEntity::CleanUp
=====================
*/
void sdClientScriptEntity::CleanUp( void ) {
	if ( baseScriptThread != NULL ) {
		gameLocal.program->FreeThread( baseScriptThread );
		baseScriptThread = NULL;
	}

	DeconstructScriptObject();

	rvClientEntity::CleanUp();
}

/*
=====================
sdClientScriptEntity::Think
=====================
*/
void sdClientScriptEntity::Think( void ) {
	UpdateScript();
	rvClientEntity::Think();
}

/*
=====================
sdClientScriptEntity::Spawn
=====================
*/
void sdClientScriptEntity::Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type ) {
	if ( _spawnArgs ) {
		spawnArgs = *_spawnArgs;
	} else {
		spawnArgs.Clear();
	}

	if ( type ) {
		scriptObject = gameLocal.program->AllocScriptObject( this, type );

		baseScriptThread = ConstructScriptObject();
	}
}

/*
=====================
sdClientScriptEntity::Spawn
=====================
*/
void sdClientScriptEntity::CreateByName( const idDict* _spawnArgs, const char* scriptObjectName ) {
	Create( _spawnArgs, gameLocal.program->FindTypeInfo( scriptObjectName ) );
}

/*
=====================
sdClientScriptEntity::UpdateScript
=====================
*/
void sdClientScriptEntity::UpdateScript( void ) {
	if ( !baseScriptThread || gameLocal.IsPaused() ) {
		return;
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( int i = 0; i < 20; i++ ) {
		if ( scriptIdealState != scriptState ) {
			SetState( scriptIdealState );
		}

		// don't call script until it's done waiting
		if ( baseScriptThread->IsWaiting() ) {
			break;
		}

//		MakeCurrent();
		baseScriptThread->Execute();
//		ResetCurrent();

		if ( scriptIdealState == scriptState ) {
			break;
		}
	}
}

/*
================
sdClientScriptEntity::ConstructScriptObject
================
*/
sdProgramThread* sdClientScriptEntity::ConstructScriptObject( void ) {
	// init the script object's data
	scriptObject->ClearObject();

	sdScriptHelper h1, h2;
	CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h1 );
	CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h2 );

	sdProgramThread* thread = NULL;

	// call script object's constructor
	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	if ( constructor ) {
		// start a thread that will initialize after Spawn is done being called
		thread = gameLocal.program->CreateThread();
		thread->SetName( "sdClientScriptEntity" );
		thread->CallFunction( scriptObject, constructor );
		thread->ManualControl();
		thread->ManualDelete();
	}

	return thread;
}

/*
================
sdClientScriptEntity::DeconstructScriptObject
================
*/
void sdClientScriptEntity::DeconstructScriptObject( void ) {
	if ( !scriptObject ) {
		return;
	}

	// call script object's destructor
	sdScriptHelper h;
	CallNonBlockingScriptEvent( scriptObject->GetDestructor(), h );

	gameLocal.program->FreeScriptObject( scriptObject );
}

/*
=====================
sdClientScriptEntity::SetState
=====================
*/
bool sdClientScriptEntity::SetState( const sdProgram::sdFunction* newState ) {
	if ( !newState ) {
		gameLocal.Error( "sdScriptEntity::SetState NULL state" );
	}

	scriptState = newState;
	scriptIdealState = scriptState;

	baseScriptThread->CallFunction( scriptObject, scriptState );

	return true;
}

/*
=====================
sdClientScriptEntity::CallNonBlockingScriptEvent
=====================
*/
void sdClientScriptEntity::CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) {
	if ( helper.Init( scriptObject, function ) ) {
		helper.Run();
		if ( !helper.Done() ) {
			gameLocal.Error( "sdClientScriptEntity::CallNonBlockingScriptEvent '%s' Cannot be Blocking", function->GetName() );
		}
	}
}

/*
=====================
sdClientScriptEntity::OnSetState
=====================
*/
void sdClientScriptEntity::OnSetState( const char* name ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( name );
	if ( !func ) {
		gameLocal.Error( "sdScriptEntity::OnSetState Can't find function '%s' in object '%s'", name, scriptObject->GetTypeName() );
	}

	scriptIdealState = func;
	baseScriptThread->DoneProcessing();
}

/*
=====================
sdClientScriptEntity::Event_GetKey
=====================
*/
void sdClientScriptEntity::Event_GetKey( const char* key ) {
	sdProgram::ReturnString( spawnArgs.GetString( key ) );
}

/*
=====================
sdClientScriptEntity::Event_GetIntKey
=====================
*/
void sdClientScriptEntity::Event_GetIntKey( const char* key ) {
	sdProgram::ReturnInteger( spawnArgs.GetInt( key ) );
}

/*
=====================
sdClientScriptEntity::Event_GetFloatKey
=====================
*/
void sdClientScriptEntity::Event_GetFloatKey( const char* key ) {
	sdProgram::ReturnFloat( spawnArgs.GetFloat( key ) );
}

/*
=====================
sdClientScriptEntity::Event_GetVectorKey
=====================
*/
void sdClientScriptEntity::Event_GetVectorKey( const char* key ) {
	sdProgram::ReturnVector( spawnArgs.GetVector( key ) );
}

/*
=====================
sdClientScriptEntity::Event_GetEntityKey
=====================
*/
void sdClientScriptEntity::Event_GetEntityKey( const char* key ) {
	const char* entname = spawnArgs.GetString( key );
	if ( !*entname ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	idEntity* ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key", entname, key );
	}

	sdProgram::ReturnEntity( ent );
}

/*
=====================
sdClientScriptEntity::Event_GetWorldOrigin
=====================
*/
void sdClientScriptEntity::Event_GetWorldOrigin( void ) {
	sdProgram::ReturnVector( GetOrigin() );
}

/*
=====================
sdClientScriptEntity::Event_GetWorldAxis
=====================
*/
void sdClientScriptEntity::Event_GetWorldAxis( int index ) {
	sdProgram::ReturnVector( GetAxis()[ index ] );
}

/*
=====================
sdClientScriptEntity::Event_GetDamagePower
=====================
*/
void sdClientScriptEntity::Event_GetDamagePower( void ) {
	sdProgram::ReturnFloat( 1.f );
}

/*
=====================
sdClientScriptEntity::Event_GetOwner
=====================
*/
void sdClientScriptEntity::Event_GetOwner( void ) {
	sdProgram::ReturnEntity( GetOwner() );
}

/*
================
sdClientScriptEntity::Event_IsOwner
================
*/
void sdClientScriptEntity::Event_IsOwner( idEntity* other ) {
	sdProgram::ReturnBoolean( false );
	for ( int i = 0; i < GetNumOwners(); i++ ) {
		if ( other == GetOwner( i ) ) {
			sdProgram::ReturnBoolean( true );
			return;
		}
	}
}

/*
=====================
sdClientScriptEntity::Event_SetState
=====================
*/
void sdClientScriptEntity::Event_SetState( const char* name ) {
	OnSetState( name );
}

/*
=====================
sdClientScriptEntity::Event_PlayMaterialEffect
=====================
*/
void sdClientScriptEntity::Event_PlayMaterialEffect( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop ) {
	rvClientEntityPtr< rvClientEffect > eff;
	eff = gameLocal.PlayEffect( spawnArgs, color, effectName, materialType, GetOrigin(), GetAxis(), loop );
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
=====================
sdClientScriptEntity::Event_PlayEffect
=====================
*/
void sdClientScriptEntity::Event_PlayEffect( const char* effectName, const char* jointName, bool loop ) {
	jointHandle_t joint;

	joint = GetAnimator() ? GetAnimator()->GetJointHandle( jointName ) : INVALID_JOINT;

	rvClientEntityPtr< rvClientEffect > eff;
	eff = PlayEffect( effectName, colorWhite.ToVec3(), NULL, joint, loop );
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
=====================
sdClientScriptEntity::Event_StopEffect
=====================
*/
void sdClientScriptEntity::Event_StopEffect( const char* effectName ) {
	int effectIndex = gameLocal.GetEffectHandle( spawnArgs, effectName, NULL );

	rvClientEntity* next;
	for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect == NULL ) {
			continue;
		}

		if ( effect->GetEffectIndex() == effectIndex ) {
			effect->Stop( false );
		}
	}
}

/*
================
sdClientScriptEntity::Event_StopEffectHandle
================
*/
void sdClientScriptEntity::Event_StopEffectHandle( int handle ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect.GetEntity()->Stop( false );
	}
}

/*
================
sdClientScriptEntity::Event_KillEffect
================
*/
void sdClientScriptEntity::Event_KillEffect( const char *effectName ) {
	int effectIndex = gameLocal.GetEffectHandle( spawnArgs, effectName, NULL );

	rvClientEntity* next;
	for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect == NULL ) {
			continue;
		}

		if ( effect->GetEffectIndex() == effectIndex ) {
			effect->Stop( true );
		}
	}
}

/*
================
sdClientScriptEntity::Event_SetOrigin
================
*/
void sdClientScriptEntity::Event_SetOrigin( const idVec3& org ) {
	SetOrigin( org );

	if ( GetPhysics() != NULL ) {
		GetPhysics()->SetOrigin( org );
	}
}

/*
================
sdClientScriptEntity::Event_SetAngles
================
*/
void sdClientScriptEntity::Event_SetAngles( const idAngles& ang ) {
	idMat3 axis = ang.ToMat3();

	SetAxis( axis );

	if ( GetPhysics() != NULL ) {
		GetPhysics()->SetAxis( axis );
	}
}

/*
================
sdClientScriptEntity::Event_SetWorldAxis
================
*/
void sdClientScriptEntity::Event_SetWorldAxis( const idVec3& fwd, const idVec3& right, const idVec3& up ) {
	idMat3 axis( fwd, right, up );
	SetAxis( axis );
}

/*
================
sdClientScriptEntity::Event_Bind
================
*/
void sdClientScriptEntity::Event_Bind( idEntity *bindMaster ) {
	Bind( bindMaster );
}

/*
================
sdClientScriptEntity::Event_BindToJoint
================
*/
void sdClientScriptEntity::Event_BindToJoint( idEntity *bindMaster, const char *boneName, float rotateWithMaster ) {
	if ( bindMaster == NULL ) {
		gameLocal.Warning( "sdClientScriptEntity::Event_BindToJoint: entity is NULL" );
		return;
	}

	if ( !bindMaster->GetAnimator() ) {
		gameLocal.Warning( "sdClientScriptEntity::Event_BindToJoint: entity '%s' cannot support skeletal models.", bindMaster->GetName() );
		return;
	}

	EnableAxisBind( rotateWithMaster > 0.f );
	jointHandle_t hand = bindMaster->GetAnimator()->GetJointHandle( boneName );

	if ( hand == INVALID_JOINT ) {
		gameLocal.Warning( "sdClientScriptEntity::Event_BindToJoint: invalid joint name %s.", boneName );
		hand = INVALID_JOINT;
	}

	Bind( bindMaster, hand );
}

/*
================
sdClientScriptEntity::Event_UnBind
================
*/
void sdClientScriptEntity::Event_UnBind( void ) {
	Unbind();
}

/*
================
sdClientScriptEntity::Event_AddCheapDecal
================
*/
void sdClientScriptEntity::Event_AddCheapDecal( idEntity *attachTo, idVec3 &origin, idVec3 &normal, const char* decalName, const char* materialName ) {
	gameLocal.AddCheapDecal( spawnArgs, attachTo, origin, normal, INVALID_JOINT, 0, decalName, materialName );
}

/*
================
sdClientScriptEntity::Event_GetJointHandle
================
*/
void sdClientScriptEntity::Event_GetJointHandle( const char* jointName ) {
	if ( GetAnimator() != NULL ) {
		sdProgram::ReturnInteger( GetAnimator()->GetJointHandle( jointName ) );
	} else {
		sdProgram::ReturnInteger( INVALID_JOINT );
	}
}

/*
================
sdClientScriptEntity::Event_Dispose
================
*/
void sdClientScriptEntity::Event_Dispose( void ) {
	Dispose();
}

/*
================
sdClientScriptEntity::Event_EnableAxisBind
================
*/
void sdClientScriptEntity::Event_EnableAxisBind( bool enabled ) {
	EnableAxisBind( enabled );
}

/*
================
sdClientScriptEntity::Event_SetGravity
================
*/
void sdClientScriptEntity::Event_SetGravity( const idVec3& gravity ) {
	if ( GetPhysics() != NULL ) {
		GetPhysics()->SetGravity( gravity );
	}
}


/*
===============================================================================

	sdClientProjectile

===============================================================================
*/

CLASS_DECLARATION( sdClientScriptEntity, sdClientProjectile )
END_CLASS

/*
=====================
sdClientProjectile::sdClientProjectile
=====================
*/
sdClientProjectile::sdClientProjectile( void ) {
}

/*
=====================
sdClientProjectile::Launch
=====================
*/
void sdClientProjectile::Launch( idEntity* owner, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis ) {
	idAngles tracerMuzzleAngles = tracerMuzzleAxis.ToAngles();
	idVec3 anglesVec( tracerMuzzleAngles.pitch, tracerMuzzleAngles.yaw, tracerMuzzleAngles.roll );

	AddOwner( owner );

	sdScriptHelper helper;
	helper.Push( tracerMuzzleOrigin );
	helper.Push( anglesVec );
	CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnLaunch" ), helper );
}

/*
===============================================================================

	sdClientProjectile_Parabolic

===============================================================================
*/
extern const idEventDef EV_SetOwner;
extern const idEventDef EV_AddOwner;
extern const idEventDef EV_Launch;

CLASS_DECLARATION( sdClientProjectile, sdClientProjectile_Parabolic )
	EVENT( EV_SetOwner,					sdClientProjectile_Parabolic::Event_SetOwner )
	EVENT( EV_AddOwner,					sdClientProjectile_Parabolic::Event_AddOwner )
	EVENT( EV_SetTeam,					sdClientProjectile_Parabolic::Event_SetGameTeam )
	EVENT( EV_Launch,					sdClientProjectile_Parabolic::Event_Launch )
END_CLASS


void					Create( const idDict* _spawnArgs, const char* scriptObjectName );

/*
=================
sdClientProjectile_Parabolic::Create
=================
*/
void sdClientProjectile_Parabolic::Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type ) {
	sdClientProjectile::Create( _spawnArgs, type );

	InitPhysics();
}

/*
=================
sdClientProjectile_Parabolic::InitPhysics
=================
*/
void sdClientProjectile_Parabolic::InitPhysics( void ) {
	float gravity				= spawnArgs.GetFloat( "gravity" );

	idVec3 gravVec = gameLocal.GetGravity();
	gravVec.Normalize();

	physicsObj.SetSelf( gameLocal.entities[ENTITYNUM_CLIENT] );

	idBounds bounds;
	bounds[ 0 ] = spawnArgs.GetVector( "mins" );
	bounds[ 1 ] = spawnArgs.GetVector( "mins" );

	idClipModel* model = new idClipModel( idTraceModel( bounds ), false );
	model->SetContents( 0 );
	physicsObj.SetClipModel( model );
	physicsObj.SetGravity( gravVec * gravity );
	physicsObj.SetClipMask( MASK_PROJECTILE | CONTENTS_BODY | CONTENTS_SLIDEMOVER );
}

/*
=================
sdClientProjectile_Parabolic::Think
=================
*/
void sdClientProjectile_Parabolic::Think( void ) {
	RunPhysics();

	sdClientProjectile::Think();
}

/*
================
sdClientProjectile_Parabolic::Event_Launch
================
*/
void sdClientProjectile_Parabolic::Event_Launch( const idVec3& velocity ) {
	idVec3 org = GetPhysics()->GetOrigin();
	idVec3 dir = velocity;
	dir.Normalize();

	idMat3 axes = dir.ToMat3();

	physicsObj.Init( org, velocity, vec3_zero, axes, gameLocal.time, -1 );

	GetPhysics()->SetOrigin( org );
	GetPhysics()->SetAxis( axes );
}

/*
================
sdClientProjectile_Parabolic::Event_AddOwner
================
*/
void sdClientProjectile_Parabolic::Event_AddOwner( idEntity* other ) {
	AddOwner( other );
}

/*
================
sdClientProjectile_Parabolic::Event_SetOwner
================
*/
void sdClientProjectile_Parabolic::Event_SetOwner( idEntity* other ) {
	AddOwner( other );
}

/*
================
sdClientProjectile_Parabolic::Event_SetGameTeam
================
*/
void sdClientProjectile_Parabolic::Event_SetGameTeam( idScriptObject* object ) {
}

/*
================
sdClientProjectile_Parabolic::Collide
================
*/
bool sdClientProjectile_Parabolic::Collide( const trace_t& collision, const idVec3 &velocity ) {
	PostEventMS( &EV_Remove, 0 );
	return true;
}

/*
===============================================================================

	sdClientAnimated

===============================================================================
*/

extern const idEventDef EV_SetAnimFrame;
extern const idEventDef EV_GetNumFrames;
extern const idEventDef EV_SetModel;
extern const idEventDef EV_IsHidden;
extern const idEventDef EV_SetJointPos;
extern const idEventDef EV_SetJointAngle;
extern const idEventDef EV_GetJointPos;
extern const idEventDef EV_Show;
extern const idEventDef EV_Hide;
extern const idEventDef EV_GetAnimatingOnChannel;
extern const idEventDef EV_PlayAnim;
extern const idEventDef EV_PlayCycle;
extern const idEventDef EV_PlayAnimBlended;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_SetShaderParms;
extern const idEventDef EV_SetColor;
extern const idEventDef EV_GetColor;

CLASS_DECLARATION( sdClientScriptEntity, sdClientAnimated )
	EVENT( EV_SetAnimFrame,									sdClientAnimated::Event_SetAnimFrame )
	EVENT( EV_GetNumFrames,									sdClientAnimated::Event_GetNumFrames )
	EVENT( EV_IsHidden,										sdClientAnimated::Event_IsHidden )
	EVENT( EV_SetJointPos,									sdClientAnimated::Event_SetJointPos )
	EVENT( EV_SetJointAngle,								sdClientAnimated::Event_SetJointAngle )
	EVENT( EV_GetJointPos,									sdClientAnimated::Event_GetJointPos )
	EVENT( EV_Show,											sdClientAnimated::Show )
	EVENT( EV_Hide,											sdClientAnimated::Hide )

	EVENT( EV_PlayAnim,										sdClientAnimated::Event_PlayAnim )
	EVENT( EV_PlayCycle,									sdClientAnimated::Event_PlayCycle )
	EVENT( EV_PlayAnimBlended,								sdClientAnimated::Event_PlayAnimBlended )
	EVENT( EV_GetAnimatingOnChannel,						sdClientAnimated::Event_GetAnimatingOnChannel )

	EVENT( EV_SetSkin,										sdClientAnimated::Event_SetSkin )
	EVENT( EV_SetModel,										sdClientAnimated::Event_SetModel )

	EVENT( EV_GetShaderParm,								sdClientAnimated::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,								sdClientAnimated::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,								sdClientAnimated::Event_SetShaderParms )
	EVENT( EV_SetColor,										sdClientAnimated::Event_SetColor )
	EVENT( EV_GetColor,										sdClientAnimated::Event_GetColor )
END_CLASS

/*
=====================
sdClientAnimated::sdClientAnimated
=====================
*/
sdClientAnimated::sdClientAnimated( void ) {
	renderEntityHandle = -1;
	memset( &renderEntity, 0, sizeof( renderEntity ) );

	animatedFlags.hidden = false;

	animator.SetEntity( this );

	lastServiceTime = 0;
	thinkFlags		= 0;
}

/*
=====================
sdClientAnimated::~sdClientAnimated
=====================
*/
sdClientAnimated::~sdClientAnimated( void ) {
	CleanUp();
}

/*
=====================
sdClientAnimated::CleanUp
=====================
*/
void sdClientAnimated::CleanUp( void ) {
	FreeModelDef();

	sdClientScriptEntity::CleanUp();
}

/*
================
sdClientAnimated::ModelCallback
================
*/
bool sdClientAnimated::ModelCallback( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime ) {

	rvClientEntityPtr<rvClientEntity> safeEnt;
#pragma warning( push )
#pragma warning( disable: 4311 )
	safeEnt.SetSpawnId( (int)renderEntity->callbackData );
#pragma warning( pop )
	rvClientEntity* ent = safeEnt.GetEntity();//gameLocal.clientEntities[ *(int *)renderEntity->callbackData ];
	if ( !ent ) {
		return false;
		//gameLocal.Error( "sdClientAnimated::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView, lastGameModifiedTime );
}

/*
=====================
sdClientAnimated::SetStaticModel
=====================
*/
void sdClientAnimated::SetStaticModel( const char* modelName ) {
	assert( modelName );

	FreeModelDef();

	renderEntity.hModel = renderModelManager->FindModel( modelName );

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = NULL;
	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	UpdateModel();
}

/*
=====================
sdClientAnimated::SetModel
=====================
*/
void sdClientAnimated::SetModel( const char* modelName ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelName );
	if ( renderEntity.hModel == NULL ) {
		SetStaticModel( modelName );
		return;
	}

	gameEdit->RefreshRenderEntity( spawnArgs, renderEntity );

	if ( renderEntity.customSkin != NULL ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = sdClientAnimated::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateModel();
}

/*
=====================
sdClientAnimated::UpdateModel
=====================
*/
void sdClientAnimated::UpdateModel( void ) {
	renderEntity.origin = GetOrigin();
	renderEntity.axis = GetAxis();
}

/*
================
sdClientAnimated::Present
================
*/
void sdClientAnimated::Present( void ) {
	UpdateModel();

	if ( !renderEntity.hModel || animatedFlags.hidden ) {
		return;
	}

	if ( renderSystem->IsSMPEnabled() ) {
		if ( animator.CreateFrame( gameLocal.time, false )  ) {
		}
	}


	// add to refresh list
	if ( renderEntityHandle == -1 ) {
		renderEntityHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( renderEntityHandle, &renderEntity );
	}
}

/*
=====================
sdClientAnimated::Create
=====================
*/
void sdClientAnimated::Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type ) {
	sdClientScriptEntity::Create( _spawnArgs, type );

	gameEdit->ParseSpawnArgsToRenderEntity( spawnArgs, renderEntity );
	rvClientEntityPtr<sdClientAnimated> safeEnt;
	safeEnt.SetEntity( this );
#pragma warning( push )
#pragma warning( disable: 4312 )
	renderEntity.callbackData = (void*)safeEnt.GetSpawnId();
#pragma warning( pop )

	const char* modelName = spawnArgs.GetString( "model" );
	if ( *modelName ) {
		SetModel( modelName );
	}

	// spawn gui entities
	if ( !networkSystem->IsDedicated() && renderEntity.hModel != NULL ) {
		for ( int i = 0; i < renderEntity.hModel->NumGUISurfaces(); i++ ) {
			const guiSurface_t* guiSurface = renderEntity.hModel->GetGUISurface( i );

			const char* guiName = spawnArgs.GetString( guiSurface->guiNum == 0 ? "gui" : va( "gui%d", guiSurface->guiNum + 1 ) );
			if ( *guiName == '\0' ) {
				continue;
			}

			idStr theme = spawnArgs.GetString( guiSurface->guiNum == 0 ? "gui_theme" : va( "gui%d_theme", guiSurface->guiNum + 1 ), "default" );

			guiHandle_t handle = gameLocal.LoadUserInterface( guiName, true, false, theme );

			if ( handle.IsValid() ) {
				sdGuiSurface* cent = reinterpret_cast< sdGuiSurface* >( sdGuiSurface::Type.CreateInstance() );

				cent->Init( this, *guiSurface, handle, 0, renderEntity.flags.weaponDepthHack );

				sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
				if ( ui != NULL ) {
					ui->Activate();
				}
			}
		}
	}
}

/*
================
sdClientAnimated::FreeModelDef
================
*/
void sdClientAnimated::FreeModelDef( void ) {
	gameEdit->DestroyRenderEntity( renderEntity );
	if ( renderEntityHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( renderEntityHandle );
		renderEntityHandle = -1;
	}
}

/*
================
sdClientAnimated::UpdateRenderEntity
================
*/
bool sdClientAnimated::UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_t* renderView, int& lastGameModifiedTime ) {
	if ( !renderSystem->IsSMPEnabled() ) {
		if ( animator.CreateFrame( gameLocal.time, false ) || lastGameModifiedTime != animator.GetTransformCount() ) {
			lastGameModifiedTime = animator.GetTransformCount();
			return true;
		}
	}

	return false;
}

/*
================
sdClientAnimated::UpdateAnimation
================
*/
void sdClientAnimated::UpdateAnimation( void ) {
	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened in the past frame
	if ( !animatedFlags.hidden ) {
		if ( gameLocal.time > lastServiceTime ) {
			animator.ServiceAnims( lastServiceTime, gameLocal.time );
			lastServiceTime = gameLocal.time;
		}
	}

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
sdClientAnimated::Think
================
*/
void sdClientAnimated::Think( void ) {
	if ( thinkFlags & TH_ANIMATE ) {
		UpdateAnimation();
	}
	sdClientScriptEntity::Think();
}

/*
================
sdClientAnimated::BecomeActive
================
*/
void sdClientAnimated::BecomeActive( int flags, bool force ) {
	thinkFlags |= flags;
}

/*
================
sdClientAnimated::BecomeInactive
================
*/
void sdClientAnimated::BecomeInactive( int flags, bool force ) {
	thinkFlags &= ~flags;
}

/*
================
sdClientAnimated::Hide
================
*/
void sdClientAnimated::Hide( void ) {

	if ( !animatedFlags.hidden ) {
		FreeModelDef();
		animatedFlags.hidden = true;
	}
}

/*
================
sdClientAnimated::Show
================
*/
void sdClientAnimated::Show( void ) {
	if ( animatedFlags.hidden ) {
		animatedFlags.hidden = false;
		Present();
	}
}

/*
================
sdClientAnimated::SetSkin
================
*/
void sdClientAnimated::SetSkin( const idDeclSkin* skin ) {
	renderEntity.customSkin = skin;
}

/*
================
sdClientAnimated::Event_SetAnimFrame
================
*/
void sdClientAnimated::Event_SetAnimFrame( const char* anim, animChannel_t channel, float frame ) {
	animator.SetFrame( channel, animator.GetAnim( anim ), frame, gameLocal.time, 0 );
}

/*
================
sdClientAnimated::Event_GetNumFrames
================
*/
void sdClientAnimated::Event_GetNumFrames( const char* animName ) {
	int anim = animator.GetAnim( animName );
	sdProgram::ReturnInteger( animator.NumFrames( anim ) );
}

/*
================
sdClientAnimated::Event_IsHidden
================
*/
void sdClientAnimated::Event_IsHidden( void ) {
	sdProgram::ReturnBoolean( animatedFlags.hidden );
}

/*
================
sdClientAnimated::Event_SetJointAngle
================
*/
void sdClientAnimated::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}


/*
================
sdClientAnimated::Event_SetJointAngle
================
*/
void sdClientAnimated::Event_SetJointAngle( jointHandle_t joint, jointModTransform_t transformType, const idAngles& angles ) {
	animator.SetJointAxis( joint, transformType, angles.ToMat3() );
}

/*
=====================
sdClientAnimated::GetJointWorldTransform
=====================
*/
bool sdClientAnimated::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		offset.Zero();
		axis.Identity();
		return false;
	}

	offset = renderEntity.origin + offset * renderEntity.axis;
	axis *= renderEntity.axis;
	return true;
}

/*
================
sdClientAnimated::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void sdClientAnimated::Event_GetJointPos( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		offset.Zero();
		gameLocal.Warning( "Joint # %d out of range on client entity '%s'", jointnum, GetName() );
	}

	sdProgram::ReturnVector( offset );
}

/*
================
sdClientAnimated::Event_PlayAnim
================
*/
void sdClientAnimated::Event_PlayAnim( animChannel_t channel, const char *animname ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) {
			gameLocal.Warning( "sdClientAnimated::Event_PlayAnim missing '%s' animation", animname );
		}
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0 );
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, 0 );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
sdClientAnimated::Event_PlayCycle
================
*/
void sdClientAnimated::Event_PlayCycle( animChannel_t channel, const char *animname ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) {
			gameLocal.Warning( "sdClientAnimated::Event_PlayCycle missing '%s' animation", animname );
		}
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0.0f );
	} else {
		animator.CycleAnim( channel, anim, gameLocal.time, 0 );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}



/*
================
sdClientAnimated::Event_PlayAnimBlended
================
*/
void sdClientAnimated::Event_PlayAnimBlended( animChannel_t channel, const char *animname, float blendTime ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) {
			gameLocal.Warning( "idAnimatedEntity::Event_PlayAnimBlended missing '%s' animation", animname );
		}
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0 );
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, SEC2MS( blendTime ) );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
sdClientAnimated::Event_GetAnimatingOnChannel
================
*/
void sdClientAnimated::Event_GetAnimatingOnChannel( animChannel_t channel ) {
	idAnimBlend *blend = animator.CurrentAnim( channel );
	if ( !blend ) {
		sdProgram::ReturnString( "" );
		return;
	}

	int index = blend->AnimNum();
	const idAnim* anim = animator.GetAnim( index );
	if ( !anim ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( anim->FullName() );
}

/*
================
sdClientAnimated::Event_SetSkin
================
*/
void sdClientAnimated::Event_SetSkin( const char* skinname ) {
	if( !*skinname ) {
		SetSkin( NULL );
	} else {
		SetSkin( gameLocal.declSkinType[ skinname ] );
	}
}

/*
================
sdClientAnimated::Event_SetModel
================
*/
void sdClientAnimated::Event_SetModel( const char* modelName ) {
	SetModel( modelName );
}


/*
=====================
sdClientAnimated::PlayEffect
=====================
*/
rvClientEffect* sdClientAnimated::PlayEffect( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t jointHandle, bool loop, const idVec3& endOrigin ) {
	if ( animatedFlags.hidden ) {
		return NULL;
	}

	return sdClientScriptEntity::PlayEffect( gameLocal.GetEffectHandle( spawnArgs, effectName, materialType ), color, jointHandle, loop, endOrigin );
}

void sdClientAnimated::Event_GetShaderParm( int parm ) {
	sdProgram::ReturnFloat( renderEntity.shaderParms[ (int)parm ] );
}


void sdClientAnimated::Event_SetShaderParm( int parm, float value ) {
	renderEntity.shaderParms[ (int)parm ] = value;
}

void sdClientAnimated::Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderEntity.shaderParms[ 0 ] = parm0;
	renderEntity.shaderParms[ 1 ] = parm1;
	renderEntity.shaderParms[ 2 ] = parm2;
	renderEntity.shaderParms[ 3 ] = parm3;
}

void sdClientAnimated::Event_SetColor( float red, float green, float blue ) {
	renderEntity.shaderParms[ SHADERPARM_RED ] = red;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = green;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = blue;
}

void sdClientAnimated::Event_GetColor() {
	sdProgram::ReturnVector( idVec3( renderEntity.shaderParms[ SHADERPARM_RED ], renderEntity.shaderParms[ SHADERPARM_GREEN ], renderEntity.shaderParms[ SHADERPARM_BLUE ] ) );
}

const char * sdClientAnimated::GetName( void ) const {

	return va( "sdClientAnimated %s", renderEntity.hModel ? renderEntity.hModel->Name() : "<NULL>" );
}

/*
===============================================================================

sdClientLight

===============================================================================
*/

extern const idEventDef EV_TurnOn;
extern const idEventDef EV_TurnOff;

CLASS_DECLARATION( sdClientScriptEntity, sdClientLight )
	EVENT( EV_TurnOn,						sdClientLight::Event_On )
	EVENT( EV_TurnOff,						sdClientLight::Event_Off )
END_CLASS

/*
=====================
sdClientLight::sdClientLight
=====================
*/
sdClientLight::sdClientLight( void ) {
	lightDefHandle = -1;
	on = false;
}

/*
=====================
sdClientLight::~sdClientLight
=====================
*/
sdClientLight::~sdClientLight( void ) {
	if ( lightDefHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightDefHandle );
	}
}

/*
=====================
sdClientLight::Create
=====================
*/
void sdClientLight::Create( const idDict* _spawnArgs, const sdProgram::sdTypeInfo* type ) {
	sdClientScriptEntity::Create( _spawnArgs, type );

	gameEdit->ParseSpawnArgsToRenderLight( spawnArgs, renderLight );
	lightDefHandle = -1;
	on = false;

	if ( spawnArgs.GetBool( "start_off", "0" ) ) {
		Event_Off();
	} else {
		Event_On();
	}
}

/*
=====================
sdClientLight::Present
=====================
*/
void sdClientLight::Present( void ) {
	float parm0, parm1, parm2;

	renderLight.origin = worldOrigin;
	renderLight.axis = worldAxis;

	parm0 = renderLight.shaderParms[0];
	parm1 = renderLight.shaderParms[1];
	parm2 = renderLight.shaderParms[2];

	if ( !on ) {
		renderLight.shaderParms[0] = 0;
		renderLight.shaderParms[1] = 0;
		renderLight.shaderParms[2] = 0;
	}

	// add to refresh list
	if ( ( lightDefHandle != -1 ) ) {
		gameRenderWorld->UpdateLightDef( lightDefHandle, &renderLight );
	} else {
		lightDefHandle = gameRenderWorld->AddLightDef( &renderLight );
	}

	renderLight.shaderParms[0] = parm0;
	renderLight.shaderParms[1] = parm1;
	renderLight.shaderParms[2] = parm2;
}

/*
=====================
sdClientLight::Event_On
=====================
*/
void sdClientLight::Event_On( void ) {
	on = true;
}

/*
=====================
sdClientLight::Event_Off
=====================
*/
void sdClientLight::Event_Off( void ) {
	on = false;
}

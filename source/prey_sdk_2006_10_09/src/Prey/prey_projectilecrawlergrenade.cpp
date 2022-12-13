#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

  hhProjectileCrawlerGrenade
	
***********************************************************************/
const idEventDef EV_ApplyExpandWound( "<applyExpandWound>" );

const idEventDef EV_DyingState( "<dyingState>" );
const idEventDef EV_DeadState( "<deadState>" );

CLASS_DECLARATION( hhProjectile, hhProjectileCrawlerGrenade )
	EVENT( EV_ApplyExpandWound,			hhProjectileCrawlerGrenade::Event_ApplyExpandWound )
	EVENT( EV_DyingState,				hhProjectileCrawlerGrenade::EnterDyingState )
	EVENT( EV_DeadState,				hhProjectileCrawlerGrenade::EnterDeadState )

	EVENT( EV_Collision_Flesh,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Metal,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_AltMetal,		hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Wood,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Stone,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Glass,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_CardBoard,		hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Tile,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Forcefield,		hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Pipe,			hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	EVENT( EV_Collision_Wallwalk,		hhProjectileCrawlerGrenade::Event_Collision_Bounce )
	//EVENT( EV_Collision_Chaff,			hhProjectileCrawlerGrenade::Event_Collision_Explode )
	EVENT( EV_Collision_Liquid,			hhProjectileCrawlerGrenade::Event_Collision_DisturbLiquid )

	EVENT( EV_AllowCollision_Chaff,		hhProjectileCrawlerGrenade::Event_AllowCollision_Collide )
END_CLASS

/*
=================
hhProjectileCrawlerGrenade::Spawn
=================
*/
void hhProjectileCrawlerGrenade::Spawn() {
	modelScale.Init( gameLocal.time, 0, 1.0f, 1.0f );
	modelProxy = NULL;

	InitCollisionInfo();

	//doesn't matter for single player, only for network logic -rww
	modelProxyCopyDone = false;

	if( !gameLocal.isClient ) {
		SpawnModelProxy();
	}

	BecomeActive( TH_TICKER );

	if (gameLocal.isClient)
	{ //rww - do this right away on the client
		//Get rid of our model.  The modelProxy is our model now.
		SetModel( "" );
	}

	//rww - allow events on client
	fl.clientEvents = true;
}

/*
=================
hhProjectileCrawlerGrenade::~hhProjectileCrawlerGrenade
=================
*/
hhProjectileCrawlerGrenade::~hhProjectileCrawlerGrenade() {
	SAFE_REMOVE( modelProxy );
}

/*
=================
hhProjectileCrawlerGrenade::InitCollisionInfo
=================
*/
void hhProjectileCrawlerGrenade::InitCollisionInfo() {
	memset( &collisionInfo, 0, sizeof(trace_t) );
	collisionInfo.fraction = 1.0f;
}

/*
=================
hhProjectileCrawlerGrenade::CopyToModelProxy
=================
*/
void hhProjectileCrawlerGrenade::CopyToModelProxy()
{
	//rww - do this the right way with an inheriting entityDef
	//idDict args = spawnArgs;
	//args.Delete( "spawnclass" );
	//args.Delete( "name" );

	idDict args;
	idDict *setArgs;
	idStr str;

	if (gameLocal.isClient)
	{
		setArgs = &modelProxy->spawnArgs;
	}
	else
	{
		args.Clear();
		setArgs = &args;
	}

	setArgs->Set( "owner", GetName() );
	setArgs->SetVector( "origin", GetOrigin() );
	setArgs->SetMatrix( "rotation", GetAxis() );

	//copy the model over
	if (spawnArgs.GetString("model", "", str))
	{
		setArgs->Set("model", str.c_str());
		if (gameLocal.isClient && modelProxy.IsValid())
		{
			modelProxy->SetModel(str.c_str());
		}
	}

	//these are now taken care of in the ent def.
	/*
	setArgs->SetBool( "useCombatModel", true );
	setArgs->SetBool( "transferDamage", false );
	setArgs->SetBool( "solid", false );
	*/

	if (!gameLocal.isClient)
	{
		modelProxy = gameLocal.SpawnObject("projectile_crawler_proxy", &args);
	}

	if( modelProxy.IsValid() ) {
		modelProxy->Bind( this, true );
		modelProxy->CycleAnim( "idle", ANIMCHANNEL_ALL );
	}

	//for debugging
	//spawnArgs.CompareArgs(modelProxy->spawnArgs);

	modelProxyCopyDone = true;
}

/*
=================
hhProjectileCrawlerGrenade::SpawnModelProxy
=================
*/
void hhProjectileCrawlerGrenade::SpawnModelProxy() {
	if (!gameLocal.isClient)
	{
		CopyToModelProxy();
	}

	//Get rid of our model.  The modelProxy is our model now.
	SetModel( "" );
}

/*
=================
hhProjectileCrawlerGrenade::Ticker
=================
*/
void hhProjectileCrawlerGrenade::Ticker() {
	if( state == StateDying ) {
		if( modelProxy.IsValid() ) {
			modelProxy->SetDeformation(DEFORMTYPE_SCALE, modelScale.GetCurrentValue(gameLocal.time));
		}
	}
}

/*
================
hhProjectileCrawlerGrenade::Launch
================
*/
void hhProjectileCrawlerGrenade::Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower ) {
	hhProjectile::Launch( start, axis, pushVelocity, timeSinceFire, launchPower, dmgPower );

	if( modelProxy.IsValid() ) {
		modelProxy->CycleAnim( "flight", ANIMCHANNEL_ALL );
	}

	float delayBeforeDying = spawnArgs.GetFloat( "delayBeforeDying" );
	float fuse = spawnArgs.GetFloat( "fuse" );

	inflateDuration = SEC2MS( hhMath::ClampFloat(0.0f, fuse, fuse - delayBeforeDying) );

	PostEventSec( &EV_DyingState, delayBeforeDying );
	PostEventSec( &EV_DeadState, fuse );
}

/*
================
hhProjectileCrawlerGrenade::SpawnFlyFx
================
*/
void hhProjectileCrawlerGrenade::SpawnFlyFx() {
	if( modelProxy.IsValid() ) {
		modelProxy->BroadcastFxInfoAlongBonePrefix( &spawnArgs, "fx_fly", "joint_legStub" );
	}
}

/*
=================
hhProjectileCrawlerGrenade::Hide
=================
*/
void hhProjectileCrawlerGrenade::Hide() {
	hhProjectile::Hide();

	if( modelProxy.IsValid() ) {
		modelProxy->Hide();
	}
}

/*
=================
hhProjectileCrawlerGrenade::Show
=================
*/
void hhProjectileCrawlerGrenade::Show() {
	hhProjectile::Show();

	if( modelProxy.IsValid() ) {
		modelProxy->Show();
	}
}

/*
=================
hhProjectileCrawlerGrenade::RemoveProjectile
=================
*/
void hhProjectileCrawlerGrenade::RemoveProjectile( const int removeDelay ) {
	hhProjectile::RemoveProjectile( removeDelay );

	if( modelProxy.IsValid() ) {
		modelProxy->PostEventMS( &EV_Remove, removeDelay );
	}
}

/*
=================
hhProjectileCrawlerGrenade::Event_ApplyExpandWound
=================
*/
void hhProjectileCrawlerGrenade::Event_ApplyExpandWound() {
	trace_t trace;

	if( !modelProxy.IsValid() || !modelProxy->GetCombatModel() ) {
		return;
	}

	idBounds clipBounds( modelProxy->GetRenderEntity()->bounds );
	idVec3 traceEnd = GetOrigin();
	idVec3 traceStart = traceEnd + hhUtils::RandomPointInShell( clipBounds.Expand(1.0f).GetRadius(), clipBounds.Expand(2.0f).GetRadius() );
	idVec3 jointOrigin, localOrigin, localNormal;
	idMat3 jointAxis, axisTranspose;
	jointHandle_t jointHandle = INVALID_JOINT;

	CancelEvents( &EV_ApplyExpandWound );
	PostEventSec( &EV_ApplyExpandWound, spawnArgs.GetFloat("expandWoundDelay") );

	if( !gameLocal.clip.TracePoint(trace, traceStart, traceEnd, modelProxy->GetCombatModel()->GetContents(), NULL) ) {
		return;
	}
	
	if( trace.c.entityNum != entityNumber ) {//Make sure we hit ourselves
		return;
	}
			
	modelProxy->AddDamageEffect( trace, vec3_zero, spawnArgs.GetString("def_expandDamage"), (!fl.networkSync || netSyncPhysics) );
}

/*
=================
hhProjectileCrawlerGrenade::Event_Collision_Bounce
=================
*/
void hhProjectileCrawlerGrenade::Event_Collision_Bounce( const trace_t* collision, const idVec3 &velocity ) {
	static const float minCollisionVelocity = 20.0f;
	static const float maxCollisionVelocity = 90.0f;

	StopSound( SND_CHANNEL_BODY, true );

	// Velocity in normal direction
	float len = velocity * -collision->c.normal;

	if( collision->fraction < VECTOR_EPSILON || len < minCollisionVelocity ) {
		idThread::ReturnInt( 0 );
		return;
	}

	StartSound( "snd_bounce", SND_CHANNEL_BODY, 0, true, NULL );
	float volume = hhUtils::CalculateSoundVolume( len, minCollisionVelocity, maxCollisionVelocity );
	HH_SetSoundVolume( volume, SND_CHANNEL_BODY );

	BounceSplat( GetOrigin(), -collision->c.normal );

	SIMDProcessor->Memcpy( &collisionInfo, collision, sizeof(trace_t) );
	collisionInfo.fraction = 0.0f;//Sometimes fraction == 1.0f

	physicsObj.SetAngularVelocity( 0.5f*physicsObj.GetAngularVelocity() );

	idThread::ReturnInt( 0 );
}

/*
=================
hhProjectileCrawlerGrenade::Event_Collision_DisturbLiquid
=================
*/
void hhProjectileCrawlerGrenade::Event_Collision_DisturbLiquid( const trace_t* collision, const idVec3 &velocity ) {
	CancelActivates();
	EnterDeadState();

	hhProjectile::Event_Collision_DisturbLiquid( collision, velocity );
}

//================
//hhProjectileCrawlerGrenade::Save
//================
void hhProjectileCrawlerGrenade::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( modelScale.GetStartTime() );	// idInterpolate<float>
	savefile->WriteFloat( modelScale.GetDuration() );
	savefile->WriteFloat( modelScale.GetStartValue() );
	savefile->WriteFloat( modelScale.GetEndValue() );
	savefile->WriteInt( inflateDuration );
	savefile->WriteInt( state );
	savefile->WriteTrace( collisionInfo );
	modelProxy.Save( savefile );
}

//================
//hhProjectileCrawlerGrenade::Restore
//================
void hhProjectileCrawlerGrenade::Restore( idRestoreGame *savefile ) {
	float set;
	savefile->ReadFloat( set );	// idInterpolate<float>
	modelScale.SetStartTime( set );
	savefile->ReadFloat( set );
	modelScale.SetDuration( set );
	savefile->ReadFloat( set );
	modelScale.SetStartValue( set );
	savefile->ReadFloat( set );
	modelScale.SetEndValue( set );

	savefile->ReadInt( inflateDuration );
	savefile->ReadInt( reinterpret_cast<int &> ( state ) );

	savefile->ReadTrace( collisionInfo );
	modelProxy.Restore( savefile );
}

/*
=================
hhProjectileCrawlerGrenade::WriteToSnapshot
=================
*/
void hhProjectileCrawlerGrenade::WriteToSnapshot( idBitMsgDelta &msg ) const
{
	msg.WriteBits(modelProxyCopyDone, 1);
	msg.WriteBits(modelProxy.GetSpawnId(), 32);

	hhProjectile::WriteToSnapshot(msg);
}

/*
=================
hhProjectileCrawlerGrenade::ReadFromSnapshot
=================
*/
void hhProjectileCrawlerGrenade::ReadFromSnapshot( const idBitMsgDelta &msg )
{
	bool newModelProxyCopyDone = !!msg.ReadBits(1);
	if (modelProxy.SetSpawnId(msg.ReadBits(32)))
	{
		if (modelProxyCopyDone != newModelProxyCopyDone &&
			modelProxy.IsValid() &&
			modelProxy->IsType(hhGenericAnimatedPart::Type))
		{
			modelProxyCopyDone = newModelProxyCopyDone;
			CopyToModelProxy();
		}
	}

	hhProjectile::ReadFromSnapshot(msg);
}

/*
=================
hhProjectileCrawlerGrenade::EnterDyingState
=================
*/
void hhProjectileCrawlerGrenade::EnterDyingState() {
	state = StateDying;
	modelScale.Init( gameLocal.GetTime(), inflateDuration, modelScale.GetCurrentValue(gameLocal.GetTime()), spawnArgs.GetFloat("inflateScale") );

	StartSound( "snd_expand_screech", SND_CHANNEL_VOICE, 0, true, NULL );
	ProcessEvent( &EV_ApplyExpandWound );
}

/*
=================
hhProjectileCrawlerGrenade::EnterDeadState
=================
*/
void hhProjectileCrawlerGrenade::EnterDeadState() {
	state = StateDead;
	CancelEvents( &EV_ApplyExpandWound );
	StopSound( SND_CHANNEL_VOICE, true );
}

/*
=================
hhProjectileCrawlerGrenade::CancelActivates
=================
*/
void hhProjectileCrawlerGrenade::CancelActivates() {
	CancelEvents( &EV_DyingState );
	CancelEvents( &EV_DeadState );
}

/***********************************************************************

  hhProjectileStickyCrawlerGrenade
	
***********************************************************************/

CLASS_DECLARATION( hhProjectileCrawlerGrenade, hhProjectileStickyCrawlerGrenade )
	EVENT( EV_Collision_Flesh,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Metal,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_AltMetal,		hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Wood,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Stone,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Glass,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_CardBoard,		hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Tile,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Forcefield,		hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Pipe,			hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )
	EVENT( EV_Collision_Wallwalk,		hhProjectileStickyCrawlerGrenade::Event_Collision_Stick )

	EVENT( EV_Activate,					hhProjectileStickyCrawlerGrenade::Event_Activate )
END_CLASS

int hhProjectileStickyCrawlerGrenade::ProcessCollision( const trace_t* collision, const idVec3& velocity ) {
	idEntity* entHit = gameLocal.entities[ collision->c.entityNum ];

	//SAFE_REMOVE( fxFly );
	FreeLightDef();
	CancelEvents( &EV_Fizzle );

	//physicsObj.SetContents( 0 );
	physicsObj.PutToRest();

	surfTypes_t matterType = gameLocal.GetMatterType( entHit, collision->c.material, "hhProjectile::ProcessCollision" );
	return PlayImpactSound( gameLocal.FindEntityDefDict(spawnArgs.GetString("def_damage")), collision->endpos, matterType );
}

idMat3 hhProjectileStickyCrawlerGrenade::DetermineCollisionAxis( const idMat3& collisionAxis ) {
	return collisionAxis;
}

void hhProjectileStickyCrawlerGrenade::BindToCollisionObject( const trace_t* collision ) {
	if( !collision || collision->fraction > 1.0f ) {
		return;
	}

	//HUMANHEAD PCF rww 05/18/06 - wait until we receive bind info from the server
	if (gameLocal.isClient) {
		return;
	}
	//HUMANHEAD END

	idEntity*	pEntity = gameLocal.entities[collision->c.entityNum];
	HH_ASSERT( pEntity );	

	// HUMANHEAD PCF pdm 05-20-06: Check for some degenerate cases to combat the server hangs happening
	if (pEntity == this || this->IsBound()) {
		assert(0);	// Report any of these
		return;
	}
	// HUMANHEAD END

	jointHandle_t jointHandle = CLIPMODEL_ID_TO_JOINT_HANDLE( collision->c.id );
	if ( jointHandle != INVALID_JOINT ) {
		SetOrigin( collision->endpos );
		SetAxis( DetermineCollisionAxis( (-collision->c.normal).ToMat3()) );
		BindToJoint( pEntity, jointHandle, true );
	} else {
		SetOrigin( collision->endpos );
		SetAxis( DetermineCollisionAxis( (-collision->c.normal).ToMat3()) );
		Bind( pEntity, true );
	}
}

void hhProjectileStickyCrawlerGrenade::Event_Collision_Stick( const trace_t* collision, const idVec3 &velocity ) {
	if (proximityDetonateTrigger.GetEntity()) { //rww - don't allow this to be called more than once in a crawler grenade's lifetime
		return;
	}
	ProcessCollision( collision, velocity );

	BindToCollisionObject( collision );
	
	fl.ignoreGravityZones = true;
	SetGravity( idVec3(0.f, 0.f, 0.f) );
	spawnArgs.SetVector("gravity", idVec3(0.f, 0.f, 0.f) );

	BounceSplat( GetOrigin(), -collision->c.normal );

	idDict dict;

	dict.SetVector( "origin", GetOrigin() );
	//dict.SetMatrix( "rotation", GetAxis() );
	dict.Set( "target", name.c_str() );

	dict.SetVector( "mins", spawnArgs.GetVector("detonationMins", "-10 -10 -10") );
	dict.SetVector( "maxs", spawnArgs.GetVector("detonationMaxs", "10 10 10") );
	if (!gameLocal.isClient) {
		proximityDetonateTrigger = gameLocal.SpawnObject( spawnArgs.GetString("def_trigger"), &dict );
		proximityDetonateTrigger->Bind( this, true );
		if ( proximityDetonateTrigger->IsType( hhTrigger::Type ) ) {
			hhTrigger *trigger = static_cast<hhTrigger*>(proximityDetonateTrigger.GetEntity());
			if ( trigger && trigger->IsEncroached() ) {
				proximityDetonateTrigger->PostEventMS( &EV_Activate, 0, this );
			}
		}
	}

	if( modelProxy.IsValid() ) {
		modelProxy->CycleAnim( "idle", ANIMCHANNEL_ALL );
	}

	// CJR:  Added this from the normal crawler collision bounce code
	SIMDProcessor->Memcpy( &collisionInfo, collision, sizeof(trace_t) );
	collisionInfo.fraction = 0.0f;//Sometimes fraction == 1.0f

	idThread::ReturnInt( 1 );
}

void hhProjectileStickyCrawlerGrenade::Event_Activate( idEntity *pActivator ) {
	StartSound( "snd_expand_screech", SND_CHANNEL_VOICE, 0, true, NULL );
	PostEventSec( &EV_Explode, spawnArgs.GetFloat( "explodeDelay", "1.0" ) );
}

void hhProjectileStickyCrawlerGrenade::Explode( const trace_t* collision, const idVec3& velocity, int removeDelay ) {
	SAFE_REMOVE( fxFly );
	hhProjectile::Explode( &collisionInfo, velocity, removeDelay );
}


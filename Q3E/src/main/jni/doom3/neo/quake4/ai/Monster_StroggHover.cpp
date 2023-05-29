
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../vehicle/Vehicle.h"

#define MAX_MISSILE_JOINTS 4
#define MAX_HOVER_JOINTS 4
class rvMonsterStroggHover : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterStroggHover );

	rvMonsterStroggHover ( void );
	~rvMonsterStroggHover ( void );

	void					InitSpawnArgsVariables( void );
	void					Spawn				( void );
	void					Save				( idSaveGame *savefile ) const;
	void					Restore				( idRestoreGame *savefile );
		
	virtual void			Think				( void );
	
	virtual bool			Collide				( const trace_t &collision, const idVec3 &velocity );
	virtual	void			Damage				( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void			OnDeath				( void );
	virtual	void			DeadMove			( void );

	virtual bool			SkipImpulse			( idEntity *ent, int id );
	
	virtual int				FilterTactical		( int availableTactical );

	virtual void			GetDebugInfo		( debugInfoProc_t proc, void* userData );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					StartHeadlight					( void );
	void					StopHeadlight					( void );

protected:

//	rvAIAction				actionRocketAttack;
//	rvAIAction				actionBlasterAttack;
	rvAIAction				actionMGunAttack;
	rvAIAction				actionMissileAttack;
	rvAIAction				actionBombAttack;
	rvAIAction				actionStrafe;
	rvAIAction				actionCircleStrafe;
	
	
	virtual bool			CheckActions				( void );
	virtual void			OnEnemyChange				( idEntity* oldEnemy );
	virtual void			OnStartMoving				( void );

	virtual const char*		GetIdleAnimName				( void );

private:

	idEntityPtr<idEntity>	marker;
	idVec3					attackPosOffset;
	bool					inPursuit;
	int						holdPosTime;

	int						strafeTime;
	bool					strafeRight;
	bool					circleStrafing;
	float					deathPitch;
	float					deathRoll;
	float					deathPitchRate;
	float					deathYawRate;
	float					deathRollRate;
	float					deathSpeed;
	float					deathGrav;
	
	int						markerCheckTime;

	bool					MarkerPosValid					( void );
	void					TryStartPursuit					( void );
	void					Pursue							( void );
	void					CircleStrafe					( void );
	void					Evade							( bool left );

	int						mGunFireRate;
	int						missileFireRate;
	int						bombFireRate;
	int						nextMGunFireTime;
	int						nextMissileFireTime;
	int						nextBombFireTime;

	int						mGunMinShots;
	int						mGunMaxShots;
	int						missileMinShots;
	int						missileMaxShots;
	int						bombMinShots;
	int						bombMaxShots;

	int						shots;

	int						evadeDebounce;
	int						evadeDebounceRate;
	float					evadeChance;
	float					evadeSpeed;
	float					strafeSpeed;
	float					circleStrafeSpeed;

	rvClientEffectPtr		effectDust;
	rvClientEffectPtr		effectHover[MAX_HOVER_JOINTS];
	rvClientEffectPtr		effectHeadlight;

	jointHandle_t			jointDust;
	int						numHoverJoints;
	jointHandle_t			jointHover[MAX_HOVER_JOINTS];
	jointHandle_t			jointBomb;
	jointHandle_t			jointMGun;
	int						numMissileJoints;
	jointHandle_t			jointMissile[MAX_MISSILE_JOINTS];
	jointHandle_t			jointHeadlight;
	jointHandle_t			jointHeadlightControl;

	renderLight_t			renderLight;
	int						lightHandle;	
//	bool					lightOn;

	void					DoNamedAttack					( const char* attackName, jointHandle_t joint );

	void					UpdateLightDef					( void );

	bool					CheckAction_Strafe				( rvAIAction* action, int animNum );
	bool					CheckAction_CircleStrafe		( rvAIAction* action, int animNum );
	bool					CheckAction_BombAttack			( rvAIAction* action, int animNum );
	//virtual bool			CheckAction_EvadeLeft			( rvAIAction* action, int animNum );
	//virtual bool			CheckAction_EvadeRight			( rvAIAction* action, int animNum );

//	stateResult_t			State_Torso_BlasterAttack		( const stateParms_t& parms );
//	stateResult_t			State_Torso_RocketAttack		( const stateParms_t& parms );
	stateResult_t			State_Torso_MGunAttack			( const stateParms_t& parms );
	stateResult_t			State_Torso_MissileAttack		( const stateParms_t& parms );
	stateResult_t			State_Torso_BombAttack			( const stateParms_t& parms );
//	stateResult_t			State_Torso_EvadeLeft			( const stateParms_t& parms );
//	stateResult_t			State_Torso_EvadeRight			( const stateParms_t& parms );
	stateResult_t			State_Torso_Strafe				( const stateParms_t& parms );
	stateResult_t			State_Torso_CircleStrafe		( const stateParms_t& parms );
	stateResult_t			State_CircleStrafe				( const stateParms_t& parms );
	stateResult_t			State_DeathSpiral				( const stateParms_t& parms );
	stateResult_t			State_Pursue					( const stateParms_t& parms );

	CLASS_STATES_PROTOTYPE ( rvMonsterStroggHover );
};

CLASS_DECLARATION( idAI, rvMonsterStroggHover )
END_CLASS

/*
================
rvMonsterStroggHover::rvMonsterStroggHover
================
*/
rvMonsterStroggHover::rvMonsterStroggHover ( ) {
	effectDust  = NULL;
	for ( int i = 0; i < MAX_HOVER_JOINTS; i++ ) {
		effectHover[i] = NULL;
	}
	effectHeadlight = NULL;
	
	shots		= 0;
	strafeTime	= 0;
	strafeRight = false;
	circleStrafing = false;
	evadeDebounce = 0;
	deathPitch	= 0;
	deathRoll	= 0;
	deathPitchRate	= 0;
	deathYawRate	= 0;
	deathRollRate	= 0;
	deathSpeed	= 0;
	deathGrav	= 0;

	markerCheckTime = 0;

	marker		= NULL;
	attackPosOffset.Zero();
	inPursuit	= false;
	holdPosTime = 0;

	nextMGunFireTime	= 0;
	nextMissileFireTime = 0;
	nextBombFireTime	= 0;

	lightHandle	= -1;
}

/*
================
rvMonsterStroggHover::~rvMonsterStroggHover
================
*/
rvMonsterStroggHover::~rvMonsterStroggHover ( ) {
	if ( lightHandle != -1 ) {
		gameRenderWorld->FreeLightDef( lightHandle );
		lightHandle = -1;
	}
}

void rvMonsterStroggHover::InitSpawnArgsVariables( void )
{
	numHoverJoints = idMath::ClampInt(0,MAX_HOVER_JOINTS,spawnArgs.GetInt( "num_hover_joints", "1" ));
	for ( int i = 0; i < numHoverJoints; i++ ) {
		jointHover[i]	= animator.GetJointHandle ( spawnArgs.GetString( va("joint_hover%d",i+1) ) );
	}
	
	jointDust = animator.GetJointHandle ( spawnArgs.GetString( "joint_dust" ) );

	jointMGun		= animator.GetJointHandle ( spawnArgs.GetString( "joint_mgun" ) );
	numMissileJoints = idMath::ClampInt(0,MAX_MISSILE_JOINTS,spawnArgs.GetInt( "num_missile_joints", "1" ));
	for ( int i = 0; i < numMissileJoints; i++ ) {
		jointMissile[i]	= animator.GetJointHandle ( spawnArgs.GetString( va("joint_missile%d",i+1) ) );
	}
	jointBomb		= animator.GetJointHandle ( spawnArgs.GetString( "joint_bomb" ) );

	mGunFireRate = SEC2MS(spawnArgs.GetFloat( "mgun_fire_rate", "0.1" ));
	missileFireRate = SEC2MS(spawnArgs.GetFloat( "missile_fire_rate", "0.25" ));
	bombFireRate = SEC2MS(spawnArgs.GetFloat( "bomb_fire_rate", "0.5" ));

	mGunMinShots = spawnArgs.GetInt( "mgun_minShots", "20" );
	mGunMaxShots = spawnArgs.GetInt( "mgun_maxShots", "40" );
	missileMinShots = spawnArgs.GetInt( "missile_minShots", "4" );
	missileMaxShots = spawnArgs.GetInt( "missile_maxShots", "12" );
	bombMinShots = spawnArgs.GetInt( "bomb_minShots", "5" );
	bombMaxShots = spawnArgs.GetInt( "bomb_maxShots", "20" );

	evadeDebounceRate = SEC2MS(spawnArgs.GetFloat( "evade_rate", "0" ));
	evadeChance = spawnArgs.GetFloat( "evade_chance", "0.6" );
	evadeSpeed = spawnArgs.GetFloat( "evade_speed", "400" );
	strafeSpeed = spawnArgs.GetFloat( "strafe_speed", "500" );
	circleStrafeSpeed = spawnArgs.GetFloat( "circle_strafe_speed", "200" );

	//LIGHT
	jointHeadlight	= animator.GetJointHandle ( spawnArgs.GetString( "joint_light" ) );
	jointHeadlightControl = animator.GetJointHandle ( spawnArgs.GetString( "joint_light_control" ) );
}

void rvMonsterStroggHover::Hide( void )
{
	StopHeadlight();
	idAI::Hide();
}

void rvMonsterStroggHover::Show( void )
{
	idAI::Show();
	StartHeadlight();
}

void rvMonsterStroggHover::StartHeadlight( void )
{
	if ( jointHeadlight != INVALID_JOINT )
	{
		lightHandle = -1;
		if ( cvarSystem->GetCVarInteger( "com_machineSpec"	) > 1 && spawnArgs.GetString("mtr_light") ) {
			idVec3		color;
			//const char* temp;

			const idMaterial *headLightMaterial = declManager->FindMaterial( spawnArgs.GetString ( "mtr_light", "lights/muzzleflash" ), false );
			if ( headLightMaterial )
			{
				renderLight.shader	  = declManager->FindMaterial( spawnArgs.GetString ( "mtr_light", "lights/muzzleflash" ), false );
				renderLight.pointLight  = spawnArgs.GetBool( "light_pointlight", "1" );
// RAVEN BEGIN
// dluetscher: added detail levels to render lights
				renderLight.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// RAVEN END
				spawnArgs.GetVector( "light_color", "0 0 0", color );
				renderLight.shaderParms[ SHADERPARM_RED ]		= color[0];
				renderLight.shaderParms[ SHADERPARM_GREEN ]		= color[1];
				renderLight.shaderParms[ SHADERPARM_BLUE ]		= color[2];
				renderLight.shaderParms[ SHADERPARM_TIMESCALE ] = 1.0f;

				renderLight.lightRadius[0] = renderLight.lightRadius[1] = 
					renderLight.lightRadius[2] = (float)spawnArgs.GetInt( "light_radius" );

				if ( !renderLight.pointLight ) {
					renderLight.target = spawnArgs.GetVector( "light_target" );
					renderLight.up	   = spawnArgs.GetVector( "light_up" );
					renderLight.right  = spawnArgs.GetVector( "light_right" );
					renderLight.end	   = spawnArgs.GetVector( "light_target" );;
				}

				//lightOn = spawnArgs.GetBool( "start_on", "1" );

				lightHandle = gameRenderWorld->AddLightDef( &renderLight );
			}
		}
		// Hide flare surface if there is one
		/*
		temp = spawnArgs.GetString ( "light_flaresurface", "" );
		if ( temp && *temp ) {
			parent->ProcessEvent ( &EV_HideSurface, temp );
		}
		*/

		// Sounds shader when turning light
		//spawnArgs.GetString ( "snd_on", "", soundOn );
		
		// Sound shader when turning light off
		//spawnArgs.GetString ( "snd_off", "", soundOff);
		
		UpdateLightDef ( );
	}
}

void rvMonsterStroggHover::StopHeadlight( void )
{
	if ( lightHandle != -1 ) {
		gameRenderWorld->FreeLightDef ( lightHandle );
		lightHandle = -1;
	}
	memset ( &renderLight, 0, sizeof(renderLight) );
}
/*
================
rvMonsterStroggHover::Spawn
================
*/
void rvMonsterStroggHover::Spawn ( void ) {
//	actionRocketAttack.Init ( spawnArgs, "action_rocketAttack", "Torso_RocketAttack", AIACTIONF_ATTACK );
//	actionBlasterAttack.Init ( spawnArgs, "action_blasterAttack", "Torso_BlasterAttack", AIACTIONF_ATTACK );
	actionMGunAttack.Init ( spawnArgs, "action_mGunAttack", "Torso_MGunAttack", AIACTIONF_ATTACK );
	actionMissileAttack.Init ( spawnArgs, "action_missileAttack", "Torso_MissileAttack", AIACTIONF_ATTACK );
	actionBombAttack.Init ( spawnArgs, "action_bombAttack", "Torso_BombAttack", AIACTIONF_ATTACK );

	actionStrafe.Init  ( spawnArgs, "action_strafe", "Torso_Strafe",	0 );
	actionCircleStrafe.Init  ( spawnArgs, "action_circleStrafe", "Torso_CircleStrafe",	AIACTIONF_ATTACK );
	
	InitSpawnArgsVariables();

	evadeDebounce = 0;

	numHoverJoints = idMath::ClampInt(0,MAX_HOVER_JOINTS,spawnArgs.GetInt( "num_hover_joints", "1" ));
	for ( int i = 0; i < numHoverJoints; i++ ) {
		if ( jointHover[i] != INVALID_JOINT ) {
			effectHover[i] = PlayEffect ( "fx_hover", jointHover[i], true );
		}
	}
	
	if ( !marker ) {
		marker = gameLocal.SpawnEntityDef( "target_null" );
	}

	//LIGHT
	StopHeadlight();
	StartHeadlight();

	if ( jointHeadlight != INVALID_JOINT )
	{
		effectHeadlight = PlayEffect( "fx_headlight", jointHeadlight, true );
	}
}

/*
================
rvMonsterStroggHover::GetDebugInfo
================
*/
void rvMonsterStroggHover::GetDebugInfo( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
	
	proc ( "rvMonsterStroggHover", "action_mGunAttack",	aiActionStatusString[actionMGunAttack.status], userData );
	proc ( "rvMonsterStroggHover", "action_missileAttack",aiActionStatusString[actionMissileAttack.status], userData );
	proc ( "rvMonsterStroggHover", "action_bombAttack",	aiActionStatusString[actionBombAttack.status], userData );
	proc ( "rvMonsterStroggHover", "action_strafe",		aiActionStatusString[actionStrafe.status], userData );
	proc ( "rvMonsterStroggHover", "action_circleStrafe",aiActionStatusString[actionCircleStrafe.status], userData );

	proc ( "rvMonsterStroggHover", "inPursuit",			inPursuit?"true":"false", userData );
	proc ( "rvMonsterStroggHover", "marker",			(!inPursuit||marker==NULL)?"0 0 0":va("%f %f %f",marker->GetPhysics()->GetOrigin().x,marker->GetPhysics()->GetOrigin().y,marker->GetPhysics()->GetOrigin().z), userData );
	proc ( "rvMonsterStroggHover", "holdPosTime",		va("%d",holdPosTime), userData );
	
	proc ( "rvMonsterStroggHover", "circleStrafing",	circleStrafing?"true":"false", userData );
	proc ( "rvMonsterStroggHover", "strafeRight",		strafeRight?"true":"false", userData );
	proc ( "rvMonsterStroggHover", "strafeTime",		va("%d",strafeTime), userData );

	proc ( "rvMonsterStroggHover", "mGunFireRate",		va("%d",mGunFireRate), userData );
	proc ( "rvMonsterStroggHover", "missileFireRate",	va("%d",missileFireRate), userData );
	proc ( "rvMonsterStroggHover", "bombFireRate",		va("%d",bombFireRate), userData );

	proc ( "rvMonsterStroggHover", "mGunMinShots",		va("%d",mGunMinShots), userData );
	proc ( "rvMonsterStroggHover", "mGunMaxShots",		va("%d",mGunMaxShots), userData );
	proc ( "rvMonsterStroggHover", "missileMinShots",	va("%d",missileMinShots), userData );
	proc ( "rvMonsterStroggHover", "missileMaxShots",	va("%d",missileMaxShots), userData );
	proc ( "rvMonsterStroggHover", "bombMinShots",		va("%d",bombMinShots), userData );
	proc ( "rvMonsterStroggHover", "bombMaxShots",		va("%d",bombMaxShots), userData );
	proc ( "rvMonsterStroggHover", "nextMGunFireTime",	va("%d",nextMGunFireTime), userData );
	proc ( "rvMonsterStroggHover", "nextMissileFireTime",va("%d",nextMissileFireTime), userData );
	proc ( "rvMonsterStroggHover", "nextBombFireTime",	va("%d",nextBombFireTime), userData );
	proc ( "rvMonsterStroggHover", "shots",				va("%d",shots), userData );

	proc ( "rvMonsterStroggHover", "evadeDebounce",		va("%d",evadeDebounce), userData );
	proc ( "rvMonsterStroggHover", "evadeDebounceRate",	va("%d",evadeDebounceRate), userData );
	proc ( "rvMonsterStroggHover", "evadeChance",		va("%g",evadeChance), userData );
	proc ( "rvMonsterStroggHover", "evadeSpeed",		va("%g",evadeSpeed), userData );
	proc ( "rvMonsterStroggHover", "strafeSpeed",		va("%g",strafeSpeed), userData );
	proc ( "rvMonsterStroggHover", "circleStrafeSpeed",	va("%g",circleStrafeSpeed), userData );
}

/*
=====================
rvMonsterStroggHover::UpdateLightDef
=====================
*/
void rvMonsterStroggHover::UpdateLightDef ( void ) {
	if ( jointHeadlight != INVALID_JOINT )
	{
		idVec3 origin;
		idMat3 axis;

		if ( jointHeadlightControl != INVALID_JOINT ) {
			idAngles jointAng;
			jointAng.Zero();
			jointAng.yaw = 10.0f * sin( ( (gameLocal.GetTime()%2000)-1000 ) / 1000.0f * idMath::PI );
			jointAng.pitch = 7.5f * sin( ( (gameLocal.GetTime()%4000)-2000 ) / 2000.0f * idMath::PI );

			animator.SetJointAxis( jointHeadlightControl, JOINTMOD_WORLD, jointAng.ToMat3() );
		}

		GetJointWorldTransform ( jointHeadlight, gameLocal.time, origin, axis );
	
		//origin += (localOffset * axis);
		
		// Include this part in the total bounds 
		// FIXME: bounds are local
		//parent->AddToBounds ( worldOrigin );
		//UpdateOrigin ( );
		
		if ( lightHandle != -1 ) {
			renderLight.origin = origin;
			renderLight.axis   = axis;	
			
			gameRenderWorld->UpdateLightDef( lightHandle, &renderLight );	
		}
	}
}

/*
================
rvMonsterStroggHover::Think
================
*/
void rvMonsterStroggHover::Think ( void ) {
	idAI::Think ( );
	
	if ( !aifl.dead )
	{
		// If thinking we should play an effect on the ground under us
		if ( !fl.hidden && !fl.isDormant && (thinkFlags & TH_THINK ) && !aifl.dead ) {
			trace_t tr;
			idVec3	origin;
			idMat3	axis;
			
			// Project the effect 80 units down from the bottom of our bbox
			GetJointWorldTransform ( jointDust, gameLocal.time, origin, axis );
			
	// RAVEN BEGIN
	// ddynerman: multiple clip worlds
			gameLocal.TracePoint ( this, tr, origin, origin + axis[0] * (GetPhysics()->GetBounds()[0][2]+80.0f), CONTENTS_SOLID, this );
	// RAVEN END

			// Start the dust effect if not already started
			if ( !effectDust ) {
				effectDust = gameLocal.PlayEffect ( gameLocal.GetEffect ( spawnArgs, "fx_dust" ), tr.endpos, tr.c.normal.ToMat3(), true );
			}
			
			// If the effect is playing we should update its attenuation as well as its origin and axis
			if ( effectDust ) {
				effectDust->Attenuate ( 1.0f - idMath::ClampFloat ( 0.0f, 1.0f, (tr.endpos - origin).LengthFast ( ) / 127.0f ) );
				effectDust->SetOrigin ( tr.endpos );
				effectDust->SetAxis ( tr.c.normal.ToMat3() );
			}
			
			// If the hover effect is playing we can set its end origin to the ground
			/*
			if ( effectHover ) {
				effectHover->SetEndOrigin ( tr.endpos );
			}
			*/
		} else if ( effectDust ) {
			effectDust->Stop ( );
			effectDust = NULL;
		}

		//Try to circle strafe or pursue
		if ( circleStrafing )
		{
			CircleStrafe();
		}
		else if ( !inPursuit )
		{
			if ( !aifl.action && move.fl.done && !aifl.scripted )
			{
				if ( GetEnemy() )
				{
					if ( DistanceTo( GetEnemy() ) > 2000.0f 
						|| (GetEnemy()->GetPhysics()->GetLinearVelocity()*(GetEnemy()->GetPhysics()->GetOrigin()-GetPhysics()->GetOrigin())) > 1000.0f )
					{//enemy is far away or moving away from us at a pretty decent speed
						TryStartPursuit();
					}
				}
			}
		}
		else
		{
			Pursue();
		}

		//Dodge
		if ( !circleStrafing ) {
			if( combat.shotAtTime && gameLocal.GetTime() - combat.shotAtTime < 1000.0f ) {
				if ( nextBombFireTime < gameLocal.GetTime() - 3000 ) {
					if ( gameLocal.random.RandomFloat() > evadeChance ) {
						//40% chance of ignoring it - makes them dodge rockets less often but bullets more often?
						combat.shotAtTime = 0;
					} else if ( evadeDebounce < gameLocal.GetTime() ) {
						//ramps down from 400 to 100 over 1 second
						float speed = evadeSpeed - ((((float)(gameLocal.GetTime()-combat.shotAtTime))/1000.0f)*(evadeSpeed-(evadeSpeed*0.25f)));
						idVec3 evadeVel = viewAxis[1] * ((combat.shotAtAngle >= 0)?-1:1) * speed;
						evadeVel.z *= 0.5f;
						move.addVelocity += evadeVel;
						move.addVelocity.Normalize();
						move.addVelocity *= speed;
						/*
						if ( move.moveCommand < NUM_NONMOVING_COMMANDS ) {
							//just need to do it once?
							combat.shotAtTime = 0;
						}
						*/
						if ( evadeDebounceRate > 1 )
						{
							evadeDebounce = gameLocal.GetTime() + gameLocal.random.RandomInt( evadeDebounceRate ) + (ceil(((float)evadeDebounceRate)/2.0f));
						}
					}
				}
			}
		}

		//If using melee rush to nav to him, stop when we're close enough to attack
		if ( combat.tacticalCurrent == AITACTICAL_MELEE 
			&& move.moveCommand == MOVE_TO_ENEMY
			&& !move.fl.done
			&& nextBombFireTime < gameLocal.GetTime() - 3000
			&& enemy.fl.visible && DistanceTo( GetEnemy() ) < 2000.0f ) {
			StopMove( MOVE_STATUS_DONE );
			ForceTacticalUpdate();
		} else {
			//whenever we're not in the middle of something, force an update of our tactical
			if ( !aifl.action ) {
				if ( !aasFind ) {
					if ( move.fl.done ) {
						if ( !inPursuit && !circleStrafing ) {
							ForceTacticalUpdate();
						}
					}
				}
			}
		}
	}

	//update light
//	if ( lightOn ) {
		UpdateLightDef ( );
//	}
}

/*
============
rvMonsterStroggHover::OnStartMoving
============
*/
void rvMonsterStroggHover::OnStartMoving ( void ) {
	idAI::OnStartMoving();
	if ( move.moveCommand == MOVE_TO_ENEMY ) {
		move.range = combat.meleeRange;
	}
}

/*
================
rvMonsterStroggHover::Save
================
*/
void rvMonsterStroggHover::Save ( idSaveGame *savefile ) const {
	savefile->WriteInt ( shots );
	savefile->WriteInt ( strafeTime	);
	savefile->WriteBool ( strafeRight );
	savefile->WriteBool ( circleStrafing );
	savefile->WriteFloat ( deathPitch );
	savefile->WriteFloat ( deathRoll );
	savefile->WriteFloat ( deathPitchRate );
	savefile->WriteFloat ( deathYawRate );
	savefile->WriteFloat ( deathRollRate );
	savefile->WriteFloat ( deathSpeed );
	savefile->WriteFloat ( deathGrav );
	savefile->WriteInt	( markerCheckTime );

	savefile->WriteVec3( attackPosOffset );
	
//	actionRocketAttack.Save ( savefile );
//	actionBlasterAttack.Save ( savefile );
	actionMGunAttack.Save ( savefile );
	actionMissileAttack.Save ( savefile );
	actionBombAttack.Save ( savefile );
	actionStrafe.Save ( savefile );
	actionCircleStrafe.Save ( savefile );
	
	for ( int i = 0; i < numHoverJoints; i++ ) {
		effectHover[i].Save ( savefile );
	}
	
	effectDust.Save ( savefile );
	effectHeadlight.Save ( savefile );

	marker.Save( savefile );
	savefile->WriteBool ( inPursuit );
	savefile->WriteInt ( holdPosTime );
	savefile->WriteInt ( nextMGunFireTime );
	savefile->WriteInt ( nextMissileFireTime );
	savefile->WriteInt ( nextBombFireTime );

	savefile->WriteRenderLight ( renderLight );
	savefile->WriteInt ( lightHandle );

	savefile->WriteInt( evadeDebounce );
}

/*
================
rvMonsterStroggHover::Restore
================
*/
void rvMonsterStroggHover::Restore ( idRestoreGame *savefile ) {
	savefile->ReadInt ( shots );
	savefile->ReadInt ( strafeTime	);
	savefile->ReadBool ( strafeRight );
	savefile->ReadBool ( circleStrafing );
	savefile->ReadFloat ( deathPitch );
	savefile->ReadFloat ( deathRoll );
	savefile->ReadFloat ( deathPitchRate );
	savefile->ReadFloat ( deathYawRate );
	savefile->ReadFloat ( deathRollRate );
	savefile->ReadFloat ( deathSpeed );
	savefile->ReadFloat ( deathGrav );
	savefile->ReadInt	( markerCheckTime );

	savefile->ReadVec3( attackPosOffset );
	
//	actionRocketAttack.Restore ( savefile );
//	actionBlasterAttack.Restore ( savefile );
	actionMGunAttack.Restore ( savefile );
	actionMissileAttack.Restore ( savefile );
	actionBombAttack.Restore ( savefile );
	actionStrafe.Restore ( savefile );
	actionCircleStrafe.Restore ( savefile );
	
	InitSpawnArgsVariables();
	//NOTE: if the def file changes the the number of numHoverJoints, this will be BAD...
	for ( int i = 0; i < numHoverJoints; i++ ) {
		effectHover[i].Restore ( savefile );
	}
	
	effectDust.Restore ( savefile );
	effectHeadlight.Restore ( savefile );

	marker.Restore( savefile );
	savefile->ReadBool ( inPursuit );
	savefile->ReadInt ( holdPosTime );
	savefile->ReadInt ( nextMGunFireTime );
	savefile->ReadInt ( nextMissileFireTime );
	savefile->ReadInt ( nextBombFireTime );

	savefile->ReadRenderLight ( renderLight );
	savefile->ReadInt ( lightHandle );
	if ( lightHandle != -1 ) {
		//get the handle again as it's out of date after a restore!
		lightHandle = gameRenderWorld->AddLightDef( &renderLight );
	}

	savefile->ReadInt ( evadeDebounce );
}

/*
================
rvMonsterStroggHover::Collide
================
*/
bool rvMonsterStroggHover::Collide( const trace_t &collision, const idVec3 &velocity ) {
	if ( aifl.dead ) { 
		StopHeadlight();
		//stop headlight
		if ( effectHeadlight ) {
			effectHeadlight->Stop ( );
			effectHeadlight = NULL;
		}
		// Stop the crash & burn effect
		for ( int i = 0; i < numHoverJoints; i++ ) {
			if ( effectHover[i] ) {
				effectHover[i]->Stop ( );
				effectHover[i] = NULL;
			}
		}
		gameLocal.PlayEffect( spawnArgs, "fx_death", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		SetState ( "State_Remove" );
		return false;
	}
	return idAI::Collide( collision, velocity );
}

/*
================
rvMonsterStroggHover::Damage
================
*/
void rvMonsterStroggHover::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								  const char *damageDefName, const float damageScale, const int location ) {
	if ( attacker == this ) {
		return;
	}
	bool wasDead = aifl.dead;
	idAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );

	if ( !wasDead && aifl.dead ) {
		SetState( "State_DeathSpiral" );
	}
}
/*
================
rvMonsterStroggHover::OnDeath
================
*/
void rvMonsterStroggHover::OnDeath ( void ) {
	// Stop the dust effect
	if ( effectDust ) {
		effectDust->Stop ( );
		effectDust = NULL;
	}

	// Stop the hover effect
	for ( int i = 0; i < numHoverJoints; i++ ) {
		if ( effectHover[i] ) {
			effectHover[i]->Stop ( );
			effectHover[i] = NULL;
		}
	}

	idAI::OnDeath ( );
}

/*
=====================
rvMonsterStroggHover::DeadMove
=====================
*/
void rvMonsterStroggHover::DeadMove( void ) {
	DeathPush ( );
	physicsObj.UseVelocityMove( true );
	RunPhysics();
}

/*
=====================
rvMonsterStroggHover::SkipImpulse
=====================
*/
bool rvMonsterStroggHover::SkipImpulse( idEntity* ent, int id ) {	
	return ((ent==this) || (move.moveCommand==MOVE_RV_PLAYBACK));
}

/*
================
rvMonsterStroggHover::CheckAction_Strafe
================
*/
bool rvMonsterStroggHover::CheckAction_Strafe ( rvAIAction* action, int animNum ) {
	if ( inPursuit && !holdPosTime ) {
		return false;
	}

	if ( !enemy.fl.visible ) {
		return false;
	}

	if ( !enemy.fl.inFov ) {
		return false;
	}

	if ( !move.fl.done ) {
		return false;
	}
	if ( evadeDebounce >= gameLocal.GetTime() ) {
		return false;
	}

	if ( animNum != -1 && !TestAnimMove ( animNum ) ) {
		//well, at least try a new attack position
		if ( combat.tacticalCurrent == AITACTICAL_RANGED ) {
			combat.tacticalUpdateTime = 0;
		}
		return false;
	}
	return true;
}

/*
================
rvMonsterStroggHover::CheckAction_CircleStrafe
================
*/
bool rvMonsterStroggHover::CheckAction_CircleStrafe ( rvAIAction* action, int animNum ) {
	if ( inPursuit ) {
		return false;
	}

	if ( !enemy.fl.visible ) {
		return false;
	}

	if ( !enemy.fl.inFov ) {
		return false;
	}

	if ( !move.fl.done ) {
		return false;
	}

	return true;
}

/*
================
rvMonsterStroggHover::CheckAction_CircleStrafe
================
*/
bool rvMonsterStroggHover::CheckAction_BombAttack ( rvAIAction* action, int animNum ) {
	if ( !GetEnemy() || !enemy.fl.visible ) {
		return false;
	}
	/*
	if ( GetPhysics()->GetLinearVelocity().Length() < 200.0f ) {
		//not moving enough
		return false;
	}
	*/
	if ( GetEnemy()->GetPhysics()->GetLinearVelocity()*(GetPhysics()->GetOrigin()-GetEnemy()->GetPhysics()->GetOrigin()) >= 250.0f ) {
		//enemy is moving toward me, drop 'em!
		return true;
	}
	return false;
}

/*
================
rvMonsterStroggHover::Spawn
================
*/
bool rvMonsterStroggHover::CheckActions ( void ) {
	if ( PerformAction ( &actionCircleStrafe,  (checkAction_t)&rvMonsterStroggHover::CheckAction_CircleStrafe ) ) {
		return true;
	}

/*
	if ( PerformAction ( &actionRocketAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerSpecialAttack ) ) {
		return true;
	}

	if ( PerformAction ( &actionBlasterAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
		return true;
	}
*/
	if ( PerformAction ( &actionBombAttack, (checkAction_t)&rvMonsterStroggHover::CheckAction_BombAttack ) ) {
		return true;
	}

	if ( PerformAction ( &actionMissileAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
		return true;
	}

	if ( PerformAction ( &actionMGunAttack, (checkAction_t)&idAI::CheckAction_RangedAttack, &actionTimerRangedAttack ) ) {
		return true;
	}

	if ( idAI::CheckActions ( ) ) {
		return true;
	}

	if ( PerformAction ( &actionStrafe,  (checkAction_t)&rvMonsterStroggHover::CheckAction_Strafe ) ) {
		return true;
	}
	
	return false;
}

/*
================
rvMonsterStroggHover::OnEnemyChange
================
*/
void rvMonsterStroggHover::OnEnemyChange ( idEntity* oldEnemy ) {
	idAI::OnEnemyChange ( oldEnemy );
	
	if ( !enemy.ent ) {
		return;
	}
}

/*
================
rvMonsterStroggHover::GetIdleAnimName
================
*/
const char* rvMonsterStroggHover::GetIdleAnimName ( void ) {
	/*
	if ( move.moveType == MOVETYPE_FLY ) {
		return "flying_idle";
	}
	*/
	return idAI::GetIdleAnimName ( );
}	

/*
================
rvMonsterStroggHover::DoNamedAttack
================
*/
void rvMonsterStroggHover::DoNamedAttack ( const char* attackName, jointHandle_t joint ) {
	if ( joint != INVALID_JOINT ) {
		StartSound ( va("snd_%s_fire",attackName), SND_CHANNEL_ANY, 0, false, NULL );
		PlayEffect ( va("fx_%s_flash",attackName), joint );
		Attack ( attackName, joint, GetEnemy() );
	}
}

/*
================
rvMonsterStroggHover::FilterTactical
================
*/
int rvMonsterStroggHover::FilterTactical ( int availableTactical ) {
	availableTactical = idAI::FilterTactical( availableTactical );
	if ( circleStrafing || inPursuit ) {
		return 0;
	}
	if ( nextBombFireTime >= gameLocal.GetTime() ) {
        availableTactical &= ~(AITACTICAL_RANGED_BITS);
	} else if ( enemy.fl.visible ) {
        availableTactical &= ~AITACTICAL_MELEE_BIT;
	}
	return availableTactical;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterStroggHover )
//	STATE ( "Torso_BlasterAttack",		rvMonsterStroggHover::State_Torso_BlasterAttack )
//	STATE ( "Torso_RocketAttack",		rvMonsterStroggHover::State_Torso_RocketAttack )
	STATE ( "Torso_MGunAttack",			rvMonsterStroggHover::State_Torso_MGunAttack )
	STATE ( "Torso_MissileAttack",		rvMonsterStroggHover::State_Torso_MissileAttack )
	STATE ( "Torso_BombAttack",			rvMonsterStroggHover::State_Torso_BombAttack )
//	STATE ( "Torso_EvadeLeft",			rvMonsterStroggHover::State_Torso_EvadeLeft )
//	STATE ( "Torso_EvadeRight",			rvMonsterStroggHover::State_Torso_EvadeRight )
	STATE ( "Torso_Strafe",				rvMonsterStroggHover::State_Torso_Strafe )
	STATE ( "Torso_CircleStrafe",		rvMonsterStroggHover::State_Torso_CircleStrafe )
	STATE ( "State_CircleStrafe",		rvMonsterStroggHover::State_CircleStrafe )
	STATE ( "State_DeathSpiral",		rvMonsterStroggHover::State_DeathSpiral )
	STATE ( "State_Pursue",				rvMonsterStroggHover::State_Pursue )
END_CLASS_STATES

/*
================
rvMonsterStroggHover::State_Torso_MGunAttack
================
*/
stateResult_t rvMonsterStroggHover::State_Torso_MGunAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_LOOP,
		STAGE_WAITLOOP,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = gameLocal.random.RandomInt ( mGunMaxShots-mGunMinShots ) + mGunMinShots;
			return SRESULT_STAGE ( STAGE_LOOP );
			
		case STAGE_LOOP:
			DoNamedAttack( "mgun", jointMGun );
			nextMGunFireTime = gameLocal.GetTime() + mGunFireRate;
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( nextMGunFireTime <= gameLocal.GetTime() ) {
				if ( --shots <= 0 ) {
					ForceTacticalUpdate();
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStroggHover::State_Torso_MissileAttack
================
*/
stateResult_t rvMonsterStroggHover::State_Torso_MissileAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_LOOP,
		STAGE_WAITLOOP,
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = gameLocal.random.RandomInt ( missileMaxShots-missileMinShots ) + missileMinShots;
			return SRESULT_STAGE ( STAGE_LOOP );
			
		case STAGE_LOOP:
			DoNamedAttack( "missile", jointMissile[gameLocal.random.RandomInt(numMissileJoints)] );
			nextMissileFireTime = gameLocal.GetTime() + missileFireRate;
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( nextMissileFireTime <= gameLocal.GetTime() ) {
				if ( --shots <= 0 || enemy.range < (actionMissileAttack.minRange*0.75f) ) {
					//out of shots or enemy too close to safely keep launching rockets at
					ForceTacticalUpdate();
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStroggHover::State_Torso_BombAttack
================
*/
stateResult_t rvMonsterStroggHover::State_Torso_BombAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_LOOP,
		STAGE_WAITLOOP,
	};
	idVec3 vel = GetPhysics()->GetLinearVelocity();
	if ( vel.z < 150.0f ) {
		vel.z += 20.0f;
		physicsObj.UseVelocityMove( true );
		GetPhysics()->SetLinearVelocity( vel );
	}
	switch ( parms.stage ) {
		case STAGE_INIT:
			move.fly_offset = 800;//go up!
			shots = gameLocal.random.RandomInt ( bombMaxShots-bombMinShots ) + bombMinShots;
			if ( GetEnemy() ) {
				//if I'm not above him, give me a quick boost first
				float zDiff = GetPhysics()->GetOrigin().z-GetEnemy()->GetPhysics()->GetOrigin().z;
				if ( zDiff < 150.0f ) {
					idVec3 vel = GetPhysics()->GetLinearVelocity();
					vel.z += 200.0f;
					if ( zDiff < 0.0f ) {
						//even more if I'm below him!
						vel.z -= zDiff*2.0f;
					}
					physicsObj.UseVelocityMove( true );
					GetPhysics()->SetLinearVelocity( vel );
				}
			}
			if ( move.moveCommand == MOVE_TO_ATTACK ) {
				StopMove( MOVE_STATUS_DONE );
			}
			if ( combat.tacticalCurrent == AITACTICAL_RANGED ) {
				ForceTacticalUpdate();
			}
			if ( move.moveCommand == MOVE_NONE
				&& !inPursuit && !circleStrafing ) {
				MoveToEnemy();
			}
			return SRESULT_STAGE ( STAGE_LOOP );
			
		case STAGE_LOOP:
			DoNamedAttack( "bomb", jointBomb );
			nextBombFireTime = gameLocal.GetTime() + bombFireRate;
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( nextBombFireTime <= gameLocal.GetTime() ) {
				if ( --shots <= 0 ) {
					move.fly_offset = spawnArgs.GetFloat("fly_offset","250");
					ForceTacticalUpdate();
					return SRESULT_DONE;
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
	}
	return SRESULT_ERROR; 
}

/*
================
rvMonsterStroggHover::State_Torso_BlasterAttack
================
*/
/*
stateResult_t rvMonsterStroggHover::State_Torso_BlasterAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			shots = gameLocal.random.RandomInt ( 8 ) + 4;
			PlayAnim ( ANIMCHANNEL_TORSO, "blaster_1_preshoot", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_LOOP:
			PlayAnim ( ANIMCHANNEL_TORSO, "blaster_1_fire", 0 );
			return SRESULT_STAGE ( STAGE_WAITLOOP );
		
		case STAGE_WAITLOOP:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				if ( --shots <= 0 ) {
					PlayAnim ( ANIMCHANNEL_TORSO, "blaster_1_postshoot", 0 );
					return SRESULT_STAGE ( STAGE_WAITEND );
				}
				return SRESULT_STAGE ( STAGE_LOOP );
			}
			return SRESULT_WAIT;
		
		case STAGE_WAITEND:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 4 ) ) {
				ForceTacticalUpdate();
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;				
	}
	return SRESULT_ERROR; 
}
*/
/*
================
rvMonsterStroggHover::State_Torso_RocketAttack
================
*/
/*
stateResult_t rvMonsterStroggHover::State_Torso_RocketAttack ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_WAITSTART,
		STAGE_LOOP,
		STAGE_WAITLOOP,
		STAGE_WAITEND
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			//DisableAnimState ( ANIMCHANNEL_LEGS );
			PlayAnim ( ANIMCHANNEL_TORSO, "rocket_range_attack", parms.blendFrames );
			return SRESULT_STAGE ( STAGE_WAITSTART );
			
		case STAGE_WAITSTART:
			if ( AnimDone ( ANIMCHANNEL_TORSO, 0 ) ) {
				ForceTacticalUpdate();
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;
	}		

	return SRESULT_ERROR;
}
*/

void rvMonsterStroggHover::Evade ( bool left ) {
	idVec3 vel = GetPhysics()->GetLinearVelocity();
	vel += viewAxis[1] * (left?strafeSpeed:-strafeSpeed);
	physicsObj.UseVelocityMove( true );
	GetPhysics()->SetLinearVelocity( vel );
}

stateResult_t rvMonsterStroggHover::State_Torso_Strafe ( const stateParms_t& parms ) {
	//fixme: trace first for visibility & obstruction?
	if ( gameLocal.random.RandomFloat() > 0.5f ) {
		Evade( false );
	} else {
		Evade( true );
	}
	return SRESULT_DONE;
}

stateResult_t rvMonsterStroggHover::State_Torso_CircleStrafe ( const stateParms_t& parms ) {
	circleStrafing = true;
	return SRESULT_DONE;
}

bool rvMonsterStroggHover::MarkerPosValid ( void )
{
	//debouncer ftw
	if( markerCheckTime > gameLocal.GetTime() )	{
		return true;
	}

	markerCheckTime = gameLocal.GetTime() + 500 + (gameLocal.random.RandomFloat() * 500);

	trace_t trace;
	gameLocal.TracePoint( this, trace, marker.GetEntity()->GetPhysics()->GetOrigin(), marker.GetEntity()->GetPhysics()->GetOrigin(), GetPhysics()->GetClipMask(), NULL );
	if ( !(trace.c.contents&GetPhysics()->GetClipMask()) )
	{//not in solid
		gameLocal.TracePoint( this, trace, marker.GetEntity()->GetPhysics()->GetOrigin(), GetEnemy()->GetEyePosition(), MASK_SHOT_BOUNDINGBOX, GetEnemy() );
		idActor* enemyAct = NULL;
		rvVehicle* enemyVeh = NULL;
		if ( GetEnemy()->IsType( rvVehicle::GetClassType() ) ) {
			enemyVeh = static_cast<rvVehicle*>(GetEnemy());
		} else if ( GetEnemy()->IsType( idActor::GetClassType() ) ) {
			enemyAct = static_cast<idActor*>(GetEnemy());
		}
		idEntity* hitEnt = gameLocal.entities[trace.c.entityNum];
		idActor* hitAct = NULL;
		if ( hitEnt && hitEnt->IsType( idActor::GetClassType() ) ) {
			hitAct = static_cast<idActor*>(hitEnt);
		}
		if ( trace.fraction >= 1.0f 
			|| (enemyAct && enemyAct->IsInVehicle() && enemyAct->GetVehicleController().GetVehicle() == gameLocal.entities[trace.c.entityNum])
			|| (enemyVeh && hitAct && hitAct->IsInVehicle() && hitAct->GetVehicleController().GetVehicle() == enemyVeh) )
		{//have a clear LOS to enemy
			if ( PointReachableAreaNum( marker.GetEntity()->GetPhysics()->GetOrigin() ) )
			{//valid AAS there...
				return true;
			}
		}
	}
	return false;
}

void rvMonsterStroggHover::TryStartPursuit ( void )
{
	if ( GetEnemy() )
	{
		inPursuit = false;
		if ( !marker.GetEntity() ) {
			//wtf?!
			assert(0);
			return;
		}
		attackPosOffset.Set( gameLocal.random.CRandomFloat()*500.0f, gameLocal.random.CRandomFloat()*500.0f, 0.0f );
		if ( attackPosOffset.Length() < 150.0f )
		{
			attackPosOffset.Normalize();
			attackPosOffset *= 150.0f;
		}
		attackPosOffset.z = (gameLocal.random.CRandomFloat()*30.0f)+50.0f + move.fly_offset;
		marker.GetEntity()->GetPhysics()->SetOrigin( GetEnemy()->GetPhysics()->GetOrigin()+attackPosOffset );
		if ( MarkerPosValid() )
		{
			if ( MoveToEntity( marker ) )
			{
				inPursuit = true;
				holdPosTime = 0;
				SetState( "State_Pursue" );
			}
		}
	}
}

void rvMonsterStroggHover::Pursue ( void )
{
	if ( marker.GetEntity() && GetEnemy() )
	{
		marker.GetEntity()->GetPhysics()->SetOrigin( GetEnemy()->GetPhysics()->GetOrigin()+attackPosOffset );
		if ( DebugFilter(ai_debugMove) ) {
			gameRenderWorld->DebugAxis( marker.GetEntity()->GetPhysics()->GetOrigin(), marker.GetEntity()->GetPhysics()->GetAxis() );
		}
		if ( MarkerPosValid() )
		{
			bool breakOff = false;
			if ( move.fl.done )
			{//even once get there, hold that position for a while...
				if ( holdPosTime && holdPosTime > gameLocal.GetTime() )
				{//held this position long enough
					breakOff = true;
				}
				else
				{
					if ( !holdPosTime )
					{//just got there, hold position for a bit
						holdPosTime = gameLocal.random.RandomInt(2000)+3000 + gameLocal.GetTime();
					}
					if ( !MoveToEntity( marker ) )
					{
						breakOff = true;
					}
				}
			}
			if ( !breakOff )
			{
				return;
			}
		}
	}
	if ( !move.fl.done )
	{
		StopMove( MOVE_STATUS_DONE );
	}
	inPursuit = false;
}

stateResult_t rvMonsterStroggHover::State_Pursue ( const stateParms_t& parms ) {
	if ( inPursuit ) {
		// Perform actions along the way
		if ( UpdateAction ( ) ) {
			return SRESULT_WAIT;
		}
		return SRESULT_WAIT;
	}
	SetState( "State_Combat" );
	return SRESULT_DONE;
}

void rvMonsterStroggHover::CircleStrafe ( void )
{
	if ( !GetEnemy() || strafeTime < gameLocal.GetTime() || !enemy.fl.visible || !enemy.fl.inFov )
	{
		//FIXME: also stop if I bump into something
		circleStrafing = false;
		strafeTime = 0;
		SetState( "State_Combat" );
		return;
	}
	if ( !strafeTime )
	{
		strafeTime = gameLocal.GetTime() + 8000;
		//FIXME: try to see which side it clear?
		strafeRight = (gameLocal.random.RandomFloat()>0.5f);
	}

	idVec3 vel = GetPhysics()->GetLinearVelocity();
	idVec3 strafeVel = viewAxis[1] * (strafeRight?-circleStrafeSpeed:circleStrafeSpeed);
	strafeVel.z = 0.0f;
	vel += strafeVel;
	vel.Normalize();
	vel *= circleStrafeSpeed;
	physicsObj.UseVelocityMove( true );
	GetPhysics()->SetLinearVelocity( vel );
	TurnToward( GetEnemy()->GetPhysics()->GetOrigin() );
}

stateResult_t rvMonsterStroggHover::State_CircleStrafe ( const stateParms_t& parms ) {
	if ( circleStrafing ) {
		// Perform actions along the way
		if ( UpdateAction ( ) ) {
			return SRESULT_WAIT;
		}
		return SRESULT_WAIT;
	}
	SetState( "State_Combat" );
	return SRESULT_DONE;
}

stateResult_t rvMonsterStroggHover::State_DeathSpiral ( const stateParms_t& parms ) {
	enum { 
		STAGE_INIT,
		STAGE_SPIRAL
	};
	switch ( parms.stage ) {
		case STAGE_INIT:
			{
				disablePain = true;

				// Make sure all animation and attack states stop
				StopAnimState ( ANIMCHANNEL_TORSO );
				StopAnimState ( ANIMCHANNEL_LEGS );

				// Start the crash & burn effects
				for ( int i = 0; i < numHoverJoints; i++ ) {
					if ( jointHover[i] != INVALID_JOINT ) {
						PlayEffect ( "fx_hurt", jointHover[i], false );
						effectHover[i] = PlayEffect ( "fx_crash", jointHover[i], true );
					}
				}
				deathPitch = viewAxis.ToAngles()[0];
				deathRoll = viewAxis.ToAngles()[2];

				deathPitchRate = gameLocal.random.RandomFloat()*0.3f + 0.1f;
				deathYawRate = gameLocal.random.RandomFloat()*2.0f + 1.5f;
				deathRollRate = gameLocal.random.RandomFloat()*3.0f + 1.0f;
				deathSpeed = gameLocal.random.RandomFloat()*300.0f + 500.0f;
				deathGrav = gameLocal.random.RandomFloat()*6.0f + 6.0f;

				strafeRight = (gameLocal.random.RandomFloat()>0.5f);
				StopSound( SND_CHANNEL_HEART, false ); 
				StartSound ( "snd_crash", SND_CHANNEL_HEART, 0, false, NULL );
			}
			return SRESULT_STAGE ( STAGE_SPIRAL );
		case STAGE_SPIRAL:
			{
				move.current_yaw += (strafeRight?-deathYawRate:deathYawRate);
				deathPitch = idMath::ClampFloat( -90.0f, 90.0f, deathPitch+deathPitchRate );
				deathRoll += (strafeRight?deathRollRate:-deathRollRate);
				viewAxis = idAngles( deathPitch, move.current_yaw, deathRoll ).ToMat3();

				idVec3 vel = GetPhysics()->GetLinearVelocity();
				idVec3 strafeVel = viewAxis[0] * deathSpeed;
				strafeVel.z = 0;
				vel += strafeVel;
				vel.Normalize();
				vel *= deathSpeed;
				vel += GetPhysics()->GetGravity()/deathGrav;

				physicsObj.UseVelocityMove( true );

				physicsObj.SetLinearVelocity( vel );

				idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
				if( emitter ) {
					refSound.parms.frequencyShift += 0.025f;
					emitter->ModifySound ( SND_CHANNEL_HEART, &refSound.parms );
				}
			}
			return SRESULT_WAIT;
	}

	return SRESULT_ERROR;
}

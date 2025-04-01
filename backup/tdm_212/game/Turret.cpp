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

// Copyright (C) 2004 Id Software, Inc.
//
/*

  Turret.cpp

  Turret that shoots at enemies detected by a security camera

*/

#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "StimResponse/StimResponseCollection.h"

/***********************************************************************

  idTurret
	
***********************************************************************/

const idEventDef EV_Turret_RegisterCams( "<registerCams>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Turret_GetTurretState("getTurretState", EventArgs(), 'f', "Returns the turret's state. \n"
	"0 = passive: no active security cameras connected. 1 = unalerted, 2 = suspicious, 3 = fully alerted, 4 = power off, 5 = destroyed.");
const idEventDef EV_Turret_Attack("attack", 
	EventArgs('e', "enemy", "enemy to attack", 
			  'd', "ignoreCollisions", "whether to ignore obstacles in the way to the enemy when planning to attack"), 
	EV_RETURNS_VOID, "Direct the turret to manually attack the specified entity. Use disableManualAttack() to disable.");

const idEventDef EV_Turret_AttackPosition("attackPosition", 
	EventArgs('v', "targetPos", "position to attack", 
			  'd', "ignoreCollisions", "whether to ignore obstacles in the way to the target position when planning to attack. Recommended true for this event."), 
	EV_RETURNS_VOID, "Direct the turret to manually attack a position. Use disableManualAttack() to disable.");

const idEventDef EV_Turret_DisableManualAttack("disableManualAttack", EventArgs(), 
	EV_RETURNS_VOID, "Stop attacking the manually specified enemy or position and return to automatic target acquisition mode.");

CLASS_DECLARATION( idEntity, idTurret )
	EVENT( EV_Turret_RegisterCams,					idTurret::Event_RegisterCams )
	EVENT( EV_Turret_GetTurretState,				idTurret::Event_GetTurretState )
	EVENT( EV_Turret_Attack,						idTurret::Event_Attack )
	EVENT( EV_Turret_AttackPosition,				idTurret::Event_AttackPosition )
	EVENT( EV_Turret_DisableManualAttack,			idTurret::Event_DisableManualAttack )
	END_CLASS

/*
================
idTurret::Save
================
*/
void idTurret::Save( idSaveGame *savefile ) const {
	savefile->WriteInt(state);
	savefile->WriteInt(nextAttackCheck);
	savefile->WriteBool(intact);
	savefile->WriteBool(flinderized);
	savefile->WriteBool(m_bPower);
	savefile->WriteBool(m_bManualModeEnemy);
	savefile->WriteBool(m_bManualModePosition);
	savefile->WriteBool(m_bManualModeIgnoreCollisions);
	savefile->WriteAngles(m_startAngles);
	savefile->WriteAngles(targetAngles);
	savefile->WriteAngles(currentAngles);
	savefile->WriteFloat(speedVertical);
	savefile->WriteFloat(speedHorizontal);
	savefile->WriteInt(timeElapsed);
	savefile->WriteInt(prevTime);
	savefile->WriteInt(nextEnemiesUpdate);
	savefile->WriteVec3(attackPos);
	savefile->WriteFloat(nextSparkTime);
	savefile->WriteBool(sparksOn);
	savefile->WriteBool(sparksPowerDependent);
	savefile->WriteBool(sparksPeriodic);
	savefile->WriteFloat(sparksInterval);
	savefile->WriteFloat(sparksIntervalRand);
	enemy.Save(savefile);
	cameraDisplay.Save(savefile);
	sparks.Save(savefile);

	int i;
	savefile->WriteInt( cams.Num() );
	for( i = 0; i < cams.Num(); i++ ) {
		cams[ i ].Save( savefile );
	}
}

/*
================
idTurret::Restore
================
*/
void idTurret::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt(state);
	savefile->ReadInt(nextAttackCheck);
	savefile->ReadBool(intact);
	savefile->ReadBool(flinderized);
	savefile->ReadBool(m_bPower);
	savefile->ReadBool(m_bManualModeEnemy);
	savefile->ReadBool(m_bManualModePosition);
	savefile->ReadBool(m_bManualModeIgnoreCollisions);
	savefile->ReadAngles(m_startAngles);
	savefile->ReadAngles(targetAngles);
	savefile->ReadAngles(currentAngles);
	savefile->ReadFloat(speedVertical);
	savefile->ReadFloat(speedHorizontal);
	savefile->ReadInt(timeElapsed);
	savefile->ReadInt(prevTime);
	savefile->ReadInt(nextEnemiesUpdate);
	savefile->ReadVec3(attackPos);
	savefile->ReadFloat(nextSparkTime);
	savefile->ReadBool(sparksOn);
	savefile->ReadBool(sparksPowerDependent);
	savefile->ReadBool(sparksPeriodic);
	savefile->ReadFloat(sparksInterval);
	savefile->ReadFloat(sparksIntervalRand);

	enemy.Restore(savefile);
	cameraDisplay.Restore(savefile);
	sparks.Restore(savefile);

	int num, i;
	cams.Clear();
	savefile->ReadInt( num );
	cams.SetNum( num );
	for( i = 0; i < num; i++ ) {
		cams[ i ].Restore( savefile );
	}
}

/*
================
idTurret::Spawn
================
*/
void idTurret::Spawn( void )
{	
	state = -1;
	m_bManualModeEnemy = false;
	m_bManualModePosition = false;
	m_bManualModeIgnoreCollisions = false;
	intact = true;
	flinderized = false;
	sparksOn = false;
	sparksPeriodic = true;
	enemy = NULL;
	cameraDisplay = NULL;
	sparks = NULL;
	attackPos = idVec3{0,0,0};
	cams.Clear();
	nextAttackCheck = 0;
	nextEnemiesUpdate = 0;
	m_startAngles = GetPhysics()->GetAxis().ToAngles();
	targetAngles = idAngles{ 0,0,0 };
	currentAngles = m_startAngles;
	timeElapsed = 0;
	prevTime = gameLocal.time;
	nextSparkTime = 0;
	speedVertical			= spawnArgs.GetFloat("rotation_speed_vertical", "15");
	speedHorizontal			= spawnArgs.GetFloat("rotation_speed_horizontal", "45");
	sparksPowerDependent	= spawnArgs.GetBool("sparks_power_dependent", "1");
	sparksInterval			= spawnArgs.GetFloat("sparks_interval", "3");
	sparksIntervalRand		= spawnArgs.GetFloat("sparks_interval_rand", "2");
	m_bPower				= !spawnArgs.GetBool("start_off", "0");
	SetPower(m_bPower);

	//Register connected security cameras and camera displays
	PostEventMS(&EV_Turret_RegisterCams, 0);
}

/*
================
idTurret::Think
================
*/
void idTurret::Think( void )
{
	RunPhysics();

	if (thinkFlags & TH_THINK)
	{
		if ( health <= 0 && fl.takedamage )
		{
			BecomeInactive( TH_THINK );
			return;
		}

		UpdateState();
		UpdateEnemies();

		timeElapsed			= gameLocal.time - prevTime;
		prevTime			= gameLocal.time;

		//check for new enemies
		if( gameLocal.time > nextEnemiesUpdate )
		{
			UpdateEnemies();
			nextEnemiesUpdate = gameLocal.time + 200;
		}

		//if an enemy is available (or an attack position has been manually selected), try to turn towards it 
		if( enemy.GetEntity() )
		{
			attackPos		= GetEnemyPosition( enemy.GetEntity() );
			targetAngles	= GetAttackAngles( attackPos );
		}
		else if( m_bManualModePosition )
		{
			targetAngles	= GetAttackAngles( attackPos );
		}

		//otherwise return to starting angles
		else if( state != 3 )
		{
			targetAngles	= m_startAngles;
			nextAttackCheck	= 0;
		}

		currentAngles	= GetPhysics()->GetAxis().ToAngles();
		idAngles delta	= ( targetAngles - currentAngles ).Normalize180();

		//update turret angles if not fully pointed at the target. Accept up to 0.1° of divergence
		if( !currentAngles.Compare(targetAngles, 0.1f) )
		{	
			//adjust turret angles by one increment, based on speed * time elapsed since last check
			idAngles increment;
			increment.yaw = speedHorizontal * MS2SEC(timeElapsed) * ((delta.yaw > 0) - (delta.yaw < 0));
			increment.pitch = speedVertical * MS2SEC(timeElapsed) * ((delta.pitch > 0) - (delta.pitch < 0));
			currentAngles += increment;

			//if delta is smaller than one full increment, set the turret angles exactly to the target angles
			if( abs(delta.yaw) < abs(increment.yaw) )
				currentAngles.yaw = targetAngles.yaw;

			if( abs(delta.pitch) < abs(increment.pitch) )
				currentAngles.pitch = targetAngles.pitch;
	
			SetAngles( currentAngles );	//let the changes take effect
		}

		//try to attack the enemy, if there is one
		if( ( enemy.GetEntity() || m_bManualModePosition ) && gameLocal.time > nextAttackCheck )
		{
			if( nextAttackCheck == 0 )
			{
				//if this is the first attack attempt after getting alerted, add some delay first
				nextAttackCheck = gameLocal.time + SEC2MS( spawnArgs.GetFloat("attack_delay", "1.5") + gameLocal.random.RandomFloat() * spawnArgs.GetFloat("attack_delay_rand", "0.5"));
			}
			else if( CanAttack( enemy.GetEntity(), true) )
			{
				//attack
				ShootCannon(attackPos);
				nextAttackCheck = gameLocal.time + SEC2MS(spawnArgs.GetFloat("attack_interval", "1.5") + gameLocal.random.RandomFloat() * spawnArgs.GetFloat("attack_interval_rand", "0.5"));
			}
			else
			{
				//attack attempt was unsuccessful: add some delay before the next attempt
				nextAttackCheck = gameLocal.time + SEC2MS( spawnArgs.GetFloat("attack_delay", "1.5") + gameLocal.random.RandomFloat() * spawnArgs.GetFloat("attack_delay_rand", "0.5"));
			}
		}

		//if we are manually attacking an enemy and it has been removed or has died, disable manual mode
		if( m_bManualModeEnemy )
		{
			if( !enemy.GetEntity() 
			|| ( enemy.GetEntity()->fl.takedamage && enemy.GetEntity()->health <= 0  ) )
				Event_DisableManualAttack();
		}
	}

	if ( state == STATE_DEAD && thinkFlags & TH_UPDATEPARTICLES )
		TriggerSparks(); // Trigger spark effect

	Present();
}

void idTurret::UpdateState( void )
{
	int newState = STATE_PASSIVE;

	//if the turret is destroyed, set STATE_DEAD
	//otherwise alert state is always 3 if in manual mode and there is a valid target to attack
	//otherwise look for alerted security cameras and adapt to their state
	if( !intact )
	{
		newState = STATE_DEAD;
	}

	else if( m_bManualModeEnemy || m_bManualModePosition )
	{
		if( m_bManualModePosition || (enemy.GetEntity() && m_bManualModeEnemy ) )
			newState = STATE_ALERTED;

		else
			newState = STATE_IDLE;
	}

	else
	{
		for (int i = 0; i < cams.Num(); i++)
		{
			idSecurityCamera *cam = static_cast<idSecurityCamera*>( cams[i].GetEntity() );
			if ( !cam )
				continue;

			int camState = cam->GetSecurityCameraState();

			//has the security camera been disabled?
			if( camState > STATE_ALERTED )
				continue;

			//is this camera alerted?
			if( camState > newState )
				newState = camState;		
		}
	}

	if( state != newState )
	{
		state = newState;
		UpdateColors();

		//provide a script hook for scripts, i.e. to generate FX
		idStr thread = spawnArgs.GetString("script_state_update", "");
		if( thread != "" && thread != "-" )
			CallScriptFunctionArgs( thread, true, 0, "e", this );
	}
}

/*
================
idTurret::UpdateColors
================
*/
void idTurret::UpdateColors( void )
{
	idVec3 newColor;

	switch (state)
	{
		case STATE_PASSIVE:		newColor = spawnArgs.GetVector("color_passive", "1 1 1"); break;
		case STATE_IDLE:		newColor = spawnArgs.GetVector("color_idle", "1 1 1"); break;
		case STATE_SUSPICIOUS:	newColor = spawnArgs.GetVector("color_suspicious", "1 1 1"); break;
		case STATE_ALERTED:		newColor = spawnArgs.GetVector("color_alerted", "1 1 1"); break;
		default:				newColor = idVec3{ 1.0f, 1.0f, 1.0f }; break;
	}

	SetColor( newColor[0], newColor[1], newColor[2] );
}

/*
================
idTurret::UpdateEnemies
================
*/
void idTurret::UpdateEnemies(void)
{
	//do nothing if no cameras have been registered so far, the turret is in manual control mode or power is off
	if( cams.Num() == 0 || m_bManualModeEnemy || m_bManualModePosition || !m_bPower )
		return;

	idEntity *bestEnemy = NULL;
	bool canAttackBestEnemy, canAttackNewEnemy;

	for (int i = 0; i < cams.Num(); i++)
	{
		idSecurityCamera *cam = static_cast<idSecurityCamera*>( cams[i].GetEntity() );
		if( !cam )
			continue;

		//the security camera has to be alerted and fully operational
		int camState = cam->GetSecurityCameraState();
		if( camState != 3 )
			continue;

		//look through all enemies of the camera to find the most suitable one
		for (int ii = 0; ii < cam->enemies.Num(); ii++)
		{
			idEntity* newEnemy = cam->enemies[ii].GetEntity();
			if( !newEnemy )
				continue;

			canAttackNewEnemy = CanAttack(newEnemy, false);

			//if we found no enemy yet, pick this first one that's found and then check the rest
			if( !bestEnemy )
			{
				bestEnemy = newEnemy;
				canAttackBestEnemy = canAttackNewEnemy;
				continue;
			}

			//if one enemy is attackable but the other isn't, pick the attackable one
			if( canAttackNewEnemy && !canAttackBestEnemy )
			{
				bestEnemy = newEnemy;
				canAttackBestEnemy = true;
			}

			else if( canAttackBestEnemy && !canAttackNewEnemy )
			{
				continue;
			}

			//otherwise decide based on distance
			else
			{
				float distNew	= ( GetPhysics()->GetOrigin() - GetEnemyPosition(newEnemy) ).Length();
				float distBest	= ( GetPhysics()->GetOrigin() - GetEnemyPosition(bestEnemy) ).Length();

				if( distNew < distBest )
				{
					bestEnemy = newEnemy;
					canAttackBestEnemy = canAttackNewEnemy;
				}
			}
		}
	}

	enemy = bestEnemy;	//allow "enemy" to be NULL
}

/*
================
idTurret::Event_RegisterCams
================
*/
void idTurret::Event_RegisterCams( void )
{
	int i;

	//One frame post-spawn, find all idSecurityCameras and add them to idList cams
	//The cameras should be targeted by the turret.
	for (i = 0; i < targets.Num(); i++)
	{
		idEntity* ent = targets[i].GetEntity();

		if( ent && ent->IsType(idSecurityCamera::Type) )
		{
			idEntityPtr<idEntity> ptr = static_cast<idEntity*>(ent);
			cams.AddUnique(ptr);
		}
	}

	//Remove registered cameras from the list of targets
	for (i = 0; i < cams.Num(); i++)
	{
		idSecurityCamera* cam = static_cast<idSecurityCamera*>( cams[i].GetEntity() );
		if (!cam)
			continue;

		for (int ii = 0; ii < targets.Num(); ii++)
		{
			if ( targets[ii].GetEntity() == cam )
			{
				targets.RemoveIndex(ii);
				break;
			}
		}
	}

	// Search entities for those who have a "cameraTarget" pointing to this turret.
	// One should be found, and set 'cameraDisplay' to that entity.
	for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if ( !ent )
			continue;	// skip past nulls in the index

		if ( ent == this )
			continue;	// skip yourself

		idEntity *ect = ent->cameraTarget;
		if ( ect )
		{
			if ( ect == this )
			{
				cameraDisplay = ent;
				break;
			}

			if ( cameraTarget == ect )
			{
				cameraDisplay = ent;
				ect->cameraFovX = cameraFovX;
				ect->cameraFovY = cameraFovY;
				break;
			}
		}
	}

	// If power to the turret starts off, hide ("switch off") the camera screen
	if( !m_bPower && cameraDisplay.GetEntity() )
		cameraDisplay.GetEntity()->Hide();
}

/*
================
idTurret::CanAttack
================
*/
bool idTurret::CanAttack( idEntity *ent, bool useCurrentAngles )
{
	//Perform checks to see whether the target can be attacked
	//allow NULL entities in case we have to fire at coordinates
	//is the enemy alive and awake?
	if( ent )
	{
		if( ent->IsType(idPlayer::Type) && ent->health <= 0  )
			return false;

		else if( ent->IsType(idAI::Type) )
		{
			idAI *ai = static_cast<idAI *>( ent );
			if( ai->AI_DEAD || ai->AI_KNOCKEDOUT )
				return false;
		}
	}

	//is the enemy visible?
	if( ent && ( ent->fl.invisible || ent->fl.notarget || ent->fl.hidden ) )
		return false;

	//is the enemy within range?
	float	maxFireRange	= spawnArgs.GetFloat("fire_range_max", "512");
	float	minFireRange	= spawnArgs.GetFloat("fire_range_min", "16");
	idVec3 enemyPos;
	if(ent)		enemyPos	= GetEnemyPosition( ent );
	else		enemyPos	= attackPos;

	float	distToPos		= (GetPhysics()->GetOrigin() - enemyPos).Length();
	if( distToPos < minFireRange || distToPos > maxFireRange )
		return false;

	//can the turret turn far enough to fire in this direction?
	float	fireTolerance	= spawnArgs.GetFloat("fire_tolerance", "15");
	idAngles attackAngles;
	if( useCurrentAngles )	attackAngles = GetPhysics()->GetAxis().ToAngles();
	else					attackAngles = GetAttackAngles(enemyPos);

	idVec3		launchPos		= GetPhysics()->GetOrigin() + ( spawnArgs.GetVector("projectile_offset", "0 0 0") * attackAngles.ToMat3() );
	idAngles	anglesToEnemy	= ( enemyPos - launchPos ).ToAngles();
	idAngles	anglesDiff		= ( anglesToEnemy - attackAngles ).Normalize180();

	if( fabs(anglesDiff.yaw) > fireTolerance || fabs(anglesDiff.pitch) > fireTolerance )
		return false;

	//only collisions left. Manual override?
	if( m_bManualModeIgnoreCollisions )
		return true;

	//perform a collision trace
	trace_t trace;
	idVec3	traceRadius		= spawnArgs.GetVector("projectile_trace_radius", "4 4 4");
	float	traceMask		= MASK_SHOT_BOUNDINGBOX;
	gameLocal.clip.TraceBounds(trace, launchPos, enemyPos, idBounds(-traceRadius, traceRadius), traceMask, this);

	//is the trace hitting something else than the desired enemy?
	idEntity *traceEnt = gameLocal.entities[trace.c.entityNum];

	if( trace.fraction < 1 && traceEnt && traceEnt != enemy.GetEntity() )
	{
		//if it is an AI and friendy fire is not wanted, check whether it's the enemy of at least one alerted camera
		if( traceEnt->IsType(idAI::Type) && spawnArgs.GetBool("friendly_fire", "1") )
		{
			bool friendlyToCam = true;

			for (int i = 0; i < cams.Num(); i++)
			{
				idSecurityCamera *cam = static_cast<idSecurityCamera*>( cams[i].GetEntity() );
				if( !cam )
					continue;

				if( cam->GetSecurityCameraState() == 3 && gameLocal.m_RelationsManager->GetRelNum( cam->team, traceEnt->team ) == -1 )
					friendlyToCam = false;
			}

			if( friendlyToCam )
				return false;
		}
		//otherwise check if it's attached to the enemy
		else if( ent && traceEnt->GetBindMaster() != ent )
		{
			return false;
		}
	}

	return true;
}

idVec3 idTurret::GetEnemyPosition( idEntity *ent )
{
	idVec3 enemyPos = attackPos;

	//do not update attackPos if a NULL entity has been passed and we are not manually attacking a position
	if( !ent && !m_bManualModePosition )
		return enemyPos;

	//do not update attackPos if the entity is invisble
	if( ent && ( ent->fl.invisible || ent->fl.notarget || ent->fl.hidden ) )
		return enemyPos;

	//use different attack position for players, AIs and non-actors
	if( ent && ent->IsType(idPlayer::Type) )
	{
		idPlayer *player = static_cast<idPlayer *>(ent);
		enemyPos = player->GetEyePosition() - idVec3{ 0,0,6 };
	}
	else if( ent )
	{
		enemyPos = ent->GetPhysics()->GetOrigin();

		if( ent->IsType(idAI::Type) )
		{
			idBounds enemyBounds	= ent->GetPhysics()->GetBounds();
			enemyPos.z				= enemyPos.z + 0.75 * ( enemyBounds[1][2] - enemyBounds[0][2] );	//aim for the upper body of an AI
		}
	}

	//predict motion of a moving enemy
	if( ent && spawnArgs.GetBool("predict_motion", "1") )
	{
		idVec3		launchPos		= GetPhysics()->GetOrigin() + ( spawnArgs.GetVector("projectile_offset", "0 0 0") * GetPhysics()->GetAxis() );
		idVec3		enemyVelocity	= ent->GetPhysics()->GetLinearVelocity();
		float		projectileSpeed	= spawnArgs.GetFloat("projectile_speed", "600");
		float		distToEnemy		= ( enemyPos - launchPos ).Length();

		//is an intercept possible or are the projectile and enemy speed too similar?
		//otherwise use a less accurate prediction method
		float	interceptTime = idPolynomial::GetInterceptTime( enemyVelocity, projectileSpeed, enemyPos, launchPos );

		if( interceptTime == 0 )
			interceptTime = distToEnemy / projectileSpeed;

		enemyPos += enemyVelocity * interceptTime;
	}

	return enemyPos;
}

idAngles idTurret::GetAttackAngles( idVec3 enemyPos )
{
	idAngles	currentAngles	= GetPhysics()->GetAxis().ToAngles();
	idAngles	anglesToEnemy	= ( enemyPos - GetPhysics()->GetOrigin() ).ToAngles().Normalize180();	

	//is this angle within our accepted vertical range?
	float	limitUp		= m_startAngles.pitch - spawnArgs.GetFloat("max_incline_up");
	float	limitDown	= m_startAngles.pitch + spawnArgs.GetFloat("max_incline_down");

	if		( anglesToEnemy.pitch < limitUp )	anglesToEnemy.pitch = limitUp;
	else if	( anglesToEnemy.pitch > limitDown )	anglesToEnemy.pitch = limitDown;

	return anglesToEnemy.Normalize180();
}

void idTurret::ShootCannon( idVec3 enemyPos )
{	
	//set launch parameters
	idVec3		launchPos		= GetPhysics()->GetOrigin() + ( spawnArgs.GetVector("projectile_offset", "0 0 0") * GetPhysics()->GetAxis() );

	//Note: the launch direction of the projectile will vary by a random number of degrees up to the value specified in this spawnarg.
	//Set to 0 for perfect accuracy.
	float		accuracy		= spawnArgs.GetFloat("accuracy", "0");
	idAngles	anglesToEnemy	= ( enemyPos - launchPos ).ToAngles().Normalize180();	

	idAngles launchAngles = idAngles{ 0,0,0 };
	launchAngles.pitch	= anglesToEnemy.pitch + ( 1 - 2 * gameLocal.random.RandomFloat() * accuracy );
	launchAngles.yaw	= anglesToEnemy.yaw + ( 1 - 2 * gameLocal.random.RandomFloat() * accuracy );
	launchAngles.roll	= anglesToEnemy.roll + ( 1 - 2 * gameLocal.random.RandomFloat() * accuracy );

	idVec3	projDir			= launchAngles.ToForward().Normalized();
	float	distToTarget	= ( enemyPos - launchPos ).Length();
	idVec3	projVel			= ( enemyPos - launchPos ) * ( spawnArgs.GetFloat("projectile_speed", "600") / distToTarget);

	//spawn and launch the projectile
	idDict	args;

	args.Set( "classname", spawnArgs.GetString("def_projectile", "atdm:turret01_projectile") );
	args.Set( "origin", launchPos.ToString() );
	args.Set( "velocity", projVel.ToString() );
	if( enemy.GetEntity() )
		args.Set( "enemy", enemy.GetEntity()->name.c_str() );

	idEntity *projectile = NULL;
	if( gameLocal.SpawnEntityDef(args, &projectile, false) && projectile )
		static_cast<idProjectile *>(projectile)->Launch( launchPos, projDir, idVec3{0.0f,0.0f,0.0f} );

	//try to rout enemies, if enabled
	RoutEnemies();

	//play sound and visual sfx
	StartSound( "snd_fire", 8, 0, false, NULL );

	//provide a script hook for scripts, i.e. to generate FX
	idStr thread = spawnArgs.GetString("script_fired", "");
	if( thread != "" && thread != "-" )
		CallScriptFunctionArgs( thread, true, 0, "e", this );
}

void idTurret::RoutEnemies( void )
{
	if( !enemy.GetEntity() || !enemy.GetEntity()->IsType(idAI::Type) )
		return;

	idAI *ai			= static_cast<idAI *>( enemy.GetEntity() );
	int rout			= spawnArgs.GetInt("rout_enemies", "0");
	int routRadius		= spawnArgs.GetInt("rout_radius", "0");

	if( rout == 1 || ( rout == 2 && !ai->GetEnemy() ) )
	{
		ai->fleeingEvent = true;
		ai->fleeingFrom = GetPhysics()->GetOrigin();
		ai->emitFleeBarks = true;

		if ( !ai->GetMemory().fleeing ) // grayman #3847 - only flee if not already fleeing
			ai->GetMind()->SwitchState("Flee");
	}

	if( routRadius <= 0 )	return;

	//also rout enemies in a radius
	idAI *newAI = NULL;	
	for ( newAI = gameLocal.spawnedAI.Next(); newAI != NULL; newAI = newAI->aiNode.Next() )
	{
		//is this AI close enough to the attacked enemy?
		if( ( newAI->GetPhysics()->GetOrigin() - ai->GetPhysics()->GetOrigin() ).Length() > routRadius)
			continue;

		//is this the enemy again?
		if( newAI == ai )
			continue;

		//is this an enemy of any security camera?
		bool enemyToCams = false;

		for (int i = 0; i < cams.Num(); i++)
		{
			idSecurityCamera *cam = static_cast<idSecurityCamera*>( cams[i].GetEntity() );
			if( !cam )
				continue;

			if( cam->GetSecurityCameraState() == 3 && gameLocal.m_RelationsManager->GetRelNum( cam->team, newAI->team ) == -1 )
				enemyToCams = true;
		}

		if( !enemyToCams )
			continue;

		//check whether rout settings are fulfilled
		if( rout == 1 || ( rout == 2 && !newAI->GetEnemy() ) )
		{
			ai->fleeingEvent = true;
			ai->fleeingFrom = GetPhysics()->GetOrigin();
			ai->emitFleeBarks = true;

			if ( !ai->GetMemory().fleeing ) // grayman #3847 - only flee if not already fleeing
				ai->GetMind()->SwitchState("Flee");
		}
	}
}

void idTurret::SetPower( bool newState )
{
	m_bPower = newState;

	if( newState == true )
	{
		Event_SetSkin( spawnArgs.GetString("skin_on", "turret_on") );
		StartSound( "snd_stationary", 7, 0, false, NULL );	//idle sound
		nextAttackCheck = 0;
		BecomeActive(TH_THINK);
	}
	else
	{
		Event_SetSkin( spawnArgs.GetString("skin_off", "turret_off") );
		StopSound(7, false);	//idle sound
		BecomeInactive(TH_THINK);
		enemy = NULL;
		UpdateState();
	}

	if ( cameraDisplay.GetEntity() )
	{
		if ( m_bPower )
			cameraDisplay.GetEntity()->Show();
		else
			cameraDisplay.GetEntity()->Hide();
	}
}

/*
================
idTurret::Activate - turn turret power on/off
================
*/
void idTurret::Activate( idEntity* activator )
{
	SetPower( !m_bPower );

	// handle trigger Responses and Signals
	TriggerResponse( activator, ST_TRIGGER );

	if ( RespondsTo(EV_Activate) || HasSignal(SIG_TRIGGER) )
	{
		Signal(SIG_TRIGGER);
		ProcessEvent(&EV_Activate, activator);
		TriggerGuis();
	}

	// handle sparks (post-destruction)
	if ( state == STATE_DEAD && spawnArgs.GetBool("sparks", "1") && sparksPowerDependent )
	{
		if ( m_bPower )
		{
			nextSparkTime = gameLocal.time;
			BecomeActive(TH_UPDATEPARTICLES);
		}
		else
		{
			BecomeInactive(TH_UPDATEPARTICLES);

			// toggle off looping particles
			if ( sparksOn && !sparksPeriodic )
			{
				idEntity *sparksEntity = sparks.GetEntity();

				sparksEntity->Activate(NULL);
				StopSound(6, false);	//sparks sound
				sparksOn = false;
			}
		}

		return;
	}
}

/*
================
idTurret::Damage

Modified version of idEntity::Damage that modifies damage from certain sources
================
*/
void idTurret::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
	const char *damageDefName, const float damageScale,	const int location, trace_t *tr)
{
	if ( !fl.takedamage )
		return;
	if ( !inflictor )
		inflictor = gameLocal.world;
	if ( !attacker )
		attacker = gameLocal.world;

	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName, true); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
		gameLocal.Error("Unknown damageDef '%s'\n", damageDefName);
	

	// check what is damaging the turret and adjust damage according to spawnargs
	idStr def = damageDefName;
	idStr inf = inflictor->GetEntityDefName();
	int damage = damageDef->GetInt( "damage" );
	float damage_mult = 1.0f;

	// has the mapper specified a spawnarg for a specific damageDef or entityDef? Should take priority
	if ( spawnArgs.GetString("damage_mult_" + def, "") != "" )
		damage_mult = spawnArgs.GetFloat("damage_mult_" + def, "1.0");

	else if ( spawnArgs.GetString("damage_mult_" + inf, "") != "" )
		damage_mult = spawnArgs.GetFloat("damage_mult_" + inf, "1.0");

	// otherwise check the standard spawnargs
	else {
		if ( def == "atdm:damage_firearrowDirect" )
			damage_mult	= spawnArgs.GetFloat("damage_mult_firearrow_direct", "1.0");

		else if ( def == "atdm:damage_firearrowSplash" )
			damage_mult	= spawnArgs.GetFloat("damage_mult_firearrow_splash", "1.0");

		else if ( def == "atdm:damage_arrow" )
			damage_mult	= spawnArgs.GetFloat("damage_mult_arrow", "0.0");

		else if ( inf == "atdm:attachment_melee_shortsword" )
			damage_mult	= spawnArgs.GetFloat("damage_mult_sword", "0.0");

		else if ( inf == "atdm:attachment_meleetest_blackjack" )
			damage		= spawnArgs.GetInt("damage_blackjack", "0");

		else if ( inflictor->IsType(idMoveable::Type) )
		{
			float mass = inflictor->GetPhysics()->GetMass();
			damage = (int)( damage * (mass / 5.0f) * spawnArgs.GetFloat("damage_mult_moveable", "1.0") );
		}

		else
			damage_mult = spawnArgs.GetFloat("damage_mult_other", "1.0");
	}

	damage *= damage_mult * damageScale;

	// inform the attacker that they hit someone
	attacker->DamageFeedback(this, inflictor, damage);
	if ( damage && damage > spawnArgs.GetInt("damage_threshold", "18") )
	{
		// do the damage.
		// if sufficient, kill the turret, otherwise play damaged FX
		health -= damage;
		if ( health <= 0 )
		{
			if ( health < -999 )
			{
				health = -999;
			}

			// decide wether to flinderize
			int flinderize_threshold = spawnArgs.GetInt("damage_flinderize", "100");
			m_bFlinderize = ( !flinderized && ( flinderize_threshold > 0 ) && ( damage >= flinderize_threshold ) );

			Killed(inflictor, attacker, damage, dir, location);
		}
		else
		{
			idStr fx;

			if ( m_bPower )
				fx = spawnArgs.GetString("fx_damage");
			else
				fx = spawnArgs.GetString("fx_damage_nopower");

			if ( fx.Length() && fx != "-" )
				idEntityFx::StartFx(fx, NULL, NULL, this, true);
		}
	}
}

/*
============
idTurret::Killed

Called whenever the turret is destroyed or damaged after destruction
============
*/
void idTurret::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idStr str;
	bool broke = false; //keeps track of whether the turret breaks during this function
	bool flind = false;	//keeps track of whether the turret flinderizes during this function

	// Become broken. Turret may flinderize if m_bFlinderize is true
	if ( !m_bIsBroken )
	{
		broke = true;
		flind = m_bFlinderize;

		idEntity::BecomeBroken(inflictor);
		Event_SetSkin( spawnArgs.GetString("skin_broken", "turret_off") );
	}
	// Turret was already broken, it's now being damaged again. Should it flinderize now?
	else if ( m_bFlinderize && !flinderized )
	{
		idEntity::Flinderize(inflictor);
		flind = true;
	}
	
	// Turret has just flinderized. Update model and skin
	if ( flind )
	{
		flinderized = true;
		
		spawnArgs.GetString("broken_flinderized", "-", str);
		if (str.Length() > 1) {
			SetModel(str);
		}
		spawnArgs.GetString("skin_broken_flinderized", "-", str);
		if (str.Length() > 1) {
			Event_SetSkin(str);
		}
	}

	// Handle destruction / post-destruction fx
	// turret is being broken right now
	StopSound(SND_CHANNEL_ANY, false);

	if ( broke )
	{
		if ( m_bPower ) {
			StartSound( "snd_death", 9, 0, false, NULL );
			str = spawnArgs.GetString("fx_destroyed");
		}
		else {
			StartSound( "snd_death_nopower", 9, 0, false, NULL);
			str = spawnArgs.GetString("fx_destroyed_nopower");
		}
	}

	// security camera was already broken
	else
	{
		if ( m_bPower )
			str = spawnArgs.GetString("fx_damage_nopower");
		else
			str = spawnArgs.GetString("fx_damage_nopower");
	}

	if ( str.Length() && str != "-" )
		idEntityFx::StartFx(str, NULL, NULL, this, true);

	if ( state == STATE_DEAD )
		return;

	intact = false;
	UpdateState();
	enemy = NULL;

	if ( spawnArgs.GetBool("notice_destroyed", "1") )
		SetStimEnabled(ST_VISUAL, true); // let AIs see that the turret is destroyed

	// Turn off the display screen

	if ( cameraDisplay.GetEntity() )
		cameraDisplay.GetEntity()->Hide();

	// Activate sparks
	if ( spawnArgs.GetBool("sparks", "1") )
	{
		if ( m_bPower || !sparksPowerDependent )
		{
			nextSparkTime = gameLocal.time + SEC2MS( spawnArgs.GetFloat("sparks_delay", "2") );
			BecomeActive(TH_UPDATEPARTICLES); // keeps stationary camera thinking to display sparks
		}
	}

	// Active a designated entity if destroyed 
	spawnArgs.GetString("break_up_target", "", str);
	idEntity *ent = gameLocal.FindEntity(str);
	if ( ent )
		ent->Activate( this );

	//provide a script hook for scripts, i.e. to generate FX
	idStr thread = spawnArgs.GetString("script_on_death", "");
	if( thread != "" && thread != "-" )
		CallScriptFunctionArgs( thread, true, 0, "e", this );
}

/*
================
idTurret::AddSparks
================
*/
void idTurret::AddSparks( void )
{
	idEntity *sparkEntity;
	idDict args;
	const char *particle, *cycleTrigger;
	spawnArgs.GetString("sparks_particle", "sparks_wires_oneshot.prt", &particle);
	cycleTrigger = ( sparksPeriodic ) ? "1" : "0";
	idVec3 spawnPos = GetPhysics()->GetOrigin() + ( spawnArgs.GetVector("sparks_offset", "0 0 0") * GetPhysics()->GetAxis() );

	args.Set( "classname", "func_emitter" );
	args.Set( "origin", spawnPos.ToString() );
	args.Set( "model", particle );
	args.Set( "cycleTrigger", cycleTrigger );
	gameLocal.SpawnEntityDef( args, &sparkEntity );
	sparks = sparkEntity;
	sparksOn = true;

	if ( sparksPeriodic )
		sparkEntity->Activate(NULL);
}

/*
================
idTurret::TriggerSparks
================
*/
void idTurret::TriggerSparks( void )
{
	//do nothing if it is not yet time to spark
	if ( gameLocal.time < nextSparkTime )
		return;

	//check whether a looping particle emitter needs to be toggled
	if ( !sparksPeriodic )
	{
		BecomeInactive(TH_UPDATEPARTICLES);

		if ( sparksPowerDependent )
		{
			if ( sparksOn == m_bPower )
				return;
			else
				sparksOn = m_bPower;
		}
	}

	//Create a func_emitter if none exists yet
	if ( !sparks.GetEntity() )
		AddSparks();
	//Otherwise use the existing one
	else
		sparks.GetEntity()->Activate(NULL);

	StopSound(6, false);	//sparks sound
	StartSound("snd_sparks", 6, 0, false, NULL);
	nextSparkTime = gameLocal.time + SEC2MS(sparksInterval) + SEC2MS(gameLocal.random.RandomFloat() * sparksIntervalRand );
}

void idTurret::Event_Attack( idEntity *ent, bool ignoreCollisions )
{
	if( !ent )
		return;

	m_bManualModeEnemy				= true;
	m_bManualModePosition			= false;
	m_bManualModeIgnoreCollisions	= ignoreCollisions;
	enemy							= ent;
}

void idTurret::Event_AttackPosition( idVec3 &targetPos, bool ignoreCollisions )
{
	m_bManualModeEnemy				= false;
	m_bManualModePosition			= true;
	m_bManualModeIgnoreCollisions	= ignoreCollisions;
	enemy							= NULL;
	attackPos						= targetPos;
}

void idTurret::Event_DisableManualAttack( void )
{
	m_bManualModeEnemy				= false;
	m_bManualModePosition			= false;
	m_bManualModeIgnoreCollisions	= false;
	enemy							= NULL;
}

/*
================
idTurret::GetTurretState
================
*/
int idTurret::GetTurretState( void )
{
	return state;
}

/*
================
idTurret::Event_GetTurretState
================
*/
void idTurret::Event_GetTurretState( void )
{
	idThread::ReturnFloat( GetTurretState() );
}
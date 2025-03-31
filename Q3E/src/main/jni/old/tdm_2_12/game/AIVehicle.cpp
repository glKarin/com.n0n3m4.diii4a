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

// Copyright (C) 2007 The Dark Mod Authors
//

#include "precompiled.h"
#pragma hdrstop



#include "AIVehicle.h"
#include "Game_local.h"
#include "DarkModGlobals.h"

//===============================================================================
//CAIVehicle
//===============================================================================

const idEventDef EV_SetController( "setController", EventArgs('e', "playerController", ""), EV_RETURNS_VOID, 
	"Let a player assume movement control of an AI vehicle\n" \
	"(may be remote control as in a player on a coach pulled by a horse)");
const idEventDef EV_ClearController( "clearController", EventArgs(), EV_RETURNS_VOID, 
	"Need separate clearController because scripting doesn't like passing in $null_entity?\n" \
	"(greebo: one could remove this function and set the argument type of setController to 'E'.");
const idEventDef EV_FrobRidable( "frobRidable", EventArgs('e', "playerController", ""), EV_RETURNS_VOID, "Called when a player directly mounts or dismounts a ridable AI." );
const idEventDef EV_GetMoveAnim( "getMoveAnim", EventArgs(), 's', "Returns the name of the player-requested movement anim for a player controlled AI vehicle" );

CLASS_DECLARATION( idAI, CAIVehicle )
	EVENT( EV_SetController,	CAIVehicle::Event_SetController )
	EVENT( EV_ClearController,	CAIVehicle::Event_ClearController )
	EVENT( EV_FrobRidable,		CAIVehicle::Event_FrobRidable )
	EVENT( EV_GetMoveAnim,		CAIVehicle::Event_GetMoveAnim )

END_CLASS

CAIVehicle::CAIVehicle( void )
{
	m_Controller			= NULL;
	m_RideJoint		= INVALID_JOINT;
	m_RideOffset.Zero();
	m_RideAngles.Zero();
	m_CurAngle		= 0.0f;
	
	m_SteerSpeed		= 0.0f;
	m_SpeedFrac			= 0.0f;
	m_SpeedTimeToMax	= 0.0f;

	m_CurMoveAnim.Clear();
	m_JumpAnim.Clear();

	m_Speeds.Clear();
}

CAIVehicle::~CAIVehicle( void )
{
}

void CAIVehicle::Spawn( void ) 
{
	AI_CONTROLLED = false; 

	// set ride joint, steering speed
	idStr JointName = spawnArgs.GetString("ride_joint", "origin");

	spawnArgs.GetFloat("steerSpeed", "5", m_SteerSpeed);
	spawnArgs.GetVector("ride_offset", "0 0 0", m_RideOffset);
	spawnArgs.GetAngles("ride_angles", "0 0 0", m_RideAngles);
	spawnArgs.GetFloat("time_to_max_speed", "2.0", m_SpeedTimeToMax);
	
	m_Speeds.Clear();
	for( int i=0; spawnArgs.FindKey(va("speed_%d_anim",i)) != NULL; i++ )
	{
		SAIVehicleSpeed speed;
		speed.Anim = spawnArgs.GetString(va("speed_%d_anim",i));
		speed.MinAnimRate = spawnArgs.GetFloat(va("speed_%d_min_rate",i),"1.0");
		speed.MaxAnimRate = spawnArgs.GetFloat(va("speed_%d_max_rate",i),"1.0");
		speed.NextSpeedFrac = spawnArgs.GetFloat(va("speed_%d_next_speed_control_frac",i),"1.0");
		
		m_Speeds.Append(speed);
	}

	m_RideJoint = animator.GetJointHandle( JointName.c_str() );

	BecomeActive( TH_THINK );
}

void CAIVehicle::Save(idSaveGame *savefile) const
{
	m_Controller.Save( savefile );

	savefile->WriteJoint( m_RideJoint );
	savefile->WriteVec3( m_RideOffset );
	savefile->WriteAngles( m_RideAngles );

	savefile->WriteFloat( m_CurAngle );
	savefile->WriteFloat( m_SpeedFrac );
	savefile->WriteFloat( m_SteerSpeed );
	savefile->WriteFloat( m_SpeedTimeToMax );

	savefile->WriteInt( m_Speeds.Num() );
	for( int i=0; i < m_Speeds.Num(); i++ )
	{
		savefile->WriteString( m_Speeds[i].Anim );
		savefile->WriteFloat( m_Speeds[i].MinAnimRate );
		savefile->WriteFloat( m_Speeds[i].MaxAnimRate );
		savefile->WriteFloat( m_Speeds[i].NextSpeedFrac );
	}
}

void CAIVehicle::Restore( idRestoreGame *savefile )
{
	m_Controller.Restore( savefile );

	savefile->ReadJoint( m_RideJoint );
	savefile->ReadVec3( m_RideOffset );
	savefile->ReadAngles( m_RideAngles );
	
	savefile->ReadFloat( m_CurAngle );
	savefile->ReadFloat( m_SpeedFrac );
	savefile->ReadFloat( m_SteerSpeed );
	savefile->ReadFloat( m_SpeedTimeToMax );

	m_Speeds.Clear();
	int num;
	savefile->ReadInt( num );
	for( int i=0; i < num; i++ )
	{
		SAIVehicleSpeed speed;
		savefile->ReadString( speed.Anim );
		savefile->ReadFloat( speed.MinAnimRate );
		savefile->ReadFloat( speed.MaxAnimRate );
		savefile->ReadFloat( speed.NextSpeedFrac );
		m_Speeds.Append( speed );
	}
}

void CAIVehicle::PlayerFrob( idPlayer *player ) 
{
	idVec3 origin;
	idMat3 axis;

	// =============== DISMOUNT ===============
	if ( m_Controller.GetEntity() ) 
	{
		if ( m_Controller.GetEntity() == player ) 
		{
			player->Unbind();
			SetController(NULL);
		}
	}
	// =============== MOUNT ===============
	else 
	{
		// attach player to the joint
		GetJointWorldTransform( m_RideJoint, gameLocal.time, origin, axis );

		// put the player in a crouch, so their view is low to the animal
		// without actually clipping into it
		idPhysics_Player *playerPhys = (idPhysics_Player *) player->GetPhysics();
		// TODO: look up PMF_DUCKED somehow (make those flags statics in the idPhysics_Player class?)
		// for now, using 1 since we know that is PMF_DUCKED
		// Or, we could move mount and dismount to idPhysics_Player...
		playerPhys->SetMovementFlags( playerPhys->GetMovementFlags() | 1 );

		player->SetOrigin( origin + m_RideOffset * axis );
		player->BindToJoint( this, m_RideJoint, true );

		SetController( player );
	}
}

void CAIVehicle::SetController( idPlayer *player )
{
	// no change
	if( player == m_Controller.GetEntity() )
		return;

	if( player != NULL )
	{
		// new controller added
		m_Controller = player;
		
		// Initialize angle to current yaw angle
		idAngles FaceAngles = viewAxis.ToAngles();

		m_CurAngle = FaceAngles.yaw;

		ClearEnemy();
		m_bIgnoreAlerts = true;
		AI_CONTROLLED = true;
		AI_RUN = false; // want to let custom vehicle "walk" script pick the legs animation
	}
	else
	{
		// controller removed, stop and return control to AI mind
		m_Controller = NULL;
		m_SpeedFrac = 0.0f;

		m_bIgnoreAlerts = false;
		AI_CONTROLLED = false;

		StopMove( MOVE_STATUS_DONE );
	}
}

void CAIVehicle::Think( void )
{
	if ( !(thinkFlags & TH_THINK ) )
		goto Quit;

	if( m_Controller.GetEntity() )
	{
		// Exit the combat state if we somehow got in it.
		// Later we can fight as directed by player, but right now it's too independent
		ClearEnemy();

		// Update controls and movement dir
		UpdateSteering();

		// Speed controls, for now just AI_MOVE
		bool bMovementReq = UpdateSpeed();

		// Request move at direction
		if( bMovementReq )
		{
			MoveAlongVector( m_CurAngle );
		}	
		else
		{
			StopMove( MOVE_STATUS_DONE );
			// just turn if no forward/back movement is requested
			TurnToward( m_CurAngle );
		}
	}

	idAI::Think();

Quit:
	return;
}

void CAIVehicle::Event_FrobRidable(idPlayer *player )
{
	// Don't mount if dead, but still let the player dismount
	if( (AI_KNOCKEDOUT || AI_DEAD) && !m_Controller.GetEntity() )
	{
		// proceed with normal AI body frobbing code
		idAI::FrobAction(true);
	}
	else
		PlayerFrob(player);
}

void CAIVehicle::UpdateSteering( void )
{
	float idealSteerAngle(0.0f), angleDelta(0.0f), turnScale(0.0f);
	idPlayer *player = m_Controller.GetEntity();

	// TODO: steer speed dependent on velocity to simulate finite turn radius

	if( idMath::Fabs(player->usercmd.rightmove) > 0 )
	{
		turnScale = 30.0f / 128.0f;
		idealSteerAngle = m_CurAngle - player->usercmd.rightmove * turnScale;
		angleDelta = idealSteerAngle - m_CurAngle;

		if ( angleDelta > m_SteerSpeed ) 
		{
			m_CurAngle += m_SteerSpeed;
		} else if ( angleDelta < -m_SteerSpeed ) 
		{
			m_CurAngle -= m_SteerSpeed;
		} else 
		{
			m_CurAngle = idealSteerAngle;
		}
	}
}

bool CAIVehicle::UpdateSpeed( void )
{
	bool bReturnVal( false );
	idPlayer *player = m_Controller.GetEntity();
	
	float DeltaVMag = 1.0f/(m_SpeedTimeToMax * USERCMD_HZ);

	if( player->usercmd.forwardmove > 0 )
		m_SpeedFrac += DeltaVMag;
	else if( player->usercmd.forwardmove < 0 )
		m_SpeedFrac -= DeltaVMag;

	// TODO: Support walking backwards, needs AI base class changes?
	// For now, only move forward
	// m_SpeedFrac = idMath::ClampFloat( -m_MaxReverseSpeed, 1.0f, m_SpeedFrac );
	m_SpeedFrac = idMath::ClampFloat( 0.0f, 1.0f, m_SpeedFrac );

	if( m_SpeedFrac > 0.0001f )
	{
		bReturnVal = true;

		for( int i=0; i < m_Speeds.Num(); i++ )
		{
			// nextfrac should default to idMath::Infinity when read from spawnargs
			float NextFrac = m_Speeds[i].NextSpeedFrac;
			if( m_SpeedFrac > NextFrac )
				continue;

			float PrevFrac;
			if( i > 0 )
				PrevFrac = m_Speeds[i-1].NextSpeedFrac;
			else
				PrevFrac = 0;

			float animFrac = (m_SpeedFrac - PrevFrac)/(NextFrac - PrevFrac);
			float animRate = m_Speeds[i].MinAnimRate + animFrac * (m_Speeds[i].MaxAnimRate - m_Speeds[i].MinAnimRate);
			m_CurMoveAnim = m_Speeds[i].Anim;

			// update anim rate
			int animNum = animator.GetAnim( m_CurMoveAnim.c_str() );
			m_animRates[animNum] = animRate;
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->UpdatePlaybackRate( animNum, this );

			break; // done, don't go on to higher speeds
		}
	}

	return bReturnVal;
}

// script events
void CAIVehicle::Event_SetController( idPlayer *player )
{
	SetController( player );
}

// script events
void CAIVehicle::Event_ClearController( void )
{
	SetController( NULL );
}

void CAIVehicle::Event_GetMoveAnim( void )
{
	if( m_Controller.GetEntity() )
		idThread::ReturnString(m_CurMoveAnim.c_str());
	else
	{
		// this should help with debugging if this accidentally gets called with no controller
		idThread::ReturnString("");
	}
}

void CAIVehicle::LinkScriptVariables( void )
{
	// Call the base class first
	idAI::LinkScriptVariables();

	AI_CONTROLLED.LinkTo( scriptObject, "AI_CONTROLLED");
}


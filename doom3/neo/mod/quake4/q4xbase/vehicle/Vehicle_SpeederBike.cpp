//----------------------------------------------------------------
// VehicleStatic.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "VehicleRigid.h"
#include "VehicleParts.h"


const idEventDef						EV_SetSpeederBikeSpeed("setSpeederBikeSpeed", "ff");
const idEventDef						EV_SetSpeederBikeMaxGravityDistance("setSpeederBikeMaxGravityDistance", "ff");
const idEventDef						EV_SetSpeederBikeBoostEnabled("setSpeederBikeBoostEnabled", "f");


class riVehicleSpeederBike : public rvVehicleRigid {
public:

	CLASS_PROTOTYPE( riVehicleSpeederBike );

							riVehicleSpeederBike		( void );
	void					Spawn						( void );
	virtual void			Think						( void );
	void					Save					( idSaveGame *savefile ) const;
	void					Restore					( idRestoreGame *savefile );
	virtual void			UpdateState				( void );
	virtual void			UpdateHUD				( int position, idUserInterface* gui );

	void					Event_SetSpeederBikeSpeed(float speed, float transitionTime);
	void					Event_SetSpeederBikeMaxGravityDistance(float speed, float transitionTime);
	void					Event_SetSpeederBikeBoostEnabled(float isBoostEnabled);

private:
	void					UpdateGUI				( void );
	void					SetBoostEnabled( bool enabled );
	bool					IsBoostEnabled( void ) const;
	float					CalcSpeed( void );

private:
	float					speed;
};

CLASS_DECLARATION( rvVehicleRigid, riVehicleSpeederBike )
	EVENT(EV_SetSpeederBikeSpeed,						riVehicleSpeederBike::Event_SetSpeederBikeSpeed)
	EVENT(EV_SetSpeederBikeMaxGravityDistance,			riVehicleSpeederBike::Event_SetSpeederBikeMaxGravityDistance)
	EVENT(EV_SetSpeederBikeBoostEnabled,				riVehicleSpeederBike::Event_SetSpeederBikeBoostEnabled)
END_CLASS

riVehicleSpeederBike::riVehicleSpeederBike( void )
{
	speed = 1.0f;
}

/*
================
riVehicleSpeederBike::Spawn
================
*/
void riVehicleSpeederBike::Spawn( void ) {
	speed = 1.0f;
}

/*
================
riVehicleSpeederBike::Think
================
*/
void riVehicleSpeederBike::Think( void )
{
	rvVehicleRigid::Think();
	UpdateGUI();
}

/*
================
riVehicleSpeederBike::Save
================
*/
void riVehicleSpeederBike::Save ( idSaveGame *savefile ) const {
	savefile->WriteFloat( speed );
}

/*
================
riVehicleSpeederBike::Restore
================
*/
void riVehicleSpeederBike::Restore ( idRestoreGame *savefile ) {
	savefile->ReadFloat( speed );
}

/*
================
riVehicleSpeederBike::UpdateState
================
*/
void riVehicleSpeederBike::UpdateState ( void ) {
	rvVehiclePosition& pos = positions[0];
	usercmd_t& cmd	= pos.mInputCmd;
	cmd.forwardmove *= speed;
	cmd.rightmove *= speed;

	vfl.driver		= pos.IsOccupied();
#ifdef __ANDROID__ //karin: for in smooth joystick on Android
	if(harm_g_normalizeMovementDirection.GetBool())
	{
		if(vfl.driver)
		{
			GAME_SETUPCMDDIRECTION(cmd, vfl.forward, vfl.backward, vfl.right, vfl.left);
		}
		else
		{
			vfl.forward = false;
			vfl.backward = false;
			vfl.right = false;
			vfl.left = false;
		}
	}
	else
	{
#endif
	vfl.forward		= (vfl.driver && cmd.forwardmove > 0);
  	vfl.backward	= (vfl.driver && cmd.forwardmove < 0);
  	vfl.right		= (vfl.driver && cmd.rightmove < 0);
  	vfl.left		= (vfl.driver && cmd.rightmove > 0);
#ifdef __ANDROID__ //karin: for in smooth joystick on Android
	}
#endif
	vfl.strafe		= (vfl.driver && cmd.buttons & BUTTON_STRAFE );
}

void riVehicleSpeederBike::SetBoostEnabled( bool enabled )
{
	rvVehiclePart *part;
	riVehiclePartBoost *boost;

	rvVehiclePosition *position = GetPosition(0);
	for(int i = 0; i < position->mParts.Num(); i++)
	{
		part = position->GetPart(i);
		if(part->IsType(riVehiclePartBoost::Type))
		{
			boost = static_cast<riVehiclePartBoost *>(part);
			boost->SetEnabled(enabled);
		}
	}
}

bool riVehicleSpeederBike::IsBoostEnabled( void ) const
{
	const rvVehiclePart *part;
	const riVehiclePartBoost *boost;

	const rvVehiclePosition *position = GetPosition(0);
	for(int i = 0; i < position->mParts.Num(); i++)
	{
		part =  position->mParts[i];
		if(part->IsType(riVehiclePartBoost::Type))
		{
			boost = static_cast<const riVehiclePartBoost *>(part);
			if (boost->IsActived())
				return true;
		}
	}
	return false;
}

/*
================
riVehicleSpeederBike::UpdateHUD
================
*/
void riVehicleSpeederBike::UpdateHUD ( int position, idUserInterface* gui ) {
	idVec3 velocity = GetPhysics()->GetLinearVelocity(0);
	velocity.z = 0.0f;
	float speed = CalcSpeed();
	float speedf = idMath::Floor( speed );
	int speedi = idMath::Ftoi(speedf);
	int speedp = idMath::Ftoi(idMath::Floor( (speed - speedf) * 100.0f ));
	gui->SetStateInt( "currenttime", speedi );
	gui->HandleNamedEvent("updatetimer");
	gui->SetStateInt( "currenttime2", speedp );
	gui->HandleNamedEvent("updatetimer2");
	gui->SetStateBool( "boost_active", IsBoostEnabled() );

	// Update position specific information
	//positions[position].UpdateHUD ( gui );

	rvVehicleRigid::UpdateHUD(position, gui);
}

float riVehicleSpeederBike::CalcSpeed( void )
{
	idVec3 velocity = GetPhysics()->GetLinearVelocity(0);
	velocity.z = 0.0f;
	float speed = velocity.Normalize();
	//speed /= MS2SEC(gameLocal.msec); // length / second
	speed *= 0.0254f/* DOOM_TO_METERS */; // meter / second
	speed *= 3.6f; // meter / hour
	return speed;
}

/*
================
riVehicleSpeederBike::UpdateGUI
================
*/
void riVehicleSpeederBike::UpdateGUI ( void ) {
	if ( renderEntity.gui[ 0 ] ) {
		renderEntity.gui[ 0 ]->SetStateFloat( "vehicle_speed", CalcSpeed() );
		renderEntity.gui[ 0 ]->StateChanged(gameLocal.time);
	}
}



void riVehicleSpeederBike::Event_SetSpeederBikeSpeed(float speed, float transitionTime)
{
	this->speed = speed;
}

void riVehicleSpeederBike::Event_SetSpeederBikeMaxGravityDistance(float speed, float transitionTime)
{
	// unused
}

void riVehicleSpeederBike::Event_SetSpeederBikeBoostEnabled(float isBoostEnabled)
{
	SetBoostEnabled(isBoostEnabled ? true : false);
}

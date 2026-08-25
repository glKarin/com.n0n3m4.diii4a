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


class riVehiclePartBoost : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( riVehiclePartBoost );

					riVehiclePartBoost	( void );

	void			Spawn				( void );
	void			Save				( idSaveGame* saveFile ) const;
	void			Restore				( idRestoreGame* saveFile );

	virtual void	RunPhysics			( void );
	void			SetEnabled			( bool enabled );
	bool			IsActived			( void ) const;

private:
	bool			UpdateState			( bool on );
	void			UpdateFov			( bool on );
	void			StartLinear			( float duration, bool usingCurrent = false );
	void			StopLinear			( float duration, bool usingCurrent = false );
	void			PlaySound			( const idSoundShader *shader, bool looping = false );
	void			StopSound			( void );

private:
	enum {
		ST_READY = 0,
		ST_INCR,
		ST_MAX,
		ST_DECR,
	};

	float			envelopeAttackSeconds;
	float			envelopeSustainSeconds;
	float			envelopeDecaySeconds;
	float			envelopeRefreshSeconds;
	float			forwardForceMax;
	float			fovIncreaseMax;
	const idSoundShader *soundBoost;
	const idSoundShader *soundBoostEnd;
	int				state;
	idInterpolate<float> fovLinear;
	idInterpolate<float> forceLinear;
};

CLASS_DECLARATION( rvVehiclePart, riVehiclePartBoost )
END_CLASS

riVehiclePartBoost::riVehiclePartBoost ( void ) {
	envelopeAttackSeconds = 1.0f;
	envelopeSustainSeconds = 1.0f;
	envelopeDecaySeconds = 1.0f;
	envelopeRefreshSeconds = 1.0f;
	forwardForceMax = 0.0f;
	fovIncreaseMax = 0.0f;
	soundBoost = NULL;
	soundBoostEnd = NULL;
	state = ST_READY;
}

/*
=====================
riVehiclePartBoost::Save
=====================
*/
void riVehiclePartBoost::Save ( idSaveGame* savefile ) const {
	savefile->WriteFloat ( envelopeAttackSeconds );
	savefile->WriteFloat ( envelopeSustainSeconds );
	savefile->WriteFloat ( envelopeDecaySeconds );
	savefile->WriteFloat ( envelopeRefreshSeconds );
	savefile->WriteFloat ( forwardForceMax );
	savefile->WriteFloat ( fovIncreaseMax );
	savefile->WriteInt ( state );
}

/*
=====================
riVehiclePartBoost::Restore
=====================
*/
void riVehiclePartBoost::Restore ( idRestoreGame* savefile ) {
	savefile->ReadFloat ( envelopeAttackSeconds );
	savefile->ReadFloat ( envelopeSustainSeconds );
	savefile->ReadFloat ( envelopeDecaySeconds );
	savefile->ReadFloat ( envelopeRefreshSeconds );
	savefile->ReadFloat ( forwardForceMax );
	savefile->ReadFloat ( fovIncreaseMax );
	savefile->ReadInt ( state );

	soundBoost = declManager->FindSound ( spawnArgs.GetString ( "snd_boost" ), false );
	soundBoostEnd = declManager->FindSound ( spawnArgs.GetString ( "snd_boostEnd" ), false );
}

/*
================
riVehiclePartBoost::Spawn
================
*/
void riVehiclePartBoost::Spawn ( void ) {
	idStr keyName;

	envelopeAttackSeconds	= SEC2MS( spawnArgs.GetFloat ( "boostEnvelopeAttackSeconds", "1" ) );
	envelopeSustainSeconds	= SEC2MS( spawnArgs.GetFloat ( "boostEnvelopeSustainSeconds", "1" ) );
	envelopeDecaySeconds	= SEC2MS( spawnArgs.GetFloat ( "boostEnvelopeDecaySeconds", "1" ) );
	envelopeRefreshSeconds	= SEC2MS( spawnArgs.GetFloat ( "boostEnvelopeRefreshSeconds", "1" ) );
	forwardForceMax			= spawnArgs.GetFloat ( "boostForwardForceMax", "0" );
	fovIncreaseMax			= spawnArgs.GetFloat ( "boostFovIncreaseMax", "0" );

	soundBoost = declManager->FindSound ( spawnArgs.GetString ( "snd_boost" ), false );
	soundBoostEnd = declManager->FindSound ( spawnArgs.GetString ( "snd_boostEnd" ), false );

	state = ST_READY;
}

/*
================
riVehiclePartBoost::RunPhysics
================
*/
void riVehiclePartBoost::RunPhysics ( void ) {
	float mult;

	if ( !IsActive ( ) ) {
		return;
	}

	bool on = position->mInputCmd.upmove < 0.0f;
	if(!UpdateState(on))
		return;

	// Determine the force multiplier from the key being pressed
	mult = forceLinear.GetCurrentValue(gameLocal.time);
	// No multiplier, no move
	if ( mult == 0.0f ) {
		return;
	}

	UpdateOrigin ( );

	// Apply the force
	parent->GetPhysics()->ApplyImpulse ( 0, worldOrigin, worldAxis[0] * mult );
}

ID_INLINE void riVehiclePartBoost::UpdateFov( bool on )
{
	idPlayer *player = gameLocal.GetLocalPlayer();

	if(!player)
		return;

	if(on)
		player->SetInfluenceFov(player->DefaultFov() + fovLinear.GetCurrentValue(gameLocal.time));
	else
		player->SetInfluenceFov(0);
}

ID_INLINE void riVehiclePartBoost::StartLinear( float duration, bool usingCurrent )
{
	if(usingCurrent)
	{
		fovLinear.Init(gameLocal.time, duration, fovLinear.GetCurrentValue(gameLocal.time), fovIncreaseMax);
		forceLinear.Init(gameLocal.time, duration, forceLinear.GetCurrentValue(gameLocal.time), forwardForceMax);
	}
	else
	{
		fovLinear.Init(gameLocal.time, duration, 0.0f, fovIncreaseMax);
		forceLinear.Init(gameLocal.time, duration, 0.0f, forwardForceMax);
	}
}

ID_INLINE void riVehiclePartBoost::StopLinear( float duration, bool usingCurrent )
{
	if(usingCurrent)
	{
		fovLinear.Init(gameLocal.time, duration, fovLinear.GetCurrentValue(gameLocal.time), 0.0f);
		forceLinear.Init(gameLocal.time, duration, forceLinear.GetCurrentValue(gameLocal.time), 0.0f);
	}
	else
	{
		fovLinear.Init(gameLocal.time, duration, fovIncreaseMax, 0.0f);
		forceLinear.Init(gameLocal.time, duration, forwardForceMax, 0.0f);
	}
}

ID_INLINE void riVehiclePartBoost::PlaySound( const idSoundShader *shader, bool looping )
{
	if(shader)
		parent->StartSoundShader( shader, soundChannel, looping ? SSF_LOOPING : 0, false, NULL );
}

ID_INLINE void riVehiclePartBoost::StopSound( void )
{
	parent->StopSound(soundChannel, false);
}

bool riVehiclePartBoost::UpdateState( bool enabled )
{
	if(enabled)
	{
		switch (state)
		{
			case ST_READY:
				PlaySound(soundBoost, true);
				StartLinear(envelopeSustainSeconds);
				state = ST_INCR;
				return false;
			case ST_INCR:
				if(forceLinear.IsDone(gameLocal.time))
				{
					PlaySound(soundBoostEnd);
					state = ST_MAX;
				}
				UpdateFov(true);
				return true;
			case ST_MAX:
				return true;
			case ST_DECR:
				StartLinear(envelopeSustainSeconds, true);
				state = ST_INCR;
				return true;
		}
	}
	else
	{
		switch (state)
		{
			case ST_READY:
				return false;
			case ST_INCR:
				StopLinear(envelopeDecaySeconds, true);
				state = ST_DECR;
				return true;
			case ST_MAX:
				StopLinear(envelopeDecaySeconds);
				state = ST_DECR;
				return true;
			case ST_DECR:
				if(forceLinear.IsDone(gameLocal.time))
				{
					state = ST_READY;
					StopSound();
					UpdateFov(false);
					return false;
				}
				else
				{
					UpdateFov(true);
					return true;
				}
		}
	}

	return false;
}

void riVehiclePartBoost::SetEnabled( bool enabled )
{
	if (this->IsActive() != enabled)
	{
		if (!enabled)
		{
			StopSound();
			UpdateFov(false);
		}
		state = ST_READY;
		Activate(enabled);
	}
}

bool riVehiclePartBoost::IsActived( void ) const
{
	return IsActive() && (state == ST_INCR || state == ST_MAX);
}



// unused
class riVehiclePartSplineTether : public rvVehiclePart {
public:

	CLASS_PROTOTYPE( riVehiclePartSplineTether );
};

CLASS_DECLARATION( rvVehiclePart, riVehiclePartSplineTether )
END_CLASS



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
	float speed = velocity.Normalize();
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

/*
================
riVehicleSpeederBike::UpdateGUI
================
*/
void riVehicleSpeederBike::UpdateGUI ( void ) {
	if ( renderEntity.gui[ 0 ] ) {
		idVec3 velocity = GetPhysics()->GetLinearVelocity(0);
		velocity.z = 0.0f;
		renderEntity.gui[ 0 ]->SetStateFloat( "vehicle_speed", velocity.Normalize() );
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

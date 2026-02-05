#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "idlib/LangDict.h"

#include "bc_meta.h"
#include "bc_radio.h"

const int LIGHT_FORWARDPOS = 8;
const int LIGHT_UPPOS = 4;
const int LIGHT_RADIUS = 16;
const int ACTIVATE_STATIC_TIME = 2000; //when turned on, how long before the interestpoint is spawned
const int INTEREST_INTERVALTIME = 1000;

const int MONSTER_FROBINDEX = 7; //make sure this matches the frobindex in interestpoint .def interest_radio

const idVec3 IDLECOLOR = idVec3(0, 1, 0);
const idVec3 FROBCOLOR = idVec3(1, .9f, 0);

CLASS_DECLARATION(idMoveableItem, idRadio)
END_CLASS

idRadio::idRadio(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
	isOn = false;

	activateTimer = 0;

	musicNotes = nullptr;
	soundwaves = nullptr;

	interestTimer = 0;
}

idRadio::~idRadio(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	StopSound(SND_CHANNEL_MUSIC);
}


void idRadio::Spawn(void)
{
	isOn = spawnArgs.GetBool("start_on", "0");

	//spawn soundwave particle.
	idAngles particleAng = GetPhysics()->GetAxis().ToAngles();
	particleAng.pitch += 90;
	idVec3 forwardDir, rightDir, upDir;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);
	idDict args;
	
	args.Clear();
	args.Set("model", "music_loop.prt");
	args.SetBool("start_off", isOn);
	musicNotes = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	musicNotes->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * 3.1f) + (upDir * 4) + (rightDir * 2.1f));
	musicNotes->SetAngles(particleAng);
	musicNotes->Bind(this, false);

	args.Clear();
	args.Set("model", "sound_waves_small.prt");
	args.SetBool("start_off", isOn);
	soundwaves = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	soundwaves->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * 3.1f) + (upDir * 4) + (rightDir * 2.1f));
	soundwaves->SetAngles(particleAng);
	soundwaves->Bind(this, true);

	SetActivate(isOn);
	activateTimer = 0;

	if (!isOn)
	{
		musicNotes->Hide();
		soundwaves->Hide();
	}

	interestTimer = 0;	
}


void idRadio::Save(idSaveGame *savefile) const
{
	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteBool( isOn ); // bool isOn

	savefile->WriteInt( activateTimer ); // int activateTimer

	savefile->WriteObject( musicNotes ); // idFuncEmitter * musicNotes
	savefile->WriteObject( soundwaves ); // idFuncEmitter * soundwaves

	savefile->WriteInt( interestTimer ); // int interestTimer
	savefile->WriteObject( interestPoint ); // idEntityPtr<idEntity> interestPoint
}

void idRadio::Restore(idRestoreGame *savefile)
{
	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadBool( isOn ); // bool isOn

	savefile->ReadInt( activateTimer ); // int activateTimer

	savefile->ReadObject( CastClassPtrRef(musicNotes) ); // idFuncEmitter * musicNotes
	savefile->ReadObject( CastClassPtrRef(soundwaves) ); // idFuncEmitter * soundwaves

	savefile->ReadInt( interestTimer ); // int interestTimer
	savefile->ReadObject( interestPoint ); // idEntityPtr<idEntity> interestPoint
}

bool idRadio::DoFrob(int index, idEntity * frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		SetActivate(!isOn);
		return true;
	}

	if (index == MONSTER_FROBINDEX)
	{
		//Monster has is investigating the radio and wants to turn it off.
		if (isOn)
		{
			SetActivate(false); //turn radio off.
			gameLocal.DoParticle("frob_lines.prt", GetPhysics()->GetOrigin()); //do particle fx.
			
			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter() + idVec3(2,2,2), idVec3(0, 0, 64)); //give it a little upward physics jostle.
		}

		return true;
	}

	return idMoveableItem::DoFrob(index, frobber);
}

bool idRadio::DoFrobHold(int index, idEntity * frobber)
{
	GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter() + idVec3(2, 2, 2), idVec3(0, 0, 32)); //give it a little upward physics jostle.

	SetActivate(!isOn);
	return true;
}

void idRadio::SetActivate(bool value)
{
	if (value)
	{
		//turn ON.
		if (!isOn)
		{
			StartSound("snd_turnon", SND_CHANNEL_BODY3);
			StartSound("snd_tune", SND_CHANNEL_BODY2);

			activateTimer = gameLocal.time + ACTIVATE_STATIC_TIME;
		}

		
		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_on")));
		displayNameHold = common->GetLanguageDict()->GetString("#str_def_gameplay_turnoff");
		BecomeActive(TH_THINK);
		SetColor(FROBCOLOR);
	}
	else if (!value)
	{
		//turn OFF.

		if (isOn)
		{
			StartSound("snd_turnoff", SND_CHANNEL_BODY3);
		}

		if (headlightHandle != -1)
			gameRenderWorld->FreeLightDef(headlightHandle);

		if (soundwaves->GetParticleActive())
			soundwaves->SetActive(false);

		if (musicNotes->GetParticleActive())
			musicNotes->SetActive(false);

		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
		displayNameHold = common->GetLanguageDict()->GetString("#str_def_gameplay_turnon");
		StopSound(SND_CHANNEL_MUSIC);
		BecomeInactive(TH_THINK);
		SetColor(IDLECOLOR);
	}

	isOn = value;
}

void idRadio::Think(void)
{
	if (isOn)
	{
		//Update renderlight.	
		if (headlightHandle != -1)
		{
			//Create the light if it doesn't exist.
			idVec3 forward, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
			headlight.origin = GetPhysics()->GetOrigin() + (forward * LIGHT_FORWARDPOS) + (up * LIGHT_UPPOS);
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
		else
		{
			//Update light's position to be in front of radio.
			headlight.shader = declManager->FindMaterial("lights/pulse04", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;
			headlight.shaderParms[0] = .3f;
			headlight.shaderParms[1] = .3f;
			headlight.shaderParms[2] = .2f;
			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
		}

		if (!IsPlayingSound(SND_CHANNEL_MUSIC) && gameLocal.time >= activateTimer)
		{
			//The radio track has ended. Play the next track.
			StartSound("snd_radio", SND_CHANNEL_MUSIC); //play the station.
		}

		// If we're being carried by the player and not the active carryable, update our position to the player
		// so that the sound emitter still works
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( player && player->GetCarryable() != this && player->HasEntityInCarryableInventory( this ) )
		{
			SetOrigin( player->GetPhysics()->GetOrigin() );
			// Update the children entities too
			idEntity* next = nullptr;
			for ( idEntity* ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
				next = ent->GetNextTeamEntity();
				ent->GetPhysics()->Evaluate( 0.0f, 0.0f );
				ent->UpdateVisuals();
			}
		}

		if (gameLocal.time > interestTimer && gameLocal.time >= activateTimer)
		{
			interestTimer = gameLocal.time + INTEREST_INTERVALTIME;
			if ( !interestPoint.IsValid() )
			{
				interestPoint = gameLocal.SpawnInterestPoint( this, GetPhysics()->GetOrigin(), spawnArgs.GetString( "interest_radiosound" ) );
			}
		}

		if (gameLocal.time >= activateTimer)
		{
			if (musicNotes->IsHidden())
			{
				musicNotes->Show();				
			}

			if (soundwaves->IsHidden())
			{
				soundwaves->Show();
				
			}			

			if (!musicNotes->GetParticleActive())
				musicNotes->SetActive(true);

			if (!soundwaves->GetParticleActive())
				soundwaves->SetActive(true);
		}
	}
	else if ( interestPoint.IsValid() )
	{
		interestPoint.GetEntity()->PostEventMS( &EV_Remove, 0 );
		interestPoint = nullptr;
	}

	idMoveableItem::Think();
}


//bool idRadio::Collide(const trace_t &collision, const idVec3 &velocity)
//{
//}

//void idRadio::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
//{
//	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);	
//}

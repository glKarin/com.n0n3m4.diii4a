#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"

#include "bc_oxygenstation.h"

#define AIRTANK_QUANTITY 1.5f //how much air to store inside the oxygen station. 1 airtank = full oxygen tank for player.

#define FROB_ANIMTIME 3500 //how long it takes for the little hand on the dial to move.
#define ANNOUNCE_CHECKTIME 2000
#define ANNOUNCE_COOLDOWNTIME 10000
#define ANNOUNCE_ACTIVATIONRADIUS 256

CLASS_DECLARATION(idAnimatedEntity, idOxygenStation)
END_CLASS

idOxygenStation::idOxygenStation(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idOxygenStation::~idOxygenStation(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}
}

void idOxygenStation::Spawn(void)
{
	fl.takedamage = false;
	state = IDLE;

	maxAirTics = (pm_airTics.GetInteger() - AIR_DEFAULTAMOUNT) * AIRTANK_QUANTITY; //amount of oxygen inside the oxygen station.
	remainingAirTics = maxAirTics;
	armStartPos = 1;
	armEndPos = 1;
	renderEntity.shaderParms[7] = armEndPos;

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	Event_PlayAnim("idle", 0, true);


	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 24.0f;
	headlight.shaderParms[0] = 0.4f; // R
	headlight.shaderParms[1] = 0.4f; // G
	headlight.shaderParms[2] = 0.7f; // B
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlight.origin = GetPhysics()->GetOrigin() + (forward  * 16) + (up * 1) ;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT);

	animState = ANIM_IDLE;

	announceTimer = 0;
	canAnnounce = true;
	BecomeActive(TH_THINK);
	
}


void idOxygenStation::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( animTimer ); // int animTimer
	savefile->WriteInt( animState ); // int animState

	savefile->WriteInt( state ); // int state

	savefile->WriteInt( remainingAirTics ); // int remainingAirTics
	savefile->WriteInt( maxAirTics ); // int maxAirTics

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle


	savefile->WriteFloat( armStartPos ); // float armStartPos
	savefile->WriteFloat( armEndPos ); // float armEndPos

	savefile->WriteInt( announceTimer ); // int announceTimer
}

void idOxygenStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( animTimer ); // int animTimer
	savefile->ReadInt( animState ); // int animState

	savefile->ReadInt( state ); // int state

	savefile->ReadInt( remainingAirTics ); // int remainingAirTics
	savefile->ReadInt( maxAirTics ); // int maxAirTics

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadFloat( armStartPos ); // float armStartPos
	savefile->ReadFloat( armEndPos ); // float armEndPos

	savefile->ReadInt( announceTimer ); // int announceTimer
}

//void idOxygenStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}

void idOxygenStation::Think()
{
	idAnimatedEntity::Think();

	if (animState == ANIM_FROBBING)
	{
		float lerp = 1 - idMath::ClampFloat(0,1, (animTimer - gameLocal.time) / (float)FROB_ANIMTIME );
		lerp = idMath::CubicEaseInOut(lerp);

		

		renderEntity.shaderParms[7] = idMath::Lerp(armStartPos, armEndPos, lerp);
		UpdateVisuals();
		

		if (gameLocal.time >= animTimer)
		{
			if (remainingAirTics > 0)
			{
				Event_PlayAnim("idle", 16, true);
			}
			else
			{
				Event_PlayAnim("idle_off", 16);
			}

			animState = ANIM_IDLE;
		}
	}

	if (!gameLocal.InPlayerConnectedArea(this))
		return;

	if (gameLocal.time > announceTimer && remainingAirTics > 0)
	{
		announceTimer = gameLocal.time + ANNOUNCE_CHECKTIME;

		if (canAnnounce)
		{
			if (gameLocal.GetLocalPlayer()->GetAirtics() < pm_airTics.GetInteger() / 2) //only play it if player oxygen level is less than 50%
			{
				float distanceToPlayer = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();

				if (distanceToPlayer <= ANNOUNCE_ACTIVATIONRADIUS)
				{
					//do LOS check.

					trace_t eyeTr;
					gameLocal.clip.TracePoint(eyeTr, GetPhysics()->GetOrigin(), gameLocal.GetLocalPlayer()->firstPersonViewOrigin, MASK_SOLID, this);
					if (eyeTr.fraction >= 1.0f)
					{
						StartSound("snd_announce", SND_CHANNEL_VOICE);
						announceTimer = gameLocal.time + ANNOUNCE_COOLDOWNTIME;
						canAnnounce = false;

						//do particle
						idVec3 forward, up;
						GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
						idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * 8) + (up * 14);
						gameLocal.DoParticle("sound_burst.prt", particlePos);
					}
				}
			}
		}
		else
		{
			float distanceToPlayer = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();

			//wait for player to exit the radius.
			if (distanceToPlayer > ANNOUNCE_ACTIVATIONRADIUS + 16)
			{
				canAnnounce = true;
			}
		}
	}
}

bool idOxygenStation::DoFrob(int index, idEntity * frobber)
{
	if (frobber == gameLocal.GetLocalPlayer())
	{
		if (remainingAirTics <= 0)
		{
			//Player frobbed oxygenstation, but oxygenstation is empty.
			StartSound("snd_error", SND_CHANNEL_BODY);
			StartSound("snd_depleted", SND_CHANNEL_VOICE);

			idVec3 forward, right, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
			idVec3 dialPosition = GetPhysics()->GetOrigin() + (forward * 2.5f) + (right * -4) + (up * 8);
			gameLocal.DoParticle("sound_burst_small.prt", dialPosition);

			gameLocal.GetLocalPlayer()->ForceShowOxygenbar(); //Make the oxygen bar appear.

			return true;
		}

		if (gameLocal.GetLocalPlayer()->GetAirtics() >= pm_airTics.GetInteger())
		{
			//Player frobbed oxygenstation, but player third lung is already 100% full.
			
			gameLocal.GetLocalPlayer()->ForceShowOxygenbar(); //Make the oxygen bar appear.

			StartSound("snd_error", SND_CHANNEL_BODY3);
			return true;
		}

		

		int remainder = gameLocal.GetLocalPlayer()->GiveAirtics(remainingAirTics);		
		if (remainder <= 0)
		{
			//Air has been taken; station has no more air to give.

			StartSound("snd_empty", SND_CHANNEL_BODY);
			state = DEPLETED;
			Event_PlayAnim("idle_off", 8);
			StopSound(SND_CHANNEL_AMBIENT);
			

			SetColor(idVec4(1, 0, 0, 1));
			//if (headlightHandle != -1)
			//{
			//	headlight.shaderParms[0] = 1;
			//	headlight.shaderParms[1] = 0;
			//	headlight.shaderParms[2] = 0;
			//	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
			//}
		}
		else
		{
			//Air has been taken ; station has some more air left in its reserves.
			StartSound("snd_giveair", SND_CHANNEL_BODY);
		}

		remainingAirTics = remainder;

		
		//update meter visual.
		armStartPos = armEndPos;
		armEndPos = (remainingAirTics / (float)maxAirTics);


		//Play anim.
		animTimer = gameLocal.time + FROB_ANIMTIME;
		animState = ANIM_FROBBING;
		Event_PlayAnim("frob", 1);


		//Aim air particles toward player.
		DoParticleTowardPlayer("pipe1");
		DoParticleTowardPlayer("pipe2");
	}

	return true;
}

void idOxygenStation::DoParticleTowardPlayer(const char *jointname)
{
	idVec3 bodyPos;
	idMat3 bodyAxis;
	jointHandle_t bodyJoint = GetAnimator()->GetJointHandle(jointname);
	if (bodyJoint == INVALID_JOINT)
	{
		gameLocal.Error("oxygenstation '%s' can't find joint '%s'", GetName(), jointname);
	}
	GetJointWorldTransform(bodyJoint, gameLocal.time, bodyPos, bodyAxis);

	idAngles directionToPlayerEyes = ((gameLocal.GetLocalPlayer()->GetEyePosition() + idVec3(1,1,-4)) - bodyPos).ToAngles();
	directionToPlayerEyes.pitch += 90;

	gameLocal.DoParticle("oxy_puff_frob.prt", bodyPos, directionToPlayerEyes.ToForward());
}
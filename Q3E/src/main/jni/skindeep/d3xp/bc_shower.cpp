#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"

#include "bc_shower.h"


const int CLEANSE_INTERVAL = 300; //do cleaning check every xx milliseconds.

const int INTEREST_LIFETIME = 1000;
const int INTEREST_ENEMYFROBINDEX = 7;

CLASS_DECLARATION(idAnimatedEntity, idShower)
END_CLASS

idShower::idShower(void)
{
}

idShower::~idShower(void)
{
	StopSound(SND_CHANNEL_BODY3);

	if (waterParticle != NULL) {
		waterParticle->PostEventMS(&EV_Remove, 0);
		waterParticle = nullptr;
	}
}

void idShower::Spawn(void)
{
	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	fl.takedamage = true;
	state = SW_OFF;
	cleanseTimer = 0;
	interestTimer = 0;

	//Spawn water particle.
	idVec3 jointPos = GetJointPosition("showerhead");
	idAngles particleAngle;
	particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 135;
	idDict args;
	args.Clear();
	args.Set("model", "water_jet_loop.prt");
	args.SetVector("origin", jointPos);
	args.SetMatrix("rotation", particleAngle.ToMat3());
	args.SetBool("start_off", true);
	waterParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
}

idVec3 idShower::GetJointPosition(idStr jointname)
{
	jointHandle_t headJoint;
	idVec3 jointPos;
	idMat3 jointAxis;
	headJoint = animator.GetJointHandle(jointname.c_str());
	this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);

	return jointPos;
}


void idShower::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteObject( waterParticle ); // idFuncEmitter * waterParticle
	savefile->WriteInt( cleanseTimer ); // int cleanseTimer
	savefile->WriteInt( interestTimer ); // int interestTimer
}

void idShower::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadObject( CastClassPtrRef(waterParticle) ); // idFuncEmitter * waterParticle
	savefile->ReadInt( cleanseTimer ); // int cleanseTimer
	savefile->ReadInt( interestTimer ); // int interestTimer
}

void idShower::Think(void)
{
	if (state == SW_ON)
	{
		if (gameLocal.time >= cleanseTimer)
		{
			cleanseTimer = gameLocal.time + CLEANSE_INTERVAL;

			//do traceline.
			
			idVec3 jointPos = GetJointPosition("showerhead");			
			idVec3 forward, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
			idVec3 waterEndPos = jointPos + (forward * 128) + (up * -128);
			
			trace_t waterTr;
			gameLocal.clip.TracePoint(waterTr, jointPos, waterEndPos, MASK_SHOT_RENDERMODEL, NULL);

			if (waterTr.c.entityNum <= MAX_GENTITIES - 2 && waterTr.c.entityNum >= 0)
			{
				if (waterTr.c.entityNum == gameLocal.GetLocalPlayer()->entityNumber)
				{
					//hit player.

					if (gameLocal.GetLocalPlayer()->GetSmelly())
					{
						gameLocal.GetLocalPlayer()->SetSmelly(false);
					}

					if (gameLocal.GetLocalPlayer()->GetOnFire())
					{
						gameLocal.GetLocalPlayer()->SetOnFire(false);
						StartSound("snd_extinguish", SND_CHANNEL_ANY);
						gameLocal.GetLocalPlayer()->SetFanfareMessage("#str_def_gameplay_extinguished");
					}
				}
			}
		}

		if (gameLocal.time > interestTimer)
		{
			interestTimer = gameLocal.time + INTEREST_LIFETIME;
			gameLocal.SpawnInterestPoint(this, GetJointPosition("showerhead"), spawnArgs.GetString("interest_water"));
		}
		
	}
	
	idAnimatedEntity::Think();
}

//void idShower::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}

bool idShower::DoFrob(int index, idEntity * frobber)
{
	if (index == INTEREST_ENEMYFROBINDEX)
	{
		//The enemy is investigating the interestpoint and wants to turn off the shower.
		if (state == SW_ON)
		{
			gameLocal.DoParticle("frob_lines.prt", GetJointPosition("handle"));
			SetActive(false);
		}

		return true;
	}

	if (state == SW_OFF)
	{
		SetActive(true);
	}
	else if (state == SW_ON)
	{
		SetActive(false);
	}

	return true;
}

void idShower::SetActive(bool value)
{
	if (value)
	{
		//turn on.
		state = SW_ON;
		Event_PlayAnim("turnon", 0);
		waterParticle->SetActive(true);
		StartSound("snd_water", SND_CHANNEL_BODY3);
		BecomeActive(TH_THINK);
	}
	else
	{
		//turn off.
		state = SW_OFF;
		Event_PlayAnim("turnoff", 0);
		waterParticle->SetActive(false);
		StopSound(SND_CHANNEL_BODY3);
		BecomeInactive(TH_THINK);

		
		//do the dribble.
		StartSound("snd_dribble", SND_CHANNEL_ANY);
		idVec3 dribblePos = GetJointPosition("showerhead");
		gameLocal.DoParticle(spawnArgs.GetString("model_dribble", "water_faucet_dribble.prt"), dribblePos);
	}
}
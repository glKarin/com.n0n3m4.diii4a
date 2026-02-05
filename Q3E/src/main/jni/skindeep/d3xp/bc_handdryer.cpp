#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "physics/Physics_RigidBody.h"

#include "bc_frobcube.h"
#include "bc_handdryer.h"

const int FROBCOOLDOWN = 1800;
const int DEATHTIMEMAX = 2000;

CLASS_DECLARATION(idAnimated, idHanddryer)

END_CLASS


idHanddryer::idHanddryer(void)
{
}

idHanddryer::~idHanddryer(void)
{
}

void idHanddryer::Spawn(void)
{
	const char *clip;

	BecomeActive(TH_THINK);
	fl.takedamage = true;

	clip = spawnArgs.GetString("clipmodel", "");

	if (!collisionModelManager->TrmFromModel(clip, trm))
	{
		gameLocal.Error("idHanddryer '%s': cannot load collision model %s", name.c_str(), clip);
		return;
	}

	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	UpdateChangeableSpawnArgs(NULL);

	isFrobbable = true;
	frobTimer = 0;
	hasDied = false;
}

void idHanddryer::Save(idSaveGame *savefile) const
{
	savefile->WriteTraceModel( trm ); // idTraceModel trm
	savefile->WriteInt( frobTimer ); // int frobTimer
	savefile->WriteInt( deathTimer ); // int deathTimer
	savefile->WriteBool( hasDied ); // bool hasDied
}

void idHanddryer::Restore(idRestoreGame *savefile)
{
	savefile->ReadTraceModel( trm ); // idTraceModel trm
	savefile->ReadInt( frobTimer ); // int frobTimer
	savefile->ReadInt( deathTimer ); // int deathTimer
	savefile->ReadBool( hasDied ); // bool hasDied
}

void idHanddryer::Think(void)
{
	idAnimated::Think();

	if (!isFrobbable && health > 0)
	{
		float towelLerp = (frobTimer - gameLocal.time) / (float)FROBCOOLDOWN;

		this->SetShaderParm(5, idMath::Lerp(0, .5f, towelLerp));

		if (gameLocal.time > frobTimer)
		{
			isFrobbable = true;
		}		
	}

	if (health <= 0)
	{
		if (gameLocal.time > deathTimer && !hasDied)
		{
			idVec3 myDir;

			hasDied = true;
			
			//In order for radiusdamage to work, call these two lines first!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			Hide();
			GetPhysics()->SetContents(0);

			//BLOW up.
			this->PostEventMS(&EV_Remove, 100);

			idMoveableItem::DropItemsBurst(this, "gib", idVec3(8, 0, 0));

			myDir = GetPhysics()->GetAxis().ToAngles().ToForward();
			gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), -myDir, 8.0f, true, 40, "textures/decals/scorch");

			//explosion.
			idEntityFx::StartFx("fx/explosion", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
			gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, spawnArgs.GetString("def_explosiondamage"));
		}
	}
}


void idHanddryer::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idVec3 forwardDir;

	if (!fl.takedamage)
		return;

	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin(), spawnArgs.GetString("def_deathinterest"));

	deathTimer = gameLocal.time + DEATHTIMEMAX;

	isFrobbable = false;

	fl.takedamage = false;

	this->Event_PlayAnim("meltdown", 1, true);

	SetSkin(declManager->FindSkin("skins/models/objects/handdryer/skin_meltdown"));

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
	idEntityFx::StartFx("fx/machine_damaged_smokelight_2sec", GetPhysics()->GetOrigin() + forwardDir * 12, mat3_identity);	



	StartSound("snd_deathcharge", SND_CHANNEL_ANY);


	
	idVec3 particlePos = GetPhysics()->GetOrigin() + (forwardDir * 12);
	gameLocal.DoParticle(spawnArgs.GetString("model_deathcharge"), particlePos);

	
}

bool idHanddryer::DoFrob(int index, idEntity * frobber)
{
	idVec3 forwardDir, rightDir;
	idAngles particleAngle;

	if (gameLocal.time < frobTimer)
		return false;

	particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 60;

	frobTimer = gameLocal.time + FROBCOOLDOWN;
	isFrobbable = false;
	this->Event_PlayAnim("activate", 1);

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, NULL);
	idEntityFx::StartFx("fx/handdryer_sanitize", GetPhysics()->GetOrigin() + forwardDir * 6 + idVec3(0,0,6.1f) + rightDir * 8, particleAngle.ToMat3());
	idEntityFx::StartFx("fx/handdryer_sanitize", GetPhysics()->GetOrigin() + forwardDir * 6 + idVec3(0, 0, 6.1f) + rightDir * -8, particleAngle.ToMat3());

	StartSound("snd_wash", SND_CHANNEL_BODY, 0, false, NULL);

	if (frobber != NULL)
	{
		if (frobber == gameLocal.GetLocalPlayer())
		{
			gameLocal.GetLocalPlayer()->SetSmelly(false);
		}
	}

	return true;
}

void idHanddryer::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idAnimated::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	idVec3 forward;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idEntityFx::StartFx( spawnArgs.GetString("fx_damage"), GetPhysics()->GetOrigin() + forward * 12, mat3_identity);

}
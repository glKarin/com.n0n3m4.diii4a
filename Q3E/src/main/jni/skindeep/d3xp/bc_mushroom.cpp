#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "bc_meta.h"
#include "bc_mushroom.h"

const int HARVESTTIME = 2000;

const int DECAL_MINTIME = 500;
const int DECAL_VARTIME = 1000;
const float DECAL_POSITION_VARIANCE = 3.0f;


CLASS_DECLARATION(idAnimated, idMushroom)
END_CLASS

idMushroom::idMushroom(void)
{
	splashEnt = NULL;
}

idMushroom::~idMushroom(void)
{
	StopSound(SND_CHANNEL_BODY);

	if (splashEnt)
	{
		delete splashEnt;
	}
}

void idMushroom::Spawn(void)
{
	isFrobbable = true;
	fl.takedamage = true;
	BecomeActive(TH_THINK);
	decalTimer = 0;

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);


	//Figure out how much clearance for dripping we have. This is to help prevent particles from dripping "through" brushes.
	jointHandle_t headJoint = animator.GetJointHandle("head");
	idVec3 headPos;
	idMat3 headMat;
	this->GetJointWorldTransform(headJoint, gameLocal.time, headPos, headMat);
	trace_t headTr;
	gameLocal.clip.TracePoint(headTr, headPos, headPos + idVec3(0, 0, -1024), MASK_SOLID, this);
	headPosition = headPos;

	const char *particleName;
	float clearanceLength = (headTr.endpos - headPos).Length();
	if (clearanceLength >= 192)
	{
		dripType = DRIP_192;
		particleName = "mushroom_drip_192.prt";
	}
	else if (clearanceLength >= 128)
	{
		dripType = DRIP_128;
		particleName = "mushroom_drip_128.prt";
	}
	else
	{
		dripType = DRIP_64;
		particleName = "mushroom_drip_64.prt";
	}

	//dripping particle fx.
	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
	idDict splashArgs;
	splashArgs.Set("model", particleName);
	splashArgs.Set("start_off", "0");
	splashArgs.SetVector("origin", headPos);
	splashEnt = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));	
	
	StartSound("snd_idle", SND_CHANNEL_BODY);
}

void idMushroom::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( splashEnt ); // idFuncEmitter * splashEnt

	savefile->WriteInt( dripType ); // int dripType
	savefile->WriteVec3( headPosition ); // idVec3 headPosition

	savefile->WriteInt( decalTimer ); // int decalTimer
}

void idMushroom::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(splashEnt) ); // idFuncEmitter * splashEnt

	savefile->ReadInt( dripType ); // int dripType
	savefile->ReadVec3( headPosition ); // idVec3 headPosition

	savefile->ReadInt( decalTimer ); // int decalTimer
}

void idMushroom::Think(void)
{
	if (!gameLocal.InPlayerConnectedArea(this))
	{
		if (!splashEnt->IsHidden())
			splashEnt->Hide();

		return;
	}

	if (splashEnt->IsHidden())
		splashEnt->Show();

	if (gameLocal.time > decalTimer)
	{
		decalTimer = gameLocal.time + DECAL_MINTIME + gameLocal.random.RandomInt(DECAL_VARTIME);

		idVec3 decalSpawnPos = headPosition;
		decalSpawnPos.x = decalSpawnPos.x - (DECAL_POSITION_VARIANCE/2.0f) + (gameLocal.random.RandomFloat() * DECAL_POSITION_VARIANCE);
		decalSpawnPos.y = decalSpawnPos.y - (DECAL_POSITION_VARIANCE/2.0f) + (gameLocal.random.RandomFloat() * DECAL_POSITION_VARIANCE);
		trace_t decalTr;
		gameLocal.clip.TracePoint(decalTr, decalSpawnPos, decalSpawnPos + idVec3(0, 0, -1024), MASK_SHOT_RENDERMODEL, this);
		if (decalTr.fraction < 1)
		{
			gameLocal.ProjectDecal(decalTr.endpos, -decalTr.c.normal, 8.0f, true, 10 + gameLocal.random.RandomInt(10), "textures/decals/mushroom_floordrip", 3.14f);
		}
	}

	idAnimated::Think();
}



bool idMushroom::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
		return false;

	if (frobber == gameLocal.GetLocalPlayer())
	{		
		this->Hide();

		//Particles.
		idAngles particleAngle;
		particleAngle = GetPhysics()->GetAxis().ToAngles();
		particleAngle.pitch += 90;
		idEntityFx::StartFx("fx/mushroom_get", GetPhysics()->GetOrigin(), particleAngle.ToMat3());
			
		//Decal.
		const char *decalName = "textures/decals/bloodsplat00";
		if (decalName[0] != '\0')
		{
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			trace_t decalTr;
			gameLocal.clip.TracePoint(decalTr, GetPhysics()->GetOrigin() + forward * 1, GetPhysics()->GetOrigin() + forward * -16, MASK_SOLID, this);
			gameLocal.ProjectDecal(decalTr.endpos, -decalTr.c.normal, 8.0f, true, 20.0f, decalName);
		}

		gameLocal.GetLocalPlayer()->AddBloodMushroom(1, GetPhysics()->GetOrigin());
		PostEventMS(&EV_Remove, 0);		
	}

	return true;
}





//void idMushroom::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}
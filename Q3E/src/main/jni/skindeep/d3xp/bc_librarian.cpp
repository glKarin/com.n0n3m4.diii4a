#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"
#include "idlib/LangDict.h"

#include "SecurityCamera.h"
#include "bc_turret.h"
#include "bc_dozerhatch.h"

#include "bc_meta.h"
#include "bc_interestpoint.h"
#include "bc_librarian.h"

#define DETECTION_INTERVAL 200

#define DETECTION_INTERESTPOINT_LIFETIME 400

const idEventDef EV_Librarian_Subvert("librariansubvert", "d");

CLASS_DECLARATION(idAnimatedEntity, idLibrarian)
	EVENT(EV_Librarian_Subvert, idLibrarian::Event_Subvert)
END_CLASS

idLibrarian::idLibrarian(void)
{
}

idLibrarian::~idLibrarian(void)
{
	StopSound(SND_CHANNEL_ANY);

	if (waterParticle != NULL) {
		waterParticle->PostEventMS(&EV_Remove, 0);
		waterParticle = nullptr;
	}
}



void idLibrarian::Spawn(void)
{
	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	fl.takedamage = true;
	state = LBR_IDLE;
	stateTimer = 0;
	detectionTimer = 0;

	//Spawn water particle.
	idAngles particleAngle;
	particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 135;
	idDict args;
	args.Clear();
	args.Set("model", "water_jet_loop.prt");
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetMatrix("rotation", particleAngle.ToMat3());
	args.SetBool("start_off", true);
	waterParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));

	suspicionDelayTime = (int)(spawnArgs.GetFloat("suspicionDelay") * 1000.0f);
	suspicionTime = (int)(spawnArgs.GetFloat("suspicionTime") * 1000.0f);

	PostEventMS(&EV_PostSpawn, 0);
}

void idLibrarian::Event_PostSpawn(void)
{
	idLocationEntity *locEnt = gameLocal.LocationForEntity(this);
	if (locEnt == NULL)
	{
		gameLocal.Error("Librarian '%s' can't find its room location.", GetName());
		return;
	}

	myLocEntNum = locEnt->entityNumber;

	BecomeActive(TH_THINK);
}



void idLibrarian::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( myLocEntNum ); // int myLocEntNum

	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( detectionTimer ); // int detectionTimer
	savefile->WriteObject( waterParticle ); // idFuncEmitter * waterParticle

	savefile->WriteInt( suspicionDelayTime ); // int suspicionDelayTime
	savefile->WriteInt( suspicionTime ); // int suspicionTime
}

void idLibrarian::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( myLocEntNum ); // int myLocEntNum

	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( detectionTimer ); // int detectionTimer
	savefile->ReadObject( CastClassPtrRef(waterParticle) ); // idFuncEmitter * waterParticle

	savefile->ReadInt( suspicionDelayTime ); // int suspicionDelayTime
	savefile->ReadInt( suspicionTime ); // int suspicionTime
}

void idLibrarian::Think(void)
{
	idAnimatedEntity::Think();

	if (state == LBR_IDLE)
	{
		if (gameLocal.time > stateTimer)
		{
			stateTimer = gameLocal.time + DETECTION_INTERVAL;

			idEntity *noiseEnt = DetectNoiseInRoom();
			if (noiseEnt != NULL)
			{
				//I heard something.
				state = LBR_SUSPICIOUS_DELAY;
				stateTimer = gameLocal.time + suspicionDelayTime;
				StartSound("snd_warning", SND_CHANNEL_VOICE);
				gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin(), vec3_zero);

				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString(spawnArgs.GetString("str_heard")), noiseEnt->displayName.c_str()), GetPhysics()->GetOrigin());
				gameLocal.AddEventLog(spawnArgs.GetString("str_warning"), GetPhysics()->GetOrigin());
			}
		}
	}
	else if (state == LBR_SUSPICIOUS_DELAY)
	{
		//nothing happens here. Grace period before we start detecting again.
		if (gameLocal.time > stateTimer)
		{
			state = LBR_SUSPICIOUS;
			detectionTimer = 0;
			stateTimer = gameLocal.time + suspicionTime;
		}
	}
	else if (state == LBR_SUSPICIOUS)
	{
		if (gameLocal.time > detectionTimer)
		{
			detectionTimer = gameLocal.time + DETECTION_INTERVAL;

			idEntity *noiseEnt = DetectNoiseInRoom();
			if (noiseEnt != NULL)
			{
				//I heard something.
				state = LBR_ALERTED;
				StartSound("snd_alert", SND_CHANNEL_VOICE);
				gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin(), vec3_zero);

				static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(this); //Summon baddies. Go to combat state.

				//Spawn a security bot.
				gameLocal.AddEventLog("#str_def_gameplay_camera_summon", GetPhysics()->GetOrigin());
				if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SpawnSecurityBot(GetPhysics()->GetOrigin()))
				{
					gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin());
					StartSound("snd_vo_deploysecurity", SND_CHANNEL_VOICE);
				}

				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString(spawnArgs.GetString("str_heard")), noiseEnt->displayName.c_str()), GetPhysics()->GetOrigin());
				gameLocal.AddEventLog(spawnArgs.GetString("str_alert"), GetPhysics()->GetOrigin());
			}
		}

		if (gameLocal.time > stateTimer)
		{
			state = LBR_IDLE;
			stateTimer = gameLocal.time + DETECTION_INTERVAL;
			StartSound("snd_cooldown", SND_CHANNEL_VOICE);
			gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), GetPhysics()->GetOrigin(), vec3_zero);

			gameLocal.AddEventLog(spawnArgs.GetString("str_cooldown"), GetPhysics()->GetOrigin());
		}
	}
	else if (state == LBR_ALERTED)
	{

	}
}

idEntity *idLibrarian::DetectNoiseInRoom()
{
	for (idEntity* entity = gameLocal.interestEntities.Next(); entity != NULL; entity = entity->interestNode.Next())
	{
		idInterestPoint *interestpoint;

		if (!entity)
			continue;

		if (entity->IsHidden())
			continue;		

		interestpoint = static_cast<idInterestPoint *>(entity);

		if (!interestpoint)
			continue;

		if (interestpoint->interesttype != IPTYPE_NOISE)
			continue;
		
		//only detect an interestpoint in its first XXX ms of lifetime. 
		//This is so that we don't detect the same interestpoint twice, for warning and alert.
		if (gameLocal.time > interestpoint->GetCreationTime() + DETECTION_INTERESTPOINT_LIFETIME)
			continue;

		idLocationEntity *locEnt = gameLocal.LocationForEntity(interestpoint);
		if (locEnt == NULL)
			continue;

		if (locEnt->entityNumber != myLocEntNum)
			continue; //not in same room as me. skip.

		//Found a match.
		return interestpoint;		
	}

	return NULL;
}

void idLibrarian::Event_Subvert(int value)
{


	for (idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden())
			continue;

		idLocationEntity *locEnt = gameLocal.LocationForEntity(ent);
		if (locEnt == NULL)
		{
			continue; //no room info.........
		}

		if (locEnt->entityNumber != myLocEntNum)
			continue;

		//in same room.

		//Subvert cameras.
		if (ent->IsType(idSecurityCamera::Type))
		{
			static_cast<idSecurityCamera *>(ent)->SetTeam(TEAM_FRIENDLY);
		}

		//Subvert turrets.
		if (ent->IsType(idTurret::Type))
		{
			static_cast<idTurret *>(ent)->DoHack();
		}

		//Subvert hatches.
		if (ent->IsType(idDozerhatch::Type))
		{
			static_cast<idDozerhatch *>(ent)->DoHack();
		}
	}
}

//void idLibrarian::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}

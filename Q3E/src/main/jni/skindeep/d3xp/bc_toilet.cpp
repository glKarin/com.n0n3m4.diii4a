#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_toilet.h"

const int FLUSHTIME = 4000;
const int FLUSH_GRAB_INTERVAL = 300;

const int WATERJET_DOUSE_RANGE = 48;
const int WATERJET_DOUSE_INTERVAL = 300;

const int STRAINTIME = 2000;

CLASS_DECLARATION(idStaticEntity, idToilet)

END_CLASS


idToilet::idToilet(void)
{
	handle = nullptr;
	grabTimer = 0;
}

idToilet::~idToilet(void)
{
}

void idToilet::Spawn(void)
{
	idDict args;

	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	isFrobbable = true;

	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", "env_toilethandle");
	handle = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	handle->SetAngles(this->GetPhysics()->GetAxis().ToAngles());

	flushMode = 1;

	BecomeActive(TH_THINK);
	fl.takedamage = spawnArgs.GetBool("takedamage", "1");

	waterjetTimer = 0;

	

	state = TLT_IDLE;
	stateTimer = 0;
}

void idToilet::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( handle ); // idAnimated* handle
	savefile->WriteInt( flushTimer ); // int flushTimer
	savefile->WriteInt( flushMode ); // int flushMode
	savefile->WriteInt( waterjetTimer ); // int waterjetTimer
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( grabTimer ); // int grabTimer
}

void idToilet::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(handle) ); // idAnimated* handle
	savefile->ReadInt( flushTimer ); // int flushTimer
	savefile->ReadInt( flushMode ); // int flushMode
	savefile->ReadInt( waterjetTimer ); // int waterjetTimer
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( grabTimer ); // int grabTimer
}

void idToilet::Think(void)
{
	idStaticEntity::Think();

	if (gameLocal.time > flushTimer + FLUSHTIME && !isFrobbable && health > 0 && state == TLT_IDLE)
	{
		isFrobbable = true;
	}

	//Search for items to grab up while it is flushing.
	if (gameLocal.time > flushTimer  && gameLocal.time < flushTimer + FLUSHTIME && state == TLT_IDLE)
	{
		if (gameLocal.time > grabTimer)
		{
			grabTimer = gameLocal.time + FLUSH_GRAB_INTERVAL;
			GrabAndFlushObjects();

			PullObjectsCloser();
		}
	}

	//Douse out the player if they're on fire.
	if (health <= 0 && gameLocal.InPlayerPVS(this))
	{
		if (gameLocal.time > waterjetTimer)
		{
			waterjetTimer = gameLocal.time + WATERJET_DOUSE_INTERVAL;
			
			idVec3 waterOrigin = GetWaterjetPosition();

			if ((waterOrigin - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).Length() <= WATERJET_DOUSE_RANGE)
			{
				trace_t tr;
				gameLocal.clip.TracePoint(tr, waterOrigin + idVec3(0,0,4), gameLocal.GetLocalPlayer()->GetEyePosition(), MASK_SOLID, this);
				
				if (tr.fraction >= 1)
				{
					if (gameLocal.GetLocalPlayer()->SetOnFire(false)) //Using toilet puts out fire.
					{
						gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_extinguished");
						gameLocal.GetLocalPlayer()->StartSound("snd_extinguish", SND_CHANNEL_ANY);
					}
				}
			}			
		}
	}

	if (state == TLT_STRAINING)
	{
		if (gameLocal.time > stateTimer)
		{
			if (fl.takedamage || spawnArgs.GetBool("tutorialToilet", "0"))
			{
				state = TLT_STRAINDONE;
				fl.takedamage = true;

				//explode.
				Damage(NULL, NULL, vec3_zero, "damage_1000", 1.0f, 0);
			}
			else
			{
				state = TLT_IDLE;
			}
		}
	}
}


void idToilet::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!(fl.takedamage || spawnArgs.GetBool("tutorialToilet", "0")))
		return;	

	gameLocal.AddEventlogDeath(this, 0, inflictor, attacker, "", EL_DESTROYED);

	fl.takedamage = false;
	StopSound(SND_CHANNEL_ANY, false);

	StartSound("snd_break", SND_CHANNEL_BODY, 0, false, NULL);

	//TODO: replace collision with new flatter collision

	SetModel(spawnArgs.GetString("brokenmodel"));
	SetSkin(declManager->FindSkin(spawnArgs.GetString("brokenskin")));
	handle->PostEventMS(&EV_Remove, 0);
	handle = nullptr;
	isFrobbable = false;

	

	//Water jet visuals/audio
	StartSound("snd_waterjet", SND_CHANNEL_BODY2);
	gameLocal.DoParticle("water_jet_loop.prt", GetWaterjetPosition(), idVec3(0,90,0), false);


	idMoveableItem::DropItemsBurst(this, "gib", idVec3(0, 0, 32));
}

idVec3 idToilet::GetWaterjetPosition()
{
	idVec3 forwardDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
	return (GetPhysics()->GetOrigin() + forwardDir * 23 + idVec3(0, 0, 6));
}

bool idToilet::DoFrob(int index, idEntity * frobber)
{
	if (state != TLT_IDLE)
		return false;

	idVec3 forwardDir, particlePos;
	idAngles particleDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);

	particleDir = forwardDir.ToAngles();	

	isFrobbable = false;

	particlePos = GetPhysics()->GetOrigin() + (forwardDir * -12) + idVec3(0, 0, -1);

	if (flushMode == 1)
	{
		//Low flow mode.
		handle->Event_PlayAnim("flush_1", 0);
		particleDir.pitch += 85;
		idEntityFx::StartFx("fx/flush", particlePos, particleDir.ToMat3());
		flushMode = 2;
	}
	else
	{
		//#2 mode.
		handle->Event_PlayAnim("flush_2", 0);
		particleDir.pitch += 45;
		idEntityFx::StartFx("fx/flush2", particlePos, particleDir.ToMat3());
		flushMode = 1;
	}

	gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin() + idVec3(0, 0, 4), spawnArgs.GetString("interest_flush"));

	flushTimer = gameLocal.time;

	gameLocal.GetLocalPlayer()->SetSmelly(false);

	if (gameLocal.GetLocalPlayer()->SetOnFire(false)) //Using toilet puts out fire.
	{
		gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_extinguished");
		gameLocal.GetLocalPlayer()->StartSound("snd_extinguish", SND_CHANNEL_ANY);
	}

	

	return true;
}

void idToilet::GrabAndFlushObjects()
{
	idEntity	*entityList[MAX_GENTITIES];
	int			listedEntities, i;

	idBounds rawBounds = GetPhysics()->GetAbsBounds();
	rawBounds = rawBounds.Expand(1);
	rawBounds[1][2] = GetPhysics()->GetOrigin().z + 32;

	//gameRenderWorld->DebugBounds(colorGreen, rawBounds);

	listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(rawBounds, entityList, MAX_GENTITIES);

	if (listedEntities <= 0)
		return;


	//sanity check: do we have targets?
	FindTargets();
	RemoveNullTargets();
	if (targets.Num() <= 0)
	{
		gameLocal.Warning("idToilet '%s' doesn't have 'target' key specified.", name.c_str());
		return;
	}
	
	bool canAcceptMoreBigItems = true;
	
	for (i = 0; i < listedEntities; i++)
	{
		idEntity *ent = entityList[i];

		if (!ent)
		{
			continue;
		}

		if (ent == this || ent->IsHidden() || ent->IsType(idActor::Type))
			continue;
		
		if (!ent->IsType(idMoveableItem::Type))
			continue;

		//Ignore items currently being held by the player.
		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
			{
				continue;
			}
		}
			

		if (ent->spawnArgs.GetBool("breakstoilet"))
		{
			//this object breaks the toilet.
			if (!canAcceptMoreBigItems)
				continue;

			canAcceptMoreBigItems = false;
			state = TLT_STRAINING;
			stateTimer = gameLocal.time + STRAINTIME;
			StartSound("snd_strain", SND_CHANNEL_ANY);

			idVec3 forwardDir, upDir;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, &upDir);
			idVec3 strainPos = GetPhysics()->GetOrigin() + forwardDir * 14 + upDir * 12;

			gameLocal.DoParticle(spawnArgs.GetString("model_strain"), strainPos, idVec3(1, 0, 0));
		}

        //ejection FX for object.
        idStr ejectFX = ent->spawnArgs.GetString("fx_eject");
        if (ejectFX.Length() > 0)
        {
            //idEntityFx::StartFx(ejectFX, GetPhysics()->GetOrigin(), mat3_identity);
			 
			//BC 3-12-2025: Manually do the FX, so that we can give it a speakername.
			idDict args;
			args.SetBool("start", true);
			args.Set("fx", ejectFX.c_str());
			args.Set("speakername", "#str_speaker_pirate");
			idEntityFx* nfx = static_cast<idEntityFx*>(gameLocal.SpawnEntityType(idEntityFx::Type, &args));
			if (nfx)
			{
				nfx->SetOrigin(GetPhysics()->GetOrigin());

				// SW 17th March 2025: Playing the VO here instead of inside the FX so that we can correctly assign its channel and use the VO manager
				idStr ejectVO = ent->spawnArgs.GetString("snd_vo_eject");
				if (ejectVO.Length() > 0)
				{
					gameLocal.voManager.SayVO(nfx, ejectVO.c_str(), VO_CATEGORY_BARK);
				}
			}




			//BC 2-13-2025: player response.
			gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay_msDelayed("snd_vo_flush_skull", 1200);
        }

		//we now have an item.		
		idVec3 outputForward;
		targets[0].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&outputForward, NULL, NULL);
		idVec3 outputLocation = targets[0].GetEntity()->GetPhysics()->GetOrigin() + (outputForward * 32);
		
		

		idVec3 partAng = idVec3(1, 0, 0);
		gameLocal.DoParticle(spawnArgs.GetString("model_itemsplash"), ent->GetPhysics()->GetOrigin(), partAng);

		ent->Teleport(outputLocation, ent->GetPhysics()->GetAxis().ToAngles(), targets[0].GetEntity());
		ent->GetPhysics()->ApplyImpulse(0, ent->GetPhysics()->GetAbsBounds().GetCenter(), outputForward * 48 * ent->GetPhysics()->GetMass()); //give it a little push.

		
	}
}

void idToilet::PullObjectsCloser()
{
	//Pull objects closer toward toilet.

	idEntity* entityList[MAX_GENTITIES];
	int			listedEntities, i;

	idBounds rawBounds = GetPhysics()->GetAbsBounds();
	rawBounds = rawBounds.Expand(32);
	rawBounds[1][2] = GetPhysics()->GetOrigin().z + 16;

	//gameRenderWorld->DebugBounds(colorGreen, rawBounds, vec3_origin, 500);

	listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(rawBounds, entityList, MAX_GENTITIES);

	if (listedEntities <= 0)
		return;

	for (i = 0; i < listedEntities; i++)
	{
		idEntity* ent = entityList[i];

		if (!ent)
		{
			continue;
		}

		if (ent == this || ent->IsHidden() || ent->health <= 0)
			continue;

		if (!ent->IsType(idMoveableItem::Type))
			continue;

		//Ignore items currently being held by the player.
		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
			{
				continue;
			}
		}
		
		idVec3 suckPoint = GetPhysics()->GetOrigin() + idVec3(0, 0, 6);

		//do LOS check.
		trace_t tr;
		gameLocal.clip.TracePoint(tr, suckPoint, ent->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);

		if (tr.fraction < 1)
			continue; //no LOS, so exit.

		#define TOILET_SUCK_FORCE 16
		idVec3 impulseDirection = suckPoint - ent->GetPhysics()->GetOrigin();
		impulseDirection.Normalize();
		ent->GetPhysics()->ApplyImpulse(0, ent->GetPhysics()->GetAbsBounds().GetCenter(), impulseDirection * TOILET_SUCK_FORCE * ent->GetPhysics()->GetMass()); //give it a little push.
	}
}
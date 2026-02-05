#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"
#include "idlib/LangDict.h"


#include "bc_key.h"
#include "bc_gunner.h"
#include "bc_doorbarricade.h"

#define KEYREQUIRE_LIGHTTIME 2000
#define PROXIMITY_CHECKTIME 300
#define PROXIMITY_RADIUS	270
#define BEAMWIDTH			12
#define BEAM_MAXLENGTH		96
#define BEAM_ACTOR_VERTICALOFFSET 40

#define PARM_REQUIRELIGHT	5
#define PARM_PROXIMITY		7

CLASS_DECLARATION(idStaticEntity, idDoorBarricade )
END_CLASS

idDoorBarricade::idDoorBarricade(void)
{
	particleDone = false;
	beamOrigin = nullptr;
	beamTarget = nullptr;

	//BC 4-12-2025: locboxes
	for (int i = 0; i < 2; i++)
	{
		locboxes[i] = nullptr;
	}
}

idDoorBarricade::~idDoorBarricade(void)
{
	delete beamOrigin;
	delete beamTarget;

	//BC 4-12-2025: locboxes
	for (int i = 0; i < 2; i++)
	{
		delete locboxes[i];
	}
}

void idDoorBarricade::Spawn(void)
{
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	team = spawnArgs.GetInt("team", "1");
	
	idVec3 keycolor = spawnArgs.GetVector("_color");
	renderEntity.shaderParms[SHADERPARM_RED] = keycolor[0];
	renderEntity.shaderParms[SHADERPARM_GREEN] = keycolor[1];
	renderEntity.shaderParms[SHADERPARM_BLUE] = keycolor[2];
	UpdateVisuals();

	proximityTimer = 0;
	proximityActive = false;
	keyrequirementTimer = 0;

	idDict args;
	args.Clear();
	args.SetVector("origin", vec3_zero);
	args.SetFloat("width", BEAMWIDTH);
	beamTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	//beamTarget->BecomeActive(TH_PHYSICS);

	args.Clear();
	args.Set("target", beamTarget->name.c_str());
	args.SetBool("start_off", false);
	args.SetVector("origin", vec3_zero);
	args.SetFloat("width", BEAMWIDTH);
	args.Set("skin", spawnArgs.GetString("skin_beam"));
	beamOrigin = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	//beamOrigin->BindToJoint(this, joint, false);
	//wireOrigin->BecomeActive(TH_PHYSICS);
	beamOrigin->Hide();

	BecomeActive(TH_THINK);

	//BC 4-12-2025: locboxes spawn	
	#define LOCBOXRADIUS 4
	idStr requiredKey = spawnArgs.GetString("requires"); //get key required by barricade
	bool requiresBlueKey = requiredKey.Find("blue") >= 0; //determine if it needs blue key or not
	idStr keyText = requiresBlueKey ? "#str_def_gameplay_requiresbluekey" : "#str_def_gameplay_requiresyellowkey"; //set the locbox text

	idVec3 forward, right;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, NULL);
	int locboxOffsetForward = spawnArgs.GetInt("locbox_fwd");
	int locboxOffsetRight = spawnArgs.GetInt("locbox_right");

	for (int i = 0; i < 2; i++)
	{
		idVec3 locboxPos = GetPhysics()->GetOrigin();

		//Set position of the locbox on the barricade.
		if (i <= 0)
			locboxPos += (forward * locboxOffsetForward) + (right * locboxOffsetRight);
		else
			locboxPos += (forward * -locboxOffsetForward) + (right * -locboxOffsetRight);

		args.Clear();
		args.Set("text", common->GetLanguageDict()->GetString(keyText.c_str()) );
		args.SetVector("origin", locboxPos);
		args.SetBool("playerlook_trigger", true);
		args.SetFloat("locboxDistScale", 3.0f);
		args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
		args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
		locboxes[i] = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));
		locboxes[i]->Bind(this, false);
	}
}

void idDoorBarricade::Save(idSaveGame *savefile) const
{
	owningDoor.Save( savefile ); //  idEntityPtr<idDoor> owningDoor

	savefile->WriteInt( keyrequirementTimer ); //  int keyrequirementTimer
	savefile->WriteBool( keyrequirementLightActive ); //  bool keyrequirementLightActive
	savefile->WriteInt( proximityTimer ); //  int proximityTimer
	savefile->WriteBool( proximityActive ); //  bool proximityActive

	savefile->WriteObject( beamOrigin ); //  idBeam* beamOrigin
	savefile->WriteObject( beamTarget ); //  idBeam* beamTarget

	savefile->WriteBool( particleDone ); //  bool particleDone

	//BC 4-12-2025: locboxes
	SaveFileWriteArray(locboxes, 2, WriteObject);
}

void idDoorBarricade::Restore(idRestoreGame *savefile)
{
	owningDoor.Restore( savefile ); //  idEntityPtr<idDoor> owningDoor

	savefile->ReadInt( keyrequirementTimer ); //  int keyrequirementTimer
	savefile->ReadBool( keyrequirementLightActive ); //  bool keyrequirementLightActive
	savefile->ReadInt( proximityTimer ); //  int proximityTimer
	savefile->ReadBool( proximityActive ); //  bool proximityActive

	savefile->ReadObject( CastClassPtrRef(beamOrigin) ); //  idBeam* beamOrigin
	savefile->ReadObject( CastClassPtrRef(beamTarget) ); //  idBeam* beamTarget

	savefile->ReadBool( particleDone ); //  bool particleDone

	//BC 4-12-2025: locboxes
	SaveFileReadArrayCast(locboxes, ReadObject, idClass*&);
}

void idDoorBarricade::Think(void)
{
	idStaticEntity::Think();

	if (gameLocal.time > keyrequirementTimer && keyrequirementLightActive)
	{
		keyrequirementLightActive = false;
		renderEntity.shaderParms[PARM_REQUIRELIGHT] = 0;
		UpdateVisuals();
	}

	if (gameLocal.time > proximityTimer && gameLocal.InPlayerConnectedArea(this))
	{
		proximityTimer = gameLocal.time + PROXIMITY_CHECKTIME;

		if (UpdateProximity())
		{
			if (!proximityActive)
			{
				proximityActive = true;
				renderEntity.shaderParms[PARM_PROXIMITY] = 1;
				UpdateVisuals();

				StartSound("snd_keyblink", SND_CHANNEL_BODY3);
			}
		}
		else if (proximityActive)
		{
			if (!beamOrigin->IsHidden())
				beamOrigin->Hide();

			proximityActive = false;
			renderEntity.shaderParms[PARM_PROXIMITY] = 0;
			UpdateVisuals();

			StopSound(SND_CHANNEL_BODY3);
		}
	}
}

bool idDoorBarricade::UpdateProximity()
{
	//Keys can be in 3 states:
	//1. on a guard.
	//2. on player.
	//3. sitting on ground.
	//(technically they can also be in lost and found machine, but we're ignoring that)

	//Check for these 3 possibilities.

	if (DoProxCheckPlayer())
		return true;

	if (DoProxCheckAI())
		return true;

	if (DoProxCheckGround())
		return true;

	return false;
}

bool idDoorBarricade::DoProxCheckAI()
{
	idStr barricadeRequirement = spawnArgs.GetString("requires");
	for (idEntity* ent = gameLocal.aimAssistEntities.Next(); ent != NULL; ent = ent->aimAssistNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden())
			continue;

		if (!ent->IsType(idAI::Type))
			continue;

		int attachmentCount = static_cast<idAI *>(ent)->attachmentsToDrop.Num();
		if (attachmentCount > 0)
		{
			for (int i = 0; i < attachmentCount; i++)
			{
				if (static_cast<idAI *>(ent)->attachmentsToDrop[i].ent.IsValid())
				{
					idStr invName = static_cast<idAI *>(ent)->attachmentsToDrop[i].ent.GetEntity()->spawnArgs.GetString("inv_name");
					if (invName.Length() > 0)
					{
						if (!idStr::Icmp(barricadeRequirement.c_str(), ent->spawnArgs.GetString("inv_name")))
						{
							UpdateLaserPosition(ent->GetPhysics()->GetOrigin() + idVec3(0, 0, BEAM_ACTOR_VERTICALOFFSET));
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool idDoorBarricade::DoProxCheckGround()
{		
	idStr barricadeRequirement = spawnArgs.GetString("requires");

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idSecurityKey::Type) || ent->IsHidden())
			continue;

		float dist = (ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length();
		if (dist > PROXIMITY_RADIUS)
			continue;

		//LOS check.
		trace_t tr;
		gameLocal.clip.TracePoint(tr, GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), MASK_SOLID, this);
		if (tr.fraction < 1)
			continue;		

		if (!idStr::Icmp(barricadeRequirement.c_str(), ent->spawnArgs.GetString("inv_name")))
		{
			//Match.
			UpdateLaserPosition(ent->GetPhysics()->GetOrigin());
			return true;
		}
	}

	return false;
}

bool idDoorBarricade::DoProxCheckPlayer()
{
	float dist = (gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length();
	if (dist > PROXIMITY_RADIUS)
		return false; //player too far.
	
	//check if player has key.
	if (gameLocal.RequirementMet_Inventory(gameLocal.GetLocalPlayer(), spawnArgs.GetString("requires"), 0))
	{
		UpdateLaserPosition(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0, BEAM_ACTOR_VERTICALOFFSET));
		return true;
	}

	return false;
}


void idDoorBarricade::UpdateLaserPosition(idVec3 targetPos)
{
	if (beamOrigin->IsHidden())
		beamOrigin->Show();

	//Position the origin.
	float offsetDist = spawnArgs.GetFloat("frobdooroffset");

	idVec3 forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);

	idVec3 pos1 = GetPhysics()->GetOrigin() + (forward * offsetDist) + idVec3(0,0,3.5f);
	idVec3 pos2 = GetPhysics()->GetOrigin() + (forward * -offsetDist) + idVec3(0, 0, 3.5f);

	float distPos1 = (pos1 - targetPos).Length();
	float distPos2 = (pos2 - targetPos).Length();

	float currentDist;
	if (distPos1 < distPos2)
	{
		beamOrigin->SetOrigin(pos1);
		currentDist = distPos1;
	}
	else
	{
		beamOrigin->SetOrigin(pos2);
		currentDist = distPos2;
	}

	//Position the target.
	if (currentDist <= BEAM_MAXLENGTH)
	{
		beamTarget->SetOrigin(targetPos);
	}
	else
	{
		//Clamp the laser length.
		idVec3 beamDirection = targetPos - beamOrigin->GetPhysics()->GetOrigin();
		beamDirection.Normalize();

		idVec3 targetPos = beamOrigin->GetPhysics()->GetOrigin() + (beamDirection * BEAM_MAXLENGTH);
		beamTarget->SetOrigin(targetPos);
	}
}


bool idDoorBarricade::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
	{
		return false;
	}

	// success conditions:
	// - the player has the keycard required to unlock the barricade
	// - the door itself is frobbing the barricade (this indicates the door is voluntarily releasing the barricade)
    if (gameLocal.RequirementMet_Inventory(frobber, spawnArgs.GetString("requires"), 1) || frobber == owningDoor.GetEntity())
    {
		if (spawnArgs.GetBool("unlocks_all", "0"))
		{
			//Unlock all barricades of the same key requirement.
			UnlockAllBarricades();

			idStr displaytext = spawnArgs.GetString("msg_unlock");
			if (displaytext.Length() > 0)
			{
				gameLocal.AddEventLog(displaytext.c_str(), GetPhysics()->GetOrigin());
			}
		}
		else
		{
			UnlockBarricade();
		}

		StartSound("snd_vo_unlock", SND_CHANNEL_VOICE);

		return true;
    }
    else
    {
        //play error sound.
        StartSound("snd_locked", SND_CHANNEL_BODY, 0, false, NULL);
		
		//failed to meet requirements. Show message.
		int showMessage = spawnArgs.GetInt("showmessage", "1");
		if (showMessage == 1)
		{
			keyrequirementLightActive = true;
			keyrequirementTimer = gameLocal.time + KEYREQUIRE_LIGHTTIME;

			renderEntity.shaderParms[PARM_REQUIRELIGHT] = 1;
			UpdateVisuals();


			//sound wave.
			idStr soundwaveModel = spawnArgs.GetString("model_locksound");
			if (soundwaveModel.Length() > 0)
			{
				idVec3 forward, right, up, soundOffset;
				GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

				soundOffset = spawnArgs.GetVector("locksoundoffset", "0 0 0");
				
				for (int i = 0; i < 2; i++)
				{
					idVec3 soundPos = GetPhysics()->GetOrigin() + (up * soundOffset.z);

					if (i <= 0)
						soundPos += (forward * soundOffset.y) + (right * soundOffset.x);
					else
						soundPos += (forward * -(soundOffset.y)) + (right * -(soundOffset.x));

					gameLocal.DoParticle(soundwaveModel.c_str(), soundPos);
				}
			}
		}
		else if (showMessage == 2)
		{
			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_unlockedelsewhere");
		}
    }

	return false;
}

void idDoorBarricade::UnlockAllBarricades()
{
	//unlock message.



	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden() || !ent->IsType(idDoorBarricade::Type))
			continue;

		if (idStr::Icmp(ent->spawnArgs.GetString("requires"), spawnArgs.GetString("requires")))
			continue;


		static_cast<idDoorBarricade *>(ent)->UnlockBarricade();		
	}

	


}

void idDoorBarricade::UnlockBarricade()
{
	//Hide();
	//
	//if (fl.hidden)
	//	return;

	StopSound(SND_CHANNEL_BODY3);

	if (owningDoor.IsValid())
	{
		owningDoor.GetEntity()->RemoveBarricade();

		if (!particleDone)
		{
			StartSound("snd_unlocked", SND_CHANNEL_BODY);
		}
	}

	if (!particleDone)
	{
		particleDone = true;

		for (int i = 0; i < 2; i++)
		{
			//Play particle effect.
			float offsetDist = spawnArgs.GetFloat("frobdooroffset");
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			idVec3 pos = GetPhysics()->GetOrigin() + (forward * (i <= 0 ? offsetDist : -offsetDist));
			gameLocal.DoParticle(spawnArgs.GetString("model_unlockparticle"), pos);


			
			//And also spawn the metal bar.
			idEntity* barEnt;
			idDict args;
			args.Set("classname", spawnArgs.GetString("def_metalbar"));
			args.SetVector("origin", pos);
			args.SetMatrix("rotation", GetPhysics()->GetAxis());
			gameLocal.SpawnEntityDef(args, &barEnt);

			//now make it fly.
			if (barEnt)
			{

				#define FLINGFORCE 32
				#define UPWARDFORCE 128
				#define RANDOMVARIANCE 32

				#define	ANGULARFORCE 256
				idVec3 flingDirection = pos - GetPhysics()->GetOrigin();
				flingDirection.Normalize();

				barEnt->GetPhysics()->SetLinearVelocity((flingDirection * (FLINGFORCE + gameLocal.random.RandomInt(RANDOMVARIANCE))) + idVec3(0, 0, UPWARDFORCE + gameLocal.random.RandomInt(RANDOMVARIANCE)));

				//Spin the bar.
				idAngles barAng = barEnt->GetPhysics()->GetAxis().ToAngles();
				barAng.pitch = 10;
				barAng.roll = 0;
				barAng.yaw += 90;
				barEnt->GetPhysics()->SetAngularVelocity(barAng.ToForward() * ANGULARFORCE);
			}



			
		}
	}
}

void idDoorBarricade::DoHack()
{
	gameLocal.AddEventLog("#str_def_gameplay_hackdoor", GetPhysics()->GetOrigin());
	//UnlockBarricade();
	UnlockAllBarricades();
}
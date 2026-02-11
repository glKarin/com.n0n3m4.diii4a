#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"

#include "idlib/LangDict.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"

#include "bc_catcage.h"




#include "WorldSpawn.h"

#include "ai/AI.h"
#include "bc_pirateship.h"
#include "bc_meta.h"
#include "bc_frobcube.h"
#include "bc_lifeboat_catpod.h"
#include "bc_catpod_interior.h"

#define FROBINDEX_EXIT 333

#define FANFAREDELAY 1000


#define TRACKER_SPAWNDELAYTIME 500
#define TRACKER_VO_GAP 100

#define TRACKER_INITIALPAUSE 1000
#define TRACKER_BARTIME 2000

CLASS_DECLARATION(idStaticEntity, idCatpodInterior)
END_CLASS

idCatpodInterior::idCatpodInterior(void)
{
	lastPlayerPosition = vec3_zero;
	lastPlayerViewangle = idAngles(0, 0, 0);
	frobBar = NULL;

	fanfareTimer = 0;
	fanfareState = FF_DORMANT;

	totalcatInhabitants = 0;

	for (int i = 0; i < CAT_INHABITANTS_MAX; i++)
	{
		catprisonerModel[i] = nullptr;
	}

	playerIsInCatpod = false;
	
	trackerTimer = 0;
	trackerModel = nullptr;
	trackerSequence = TRK_NONE;
	trackerProgressState = TPS_DORMANT;
	trackerProgressStartValue = 0;
	trackerProgressEndValue = 0;
	trackerProgressTimer = 0;
}

idCatpodInterior::~idCatpodInterior(void)
{
}

void idCatpodInterior::Spawn(void)
{
	state = IDLE;
	fl.takedamage = false;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);	

	//Spawn light.
	idDict args;
	args.Clear();
	args.Set("classname", "light");
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + idVec3(0,0,64));
	//args.SetVector("light_center", idVec3(0, 0, 16));
	args.SetInt("noshadows", 1);
	ceilingLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &args);
	ceilingLight->SetRadius(80);
	ceilingLight->SetColor(idVec4(0.5f, 0.5f, 0.4f, 1));	
	
	fanfareTimer = gameLocal.time + 1500;

	// SW 3rd April 2025: Make sure the cat pod interior has a valid location
	if (gameLocal.LocationForEntity(this) == NULL)
	{
		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin());
		args.Set("location", "#str_def_gameplay_100044"); // "Escape pod"
		gameLocal.SpawnEntityType(idLocationEntity::Type, &args);
	}

	PostEventMS(&EV_PostSpawn, 100);
}

void idCatpodInterior::Event_PostSpawn(void)
{
	//Populate the cat seats.

	//These are the forward/right/up positions of the seats inside the pod.
	idVec3 seatPositions[CAT_INHABITANTS_MAX] =
	{
		idVec3(16,  -39,    44),
		idVec3(0,  -39,    44),
		idVec3(-16,  -39,    44),


		idVec3(16, 39,    20),
		idVec3(0, 39,    20),
		idVec3(-16, 39,    20)
	};

	trackerSequence = (gameLocal.world->spawnArgs.GetBool("tracker_in_pod")) ? TRK_WAITINGFORPLAYER : TRK_NONE;

	int currentCatIndex = 0;

	idStr catSpeakername = "";
	int catVoiceprint = 0;

	for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
	{
		if (!cageEnt)
			continue;

		if (!cageEnt->IsType(idCatcage::Type))
			continue;

		idStr catmodelname = cageEnt->spawnArgs.GetString("model_cat");		
		
		if (catmodelname.Length() <= 0)
			continue;

		if (trackerSequence == TRK_WAITINGFORPLAYER && currentCatIndex <= 0)
		{
			catSpeakername = gameLocal.GetCatViaModel(catmodelname, &catVoiceprint);
		}

		idAngles catAngle = GetPhysics()->GetAxis().ToAngles();

		if (currentCatIndex <= 2)
			catAngle.yaw += -90;
		else
			catAngle.yaw += 90;

		idVec3 currentSeatPos = seatPositions[currentCatIndex];
		idVec3 forward, right, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		currentSeatPos = GetPhysics()->GetOrigin() + (forward * currentSeatPos.x) + (right * currentSeatPos.y) + (up * currentSeatPos.z);

		idDict args;
		args.Clear();
		args.SetVector("origin", currentSeatPos);
		args.SetFloat("angle", catAngle.yaw);
		args.Set("classname", "monster_npc_cat");
		args.Set("model", catmodelname.c_str());
		args.Set("anim", "sitting");
		args.SetInt("cycle", -1);
		args.SetInt("notriggeronanim", 1);
		args.SetFloat("wait", .1f);
		args.Set("speakername", catSpeakername.c_str());
		args.SetInt("voiceprint", catVoiceprint);
		catprisonerModel[currentCatIndex] = (idAnimated*)gameLocal.SpawnEntityType(idAnimated::Type, &args);

		currentCatIndex++;

		if (currentCatIndex >= CAT_INHABITANTS_MAX - 1)
		{
			break; //no more slots. All maxed out.
		}
	}

	totalcatInhabitants = currentCatIndex;


	
	if (trackerSequence == TRK_WAITINGFORPLAYER)
	{
		//spawn the tracker object.
		idVec3 forward, right, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		idVec3 trackerPosition = GetPhysics()->GetOrigin() + (forward * 15) + (right * -23) + (up * 54);

		idAngles trackerAngle = GetPhysics()->GetAxis().ToAngles();
		trackerAngle.yaw -= 90;

		idDict args;
		args.Clear();
		args.SetVector("origin", trackerPosition);
		args.SetMatrix("rotation", trackerAngle.ToMat3());
		args.Set("classname", "func_animated");
		args.Set("model", "model_tracker");
		args.Set("anim", "idle");
		args.SetInt("cycle", -1);
		args.SetInt("notriggeronanim", 1);
		args.SetFloat("wait", .1f);
		args.SetBool("hide", 1);
		args.Set("snd_trackerreveal", "notification");
		trackerModel = (idAnimated*)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	}
}



void idCatpodInterior::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state

	savefile->WriteObject( ceilingLight ); //  idLight *				 ceilingLight

	savefile->WriteObject( frobBar ); //  idEntity*				 frobBar

	savefile->WriteVec3( lastPlayerPosition ); //  idVec3 lastPlayerPosition
	savefile->WriteAngles( lastPlayerViewangle ); //  idAngles lastPlayerViewangle


	savefile->WriteInt( fanfareTimer ); //  int fanfareTimer
	savefile->WriteInt( fanfareState ); //  int fanfareState

	SaveFileWriteArray(catprisonerModel, CAT_INHABITANTS_MAX, WriteObject); // idAnimated* catprisonerModel[CAT_INHABITANTS_MAX];
	savefile->WriteInt( totalcatInhabitants ); //  int totalcatInhabitants
	savefile->WriteBool( playerIsInCatpod ); //  bool playerIsInCatpod

	savefile->WriteInt( trackerSequence ); //  int trackerSequence

	savefile->WriteObject( trackerModel ); //  idAnimated*				 trackerModel
	savefile->WriteInt( trackerTimer ); //  int trackerTimer

	savefile->WriteFloat( trackerProgressStartValue ); //  float trackerProgressStartValue
	savefile->WriteFloat( trackerProgressEndValue ); //  float trackerProgressEndValue
	savefile->WriteInt( trackerProgressState ); //  int trackerProgressState
	savefile->WriteInt( trackerProgressTimer ); //  int trackerProgressTimer
}

void idCatpodInterior::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state

	savefile->ReadObject( CastClassPtrRef(ceilingLight) ); //  idLight *				 ceilingLight

	savefile->ReadObject( frobBar ); //  idEntity*				 frobBar

	savefile->ReadVec3( lastPlayerPosition ); //  idVec3 lastPlayerPosition
	savefile->ReadAngles( lastPlayerViewangle ); //  idAngles lastPlayerViewangle


	savefile->ReadInt( fanfareTimer ); //  int fanfareTimer
	savefile->ReadInt( fanfareState ); //  int fanfareState

	SaveFileReadArrayCast( catprisonerModel, ReadObject, idClass*& ); // idAnimated* catprisonerModel[CAT_INHABITANTS_MAX];
	savefile->ReadInt( totalcatInhabitants ); //  int totalcatInhabitants
	savefile->ReadBool( playerIsInCatpod ); //  bool playerIsInCatpod

	savefile->ReadInt( trackerSequence ); //  int trackerSequence

	savefile->ReadObject( CastClassPtrRef(trackerModel) ); //  idAnimated*				 trackerModel
	savefile->ReadInt( trackerTimer ); //  int trackerTimer

	savefile->ReadFloat( trackerProgressStartValue ); //  float trackerProgressStartValue
	savefile->ReadFloat( trackerProgressEndValue ); //  float trackerProgressEndValue
	savefile->ReadInt( trackerProgressState ); //  int trackerProgressState
	savefile->ReadInt( trackerProgressTimer ); //  int trackerProgressTimer
}

void idCatpodInterior::Think(void)
{	
	if (fanfareState == FF_WAITING && gameLocal.time > fanfareTimer)
	{
		fanfareState = FF_DONE;
		Event_GuiNamedEvent(1, "startFanfare");
	}

	//make the cats look at player.
	if (totalcatInhabitants > 0 && playerIsInCatpod)
	{
		for (int i = 0; i < totalcatInhabitants; i++)
		{
			//make cat look at player.
			catprisonerModel[i]->Event_TurnJointToward("head", gameLocal.GetLocalPlayer()->GetEyePosition());
		}
	}


	if (trackerSequence == TRK_SPAWNDELAY)
	{
		if (gameLocal.time - trackerTimer > TRACKER_SPAWNDELAYTIME)
		{
			trackerModel->Show();
			trackerModel->Event_PlayAnim("idle", 0, true);

			gameLocal.DoParticle(spawnArgs.GetString("model_trackersparkle"), trackerModel->GetPhysics()->GetOrigin());
			trackerModel->StartSound("snd_trackerreveal", SND_CHANNEL_ANY);
			trackerSequence = TRK_VOLINE_1;

			//Cat: say the first VO Line.
			if (totalcatInhabitants > 0)
			{
				idStr catVOline = gameLocal.world->spawnArgs.GetString("tracker_vo1");
				if (catVOline.Length() > 0)
				{

					//hack
					//modify the vo line to use the correct voiceprint (ascertained in GetCatViaModel())
					int voiceprint = catprisonerModel[0]->spawnArgs.GetInt("voiceprint");					
					catVOline = idStr::Format("%s_%s", (voiceprint <= 0) ? "cat_a" : "cat_b", catVOline.c_str());
					


					int len = 0;
					catprisonerModel[0]->StartSoundShader(declManager->FindSound(catVOline.c_str()), SND_CHANNEL_VOICE, 0, false, &len);
					trackerTimer = gameLocal.time + len + TRACKER_VO_GAP;

					//play sound particle.
					idStr soundparticle = spawnArgs.GetString("model_soundwave");
					if (soundparticle.Length() > 0)
					{
						idMat3 hitAxis;
						idVec3 hitOrigin;
						jointHandle_t hitJoint = catprisonerModel[0]->GetAnimator()->GetJointHandle("head");
						catprisonerModel[0]->GetAnimator()->GetJointTransform(hitJoint, gameLocal.time, hitOrigin, hitAxis);
						hitOrigin = catprisonerModel[0]->GetPhysics()->GetOrigin() + hitOrigin * catprisonerModel[0]->GetPhysics()->GetAxis();

						gameLocal.DoParticle(soundparticle.c_str(), hitOrigin);
					}
				}
			}

			trackerProgressState = TPS_STARTPAUSE;
			trackerProgressTimer = gameLocal.time;

			trackerProgressStartValue = gameLocal.world->spawnArgs.GetFloat("tracker_start");
			trackerProgressEndValue = gameLocal.world->spawnArgs.GetFloat("tracker_end");

			trackerModel->GetRenderEntity()->shaderParms[7] = trackerProgressStartValue;
			trackerModel->UpdateVisuals();
		}
	}
	else if (trackerSequence == TRK_VOLINE_1)
	{
		if (gameLocal.time >= trackerTimer)
		{
			trackerSequence = TRK_DONE;
			idStr ninaVOline = gameLocal.world->spawnArgs.GetString("tracker_vo2");
			if (ninaVOline.Length() > 0)
			{
				gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), ninaVOline, VO_CATEGORY_NARRATIVE);
			}
		}
	}


	if (trackerProgressState == TPS_STARTPAUSE)
	{
		if (gameLocal.time > trackerProgressTimer + TRACKER_INITIALPAUSE)
		{
			

			trackerProgressState = TPS_LERPING;
			trackerProgressTimer = gameLocal.time;
		}
	}
	if (trackerProgressState == TPS_LERPING)
	{
		//The tracker progress bar values are kinda weird!
		//0.9 = empty.
		//0.8 = a little filled up
		//0.4 = about halfway
		//0.1 = almost fully filled
		//0.0 = fully filled

		float lerp = (gameLocal.time - trackerProgressTimer) / (float)TRACKER_BARTIME;
		float parmValue = idMath::Lerp(trackerProgressStartValue, trackerProgressEndValue, lerp);
		trackerModel->GetRenderEntity()->shaderParms[7] = parmValue;
		trackerModel->UpdateVisuals();

		if (gameLocal.time >= trackerProgressTimer + TRACKER_BARTIME)
		{
			trackerProgressState = TPS_DORMANT;
		}			
	}


	idStaticEntity::Think();
}

bool idCatpodInterior::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	if (index == FROBINDEX_EXIT)
	{
		DoExitPod();
	}



	return true;
}

//This gets called when the player enters the pod.
void idCatpodInterior::SetPlayerEnter()
{
	lastPlayerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	lastPlayerViewangle = gameLocal.GetLocalPlayer()->viewAngles;

	//Fade up from black.
	gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 1.0f, 0);	
	gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, 500);

	EquipSlots();

	//Teleport player in.
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idAngles teleportAngle = GetPhysics()->GetAxis().ToAngles();
	idVec3 teleportPos = GetPhysics()->GetOrigin() + (forward * -12) + (up * 1);
	gameLocal.GetLocalPlayer()->Teleport(teleportPos, teleportAngle, this);


	//Spawn the exit frob stuff.	
	idVec3 frobPos = GetPhysics()->GetOrigin() + (forward * 11) + (right * 32) + (up * 56);
	idDict args;
	args.Clear();
	args.Set("model", "models/objects/frobcube/cube4x4.ase");
	args.Set("displayname", common->GetLanguageDict()->GetString("#str_gui_hud_100131"));
	frobBar = gameLocal.SpawnEntityType(idFrobcube::Type, &args);
	frobBar->SetOrigin(frobPos);
	frobBar->GetPhysics()->GetClipModel()->SetOwner(this);
	static_cast<idFrobcube*>(frobBar)->SetIndex(FROBINDEX_EXIT);



	//BC 3-23-2025: loc box
	#define LOCBOXRADIUS 2.5f
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_def_gameplay_exitcatpod"));
	args.SetVector("origin", frobPos);
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));


	//3-30-2025: locbox
	idVec3 titleLocboxPos = GetPhysics()->GetOrigin() + (forward * 32) + (up * 75);
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_def_gameplay_catpod_carepackage"));
	args.SetVector("origin", titleLocboxPos);
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));

	


	fanfareState = FF_WAITING;
	fanfareTimer = gameLocal.time + FANFAREDELAY;

	playerIsInCatpod = true;

	if (trackerSequence == TRK_WAITINGFORPLAYER)
	{
		//start the tracker sequence.
		trackerSequence = TRK_SPAWNDELAY;
		trackerTimer = gameLocal.time;

		// SW 13th Feb 2025: Stop player stomping on tracker VO 
		gameLocal.GetLocalPlayer()->systemicVoEnabled = false;
	}
	else
	{
		gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay_msDelayed("snd_vo_catpod_response", 500);
	}

	BecomeActive(TH_THINK);
}


//Give it a def category, and it will randomly select an item from it.
idStr idCatpodInterior::GetEquipClassname(idStr defCategory)
{
	idList<idStr> primaryCandidates;
	const idKeyValue* kv;
	kv = this->spawnArgs.MatchPrefix(defCategory, NULL);
	while (kv)
	{
		idStr candidate = kv->GetValue();
		if (candidate.Length() > 0)
		{
			primaryCandidates.Append(candidate);
		}

		kv = this->spawnArgs.MatchPrefix(defCategory, kv); //Iterate to next entry.
	}

	if (primaryCandidates.Num() <= 0)
		return "";

	int primaryIdx = gameLocal.random.RandomInt(primaryCandidates.Num());
	return primaryCandidates[primaryIdx];
}

idEntity* idCatpodInterior::SpawnEquipSingle(idStr defName)
{
	if (defName.Length() <= 0)
		return NULL;

	idStr angleKeyname = idStr::Format("%s_angle", defName.c_str());
	idVec3 itemDir = spawnArgs.GetVector(angleKeyname);
	idAngles itemAng = idAngles(itemDir.x, itemDir.y, itemDir.z);

	idEntity* primaryEnt;
	idDict args;
	args.Clear();
	args.SetMatrix("rotation", itemAng.ToMat3());
	args.Set("classname", defName.c_str());
	args.Set("bind", this->GetName());
	args.SetVector("itemlinecolor", vec3_zero);
	gameLocal.SpawnEntityDef(args, &primaryEnt);

	return primaryEnt;
}

void idCatpodInterior::EquipSlots()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//spawn primary weapon
	idStr primaryDefname = GetEquipClassname("def_primary");
	idEntity* primaryItem = SpawnEquipSingle(primaryDefname);
	if (primaryItem)
	{
		idVec3 itemPos = GetPhysics()->GetOrigin() + (forward * 28) + (up * 64);
		primaryItem->SetOrigin(itemPos);
	}

	//spawn ammo
	#define AMMOCOUNT 2
	idStr ammoDefName = idStr::Format("%s_ammo", primaryDefname.c_str());
	ammoDefName = spawnArgs.GetString(ammoDefName);
	if (ammoDefName.Length() > 0)
	{
		for (int i = 0; i < AMMOCOUNT; i++)
		{
			idEntity* ammoItem = SpawnEquipSingle(ammoDefName);
			if (ammoItem)
			{
				idVec3 itemPos = GetPhysics()->GetOrigin() + (forward * 28) + (up * 54) + (right * -12) +  (right * (i * 24));
				ammoItem->SetOrigin(itemPos);
			}
		}
	}


	//spawn secondary item
	#define SECNDARYCOUNT 2
	for (int i = 0; i < SECNDARYCOUNT; i++)
	{
		idStr secondaryDefname = GetEquipClassname("def_secondary");
		idEntity* secondaryItem = SpawnEquipSingle(secondaryDefname);
		if (secondaryItem)
		{
			idVec3 itemPos = GetPhysics()->GetOrigin() + (forward * 28) + (up * 36) + (right * -12) + (right * (i * 24));
			secondaryItem->SetOrigin(itemPos);
		}
	}

}

//Verify the player is being returned to a space that has clearance. This is helpful for things such as: if player was near doors that were about to close
idVec3 idCatpodInterior::GetSafeLastPosition()
{
	idBounds playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds[1].z = pm_normalheight.GetFloat(); //When we teleport, it's in a standing stance. So we technically need clearance for standing size.
	playerbounds.Expand(1);
	
	#define CANDIDATEOFFSETCOUNT 17
	idVec3 candidateOffsets[] =
	{
		idVec3(0, 0, 0),
	
		idVec3(0, -32, 0),
		idVec3(0, -64, 0),
	
		idVec3(-32, 0, 0),
		idVec3(-64, 0, 0),
	
		idVec3(0, 32, 0),
		idVec3(0, 64, 0),
	
		idVec3(32, 0, 0),
		idVec3(64, 0, 0),


		idVec3(0, -32, -76),
		idVec3(0, -64, -76),

		idVec3(-32, 0, -76),
		idVec3(-64, 0, -76),

		idVec3(0, 32, -76),
		idVec3(0, 64, -76),

		idVec3(32, 0, -76),
		idVec3(64, 0, -76),
	};

	//Find best position that's closest to the original position.
	float shortestDistance = 9999999;
	idVec3 bestPosition = vec3_zero;
	idVec3 candidateStartPosition = lastPlayerPosition;
	for (int i = 0; i < CANDIDATEOFFSETCOUNT; i++)
	{
		idVec3 adjustedPos = candidateStartPosition + candidateOffsets[i];

		int penetrationContents = gameLocal.clip.Contents(adjustedPos, NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then skip it.
		}

		trace_t tr;		
		gameLocal.clip.TraceBounds(tr, adjustedPos, adjustedPos, playerbounds, MASK_SOLID, NULL);
		if (tr.fraction >= 1)
		{
			float dist = (adjustedPos - lastPlayerPosition).LengthSqr();
			if (dist < shortestDistance)
			{
				shortestDistance = dist;
				bestPosition = adjustedPos;
			}
		}
	}

	if (bestPosition == vec3_zero)
	{
		//couldn't find a good spot. Just use original position.
		return lastPlayerPosition;
	}

	return bestPosition;
}

void idCatpodInterior::DoExitPod()
{
	//When player frobs the exit button

	//Fade up from black.
	gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 1.0f, 0);
	gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, 500);

	//Teleport player back to where they frobbed the pod.
	gameLocal.GetLocalPlayer()->Teleport(GetSafeLastPosition(), lastPlayerViewangle, NULL);

	
	//Tell the cat pod to launch.
	LaunchCatPod();

	//Start reinforcements.
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->StartReinforcementsSequence();


	//Attempt to look at the pirate ship if LOS is possible.
	idEntity* pirateship = GetPirateShip();
	if (pirateship)
	{
		//Found pirateship.
		//do LOS check.
		trace_t tr;
		gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, pirateship->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
		
		if (tr.fraction >= 1)
		{
			//look at it.
			gameLocal.GetLocalPlayer()->SetViewLerp(pirateship->GetPhysics()->GetOrigin(), 300);
		}
	}

	playerIsInCatpod = false;

	// SW 13th Feb 2025: We may have turned off systemic VO upon entering the pod,
	// if there's a tracker in here. This allowed Nina to talk to the cats without it getting stomped on.
	// But we need to re-enable it when we leave!
	if (gameLocal.world->spawnArgs.GetBool("tracker_in_pod", "0"))
	{
		gameLocal.GetLocalPlayer()->systemicVoEnabled = true;
	}

	

	BecomeInactive(TH_THINK);
}

bool idCatpodInterior::GetPlayerIsInPod()
{
	return playerIsInCatpod;
}

idEntity* idCatpodInterior::GetPirateShip()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idPirateship::Type))
		{
			if (static_cast<idPirateship*>(ent)->IsDormant() || ent->IsHidden())
				continue;

			return ent;
		}
	}

	return NULL;
}

//Make the cat pod start its launch to leave the map.
void idCatpodInterior::LaunchCatPod()
{
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden() || !ent->IsType(idCatpod::Type))
			continue;

		// SW 10th March 2025: Any items that are still inside the pod should be ejected now
		static_cast<idCatpod*>(ent)->EjectItems(GetItemsInside());
		static_cast<idCatpod*>(ent)->StartTakeoff();
	}
}

// SW 10th March 2025
// Returns a list of moveable items inside the pod which will be ejected when the player leaves
idList<idMoveableItem*> idCatpodInterior::GetItemsInside(void)
{
	idEntity* entityArray[64];
	idList<idMoveableItem*> entityList;
	int count = gameLocal.EntitiesWithinAbsBoundingbox(this->GetPhysics()->GetAbsBounds().Expand(16), entityArray, 64);
	for (int i = 0; i < count; i++)
	{
		if (entityArray[i]->IsType(idMoveableItem::Type))
		{
			entityList.Append(static_cast<idMoveableItem*>(entityArray[i]));
		}
	}

	return entityList;
}
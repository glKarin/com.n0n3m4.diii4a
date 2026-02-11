#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"

//#include "trigger.h"

#include "bc_trashexit.h"
#include "bc_trashchute.h"

const float MOVETIME = .4f;
const float SLOWMOVETIME = .9f;
const int DETECT_INTERVAL = 1000;

const int PLATFORM_DEFAULT_HEIGHT = 42;

const int CHARGEUP_TIME = 500;
const int SLAM_TIME = 400;
const int RESET_TIME = 400;


const int EXIT_SPAWN_FORWARD_OFFSET = 32;

const float TRASHLIGHT_R = .29f;
const float TRASHLIGHT_G = .5f;
const float TRASHLIGHT_B = 1.0f;

#define SHUTTER_CLOSETIME 3000


const idEventDef EV_Chute_Enable("setChuteEnable", "d");
const idEventDef EV_Chute_IsEnabled("isChuteEnabled", NULL, 'd');

CLASS_DECLARATION(idStaticEntity, idTrashchute)
	EVENT(EV_PostSpawn, idTrashchute::Event_PostSpawn)
	EVENT(EV_Chute_Enable, idTrashchute::Event_ChuteEnable)
	EVENT(EV_Chute_IsEnabled, idTrashchute::Event_IsChuteEnabled)
	EVENT(EV_SpectatorTouch, idTrashchute::Event_SpectatorTouch)
END_CLASS

idTrashchute::idTrashchute(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	platform = nullptr;

	detectionTimer = false;
	stateTimer = false;

	trashState = false;

	gateModel = nullptr;

	shutterState = SHT_IDLE;
	shutterTimer = 0;
}

idTrashchute::~idTrashchute(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idTrashchute::Spawn(void)
{
	idDict args;

	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + idVec3(0,0, PLATFORM_DEFAULT_HEIGHT));
	args.Set("model", "models/objects/trashchute/platform.ase");
	//args.Set("snd_accel", "m1_accel");
	//args.Set("snd_decel", "m1_decel");
	//args.Set("snd_move", "m1_move");
	platform = (idMover *)gameLocal.SpawnEntityType(idMover::Type, &args);
	platform->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	platform->Event_SetMoveTime(MOVETIME);
	platform->GetPhysics()->GetClipModel()->SetOwner(this);
	//carousel->Event_SetAccellerationTime(ACCELTIME);
	//carousel->Event_SetDecelerationTime(DECELTIME);

	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	fl.takedamage = false;
	isFrobbable = false;

	BecomeActive(TH_THINK);

	detectionTimer = 0;
	stateTimer = 0;
	trashState = TRASH_IDLE;

	//renderlight.
	if (1)
	{
		// Light source.
		#define LIGHTRADIUS 24
		idVec3 forward, up;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

		headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHTRADIUS;
		headlight.shaderParms[0] = 0;
		headlight.shaderParms[1] = TRASHLIGHT_R;
		headlight.shaderParms[2] = TRASHLIGHT_G;
		headlight.shaderParms[3] = TRASHLIGHT_B;
		headlight.noShadows = true;
		headlight.isAmbient = false;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);

		headlight.origin = GetPhysics()->GetOrigin() + (forward * 24) + (up * 36); //place it in front of emblem.
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}


	//spawn gate model.
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 48.1f));
	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	args.Set("model", spawnArgs.GetString("model_gate"));
	args.Set("skin", spawnArgs.GetString("skin_gate"));
	args.SetInt("solid", 0);
	args.SetBool("noclipmodel", true);
	args.Set("snd_open", "shutter_3sec");
	gateModel = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	gateModel->Hide();

	if (!spawnArgs.GetBool("start_on", "1"))
	{
		Event_ChuteEnable(0);
	}
	

	



	PostEventMS(&EV_PostSpawn, 0); //Need this for post-spawn to work!
}

void idTrashchute::Event_PostSpawn(void) //We need to do this post-spawn because not all ents exist when Spawn() is called. So, we need to wait until AFTER spawn has happened, and call this post-spawn function.
{
	if (targets.Num() <= 0)
	{
		gameLocal.Error("trashchute '%s' is missing a env_trashexit target.", GetName());
		return;
	}

	if (!targets[0].IsValid())
	{
		gameLocal.Error("trashchute '%s' is missing a env_trashexit target.", GetName());
		return;
	}

	if (!targets[0].GetEntity()->IsType(idTrashExit::Type))
	{
		gameLocal.Error("trashchute '%s' must target an env_trashexit.", GetName());
		return;
	}
	else
	{
		idEntity * exitEnt = targets[0].GetEntity();
		static_cast<idTrashExit *>(exitEnt)->SetupChute(this, spawnArgs.GetBool("start_on", "1"));
	}
}

void idTrashchute::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( platform ); // idMover * platform

	savefile->WriteInt( detectionTimer ); // int detectionTimer

	savefile->WriteInt( stateTimer ); // int stateTimer

	savefile->WriteInt( trashState ); // int trashState

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteObject( gateModel ); // idEntity * gateModel

	savefile->WriteInt( shutterState ); // int shutterState
	savefile->WriteInt( shutterTimer ); // int shutterTimer
}

void idTrashchute::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( CastClassPtrRef(platform) ); // idMover * platform

	savefile->ReadInt( detectionTimer ); // int detectionTimer

	savefile->ReadInt( stateTimer ); // int stateTimer

	savefile->ReadInt( trashState ); // int trashState

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( gateModel ); // idEntity * gateModel

	savefile->ReadInt( shutterState ); // int shutterState
	savefile->ReadInt( shutterTimer ); // int shutterTimer
}

void idTrashchute::Think(void)
{
	idStaticEntity::Think();

	if (trashState == TRASH_DEACTIVATED)
	{
		//do nothing.
	}
	else if (trashState == TRASH_IDLE)
	{
		if (gameLocal.time >= detectionTimer)
		{
			idEntity	*entityList[MAX_GENTITIES];
			int			listedEntities, i;			

			detectionTimer = gameLocal.time + DETECT_INTERVAL;

			//detect if there's stuff waiting to be trashed...
			listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(GetTrashBounds(), entityList, MAX_GENTITIES);

			if (listedEntities > 0)
			{
				int totalCount = 0;
				for (i = 0; i < listedEntities; i++)
				{
					idEntity *ent = entityList[i];

					if (!ent)
					{
						continue;
					}

					if (ent == this || ent == platform || ent->IsHidden())
						continue;

					if (ent == gameLocal.GetLocalPlayer())
					{
						if (static_cast<idPlayer *>(ent)->noclip) //ignore player if they're in noclip.
							continue;

						//BC ignore player if player is currently jockeying.
						if (static_cast<idPlayer*>(ent)->IsJockeying())
							continue;
					}

					//Ignore items currently being held by the player.
					if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
					{
						if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
						{
							continue;
						}
					}

					//if (((ent->IsType(idMoveableItem::Type) || ent->IsType(idMoveable::Type)) && ent->IsAtRest()) || ent->IsType(idActor::Type))
					if (((ent->IsType(idMoveableItem::Type) || ent->IsType(idMoveable::Type))) || ent->IsType(idActor::Type))
					{
						totalCount++;
						break;
					}					
				}

				if (totalCount > 0)
				{
					//CHUTE detects something and is activating....!!!

					idVec3 forwardDir;
					idAngles particleAng = GetPhysics()->GetAxis().ToAngles();
					idVec3 movePos;

					trashState = TRASH_CHARGEUP;
					stateTimer = gameLocal.time + CHARGEUP_TIME;

					StartSound("snd_chargeup", SND_CHANNEL_BODY2, 0, false, NULL);
					SetSkin(declManager->FindSkin("skins/models/objects/trashchute/skin_blink"));

					StartSound("snd_jingle", SND_CHANNEL_VOICE, 0, false, NULL);					

					this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
					particleAng.pitch += 90;
					idEntityFx::StartFx("fx/trash_chargeup", GetPhysics()->GetOrigin() + forwardDir * 16 + idVec3(0, 0, 6), particleAng.ToMat3());

					idEntityFx::StartFx("fx/music", GetPhysics()->GetOrigin() + idVec3(0, 0, 52), mat3_identity);

					movePos = this->GetPhysics()->GetOrigin() + idVec3(0, 0, PLATFORM_DEFAULT_HEIGHT - 2);
					platform->Event_SetMoveTime(SLOWMOVETIME);
					platform->Event_MoveToPos(movePos);
				}
			}
		}
	}
	else if (trashState == TRASH_CHARGEUP)
	{
		//Is charging up......

		if (gameLocal.time > stateTimer)
		{
			//Chargeup done. Slam it down.
			
			idVec3 movePos = GetPhysics()->GetOrigin();
			idAngles particleAng = idAngles(0,0,0);

			trashState = TRASH_SLAMMINGDOWN;
			stateTimer = gameLocal.time + SLAM_TIME;

			StartSound("snd_slam1", SND_CHANNEL_BODY, 0, false, NULL);

			platform->Event_SetMoveTime(MOVETIME);
			platform->Event_MoveToPos(movePos);
			
			particleAng.pitch += 180;
			idEntityFx::StartFx("fx/trash_windsuck", GetPhysics()->GetOrigin() + idVec3(0,0,64), particleAng.ToMat3());
		}
	}
	else if (trashState == TRASH_SLAMMINGDOWN)
	{
		//Platform is going down into hole.

		if (gameLocal.time > stateTimer)
		{
			//Platform is at bottom of hole.

			idVec3 resetPos;
			int hasTeleportedPlayer = 0;
			idVec3 debrisSpawnPos, debrisDir;

			//Eject the stuff here.
			idEntity	*entityList[MAX_GENTITIES];
			int			listedEntities, i;

			listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(GetTrashBounds(), entityList, MAX_GENTITIES);

			for (i = 0; i < listedEntities; i++)
			{
				idEntity *ent = entityList[i];

				if (!ent)
				{
					continue;
				}

				if (ent == this || ent == platform)
					continue;

				if (ent->IsType(idMoveableItem::Type) || ent->IsType(idMoveable::Type) || ent->IsType(idActor::Type))
				{
					//ent->PostEventMS(&EV_Remove, 0);
					int totalOutputs, randomIdx; //TODO: chooses between random destinations.... we don't wnat this, remove this. just make it use first Target.
					idVec3 outputLocation;
					int pushSpeed;
					idAngles exitAngle;

					int randomRadius = spawnArgs.GetInt("ejectradius", "32");

					FindTargets();
					RemoveNullTargets();
					if (!targets.Num())
					{
						gameLocal.Error("idTrashchute '%s' at (%s) doesn't have 'target' key specified.", name.c_str(), GetPhysics()->GetOrigin().ToString(0));
					}

					totalOutputs = targets.Num();

					randomIdx = gameLocal.random.RandomInt(totalOutputs);
					exitAngle = targets[randomIdx].GetEntity()->GetPhysics()->GetAxis().ToAngles();

					if (!targets[randomIdx].GetEntity())
					{
						gameLocal.Error("idTrashchute '%s' at (%s) has an invalid 'target' key.", name.c_str(), GetPhysics()->GetOrigin().ToString(0));
					}

					outputLocation = targets[randomIdx].GetEntity()->GetPhysics()->GetOrigin();

					if (randomRadius > 0 && !ent->IsType(idPlayer::Type))
					{
						idVec3 forward, right, up;
						targets[randomIdx].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

						outputLocation += (forward * (EXIT_SPAWN_FORWARD_OFFSET + gameLocal.random.RandomInt(randomRadius))) 
							+ (right * (-randomRadius + gameLocal.random.RandomInt(randomRadius*2)))
							+ (up * (-randomRadius + gameLocal.random.RandomInt(randomRadius * 2)));

						
					}

					//If ejecting player out...
					if (ent->IsType(idPlayer::Type))
					{						
						if (static_cast<idPlayer *>(ent)->noclip) //ignore player if they're in noclip.
							continue;						

						static_cast<idPlayer *>(ent)->SetViewFade(0, 0, 0, 0, 500);

						hasTeleportedPlayer = 1;
						debrisSpawnPos = targets[randomIdx].GetEntity()->GetPhysics()->GetOrigin();
						debrisSpawnPos += exitAngle.ToForward() * 16;

						if (exitAngle.pitch < 45 && exitAngle.pitch > -45)
						{
							//If player is ejecting mostly in a horizontal line direction, then lower them down a bit so that the eyeline lines up with the trash chute model.
							outputLocation += exitAngle.ToForward() * EXIT_SPAWN_FORWARD_OFFSET;
							outputLocation.z -= 64;
						}
						else if (exitAngle.pitch > 45)
						{
							//If player ejecting from ceiling, then push them forward a bit so that they don't clip into world geometry.
							outputLocation += exitAngle.ToForward() * 80;
						}
						else
						{
							//Ejecting from floor.
							outputLocation += exitAngle.ToForward() * 8;
						}
					}
					
					//If player is carrying a body.......
					if (gameLocal.GetLocalPlayer()->bodyDragger.isDragging)
					{
						if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.IsValid())
						{
							if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.GetEntityNum() == ent->entityNumber)
							{
								gameLocal.GetLocalPlayer()->bodyDragger.StopDrag(false);

								//TODO: make a nice particle effect here of the body disappearing.
							}
						}
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
						gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay_msDelayed("snd_vo_trash_skull", 1200);
                    }

					// SW 6th May 2025: Do a little particle effect on objects sitting higher in the chute (or balanced on the lip) so they don't appear to just 'vanish'
					if (ent->IsType(idMoveableItem::Type) || ent->IsType(idMoveable::Type)
						&& ent->GetPhysics()->GetOrigin().z > GetPhysics()->GetOrigin().z + 32)
					{
						gameLocal.DoParticle(this->spawnArgs.GetString("model_teleport_prt"), ent->GetPhysics()->GetAbsBounds().GetCenter());
					}

					//Teleport the object.
					ent->Teleport(outputLocation, ent->GetPhysics()->GetAxis().ToAngles(), targets[randomIdx].GetEntity());

					pushSpeed = spawnArgs.GetInt("pushspeed", "256");

					if (pushSpeed > 0)
					{
						//trash chute ejecting trash at speed.
						idVec3 ejectDir = targets[randomIdx].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward();
						ent->GetPhysics()->SetLinearVelocity(ejectDir * pushSpeed);
					}

					debrisDir = targets[randomIdx].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward();
				}				
			}

			trashState = TRASH_RESETTING;
			stateTimer = gameLocal.time + RESET_TIME;			

			resetPos = this->GetPhysics()->GetOrigin() + idVec3(0, 0, PLATFORM_DEFAULT_HEIGHT);
			platform->Event_MoveToPos(resetPos);

			if (hasTeleportedPlayer)
			{
				idAngles forwardDir;
				idAngles particleDir;
				const idKeyValue *kv;

				//Spew out some trash debris (fishbones, applecore, etc)
				int debrisSpeed = spawnArgs.GetInt("pushspeed", "256");
				if (debrisSpeed <= 0)
				{
					debrisSpeed = 4;
				}
				else
				{
					debrisSpeed = debrisSpeed  * .75f;
				}				

				kv = this->spawnArgs.MatchPrefix("def_trashgib", NULL);
				while (kv)
				{
					gameLocal.SetDebrisBurst(kv->GetValue(), debrisSpawnPos, 4, 16, debrisSpeed, debrisDir);
					kv = this->spawnArgs.MatchPrefix("def_trashgib", kv);
				}

				//play sound fx 
				gameLocal.GetLocalPlayer()->StartSound("snd_trasheject", SND_CHANNEL_ANY, 0, false, NULL);

				forwardDir = targets[0].GetEntity()->GetPhysics()->GetAxis().ToAngles();
				particleDir = forwardDir;
				particleDir.pitch += 90;
				idEntityFx::StartFx("fx/wind_local", debrisSpawnPos + forwardDir.ToForward() * 64, particleDir.ToMat3());

				gameLocal.GetLocalPlayer()->SetSmelly(true);

				//make player look forward.
				if (forwardDir.pitch == 0)
				{
					//Pitch.
					gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, 10);

					//Turn to face yaw.
					gameLocal.GetLocalPlayer()->SetViewYawLerp(forwardDir.yaw, 10);
				}
			}
		}
	}
	else if (trashState == TRASH_RESETTING)
	{
		//Platform is rising back up.

		if (gameLocal.time > stateTimer)
		{
			//Platform back in original position.
			trashState = TRASH_IDLE;

			StartSound("snd_slam2", SND_CHANNEL_BODY, 0, false, NULL);

			StartSound("snd_chargedown", SND_CHANNEL_BODY2, 0, false, NULL);

			SetSkin(declManager->FindSkin("skins/models/objects/trashchute/skin"));

			idEntityFx::StartFx("fx/trash_windblow", GetPhysics()->GetOrigin() + idVec3(0, 0, 64), mat3_identity);
		}
	}

	if (shutterState == SHT_SHUTTERING)
	{
		float lerp = (gameLocal.time - shutterTimer) / (float)SHUTTER_CLOSETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = idMath::CubicEaseOut(lerp);

		gateModel->GetRenderEntity()->shaderParms[7] = lerp;
		gateModel->UpdateVisuals();

		if (gameLocal.time >= shutterTimer + SHUTTER_CLOSETIME)
		{
			gateModel->Hide();
			shutterState = SHT_SHUTTERED;
		}
	}
}

//These are the bounds we check for moveable things to suck up.......
idBounds idTrashchute::GetTrashBounds()
{
	idBounds rawBounds = GetPhysics()->GetAbsBounds();
	rawBounds = rawBounds.Expand(-4);
	rawBounds[1][2] = GetPhysics()->GetOrigin().z + 56;

	//gameRenderWorld->DebugBounds(colorGreen, rawBounds,vec3_origin, 20000);

	return rawBounds;
}

bool idTrashchute::DoFrob(int index, idEntity * frobber)
{

	return true;
}

//Lockdown control.
void idTrashchute::Event_ChuteEnable(int value)
{
	//toggle chute on/off

	if (value <= 0)
	{
		//turn off. trashchute is deactivated, the gate blocks usage.

		//reset platform position.
		if (trashState == TRASH_SLAMMINGDOWN || trashState == TRASH_RESETTING)
		{
			idVec3 resetPos = this->GetPhysics()->GetOrigin() + idVec3(0, 0, PLATFORM_DEFAULT_HEIGHT);
			platform->Event_MoveToPos(resetPos);
		}

		trashState = TRASH_DEACTIVATED;

		headlight.shaderParms[0] = 0;
		headlight.shaderParms[1] = 0;
		headlight.shaderParms[2] = 0;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
		platform->SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_platform_off")));

		gateModel->Show();
		gateModel->GetPhysics()->SetContents(CONTENTS_SOLID | CONTENTS_RENDERMODEL);
	}
	else 
	{
		//turn on. trashchute is available for use, the gate disappears.

		if (trashState == TRASH_DEACTIVATED)
		{
			trashState = TRASH_IDLE;
		}

		headlight.shaderParms[1] = TRASHLIGHT_R;
		headlight.shaderParms[2] = TRASHLIGHT_G;
		headlight.shaderParms[3] = TRASHLIGHT_B;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_on")));
		platform->SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_platform_on")));

		//Start animation of shutter opening up.
		//gateModel->Hide();
		gateModel->StartSound("snd_open", SND_CHANNEL_ANY);
		gateModel->GetPhysics()->SetContents(0);
		shutterState = SHT_SHUTTERING;
		shutterTimer = gameLocal.time;

	}

}

bool idTrashchute::IsChuteEnabled()
{
	if (trashState == TRASH_DEACTIVATED)
		return false;

	return true;
}

void idTrashchute::Event_IsChuteEnabled(void)
{
	idThread::ReturnFloat((float)IsChuteEnabled());
}

void idTrashchute::Event_SpectatorTouch(idEntity* other, trace_t* trace)
{
	idVec3		contact, translate, normal;
	idBounds	bounds;
	idPlayer* p;

	assert(other && other->IsType(idPlayer::Type) && static_cast<idPlayer*>(other)->spectating);

	p = static_cast<idPlayer*>(other);
	// avoid flicker when stopping right at clip box boundaries
	if (p->lastSpectateTeleport > gameLocal.hudTime - 300) { //BC was 1000
		return;
	}

	// SW May 7th 2025: All our trash chutes only have a single exit and there's no reason to copy all this random selection stuff from Think(),
	// but I'd feel bad if I broke it now.
	int totalOutputs = targets.Num();

	int randomIdx = gameLocal.random.RandomInt(totalOutputs);
	idAngles exitAngle = targets[randomIdx].GetEntity()->GetPhysics()->GetAxis().ToAngles();

	if (!targets[randomIdx].GetEntity())
	{
		gameLocal.Error("idTrashchute '%s' at (%s) has an invalid 'target' key.", name.c_str(), GetPhysics()->GetOrigin().ToString(0));
	}

	idVec3 outputLocation = targets[randomIdx].GetEntity()->GetPhysics()->GetOrigin();
	outputLocation += exitAngle.ToForward() * 48;

	p->SetOrigin(outputLocation);
	p->SetViewAngles(exitAngle);
	p->lastSpectateTeleport = gameLocal.hudTime;
	gameLocal.GetLocalPlayer()->StartSound("snd_spectate_door", SND_CHANNEL_ANY);
}
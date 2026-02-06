#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "framework/DeclEntityDef.h"

#include "bc_lever.h"
#include "bc_airlock.h"
#include "bc_airlock_accumulator.h"

//The airlock presssure accumulator.
//When these are all deactivated/destroyed, it does the emergency airlock purge.

#define ANIM_TIME 3900 //about how many milliseconds for deflate anim.

#define BUTTON_ANIMTIME  500

#define STATUSUPDATE_DELAY 300 //delay before it tells the airlock that it's deflated.

#define FROBINDEX_EMERGENCYBUTTON 2

CLASS_DECLARATION(idAnimated, idAirlockAccumulator)
	EVENT(EV_PostSpawn, idAirlockAccumulator::Event_PostSpawn)
END_CLASS


idAirlockAccumulator::idAirlockAccumulator(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idAirlockAccumulator::~idAirlockAccumulator(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idAirlockAccumulator::Spawn(void)
{
	state = APA_IDLE;
	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	SetColor(0, 1, 0);
	PostEventMS(&EV_PostSpawn, 0);
	isFrobbable = false;
	SetShaderParm(5, 1);
	hasUpdatedAccumStatus = false;
	accumStatusTimer = 0;

	airlockEnt = NULL;
	cableEnt = NULL;


	idVec3 up, forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	//The valve button.
	if (1)
	{
		idDict args;
	
		args.Set("classname", "env_lever_valve");		
		args.SetInt("frobindex", FROBINDEX_EMERGENCYBUTTON);
		args.Set("displayname", displayName.c_str());
		gameLocal.SpawnEntityDef(args, &emergencyButton);
	
		if (!emergencyButton)
		{
			gameLocal.Error("idAirlockAccumulator '%s' failed to spawn emergency button.\n", GetName());
		}
		else
		{
			idVec3 buttonPos;
			idAngles buttonAng;	
			buttonAng = GetPhysics()->GetAxis().ToAngles();			
			buttonPos = GetPhysics()->GetOrigin() + (forward * 8) + (up * -13);	
			emergencyButton->SetOrigin(buttonPos);
			emergencyButton->SetAxis(buttonAng.ToMat3());
			emergencyButton->GetPhysics()->GetClipModel()->SetOwner(this);


			jointHandle_t centerJoint;
			centerJoint = animator.GetJointHandle("Bone.006");
			if (centerJoint == INVALID_JOINT)
			{
				gameLocal.Error("idAirlockAccumulator '%s' has bad bone setup.", GetName());
			}

			emergencyButton->BindToJoint(this, centerJoint, true);
		}
	}

	//renderlight.
	if (1)
	{
		// Light source. We use a renderlight (i.e. not a real light entity) to prevent weird physics binding jitter
		headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 12.0f;
		headlight.shaderParms[0] = 0;
		headlight.shaderParms[1] = 0.7f;
		headlight.shaderParms[2] = 0;
		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = false;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);

		headlight.origin = GetPhysics()->GetOrigin() + (forward * 12) + (up * -14);
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}
}


void idAirlockAccumulator::Event_PostSpawn(void) //We need to do this post-spawn because not all ents exist when Spawn() is called. So, we need to wait until AFTER spawn has happened, and call this post-spawn function.
{
	//Set the airlock it's assigned to.
	if (targets.Num() <= 0)
	{
		gameLocal.Error("airlock accumulator '%s' is not assigned to an airlock.", GetName());
		return;
	}

	bool foundAirlock = false;
	bool foundCable = false;

	for (int i = 0; i < targets.Num(); i++)
	{
		if (!targets[i].IsValid())
			continue;

		if (targets[i].GetEntity()->IsType(idAirlock::Type))
		{
			if (foundAirlock)
			{
				gameLocal.Error("airlock accumulator '%s' targets multiple airlocks.", GetName());
				return;
			}

			airlockEnt = targets[i].GetEntity();
			foundAirlock = true;

			static_cast<idAirlock *>(airlockEnt.GetEntity())->InitializeAccumulator(this);
		}
		else if (targets[i].GetEntity()->IsType(idStaticEntity::Type))
		{
			if (foundCable)
			{
				gameLocal.Error("airlock accumulator '%s' targets multiple cables.", GetName());
				return;
			}

			cableEnt = targets[i].GetEntity();
			foundCable = true;
		}
	}

	if (!foundAirlock)
	{
		gameLocal.Error("airlock accumulator '%s' is not assigned to an airlock. Accumulator needs to target an airlock.", GetName());
		return;
	}

	if (!foundCable)
	{
		gameLocal.Warning("airlock accumulator '%s' is not assigned to a cable. Accumulator needs to target a func_static.", GetName());
	}

	//Ok, we now have an airlock assigned to this.

	Event_PlayAnim("breathing", 0, true);
}

void idAirlockAccumulator::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( airlockEnt ); //  idEntityPtr<idEntity> airlockEnt

	savefile->WriteInt( stateTimer ); //  int stateTimer
	savefile->WriteInt( state ); //  int state

	savefile->WriteObject( emergencyButton ); //  idEntity * emergencyButton

	savefile->WriteBool( hasUpdatedAccumStatus ); //  bool hasUpdatedAccumStatus
	savefile->WriteInt( accumStatusTimer ); //  int accumStatusTimer

	savefile->WriteRenderLight( headlight ); //  renderLight_t headlight
	savefile->WriteInt( headlightHandle ); //  int headlightHandle

	savefile->WriteObject( cableEnt ); //  idEntityPtr<idEntity> cableEnt
}

void idAirlockAccumulator::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( airlockEnt ); //  idEntityPtr<idEntity> airlockEnt

	savefile->ReadInt( stateTimer ); //  int stateTimer
	savefile->ReadInt( state ); //  int state

	savefile->ReadObject( emergencyButton ); //  idEntity * emergencyButton

	savefile->ReadBool( hasUpdatedAccumStatus ); //  bool hasUpdatedAccumStatus
	savefile->ReadInt( accumStatusTimer ); //  int accumStatusTimer

	savefile->ReadRenderLight( headlight ); //  renderLight_t headlight
	savefile->ReadInt( headlightHandle ); //  int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( cableEnt ); //  idEntityPtr<idEntity> cableEnt
}

void idAirlockAccumulator::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	idAnimated::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	if (state != APA_DEAD && health <= 0)
	{
		SetDeflate();
	}
}

void idAirlockAccumulator::SetDeflate()
{
	state = APA_DEFLATING;
	StartSound("snd_deflate", SND_CHANNEL_BODY);
	Event_PlayAnim("deflate", 1, false);
	SetColor(1, 0, 0);
	stateTimer = gameLocal.time + ANIM_TIME;
	fl.takedamage = false;
	BecomeActive(TH_THINK);
	accumStatusTimer = gameLocal.time + STATUSUPDATE_DELAY;
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	//particle fx
	idVec3 up, forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	idAngles particleAng = forward.ToAngles();	
	particleAng.pitch += 90;
	idEntityFx::StartFx("fx/airlock_accum_deflate", GetPhysics()->GetOrigin() + up * 16, particleAng.ToMat3());
	idEntityFx::StartFx("fx/airlock_accum_deflate", GetPhysics()->GetOrigin() + up * -16, particleAng.ToMat3());

	//Delete the button.
	emergencyButton->Hide();
	emergencyButton->PostEventMS(&EV_Remove, 0);
	emergencyButton = nullptr;

	//Pop off a physics button.
	idEntity *physicsValve;
	idDict args;
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + (up * -13) + (forward * 8));
	args.Set("classname", spawnArgs.GetString("def_valve"));
	gameLocal.SpawnEntityDef(args, &physicsValve);
	if (physicsValve->IsType(idItem::Type))
	{
		static_cast<idItem *>(physicsValve)->SetJustDropped(true);
	}

	SetSkin(declManager->FindSkin("skins/airlock_pressure/skin_deflate"));
	spawnArgs.SetBool("zoominspect", false); // SW 6th May 2025: Can't inspect once the label disappears


	//Make the cable func_static stop animating.
	if (cableEnt.IsValid())
	{
		cableEnt.GetEntity()->SetShaderParm(7, 1);
	}
}


void idAirlockAccumulator::Think(void)
{
	if (state == APA_DEFLATING)
	{
		float lerp = (stateTimer - gameLocal.time) / (float)ANIM_TIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		SetShaderParm(5, lerp); //slowly darken the material.
		

		//We do a little delay before updating the airlock logic because it looks weird when the accumulator deflate & airlock open both happen on exactly the same frame.
		if (!hasUpdatedAccumStatus && gameLocal.time >= accumStatusTimer)
		{
			//update airlock logic.
			hasUpdatedAccumStatus = true;			
			if (airlockEnt.IsValid())
			{
				static_cast<idAirlock *>(airlockEnt.GetEntity())->DoAccumulatorStatusUpdate(spawnArgs.GetInt("index"));
				
			}
		}

		if (gameLocal.time >= stateTimer)
		{
			gameLocal.DoParticle("smoke_ring17.prt", GetPhysics()->GetOrigin());
			state = APA_DEAD;
			BecomeInactive(TH_THINK);
		}
	}
	else if (state == APA_BUTTONANIMATING)
	{
		if (gameLocal.time >= stateTimer)
		{
			SetDeflate();
		}
	}

	idAnimated::Think();
}

bool idAirlockAccumulator::IsDeflated()
{
	return (state == APA_DEAD || state == APA_DEFLATING);
}

bool idAirlockAccumulator::DoFrob(int index, idEntity * frobber)
{
	//if (airlockEnt.IsValid())
	//{
	//	if (airlockEnt.GetEntity()->IsType(idAirlock::Type))
	//	{
	//		if (static_cast<idAirlock *>(airlockEnt.GetEntity())->IsFuseboxgateShut())
	//		{
	//			//If airlock is fusebox-lock-downed, then don't allow frob.
	//			StartSound("snd_locked", SND_CHANNEL_ANY);
	//			return true;
	//		}
	//	}
	//}

	if (state == APA_IDLE && index == FROBINDEX_EMERGENCYBUTTON)
	{
		state = APA_BUTTONANIMATING;
		stateTimer = gameLocal.time + BUTTON_ANIMTIME;		
		static_cast<idLever *>(emergencyButton)->SetActive(false);
		BecomeActive(TH_THINK);

		SetColor(1, .8f, 0);
		headlight.shaderParms[0] = 1;
		headlight.shaderParms[1] = 0.8f;
		headlight.shaderParms[2] = 0;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

		spawnArgs.SetBool("zoominspect", false); //BC 5-8-2025 call this again earlier, for players who quickly inspect right after frobbing it
	}

	return true;
}
#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "Trigger.h"

#include "bc_cat.h"

const int EVADEJUMP_TRACECOUNT = 16;
const int EVADEJUMP_MAXDISTANCE = 212;
const int EVADEJUMP_MINDISTANCE = 64;
const int CAT_RADIUS = 12;
const int CAT_MOVETIME = 300;
const int SMEARANIMATION_THRESHOLD = 120;
const int RESCUEJUMP_TIME = 800; //when released from cage, disappears after XX milliseconds.

const int CATJUMP_POD_TIME = 500;

CLASS_DECLARATION(idAnimated, idCat)

END_CLASS



//States:
//CAT_CAGED = inside cage.
//CAT_RESCUEJUMP = is doing the "I am being rescued" animation.
//CAT_HIDDEN = has been rescued, can be called, is available.


idCat::idCat(void)
{
	catfriendNode.SetOwner(this);
	catfriendNode.AddToEnd(gameLocal.catfriendsEntities);

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idCat::~idCat(void)
{
	catfriendNode.Remove();

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}
}

void idCat::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	fl.takedamage = true;

	if (spawnArgs.GetBool("caged", "0"))
	{
		catState = CAT_CAGED;
	}
	else
	{
		catState = CAT_IDLE;
	}

	headJoint = animator.GetJointHandle("head");
}

void idCat::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( catState ); //  int catState

	savefile->WriteVec3( startPosition ); //  idVec3 startPosition
	savefile->WriteVec3( targetPosition ); //  idVec3 targetPosition
	savefile->WriteAngles( targetAngle ); //  idAngles targetAngle
	savefile->WriteVec3( targetMovedir ); //  idVec3 targetMovedir
	savefile->WriteVec3( targetNormal ); //  idVec3 targetNormal

	savefile->WriteBool( hasPlayedUnstretchAnimation ); //  bool hasPlayedUnstretchAnimation
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteObject( lookEnt ); //  idEntityPtr<idEntity> lookEnt
	savefile->WriteJoint( headJoint ); //  saveJoint_t headJoint


	savefile->WriteRenderLight( headlight ); //  renderLight_t headlight
	savefile->WriteInt( headlightHandle ); //  int headlightHandle

}

void idCat::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( catState ); //  int catState

	savefile->ReadVec3( startPosition ); //  idVec3 startPosition
	savefile->ReadVec3( targetPosition ); //  idVec3 targetPosition
	savefile->ReadAngles( targetAngle ); //  idAngles targetAngle
	savefile->ReadVec3( targetMovedir ); //  idVec3 targetMovedir
	savefile->ReadVec3( targetNormal ); //  idVec3 targetNormal

	savefile->ReadBool( hasPlayedUnstretchAnimation ); //  bool hasPlayedUnstretchAnimation
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadObject( lookEnt ); //  idEntityPtr<idEntity> lookEnt
	savefile->ReadJoint( headJoint ); //  saveJoint_t headJoint


	savefile->ReadRenderLight( headlight ); //  renderLight_t headlight
	savefile->ReadInt( headlightHandle ); //  int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

}

void idCat::Think(void)
{
	idAnimated::Think();

	lookEnt = gameLocal.GetLocalPlayer();

	if (catState == CAT_RESCUEJUMP)
	{
		if (gameLocal.time >= stateTimer)
		{
			catState = CAT_AVAILABLE;
			SetHeadlight(false);
			Hide();

			idVec3 jointPos = GetHeadPos();
			idEntityFx::StartFx("fx/pickupitem", &jointPos, &mat3_identity, NULL, false); //BC
		}
	}
	else if (catState == CAT_JUMP_LOOP)
	{
		idVec3 newcatPosition;
		idVec3 adjustedTargetPosition = targetPosition + (targetMovedir * -CAT_RADIUS);

		float lerp = min(1.0f, (gameLocal.time - stateTimer) / (float)CAT_MOVETIME);
		lerp = idMath::CubicEaseOut(lerp);

		if (lerp >= 1.0f)
		{
			idMat3 fxMat = mat3_identity;			

			lerp = 1.0f;
			catState = CAT_IDLE;

			//Attach to wall & orient self.
			Event_PlayAnim("jump_land", 1);
			GetPhysics()->SetAxis(targetAngle.ToMat3());

			adjustedTargetPosition = targetPosition;

			idEntityFx::StartFx("fx/smoke_ring01", &targetPosition, &fxMat, NULL, false);
		}
		else if (lerp >= 0.5f && !hasPlayedUnstretchAnimation)
		{
			hasPlayedUnstretchAnimation = true;
			Event_PlayAnim("jump_unstretch", 1);
		}

		newcatPosition.Lerp(startPosition, adjustedTargetPosition, lerp);
		GetPhysics()->SetOrigin(newcatPosition);
		UpdateVisuals();
	}
	else if (catState == CAT_FAILSAFEJUMP)
	{
		idVec3 newcatPosition;
		float lerp = min(1.0f, (gameLocal.time - stateTimer) / (float)CAT_MOVETIME);

		if (lerp > 1)
			lerp = 1;

		if (lerp >= 1.0f)
		{
			catState = CAT_IDLE;
			//idEntityFx::StartFx("fx/smoke_ring01", &targetPosition, &mat3_identity, NULL, false);
		}
		
		newcatPosition.Lerp(startPosition, targetPosition, lerp);
		GetPhysics()->SetOrigin(newcatPosition);
		UpdateVisuals();		
	}
	else if (catState == CAT_JUMPING_TO_CATPOD_DELAY)
	{
		if (gameLocal.time >= stateTimer)
		{
			//delay is done. start the pod jump.
			stateTimer = gameLocal.time + CATJUMP_POD_TIME;
			catState = CAT_JUMPING_TO_CATPOD;

			//Position cat.
			SetOrigin(startPosition);

			//Aim it toward the correct angle.
			idVec3 aimDir = targetPosition - startPosition;
			aimDir.Normalize();
			SetAxis(aimDir.ToMat3());
			Event_PlayAnim("catpod_jump", 0, true);
			Show();
		}
	}
	else if (catState == CAT_JUMPING_TO_CATPOD)
	{
		//Move the cat.
		float lerp = (stateTimer - gameLocal.time) / (float)CATJUMP_POD_TIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = 1 - lerp;
		
		idVec3 lerpedCatPos;
		lerpedCatPos.Lerp(startPosition, targetPosition, lerp);
		SetOrigin(lerpedCatPos);

		if (gameLocal.time >= stateTimer)
		{
			//jump is done.
			catState = CAT_JUMP_TO_CATPOD_DONE;
			idEntityFx::StartFx(spawnArgs.GetString("fx_catpodenter"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);			
			Hide();
		}
	}


	//L O O K I N G
	if (lookEnt.IsValid())
	{
		idRotation lookRotation;
		idVec3 lookVec;
		idVec3 lookPoint;

		idVec3 jointPos;
		idMat3 jointAxis;
		idMat3 mat;

		if (lookEnt.GetEntity()->IsType(idActor::Type))
		{
			lookPoint = static_cast<idActor*>(lookEnt.GetEntity())->GetEyePosition();
		}
		else
		{
			lookPoint = lookEnt.GetEntity()->GetPhysics()->GetOrigin();
		}

		animator.GetJointTransform(headJoint, gameLocal.time, jointPos, jointAxis);
		jointPos = this->GetPhysics()->GetOrigin() + jointPos * this->GetPhysics()->GetAxis();
		lookVec = lookPoint - jointPos;
		lookVec.Normalize();

		mat = this->GetPhysics()->GetAxis().Transpose();
		animator.SetJointAxis(headJoint, JOINTMOD_WORLD,  lookVec.ToMat3() * mat );
	}

	if (headlightHandle != -1)
	{
		idVec3 jointPos = GetHeadPos();
		headlight.origin = jointPos;
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}
}

idVec3 idCat::GetHeadPos()
{
	idVec3 jointPos;
	idMat3 jointAxis;
	animator.GetJointTransform(headJoint, gameLocal.time, jointPos, jointAxis);

	return (this->GetPhysics()->GetOrigin() + jointPos * this->GetPhysics()->GetAxis());
}

void idCat::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	if (catState != CAT_IDLE)
		return;

	DoSafetyJump();
}

//Find a safe spot to jump to, and jump there.
void idCat::DoSafetyJump(void)
{
	//Do a points-on-sphere check.
	idVec3* pointsOnSphere;
	int i;

	pointsOnSphere = gameLocal.GetPointsOnSphere(EVADEJUMP_TRACECOUNT);

	StartSound("snd_annoyed", SND_CHANNEL_VOICE, 0, false, NULL);

	
	for (i = 0; i < EVADEJUMP_TRACECOUNT; i++)
	{
		trace_t trWire, trBounds;
		idVec3 wireDir;
		float distanceToJumpPos;
		idVec3 boundBoxPos;
		//idAngles finalAngle;
		idVec3 jumpDir;
		idMat3 fxMat = mat3_identity;

		idFuncSmoke *splashEnt;
		idDict splashArgs;

		trace_t downTr;
		idVec3 localUp;





		wireDir = pointsOnSphere[i];
		wireDir.Normalize();

		gameLocal.clip.TracePoint(trWire, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + (wireDir * EVADEJUMP_MAXDISTANCE), MASK_SOLID, this);

		distanceToJumpPos = (trWire.endpos - GetPhysics()->GetOrigin()).LengthFast();

		//Ignore short jumps.
		if (distanceToJumpPos < EVADEJUMP_MINDISTANCE)
			continue;

		//Ignore jumps that don't hit a wall.
		if (trWire.fraction >= 1.0f)
			continue;

		//Bounding box check.
		boundBoxPos = trWire.endpos + (trWire.c.normal * (CAT_RADIUS + .1f));
		gameLocal.clip.TraceBounds(trBounds, boundBoxPos, boundBoxPos, idBounds(idVec3(-CAT_RADIUS, -CAT_RADIUS, -CAT_RADIUS), idVec3(CAT_RADIUS, CAT_RADIUS, CAT_RADIUS)), MASK_SOLID, this);
		//gameRenderWorld->DebugBounds(colorRed, idBounds(idVec3(-CAT_RADIUS, -CAT_RADIUS, -CAT_RADIUS), idVec3(CAT_RADIUS, CAT_RADIUS, CAT_RADIUS)), boundBoxPos, 10000);
		//gameRenderWorld->DebugSphere(colorCyan, idSphere(boundBoxPos, 4), 10000);

		if (trBounds.fraction < 1)
		{
			//gameRenderWorld->DebugSphere(colorRed, idSphere(boundBoxPos, 4), 10000);
			continue;
		}

		//CAT CLAW DECAL		
		GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &localUp);
		gameLocal.clip.TracePoint(downTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + (localUp * -16), MASK_SOLID, this);		
		gameLocal.ProjectDecal(downTr.endpos, -downTr.c.normal, 8.0f, true, 20.0f, "textures/decals/catclaws");

		//gameRenderWorld->DebugArrow(trWire.fraction < 1 ? colorGreen : colorRed, GetPhysics()->GetOrigin(), trWire.endpos, 4, 10000);

		//GetPhysics()->SetOrigin(trWire.endpos);
		targetAngle = trWire.c.normal.ToAngles();
		targetAngle.pitch += 90;
		//GetPhysics()->SetAxis(finalAngle.ToMat3());

		catState = CAT_JUMP_LOOP;

		//If cat is jumping a far distance, do smear animation. If not far distance, then don't smear.
		if (distanceToJumpPos > SMEARANIMATION_THRESHOLD)
		{
			Event_PlayAnim("jump_loop_stretch", 1);
			hasPlayedUnstretchAnimation = false;
		}
		else
		{
			Event_PlayAnim("jump_loop", 1);
			hasPlayedUnstretchAnimation = true;
		}

		

		//Face the direction that cat is jumping toward.
		jumpDir = trWire.endpos - GetPhysics()->GetOrigin();
		jumpDir.Normalize();
		GetPhysics()->SetAxis(jumpDir.ToMat3());

		startPosition = GetPhysics()->GetOrigin();
		targetPosition = trWire.endpos;
		targetMovedir = jumpDir;
		targetNormal = trWire.c.normal;

		stateTimer = gameLocal.time;

		//C A T   H A I R S
		//idEntityFx::StartFx("fx/cat_hairs", &startPosition, &fxMat, NULL, false);
		splashArgs.Set("smoke", "cat_hairs.prt");
		splashArgs.Set("start_off", "0");
		splashEnt = static_cast<idFuncSmoke *>(gameLocal.SpawnEntityType(idFuncSmoke::Type, &splashArgs));
		splashEnt->GetPhysics()->SetOrigin(GetPhysics()->GetOrigin());
		splashEnt->Bind(this, false);
		//splashEnt->PostEventMS(&EV_Activate, 0, this);
		splashEnt->PostEventMS(&EV_Remove, CAT_MOVETIME + 50);


		UpdateVisuals();

		

		return;
	}


	//Failed to find a place to jump to. So.....

	Event_PlayAnim("jumpup", 1);


	catState = CAT_FAILSAFEJUMP;
	startPosition = GetPhysics()->GetOrigin() + idVec3(0,0,48);
	targetPosition = GetPhysics()->GetOrigin();
	stateTimer = gameLocal.time;


}

//Gets called when cat bursts out of cage.
void idCat::DoCageBurst()
{
	Event_PlayAnim("jump_rescue", 1);
	catState = CAT_RESCUEJUMP;
	stateTimer = gameLocal.time + RESCUEJUMP_TIME;

	SetHeadlight(true);
}

void idCat::SetHeadlight(bool value)
{
	if (value)
	{
		if (headlightHandle == -1)
		{
			headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 32.0f;
			headlight.shaderParms[0] = 0.4f; // R
			headlight.shaderParms[1] = 0.4f; // G
			headlight.shaderParms[2] = 0.4f; // B
			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = true;
			headlight.axis = mat3_identity;
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
		}
	}
	else
	{
		if (headlightHandle != -1)
		{
			gameRenderWorld->FreeLightDef(headlightHandle);
		}
	}
}

bool idCat::IsAvailable()
{
	return (catState == CAT_AVAILABLE);
}

bool idCat::IsRescued()
{
	return (catState != CAT_CAGED && catState != CAT_RESCUEJUMP);
}

bool idCat::IsCaged()
{
	return (catState == CAT_CAGED);
}

void idCat::PutInPod(idVec3 spawnPos, idVec3 cubbyPos, int delaytime)
{
	catState = CAT_JUMPING_TO_CATPOD_DELAY;
	stateTimer = gameLocal.time + delaytime;
	startPosition = spawnPos;
	targetPosition = cubbyPos;
}

bool idCat::IsInPod()
{
	return (catState == CAT_JUMPING_TO_CATPOD_DELAY
		|| catState == CAT_JUMPING_TO_CATPOD
		|| catState == CAT_JUMP_TO_CATPOD_DONE);
}
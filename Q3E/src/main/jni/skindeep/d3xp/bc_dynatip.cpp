#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "BrittleFracture.h"
#include "bc_notewall.h"
#include "bc_dynatip.h"

#define ICONWIDTH 18
#define ICONHEIGHT 24

#define ARROWWIDTH 16
#define ARROWHEIGHT 16
#define ARROWDISTANCE 24

#define COMPASSSIZE 60

#define ONSCREEN_THRESHOLD_X 300
#define ONSCREEN_THRESHOLD_Y 250

#define LERPTIME 200



const idEventDef EV_dyna_setDynatipActive("setDynatipActive", "d");
const idEventDef EV_dyna_setDynatipTarget("setDynatipTarget", "e");

CLASS_DECLARATION(idEntity, idDynaTip)
	EVENT(EV_dyna_setDynatipActive, idDynaTip::SetDynatipActive)
	EVENT(EV_dyna_setDynatipTarget, idDynaTip::Event_SetTarget)
END_CLASS

idDynaTip::idDynaTip(void)
{
	dynatipNode.SetOwner(this);
	dynatipNode.AddToEnd(gameLocal.dynatipEntities);

	displayText = "";
	offscreenDrawPos = vec2_zero;
	tipState = TIP_OFFSCREEN;
	lerpTimer = 0;
	losTimer = 0;
	initialized = false;
	dotAng = -1.0f;
}

idDynaTip::~idDynaTip(void)
{
	dynatipNode.Remove();
}

void idDynaTip::Spawn(void)
{
	idStr keybind = gameLocal.GetKeyFromBinding(spawnArgs.GetString("keybind"));
	displayText = idStr::Format(spawnArgs.GetString("text"), keybind.c_str());

	drawOffset = spawnArgs.GetVector("drawOffset");

	hasGainedInitialLOS = spawnArgs.GetBool("force_on"); //if we want to skip the check that ensures the player has LOS before initializing dynatip logic

	BecomeActive(TH_THINK);
	PostEventMS(&EV_PostSpawn, 0);
}

void idDynaTip::SetDynatipActive(int value)
{
	tipState = (value >= 1) ? TIP_NODRAW : TIP_DORMANT;
	BecomeActive(TH_THINK);
}

void idDynaTip::Event_PostSpawn(void)
{
	if (targets.Num() <= 0)
	{
		gameLocal.Error("DynaTip '%s' has no targets.", GetName());
		return;
	}

	targetEnt = targets[0];
	if (!targetEnt.IsValid())
	{
		gameLocal.Error("DynaTip '%s' has invalid target.", GetName());
		return;
	}

	//success, found target ent.
	idStr materialName = spawnArgs.GetString("mtr_icon");
	iconMaterial = declManager->FindMaterial(materialName.c_str());
	if (iconMaterial == NULL)
	{
		gameLocal.Error("DynaTip '%s' has invalid icon material '%s'.", GetName(), materialName.c_str());
		return;
	}

	idStr arrowName = spawnArgs.GetString("mtr_arrow");
	arrowMaterial = declManager->FindMaterial(arrowName.c_str());
	if (arrowMaterial == NULL)
	{
		gameLocal.Error("DynaTip '%s' has invalid arrow material '%s'.", GetName(), arrowName.c_str());
		return;
	}	

	if (!spawnArgs.GetBool("start_on", "1"))
	{
		SetDynatipActive(0);
	}

	initialized = true;

	// timeGroup = TIME_GROUP2; // doesn't fix camera update lag
}

void idDynaTip::Save(idSaveGame *savefile) const
{
	savefile->WriteString( displayText ); //  idString displayText
	savefile->WriteObject( targetEnt ); //  idEntityPtr<idEntity> targetEnt
	savefile->WriteMaterial( iconMaterial ); // const  idMaterial *		 iconMaterial
	savefile->WriteMaterial( arrowMaterial ); // const  idMaterial *		 arrowMaterial
	savefile->WriteVec2( offscreenDrawPos ); //  idVec2 offscreenDrawPos
	savefile->WriteVec2( actualscreenDrawPos ); //  idVec2 actualscreenDrawPos

	savefile->WriteVec3( drawOffset ); //  idVec3 drawOffset

	savefile->WriteFloat( dotAng ); //  float dotAng

	savefile->WriteInt( tipState ); //  int tipState
	savefile->WriteInt( lerpTimer ); //  int lerpTimer

	savefile->WriteBool( hasGainedInitialLOS ); //  bool hasGainedInitialLOS
	savefile->WriteInt( losTimer ); //  int losTimer

	savefile->WriteBool( initialized ); //  bool initialized
}

void idDynaTip::Restore(idRestoreGame *savefile)
{
	savefile->ReadString( displayText ); //  idString displayText
	savefile->ReadObject( targetEnt ); //  idEntityPtr<idEntity> targetEnt
	savefile->ReadMaterial( iconMaterial ); // const  idMaterial *		 iconMaterial
	savefile->ReadMaterial( arrowMaterial ); // const  idMaterial *		 arrowMaterial
	savefile->ReadVec2( offscreenDrawPos ); //  idVec2 offscreenDrawPos
	savefile->ReadVec2( actualscreenDrawPos ); //  idVec2 actualscreenDrawPos

	savefile->ReadVec3( drawOffset ); //  idVec3 drawOffset

	savefile->ReadFloat( dotAng ); //  float dotAng

	savefile->ReadInt( tipState ); //  int tipState
	savefile->ReadInt( lerpTimer ); //  int lerpTimer

	savefile->ReadBool( hasGainedInitialLOS ); //  bool hasGainedInitialLOS
	savefile->ReadInt( losTimer ); //  int losTimer

	savefile->ReadBool( initialized ); //  bool initialized
}

void idDynaTip::SetDynatipComplete()
{
	BecomeInactive(TH_THINK);

	// SW: call script when dynatip is completed (for vignettes, scripted sequences, etc)
	idStr scriptName;
	if (spawnArgs.GetString("callOnComplete", "", scriptName) && !scriptName.IsEmpty())
	{
		gameLocal.RunMapScript(scriptName);
	}

	// Allows us to reset the dynatip later if we choose to (e.g. for resetting tutorial rooms)
	if (spawnArgs.GetBool("removeOnComplete", "1"))
	{
		PostEventMS(&EV_Remove, 0);
	}
	
}

void idDynaTip::RecalculatePosition()
{
	// SW: Skip recalculating position if entity is destroyed
	// (this accommodates for the scenario where the entity is destroyed after our Think(), but before our Draw())
	if (!targetEnt.IsValid() || targetEnt.GetEntity()->GetPhysics() == NULL)
		return;

	//dotproduct to see if it's behind me.
	idVec3 targetEntPos = targetEnt.GetEntity()->GetPhysics()->GetOrigin();
	if (targetEnt.GetEntity()->IsType(idBrittleFracture::Type))
	{
		//BrittleFracture origin point will move around depending on shard configuration, so force it to be center.
		targetEntPos = targetEnt.GetEntity()->spawnArgs.GetVector("origin");
	}
	else
	{
		targetEntPos = gameLocal.GetLocalPlayer()->GetZoominspectAdjustedPosition(targetEnt.GetEntity());
	}


	idVec3 dirToTarget = targetEntPos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
	dirToTarget.Normalize();
	float vdot = DotProduct(dirToTarget, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
	dotAng = vdot;

	idVec2 entScreenPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(targetEntPos);
	idVec2 dirToScreenPos = entScreenPos - idVec2(320, 240);
	dirToScreenPos.Normalize();

	float finalAngle = atan2(dirToScreenPos.x, dirToScreenPos.y);

	if (vdot > 0)
		finalAngle += DEG2RAD(90);
	else
		finalAngle += DEG2RAD(-90);

	offscreenDrawPos = vec2_zero;
	offscreenDrawPos.x -= cos(finalAngle) * COMPASSSIZE;
	offscreenDrawPos.y += sin(finalAngle) * COMPASSSIZE;

	//For the on-screen sprite pos, we raise it up a little so that it doesn't overlap the object.
	idVec3 iconWorldPos;
	if (drawOffset != vec3_zero)
	{
		iconWorldPos = GetIconPos();
	}
	else
	{
		iconWorldPos = targetEntPos;
	}

	actualscreenDrawPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(iconWorldPos);
}

void idDynaTip::Think(void)
{
	//idEntity::Think();

	if (!initialized)
		return;

	if (tipState == TIP_DORMANT)
		return;

	if (!targetEnt.IsValid())
	{
		SetDynatipComplete();
		return;
	}

	if (targetEnt.GetEntity()->IsType(idNoteWall::Type))
	{
		if (static_cast<idNoteWall *>(targetEnt.GetEntity())->GetRead())
		{
			//Done. Delete this dynatip.
			SetDynatipComplete();
			return;
		}
	}

	if (targetEnt.GetEntity()->spawnArgs.GetInt("dyna_inspectable") == 2)
	{
		SetDynatipComplete();
		return;
	}

	if (!gameLocal.InPlayerPVS(targetEnt.GetEntity()))
	{
		tipState = TIP_NODRAW;
		return;
	}

	//Custom maximum distance that dynatip can appear.
	float distance = (targetEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetEyePosition()).Length();
	if (distance > spawnArgs.GetFloat("maxdistance"))
	{
		tipState = TIP_NODRAW;
		return;
	}

	RecalculatePosition();

	if (tipState == TIP_OFFSCREEN || tipState == TIP_ONSCREEN || tipState == TIP_NODRAW)
	{
		if (!hasGainedInitialLOS)
		{
			//In order for dynatip to first appear, player needs to gain LOS to it.
			if (gameLocal.time > losTimer)
			{
				losTimer = gameLocal.time + 200;

				//do LOS check.
				idVec3 iconPos = GetIconPos();				
				trace_t tr;
				gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, iconPos, MASK_OPAQUE, NULL);
				float trDist = (iconPos - tr.endpos).Length();
				if (trDist <= 1)
				{
					//gained LOS. Great. Next update, begin drawing the thing!
					StartSound("snd_appear", SND_CHANNEL_ANY);
					hasGainedInitialLOS = true;
				}
			}

			return;
		}

		//dotproduct to see if it's behind me.
		idVec3 dirToTarget = targetEnt.GetEntity()->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->firstPersonViewOrigin;
		dirToTarget.Normalize();
		float vdot = DotProduct(dirToTarget, gameLocal.GetLocalPlayer()->viewAngles.ToForward());
		if (vdot > 0
			&& (actualscreenDrawPos.x > 320 - ONSCREEN_THRESHOLD_X / 2) && (actualscreenDrawPos.x < 320 + ONSCREEN_THRESHOLD_X / 2)
			&& (actualscreenDrawPos.y > 240 - ONSCREEN_THRESHOLD_Y / 2) && (actualscreenDrawPos.y < 240 + ONSCREEN_THRESHOLD_Y / 2))
		{
			//transitioning to onscreen.

			if (tipState == TIP_OFFSCREEN)
			{
				tipState = TIP_LERPINGTO_ONSCREEN;
				lerpTimer = gameLocal.time;
			}
			else
			{
				tipState = TIP_ONSCREEN;
			}
		}
		else
		{
			//transitioning to offscreen.

			if (tipState == TIP_ONSCREEN)
			{
				tipState = TIP_LERPINGTO_OFFSCREEN;
				lerpTimer = gameLocal.time;
			}
			else
			{
				tipState = TIP_OFFSCREEN;
			}
		}
	}
	else if (tipState == TIP_LERPINGTO_ONSCREEN || tipState == TIP_LERPINGTO_OFFSCREEN)
	{
		//handle the lerp logic.
		float lerp = (gameLocal.time - lerpTimer) / (float)LERPTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		if (tipState == TIP_LERPINGTO_OFFSCREEN)
			lerp = 1 - lerp;

		if (gameLocal.time >= lerpTimer + LERPTIME)
		{
			if (tipState == TIP_LERPINGTO_ONSCREEN)
				tipState = TIP_ONSCREEN;
			else
				tipState = TIP_OFFSCREEN;
		}
	}
}

idVec3 idDynaTip::GetIconPos()
{
	idVec3 forward, right, up;
	targetEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);


	//return targetEnt.GetEntity()->GetPhysics()->GetOrigin() + (forward * drawOffset.x) + (right * drawOffset.y) + (up * drawOffset.z);
	return targetEnt.GetEntity()->GetPhysics()->GetOrigin() + (forward * drawOffset.x) + (right * drawOffset.y) + idVec3(0, 0, drawOffset.z);
}

void idDynaTip::Draw(void)
{
	if (!initialized)
		return;

	RecalculatePosition(); // recalcing dynatip position code because of lagged update of localplayer camera

	if (tipState == TIP_OFFSCREEN)
	{
		idVec2 drawPos = idVec2(320, 240) + offscreenDrawPos;

		//Draw icon.
		renderSystem->SetColor(idVec4(1, 1, 1, 1));
		renderSystem->DrawStretchPic(
			drawPos.x - ICONWIDTH / 2,
			drawPos.y - ICONHEIGHT / 2,
			ICONWIDTH, ICONHEIGHT,
			0, 0, 1, 1, iconMaterial);

		//Arrow.
		idVec2 arrowDir = drawPos - idVec2(320, 240);
		arrowDir.Normalize();
		idVec2 arrowPos = drawPos + arrowDir * ARROWDISTANCE;
		float arrowAngle = atan2(arrowDir.x, arrowDir.y);
		arrowAngle += DEG2RAD(180);
		renderSystem->DrawStretchPicRotated(
			arrowPos.x - ARROWWIDTH / 2,
			arrowPos.y - ARROWHEIGHT / 2,
			ARROWWIDTH, ARROWHEIGHT,
			0, 0, 1, 1, arrowMaterial, arrowAngle);
	}
	else if (tipState == TIP_ONSCREEN)
	{
		renderSystem->SetColor(idVec4(1, 1, 1, 1));
		renderSystem->DrawStretchPic(
			actualscreenDrawPos.x - ICONWIDTH / 2,
			actualscreenDrawPos.y - (ICONHEIGHT / 2),
			ICONWIDTH, ICONHEIGHT,
			0, 0, 1, 1, iconMaterial);

		// text moved to DrawText(void)
	}
	else if (tipState == TIP_LERPINGTO_ONSCREEN || tipState == TIP_LERPINGTO_OFFSCREEN)
	{
		//Lerping.
		float lerp = (gameLocal.time - lerpTimer) / (float)LERPTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		if (tipState == TIP_LERPINGTO_OFFSCREEN)
			lerp = 1 - lerp;

		idVec2 lerpedPos;
		lerpedPos.Lerp(idVec2(320, 240) + offscreenDrawPos, actualscreenDrawPos, lerp);

		renderSystem->SetColor(idVec4(1, 1, 1, 1));
		renderSystem->DrawStretchPic(
			lerpedPos.x - ICONWIDTH / 2,
			lerpedPos.y - (ICONHEIGHT / 2),
			ICONWIDTH, ICONHEIGHT,
			0, 0, 1, 1, iconMaterial);
	}
}

void idDynaTip::DrawText(void)
{
	if (!initialized)
		return;

	if (tipState == TIP_ONSCREEN)
	{
		#define TEXTGAP 3
		idVec2 textPos = idVec2(actualscreenDrawPos.x + ICONWIDTH / 2 + TEXTGAP, actualscreenDrawPos.y - ICONHEIGHT / 2 + 2);
		gameLocal.GetLocalPlayer()->hud->DrawArbitraryText(displayText.c_str(), .6f, idVec4(1, 1, 1, 1), textPos.x, textPos.y, "sofia", idVec4(0, 0, 0, 1), idDeviceContext::ALIGN_LEFT);
	}
}


// SW: Let us change dynatip target on the fly (e.g. if we want to assign it to one of many objects based on some criteria)
// This *should* be safe to do while the dynatip is visible, but it's not the intended application.
void idDynaTip::Event_SetTarget(idEntity* entity)
{
	targetEnt = entity;
}

		
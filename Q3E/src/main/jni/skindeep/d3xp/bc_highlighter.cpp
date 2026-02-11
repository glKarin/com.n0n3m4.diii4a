//#include "script/Script_Thread.h"
#include "framework/DeclEntityDef.h"
#include "gamesys/SysCvar.h"
#include "idlib/LangDict.h"

#include "WorldSpawn.h"
#include "Player.h"
#include "Actor.h"
#include "bc_meta.h"
#include "bc_highlighter.h"

const int	LERPON_TIME_BEHIND		= 400;		//time to lerp highlight on. (TARGET IS BEHIND)
const int	LERPON_TIME_ALMOSTLOOK	= 200;		//time to lerp highlight on. (TARGET IS NEAR SIDE OF SCREEN)


const int	PAUSETIME = 2000;		//how long highlight stays onscreen.
const int	LERPOFF_TIME = 200;		//time to lerp the highlight off.
const float ARROWVARIANCE = .3f;	//random angle variance for arrows.
const int	COOLDOWNTIME = 1000;	//time cooldown between highlight activation.

#define BUFFERSIZE 16				//how much buffer space we give around the object of interest
#define LETTERBOX_MINSIZE 64
#define LETTERBOX_LERPTIME 300

#define TOPBAR_DEFAULTY		60
#define BOTTOMBAR_DEFAULTY	420

CLASS_DECLARATION(idEntity, idHighlighter)
END_CLASS

idHighlighter::idHighlighter()
{
	highlightCamera = nullptr;

	cameraStartAngle = {};
	cameraEndAngle = {};

	state = HLR_NONE;
	stateTimer = 0;
	transitionTime = 0;
	
	for (int i = 0; i < MAX_ARROWS; i++)
	{
		arrowPositions[i] = vec2_zero;
		arrowAngles[i] = 0;
		entNames[i] = NULL;
	}

	for (int i = 0; i < 2; i++)
	{
		barYPositions[i] = 0;
	}

	cooldownTimer = 0;

	for (int i = 0; i < 2; i++)
	{
		highlightEntityInfos[i].bounds = idBounds();
		highlightEntityInfos[i].name = "";
		highlightEntityInfos[i].position = vec3_zero;
	}

	cameraRotateActive = false;
	letterboxTimer = 0;
	letterboxLerpActive = false;
	cameraOffsetPosition = vec3_zero;
	cameraOffsetActive = false;
	cameraStartOffsetPosition = vec3_zero;

	worldIsPaused = false;
}

idHighlighter::~idHighlighter()
{
}


void idHighlighter::Spawn()
{
	idDict args;

	//camera when the event is off-screen.
	idEntity* newcamera;	
	args.Clear();
	args.Set("classname", "func_cameraview");
	args.SetInt("fov", 90);
	gameLocal.SpawnEntityDef(args, &newcamera);
	if (!newcamera|| !newcamera->IsType(idCameraView::Type))
	{
		common->Error("Highlight camera setup had a fatal error.\n");
		return;
	}
	highlightCamera = static_cast<idCameraView*>(newcamera);

	BecomeActive(TH_THINK);
}

void idHighlighter::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( state ); // int state

	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( transitionTime ); // int transitionTime

	SaveFileWriteArray(entNames, MAX_ARROWS, WriteString); // idString entNames[MAX_ARROWS]
	SaveFileWriteArray(arrowPositions, MAX_ARROWS, WriteVec2); // idVec2 arrowPositions[MAX_ARROWS]
	SaveFileWriteArray(arrowAngles, MAX_ARROWS, WriteFloat); // float arrowAngles[MAX_ARROWS]

	SaveFileWriteArray(barYPositions, 2, WriteInt); // int barYPositions[2]

	savefile->WriteInt( letterboxTimer ); // int letterboxTimer
	savefile->WriteBool( letterboxLerpActive ); // bool letterboxLerpActive

	savefile->WriteInt( cooldownTimer ); // int cooldownTimer
	savefile->WriteObject( highlightCamera ); // idCameraView* highlightCamera

	savefile->WriteInt(2); // highlightEntity_t highlightEntityInfos[2];
	for (int idx = 0; idx < 2; idx++)
	{
		savefile->WriteVec3( highlightEntityInfos[idx].position ); // idVec3 position
		savefile->WriteString( highlightEntityInfos[idx].name ); // idString name
		savefile->WriteBounds( highlightEntityInfos[idx].bounds ); // idBounds bounds
	}

	savefile->WriteBool( cameraRotateActive ); // bool cameraRotateActive
	savefile->WriteAngles( cameraStartAngle ); // idAngles cameraStartAngle
	savefile->WriteAngles( cameraEndAngle ); // idAngles cameraEndAngle


	savefile->WriteBool( cameraOffsetActive ); // bool cameraOffsetActive
	savefile->WriteVec3( cameraOffsetPosition ); // idVec3 cameraOffsetPosition
	savefile->WriteVec3( cameraStartOffsetPosition ); // idVec3 cameraStartOffsetPosition

	savefile->WriteBool( worldIsPaused ); // bool worldIsPaused
}

void idHighlighter::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( state ); // int state

	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( transitionTime ); // int transitionTime

	SaveFileReadArray(entNames, ReadString); // idString entNames[MAX_ARROWS]
	SaveFileReadArray(arrowPositions, ReadVec2); // idVec2 arrowPositions[MAX_ARROWS]
	SaveFileReadArray(arrowAngles, ReadFloat); // float arrowAngles[MAX_ARROWS]

	SaveFileReadArray(barYPositions, ReadInt); // int barYPositions[2]

	savefile->ReadInt( letterboxTimer ); // int letterboxTimer
	savefile->ReadBool( letterboxLerpActive ); // bool letterboxLerpActive

	savefile->ReadInt( cooldownTimer ); // int cooldownTimer
	savefile->ReadObject( CastClassPtrRef(highlightCamera) ); // idCameraView* highlightCamera

	int num;
	savefile->ReadInt(num); // highlightEntity_t highlightEntityInfos[2];
	for (int idx = 0; idx < num; idx++)
	{
		savefile->ReadVec3( highlightEntityInfos[idx].position ); // idVec3 position
		savefile->ReadString( highlightEntityInfos[idx].name ); // idString name
		savefile->ReadBounds( highlightEntityInfos[idx].bounds ); // idBounds bounds
	}

	savefile->ReadBool( cameraRotateActive ); // bool cameraRotateActive
	savefile->ReadAngles( cameraStartAngle ); // idAngles cameraStartAngle
	savefile->ReadAngles( cameraEndAngle ); // idAngles cameraEndAngle


	savefile->ReadBool( cameraOffsetActive ); // bool cameraOffsetActive
	savefile->ReadVec3( cameraOffsetPosition ); // idVec3 cameraOffsetPosition
	savefile->ReadVec3( cameraStartOffsetPosition ); // idVec3 cameraStartOffsetPosition

	savefile->ReadBool( worldIsPaused ); // bool worldIsPaused
}

void idHighlighter::Think(void)
{
	if (state == HLR_LERPON)
	{
		float lerp = (gameLocal.hudTime - stateTimer) / (float)transitionTime;
		lerp = idMath::ClampFloat(0, 1, lerp);

		if (gameLocal.hudTime > stateTimer && !worldIsPaused)
		{
			gameLocal.menuPause = true; //Pause the game world. We do this now so that the game world progresses forward a little bit to allow things like explosions, etc to visually appear/spawn.
			worldIsPaused = true;
		}

		if (cameraRotateActive)
		{
			//rotate the camera.
			idAngles lerpedCameraAngle;
			lerpedCameraAngle.pitch = idMath::Lerp(cameraStartAngle.pitch, cameraEndAngle.pitch, lerp);
			lerpedCameraAngle.yaw = idMath::Lerp(cameraStartAngle.yaw, cameraEndAngle.yaw, lerp);
			lerpedCameraAngle.roll = idMath::Lerp(cameraStartAngle.roll, cameraEndAngle.roll, lerp);

			highlightCamera->SetAxis(lerpedCameraAngle.ToMat3());
		}

		if (cameraOffsetActive)
		{
			idVec3 lerpedCameraPosition;
			lerpedCameraPosition.x = idMath::Lerp(cameraStartOffsetPosition.x, cameraOffsetPosition.x, lerp);
			lerpedCameraPosition.y = idMath::Lerp(cameraStartOffsetPosition.y, cameraOffsetPosition.y, lerp);
			lerpedCameraPosition.z = idMath::Lerp(cameraStartOffsetPosition.z, cameraOffsetPosition.z, lerp);
			highlightCamera->SetOrigin(lerpedCameraPosition);
		}

		
		#define EXTRA_CAMERA_LERPONTIME 50 //We add a little bit of extra time, so that the camera has enough time to settle in, and therefore the arrow positions are accurate.
		if (gameLocal.hudTime >= stateTimer + transitionTime + EXTRA_CAMERA_LERPONTIME) 
		{
			InitializeArrows(highlightEntityInfos[0], highlightEntityInfos[1]);
			state = HLR_PAUSED;
			stateTimer = gameLocal.hudTime;

			letterboxTimer = gameLocal.hudTime;
			letterboxLerpActive = true;
		}
	}
	else if (state == HLR_PAUSED)
	{		

		if (gameLocal.hudTime >= stateTimer + PAUSETIME)
		{
			DoSkip();
		}
	}
	else if (state == HLR_LERPOFF)
	{
		float lerp = (gameLocal.hudTime - stateTimer) / (float)LERPOFF_TIME;
		lerp = idMath::ClampFloat(0, 1, lerp);

		if (cameraRotateActive)
		{
			//rotate the camera back to its original angle.
			idAngles lerpedCameraAngle;
			lerpedCameraAngle.pitch = idMath::Lerp( cameraEndAngle.pitch, cameraStartAngle.pitch, lerp);
			lerpedCameraAngle.yaw = idMath::Lerp(cameraEndAngle.yaw, cameraStartAngle.yaw,  lerp);
			lerpedCameraAngle.roll = idMath::Lerp(cameraEndAngle.roll, cameraStartAngle.roll,  lerp);

			highlightCamera->SetAxis(lerpedCameraAngle.ToMat3());
			highlightCamera->UpdateVisuals();
		}

		if (cameraOffsetActive)
		{
			idVec3 lerpedCameraPosition;
			lerpedCameraPosition.x = idMath::Lerp( cameraOffsetPosition.x, cameraStartOffsetPosition.x, lerp);
			lerpedCameraPosition.y = idMath::Lerp(cameraOffsetPosition.y, cameraStartOffsetPosition.y, lerp);
			lerpedCameraPosition.z = idMath::Lerp(cameraOffsetPosition.z, cameraStartOffsetPosition.z, lerp);
			highlightCamera->SetOrigin(lerpedCameraPosition);
		}

		if (gameLocal.hudTime >= stateTimer + LERPOFF_TIME)
		{
			state = HLR_NONE;
			gameLocal.menuPause = false;

			if (cameraRotateActive || gameLocal.GetLocalPlayer()->IsHidden())
			{
				// SW 26th March 2025: Pretty sure this is supposed to be a 'show', not 'hide'
				gameLocal.GetLocalPlayer()->Show();
				highlightCamera->SetActive(false);
			}
		}
	}
}


//Generally:
//ent0 = the thing that got killed.
//ent1 = the inflicting object
bool idHighlighter::DoHighlight(idEntity* ent0, idEntity* ent1)
{
	if (!g_eventhighlighter.GetBool())
		return false;

	if (gameLocal.time < 1000) //if at game start, ignore.
		return false;

	if (ent0 == NULL)
		return false;

	if (cooldownTimer > gameLocal.time)
		return false;

	//BC 3-14-2025: skip if player is dead.
	if (gameLocal.GetLocalPlayer()->health <= 0)
		return false;

	// SW 26th March 2025: skip if player is defibrillating
	if (gameLocal.GetLocalPlayer()->GetDefibState())
		return false;

	cooldownTimer = gameLocal.time + COOLDOWNTIME;

	int eventVisibleToPlayer = IsEventVisibleToPlayer(ent0, ent1);

	cameraOffsetActive = false;
	if (eventVisibleToPlayer == VIS_INVALID)
	{
		//No LOS from here.
		//Try to find a viable camera position.
		//This will result in lerping the camera to a new position that has LOS to the entity.
		cameraOffsetPosition = FindViableCameraOffset(ent0);
		if (cameraOffsetPosition == vec3_zero)
		{
			//Failed to find a viable camera position. Exit here.
			return false;
		}

		//Found a viable camera position.
		eventVisibleToPlayer = VIS_BEHIND;
		cameraOffsetActive = true;
		cameraStartOffsetPosition = gameLocal.GetLocalPlayer()->renderView->vieworg; // SW 26th March 2025: Adjusting how we calculate the view origin. This should accommodate for different camera heights and also jockey mode
	}
	
	if (eventVisibleToPlayer == VIS_BEHIND || eventVisibleToPlayer == VIS_ALMOSTDIRECTLOOK)
	{
		//The event is off-screen.
		//Enable the highlightcamera.
		cameraRotateActive = true;
		// SW 26th March 2025: don't hide the player if we're jockeying (i.e. third-person)
		if (!gameLocal.GetLocalPlayer()->IsJockeying())
		{
			gameLocal.GetLocalPlayer()->Hide();
		}
		

		highlightCamera->SetOrigin(gameLocal.GetLocalPlayer()->renderView->vieworg);
		highlightCamera->SetAxis(gameLocal.GetLocalPlayer()->renderView->viewaxis);

		cameraStartAngle = gameLocal.GetLocalPlayer()->renderView->viewaxis.ToAngles();
		cameraStartAngle.yaw = idMath::AngleNormalize360(cameraStartAngle.yaw);

		//cameraMover->SetOrigin(gameLocal.GetLocalPlayer()->firstPersonViewOrigin);
		//cameraMover->SetAxis(gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		//Where to aim camera at.
		idAngles targetAngle;
		idVec3 targetPosition = GetEntityLookpoint(ent0);		
		
		//gameRenderWorld->DebugArrowSimple(targetPosition);
		//gameRenderWorld->DebugLine(colorGreen, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, targetPosition, 90000);

		idVec3 cameraEndPosition = (cameraOffsetPosition != vec3_zero) ? cameraOffsetPosition : gameLocal.GetLocalPlayer()->renderView->vieworg;

		cameraEndAngle = (targetPosition - cameraEndPosition).ToAngles();
		cameraEndAngle.pitch = idMath::AngleNormalize180(cameraEndAngle.pitch); // SW 26th March 2025: Making it so pitch is -90 to 90 so the camera doesn't do a sicknasty flip if the highlight point is above us
		cameraEndAngle.yaw = idMath::AngleNormalize360(cameraEndAngle.yaw);
		cameraEndAngle.roll = 0;

		//Do some maths to ensure the camera does the shortest movement distance possible.
		float delta_yaw = cameraStartAngle.yaw - cameraEndAngle.yaw;
		if (idMath::Fabs(delta_yaw) > 180.f)
		{
			if (cameraStartAngle.yaw > cameraEndAngle.yaw)
				cameraEndAngle.yaw += 360;
			else
				cameraStartAngle.yaw += 360;
		}

		highlightCamera->SetActive(true);
	}
	else
	{
		cameraRotateActive = false;
	}


	if (eventVisibleToPlayer == VIS_BEHIND)
		transitionTime = LERPON_TIME_BEHIND;
	else if (eventVisibleToPlayer == VIS_ALMOSTDIRECTLOOK)
		transitionTime = LERPON_TIME_ALMOSTLOOK;
	else
		transitionTime = 10;
	
	stateTimer = gameLocal.hudTime; //We use Hudtime because Hudtime doesn't pause. (normal Time does pause, so don't use it here)	
	state = HLR_LERPON;

	

	SetHighlightEntityInfo(ent0, ent1);	

	worldIsPaused = false;

	return true;
}

//Try to find a camera position that is NOT the player's first-person view origin.
idVec3 idHighlighter::FindViableCameraOffset(idEntity* ent)
{
	//forward, right, up (up not used)
	//We bias the movement to strafing, as it looks/feels weird to lerp forward.
	#define OFFSETARRAYSIZE 6
	idVec3 offsetArray[] =
	{
		idVec3(0, 32, 0),
		idVec3(0, -32, 0),
		idVec3(0, 64, 0),
		idVec3(0, -64, 0),

		idVec3(32, 32, 0),
		idVec3(32, -32, 0),
	};

	idVec3 lookpoint = GetEntityLookpoint(ent);

	idVec3 forward, right;
	idAngles playerViewAngles = gameLocal.GetLocalPlayer()->viewAngles;
	playerViewAngles.pitch = 0;
	playerViewAngles.roll = 0;
	playerViewAngles.ToVectors(&forward, &right, NULL);

	for (int i = 0; i < OFFSETARRAYSIZE; i++)
	{
		// SW 26th March 2025: Adjusting how we calculate the view origin. This should accommodate for different camera heights and also jockey mode
		idVec3 playerCameraPos = gameLocal.GetLocalPlayer()->renderView->vieworg + (forward * offsetArray[i].x) + (right * offsetArray[i].y);
		
		trace_t wallcheckTr;
		// SW 26th March 2025: Adjusting how we calculate the view origin. This should accommodate for different camera heights and also jockey mode
		gameLocal.clip.TracePoint(wallcheckTr, gameLocal.GetLocalPlayer()->renderView->vieworg, playerCameraPos, MASK_OPAQUE, gameLocal.GetLocalPlayer());
		if (wallcheckTr.fraction < 1)
			continue; //There's a wall or something in the way.

		trace_t tr;
		gameLocal.clip.TracePoint(tr, playerCameraPos, lookpoint, MASK_SOLID, NULL);
		if (CanTraceSeeEntity(tr, ent))
		{
			return playerCameraPos;
		}
	}

	return vec3_zero;
}

//Because there's a delay between the highlighter activation and the arrow presentation, it's possible that the
//entity may be gone/deleted/null. So, we record entity info immediately when the highlighter is activated.
void idHighlighter::SetHighlightEntityInfo(idEntity* ent0, idEntity* ent1)
{
	//Set the highlight entity infos.
	for (int i = 0; i < 2; i++)
	{
		idEntity* curEnt = (i <= 0) ? ent0 : ent1;

		if (curEnt == nullptr || curEnt == gameLocal.GetLocalPlayer())
		{
			highlightEntityInfos[i].bounds = idBounds();
			highlightEntityInfos[i].name = "";
			highlightEntityInfos[i].position = vec3_zero;
			continue;
		}

		idVec3 adjustedPosition;
		if (curEnt->IsType(idActor::Type))
		{
			adjustedPosition = static_cast<idActor*>(curEnt)->GetEyeBonePosition() + idVec3(0, 0, -2);
		}
		else
		{
			adjustedPosition = curEnt->GetPhysics()->GetOrigin();
		}

		idBounds curBounds;
		if (curEnt->spawnArgs.GetBool("highlight_boundcheck", "1"))
		{
			curBounds = idBounds(idVec3(curEnt->GetPhysics()->GetOrigin().x, curEnt->GetPhysics()->GetOrigin().y, curEnt->GetPhysics()->GetAbsBounds()[0].z - BUFFERSIZE), idVec3(curEnt->GetPhysics()->GetOrigin().x, curEnt->GetPhysics()->GetOrigin().y, curEnt->GetPhysics()->GetAbsBounds()[1].z + BUFFERSIZE));
		}
		else
		{
			curBounds = idBounds(idVec3(curEnt->GetPhysics()->GetOrigin().x, curEnt->GetPhysics()->GetOrigin().y, curEnt->GetPhysics()->GetOrigin().z - BUFFERSIZE), idVec3(curEnt->GetPhysics()->GetOrigin().x, curEnt->GetPhysics()->GetOrigin().y, curEnt->GetPhysics()->GetOrigin().z + BUFFERSIZE));
		}

		highlightEntityInfos[i].bounds = curBounds;
		highlightEntityInfos[i].name = ParseName(curEnt);;
		highlightEntityInfos[i].position = adjustedPosition;
	}
}

//This sets the positions of the arrows and labels. This gets called when HLR_LERPON transitions to HLR_PAUSED
void idHighlighter::InitializeArrows(highlightEntity_t hlEnt0, highlightEntity_t hlEnt1)
{
	//reset the arrows.
	for (int i = 0; i < MAX_ARROWS; i++)
	{
		arrowPositions[i] = vec2_zero;
		arrowAngles[i] = 0;
		entNames[i] = "";
	}

	//Calculate where the arrowhead pointy tip is.
	if (hlEnt0.name.Length() > 0)
	{
		arrowPositions[0] = gameLocal.GetLocalPlayer()->GetWorldToScreen(hlEnt0.position);
		entNames[0] = hlEnt0.name.c_str();
	}

	if (hlEnt1.name.Length() > 0)
	{		
		arrowPositions[1] = gameLocal.GetLocalPlayer()->GetWorldToScreen(hlEnt1.position);
		entNames[1] = hlEnt1.name.c_str();
	}

	//Calculate where the arrow should point.

	if (arrowPositions[1] == vec2_zero)
	{
		//There's only one arrow.
		arrowAngles[0] = .3f;
	}
	else
	{
		//There's 2 arrows.
		//Figure out which one is further left.
		if (arrowPositions[0].x < arrowPositions[1].x)
		{
			//arrow 0 is further left.
			arrowAngles[0] = 3.14 + (gameLocal.random.CRandomFloat() * ARROWVARIANCE);
			arrowAngles[1] = 0 + (gameLocal.random.CRandomFloat() * ARROWVARIANCE);
		}
		else
		{
			arrowAngles[0] = 0 + (gameLocal.random.CRandomFloat() * ARROWVARIANCE);
			arrowAngles[1] = 3.14 + (gameLocal.random.CRandomFloat() * ARROWVARIANCE);
		}
	}


	//Calculate where to draw the bars.

	//We dont want the bar to overlap the entities. Therefore, generate
	//a 'bubble' around the entities to create some buffer space.

	//First get ent0.

	idVec2 ent0_upperbound = GetUpperBound(hlEnt0.bounds);
	idVec2 ent0_lowerbound = GetLowerBound(hlEnt0.bounds);

	//gameRenderWorld->DebugArrowSimple(idVec3(ent0->GetPhysics()->GetOrigin().x, ent0->GetPhysics()->GetOrigin().y, ent0->GetPhysics()->GetAbsBounds()[1].z));
	//gameRenderWorld->DebugArrowSimple(idVec3(ent0->GetPhysics()->GetOrigin().x, ent0->GetPhysics()->GetOrigin().y, ent0->GetPhysics()->GetAbsBounds()[0].z));

	float screenTopY = ent0_upperbound.y;
	float screenBottomY = ent0_lowerbound.y;

	//compare to ent1.
	if (hlEnt1.name.Length()  > 0)
	{
		idVec2 ent1_upperbound = GetUpperBound(hlEnt1.bounds);
		idVec2 ent1_lowerbound = GetLowerBound(hlEnt1.bounds);

		//gameRenderWorld->DebugArrowSimple(idVec3(ent1->GetPhysics()->GetOrigin().x, ent1->GetPhysics()->GetOrigin().y, ent1->GetPhysics()->GetAbsBounds()[1].z));
		//gameRenderWorld->DebugArrowSimple(idVec3(ent1->GetPhysics()->GetOrigin().x, ent1->GetPhysics()->GetOrigin().y, ent1->GetPhysics()->GetAbsBounds()[0].z));

		if (ent1_upperbound.y < screenTopY && ent1_upperbound.y > 0)
			screenTopY = ent1_upperbound.y;

		if (ent1_lowerbound.y > screenBottomY && ent1_lowerbound.y < 480)
			screenBottomY = ent1_lowerbound.y;
	}

	barYPositions[0] = screenTopY;
	barYPositions[1] = screenBottomY;
}

idVec2 idHighlighter::GetUpperBound(idBounds entBounds)
{
	idVec2 barPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(entBounds[1]);
	barPos.y = max(barPos.y, LETTERBOX_MINSIZE);
	return barPos;
}

idVec2 idHighlighter::GetLowerBound(idBounds entBounds)
{
	idVec2 barPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(entBounds[0]);
	barPos.y = min(barPos.y, 480 - LETTERBOX_MINSIZE);
	return barPos;
}

idVec3 idHighlighter::GetEntityLookpoint(idEntity* ent)
{
	if (ent->IsType(idActor::Type))
	{
		return static_cast<idActor*>(ent)->GetEyeBonePosition() + idVec3(0, 0, -20);
	}
	
	
	return ent->GetPhysics()->GetOrigin();
}

int idHighlighter::IsEventVisibleToPlayer(idEntity* ent0, idEntity* ent1)
{
	//if (!gameLocal.InPlayerConnectedArea(ent0))
	//	return false;

	//traceline check.
	idVec3 lookpoint = GetEntityLookpoint(ent0);
	trace_t tr;
	// SW 26th March 2025: Adjusting how we calculate the view origin. This should accommodate for different camera heights and also jockey mode
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->renderView->vieworg, lookpoint, MASK_SOLID, NULL);
	if (!CanTraceSeeEntity(tr, ent0))
	{		
		return VIS_INVALID; //No direct LOS.
	}

	// SW 26th March 2025: Adjusting how we calculate the view origin. This should accommodate for different camera heights and also jockey mode
	idVec3 dirToEvent = lookpoint - gameLocal.GetLocalPlayer()->renderView->vieworg;
	dirToEvent.Normalize();
	float facingResult = DotProduct(dirToEvent, gameLocal.GetLocalPlayer()->viewAngles.ToForward());

	//Do a check to see how centered the event is on the screen. If it's at the edge of the screen, then return false.
	//0.99  = target is directly in front of player
	//0.0   = target is directly to side of player
	//-0.99 = target is directly behind player
	
	// SW 26th March 2025: Adjusting dot product threshold to stamp out some nasty edge-cases where the highlighter bars would cover the whole dang screen.
	// This is maybe a bit harsh yaw-wise, but pitch-wise it's necessary
	#define DOT_THRESHOLD .85f
	if (facingResult >= DOT_THRESHOLD)
		return VIS_DIRECTLOOK; //Looking directly at the target.	
	else if (facingResult > 0)
		return VIS_ALMOSTDIRECTLOOK;	//Target is onscreen, but is near-ish the edge of the screen

	return VIS_BEHIND;	//Can see target, but is off-screen.
}

bool idHighlighter::CanTraceSeeEntity(trace_t tr, idEntity* ent)
{
	if (tr.fraction >= 1)
		return true;

	if (tr.c.entityNum == ent->entityNumber)
		return true;

	//see if the owner is the ent.

	if (tr.c.entityNum > 0 && tr.c.entityNum < 8190) //skip worldspawn......
	{
		if (!gameLocal.entities[tr.c.entityNum]->IsType(idWorldspawn::Type))
		{
			if (gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetClipModel()->GetOwner() != NULL)
			{
				if (gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetClipModel()->GetOwner()->entityNumber == ent->entityNumber)
					return true;
			}
		}
	}

	return false;
}

idStr idHighlighter::ParseName(idEntity* ent)
{
	if (ent->IsType(idMoveableItem::Type))
	{
		if (static_cast<idMoveableItem*>(ent)->GetSparking())
		{
			return common->GetLanguageDict()->GetString("#str_def_gameplay_900097"); //"spark" BC 3-25-2025: loc fix
		}
	}

	return ent->displayName;
}

bool idHighlighter::IsHighlightActive()
{
	return (state != HLR_NONE);
}

void idHighlighter::DrawBars()
{	
	

	//Draw bars.
	for (int i = 0; i < 2; i++)
	{
		float bartopDrawY = 0;
		float barbottomDrawY = 0;		
	
		if (state == HLR_LERPON)
		{
			//float lerp = (gameLocal.hudTime - stateTimer) / (float)transitionTime;
			//lerp = idMath::ClampFloat(0, 1, lerp);
			//
			////bartopDrawY = idMath::PopLerp(-480, -(480 - barYPositions[0]) + 10, -(480 - barYPositions[0]), lerp);
			////barbottomDrawY = idMath::PopLerp(480, barYPositions[1] - 10, barYPositions[1], lerp);
			//
			//bartopDrawY = idMath::Lerp(-470, -(480 - barYPositions[0]), lerp);
			//barbottomDrawY = idMath::Lerp(470, barYPositions[1], lerp);

			bartopDrawY = -(480 - TOPBAR_DEFAULTY);
			barbottomDrawY = BOTTOMBAR_DEFAULTY;
		}
		else if (state == HLR_LERPOFF)
		{
			float lerp = (gameLocal.hudTime - stateTimer) / (float)LERPOFF_TIME;
			lerp = idMath::ClampFloat(0, 1, lerp);
	
			bartopDrawY = idMath::Lerp(-480, -(480 - barYPositions[0]), 1 - lerp);
			barbottomDrawY = idMath::Lerp(480, barYPositions[1], 1 - lerp);
		}
		else
		{
			if (letterboxLerpActive)
			{
				float letterboxLerp = (gameLocal.hudTime - letterboxTimer) / (float)LETTERBOX_LERPTIME;
				letterboxLerp = idMath::ClampFloat(0, 1, letterboxLerp);
				
				bartopDrawY = idMath::PopLerp(-(480 - TOPBAR_DEFAULTY), -(480 - barYPositions[0]) + 10, -(480 - barYPositions[0]), letterboxLerp);
				barbottomDrawY = idMath::PopLerp(BOTTOMBAR_DEFAULTY, barYPositions[1] - 10, barYPositions[1], letterboxLerp);				

				if (gameLocal.hudTime >= letterboxTimer + LETTERBOX_LERPTIME)
				{
					letterboxLerpActive = false;
				}
			}
			else
			{
				//fully activated paused state.
				bartopDrawY = -(480 - barYPositions[0]);
				barbottomDrawY = barYPositions[1];
			}			
		}
	
	
		//Top bar.
		renderSystem->DrawStretchPic(0, bartopDrawY, 640, 480, 0, 0, 1, 1, declManager->FindMaterial(spawnArgs.GetString("mtr_highlightbar_top")));
	
		//Bottom bar.
		renderSystem->DrawStretchPic(0, barbottomDrawY, 640, 480, 0, 0, 1, 1, declManager->FindMaterial(spawnArgs.GetString("mtr_highlightbar_btm")));
	}

	

	//Draw arrows and text.
	//gameLocal.GetLocalPlayer()->hud->DrawArbitraryText("Spark", .9f, colorWhite, 50, 50, "sofia", colorBlack);

	if (state != HLR_PAUSED)
		return;

	renderSystem->SetColor4(1, 1, 1, 1);
	for (int i = 0; i < MAX_ARROWS; i++)
	{
		if (arrowPositions[i] == vec2_zero)
			continue;

		//Draw arrow.
		idVec2 arrowPos = arrowPositions[i];
		renderSystem->DrawStretchPicRotated(arrowPos.x - 32, arrowPos.y - 8, 64, 16, 0, 0, 1, 1, declManager->FindMaterial( spawnArgs.GetString("mtr_highlightarrow1")), arrowAngles[i]);
		//renderSystem->DrawStretchPic(arrowPos.x - 4, arrowPos.y - 4, 8, 8, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/autoaimdot")); //debug for drawing where the specific XY point is.

		//Draw text.
		if (entNames[i].Length() > 0)
		{
			#define ARROWDRAWLENGTH 32
			idVec2 textPos = arrowPos;
			textPos.x -= cos(arrowAngles[i]) * -ARROWDRAWLENGTH;
			textPos.y += sin(arrowAngles[i]) * -ARROWDRAWLENGTH;
			int textAlignment = 0;
			if (arrowAngles[i] > idMath::PI / 2 && arrowAngles[i] < idMath::PI * 1.5f)
			{
				//right aligned text.
				textAlignment = idDeviceContext::ALIGN_RIGHT;
			}
			else
			{
				//left aligned text.
				textAlignment = idDeviceContext::ALIGN_LEFT;
			}

			textPos.y -= 17; //Nudge it up to make it look right.			
			gameLocal.GetLocalPlayer()->hud->DrawArbitraryText(entNames[i].c_str(), .9f, colorWhite, textPos.x, textPos.y, "inocua", colorBlack, textAlignment);
			//renderSystem->DrawStretchPic(textPos.x - 4, textPos.y - 4, 8, 8, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/autoaimdot")); //debug for drawing where the specific XY point is.
		}
	}
}

//This gets called if player presses ESC
void idHighlighter::DoSkip()
{
	state = HLR_LERPOFF;
	stateTimer = gameLocal.hudTime;
}
#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Mover.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "bc_ventpeek.h"

const idEventDef EV_SetVentpeekCanExit("setVentpeekCanExit", "d");

CLASS_DECLARATION(idStaticEntity, idVentpeek)
	EVENT(EV_PostSpawn, idVentpeek::Event_PostSpawn)
	EVENT(EV_StopPeek, idVentpeek::StopPeek)
	EVENT(EV_Activate, idVentpeek::Event_Activate)
	EVENT(EV_SetVentpeekCanExit, idVentpeek::Event_SetVentpeekCanExit)
END_CLASS

#define CAMERA_MOVETIME	150
#define FADETIME		50
#define FADEUPTIME		100

idVentpeek::idVentpeek(void)
{
}

idVentpeek::~idVentpeek(void)
{
}

void idVentpeek::Spawn(void)
{
	//GetPhysics()->SetContents(0);
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	isFrobbable = true;
	forVentDoor = false;
	forTelescope = false;
	peekState = PEEK_NONE;
	peekTimer = 0;
	ownerEnt = NULL;

	harc = spawnArgs.GetInt("yaw_arc");
	varc = spawnArgs.GetInt("pitch_arc");
	rotateScale = 1.0f;
	bidirectional = spawnArgs.GetBool( "bidirectional" );
	stopIfOpen = spawnArgs.GetBool( "stop_if_open" );
	lockListener = spawnArgs.GetBool("lockListener");

	turningSoundThreshold = spawnArgs.GetFloat("turning_sound_threshold");
	turningSoundDelay = spawnArgs.GetInt("turning_sound_delay");

	peekType = PEEKTYPE_NORMAL;
	
	canFrobExit = spawnArgs.GetBool("canFrobExit", "1");

	PostEventMS(&EV_PostSpawn, 0);	
}

void idVentpeek::Event_PostSpawn(void)
{
	idVec3 forward, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	if (!spawnArgs.GetBool("spawnpeek", "0"))
	{
		//sanity check
		if (targets.Num() <= 0)
		{
			gameLocal.Error("ventpeek '%s' has no target.", GetName());
			return;
		}

		peekEnt = targets[0].GetEntity();

		if (!peekEnt.IsValid())
		{
			gameLocal.Error("ventpeek '%s' has bad target.", GetName());
			return;
		}

		float offset;
		offset = spawnArgs.GetFloat("cam_offset");
		if (offset != 0)
		{
			peekEnt.GetEntity()->SetOrigin(peekEnt.GetEntity()->GetPhysics()->GetOrigin() + (forward * -offset));
		}
	}
	else
	{
		//Auto spawn a target null.
		idDict args;
		idEntity *nullEnt;

		args.Clear();
		args.Set("classname", "target_null");
		args.SetInt("angle", this->GetPhysics()->GetAxis().ToAngles().yaw + 180);
		gameLocal.SpawnEntityDef(args, &nullEnt);

		if (!nullEnt)
		{
			gameLocal.Error("ventpeek '%s' failed to spawn null ent.", this->GetName());
		}

		float offset = spawnArgs.GetFloat( "peek_offset", "-5.0" );
		nullEnt->SetOrigin(this->GetPhysics()->GetOrigin() + (forward * offset) + (up * 1.5f));
		peekEnt = nullEnt;
	}
	

	//For floor/ceiling setups.
	if (spawnArgs.GetBool("autodir"))
	{
		idVec3 myAngle;
		myAngle = GetPhysics()->GetAxis().ToAngles().ToForward();

		if (myAngle.z == -1)
		{
			//vent is on CEILING.
			peekType = PEEKTYPE_CEILING;
		}
		else if (myAngle.z == 1)
		{
			//vent is on FLOOR.
			peekType = PEEKTYPE_GROUND;
		}
	}
}

void idVentpeek::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( peekEnt ); // idEntityPtr<idEntity> peekEnt

	savefile->WriteBool( forVentDoor ); // bool forVentDoor
	savefile->WriteBool( forTelescope ); // bool forTelescope

	savefile->WriteObject( ownerEnt ); // idEntityPtr<idEntity> ownerEnt

	savefile->WriteInt( harc ); // int harc
	savefile->WriteInt( varc ); // int varc
	savefile->WriteFloat( rotateScale ); // float rotateScale

	savefile->WriteInt( peekTimer ); // int peekTimer

	savefile->WriteInt( peekState ); // int peekState

	savefile->WriteInt( peekType ); // int peekType
	savefile->WriteBool( bidirectional ); // bool bidirectional
	savefile->WriteBool( stopIfOpen ); // bool stopIfOpen

	savefile->WriteFloat( turningSoundThreshold ); // float turningSoundThreshold
	savefile->WriteInt( turningSoundDelay ); // int turningSoundDelay
	savefile->WriteBool( lockListener ); // bool lockListener

	savefile->WriteBool( canFrobExit ); // bool canFrobExit
}

void idVentpeek::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( peekEnt ); // idEntityPtr<idEntity> peekEnt

	savefile->ReadBool( forVentDoor ); // bool forVentDoor
	savefile->ReadBool( forTelescope ); // bool forTelescope

	savefile->ReadObject( ownerEnt ); // idEntityPtr<idEntity> ownerEnt

	savefile->ReadInt( harc ); // int harc
	savefile->ReadInt( varc ); // int varc
	savefile->ReadFloat( rotateScale ); // float rotateScale

	savefile->ReadInt( peekTimer ); // int peekTimer

	savefile->ReadInt( peekState ); // int peekState

	savefile->ReadInt( peekType ); // int peekType
	savefile->ReadBool( bidirectional ); // bool bidirectional
	savefile->ReadBool( stopIfOpen ); // bool stopIfOpen

	savefile->ReadFloat( turningSoundThreshold ); // float turningSoundThreshold
	savefile->ReadInt( turningSoundDelay ); // int turningSoundDelay
	savefile->ReadBool( lockListener ); // bool lockListener

	savefile->ReadBool( canFrobExit ); // bool canFrobExit
}

void idVentpeek::Think(void)
{
	idStaticEntity::Think();

	if ( peekState != PEEK_NONE && stopIfOpen && ownerEnt.GetEntity() && ownerEnt.GetEntity()->IsType( idDoor::Type ) )
	{
		idDoor* doorEnt = static_cast<idDoor*>(ownerEnt.GetEntity());
		if ( doorEnt->IsOpen() )
			StopPeek();
	}

	if (peekState == PEEK_MOVECAMERATOWARD)
	{
		if (gameLocal.time >= peekTimer + CAMERA_MOVETIME - FADETIME)
		{
			peekState = PEEK_FADEOUT;
			peekTimer = gameLocal.time;
			
			//fade to black.
			gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 1, FADETIME);
		}
	}
	else if (peekState == PEEK_FADEOUT)
	{
		if (gameLocal.time >= peekTimer + FADETIME)
		{
			//fade from color.
			gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, FADEUPTIME);
			peekState = PEEK_PEEKING;

			//Enter peek state.
			gameLocal.GetLocalPlayer()->peekObject = this;

			if (this->spawnArgs.GetBool("use_targetangle", "0") && peekEnt.IsValid())
			{
				//Use the yaw of the target ent.
				float targetYaw = peekEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles().yaw;
				gameLocal.GetLocalPlayer()->SetViewYawLerp(targetYaw, 1);
			}
		}
	}
}

void idVentpeek::StopPeek(bool fast)
{
	float angDelta;

	if (!fast)
	{
		gameLocal.GetLocalPlayer()->SetViewFade(0, 0, 0, 0.0f, 300);
	}
	
	BecomeInactive(TH_THINK);
	peekState = PEEK_NONE;

	gameLocal.GetLocalPlayer()->peekObject = NULL;

	if ( !forVentDoor ) {
		isFrobbable = true;
	}

	if (!fast)
	{
		gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, 1); //snap the pitch lerp back to straight forward

		angDelta = (this->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).ToYaw();
		gameLocal.GetLocalPlayer()->SetViewYawLerp(angDelta, 1);
	}
	

	StartSound("snd_activate", SND_CHANNEL_ANY, 0, false, NULL);
}

void idVentpeek::GetAngleRestrictions(float &yaw_min, float &yaw_max, float &pitch_min, float &pitch_max)
{

	if (peekType == PEEKTYPE_NORMAL)
	{
		idMat3		axis;
		idAngles	angs;

		axis = peekEnt.GetEntity()->GetPhysics()->GetAxis();
		angs = axis.ToAngles();

		yaw_min = angs.yaw - (float)harc;
		yaw_min = idMath::AngleNormalize180(yaw_min);

		yaw_max = angs.yaw + (float)harc;
		yaw_max = idMath::AngleNormalize180(yaw_max);

		pitch_min = max(angs.pitch - (float)varc, -89.0f);
		pitch_max = min(angs.pitch + (float)varc, 89.0f);

	}
	else
	{
		yaw_min = 0;
		yaw_max = 0;

		if (peekType == PEEKTYPE_CEILING)
		{
			//Ceiling.
			pitch_min = -89;
			pitch_max = 0;
		}
		else
		{
			//Ground.
			pitch_min = 0;
			pitch_max = 89;
		}
	}
}

float idVentpeek::GetRotateScale(void)
{
	return rotateScale;
}


bool idVentpeek::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	idVec3 forward, up;	

	if ( !forVentDoor ) {
		isFrobbable = false;
	}

	//make camera move toward the crack.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &up, NULL);

	// For bidirectional vent peeks, we need to adjust the rotation/peek ent to face correct direction
	if ( bidirectional && peekEnt.IsValid() )
	{
		idVec3 playerForward;
		gameLocal.GetLocalPlayer()->viewAngles.ToVectors( &playerForward );
		if ( DotProduct( forward, playerForward ) > 0.0f )
		{
			idAngles angles = this->GetPhysics()->GetAxis().ToAngles();
			angles.yaw += 180.0f;
			angles.ToVectors( &forward, &up );
			GetPhysics()->SetAxis( angles.ToMat3() );
			angles.yaw += 180.0f;
			peekEnt.GetEntity()->SetAngles( angles );
			float offset = spawnArgs.GetFloat( "peek_offset", "-5.0" );
			peekEnt.GetEntity()->SetOrigin( this->GetPhysics()->GetOrigin() + ( forward * offset ) + ( up * 1.5f ) );
		}
	}

	gameLocal.GetLocalPlayer()->SetViewposAbsLerp(GetPhysics()->GetOrigin() + (forward * 2) + idVec3(0,0,1), CAMERA_MOVETIME);

	if (peekType == PEEKTYPE_NORMAL)
	{
		//Normal ventpeek on wall.
		gameLocal.GetLocalPlayer()->SetViewPitchLerp(0, CAMERA_MOVETIME);
		gameLocal.GetLocalPlayer()->SetViewYawLerp(GetPhysics()->GetAxis().ToAngles().yaw + 180, CAMERA_MOVETIME);
	}		

	peekState = PEEK_MOVECAMERATOWARD;
	peekTimer = gameLocal.time;
	BecomeActive(TH_THINK);

	StartSound("snd_activate", SND_CHANNEL_ANY, 0, false, NULL);

	// Ventpeeks can call scripts on frob (for vignette purposes)
	const char* callName = spawnArgs.GetString("call", "");
	if (*callName != '\0')
	{
		const function_t* scriptFunction;
		scriptFunction = gameLocal.program.FindFunction(callName);

		if (!scriptFunction)
		{
			gameLocal.Warning("idVentpeek: Could not find script function '%s'", callName);
		}
		else
		{
			// Execute immediately
			idThread* thread;
			thread = new idThread(scriptFunction);
			thread->DelayedStart(0);
		}
	}

	return true;
}

float idVentpeek::GetTurningSoundThreshold(void)
{
	return turningSoundThreshold;
}
int idVentpeek::GetTurningSoundDelay(void)
{
	return turningSoundDelay;
}

bool idVentpeek::GetLockListener(void)
{
	return lockListener;
}

bool idVentpeek::Event_Activate(idEntity *activator)
{
	return DoFrob(0, activator);	
}

bool idVentpeek::CanFrobExit(void)
{
	return canFrobExit;
}

void idVentpeek::Event_SetVentpeekCanExit(bool toggle)
{
	canFrobExit = toggle;
}




// =================================== VENTPEEK TELESCOPE ===================================




CLASS_DECLARATION(idVentpeek, idVentpeekTelescope)
	EVENT(EV_SetPanAndZoom, idVentpeekTelescope::SetPanAndZoom)
	EVENT(EV_LockOn, idVentpeekTelescope::LockOn)
	EVENT(EV_CrackLens, idVentpeekTelescope::CrackLens)
	EVENT(EV_ForceSetTarget, idVentpeekTelescope::ForceSetTarget)
	EVENT(EV_SetVentpeekFOVLerp, idVentpeekTelescope::SetVentpeekFOVLerp)
	EVENT(EV_StartFastForward, idVentpeekTelescope::StartFastForward)
	EVENT(EV_StopFastForward, idVentpeekTelescope::StopFastForward)
END_CLASS

void idVentpeekTelescope::Think(void)
{
	idVentpeek::Think();

	HandleFastForward();

	HandleZoomTransitions();

	HandleLookTargets();

	HandleCamera();
}

bool idVentpeekTelescope::DoFrob(int index, idEntity* frobber)
{
	GetParametersFromTarget(peekEnt);
	currentFov = maxFov; // Start at the absolute lowest zoom level

	return idVentpeek::DoFrob(index, frobber);
}

// Debugging function. Force the telescope to look through a particular target ent, whether or not it's part of the sequence.
void idVentpeekTelescope::ForceSetTarget(idEntity* ent)
{
	peekEnt = ent;
	GetParametersFromTarget(peekEnt);
	currentFov = maxFov;

	// Make sure the player's view starts off pointing in the direction of the peek ent
	idAngles angles = peekEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
	gameLocal.GetLocalPlayer()->SetViewAngles(angles);
	gameLocal.GetLocalPlayer()->SetViewLerpAngles(angles, ZOOM_TRANSITION_MAXTIME / 4);
}

void idVentpeekTelescope::HandleFastForward(void)
{
	if (isFastForward)
	{
		fastForwardWait = max(fastForwardWait - (gameLocal.time - gameLocal.previousTime), 0);
		if (fastForwardWait == 0)
		{
			switch (zoomTransitionState)
			{
				case (ZOOMIN_BLURIN):
				{
					zoomTransitionTimer = zoomTransitionTimerStart = (ZOOM_TRANSITION_MAXTIME / 2) * (1.0f - 0.1f * fastForwardIteration);
					fastForwardWait = (ZOOM_TRANSITION_MAXTIME / 2) * (1.0f - 0.15f * fastForwardIteration); // Reduce the amount of time until we next auto-advance, to simulate Nina getting more impatient
					MoveToNextTarget();
					zoomTransitionState = ZOOMIN_BLUROUT;
					break;
				}
				case (ZOOMIN_BLUROUT):
				case (ZOOM_NONE):
				{
					fastForwardIteration = min(fastForwardIteration + 1, MAX_FASTFORWARD_SPEEDUPS);
					this->AutoAdvance();
					break;
				}
			}
		}
	}
}

void idVentpeekTelescope::HandleZoomTransitions(void)
{
	if (zoomTransitionState != ZOOM_NONE)
	{
		zoomTransitionTimer = max(zoomTransitionTimer - (gameLocal.time - gameLocal.previousTime), 0);
		if (zoomTransitionTimer == 0)
		{
			// We're ready to change state, but to what? And what's the consequences?
			switch (zoomTransitionState)
			{
			case (ZOOMIN_BLURIN):
			{
				zoomTransitionTimer = zoomTransitionTimerStart = ZOOM_TRANSITION_MAXTIME / 2;
				MoveToNextTarget();
				zoomTransitionState = ZOOMIN_BLUROUT;
				break;
			}
			case (ZOOMIN_BLUROUT):
			{
				zoomTransitionState = ZOOM_NONE;
				break;
			}
			case (ZOOMOUT_BLURIN):
			{
				zoomTransitionTimer = zoomTransitionTimerStart = ZOOM_TRANSITION_MAXTIME / 2;
				MoveToPreviousTarget();
				zoomTransitionState = ZOOMOUT_BLUROUT;
				break;
			}
			case (ZOOMOUT_BLUROUT):
			{
				zoomTransitionState = ZOOM_NONE;
				break;
			}
			default:
				return;
			}
		}
	}
}

void idVentpeekTelescope::HandleLookTargets(void)
{
	// Start by updating which things the player can see, and how long they've seen them
	for (int i = 0; i < lookTargets.Num(); i++)
	{
		if (!lookTargets[i].seen)
		{
			if (this->CanSee(lookTargets[i].entity))
			{
				lookTargets[i].seenTime += gameLocal.time - gameLocal.previousTime; // Increase the amount of time this thing has been visible for.
				if (lookTargets[i].seenTime >= g_teleSeenTime.GetInteger())
				{
					// We've now definitely 'seen' this thing. Mark it as such, and call its associated 'seen' function (if applicable)
					lookTargets[i].seen = true;
					if (lookTargets[i].seenFunction != NULL)
					{
						idThread* thread;
						thread = new idThread(lookTargets[i].seenFunction);
						thread->DelayedStart(0);
					}
				}
				if (developer.GetInteger())
					gameRenderWorld->DebugBounds(idVec4(1, 0, 0, 1), lookTargets[i].entity->GetPhysics()->GetBounds(), lookTargets[i].entity->GetPhysics()->GetOrigin());
			}
			else
			{
				lookTargets[i].seenTime = 0; // We are still looking for this thing, but it is not currently in view.
			}
		}
	}
	
	// Try to focus on our current target first, if applicable. This should make it a little more 'sticky',
	// and prevent us from flipping back and forth between focus targets if they accidentally overlap for whatever reason.
	if (currentFocusTarget != NULL)
	{
		if (this->IsFocusedOn(currentFocusTarget->entity))
		{
			// Record that the player has continued to look at the focus target.
			focusTime += gameLocal.time - gameLocal.previousTime;

			if (developer.GetInteger())
				gameRenderWorld->DebugBounds(idVec4(0, 1, 0, 1), currentFocusTarget->entity->GetPhysics()->GetBounds(), currentFocusTarget->entity->GetPhysics()->GetOrigin());
		}
		else
		{
			// The player has broken focus -- reset our records
			focusTime = 0;
			currentFocusTarget = NULL;
		}
	}
	if (currentFocusTarget == NULL)
	{
		// Try to acquire a new focus target from the list of things we've seen
		for (int i = 0; i < lookTargets.Num(); i++)
		{
			if (lookTargets[i].seen && this->IsFocusedOn(lookTargets[i].entity))
			{
				// New focus target acquired.
				currentFocusTarget = &lookTargets[i];
				focusTime = 0;
				break;
			}
		}
	}
	

	// See if we've focused enough on our target to trigger its associated focus function
	if (currentFocusTarget != NULL && focusTime > g_teleFocusTime.GetInteger() && currentFocusTarget->focusFunction != NULL)
	{
		idThread* thread;
		thread = new idThread(currentFocusTarget->focusFunction);
		thread->DelayedStart(0);

		// We can now remove this entry from our look targets entirely
		focusTime = 0;
		lookTargets.Remove(*currentFocusTarget);
		currentFocusTarget = NULL;
	}
}

// Returns a frustrum described by four planes representing the four sides of the viewport,
// rotating clockwise from the player's perspective starting at 12:00. Like this:
//
//  ---------
//  |   0   |
//  |3     1|
//  |   2   |
//  ---------
//
// Each plane's normal points 'inward'
void idVentpeekTelescope::BuildViewFrustum(idPlane(&viewFrustum)[4])
{
	idVec3 eyeballPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;

	// What direction is the player looking, and what is their FOV?
	float fovX, fovY;
	gameLocal.CalcFov(gameLocal.GetLocalPlayer()->CalcFov(true), fovX, fovY);

	idVec3 up, forward, right;
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(&forward, &right, &up);

	// Top
	viewFrustum[0] = idPlane(-up, 0);
	viewFrustum[0].FitThroughPoint(eyeballPos);
	viewFrustum[0].RotateSelf(eyeballPos, idRotation(eyeballPos, right, -fovY / 2).ToMat3());

	// Right
	viewFrustum[1] = idPlane(-right, 0);
	viewFrustum[1].FitThroughPoint(eyeballPos);
	viewFrustum[1].RotateSelf(eyeballPos, idRotation(eyeballPos, up, fovY / 2).ToMat3());

	// Bottom
	viewFrustum[2] = idPlane(up, 0);
	viewFrustum[2].FitThroughPoint(eyeballPos);
	viewFrustum[2].RotateSelf(eyeballPos, idRotation(eyeballPos, right, fovY / 2).ToMat3());

	// Left
	viewFrustum[3] = idPlane(right, 0);
	viewFrustum[3].FitThroughPoint(eyeballPos);
	viewFrustum[3].RotateSelf(eyeballPos, idRotation(eyeballPos, up, -fovY / 2).ToMat3());
}

// Loose 'can the player currently see this thing?' check.
// Contrasts with the tighter requirements of IsFocusedOn,
// and can also be used as a preliminary check for it.
bool idVentpeekTelescope::CanSee(const idEntity* target)
{
	// Pretend not to see it if marked dormant
	if (target->spawnArgs.GetBool("dormant", "0"))
		return false;

	idVec3 eyeballPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;

	// Sanity check if we are in the same PVS
	int eyeballArea = gameLocal.pvs.GetPVSArea(eyeballPos);
	if (eyeballArea == -1)
		return false; // In a solid

	pvsHandle_t pvs = gameLocal.pvs.SetupCurrentPVS(eyeballArea);

	if (!gameLocal.pvs.InCurrentPVS(pvs, target->GetPhysics()->GetOrigin()))
	{
		gameLocal.pvs.FreeCurrentPVS(pvs);
		return false;
	}
	gameLocal.pvs.FreeCurrentPVS(pvs);

	// Okay, let's build a view frustum and see if the target is inside it.
	idPlane viewFrustum[4];
	BuildViewFrustum(viewFrustum);

	// For this loose check, our definition of 'inside' is fairly generous.
	// Broadly speaking, we only need the center of the bounding box to be inside the frustum.
	// It's okay for the box itself to be partially poking outside.
	idBounds targetBounds = target->GetPhysics()->GetBounds();
	targetBounds.TranslateSelf(target->GetPhysics()->GetOrigin());
	idVec3 center = targetBounds.GetCenter();

	bool inside = true;
	for (int i = 0; i < 4; i++)
	{
		if (viewFrustum[i].Side(center) != PLANESIDE_FRONT)
		{
			inside = false;
			break;
		}
	}
	return inside;
}

// SW: Returns true if the player is focused on an entity while looking through a telescope.
// Our definition of 'focus' is that the entity is in the center of the view (inside a box dictated by the error margin)
bool idVentpeekTelescope::IsFocusedOn(const idEntity* target)
{
	float errorMargin = g_teleFocusError.GetFloat();
	idVec3 eyeballPos = gameLocal.GetLocalPlayer()->firstPersonViewOrigin;

	// Check first if we can see the target at all
	if (!CanSee(target))
		return false;

	float fovX, fovY;
	gameLocal.CalcFov(gameLocal.GetLocalPlayer()->CalcFov(true), fovX, fovY);

	idVec3 up, forward, right;
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(&forward, &right, &up);

	idPlane viewFrustum[4];
	BuildViewFrustum(viewFrustum);

	// Build a shrunken frustum which is scaled to a fraction of the player's FOV
	viewFrustum[0].RotateSelf(eyeballPos, idRotation(eyeballPos, right, (fovY / 2.0f) * (1.0f - errorMargin)).ToMat3());
	viewFrustum[1].RotateSelf(eyeballPos, idRotation(eyeballPos, up, (fovY / 2.0f) * -(1.0f - errorMargin)).ToMat3());
	viewFrustum[2].RotateSelf(eyeballPos, idRotation(eyeballPos, right, (fovY / 2.0f) * -(1.0f - errorMargin)).ToMat3());
	viewFrustum[3].RotateSelf(eyeballPos, idRotation(eyeballPos, up, (fovY / 2.0f) * (1.0f - errorMargin)).ToMat3());

	idBounds targetBounds = target->GetPhysics()->GetBounds();
	targetBounds.TranslateSelf(target->GetPhysics()->GetOrigin());
	idVec3 center = targetBounds.GetCenter();

	bool inside = true;
	for (int i = 0; i < 4; i++)
	{
		if (viewFrustum[i].Side(center) != PLANESIDE_FRONT)
		{
			inside = false;
			break;
		}
	}

	return inside;
}

// Returns a value from 0.0 to 1.0 to indicate how much to blur the current view (based on the current zoom transition state)
// This should probably be a nice ease at some point instead of a linear transition
float idVentpeekTelescope::GetCurrentBlur()
{
	if (zoomTransitionState == ZOOMIN_BLURIN || zoomTransitionState == ZOOMOUT_BLURIN)
	{
		return 1.0f - ((float)zoomTransitionTimer / (float)zoomTransitionTimerStart);
	}
	else if (zoomTransitionState == ZOOMIN_BLUROUT || zoomTransitionState == ZOOMOUT_BLUROUT)
	{
		return (float)zoomTransitionTimer / (float)zoomTransitionTimerStart;
	}
	else
		return 0.0f;
}

void idVentpeekTelescope::Event_PostSpawn(void)
{
	forTelescope = true;
	zoomTransitionState = ZOOM_NONE;
	zoomTransitionTimer = 0;
	zoomTransitionTimerStart = 0;

	cameraTimer = 0;
	cameraState = CAMERA_IDLE;
	canUseCamera = false;
	photoCount = 0;

	lookTargets = idList<idLookTarget>();
	currentFocusTarget = NULL;
	focusTime = 0;

	canPanAndZoom = true;
	isLockedOn = false;
	isCracked = false;

	fovLerpStart = 0;
	fovLerpEnd = 0;
	fovLerpStartTime = 0;
	fovLerpEndTime = 0;
	fovIsLerping = false;

	isFastForward = false;
	fastForwardIteration = 0;
	fastForwardWait = 0;

	lockOnFunction = NULL; // SW 2nd April 2025: fixes save/load crash in observatory

	//sanity check
	if (targets.Num() <= 0)
	{
		gameLocal.Error("ventpeek '%s' has no target.", GetName());
		return;
	}

	peekEnt = targets[0].GetEntity();

	if (!peekEnt.IsValid())
	{
		gameLocal.Error("ventpeek '%s' has bad target.", GetName());
		return;
	}
}

void idVentpeekTelescope::Save(idSaveGame* savefile) const
{
	savefile->WriteFloat( maxFov ); // float maxFov
	savefile->WriteFloat( minFov ); // float minFov
	savefile->WriteFloat( fovStep ); // float fovStep
	savefile->WriteFloat( currentFov ); // float currentFov

	savefile->WriteFloat( fovLerpStart ); // float fovLerpStart
	savefile->WriteFloat( fovLerpEnd ); // float fovLerpEnd
	savefile->WriteFloat( fovLerpStartTime ); // float fovLerpStartTime
	savefile->WriteFloat( fovLerpEndTime ); // float fovLerpEndTime
	savefile->WriteInt( fovLerpType ); // int fovLerpType

	savefile->WriteBool( fovIsLerping ); // bool fovIsLerping

	savefile->WriteBool( canUseCamera ); // bool canUseCamera
	savefile->WriteInt( cameraTimer ); // int cameraTimer
	savefile->WriteInt( cameraState ); // int cameraState

	savefile->WriteInt( photoCount ); // int photoCount

	savefile->WriteBool( canPanAndZoom ); // bool canPanAndZoom
	savefile->WriteBool( isLockedOn ); // bool isLockedOn
	savefile->WriteFunction( lockOnFunction ); // const function_t* lockOnFunction

	savefile->WriteObject( nextTarget ); // idEntityPtr<idEntity> nextTarget
	savefile->WriteObject( previousTarget ); // idEntityPtr<idEntity> previousTarget

	savefile->WriteInt( zoomTransitionState ); // int zoomTransitionState
	savefile->WriteInt( zoomTransitionTimer ); // int zoomTransitionTimer
	savefile->WriteInt( zoomTransitionTimerStart ); // int zoomTransitionTimerStart


	savefile->WriteInt( lookTargets.Num() ); // idList<idLookTarget> lookTargets

	for (int idx = 0; idx < lookTargets.Num(); idx++)
	{
		savefile->WriteObject( lookTargets[idx].entity ); // const idEntity* entity
		savefile->WriteFunction( lookTargets[idx].focusFunction ); // const function_t* focusFunction
		savefile->WriteFunction( lookTargets[idx].seenFunction ); // const function_t* seenFunction
		savefile->WriteInt( lookTargets[idx].seenTime ); // int seenTime
		savefile->WriteBool( lookTargets[idx].seen ); // bool seen
	}

	int foundIdx = -1; // idLookTarget* currentFocusTarget
	for (int idx = 0; idx < lookTargets.Num(); idx++)
	{
		if (currentFocusTarget == &lookTargets[idx])
		{
			foundIdx = idx;
			break;
		}
	}
	savefile->WriteInt(foundIdx);

	savefile->WriteInt( focusTime ); // int focusTime

	savefile->WriteBool( isCracked ); // bool isCracked
	savefile->WriteBool( isFastForward ); // bool isFastForward
	savefile->WriteInt( fastForwardIteration ); // int fastForwardIteration
	savefile->WriteInt( fastForwardWait ); // int fastForwardWait
}

void idVentpeekTelescope::Restore(idRestoreGame* savefile)
{
	savefile->ReadFloat( maxFov ); // float maxFov
	savefile->ReadFloat( minFov ); // float minFov
	savefile->ReadFloat( fovStep ); // float fovStep
	savefile->ReadFloat( currentFov ); // float currentFov

	savefile->ReadFloat( fovLerpStart ); // float fovLerpStart
	savefile->ReadFloat( fovLerpEnd ); // float fovLerpEnd
	savefile->ReadFloat( fovLerpStartTime ); // float fovLerpStartTime
	savefile->ReadFloat( fovLerpEndTime ); // float fovLerpEndTime
	savefile->ReadInt( fovLerpType ); // int fovLerpType

	savefile->ReadBool( fovIsLerping ); // bool fovIsLerping

	savefile->ReadBool( canUseCamera ); // bool canUseCamera
	savefile->ReadInt( cameraTimer ); // int cameraTimer
	savefile->ReadInt( cameraState ); // int cameraState

	savefile->ReadInt( photoCount ); // int photoCount

	savefile->ReadBool( canPanAndZoom ); // bool canPanAndZoom
	savefile->ReadBool( isLockedOn ); // bool isLockedOn
	savefile->ReadFunction( lockOnFunction ); // const function_t* lockOnFunction

	savefile->ReadObject( nextTarget ); // idEntityPtr<idEntity> nextTarget
	savefile->ReadObject( previousTarget ); // idEntityPtr<idEntity> previousTarget

	savefile->ReadInt( zoomTransitionState ); // int zoomTransitionState
	savefile->ReadInt( zoomTransitionTimer ); // int zoomTransitionTimer
	savefile->ReadInt( zoomTransitionTimerStart ); // int zoomTransitionTimerStart

	int num;
	savefile->ReadInt( num ); // idList<idLookTarget> lookTargets
	lookTargets.SetNum( num );
	for (int idx = 0; idx < num; idx++)
	{
		idClass* obj;
		savefile->ReadObject( obj ); // const idEntity* entity
		lookTargets[idx].entity = (idEntity*)obj;
		savefile->ReadFunction( lookTargets[idx].focusFunction ); // const function_t* focusFunction
		savefile->ReadFunction( lookTargets[idx].seenFunction ); // const function_t* seenFunction
		savefile->ReadInt( lookTargets[idx].seenTime ); // int seenTime
		savefile->ReadBool( lookTargets[idx].seen ); // bool seen
	}

	int foundIdx;
	savefile->ReadInt(foundIdx); // idLookTarget* currentFocusTarget

	if (foundIdx >= 0)
	{
		currentFocusTarget = &lookTargets[foundIdx];
	}
	else
	{
		currentFocusTarget = nullptr;
	}

	savefile->ReadInt( focusTime ); // int focusTime

	savefile->ReadBool( isCracked ); // bool isCracked
	savefile->ReadBool( isFastForward ); // bool isFastForward
	savefile->ReadInt( fastForwardIteration ); // int fastForwardIteration
	savefile->ReadInt( fastForwardWait ); // int fastForwardWait
}

bool idVentpeekTelescope::IsLensCracked()
{
	return isCracked;
}

void idVentpeekTelescope::CrackLens()
{
	isCracked = true;
	// Play the sound. The overlay is handled in the player view code alongside the rest of the aperture.
	StartSound("snd_lenscrack", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
}

void idVentpeekTelescope::SetPanAndZoom(bool b)
{
	canPanAndZoom = b;
}

bool idVentpeekTelescope::CanPanAndZoom(void)
{
	return canPanAndZoom;
}

// Disable panning and zooming, lerp to point at the target, and change the HUD
void idVentpeekTelescope::LockOn(idEntity* entity)
{
	SetPanAndZoom(false);

	idPhysics* phys = entity->GetPhysics();
	idVec3 center = phys->GetBounds().Translate(phys->GetOrigin()).GetCenter();
	idAngles angleToTarget = (center - peekEnt.GetEntity()->GetPhysics()->GetOrigin()).ToAngles().Normalize180();

	gameLocal.GetLocalPlayer()->SetViewLerpAngles(angleToTarget, LOCKON_LERP_TIME);

	gameLocal.GetLocalPlayer()->hud->SetStateBool("takefinalphoto", true);

	isLockedOn = true;
}

void idVentpeekTelescope::SetVentpeekFOVLerp(float endFov, int timeMS, int lerpType)
{
	fovLerpStart = currentFov;
	currentFov = fovLerpEnd;
	fovLerpEnd = endFov;
	fovLerpStartTime = gameLocal.time;
	fovLerpEndTime = gameLocal.time + timeMS;
	fovLerpType = lerpType;
	fovIsLerping = true;
}

bool idVentpeekTelescope::HasNextTarget(void)
{
	return nextTarget.IsValid() && nextTarget.GetEntityNum() != peekEnt.GetEntityNum();
}

void idVentpeekTelescope::MoveToNextTarget(void)
{
	if (this->HasNextTarget())
	{
		peekEnt = nextTarget;
		GetParametersFromTarget(peekEnt);
		currentFov = maxFov;

		// Make sure the player's view starts off pointing in the direction of the next peek ent
		idAngles angles = peekEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles();
		gameLocal.GetLocalPlayer()->SetViewAngles(angles);
		gameLocal.GetLocalPlayer()->SetViewLerpAngles(angles, ZOOM_TRANSITION_MAXTIME / 4);
	}
}

bool idVentpeekTelescope::HasPreviousTarget(void)
{
	return previousTarget.IsValid() && previousTarget.GetEntityNum() != peekEnt.GetEntityNum();
}

void idVentpeekTelescope::MoveToPreviousTarget(void)
{
	if (this->HasPreviousTarget())
	{
		peekEnt = previousTarget;
		GetParametersFromTarget(peekEnt);
		currentFov = maxFov;
		gameLocal.GetLocalPlayer()->SetViewAngles(peekEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles());
	}
}

// Each target in the sequence has its own parameters for how we can pan and zoom,
// as well as possible things the player can look at (or focus on) to trigger functionality.
// We update these when we change targets.
void idVentpeekTelescope::GetParametersFromTarget(idEntityPtr<idEntity> target)
{
	idEntity* targetEnt = target.GetEntity();
	harc = targetEnt->spawnArgs.GetInt("yaw_arc");
	varc = targetEnt->spawnArgs.GetInt("pitch_arc");
	rotateScale = targetEnt->spawnArgs.GetFloat("rotate_scale");
	if (rotateScale == 0)
		rotateScale = 1.0f;

	maxFov = targetEnt->spawnArgs.GetFloat("max_fov");
	minFov = targetEnt->spawnArgs.GetFloat("min_fov");
	fovStep = targetEnt->spawnArgs.GetFloat("fov_step");
	if (fovStep == 0)
		fovStep = DEFAULT_FOV_STEP;

	canUseCamera = targetEnt->spawnArgs.GetBool("use_camera", "0");

	const char* nextName;
	const char* previousName;

	nextName = targetEnt->spawnArgs.GetString("target_next", "");
	if (*nextName != '\0')
	{
		nextTarget = gameLocal.FindEntity(nextName);
	}
	else
	{
		nextTarget = targetEnt;
	}

	previousName = targetEnt->spawnArgs.GetString("target_previous", "");
	if (*previousName != '\0')
	{
		previousTarget = gameLocal.FindEntity(previousName);
	}
	else
	{
		previousTarget = targetEnt;
	}

	// Acquire look targets, if there are any.
	// We look for keyvalues starting with 'look_target', 'look_call' and 'focus_call' with corresponding numbering.
	// If the target exists, and it has a look call OR a focus call, we add it to the list
	lookTargets.Clear();
	currentFocusTarget = NULL;
	for (int i = 1; i <= MAX_LOOK_TARGETS; i++)
	{
		const char* lookTargetName = targetEnt->spawnArgs.GetString(va("look_target%d", i), "");
		const char* lookCall = targetEnt->spawnArgs.GetString(va("look_call%d", i), "");
		const char* focusCall = targetEnt->spawnArgs.GetString(va("focus_call%d", i), "");

		if (*lookTargetName == '\0') // This entry probably doesn't exist, skip it
			continue;

		const idEntity* lookTarget = gameLocal.FindEntity(lookTargetName);

		if (!lookTarget)
		{
			gameLocal.Warning("idVentpeekTelescope: Could not find look target '%s'", lookTargetName);
			continue; // Invalid or missing target entity
		}

		const function_t* lookFunction = NULL;
		if (*lookCall != '\0')
		{
			lookFunction = gameLocal.program.FindFunction(lookCall);
			if (!lookFunction)
			{
				gameLocal.Warning("idVentpeekTelescope: Could not find look function '%s'", lookCall);
				continue; // Invalid or missing function
			}
		}

		const function_t* focusFunction = NULL;
		if (*focusCall != '\0')
		{
			focusFunction = gameLocal.program.FindFunction(focusCall);
			if (!focusFunction)
			{
				gameLocal.Warning("idVentpeekTelescope: Could not find focus function '%s'", focusCall);
				continue; // Invalid or missing function
			}
		}

		// If we've made it this far, we have a valid target with a valid look function OR a valid focus function. Append them to the list.
		lookTargets.Append(idLookTarget(lookTarget, focusFunction, lookFunction));
	}

	// Get the function to call when we take our lock-on picture (if available)
	const char* lockonFunctionName = targetEnt->spawnArgs.GetString("lockon_call", "");

	if (*lockonFunctionName != '\0')
	{
		const function_t* scriptFunction;
		scriptFunction = gameLocal.program.FindFunction(lockonFunctionName);

		if (!scriptFunction)
		{
			gameLocal.Warning("idVentpeekTelescope: Could not find script function '%s'", lockonFunctionName);
		}
		else
		{
			lockOnFunction = scriptFunction;
		}
	}
	else
	{
		lockOnFunction = NULL;
	}

	// Get the function to call immediately upon switching to this target (if available)
	const char* callName = targetEnt->spawnArgs.GetString("call", "");
	if (*callName != '\0')
	{
		const function_t* scriptFunction;
		scriptFunction = gameLocal.program.FindFunction(callName);

		if (!scriptFunction)
		{
			gameLocal.Warning("idVentpeekTelescope: Could not find script function '%s'", callName);
		}
		else
		{
			// Execute immediately
			idThread* thread;
			thread = new idThread(scriptFunction);
			thread->DelayedStart(0);
		}
	}
}

// Try to zoom in (either within the current scope, or by transitioning to the next scope)
void idVentpeekTelescope::ZoomIn()
{
	// Don't let the player change zoom if we're in the middle of a transition (or if we're locked)
	if (zoomTransitionState == ZOOM_NONE && canPanAndZoom)
	{
		if (currentFov > minFov)
		{
			// We can still zoom in within this scene, so do that
			StartSound("snd_zoomin", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
			currentFov = max(currentFov - fovStep, minFov);
		}
		else if (this->HasNextTarget())
		{
			// We're already zoomed all the way in, but we can transition to our next target. So do that.
			StartSound("snd_zoomtransition", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);

			zoomTransitionState = ZOOMIN_BLURIN;
			zoomTransitionTimer = zoomTransitionTimerStart = ZOOM_TRANSITION_MAXTIME / 2;

			// Do a little lerp to face forward 
			gameLocal.GetLocalPlayer()->SetViewLerpAngles(peekEnt.GetEntity()->GetPhysics()->GetAxis().ToAngles(), zoomTransitionTimerStart);
		}
		// Else: we're zoomed all the way in and there's nowhere to go. Do nothing.
	}
}

void idVentpeekTelescope::AutoAdvance()
{
	if (this->HasNextTarget())
	{
		// Gradually escalate the zoom transition sound in pitch (is there really no better way to manipulate sound shaders?)
		switch (fastForwardIteration)
		{
			case 0:
			{
				StartSound("snd_zoomtransition", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}
			case 1:
			{
				StartSound("snd_zoomtransition1", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}
			case 2:
			{
				StartSound("snd_zoomtransition2", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}
			case 3:
			{
				StartSound("snd_zoomtransition3", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}
			case 4:
			{
				StartSound("snd_zoomtransition4", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}
			default:
			{
				StartSound("snd_zoomtransition5", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
				break;
			}

		}
		

		zoomTransitionState = ZOOMIN_BLURIN;
		zoomTransitionTimer = zoomTransitionTimerStart = (ZOOM_TRANSITION_MAXTIME / 2) * (1.0f - 0.1f * fastForwardIteration); 
		fastForwardWait = (ZOOM_TRANSITION_MAXTIME / 2) * (1.0f - 0.15f * fastForwardIteration); // speed up to simulate nina getting more impatient with each iteration (up to a cap)
		
	}
}

// Try to zoom out (either within the current scope, or by transitioning to the previous scope)
void idVentpeekTelescope::ZoomOut()
{
	// Don't let the player change zoom if we're in the middle of a transition (or if we're locked
	if (zoomTransitionState == ZOOM_NONE && canPanAndZoom)
	{
		if (currentFov < maxFov)
		{
			// We can still zoom out in this scene, so do that
			StartSound("snd_zoomout", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
			currentFov = min(currentFov + fovStep, maxFov);
		}
		else if (this->HasPreviousTarget())
		{
			// We're already zoomed all the way out, but we can transition to our previous target. So do that.
			StartSound("snd_zoomtransition", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);

			zoomTransitionState = ZOOMOUT_BLURIN;
			zoomTransitionTimer = zoomTransitionTimerStart = ZOOM_TRANSITION_MAXTIME / 2;
		}
		// Else: we're zoomed all the way out and there's nowhere to go. Do nothing.
	}
}

// Tell the player what FOV they should be using while looking through this telescope.
// currentFov will change as the player makes inputs to ZoomIn() and ZoomOut(),
// and then additional FOV changes will be applied by our zoom transitions
float idVentpeekTelescope::GetCurrentFOV()
{
	float returnFov = currentFov;
	if (!fovIsLerping)
	{
		if (zoomTransitionState == ZOOMIN_BLURIN)
		{
			returnFov -= currentFov * ZOOM_FOV_PERCENT_CHANGE * idMath::CubicEaseIn(1 - ((float)zoomTransitionTimer / (float)zoomTransitionTimerStart));
		}
		else if (zoomTransitionState == ZOOMIN_BLUROUT)
		{
			returnFov += currentFov * ZOOM_FOV_PERCENT_CHANGE * idMath::CubicEaseIn((float)zoomTransitionTimer / (float)zoomTransitionTimerStart);
		}
		else if (zoomTransitionState == ZOOMOUT_BLURIN)
		{
			returnFov += currentFov * ZOOM_FOV_PERCENT_CHANGE * idMath::CubicEaseIn(1 - ((float)zoomTransitionTimer / (float)zoomTransitionTimerStart));
		}
		else if (zoomTransitionState == ZOOMOUT_BLUROUT)
		{
			returnFov -= currentFov * ZOOM_FOV_PERCENT_CHANGE * idMath::CubicEaseIn((float)zoomTransitionTimer / (float)zoomTransitionTimerStart);
		}
	}
	else
	{
		// Doing a FOV lerp
		if (gameLocal.time <= fovLerpEndTime)
		{
			switch (fovLerpType)
			{
				case LERPTYPE_LINEAR:
				{
					returnFov = idMath::Lerp(fovLerpStart, fovLerpEnd, (float)(gameLocal.time - fovLerpStartTime) / (float)(fovLerpEndTime - fovLerpStartTime));
					break;
				}
				case LERPTYPE_CUBIC_EASE_IN:
				{
					returnFov = idMath::Lerp(fovLerpStart, fovLerpEnd, idMath::CubicEaseIn((float)(gameLocal.time - fovLerpStartTime) / (float)(fovLerpEndTime - fovLerpStartTime)));
					break;
				}
				case LERPTYPE_CUBIC_EASE_OUT:
				{
					returnFov = idMath::Lerp(fovLerpStart, fovLerpEnd, idMath::CubicEaseOut((float)(gameLocal.time - fovLerpStartTime) / (float)(fovLerpEndTime - fovLerpStartTime)));
					break;
				}
				case LERPTYPE_CUBIC_EASE_INOUT:
				{
					returnFov = idMath::Lerp(fovLerpStart, fovLerpEnd, idMath::CubicEaseInOut((float)(gameLocal.time - fovLerpStartTime) / (float)(fovLerpEndTime - fovLerpStartTime)));
					break;
				}
			}
		}
		else
		{
			// Complete the lerp
			currentFov = fovLerpEnd;
			returnFov = currentFov;
			fovIsLerping = false;
		}
	}

	return returnFov;
}

int idVentpeekTelescope::GetPhotoCount()
{
	return photoCount;
}

bool idVentpeekTelescope::CanTakePhoto()
{
	return canUseCamera && cameraState == CAMERA_IDLE;
}

// Plays a short warmup sound before doing a shutter effect. Largely just for flavour EXCEPT during the end of the sequence
void idVentpeekTelescope::TakePhoto()
{
	StartSound("snd_warmup", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL);
	cameraState = CAMERA_WARMUP;
	cameraTimer = gameLocal.time + CAMERA_WARMUP_TIMER;
	photoCount++;

	
	if (isLockedOn && lockOnFunction != NULL)
	{
		canUseCamera = false; // lock the player out of mashing the camera
		gameLocal.GetLocalPlayer()->hud->SetStateBool("takefinalphoto", false); // Hide the "LMB: TAKE PHOTO" prompt
		idThread* thread;
		thread = new idThread(lockOnFunction); // This should be the little scripted bit where Zena looks at the camera, etc
		thread->DelayedStart(0);
	}
}

void idVentpeekTelescope::HandleCamera(void)
{
	if (cameraState == CAMERA_WARMUP && gameLocal.time > cameraTimer)
	{
		// State change: warmup is done, move to shutter
		cameraState = CAMERA_CLOSE_SHUTTER;
		cameraTimer = gameLocal.time + CAMERA_CLOSE_SHUTTER_TIMER;

		StartSound("snd_shutter", SND_CHANNEL_ANY);
	}
	else if (cameraState == CAMERA_CLOSE_SHUTTER && gameLocal.time > cameraTimer)
	{
		// State change: shutter has closed, open shutter
		cameraState = CAMERA_OPEN_SHUTTER;
		cameraTimer = gameLocal.time + CAMERA_OPEN_SHUTTER_TIMER;
	}
	else if (cameraState == CAMERA_OPEN_SHUTTER && gameLocal.time > cameraTimer)
	{
		// State change: shutter has opened, return to idle
		cameraState = CAMERA_IDLE;
	}
	// else: camera is idle, do nothing
}

// 1.0 to 0.0, shrinks and grows during shutter sequence
float idVentpeekTelescope::GetApertureSize(void)
{
	if (cameraState == CAMERA_CLOSE_SHUTTER)
	{
		return max((float)(cameraTimer - gameLocal.time) / (CAMERA_CLOSE_SHUTTER_TIMER), 0.0f);
	}
	else if (cameraState == CAMERA_OPEN_SHUTTER)
	{
		return 1.0f - max((float)(cameraTimer - gameLocal.time) / (CAMERA_OPEN_SHUTTER_TIMER), 0.0f);
	}
	else return 1.0f;
}

float idVentpeekTelescope::GetRotateScale(void)
{
	// Stop the player from making panning inputs by making rotation scale 0
	return canPanAndZoom ? rotateScale : 0.0f;
}

void idVentpeekTelescope::StartFastForward(void)
{
	isFastForward = true;
	canPanAndZoom = false;
	fastForwardWait = ZOOM_TRANSITION_MAXTIME / 2;
}

void idVentpeekTelescope::StopFastForward(void)
{
	isFastForward = false;
	canPanAndZoom = true;
}
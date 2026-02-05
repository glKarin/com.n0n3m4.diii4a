#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "bc_meta.h"
#include "bc_ventdoor.h"
#include "bc_ventpeek.h"


const idEventDef EV_Vent_Close("close", NULL);

CLASS_DECLARATION(idDoor, idVentdoor)
EVENT(EV_Vent_Close, idVentdoor::VentClose)
END_CLASS

#define NUDGE_THRESHOLDDIST_TOP 16 //player on top of door, wants to go downward
#define NUDGE_THRESHOLDDIST_BOTTOM 36 //player below door, wants go go upward

const int DOOR_CLOSEDELAY = 600;
const int FORCEDUCK_BASE_DURATION = 400;

//TODO: activate clamber system when entering doors that raise upward/downward.

//TODO: only crouch if the area you're entering has a low ceiling. This is to prevent the awkward crouch-then-immediate-uncrouch situation.

//TODO: make unfrobbable when open.

//TODO: Position the open/close particle fx.

idVentdoor::idVentdoor()
{
	ventdoorNode.SetOwner(this);
	ventdoorNode.AddToEnd(gameLocal.ventdoorEntities);	

	locbox = NULL;

	ventTimer = 0;
	ventState = 0;
}

idVentdoor::~idVentdoor(void)
{
	ventdoorNode.Remove();

	//delete peekSparkle;
}

void idVentdoor::Spawn(void)
{
	ventState = VENT_NONE;
	ventTimer = 0;

	displayNamePeek = spawnArgs.GetString( "displaynamepeek" );

	idVec3 forwardDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, &upDir);
	idBounds doorBounds = this->GetPhysics()->GetBounds();

	if (spawnArgs.GetBool("zipdoor", "1"))
	{
		idVec3 peekPos = this->GetPhysics()->GetOrigin() + ( forwardDir * 1.0f ) + ( upDir * ( doorBounds[0].z + .1f ) ); //nudge it up a little bit, so that it doesnt z fight with the doorframe.
		SpawnVentPeek( "env_ventpeek_ventdoor", peekPos );
	}
	else
	{
		peekEnt = NULL;
		//peekSparkle = NULL;
	}

	for (int i = 0; i < 2; i++)
	{
		//Spawn the purge warning signs.
		//0 = the outside sign.
		//1 = the interior sign.

		float forwardOffset = (i <= 0) ? -1.8f : 1.8f;

		idVec3 signPos = GetPhysics()->GetOrigin() + (forwardDir * forwardOffset) + (upDir * (-doorBounds[0].z + .5f));
		idAngles signAngle = GetPhysics()->GetAxis().ToAngles();
		if (fabs(signAngle.pitch) < .1f)
		{			
		}
		else
		{
			signAngle.pitch += 180;
		}

		signAngle.yaw += (i <= 0) ? 180 : 0;

		idDict args;
		args.Clear();
		args.SetVector("origin", signPos);
		args.SetMatrix("rotation", signAngle.ToMat3());
		args.SetBool("hide", true);
		args.Set("model", "model_purgewarning");

		if (i >= 1)
		{
			args.Set("skin", "skins/objects/vent_purgewarning/skin_interior");
		}

		warningsign[i] = (idAnimatedEntity*)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
		warningsign[i]->Event_PlayAnim("stowed", 0);
	}

	//BC 3-25-2025: locbox
	#define LOCBOXRADIUS 4
	idVec3 locboxPos = GetPhysics()->GetOrigin() + (upDir * (-doorBounds[0].z + .5f) + (upDir * -LOCBOXRADIUS));
	idDict args;
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_def_gameplay_ventpurgesign"));
	args.SetVector("origin", locboxPos);
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	locbox = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));
	locbox->Hide();

}

void idVentdoor::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( ventTimer ); //  int ventTimer
	savefile->WriteInt( ventState ); //  int ventState
	SaveFileWriteArray( warningsign, 2, WriteObject ); // idAnimatedEntity *warningsign[2]
	savefile->WriteString( displayNamePeek ); //  idString displayNamePeek
	savefile->WriteObject( locbox ); //  idEntity* locbox
}

void idVentdoor::Restore(idRestoreGame *savefile)
{	
	savefile->ReadInt( ventTimer ); //  int ventTimer
	savefile->ReadInt( ventState ); //  int ventState
	SaveFileReadArrayCast( warningsign, ReadObject, idClass*& ); // idAnimatedEntity *warningsign[2]
	savefile->ReadString( displayNamePeek ); //  idString displayNamePeek
	savefile->ReadObject( locbox ); //  idEntity* locbox
}

void idVentdoor::VentClose()
{
	this->Close();
}

void idVentdoor::Event_Reached_BinaryMover(void)
{
	//Door has fully opened or fully closed. Do a little smoke puff effect.
	if (moverState == MOVER_2TO1 || moverState == MOVER_1TO2)
	{
		idVec3 machineForward;
		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&machineForward, NULL, NULL);
		idEntityFx::StartFx("fx/doorclose", GetPhysics()->GetOrigin() + machineForward * -24, mat3_default);
	}

	idDoor::Event_Reached_BinaryMover();
}

idVec3 idVentdoor::GetPlayerDestinationPos()
{
	idVec3 doorFrontDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors( &doorFrontDir, NULL, NULL );
	
	float facingResult = GetFacingResult();

	idVec3 playerDestinationPosition = vec3_zero;

	if ( facingResult < 0 )
	{
		return vec3_zero;
	}

	if ( doorFrontDir.z == 1 )
	{
		//Player is attempting to enter a ceiling vent.
		playerDestinationPosition = GetPhysics()->GetOrigin() + idVec3( 0, 0, 2 );
	}
	else
	{
		trace_t ventTr;
		idVec3 downDir;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors( &downDir, NULL, NULL );
		gameLocal.clip.TracePoint( ventTr, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + ( downDir * 72 ), MASK_SOLID, this );

		if ( developer.GetInteger() >= 2 )
		{
			gameRenderWorld->DebugArrow( colorRed, GetPhysics()->GetOrigin(), ventTr.endpos, 4, 60000 );
		}

		if ( ventTr.fraction >= 1.0f )
		{
			//ENTERING VENT ON WALL.
			//Traced beyond the door, and the trace did NOT hit a wall. No wall found, so zip player to behind the door.

			//Check if door is angled orthogonal. If NOT at orthogonal angle, then we move the player farther in, so that the door doesn't clip the player's bounding box.
			int offsetAmount;
			idAngles doorAngle = GetPhysics()->GetAxis().ToAngles();

  			float orthoTest = fmod( idMath::Fabs(doorAngle.yaw), 90.0f );
			if ( orthoTest < 1.0f || orthoTest > 89.0f )
				offsetAmount = 24;
			else
				offsetAmount = 32;

			//Get forward position.
			idVec3 candidatePos = GetPhysics()->GetOrigin() + ( downDir * offsetAmount );
			trace_t downTr;

			//Now trace down to find the ground.
			gameLocal.clip.TracePoint( downTr, candidatePos, candidatePos + idVec3( 0, 0, -80 ), MASK_SOLID, this );

			if ( developer.GetInteger() >= 2 )
			{
				gameRenderWorld->DebugArrow( colorMagenta, candidatePos, downTr.endpos, 4, 60000 );
			}

			// fix the case where there's a drop, but the player clips into wall
			if (downTr.fraction >= 1.0f)
			{
				// trace backwards from the drop to find wall
				trace_t wallTrace;
				idVec3 dropPos = candidatePos + idVec3(0, 0, pm_crouchheight.GetFloat()*0.5f - pm_normalheight.GetFloat());
				float playerRadius = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds()[1][1];
				gameLocal.clip.TracePoint( wallTrace, dropPos, dropPos + (downDir*-playerRadius), MASK_SOLID, this );

				if (wallTrace.fraction < 1.0f) // wall found
				{ // pushout from wall
					playerDestinationPosition = wallTrace.endpos + (downDir * playerRadius);
				}
				else
				{
					playerDestinationPosition = downTr.endpos;
				}
			}
			else
			{
				if (offsetAmount >= 32)
				{	// if diag offset, first check that end pos is actually lower than the closer ortho offset
					idVec3 candidatePosClose = GetPhysics()->GetOrigin() + ( downDir * 24 );
					trace_t downTrClose;
					gameLocal.clip.TracePoint(downTrClose, candidatePosClose, candidatePosClose + idVec3(0, 0, -80), MASK_SOLID, this);
					playerDestinationPosition = downTr.endpos.z < downTrClose.endpos.z ? downTrClose.endpos : downTr.endpos;
				}
				else
				{
					playerDestinationPosition = downTr.endpos;
				}
			}
		}
		else
		{
			//ENTERING VENT ON GROUND.
			//Great. Hit a wall. Now trace downward.
			trace_t downTr;
			idVec3 wallHitPoint = ventTr.endpos + ( downDir * -.1f );

			gameLocal.clip.TracePoint( downTr, wallHitPoint, wallHitPoint + idVec3( 0, 0, -80 ), MASK_SOLID, this );

			if ( developer.GetInteger() >= 2 )
			{
				gameRenderWorld->DebugArrow( colorOrange, wallHitPoint, downTr.endpos, 4, 5000 );
			}

			if ( downTr.fraction < 1 )
			{
				//Found the ground.
				idBounds	playerbounds;
				idVec3		candidatePosition;

				playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
				playerbounds[1].z = pm_crouchheight.GetFloat(); //Get crouching bounding box.

				for ( int i = 0; i < 32; i += 4 )
				{
					trace_t		boundTr;

					candidatePosition = ( downTr.endpos + idVec3( 0, 0, .1f ) ) + ( downDir * -i );

					gameLocal.clip.TraceBounds( boundTr, candidatePosition, candidatePosition, playerbounds, MASK_SOLID, NULL );
					//gameRenderWorld->DebugBounds(colorYellow, playerbounds, candidatePosition, 5000);

					if ( boundTr.fraction >= 1.0f )
					{
						//Great. Found a valid candidate.
						playerDestinationPosition = candidatePosition;

						if ( developer.GetInteger() >= 2 )
						{
							gameRenderWorld->DebugBounds( colorGreen, playerbounds, playerDestinationPosition, 5000 );
						}

						break;
					}
				}
			}
		}
	}

	return playerDestinationPosition;
}

float idVentdoor::GetFacingResult() const
{
	idVec3 doorFrontDir;
	idVec3 doorOrigin;
	idVec3 toPlayer;
	float facingResult;

	idVec3 playerDestinationPosition = vec3_zero;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors( &doorFrontDir, NULL, NULL );
	doorOrigin = this->GetPhysics()->GetOrigin();
	toPlayer = doorOrigin - gameLocal.GetLocalPlayer()->GetEyePosition();
	facingResult = DotProduct( toPlayer, doorFrontDir );

	// Ventdoors without this keyvalue should always be treated as if we're on the inside of them
	// (At least, for the purposes of movement)
	if ( !spawnArgs.GetBool( "zipdoor", "1" ) )
	{
		facingResult = -1;
	}

	return facingResult;
}

bool idVentdoor::DoFrob(int index, idEntity * frobber)
{
	if (frobber && frobber == gameLocal.GetLocalPlayer())
	{
		if (this->IsBarricaded())
		{
			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_unlockedelsewhere");
			StartSound("snd_locked", SND_CHANNEL_ANY);
			return false;
		}

		idVec3 doorFrontDir;

		idVec3 playerDestinationPosition = vec3_zero;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&doorFrontDir, NULL, NULL);
		float facingResult = GetFacingResult();

		//Get what side player is frobbing the ventdoor.
		if (facingResult < 0)
		{
			//Player is inside vent and wants to exit.
			//this->wait = 3; //Stay open longer if player is inside vent shaft.

			//If player is opening a ventdoor below themselves, we do a little nudge to center the player so they can easily drop through it.
			if (doorFrontDir.z == 1)
			{
				//Player is ABOVE the door.
				float distanceToPlayer = (GetPhysics()->GetOrigin() - idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z)).Length();
				if (distanceToPlayer <= NUDGE_THRESHOLDDIST_TOP)
				{
					//Nudge player to center.
					gameLocal.GetLocalPlayer()->StartMovelerp(idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z + .1f));
				}
			}
			else if (doorFrontDir.z == -1)
			{
				//Player is UNDERNEATH the door.
				float distanceToPlayer = (GetPhysics()->GetOrigin() - idVec3(gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().x, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().y, GetPhysics()->GetOrigin().z)).Length();
				if (distanceToPlayer <= NUDGE_THRESHOLDDIST_BOTTOM)
				{
					//Nudge player to center.
					gameLocal.GetLocalPlayer()->StartMovelerp(idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin().z + .1f));
				}
			}
		}
		else
		{
			//Player is outside vent and wants to enter it.
			playerDestinationPosition = GetPlayerDestinationPos();
			if (playerDestinationPosition != vec3_zero)
			{
				int targetYaw = -1;

				ventState = VENT_PLAYERZIPPING;
				ventTimer = gameLocal.time + 500;

				gameLocal.GetLocalPlayer()->ZippingTo(playerDestinationPosition, this->GetAperture(), FORCEDUCK_BASE_DURATION + duration);

				if (developer.GetInteger() >= 2)
				{
					idBounds	playerbounds;
					playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
					gameRenderWorld->DebugBounds(colorCyan, playerbounds, playerDestinationPosition, 5000);
				}


				PostEventMS(&EV_Vent_Close, DOOR_CLOSEDELAY);

				targetYaw = spawnArgs.GetInt("targetyaw", "-1");
				if (targetYaw >= 0)
				{
					gameLocal.GetLocalPlayer()->SetViewYawLerp(targetYaw, 400);
				}
				
				//Check if player is being SEEN entering the vent.
				if (gameLocal.GetAmountEnemiesSeePlayer(true) > 0 && gameLocal.GetLocalPlayer()->GetDarknessValue() > 0)
				{
					gameLocal.AddEventLog("#str_def_gameplay_vent_seenenter", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin());
					gameLocal.GetLocalPlayer()->confinedStealthActive = false;
					static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartVentPurge(nullptr); //We don't really track who saw the player, so just use generic voiceprint A...
				}
			}
		}

		if (peekEnt != NULL)
		{
			peekEnt->Hide();
			
			//peekSparkle->SetActive(false);
			//peekSparkle->Hide();
		}
	}

	if (this->IsBarricaded())
	{
		StartSound("snd_cancel", SND_CHANNEL_ANY);
		return false;
	}

	//this->isFrobbable = false;
	Open();

	return true;
}

bool idVentdoor::IsFrobHoldable() const
{
	return peekEnt != NULL && GetFacingResult() < 0.0f;
}

//const idStr& idVentdoor::GetFrobName() const
//{
//	return IsFrobHoldable() ? displayNamePeek : displayName;	
//}



// SW:
// This returns a winding representing the four corners of the ventdoor's aperture.
// We use this simplified representation to control how the camera passes through this aperture when zipping into a ventdoor,
// though it may have other uses.
//
// Assumptions made here:
// - The ventdoor clipmodel in its 'natural' state stands vertically, with its main dimensions along the Y and Z axes
// - The ventdoor is a rectangular prism, not a triangle or some other funky shape
idWinding* idVentdoor::GetAperture()
{
	idClipModel* clipmodel = this->GetPhysics()->GetClipModel();
	idVec3 bboxPoints[8];

	clipmodel->GetBounds().ToPoints(bboxPoints);
	float minY = MAX_WORLD_COORD;
	float maxY = -MAX_WORLD_COORD;
	float minZ = MAX_WORLD_COORD;
	float maxZ = -MAX_WORLD_COORD;

	for (int i = 0; i < 8; i++)
	{
		if (bboxPoints[i].y < minY)
			minY = bboxPoints[i].y;
		if (bboxPoints[i].y > maxY)
			maxY = bboxPoints[i].y;
		if (bboxPoints[i].z < minZ)
			minZ = bboxPoints[i].z;
		if (bboxPoints[i].z > maxZ)
			maxZ = bboxPoints[i].z;
	}

	idVec3 points[] = {
		idVec3(0, minY, minZ),
		idVec3(0, maxY, minZ),
		idVec3(0, maxY, maxZ),
		idVec3(0, minY, maxZ) };

	idRotation rotation = this->GetPhysics()->GetAxis().ToRotation();
	idVec3 origin = this->GetPhysics()->GetOrigin();
	for (int i = 0; i < 4; i++)
	{
		rotation.RotatePoint(points[i]);
		points[i] += origin;
	}
	idWinding* apertureWinding = new idWinding(points, 4);
	
	return apertureWinding;
}


void idVentdoor::SetPurgeSign( bool value)
{
	if (value)
	{
		for (int i = 0; i < 2; i++)
		{
			warningsign[i]->Event_PlayAnim("deploy", 0);
			warningsign[i]->Show();			
		}

		locbox->Show();
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			warningsign[i]->Event_PlayAnim("undeploy", 0);			
		}

		locbox->Hide();
	}
}

// SW: Override idDoor's method to prevent vents from messing with any nearby cluster portals
// (BC because ai can not enter vent spaces, we are going to have ventdoors not toggle aas on/off. The reason 
// we're doing this is because the ventdoor aas toggle was causing some pathing issues.)
void idVentdoor::SetAASAreaState(bool closed) { /* do nothing */ }
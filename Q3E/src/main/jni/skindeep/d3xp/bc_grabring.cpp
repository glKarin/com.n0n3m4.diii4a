#include "sys/platform.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"

#include "bc_grabring.h"



CLASS_DECLARATION(idEntity, idGrabRing)
END_CLASS


idGrabRing::idGrabRing(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;
}

idGrabRing::~idGrabRing(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}
}

void idGrabRing::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	isFrobbable = true;


	idVec3 up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
	headlight.shader = declManager->FindMaterial("lights/pulse03_ext", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 24.0f;
	headlight.shaderParms[0] = 0;
	headlight.shaderParms[1] = .5f;
	headlight.shaderParms[2] = 0;
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	headlight.origin = GetPhysics()->GetOrigin() + up * 8;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
}

void idGrabRing::Save(idSaveGame *savefile) const
{
	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle
}

void idGrabRing::Restore(idRestoreGame *savefile)
{
	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}
}

void idGrabRing::Think(void)
{
	idEntity::Think();


}


const int GRAB_MAXGRABRANGE = 128; //how far we allow the player to 'hang on' to the grab ring.
const int GRAB_CHECKINTERVAL = 8; //when doing tracebound check, distance between checks. Smaller number = more accurate
const int GRAB_BUFFERSPACE = 4; //when doing tracebound check, increase bounding box by XX size, for safety sake

bool idGrabRing::DoFrob(int index, idEntity * frobber)
{
	//Player wants to grab this grab ring.


	//BC 4-21-2025: only allow if player directly frobs me (ignore if player throws object at me)
	if (frobber == nullptr)
		return false;

	if (frobber != gameLocal.GetLocalPlayer())
		return false;



	//Try to find safe clearance for the player.
	idVec3 destinationPos = vec3_zero;

	

	//Ideally, we want the player's head to be positioned as close as possible to the grab ring. To make it look like the player's arms are within range of grabbing the grab ring.
	//So, do player bounding box checks to position the player's head close to the grab ring.
	destinationPos = GetClearancePosition(0, 0);
	
	//If we can't find a spot, we do additional checks where we wiggle the offset a little.
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(8, 0); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(-8, 0); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(0, 8); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(0, -8); }
	
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(8, 8); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(-8, 8); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(8, -8); }
	if (destinationPos == vec3_zero) { destinationPos = GetClearancePosition(-8, -8); }

	if (destinationPos == vec3_zero)
	{
		//Every fallback check failed.
		gameLocal.Warning("cannot find grabring clearance: '%s'\n", GetName());
		StartSound("snd_cancel", SND_CHANNEL_ANY);
		return false;
	}

	StartSound("snd_grab", SND_CHANNEL_ANY);
	gameLocal.GetLocalPlayer()->StartGrabringGrab(destinationPos, this);
	isFrobbable = false;

	return true;
}

idVec3 idGrabRing::GetClearancePosition(int rightOffset, int forwardOffset)
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	int playerEyeHeight = gameLocal.GetLocalPlayer()->EyeHeight();
	idBounds playerbounds;
	playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds.ExpandSelf(GRAB_BUFFERSPACE); //expand a little for safety sake.

	for (int i = GRAB_BUFFERSPACE + 1; i < GRAB_MAXGRABRANGE; i += GRAB_CHECKINTERVAL)
	{
		trace_t tr;

		//we nudge it down (playerEyeHeight) to bias the player's eyeball to be near the grab ring.
		idVec3 checkPos = GetPhysics()->GetOrigin() + (up * i) + idVec3(0, 0, -playerEyeHeight) + (right * rightOffset) + (forward  * forwardOffset); 
		gameLocal.clip.TraceBounds(tr, checkPos, checkPos, playerbounds, MASK_SOLID, this);
		//gameRenderWorld->DebugBounds((tr.fraction < 1) ? colorRed : colorGreen, playerbounds, checkPos, 60000);

		if (tr.fraction >= 1)
		{
			return checkPos;
		}
	}

	return vec3_zero;
}



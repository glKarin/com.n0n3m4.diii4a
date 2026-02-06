#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"

#include "bc_teleportpuck.h"






CLASS_DECLARATION(idMoveableItem, idTeleportPuck)

END_CLASS

//TODO: Glow FX when it hits a surface.

//TODO: pre-cache whatever it needs.



void idTeleportPuck::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( HasCollided ); // bool HasCollided
	savefile->WriteVec3( startPosition ); // idVec3 startPosition
}

void idTeleportPuck::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( HasCollided ); // bool HasCollided
	savefile->ReadVec3( startPosition ); // idVec3 startPosition
}

void idTeleportPuck::Spawn(void)
{
	HasCollided = false;
	startPosition = this->GetPhysics()->GetOrigin();
}

void idTeleportPuck::Think(void)
{
	// SW: Because thrown objects slowly drift to a halt in a vacuum, 
	// it's possible to get a case where the thrown puck stays forever in the pre-collision state, unable to be interacted with.
	// We break out of this special case by destroying the puck and creating the pickup again.
	if (!HasCollided && this->GetPhysics()->GetLinearVelocity().LengthFast() < THROWABLE_DRIFT_THRESHOLD)
	{
		idMoveableItem::TryRevertToPickup();
	}

	idMoveableItem::Think();
}

bool idTeleportPuck::Collide(const trace_t &collision, const idVec3 &velocity)
{
	float		v;
	idAngles	destinationAngle;
	idVec3		destinationPos;
	idVec3		particlePos;	

	idMoveableItem::Collide(collision, velocity);

	if (HasCollided)
		return false;

	v = -(velocity * collision.c.normal);

	if (v < 1)
		return false;

	// SW: Cut outgoing velocity so that the telepuck doesn't bounce quite so violently around after landing
	GetPhysics()->SetLinearVelocity(velocity * 0.5f);
	
	destinationAngle = idAngles(gameLocal.GetLocalPlayer()->viewAngles.pitch, gameLocal.GetLocalPlayer()->viewAngles.yaw, 0);

	//Check if destination is clear.
	destinationPos = FindClearDestination(GetPhysics()->GetOrigin(), collision);
	
	//BC the new phase behavior.
	//destinationPos = gameLocal.GetPhasedDestination(collision, startPosition);	

	gameLocal.ProjectDecal(collision.endpos, -collision.c.normal, 8.0f, true, 96, "textures/decals/glowball");

	this->isFrobbable = true;
	HasCollided = true;


	if (destinationPos != vec3_zero)
	{
		//player has successfully phase-teleported.
		//make the decal appear.
		trace_t decalTr;

		idVec3 collisionDir = collision.endpos - destinationPos;
		collisionDir.Normalize();
		gameLocal.clip.TracePoint(decalTr, destinationPos, destinationPos + (collisionDir * 64), MASK_SOLID, NULL);
		if (decalTr.fraction < 1)
		{
			gameLocal.ProjectDecal(decalTr.endpos, -decalTr.c.normal, 8.0f, true, 96, "textures/decals/donutburn_yellow");
		}


        //teleport the puck itself.
        if (gameLocal.GetAirlessAtPoint(destinationPos))
        {
            //if puck ends up in outer space,we do some logic to make the puck appear IN FRONT OF PLAYER'S EYES to make the teleport little easier to understand, puck easier to see, etc etc            
            trace_t airlessTr;
            gameLocal.clip.TracePoint(airlessTr, destinationPos, destinationPos + idVec3(0, 0, 64), MASK_SOLID, NULL);
            this->GetPhysics()->SetOrigin(airlessTr.endpos);

            GetPhysics()->SetLinearVelocity(velocity * 0.1f); //slow down its speed
        }
        else
        {
            //puck is NOT in an airless environment. Just make the puck teleport normally.
            this->GetPhysics()->SetOrigin(destinationPos);
        }


		GetPhysics()->SetAngularVelocity(this->GetPhysics()->GetAngularVelocity() * .2f); //reduce the spinning behavior.

		UpdateGravity(true);
	}
	else
	{
		//failed to phase through a wall. fall back to the default teleport behavior.
		destinationPos = FindClearDestination(GetPhysics()->GetOrigin(), collision);
	}

	if (destinationPos == vec3_zero)
	{
		return false; //failed all teleport checks. get outta here.
	}


	gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);

	particlePos = destinationPos + destinationAngle.ToForward() * 32; //make particle appear in front of player so that they can see the distortion fx

	//TODO: make decal appear correctly when it hits a wall.
	idEntityFx::StartFx("fx/teleport01", &particlePos, &mat3_identity, NULL, false);

	//Teleport player.
	gameLocal.GetLocalPlayer()->Teleport(destinationPos, destinationAngle, NULL, true);	
	return false;
}




//This is the teleport behavior when NOT phasing through a wall.
//0,0,0 = fail.
idVec3 idTeleportPuck::FindClearDestination(idVec3 pos, trace_t collisionInfo)
{
	idBounds	playerbounds;
	trace_t		trBounds;
	int			i;
	
	playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds[1].z = pm_crouchheight.GetFloat(); //Get crouching bounding box.
	gameLocal.clip.TraceBounds(trBounds, pos, pos, playerbounds, MASK_SOLID, gameLocal.GetLocalPlayer());	
	if (trBounds.fraction >= 1)
	{
		//gameRenderWorld->DebugBounds(colorYellow, playerbounds, pos, 60000);
		//All clear. Return position.
		return pos;
	}

	//Area is not clear! Find a good clear position.

	//Extrude out from the collision normal.
	for (i = 0; i < 64; i += 8)
	{
		trace_t trCheck;
		idVec3 newPos = pos + (collisionInfo.c.normal * i);
		gameLocal.clip.TraceBounds(trCheck, newPos, newPos, playerbounds, MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());
		if (trCheck.fraction >= 1)
		{
			//gameRenderWorld->DebugBounds(colorGreen, playerbounds, newPos, 60000);
			return newPos;
		}
	}


	//Try some other positions. Test out 8 ordinal directions, sprouting out of the collision normal.
	idVec2 offsets[8] =
	{
		//Prioritize the higher-altitude ones first.
		idVec2(0,1),
		idVec2(1,1),
		idVec2(-1,1),

		idVec2(1,0),
		idVec2(-1,0),

		idVec2(0,-1),
		idVec2(1,-1),		
		idVec2(-1,-1),
	};

	idVec3 right, up;
	collisionInfo.c.normal.ToAngles().ToVectors(NULL, &right, &up);
	#define OFFSETDIST 32
	for (i = 0; i < 8; i++)
	{
		trace_t trCheck;
		idVec3 newPos = pos + (collisionInfo.c.normal * 24) + (right * (offsets[i].x*OFFSETDIST)) + (up * (offsets[i].y*OFFSETDIST));
		gameLocal.clip.TraceBounds(trCheck, newPos, newPos, playerbounds, MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());
		//gameRenderWorld->DebugBounds(colorGreen, playerbounds, newPos, 60000);
		if (trCheck.fraction >= 1)
		{	
			return newPos;
		}
	}


	
	return vec3_zero;
}

bool idTeleportPuck::DoFrob(int index, idEntity * frobber)
{
	this->Hide();
	fl.takedamage = false;
	PostEventMS(&EV_Remove, 100);

	//gameLocal.GetLocalPlayer()->Give("weapon", "weapon_telegun");
	//gameLocal.GetLocalPlayer()->SetAmmoDelta("ammo_telegun", 1);
	gameLocal.GetLocalPlayer()->GiveItem("weapon_telegun");

	//Particle fx for picking up item.
	idEntityFx::StartFx("fx/pickupitem", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

	gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);


	return true;
}

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"

#include "bc_zena.h"

CLASS_DECLARATION(idAnimatedEntity, idZena)
END_CLASS

#define ANIM_BLENDFRAMES		12
#define FOOTSTEPPARTICLETIME	200

idZena::idZena(void)
{
	state = ZENA_IDLE;
	currentAnim = -1;
	footstepparticleTimer = 0;
}

idZena::~idZena(void)
{
}

void idZena::Spawn(void)
{
	fl.takedamage = false;


	BecomeActive(TH_THINK);
}

void idZena::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( currentAnim ); // int currentAnim
	savefile->WriteInt( footstepparticleTimer ); // int footstepparticleTimer
}

void idZena::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( currentAnim ); // int currentAnim
	savefile->ReadInt( footstepparticleTimer ); // int footstepparticleTimer
}

void idZena::Think(void)
{
	if (fl.hidden || gameLocal.GetLocalPlayer()->noclip)
		return;

	//make it mirror the player's position. Assume map xyz is 0,0,0 the center point.
	idVec3 playerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	idVec3 zenaPosition = idVec3(-playerPosition.x, -playerPosition.y, playerPosition.z);
	SetOrigin(zenaPosition);

	//Just make zena always face the player.
	idVec3 dirToPlayer = playerPosition - idVec3(zenaPosition.x, zenaPosition.y, playerPosition.z);
	dirToPlayer.Normalize();
	SetAngles(dirToPlayer.ToAngles());

	//Animation.	
	idPlayer* player = gameLocal.GetLocalPlayer();	
	int playerStrafing = player->usercmd.rightmove;	
	int playerForward = player->usercmd.forwardmove;

	if (playerForward > 0 && !playerStrafing)
	{
		SetZenaAnim((int)ANIM_FORWARD);
	}
	else if (playerForward < 0 && !playerStrafing)
	{
		SetZenaAnim((int)ANIM_BACKWARD);
	}
	else if (playerStrafing > 0)
	{
		SetZenaAnim((int)ANIM_STRAFERIGHT);
	}
	else if (playerStrafing < 0)
	{
		SetZenaAnim((int)ANIM_STRAFELEFT);
	}
	else
	{
		SetZenaAnim((int)ANIM_IDLE);
	}			

	UpdateFootstepParticles();

	idAnimatedEntity::Think();
}

void idZena::SetZenaAnim(int animType)
{
	if (animType == currentAnim)
		return;

	currentAnim = animType;

	idStr animStr = "";
	switch (animType)
	{
		case (int)ANIM_IDLE:		animStr = "idle"; break;
		case (int)ANIM_FORWARD:		animStr = "walk_forward"; break;
		case (int)ANIM_BACKWARD:	animStr = "walk_backward"; break;
		case (int)ANIM_STRAFELEFT:	animStr = "walk_left"; break;
		case (int)ANIM_STRAFERIGHT:	animStr = "walk_right"; break;
	}

	if (animStr.Length() <= 0)
		return;

	Event_PlayAnim(animStr, ANIM_BLENDFRAMES, true);
}

void idZena::UpdateFootstepParticles()
{
	if (!gameLocal.GetLocalPlayer()->GetPhysics()->HasGroundContacts() || footstepparticleTimer > gameLocal.time || currentAnim == ANIM_IDLE)
		return;


	footstepparticleTimer = gameLocal.time + FOOTSTEPPARTICLETIME;

	gameLocal.DoParticle(spawnArgs.GetString("model_footsteps"), GetPhysics()->GetOrigin());
}
#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "idlib/LangDict.h"

#include "SmokeParticles.h"

#include "bc_key.h"

#define GLOW_RADIUS "12"

//const int JUMP_MINTIME = 2000;
//const int JUMP_MAXTIME = 4000;
//const float JUMP_POWER = 256;

CLASS_DECLARATION(idMoveableItem, idSecurityKey)
END_CLASS

idSecurityKey::idSecurityKey(void)
{
    memset(&headlight, 0, sizeof(headlight));
    headlightHandle = -1;
}

idSecurityKey::~idSecurityKey(void)
{
    if (headlightHandle != -1)
        gameRenderWorld->FreeLightDef(headlightHandle);
}

void idSecurityKey::Spawn(void)
{
	BecomeActive(TH_THINK);	
	thinkTimer = 0;
}

void idSecurityKey::Save(idSaveGame *savefile) const
{
	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteInt( thinkTimer ); // int thinkTimer
}

void idSecurityKey::Restore(idRestoreGame *savefile)
{
	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( thinkTimer ); // int thinkTimer
}

void idSecurityKey::Think(void)
{
	idMoveableItem::Think();	

	//Don't do proximity check if I'm being held by the player.
	//if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	//{
	//	if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
	//	{
	//		return;
	//	}
	//}

    if (headlightHandle != -1)
    {
		//Turn off glow light if being held by player.
		idPlayer* player = gameLocal.GetLocalPlayer();
		if (player && player->GetCarryable() == this && player->HasEntityInCarryableInventory(this))
		{
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 0;
		}
		else
		{
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = spawnArgs.GetFloat("glow_radius", GLOW_RADIUS);
		}

		//Light is on. Update position of the light glow.
        idVec3 up;
        GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
        headlight.origin = GetPhysics()->GetOrigin();
        gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);		
    }
	else if (!fl.hidden)
	{
		//Create the light.
		headlight.shader = declManager->FindMaterial("lights/defaultProjectedLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = spawnArgs.GetFloat("glow_radius", GLOW_RADIUS);
		headlight.shaderParms[0] = spawnArgs.GetVector("_color").x;
		headlight.shaderParms[1] = spawnArgs.GetVector("_color").y;
		headlight.shaderParms[2] = spawnArgs.GetVector("_color").z;
		headlight.shaderParms[3] = 1.0f;
		headlight.noShadows = true;
		headlight.isAmbient = true;
		headlight.axis = mat3_identity;
		headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	}


	//DoJumpLogic();
}

//Make the key do a little jump
//void idSecurityKey::DoJumpLogic()
//{
//	if (gameLocal.time < jumpTimer)
//		return;
//
//	jumpTimer = gameLocal.time + gameLocal.random.RandomInt(JUMP_MINTIME, JUMP_MAXTIME);
//
//	if (GetBindMaster() != NULL || IsHidden() ||  !GetPhysics()->HasGroundContacts() || gameLocal.GetAirlessAtPoint(GetPhysics()->GetOrigin()))
//		return;
//
//	//Jump.
//	idVec3 jumpPower = idVec3(0, 0, 1);
//	jumpPower *= JUMP_POWER * GetPhysics()->GetMass();
//	GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), jumpPower);
//}

//bool idSecurityKey::DoFrob(int index, idEntity * frobber)
//{
//	return idMoveableItem::DoFrob(index, frobber);
//}

void idSecurityKey::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	StopSound(SND_CHANNEL_BODY); //stop any ambient sound it might be playing

	//idStr inflictorName = "?";
	//idStr attackerName = "?";
	//if (inflictor != NULL) inflictorName = inflictor->GetName();
	//if (attacker != NULL) attackerName = attacker->GetName();
	//gameLocal.Warning("key '%s' destroyed. Inflictor='%s' Attacker='%s'", GetName(), inflictorName.c_str(), attackerName.c_str());

//#if _DEBUG
//	gameRenderWorld->DebugTextSimple("KEY DESTROYED", GetPhysics()->GetOrigin() + idVec3(0, 0, 64), 90000, colorRed);
//	gameRenderWorld->DebugArrowSimple(GetPhysics()->GetOrigin(), 90000, colorRed);
//#endif

	//Zap it to the lost and found.
	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_lostfound_sent"), displayName.c_str()), GetPhysics()->GetOrigin());
	gameLocal.GetLocalPlayer()->AddLostInSpace(entityDefNumber);

    idMoveableItem::Killed(inflictor, attacker, damage, dir, location);
}

void idSecurityKey::Hide(void)
{
	idMoveableItem::Hide();

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}
}

bool idSecurityKey::Collide(const trace_t& collision, const idVec3& velocity)
{
	return idMoveableItem::Collide(collision, velocity);

	//Detect if key unlocks a thing when it collides.


}
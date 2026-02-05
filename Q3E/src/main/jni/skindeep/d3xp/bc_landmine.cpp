#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"


#include "bc_trigger_gascloud.h"
#include "bc_landmine.h"

const int IDLE_DELAYTIME = 8000;		//Time between idle beeps.
const int ARM_TIME = 1600;				//How long it takes to arm when placed down.
const int TRIPDELAY = 800;				//when tripped, does a little delay before exploding.
const int PROXIMITY_CHECKTIME = 100;	//How often to check for targets.
const int WARNING_RADIUS = 96;			//When something is this close, does a special warning beep.

const int LIGHT_RADIUS = 24;
const int LIGHT_FLASHINTERVAL = 2000;
const int LIGHT_FLASHTIME = 200;
const int LIGHT_FLASHTIMELONG = 500;


CLASS_DECLARATION(idStaticEntity, idLandmine)
END_CLASS

idLandmine::idLandmine(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	landmineNode.SetOwner(this);
	landmineNode.AddToEnd(gameLocal.landmineEntities);
}

idLandmine::~idLandmine(void)
{
	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
	}

	landmineNode.Remove();
}

void idLandmine::Spawn(void)
{
	idDict args;
	idVec3 forwardDir, rightDir;

	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = false;

	BecomeActive(TH_THINK);
	fl.takedamage = true;

	mineState = STATE_ARMING;
	StartSound("snd_arm", SND_CHANNEL_BODY);
	mineTimer = gameLocal.time + ARM_TIME;

	idleTimer = 0;
	thinkTimer = 0;

	gameLocal.DoParticle(spawnArgs.GetString("smoke_place"), GetPhysics()->GetOrigin(), idVec3(1,0,0));

	team = spawnArgs.GetInt("team", "1");

	SetColor(1, 1, 0); //blinks yellow when it is arming.
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_blinkfast")));

	readyForWarning = true;

	//Spawn light.
	idVec3 up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
	headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;
	headlight.shaderParms[0] = 1.0f; // R
	headlight.shaderParms[1] = 1.0f; // G
	headlight.shaderParms[2] = 0.0f; // B
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlight.origin = GetPhysics()->GetOrigin() + (up * 8);
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);

	lightflashState = LFL_IDLE;
	lightflashTimer = 0;
}

void idLandmine::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( mineState ); // int mineState
	savefile->WriteInt( mineTimer ); // int mineTimer

	savefile->WriteInt( idleTimer ); // int idleTimer

	savefile->WriteInt( thinkTimer ); // int thinkTimer

	savefile->WriteBool( readyForWarning ); // bool readyForWarning

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteInt( lightflashState ); // int lightflashState
	savefile->WriteInt( lightflashTimer ); // int lightflashTimer
}

void idLandmine::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( mineState ); // int mineState
	savefile->ReadInt( mineTimer ); // int mineTimer

	savefile->ReadInt( idleTimer ); // int idleTimer

	savefile->ReadInt( thinkTimer ); // int thinkTimer

	savefile->ReadBool( readyForWarning ); // bool readyForWarning

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadInt( lightflashState ); // int lightflashState
	savefile->ReadInt( lightflashTimer ); // int lightflashTimer
}



void idLandmine::Think(void)
{
	idStaticEntity::Think();

	if (mineState == STATE_ARMING)
	{
		//Mine was just laid down, is arming itself.
		if (gameLocal.time >= mineTimer)
		{
			//Is now armed.
			mineState = STATE_ARMED;
			idleTimer = gameLocal.time + IDLE_DELAYTIME;
			gameLocal.DoParticle(spawnArgs.GetString("smoke_sound"), GetPhysics()->GetOrigin() + idVec3(0, 0, 4));
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_default")));

			lightflashTimer = gameLocal.time; //do the light immediately.
		}
	}
	else if (mineState == STATE_ARMED)
	{
		//Idle beep noise.
		if (gameLocal.time >= idleTimer && team == TEAM_ENEMY)
		{
			idleTimer = gameLocal.time + IDLE_DELAYTIME;
			StartSound("snd_idle", SND_CHANNEL_AMBIENT);
			gameLocal.DoParticle(spawnArgs.GetString("smoke_sound"), GetPhysics()->GetOrigin() + idVec3(0, 0, 4));
			DoLightFlash(LIGHT_FLASHTIMELONG);
		}

		if (lightflashState == LFL_FLASHING)
		{
			if (gameLocal.time > lightflashTimer)
			{
				SetLightColor(-1);//turn off lights.
				lightflashState = LFL_IDLE;
				lightflashTimer = gameLocal.time + LIGHT_FLASHINTERVAL;
			}
		}
		else if (lightflashState == LFL_IDLE)
		{
			if (gameLocal.time > lightflashTimer)
			{
				DoLightFlash(LIGHT_FLASHTIME);
			}
		}


		
		

		//Proximity check.
		if (gameLocal.time >= mineTimer)
		{
			mineTimer = gameLocal.time + PROXIMITY_CHECKTIME; //how often to do proximity check.

			bool someoneIsInWarningDistance = false;

			for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
			{
				if (!entity || entity->team == this->team || !entity->IsActive() || entity->health <= 0)
					continue;

				if (entity == gameLocal.GetLocalPlayer() && (gameLocal.GetLocalPlayer()->fl.notarget || gameLocal.GetLocalPlayer()->noclip)) //ignore noclip player.
					continue;

				float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();
				int triggerRadius = (team == TEAM_FRIENDLY) ? spawnArgs.GetInt("trigger_radius_ai", "48") : spawnArgs.GetInt("trigger_radius", "16"); //The AI and player have different trigger radii.
				if (distance < triggerRadius)
				{
					StartTripDelay();
					return;
				}
				else if (distance < WARNING_RADIUS)
				{
					someoneIsInWarningDistance = true;
				}
			}

			if (someoneIsInWarningDistance)
			{
				//Someone is near the landmine. Do the warning sound.
				if (readyForWarning)
				{
					readyForWarning = false;
					StartSound("snd_warning", SND_CHANNEL_BODY3);
					gameLocal.DoParticle(spawnArgs.GetString("smoke_sound"), GetPhysics()->GetOrigin() + idVec3(0, 0, 4));
					DoLightFlash(LIGHT_FLASHTIMELONG);
				}
			}
			else
			{
				//no one is near the landmine. Make the warning available again.
				readyForWarning = true;
			}
		}

	}
	else if (mineState == STATE_TRIPDELAY)
	{
		//About to explode. Is doing the delay before explosion.
		if (gameLocal.time >= mineTimer)
		{
			//kaboom. explode.
			mineState = STATE_EXPLODED;
			idEntityFx::StartFx( spawnArgs.GetString("fx_explode"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);			
			
			//const char *splashDmgDef;
			//splashDmgDef = spawnArgs.GetString("def_splash_damage", "damage_explosion");
			//if (splashDmgDef && *splashDmgDef)
			//{
			//	gameLocal.RadiusDamage(GetPhysics()->GetAbsBounds().GetCenter(), this, NULL, this, this, splashDmgDef);
			//	gameLocal.ThrowShrapnel(GetPhysics()->GetAbsBounds().GetCenter(), spawnArgs.GetString("def_projectile", "projectile_shrapnel"), this);
			//}

			//gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), "interest_explosion");

			DoGasSpew();



			fl.takedamage = false;
			Hide();
			PostEventMS(&EV_Remove, 0);
		}
	}
}

void idLandmine::DoLightFlash(int duration)
{
	if (team == TEAM_ENEMY)
		SetLightColor(TEAM_ENEMY);
	else
		SetLightColor(TEAM_FRIENDLY);

	lightflashState = LFL_FLASHING;
	lightflashTimer = gameLocal.time + duration;
}

void idLandmine::SetLightColor(int faction)
{
	idVec3 lightColor;

	if (faction == TEAM_ENEMY)
	{
		SetColor(ENEMYCOLOR); //red
		lightColor = ENEMYCOLOR;
		renderEntity.shaderParms[7] = 1;
	}
	else if (faction == TEAM_FRIENDLY)
	{
		SetColor(FRIENDLYCOLOR); //blue
		lightColor = FRIENDLYCOLOR;
		renderEntity.shaderParms[7] = 1;
	}
	else if (faction == -1)
	{
		SetColor(0, 0, 0);
		lightColor = vec3_zero;
		renderEntity.shaderParms[7] = 0;
	}

	if (headlightHandle != -1)
	{
		if (lightColor == vec3_zero)
		{
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 0; // make the light basically go away.
		}
		else
		{
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHT_RADIUS;
			headlight.shaderParms[0] = lightColor.x;
			headlight.shaderParms[1] = lightColor.y;
			headlight.shaderParms[2] = lightColor.z;
		}

		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}

	UpdateVisuals();
}

void idLandmine::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	StartTripDelay();
}

// SW 11th March 2025
// Restoring the old frob functionality as hack functionality
void idLandmine::DoHack()
{
	if (mineState == STATE_ARMED || mineState == STATE_ARMING || mineState == STATE_TRIPDELAY || mineState == STATE_RADIUSWARNING)
	{
		//disarm.
		StartSound("snd_disarm", SND_CHANNEL_BODY2);
		mineState = STATE_ARMED;
		isFrobbable = false;

			
		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_default")));
		team = TEAM_FRIENDLY;
		DoLightFlash(LIGHT_FLASHTIMELONG);

		gameLocal.DoParticle("hack01.prt", GetPhysics()->GetOrigin() + idVec3(0, 0, 4));
	}		
}

void idLandmine::StartTripDelay()
{
	if (mineState == STATE_TRIPDELAY)
		return;

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_blinkfast")));
	StartSound("snd_trigger", SND_CHANNEL_BODY);
	mineState = STATE_TRIPDELAY;
	mineTimer = gameLocal.time + TRIPDELAY;

	gameLocal.DoParticle(spawnArgs.GetString("smoke_trigger"), GetPhysics()->GetOrigin() + idVec3(0, 0, 4));


	idEntityFx::StartFx("fx/frob_lines", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);


	DoLightFlash(TRIPDELAY);
}

void idLandmine::DoGasSpew()
{
	//Spew.		
	idVec3 itemCenter = GetPhysics()->GetOrigin() + idVec3(0,0,32);
	int radius = spawnArgs.GetInt("spewRadius", "32");

	idDict args;
	args.Clear();
	args.SetVector("origin", itemCenter);
	args.SetVector("mins", idVec3(-radius, -radius, -radius));
	args.SetVector("maxs", idVec3(radius, radius, radius));

	idTrigger_gascloud* spewTrigger;
	args.Set("spewParticle", spawnArgs.GetString("spewParticle", "gascloud01.prt"));
	args.SetInt("spewLifetime", spawnArgs.GetInt("spewLifetime", "9000"));	
	spewTrigger = (idTrigger_gascloud *)gameLocal.SpawnEntityType(idTrigger_gascloud::Type, &args);

	
	
}
#include "framework/DeclEntityDef.h"

#include "Fx.h"
#include "bc_ftl.h"
#include "bc_skullsaver.h"
#include "bc_meta.h"
#include "bc_lifeboat.h"

const int BOAT_SPEED = 1024;
const int BOAT_LENGTH = 64;


const int THRUSTER_ANIMTIME = 2000;
const int DESPAWN_MAXTIME = 3000;

const int PUSHAWAY_FORCE = 1024;

const int IMPACT_RADIUS = 256;
const int IMPACT_FORCE = 128;

const int GOODIE_BOUNDSRADIUS = 8;


#define THRUST_UPDATETIME 300

#define AMOUNT_TO_ADD_TO_FTL_COUNTDOWN 30000


const idEventDef EV_LaunchPod("LaunchPod", "vv");



CLASS_DECLARATION(idMoveable, idLifeboat)
	EVENT(EV_Touch, idLifeboat::Event_Touch)
	EVENT(EV_LaunchPod, idLifeboat::Event_LaunchPod)
END_CLASS


//TODO: do distance check to destination, make it stop if it hasn't already after a few seconds.
//TODO: special noise on last 10 seconds.
//TODO: do a little gui popup ("5 souls onboard") when it leaves



const float LIGHT_RADIUS = 80.0f;

idLifeboat::idLifeboat(void)
{
	memset(&boatlight, 0, sizeof(boatlight));
	boatlightHandle = -1;
}

idLifeboat::~idLifeboat(void)
{
	StopSound(SND_CHANNEL_ANY, false);

	if (tractorbeam)
		delete tractorbeam;

	if (tractorbeamTarget)
		delete tractorbeamTarget;

	if (boatlightHandle != -1)
		gameRenderWorld->FreeLightDef(boatlightHandle);

	if (shopMonitor != NULL)
	{
		delete shopMonitor;
	}
}

void idLifeboat::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // lifeboat_state_t state

	savefile->WriteVec3( targetDirection ); // idVec3 targetDirection
	savefile->WriteVec3( despawnPosition ); // idVec3 despawnPosition

	savefile->WriteInt( thrustTimer ); // int thrustTimer

	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( lastSecondDisplay ); // int lastSecondDisplay

	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke

	savefile->WriteObject( animatedThrusters ); // idAnimated* animatedThrusters

	savefile->WriteBool( damageSmokeDone ); // bool damageSmokeDone

	savefile->WriteInt( damageTimer ); // int damageTimer

	savefile->WriteRenderLight( boatlight ); // renderLight_t boatlight
	savefile->WriteInt( boatlightHandle ); // int boatlightHandle

	savefile->WriteInt( storingCount ); // int storingCount

	savefile->WriteInt( bodypullTimer ); // int bodypullTimer

	savefile->WriteObject( tractorbeam ); // idBeam* tractorbeam
	savefile->WriteObject( tractorbeamTarget ); // idBeam* tractorbeamTarget
	savefile->WriteObject( tractorPtr ); // idEntityPtr<idEntity> tractorPtr

	savefile->WriteMat3( displayAngle ); // idMat3 displayAngle

	savefile->WriteBool( hasTractorBeam ); // bool hasTractorBeam

	savefile->WriteObject( shopMonitor ); // idEntity * shopMonitor
	savefile->WriteInt( shopState ); // int shopState

	savefile->WriteInt( lifeboatStayTime ); // int lifeboatStayTime
}

void idLifeboat::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( (int&)state ); // lifeboat_state_t state

	savefile->ReadVec3( targetDirection ); // idVec3 targetDirection
	savefile->ReadVec3( despawnPosition ); // idVec3 despawnPosition

	savefile->ReadInt( thrustTimer ); // int thrustTimer

	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( lastSecondDisplay ); // int lastSecondDisplay

	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke

	savefile->ReadObject( CastClassPtrRef(animatedThrusters) ); // idAnimated* animatedThrusters

	savefile->ReadBool( damageSmokeDone ); // bool damageSmokeDone

	savefile->ReadInt( damageTimer ); // int damageTimer

	savefile->ReadRenderLight( boatlight ); // renderLight_t boatlight
	savefile->ReadInt( boatlightHandle ); // int boatlightHandle
	if ( boatlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( boatlightHandle, &boatlight );
	}

	savefile->ReadInt( storingCount ); // int storingCount

	savefile->ReadInt( bodypullTimer ); // int bodypullTimer

	savefile->ReadObject( CastClassPtrRef(tractorbeam) ); // idBeam* tractorbeam
	savefile->ReadObject( CastClassPtrRef(tractorbeamTarget) ); // idBeam* tractorbeamTarget
	savefile->ReadObject( tractorPtr ); // idEntityPtr<idEntity> tractorPtr

	savefile->ReadMat3( displayAngle ); // idMat3 displayAngle

	savefile->ReadBool( hasTractorBeam ); // bool hasTractorBeam

	savefile->ReadObject( shopMonitor ); // idEntity * shopMonitor
	savefile->ReadInt( shopState ); // int shopState

	savefile->ReadInt( lifeboatStayTime ); // int lifeboatStayTime
}

void idLifeboat::Spawn( void )
{
	lifeboatStayTime = spawnArgs.GetInt("staytime", "45000");

	state = INITIALIZING;
	targetDirection = vec3_zero;
	thrustTimer = 0;
	stateTimer = 0;
	lastSecondDisplay = 0;
	fl.takedamage = false;
	damageSmokeDone = false;
	animatedThrusters = NULL;
	//lastDamageFXTime = 0;
	damageTimer = 0;
	idleSmoke = NULL;
	storingCount = 0;
	isFrobbable = true; // SW: Necessary to allow player to interact with GUIs with +frob

	bodypullTimer = 0;

	GetPhysics()->SetGravity(idVec3(0, 0, 0));


	//Spawn the ambient glow.	
	boatlight.shader = declManager->FindMaterial("lights/pointlight_exterior", false);
	boatlight.pointLight = true;
	boatlight.lightRadius[0] = LIGHT_RADIUS;
	boatlight.lightRadius[1] = LIGHT_RADIUS;
	boatlight.lightRadius[2] = LIGHT_RADIUS;
	boatlight.shaderParms[0] = 0.25f; // R
	boatlight.shaderParms[1] = 0.175f; // G
	boatlight.shaderParms[2] = 0.0f; // B
	boatlight.shaderParms[3] = 1.0f; // ???
	boatlight.noShadows = true;
	boatlight.isAmbient = true;
	boatlight.axis = mat3_identity;
	boatlightHandle = gameRenderWorld->AddLightDef(&boatlight);

	


	//tractor beam.
	idDict args;

	//Laser endpoint.
	tractorbeamTarget = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	tractorbeamTarget->BecomeActive(TH_PHYSICS);
	//tractorbeamTarget->SetOrigin(GetPhysics()->GetOrigin());	

	//Laser startpoint.
	args.Clear();
	args.Set("target", tractorbeamTarget->name.c_str());
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.SetBool("start_off", false);
	args.Set("width", spawnArgs.GetString("laserwidth", "16"));
	args.Set("skin", spawnArgs.GetString("laserskin", "skins/beam_tractor"));
	tractorbeam = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
	tractorbeam->Hide(); //For some reason start_off causes problems.... so spawn it normally, and then do a hide() here. oh well :/
	

	tractorPtr = NULL;
	displayAngle = mat3_identity;

	BecomeActive(TH_THINK);

	hasTractorBeam = spawnArgs.GetBool("tractorbeam");

	shopMonitor = NULL;
	shopState = SHOP_UNSPAWNED;
}

void idLifeboat::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	if (!fl.takedamage)
		return;

	fl.takedamage = false;
	StartSound("snd_deathalarm", SND_CHANNEL_ANY, 0, false, NULL);
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_damaged")));
	StartTakeoff();
}

void idLifeboat::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	if (!fl.takedamage)
		return;

	//if (inflictor != NULL)
	//{
	//	if (inflictor->IsType(idActor::Type) || inflictor->IsType(idSkullsaver::Type))
	//		return;
	//}

	
	//BC 3-11-2025: ignore damage if player is inside the pod.
	if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerIsCurrentlyInCatpod())
		return;


	idMoveable::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	//if (gameLocal.time > lastDamageFXTime)
	//{
	//	lastDamageFXTime = gameLocal.time + 300;
	//	gameLocal.DoParticle("machine_damaged_smokelight_1sec.prt", GetPhysics()->GetOrigin());
	//}


	if (!damageSmokeDone)
	{
		damageSmokeDone = true;
		idleSmoke->SetModel("machine_damaged_smokeheavy3.prt");
	}
}

void idLifeboat::Think( void )
{

	if (!tractorbeam->IsHidden())
		tractorbeam->Hide(); //for some reason this needs to be called........

	if (state == THRUSTING)
	{
		GetPhysics()->SetAxis(displayAngle);

		if (gameLocal.time > thrustTimer)
		{
			idVec3 force;
			idVec3 ramDir;
		
			
			force = (targetDirection * BOAT_SPEED);
			this->GetPhysics()->SetLinearVelocity(force);

			thrustTimer = gameLocal.time + THRUST_UPDATETIME;
		}

		RunPhysics();
	}
	else if (state == LANDED)
	{
		if (lifeboatStayTime > 0) //only do timer stuff if the staytime is longer than zero. Otherwise, just stay forever. Stay forever is behavior for the scripted pod.
		{
			int remainingTimeMS = stateTimer - gameLocal.time;

			int remainingSeconds = remainingTimeMS / 1000;
			if (lastSecondDisplay != remainingSeconds)
			{
				lastSecondDisplay = remainingSeconds;
				StartSound("snd_tick", SND_CHANNEL_ANY, 0, false, NULL);
				Event_SetGuiParm("gui_parm0", idStr::Format("%d", remainingSeconds));

				float lerp = remainingTimeMS / (float)lifeboatStayTime;
				lerp = idMath::ClampFloat(0, 1, lerp);
				Event_SetGuiFloat("timerLerp", lerp);
			}

			if (remainingTimeMS <= 0)
			{
				//time to blast off.
				StartTakeoff();
			}
		}

		Think_Landed();
	}
	else if (state == EXITWARMUP)
	{
		if (gameLocal.time >= stateTimer)
		{
			fl.takedamage = false;

			state = EXITING;
			GetPhysics()->Activate();
			thrustTimer = 0;
			stateTimer = gameLocal.time + DESPAWN_MAXTIME;

			if (idleSmoke)
			{
				idleSmoke->PostEventMS(&EV_Remove, 0);
				idleSmoke = nullptr;
			}

			StartSound("snd_moving", SND_CHANNEL_AMBIENT, 0, false, NULL);
			gameLocal.DoParticle("smoke_ring08.prt", GetPhysics()->GetOrigin());
		}
	}
	else if (state == EXITING)
	{
		//Push it back toward it original spawnposition.
		if (gameLocal.time > thrustTimer)
		{
			idVec3 force;
			idVec3 ramDir;

			ramDir = despawnPosition - this->GetPhysics()->GetOrigin();
			ramDir.NormalizeFast();
			force = (ramDir * BOAT_SPEED);
			this->GetPhysics()->SetLinearVelocity(force);

			thrustTimer = gameLocal.time + THRUST_UPDATETIME;
		}

		//TODO: do a backup distance check.

		//If the pod just .... doesn't hit the skybox for some reason, have a timeout timer that just despawns the pod after a while.
		if (gameLocal.time >= stateTimer)
		{
			//Despawn the pod.
			Despawn();
		}

		RunPhysics();
	}
	
	//Update light position.
	if (boatlightHandle != -1)
	{
		boatlight.origin = GetPhysics()->GetOrigin();
		gameRenderWorld->UpdateLightDef(boatlightHandle, &boatlight);
	}

	Present();
}

void idLifeboat::Think_Landed()
{
	//make bodies gravitate toward the lifeboat.
	UpdateBodyPull();

	//Update the tractor beam visual.
	bool shouldHideBeam = true;
	if (tractorPtr.IsValid())
	{
		if (tractorPtr.GetEntity() != NULL)
		{
			tractorbeam->SetOrigin(tractorPtr.GetEntity()->GetPhysics()->GetOrigin());
			shouldHideBeam = false;
		}
	}

	if (shouldHideBeam)
		tractorbeam->Hide();
	else if (tractorbeam->IsHidden())
		tractorbeam->Show();

	int remainingTimeMS = stateTimer - gameLocal.time;
	int remainingSeconds = remainingTimeMS / 1000;
	UpdateShopMonitor(remainingSeconds);

	Event_SetGuiInt("gui_parm1", storingCount);
}

void idLifeboat::Despawn()
{
	if (state == DORMANT)
		return;

	//Pod has successfully left the map.
	//TODO: Confirm the bodies inside the pod.
	
	state = DORMANT;	
	gameLocal.DoParticle("sparkle_lifepod.prt", GetPhysics()->GetOrigin());
	StopSound(SND_CHANNEL_ANY, false);
	StartSound("snd_sparkle", SND_CHANNEL_ANY, 0, false, NULL);

	if (animatedThrusters)
	{
		animatedThrusters->StopSound(SND_CHANNEL_ANY, false);
	}


	//Delete self.
	Hide();
	physicsObj.SetContents(0);
	PostEventMS(&EV_Remove, 0);
}

//Call this when the lifeboat is spawned.
void idLifeboat::Launch(idVec3 _targetPos)
{
	//Direction that we want the pod to thrust into the map.
	targetDirection = _targetPos - GetPhysics()->GetOrigin();
	targetDirection.Normalize();

	//Trajectory it takes when exiting the map.
	idVec3 exitTrajectoryDir = GetPhysics()->GetOrigin() - _targetPos;
	exitTrajectoryDir.Normalize();
	despawnPosition = GetPhysics()->GetOrigin() + (exitTrajectoryDir * 512); //We need to push the despawn position BEHIND the skybox, so that the pod will attempt to dig into the skybox and thus call the collision function.


	state = THRUSTING;
	GetPhysics()->Activate();

	StartSound("snd_moving", SND_CHANNEL_AMBIENT, 0, false, NULL);

	gameLocal.DoParticle("sparkle_lifepod.prt", GetPhysics()->GetOrigin());
	StartSound("snd_sparkle", SND_CHANNEL_ANY, 0, false, NULL);

	displayAngle = GetPhysics()->GetAxis();
	
	isFrobbable = true; // SW 10th March 2025
}


//Detect when touch the ship, or touch the skybox.
bool idLifeboat::Collide(const trace_t &collision, const idVec3 &velocity)
{
	idAngles particleDir;
	idVec3 forwardDir;	

	//TODO: detect when body touches this?

	if (state == THRUSTING)
	{
		if (collision.c.material)
		{
			if (collision.c.material->GetSurfaceFlags() >= 256)
			{
				//Hit the SKY. Make lifepod immediately disappear.
				Despawn();
				return true;
			}
		}

		//if (gameLocal.time > damageTimer)
		//{
		//	damageTimer = gameLocal.time + 200; //clamp damage every XX milliseconds.
		//	gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, "damage_explosion");
		//}

		//Detect if it hit something damageable.
		if (collision.c.entityNum <= MAX_GENTITIES - 2 && collision.c.entityNum >= 0)
		{
			if (gameLocal.entities[collision.c.entityNum]->fl.takedamage)
			{
				//If it's an actor, then obliterate it.
				if (gameLocal.entities[collision.c.entityNum]->IsType(idActor::Type))
				{
					if (gameLocal.entities[collision.c.entityNum]->health > 0)
					{
						//If they're alive, then inflict damage on them.
						gameLocal.entities[collision.c.entityNum]->Damage(this, this, vec3_zero, "damage_lifeboat", 1.0f, 0);
					}
				}

				if (gameLocal.entities[collision.c.entityNum]->spawnArgs.GetBool("destroyed_by_lifeboat", "0") && gameLocal.entities[collision.c.entityNum]->health > 0)
				{
					gameLocal.entities[collision.c.entityNum]->Damage(gameLocal.entities[collision.c.entityNum], gameLocal.entities[collision.c.entityNum], vec3_zero, "damage_lifeboat", 1.0f, 0);
				}

				if (gameLocal.entities[collision.c.entityNum]->IsType(idItem::Type) || gameLocal.entities[collision.c.entityNum]->IsType(idActor::Type) || gameLocal.entities[collision.c.entityNum]->IsType(idMoveable::Type))
				{
					idVec3 pushDir;
					pushDir = gameLocal.entities[collision.c.entityNum]->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
					pushDir.Normalize();

					gameLocal.entities[collision.c.entityNum]->GetPhysics()->ApplyImpulse(0, gameLocal.entities[collision.c.entityNum]->GetPhysics()->GetOrigin(), pushDir * PUSHAWAY_FORCE * gameLocal.entities[collision.c.entityNum]->GetPhysics()->GetMass());
					//gameLocal.entities[collision.c.entityNum]->GetPhysics()->SetLinearVelocity(pushDir * PUSHAWAY_FORCE);
					return false; //Just keep thrusting forward.
				}		
				
			}
		}

		//Because collision detection detects the specific point of collision, we need to do a broader wider check of things to damage.

		state = LANDED;

		if (spawnArgs.GetBool("takedamage", "1"))
			fl.takedamage = true;
		else
			fl.takedamage = false;



		forwardDir = GetPhysics()->GetAxis().ToAngles().ToForward();
		particleDir = forwardDir.ToAngles();
		particleDir.pitch -= 90;

		GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * 8)); //Dig into the ground a little.

		StopSound(SND_CHANNEL_AMBIENT, false);
		StartSound("snd_impact", SND_CHANNEL_ANY, 0, false, NULL);
		idEntityFx::StartFx(spawnArgs.GetString("fx_collide"), collision.endpos, particleDir.ToMat3());
		gameLocal.ProjectDecal(collision.c.point, -collision.c.normal, 8.0f, true, 300, "textures/decals/scorch1024_faded");

		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_normal")));

		stateTimer = gameLocal.time + lifeboatStayTime;

		GetPhysics()->PutToRest(); //Turn off physics.

		//Create idle smoke from the rocket.
		if (1)
		{
			idDict args;
			idVec3 forwardDir;
			idAngles smokeAng = GetPhysics()->GetAxis().ToAngles();
			smokeAng.pitch -= 90;

			args.Clear();
			args.Set("model", "smoke_trail01.prt");
			args.Set("start_off", "0");
			idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, NULL, NULL);
			idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + (forwardDir * -BOAT_LENGTH / 2));
			idleSmoke->SetAngles(smokeAng);
		}		

		tractorbeamTarget->GetPhysics()->SetOrigin(this->GetPhysics()->GetOrigin());



		//Push away things that are at the impact zone.
		idEntity		*entityList[MAX_GENTITIES];
		int entityCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), IMPACT_RADIUS, entityList, MAX_GENTITIES);
		for (int i = 0; i < entityCount; i++)
		{
			idEntity *ent = entityList[i];

			if (!ent)
				continue;

			if (ent->IsHidden()) //only want dead bodies.
				continue;

			if (ent->IsType(idItem::Type) || ent->IsType(idActor::Type) || ent->IsType(idMoveable::Type))
			{
				//give it a physics push.

				//make sure we have LOS
				trace_t tr;
				gameLocal.clip.TracePoint(tr, this->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), MASK_SOLID, this);
				if (tr.fraction < 1.0f)
					continue;

				idVec3 pushDir = ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin();
				pushDir.NormalizeFast();
				idVec3 pushForce = IMPACT_FORCE * pushDir * ent->GetPhysics()->GetMass();
				ent->GetPhysics()->ApplyImpulse(0, ent->GetPhysics()->GetAbsBounds().GetCenter(), pushForce);				
			}
		}

		OnLanded();

		SetupShopMonitor();
	}
	else if (state == EXITING)
	{
		//Wait for lifepod to hit skybox.
		//THIS..... doesn't work for some reason :(
		if (collision.c.material)
		{
			if (collision.c.material->GetSurfaceFlags() >= 256)
			{
				//Hit the SKY. Make lifeboat immediately disappear.
				Despawn();
				return true;
			}
		}
	}

	return true;
}

void idLifeboat::SetupShopMonitor()
{
	if (!spawnArgs.GetBool("shopboat", "0"))
	{
		return;
	}

	//This is a shop boat.
	//Determine what place has the most clearance.
	//We check 4 locations, in order of priority:
	// LEFT
	// RIGHT
	// DOWN
	// BEHIND

	//We intentionally do not check the UP position. When pod attaches to hull horizontally (like a thumbtack on a corkboard wall),
	//it's difficult to accommodate room for the player's standing height to awkwardly stand on the lifeboat while reading monitor.

	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//These are the positions we check for clearance. (xyz = forward, right, up)
	const int ARRAYSIZE = 9;
	idVec3 checkPositions[] =
	{
		//Left position.
		idVec3(0,-55,0),

		//Right pos.
		idVec3(0,55,0),

		//Down pos.
		idVec3(0,0,-55),

		//Down-behind pos.
		idVec3(-32,0,-55),

		//Left-behind position.
		idVec3(-32,-55,0),

		//Right-behind pos.
		idVec3(-32,55,0),

		//Left-behind-up position.
		idVec3(-32,-55,16),

		//Right-behind-up pos.
		idVec3(-32,55,16),

		//Behind pos.
		idVec3(-55,0,0),
	};

	idBounds playerBound = idBounds(idVec3(-24, -24, -64), idVec3(24, 24, 16));

	for (int i = 0; i < ARRAYSIZE; i++)
	{
		idVec3 candidatePos = GetPhysics()->GetOrigin() + (forward * checkPositions[i].x) + (right * checkPositions[i].y) + (up * checkPositions[i].z);
		
		trace_t tr;
		gameLocal.clip.TraceBounds(tr, candidatePos, candidatePos, playerBound, MASK_SHOT_RENDERMODEL, NULL);
		
		if (tr.fraction >= 1)
		{
			//Clearance found. Success.
			SpawnShopMonitor(candidatePos);
			return;
		}
	}

	//Failed to find a spawn position.
	//Fallback: just spawn it behind the pod, skip the clearance check. This is probably (?) the safest place to force the monitor to spawn.
	SpawnShopMonitor(GetPhysics()->GetOrigin() + (forward * -55));
}

void idLifeboat::SpawnShopMonitor(idVec3 pos)
{
	idDict args;
	args.Set("classname", "env_supplystation");
	//args.Set("model", "models/objects/monitor/monitor_zerog.ase");
	args.SetVector("origin", pos);
	args.Set("podowner", this->GetName());
	//args.SetMatrix("rotation", spawnAngle.ToMat3());
	//args.Set("gui", "guis/game/supplyboat_shop.gui");
	gameLocal.SpawnEntityDef(args, &shopMonitor);

	shopState = SHOP_IDLE;
}

void idLifeboat::UpdateShopMonitor(int secondsRemaining)
{
	if (shopState != SHOP_IDLE || shopMonitor == NULL)
	{
		return;
	}

	shopMonitor->Event_SetGuiInt("gui_parm0", secondsRemaining);
}

void idLifeboat::StartTakeoff()
{
	idDict args;

	if (state == EXITWARMUP || state == EXITING || state == DORMANT) //if button somehow gets pressed more than once, only acknowledge the first press.
		return;

	if (shopState == SHOP_IDLE && shopMonitor != NULL)
	{
		gameLocal.DoParticle("smoke_ring17.prt", GetPhysics()->GetOrigin());
		shopMonitor->Hide();
	}

	state = EXITWARMUP;
	Event_SetGuiParm("gui_parm0", "0");	

	//Spawn the thruster model.
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", "model_retrothrusters");
	animatedThrusters = (idAnimated *)gameLocal.SpawnEntityType(idAnimated::Type, &args);
	animatedThrusters->SetAxis(GetPhysics()->GetAxis());
	animatedThrusters->Bind(this, true);
	animatedThrusters->Event_PlayAnim("deploy", 0, false);

	stateTimer = gameLocal.time + THRUSTER_ANIMTIME;

	tractorbeam->Hide();

	isFrobbable = false; // SW 10th March 2025: don't let the player frob it during takeoff warmup, it's too messy

	OnTakeoff();
}

//When the gui is interacted with.
void idLifeboat::DoGenericImpulse(int index)
{
	if (index == 0 && state == LANDED)
	{
		//Take off now.
		StartTakeoff();
	}
}

void idLifeboat::Event_Touch(idEntity *other, trace_t *trace)
{
	if (!hasTractorBeam)
		return;

	//Check if it can be rescued.
	//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->IsEscalationMaxed())
	//{
	//	return; //Currently in FTL chargeup phase.
	//}

	if (other->spawnArgs.GetInt("lifepod_storeable") > 0)
	{
        //Tell the HUD to do its stuff.


        //count how many enemies remain in the level.
        //int enemiesRemaining = 0;
        //for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
        //{
        //    if (!entity || entity->team != TEAM_ENEMY || entity == other)
        //        continue;
		//
		//	//TODO: ignore robots, turrets.
		//
        //    enemiesRemaining++;
        //}
        ////TODO: do a special message if  enemiesRemaining <= 0
        //gameLocal.GetLocalPlayer()->hud->SetStateString("eliminations_remaining", idStr::Format("PIRATES REMAINING: %d", enemiesRemaining));        
        
		//store the body.
		gameLocal.DoParticle("sparkle_body.prt", other->GetPhysics()->GetOrigin());
		StartSound("snd_jingle", SND_CHANNEL_ANY, 0, false, NULL);
		other->Hide();
		other->PostEventMS(&EV_Remove, 0);		//Remove the object.
		
		if (storingCount <= 0)
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_success")));

			//make light turn blue.
			boatlight.shaderParms[0] = 0;
			boatlight.shaderParms[1] = .175f;
			boatlight.shaderParms[2] = 1;			
		}

		storingCount++;		

        //if it is a SKULLSAVER then we need to do some cleanup work.
        if (other->IsType(idSkullsaver::Type))
        {
			static_cast<idSkullsaver *>(other)->StoreSkull();
        }
		
		//TODO: make a record that the body has been stored.


		//StartTakeoff(); //Leave immediately.


		//Add time to the FTL countdown.
		//idMeta *meta = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity());
		//if (meta)
		//{
		//	idFTL *ftl = static_cast<idFTL *>(meta->GetFTLDrive.GetEntity());
		//	if (ftl)
		//	{
		//		ftl->AddToCountdown(AMOUNT_TO_ADD_TO_FTL_COUNTDOWN);
		//	}
		//}


		//Spawn goodies.

		// SW: Testing removing the oxygen system, just spawn an armour plate
		for (int i = 0; i < 1; i++)
		{
			idVec3 spawnPosition = FindGoodieSpawnposition();
			if (spawnPosition == vec3_zero)
				continue;

			idDict args;
			args.Clear();
			args.SetVector("origin", spawnPosition);

			
			if (/*i <= 0*/ true)
			{
				//spawn armor plate.
				args.Set("classname", "moveable_armorplate");
			}
			else
			{
				//last item is oxygen.
				// SW: Commented out since we're testing removing the oxygen system
				//args.Set("classname", "moveable_oxygenpack");
			}

			idEntity *goodieEnt;
			gameLocal.SpawnEntityDef(args, &goodieEnt);

			if (goodieEnt)
			{
				//Nudge the item toward player's position.
				idVec3 dirToPlayer = gameLocal.GetLocalPlayer()->GetEyePosition() - spawnPosition;
				dirToPlayer.NormalizeFast();
				goodieEnt->GetPhysics()->SetLinearVelocity(dirToPlayer * gameLocal.random.RandomInt(32,64));
				goodieEnt->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomInt(-64, 64), gameLocal.random.RandomInt(-64, 64), 0));
			}
		}
	}
}

//Spawn an item.
void idLifeboat::SpawnGoodie(const char *defName)
{
	idVec3 spawnPosition = FindGoodieSpawnposition();
	if (spawnPosition == vec3_zero)
		return;

	idDict args;	
	args.SetVector("origin", spawnPosition);
	args.Set("classname", defName);
	idEntity *goodieEnt = NULL;
	gameLocal.SpawnEntityDef(args, &goodieEnt);

	if (goodieEnt)
	{
		//Nudge the item toward player's position.
		idVec3 dirToPlayer = gameLocal.GetLocalPlayer()->GetEyePosition() - spawnPosition;
		dirToPlayer.NormalizeFast();
		goodieEnt->GetPhysics()->SetLinearVelocity(dirToPlayer * gameLocal.random.RandomInt(32, 64));
		goodieEnt->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomInt(-64, 64), gameLocal.random.RandomInt(-64, 64), 0));

		StartSound("snd_sparkle", SND_CHANNEL_ANY, 0, false, NULL);
		gameLocal.DoParticle("sparkle_lifepod_hint.prt", spawnPosition);		
	}
}

idVec3 idLifeboat::FindGoodieSpawnposition()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	idList<idVec3> candidateSpots;
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (right * 48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (right * -48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (up * 48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (up * -48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (up * 48) + (right * 48));
	candidateSpots.Append(GetPhysics()->GetOrigin() + (forward * -48) + (up * -48) + (right * -48));
	candidateSpots.Shuffle();

	
	for (int i = 0; i < candidateSpots.Num(); i++)
	{
		trace_t boundTr;
		idVec3 spot = candidateSpots[i];
		gameLocal.clip.TraceBounds(boundTr, spot, spot, idBounds(idVec3(-GOODIE_BOUNDSRADIUS, -GOODIE_BOUNDSRADIUS, -GOODIE_BOUNDSRADIUS), idVec3(GOODIE_BOUNDSRADIUS, GOODIE_BOUNDSRADIUS, GOODIE_BOUNDSRADIUS)), MASK_SHOT_RENDERMODEL, NULL);

		if (boundTr.fraction >= 1)
		{
			return spot;
		}
	}	

	return vec3_zero; //no valid clearance found.
}

void idLifeboat::UpdateBodyPull()
{
	if (gameLocal.time < bodypullTimer || !hasTractorBeam)
		return;

	#define PULL_TIMEINTERVAL 300
	#define PULLRADIUS 512

	bodypullTimer = gameLocal.time + PULL_TIMEINTERVAL;

	float closestDistance = 99999;
	int closestIndex = -1;

	int entityCount;
	idEntity *entityList[MAX_GENTITIES];
	entityCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), PULLRADIUS, entityList, MAX_GENTITIES);
	for (int i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];

		if (!ent)
			continue;

		if (ent->IsHidden()) //only want dead bodies.
			continue;

		if ((ent->IsType(idActor::Type) && ent->health <= 0) || ent->IsType(idSkullsaver::Type))
		{
			//allow these entity types.
		}
		else
		{
			continue;
		}

		//Check if it's being carried by the player.
		//if (gameLocal.GetLocalPlayer()->bodyDragger.isDragging)
		//{
		//	if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.IsValid())
		//	{
		//		if (gameLocal.GetLocalPlayer()->bodyDragger.dragEnt.GetEntityNum() == ent->entityNumber)
		//		{
		//			//Ignore if player is holding onto body.
		//			continue;
		//		}
		//	}
		//}

		trace_t bodyTr;
		gameLocal.clip.TracePoint(bodyTr, this->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), MASK_SOLID, ent);

		if (bodyTr.fraction < 1.0f)
			continue; //do not have LOS.

		//have LOS. Determine whether it's the closest body.
		float curDist = (this->GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).LengthFast();

		if (curDist < closestDistance)
		{
			closestDistance = curDist;
			closestIndex = ent->entityNumber;
		}
	}

	if (closestIndex >= 0)
	{
		//have LOS. Pull the body.
		#define PULLSPEED 6000
		idVec3 dirToPod = this->GetPhysics()->GetOrigin() - gameLocal.entities[closestIndex]->GetPhysics()->GetOrigin();
		dirToPod.NormalizeFast();

		
		if (!gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(gameLocal.entities[closestIndex])) //if player is holding it, then don't pull it.
		{
			gameLocal.entities[closestIndex]->GetPhysics()->ApplyImpulse(0, gameLocal.entities[closestIndex]->GetPhysics()->GetAbsBounds().GetCenter(), dirToPod * PULLSPEED);

			if (gameLocal.entities[closestIndex]->IsType(idSkullsaver::Type))
			{
				//if it's a skullsaver, then we turn off the conveyor logic. We don't want the tractor beam to be fighting against the skullsaver respawn conveyor.
				static_cast<idSkullsaver *>(gameLocal.entities[closestIndex])->ResetConveyTime();
			}
		}
		

		//Check distance.
		#define STORAGE_PROXIMITY 64
		float distance = (gameLocal.entities[closestIndex]->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
		if (distance <= STORAGE_PROXIMITY)
		{
			//store the person
			Event_Touch(gameLocal.entities[closestIndex], NULL);
			tractorPtr = NULL;
			return;
		}
		else
		{
			tractorPtr = gameLocal.entities[closestIndex];
		}
	}
	else
	{
		tractorPtr = NULL;
	}
}


bool idLifeboat::DoFrob(int index, idEntity * frobber)
{
	//Deposit the cats.
	
	
	return true;
}

//This is for the script call.
void idLifeboat::Event_LaunchPod(const idVec3 &originPos, const idVec3 &destinationPos)
{
	SetOrigin(originPos);

	//Set pod orientation.
	idVec3 dirToTarget = destinationPos - originPos;
	dirToTarget.Normalize();
	SetAxis(dirToTarget.ToMat3());

	Launch(destinationPos);
}

// SW 10th March 2025:
// The player has to juggle a lot of items inside the cat pod and sometimes they might accidentally exit prematurely
// (or not be able to carry everything they want)
// To be kind to the player, we eject items into the vacuum of space here.
void idLifeboat::EjectItems(idList<idMoveableItem*> items)
{
	idVec3 itemPos, ejectVelocity;
	idVec3 lifeboatCenter = this->GetPhysics()->GetAbsBounds().GetCenter();
	int count = items.Num();
	for (int i = 0; i < count; i++)
	{
		itemPos = FindGoodieSpawnposition();

		items[i]->Unbind();
		items[i]->Teleport(itemPos, items[i]->GetPhysics()->GetAxis().ToAngles(), NULL);
		if (itemPos == vec3_zero)
		{
			gameLocal.Warning("idLifeboat::EjectItems: item was ejected at vec3_zero");
		}
		else
		{
			ejectVelocity = (itemPos - lifeboatCenter).Normalized() * 100;
			items[i]->GetPhysics()->SetLinearVelocity(ejectVelocity);
			items[i]->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomFloat() * 100, gameLocal.random.RandomFloat() * 100, gameLocal.random.RandomFloat() * 100));
		}
		
	}
}

void idLifeboat::OnLanded()
{
}

void idLifeboat::OnTakeoff()
{
}

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "ai/AI.h"
#include "idlib/LangDict.h"

#include "framework/DeclEntityDef.h"
#include "SmokeParticles.h"
#include "Misc.h"

#include "bc_ftl.h"
#include "bc_meta.h"
#include "bc_lostandfound.h"
#include "bc_skullsaver.h"
#include "bc_catpod_interior.h"

//const int PEPPERSPEW_FALLSPEED_THRESHOLD = 200; //speed at which pepperbag must hit surface before spewing pepper.
//const int PEPPER_LIFETIME = 9000;

const int SKULL_JUMPMINTIME = 500;
const int SKULL_JUMPRANDOMTIME = 1500;

const float SKULL_JUMPPOWER = 128;

const float SKULL_SPAWNJUMPPOWER = 384;
const float SKULL_SPAWNJUMPPOWER_ZEROG = 16;

const int SKULL_FAILTIMER = 5000; //if fail to respawn, wait this long before attempting again.
const int SKULL_FAILTIMER_VARIANCE = 3000; //random variance

const int YELL_INITIAL_DELAY = 5000; //when skullsaver first spawns, have an initial delay. This is so it doesn't just immediately alert everyone nearby.
const int YELL_MINTIME = 8000;
const int YELL_RANDOMVARIATIONTIME = 6000;

const int RANT_MISSIONTIMER = 360000; //how many MS before the rant is allowed to happen.
const int RANT_CHANCE = 20; // probability chance out of XX that rant can happen.

//Debug for making skull yell often. DO NOT CHECK IN THESE VALUES!
//const int YELL_INITIAL_DELAY = 1000; //when skullsaver first spawns, have an initial delay. This is so it doesn't just immediately alert everyone nearby.
//const int YELL_MINTIME = 3000;
//const int YELL_RANDOMVARIATIONTIME = 100;

const idStr REGENERATIONTIME = "5000";


const int JUMP_CLOSEDISTANCE_THRESHOLD = 64; //when skullsaver less than this distance to player, it will attempt to jump AWAY from player. if further than this, it will attempt to jump CLOSER to player.

const int ENEMY_FROBINDEX = 7; //Determines when an enemy frobs the skullsaver (to pick it up). This has to match 'frobindex' in the interestpoint definition.

const int CONVEY_RESPAWNBEACON_OFFSET = 68; //how high off the respawn beacon do we float to.
const float CONVEY_PATH_WIDTH = 8.0f;

const int NINAREPLY_TIMEGAP = 150; //how many milliseconds between the skull noise + nina's reply.

CLASS_DECLARATION(idMoveableItem, idSkullsaver)

END_CLASS

idCVar g_skullsaver_debug( "g_debugskullsaver", "0", CVAR_CHEAT | CVAR_SYSTEM | CVAR_INTEGER, "viz skullsaver spawn and pathing");


idSkullsaver::idSkullsaver(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	skullsaverNode.SetOwner(this);
	skullsaverNode.AddToEnd(gameLocal.skullsaverEntities);

	destinationArea = 0;
	isLowHealthState = false;
	damageParticleFlyTime = 0;
	damageParticle = NULL;

	waitingForNinaReply = false;
	ninaReplyTimer = 0;
}

idSkullsaver::~idSkullsaver(void)
{
	StopSound(SND_CHANNEL_ANY);

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	skullsaverNode.Remove();

	for (int i = 0; i < CONVEY_MAX_PATH; i++)
	{
		if (this->pathBeamTarget[i])
		{
			this->pathBeamTarget[i]->PostEventMS(&EV_Remove, 0);
			pathBeamTarget[i] = nullptr;
		}
		if (this->pathBeamOrigin[i])
		{
			this->pathBeamOrigin[i]->PostEventMS(&EV_Remove, 0);
			pathBeamOrigin[i] = nullptr;
		}
	}
}

void idSkullsaver::Spawn(void)
{
	//GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	//GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	this->fl.takedamage = spawnArgs.GetBool("takedamage", "1");
	nextJumpTime = gameLocal.time + 100;

	lifetime = gameLocal.time + (spawnArgs.GetInt("lifetime", "30") * 1000);
	state = SKULLSAVE_JUSTSPAWNED;	

	

	idleSmoke = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, "skullsaver_idle.prt"));	
	idleSmokeFlyTime = gameLocal.time + 1;

	springPartner = NULL;

	//Make purple line come out of it.
	showItemLine = true;
	itemLineColor.x = .7f;
	itemLineColor.y = 0;
	itemLineColor.z = 1;

	idDict args;
	args.Clear();
	args.Set("model", "sound_burst.prt");
	args.Set("start_off", "1");
	soundwaves = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	soundwaves.GetEntity()->SetOrigin(GetPhysics()->GetOrigin());
	soundwaves.GetEntity()->Bind(this, true);
	heybarkTimer = gameLocal.time + YELL_INITIAL_DELAY + gameLocal.random.RandomInt(YELL_RANDOMVARIATIONTIME);


	//BC 2-14-2025: regeneration particles.
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_regeneration"));
	args.Set("start_off", "1");
	regnerationParticle = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	regnerationParticle.GetEntity()->SetOrigin(GetPhysics()->GetOrigin());
	regnerationParticle.GetEntity()->Bind(this, true);
	

	conveyorDelayTime = gameLocal.time + spawnArgs.GetInt("convey_delay_time", "2000");
	conveyorRespawnpoint = NULL;
	conveyorTotalMoveTime = 0;
	conveyorStartMoveTime = 0;

	hasStoredSkull = false;

	aas = gameLocal.GetAAS( "aas32_flybot" );

	if ( !aas )
	{
		gameLocal.Warning( "Skullsaver: map does not have aas32_flybot data. Falling back to aas24." );
		aas = gameLocal.GetAAS( "aas24" );
	}

	for ( int i = 0; i < CONVEY_MAX_PATH; i++ )
	{
		args.Clear();
		args.SetVector( "origin", vec3_origin );
		args.SetFloat( "width", CONVEY_PATH_WIDTH );
		this->pathBeamTarget[i] = ( idBeam* )gameLocal.SpawnEntityType( idBeam::Type, &args );

		args.Clear();
		args.Set( "target", pathBeamTarget[i]->name.c_str() );
		args.SetBool( "start_off", false );
		args.SetVector( "origin", vec3_origin );
		args.SetFloat( "width", CONVEY_PATH_WIDTH );
		args.Set( "skin", spawnArgs.GetString("model_beam") );

		this->pathBeamOrigin[i] = ( idBeam* )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->pathBeamOrigin[i]->BecomeActive( TH_PHYSICS );
		this->pathBeamTarget[i]->BecomeActive( TH_PHYSICS );
	}

	//So, there are 2 smoke effects for when it's damaged: one that leaves a trail, and one that's static-bound to the skullsaver.
	//This is the one that's static-bound to the skullsaver.
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_damageparticle2"));
	args.Set("start_off", "1");
	damageEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	damageEmitter.GetEntity()->SetOrigin(GetPhysics()->GetOrigin());
	damageEmitter.GetEntity()->Bind(this, false);

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.
	PostEventMS(&EV_PostSpawn, 0);
}

void idSkullsaver::Event_PostSpawn(void)
{
	lostandfoundMachine = FindLostandfoundMachine();

	//Handle the skullsaver being spawned in outer space.
	UpdateGravity(true);

	//gameLocal.voManager.SayVO(this, "snd_respawn", VO_CATEGORY_BARK);
	StartSound("snd_popoff", SND_CHANNEL_VOICE);
}

void idSkullsaver::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( bodyOwner ); // idEntityPtr<idEntity> bodyOwner
	savefile->WriteObject( springPartner ); // idEntityPtr<idEntity> springPartner

	savefile->WriteInt( nextJumpTime ); // int nextJumpTime
	savefile->WriteInt( lifetime ); // int lifetime

	savefile->WriteInt( state ); // int state

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteParticle( idleSmoke ); // const idDeclParticle * idleSmoke
	savefile->WriteInt( idleSmokeFlyTime ); // int idleSmokeFlyTime

	savefile->WriteInt( heybarkTimer ); // int heybarkTimer

	soundwaves.Save(savefile); // idEntityPtr<idFuncEmitter> soundwaves

	regnerationParticle.Save(savefile); // idEntityPtr<idFuncEmitter> regnerationParticle


	savefile->WriteInt( conveyorDelayTime ); // int conveyorDelayTime
	savefile->WriteObject( conveyorRespawnpoint ); // idEntityPtr<idEntity> conveyorRespawnpoint
	savefile->WriteFloat( conveyorTotalMoveTime ); // float conveyorTotalMoveTime
	savefile->WriteVec3( conveyorStartPosition ); // idVec3 conveyorStartPosition
	savefile->WriteInt( conveyorStartMoveTime ); // int conveyorStartMoveTime
	savefile->WriteVec3( conveyorDestPosition ); // idVec3 conveyorDestPosition
	savefile->WriteVec3( conveyorRespawnPos ); // idVec3 conveyorRespawnPos
	
	// idAAS* aas // regen
	//	aas = gameLocal.GetAAS( "aas32_flybot" );

	//if ( !aas )
	//{
	//	gameLocal.Warning( "Skullsaver: map does not have aas32_flybot data. Falling back to aas24." );
	//	aas = gameLocal.GetAAS( "aas24" );
	//}
	//
	savefile->WriteInt( destinationArea ); // int destinationArea

	SaveFileWriteArray( pathPoints, pathPoints.Num(), WriteVec3 ); // idList<idVec3> pathPoints
	SaveFileWriteArray( pathBeamOrigin, CONVEY_MAX_PATH, WriteObject ); // idBeam* pathBeamOrigin[CONVEY_MAX_PATH]
	SaveFileWriteArray( pathBeamTarget, CONVEY_MAX_PATH, WriteObject ); // idBeam* pathBeamTarget[CONVEY_MAX_PATH]

	savefile->WriteBool( hasStoredSkull ); // bool hasStoredSkull

	savefile->WriteBool( isLowHealthState ); // bool isLowHealthState
	savefile->WriteParticle( damageParticle ); // const idDeclParticle * damageParticle
	savefile->WriteInt( damageParticleFlyTime ); // int damageParticleFlyTime
	damageEmitter.Save(savefile); // idEntityPtr<idFuncEmitter> damageEmitter

	savefile->WriteObject( lostandfoundMachine ); // idEntityPtr<idEntity> lostandfoundMachine

	aiSpawnLocOrig.Save( savefile ); // idEntityPtr<idLocationEntity> aiSpawnLocOrig

	savefile->WriteBool( waitingForNinaReply ); // bool waitingForNinaReply
	savefile->WriteInt( ninaReplyTimer ); // int ninaReplyTimer

	savefile->WriteBool( rantcheckDone ); // bool rantcheckDone
}

void idSkullsaver::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( bodyOwner ); // idEntityPtr<idEntity> bodyOwner
	savefile->ReadObject( springPartner ); // idEntityPtr<idEntity> springPartner

	savefile->ReadInt( nextJumpTime ); // int nextJumpTime
	savefile->ReadInt( lifetime ); // int lifetime

	savefile->ReadInt( state ); // int state

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadParticle( idleSmoke ); // const idDeclParticle * idleSmoke
	savefile->ReadInt( idleSmokeFlyTime ); // int idleSmokeFlyTime

	savefile->ReadInt( heybarkTimer ); // int heybarkTimer

	if (savefile->GetSaveVersion() < SAVEGAME_VERSION_0004) {
		idFuncEmitter* soundwavesTemp = nullptr;
		savefile->ReadObject(CastClassPtrRef(soundwavesTemp)); // idFuncEmitter * soundwaves
		soundwaves = soundwavesTemp;

		idFuncEmitter* regnerationParticleTemp = nullptr;
		savefile->ReadObject(CastClassPtrRef(regnerationParticle)); // idFuncEmitter* regnerationParticle
		regnerationParticle = regnerationParticleTemp;
	} else {
		soundwaves.Restore(savefile);
		regnerationParticle.Restore(savefile);
	}

	
	savefile->ReadInt( conveyorDelayTime ); // int conveyorDelayTime
	savefile->ReadObject( conveyorRespawnpoint ); // idEntityPtr<idEntity> conveyorRespawnpoint
	savefile->ReadFloat( conveyorTotalMoveTime ); // float conveyorTotalMoveTime
	savefile->ReadVec3( conveyorStartPosition ); // idVec3 conveyorStartPosition
	savefile->ReadInt( conveyorStartMoveTime ); // int conveyorStartMoveTime
	savefile->ReadVec3( conveyorDestPosition ); // idVec3 conveyorDestPosition
	savefile->ReadVec3( conveyorRespawnPos ); // idVec3 conveyorRespawnPos

	// idAAS* aas // regen
	aas = gameLocal.GetAAS( "aas32_flybot" );
	if ( !aas )
	{
		gameLocal.Warning( "Skullsaver: map does not have aas32_flybot data. Falling back to aas24." );
		aas = gameLocal.GetAAS( "aas24" );
	}
	
	savefile->ReadInt( destinationArea ); // int destinationArea

	SaveFileReadList( pathPoints, ReadVec3 ); // idList<idVec3> pathPoints
	SaveFileReadArrayCast( pathBeamOrigin, ReadObject, idClass*& ); // idBeam* pathBeamOrigin[CONVEY_MAX_PATH]
	SaveFileReadArrayCast( pathBeamTarget, ReadObject, idClass*& ); // idBeam* pathBeamTarget[CONVEY_MAX_PATH]

	savefile->ReadBool( hasStoredSkull ); // bool hasStoredSkull

	savefile->ReadBool( isLowHealthState ); // bool isLowHealthState
	savefile->ReadParticle( damageParticle ); // const idDeclParticle * damageParticle
	savefile->ReadInt( damageParticleFlyTime ); // int damageParticleFlyTime
	if (savefile->GetSaveVersion() < SAVEGAME_VERSION_0004) {
		idFuncEmitter* damageEmitterTemp = nullptr;
		savefile->ReadObject(CastClassPtrRef(damageEmitterTemp)); // idFuncEmitter * damageEmitter
		damageEmitter = damageEmitterTemp;
	} else {
		damageEmitter.Restore(savefile);
	}

	savefile->ReadObject( lostandfoundMachine ); // idEntityPtr<idEntity> lostandfoundMachine

	aiSpawnLocOrig.Restore( savefile ); // idEntityPtr<idLocationEntity> aiSpawnLocOrig

	savefile->ReadBool( waitingForNinaReply ); // bool waitingForNinaReply
	savefile->ReadInt( ninaReplyTimer ); // int ninaReplyTimer

	savefile->ReadBool( rantcheckDone ); // bool rantcheckDone
}

idEntity* idSkullsaver::FindLostandfoundMachine()
{	
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idLostAndFound::Type))
		{
			return ent;
		}
	}

	//failed to find machine.
	gameLocal.Warning("idSkullsaver '%s' unable to find lostandfound machine.", GetName());	
	return NULL;
}

void idSkullsaver::Think(void)
{
	//UpdateSparkTimer();

	if (state == SKULLSAVER_INESCAPEPOD)
	{
		Present();
		return;
	}
	else if (state == SKULLSAVE_JUSTSPAWNED)
	{
		if (gameLocal.time >= nextJumpTime)
		{
			//Initial jump up.
			idVec3 jumpPower;
			jumpPower = idVec3(-.2f + gameLocal.random.RandomFloat() * .4f, -.2f + gameLocal.random.RandomFloat() * .4f, 1);
			jumpPower *= (gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin()) ? SKULL_SPAWNJUMPPOWER_ZEROG : SKULL_SPAWNJUMPPOWER) * GetPhysics()->GetMass();
			GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), jumpPower);
			GetPhysics()->SetAngularVelocity(idVec3(32, 64, 0));
			state = SKULLSAVE_IDLE;

			nextJumpTime = gameLocal.time + SKULL_JUMPMINTIME + gameLocal.random.RandomInt(SKULL_JUMPRANDOMTIME);
		}
	}
	else if (state == SKULLSAVE_IDLE)
	{

		//Particles.
		if (idleSmoke != NULL && idleSmokeFlyTime && !IsHidden() && health > 0)
		{
			idVec3 dir = idVec3(0, 1, 0);

			if (!gameLocal.smokeParticles->EmitSmoke(idleSmoke, idleSmokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup))
			{
				idleSmokeFlyTime = gameLocal.time;
			}
		}


		if (gameLocal.time > nextJumpTime && spawnArgs.GetBool("wiggle_enabled", "1"))
		{
			//set up next jump time.
			nextJumpTime = gameLocal.time + SKULL_JUMPMINTIME + gameLocal.random.RandomInt(SKULL_JUMPRANDOMTIME);

			bool airless = gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin());

			//jump up.			
			bool isCarried = false;
			if (gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this)) //Only do the jump if player is not holding onto the skull.
			{
				isCarried = true;				
			}

			if (!isCarried)
			{
				gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_spawn"));
			}

			if (GetPhysics()->HasGroundContacts() && !airless && !isCarried)
			{
				
				float verticalJumpAmount = 0.5f + (gameLocal.random.RandomFloat() * .5f);
				idVec3 jumpPower = idVec3(0,0, verticalJumpAmount);
				//idVec3 skullPos = GetPhysics()->GetOrigin();
				//idVec3 playerPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
				//skullPos.z = playerPos.z;

				//float dist = (skullPos - playerPos).LengthFast();
				//if (dist > JUMP_CLOSEDISTANCE_THRESHOLD)
				//	jumpPower = playerPos - skullPos; //skull is far away. jump toward player.
				//else
				//	jumpPower = skullPos - playerPos; //skull is close to player. jump away from player.

				//jumpPower.Normalize();
				//jumpPower *= .3f;
				//jumpPower.z = 1.0f;

				jumpPower *= SKULL_JUMPPOWER * GetPhysics()->GetMass();
				GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), jumpPower);
			}

			//Do the yell.
			if (gameLocal.time > heybarkTimer && !gameLocal.GetLocalPlayer()->IsJockeying()) //BC 4-15-2025: don't do skull conversation if player is jockeying
			{
				int barkLength = 0;

				if ((gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this) && gameLocal.GetLocalPlayer()->GetCarryable() != this))
				{
					//Is in pocket. Muffled.
					StartSound("snd_hey_muffled", SND_CHANNEL_VOICE, 0, false, &barkLength);
					gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_yell_muffled"));
					gameLocal.GetLocalPlayer()->DoLocalSoundwave(gameLocal.GetLocalPlayer()->spawnArgs.GetString("model_soundwave_faint"));
				}
				else if (!gameLocal.InPlayerPVS(this))
				{
					//Is in a different PVS than player, and is not in player inventory.
					StartSound("snd_hey_muffled", SND_CHANNEL_VOICE, 0, false, &barkLength);
					gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_yell"));
				}
				else
				{
					//Normal yell.

					//Check if player is actively holding the skull.
					bool holdingMe = false;
					if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL) //See if player is holding onto the skull.
					{
						if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
						{
							holdingMe = true;
						}
					}

					//BC 2-18-2025: do nothing if in mech.
					bool doSoundwave = true;
					if (gameLocal.GetLocalPlayer()->IsInMech() && holdingMe)
					{
						doSoundwave = false;
					}
					else if (holdingMe)
					{
						//BC 2-14-2025: skullsaver rant check.
						//Check for conditions to allow for rant to happen. We only allow it if XX time has passed in mission, and a random check passes.
						bool doRant = false;						
						if (!rantcheckDone && gameLocal.time > RANT_MISSIONTIMER)
						{
							rantcheckDone = true;

							if (gameLocal.random.RandomInt(RANT_CHANCE) <= 0)
							{
								doRant = true;
							}
						}

						if (StartSound(doRant  ? "snd_vo_heldrant" : "snd_held", SND_CHANNEL_VOICE, 0, false, &barkLength)) //skull says something to nina.
						{
							//BC 2-14-2025: logic for nina replying to skull
							waitingForNinaReply = true;
							ninaReplyTimer = gameLocal.time + barkLength + NINAREPLY_TIMEGAP;

							//BC 5-9-2025: fixed bug where skullsaver (when being held) wasn't generating interestpoint.
							gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_skullnoise"));							
						}
					}
					else
					{
						StartSound("snd_hey", SND_CHANNEL_VOICE, 0, barkLength); //skullsaver rolling around on ground.
						gameLocal.SpawnInterestPoint(this, this->GetPhysics()->GetOrigin(), spawnArgs.GetString("interest_yell"));
					}

					if (doSoundwave && soundwaves.IsValid())
					{
						soundwaves.GetEntity()->SetActive(true);
					}
				}

				heybarkTimer = gameLocal.time + barkLength + YELL_MINTIME + gameLocal.random.RandomInt(YELL_RANDOMVARIATIONTIME);
			}
		}



		//Check if it's in player's inventory pocket.
		if (gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this) && fl.hidden)
		{
			//idVec3 forward = gameLocal.GetLocalPlayer()->viewAngles.ToForward();
			this->GetPhysics()->SetOrigin(gameLocal.GetLocalPlayer()->GetEyePosition() + idVec3(0, 0, -16));
		}


		if (!gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this) && !fl.hidden  && gameLocal.time > conveyorDelayTime && GetPhysics()->HasGroundContacts() && spawnArgs.GetBool("respawn_enabled", "1"))
		{
			//Skullsaver 3.0: I am ready to start levitating toward the respawn point.
			state = SKULLSAVER_CONVEYJUMP;
			nextJumpTime = gameLocal.time + 350; //delay time to wait for the skull to be at (or near-ish) the apex of its jump

			//Jump up. Ideally we want the respawn convey to happen when the skull is airborne.
			bool airless = gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin());
			if (!airless && !gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this))
			{
 				idVec3 jumpPower = idVec3(0, 0, 1);
 				jumpPower *= SKULL_SPAWNJUMPPOWER * GetPhysics()->GetMass();
 				GetPhysics()->ApplyImpulse(0, GetPhysics()->GetAbsBounds().GetCenter(), jumpPower);
			}
		}

		//Check if it is time to despawn.
		//if (gameLocal.time > lifetime)
		//{
		//	doRespawnJump();
		//}

		if (waitingForNinaReply && gameLocal.time > ninaReplyTimer)
		{
			waitingForNinaReply = false;

			//see if it's valid for nina to reply.
			if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL) //See if player is holding onto the skull.
			{
				if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
				{
					gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay("snd_vo_ninaskullrespond");					
				}
			}
		}
	}
	else if (state == SKULLSAVER_CONVEYJUMP)
	{
		if (gameLocal.time > nextJumpTime)
		{
			conveyorRespawnpoint = GetNearestRespawnpoint(aiSpawnLocOrig.GetEntity());

			// SW 22nd April 2025: Do not let the skullsaver attempt to convey while inside the cat pod (this causes all kinds of problems).
			// Try to find a catpod interior entity nearby. If it is, we assume we're inside (it's a fairly safe assumption since the cat pod lives in its own little pocket dimension)
			idEntity* entities[64];
			bool inCatPod = false;
			int entCount = gameLocal.EntitiesWithinRadius(this->GetPhysics()->GetOrigin(), 256, entities, 64);
			for (int i = 0; i < entCount; i++)
			{
				if (entities[i]->IsType(idCatpodInterior::Type))
				{
					inCatPod = true;
					break;
				}
			}

			if (conveyorRespawnpoint.IsValid() && !inCatPod)
			{
				state = SKULLSAVER_CONVEYING;
				BecomeInactive(TH_PHYSICS);
				SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_conveyed"))); //make it turn to the conveyor glowing skin.

				GetPhysics()->SetGravity(idVec3(0, 0, 0));
				//GetPhysics()->SetContents(CONTENTS_CORPSE);

				StartSound("snd_convey", SND_CHANNEL_BODY); // SW 17th March 2025: Moving this to a body channel (it shouldn't be affected by the voice volume slider)
				gameLocal.DoParticle("skullsaver_conveystart.prt", GetPhysics()->GetOrigin());

				conveyorRespawnPos = conveyorRespawnpoint.GetEntity()->GetPhysics()->GetOrigin() + idVec3( 0, 0, CONVEY_RESPAWNBEACON_OFFSET );

				SetupPath();


				//BC 2-13-2025: make Nina say vo if shes sees skull start conveying.
				if (DoesPlayerHaveLOStoMe_Simple())
				{
					gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay("snd_vo_see_skull");
				}

				heybarkTimer = gameLocal.time + 2500; //BC 2-14-2025: force a delay before first convey bark can happen.
			}
			else
			{
				//No valid respawn position. Just go back to idle.
				state = SKULLSAVE_IDLE;
				nextJumpTime = gameLocal.time + SKULL_JUMPRANDOMTIME;
				conveyorDelayTime = gameLocal.time + spawnArgs.GetInt("convey_delay_time", "2000");
			}
		}
	}
	else if (state == SKULLSAVER_CONVEYING)
	{
		if ( !pathBeamOrigin[0]->IsHidden() )
		{
			pathBeamOrigin[0]->SetOrigin( GetPhysics()->GetOrigin() );
		}
		
		//Particles.
		if (idleSmoke != NULL && idleSmokeFlyTime && !IsHidden())
		{
			idVec3 dir = idVec3(0, 1, 0);

			if (!gameLocal.smokeParticles->EmitSmoke(idleSmoke, idleSmokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup))
			{
				idleSmokeFlyTime = gameLocal.time;
			}
		}



		//skullsaver 3.0
		//gently floating toward respawn point.
		if (conveyorRespawnpoint.IsValid())
		{
			//Float toward the respawn point.
			float lerp = (gameLocal.time - conveyorStartMoveTime) / (float)conveyorTotalMoveTime;
			//lerp = idMath::CubicEaseInOut(lerp);
			idVec3 newPos;
			newPos.Lerp(conveyorStartPosition, conveyorDestPosition, lerp);
			GetPhysics()->SetOrigin(newPos);

			//Check if it has arrived at respawn point or if we need to continue on path.
			if (lerp >= 1)
			{
				if ( ( conveyorRespawnPos - GetPhysics()->GetOrigin() ).Length() < 4.0f )
				{
					//has arrived. Do respawn.
					state = SKULLSAVER_STARTRESPAWN;
					nextJumpTime = gameLocal.time + spawnArgs.GetInt("convey_respawn_time", REGENERATIONTIME);

					StartSound("snd_vo_respawn", SND_CHANNEL_VOICE);
					StartSound( "snd_startrespawn", SND_CHANNEL_BODY3 );
					if (soundwaves.IsValid()) {
						soundwaves.GetEntity()->SetActive(true);
					}

					if (regnerationParticle.IsValid()) {
						regnerationParticle.GetEntity()->SetActive(true);
					}
				}
				else
				{
					SetupPath();
				}
			}
		}


		//BC 2-14-2025: Skull barks when it's conveying.
		if (gameLocal.time > heybarkTimer)
		{
			StartSound("snd_vo_respawn", SND_CHANNEL_VOICE);
			heybarkTimer = gameLocal.time + YELL_MINTIME + gameLocal.random.RandomInt(YELL_RANDOMVARIATIONTIME);
			if (soundwaves.IsValid()) {
				soundwaves.GetEntity()->SetActive(true);
			}
		}		


	}
	else if (state == SKULLSAVER_STARTRESPAWN)
	{
		if (gameLocal.time >= nextJumpTime)
		{
			if (bodyOwner.IsValid() && bodyOwner.GetEntity()->IsType(idAI::Type))
			{
				gameLocal.GetLocalPlayer()->EnemyHasRespawned();

				StopSound(SND_CHANNEL_ANY);

				idEntityFx::StartFx("fx/skullsaver_sparkle", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
				this->Hide();

				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_skull_regenerated"), bodyOwner.GetEntity()->displayName.c_str()), GetPhysics()->GetOrigin());

				state = SKULLSAVE_DONE;
				bodyOwner.GetEntity()->Teleport(conveyorRespawnpoint.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 4), idAngles(0, 0, 0), NULL);
				static_cast<idAI *>(bodyOwner.GetEntity())->Resurrect();

				PostEventMS(&EV_Remove, 0);
			}
		}
	}
	else if (state == SKULLSAVER_LOSTANDFOUNDRESPAWN)
	{
		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
		{
			if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
			{
				gameLocal.GetLocalPlayer()->DropCarryable(this);
			}
		}

		//Make it useable again.
		Unbind();
		Show();		
		health = 1;
		fl.takedamage = true;

		//Teleport to lost and found machine
		if (lostandfoundMachine.IsValid())
		{
			if (lostandfoundMachine.GetEntity()->IsType(idLostAndFound::Type))
			{
				gameLocal.DoParticle(spawnArgs.GetString("model_teleport"), GetPhysics()->GetOrigin());

				idVec3 dispenserPos = static_cast<idLostAndFound*>(lostandfoundMachine.GetEntity())->GetDispenserPosition();
				SetOrigin(dispenserPos);
				GetPhysics()->SetLinearVelocity(idVec3(0, 0, 16));
				GetPhysics()->SetAngularVelocity(idVec3(16, 16, 0));

				idStr lostfoundMessage = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_lostfound_sent"), displayName.c_str());
				gameLocal.AddEventLog(lostfoundMessage.c_str(), this->GetPhysics()->GetOrigin());

				gameLocal.GetLocalPlayer()->SetCenterMessage(lostfoundMessage.c_str());

				gameLocal.GetLocalPlayer()->SetImpactSlowmo(true, 100);
			}
		}

		
		ResetConveyTime();

		GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
		GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);
	}
	

	//int remainingTime = (int)((lifetime - gameLocal.time) / 1000.0f);
	//Event_SetGuiInt("gui_parm0", remainingTime);


	idMeta *meta = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity());
	idFTL *ftl = static_cast<idFTL *>(meta->GetFTLDrive.GetEntity());
	
	int seconds = 0;
	if (ftl != NULL)
	{
		seconds = (int)(ftl->GetPublicTimer() / 1000.0f);
	}
	Event_SetGuiInt("gui_parm0", seconds);

	// Update light source's position
	if (headlightHandle != -1)
	{
		headlight.origin = GetPhysics()->GetOrigin();
		gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
	}
	else if (!fl.hidden && spawnArgs.GetBool("useLight", "1"))
	{
		// Light source. We use a renderlight (i.e. not a real light entity) to prevent weird physics binding jitter
		headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
		headlight.pointLight = true;
		headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 48.0f;
		headlight.shaderParms[0] = 0.3f; // R
		headlight.shaderParms[1] = 0.0f; // G
		headlight.shaderParms[2] = 0.3f; // B
		headlight.shaderParms[3] = 1.0f; // ???
		headlight.noShadows = true;
		headlight.isAmbient = true;
		headlight.axis = mat3_identity;

		headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	}

	if (fl.hidden)
		return;
	
	if (isLowHealthState)
	{
		if (damageParticle != NULL && damageParticleFlyTime && !IsHidden())
		{
			idAngles particleAngle = gameLocal.GetLocalPlayer()->viewAngles;
			particleAngle.roll = 0;
			particleAngle.pitch = 112; //we want the particle to be angled a bit DOWNWARD so that it obscures more of the player's screen. 90 = straight left, 135 = 45 degrees down.
			particleAngle.yaw += 90; //yaw of particle. We want it to go leftward so that it obscures more of the player's screen.
			
			if (!gameLocal.smokeParticles->EmitSmoke(damageParticle, damageParticleFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), particleAngle.ToMat3(), timeGroup))
			{
				damageParticleFlyTime = gameLocal.time;
			}
		}
	}

	// SW 24th Feb 2025
	if (soundwaves.IsValid())
	{
		soundwaves.GetEntity()->Show();
	}

	UpdateSpacePush();
	RunPhysics();
	Present();

	
}

idEntity *idSkullsaver::GetNearestRespawnpoint(idLocationEntity * restrictLocation)
{
	spawnSpot_t		spot;
	spot.ent = gameLocal.FindEntityUsingDef(NULL, "info_enemyspawnpoint");

	if (!spot.ent)
		return NULL;

	idEntity *nearestEnt = NULL;
	float closestDistance = 99999;

	while (spot.ent)
	{
		if( restrictLocation != nullptr && restrictLocation != gameLocal.LocationForEntity(spot.ent) )
		{ // contain to location
			spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint");
			continue;
		}

		if (gameLocal.GetAirlessAtPoint(spot.ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 4))) //Check if there's oxygen at this respawn pad.
		{
			//No oxygen at this spot. Skip it, so that we're not spawning someone inside zero-g.
			spot.ent = gameLocal.FindEntityUsingDef( spot.ent, "info_enemyspawnpoint" );
			continue;
		}

		float distance = (spot.ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).Length();
		if (distance < closestDistance)
		{
			closestDistance = distance;
			nearestEnt = spot.ent;
		}

		spot.ent = gameLocal.FindEntityUsingDef(spot.ent, "info_enemyspawnpoint");
	}

	if (nearestEnt == NULL)
		return NULL;

	return nearestEnt;
}


void idSkullsaver::SetupPath()
{
	if (aas == NULL)
	{
		gameLocal.Error("Skullsaver: map has no pathing data. Run DMAP.");
	}

	// blendo eric: added TFL_INVALID to allow travel through doors (and TFL_AIR cuz why not?)
	const int SKULL_SAVER_TRAVEL_TYPE = TFL_WALK | TFL_FLY  | TFL_AIR | TFL_INVALID;
	// blendo eric: added AREA_NOWALK|AREA_LADDER|AREA_LEDGE|AREA_GAP to reachable areas, to allow skullsaver to allow more fly route options
	const int SKULL_SAVER_AREA_TYPE = AREA_REACHABLE_FLY | AREA_REACHABLE_WALK | AREA_NOWALK|AREA_LADDER|AREA_LEDGE|AREA_GAP;


	idBounds bounds = aas->GetSettings()->boundingBoxes[0];
	idVec3 startPos = GetPhysics()->GetOrigin();



	int startArea = aas->PointReachableAreaNum( startPos, bounds, SKULL_SAVER_AREA_TYPE );
	aas->PushPointIntoAreaNum( startArea, startPos );

	idVec3 destPos = conveyorRespawnpoint.GetEntity()->GetPhysics()->GetOrigin() + idVec3( 0, 0, CONVEY_RESPAWNBEACON_OFFSET );

	destinationArea = aas->PointReachableAreaNum( destPos, bounds, SKULL_SAVER_AREA_TYPE  );
	aas->PushPointIntoAreaNum( destinationArea, destPos );

#ifdef _DEBUG
	if(g_skullsaver_debug.GetInteger() > 0) {
		const int VIZ_TIME = 5000;
		gameRenderWorld->DebugSphere( colorWhite, idSphere(startPos,5.0f), VIZ_TIME);
		gameRenderWorld->DebugTextSimple( idStr::Format("start area: %d", startPos ),  destPos, VIZ_TIME);
		gameRenderWorld->DebugSphere( colorLtGrey, idSphere(destPos,5.0f), VIZ_TIME);
		gameRenderWorld->DebugTextSimple( idStr::Format("dest area: %d",destinationArea ),  destPos, VIZ_TIME);
	}
#endif

	bool foundPath = false;
	aasPath_t conveyPath;

	// Try to find a path if we have valid areas
	if ( startArea && destinationArea )
	{
		foundPath = aas->FlyPathToGoal( conveyPath, startArea, startPos, destinationArea, destPos, SKULL_SAVER_TRAVEL_TYPE );
		if(!foundPath)
		{
			foundPath = aas->WalkPathToGoal( conveyPath, startArea, startPos, destinationArea, destPos, SKULL_SAVER_TRAVEL_TYPE );
		}
	}

	conveyorStartPosition = GetPhysics()->GetOrigin();
	if ( foundPath )
	{
		conveyorDestPosition = conveyPath.moveGoal;
		
		// Build list of points on path
		pathPoints.Clear();
		pathPoints.Append( conveyorStartPosition );
		pathPoints.Append( conveyPath.moveGoal );

		float destDist = ( conveyPath.moveGoal - destPos ).Length();
		while ( destDist > 10.0f )
		{
			int tempArea = conveyPath.moveAreaNum;
			idVec3 tempGoal = conveyPath.moveGoal;
			if ( aas->FlyPathToGoal( conveyPath, tempArea, tempGoal, destinationArea, destPos, SKULL_SAVER_TRAVEL_TYPE )
				|| aas->WalkPathToGoal( conveyPath, tempArea, tempGoal, destinationArea, destPos, SKULL_SAVER_TRAVEL_TYPE ) )
			{
				pathPoints.Append( conveyPath.moveGoal );
				destDist = ( conveyPath.moveGoal - destPos ).Length();
			}
			else
			{
				// Possible infinite loop prevention?
				gameLocal.Warning( "AAS path for skullsaver %s may be an infinite loop, using partial path.", GetName() );
				destDist = 0.0f;
			}

			if ( pathPoints.Num() > 50 )
			{
				gameLocal.Warning( "AAS path for skullsaver %s may be an infinite loop, using partial path.", GetName() );
				destDist = 0.0f;
			}
		}
	}
	else
	{
		gameLocal.Warning( "Could not find AAS path for skullsaver %s, using fallback behavior.", GetName() );
		conveyorDestPosition.Lerp( conveyorStartPosition, destPos, 0.25f );

		// Fill out the approximate path
		pathPoints.Clear();
		pathPoints.Append( conveyorStartPosition );
		pathPoints.Append( conveyorDestPosition );
		pathPoints.Append( destPos );
	}

	float conveyDistance = ( conveyorDestPosition - GetPhysics()->GetOrigin() ).Length();
	conveyorTotalMoveTime = ( conveyDistance / ( float )spawnArgs.GetFloat("convey_time_factor", "8.0")) * 1000.0f;
	conveyorStartMoveTime = gameLocal.time;

	for ( int i = 0; i < CONVEY_MAX_PATH; i++ )
	{
		if ( i < pathPoints.Num() - 1 )
		{
			pathBeamOrigin[i]->SetOrigin( pathPoints[i] );
			pathBeamTarget[i]->SetOrigin( pathPoints[i + 1] );
			pathBeamOrigin[i]->Show();
		}
		else
		{
			pathBeamOrigin[i]->Hide();
		}
	}
}

//This gets called from idMeta to respawn the skullsaver.
void idSkullsaver::doRespawnJump(idVec3 spawnPosition)
{
	//bool isCarryingMe = false;
	//if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL) //Only do the jump if player is not holding onto the skull.
	//{
	//	if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
	//	{
	//		isCarryingMe = true;
	//	}
	//}


	//Great, we have a spawn point. Spawn the monster.
	if (bodyOwner.IsValid())
	{
		if (bodyOwner.GetEntity()->IsType(idAI::Type))
		{
			gameLocal.GetLocalPlayer()->EnemyHasRespawned();

			idEntityFx::StartFx("fx/skullsaver_sparkle", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
			this->Hide();

			gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_skull_regenerated"), bodyOwner.GetEntity()->displayName.c_str()), GetPhysics()->GetOrigin());

			state = SKULLSAVE_DONE;
			bodyOwner.GetEntity()->Teleport(spawnPosition, idAngles(0, 0, 0), NULL);
			static_cast<idAI *>(bodyOwner.GetEntity())->Resurrect();

			//NOTE: the stopragdoll() call in State_Resurrect technically teleports the monster back to its spawnargs "origin" value.
			//So, in the teleport() call above, it sets "origin" to the place where we want to teleport the monster.

			//Do some cleanup. Remove it from player inventory.
			const char *weaponName = spawnArgs.GetString("weapon");
			gameLocal.GetLocalPlayer()->RemoveCarryableFromInventory(weaponName);

			PostEventMS(&EV_Remove, 0);
		}
	}
	else
	{
		//This shouldn't happen...............
		gameLocal.Warning("FAILED TO FIND A VALID RESPAWN POINT FOR '%s'\n", this->GetName());

		StartSound("snd_fail", SND_CHANNEL_ANY, 0, false, NULL);
		lifetime = gameLocal.time + SKULL_FAILTIMER + gameLocal.random.RandomInt(SKULL_FAILTIMER_VARIANCE);
		state = SKULLSAVE_IDLE;
		nextJumpTime = gameLocal.time + 500;

		return;
	}

}



bool idSkullsaver::Collide(const trace_t &collision, const idVec3 &velocity)
{
	//BC turn this off for Skullsaver 3.0
	idEntity *other;
	other = gameLocal.entities[collision.c.entityNum];
	if (other)
	{
		if (other->RespondsTo(EV_Touch))
		{
			other->ProcessEvent(&EV_Touch, this, &collision);
		}
	}

	return idMoveableItem::Collide(collision, velocity);

	//if (state == SKULLSAVER_ENEMYCARRIED)
	//	return false;
	//
	////Allow this to trigger ev_touch events. This is for storing the skullsaver inside lifeboats.
	//idEntity *other;
	//other = gameLocal.entities[collision.c.entityNum];
	//if (other)
	//{
	//	if (other->RespondsTo(EV_Touch))
	//	{
	//		other->ProcessEvent(&EV_Touch, this, &collision);
	//	}
	//
	//	//Spring tether test.
	//	//if (other->IsType(idSkullsaver::Type) && other != this)
	//	//{
	//	//	//touched another skullsaver.
	//	//	bool createSpring = true;
	//	//
	//	//	if (this->springPartner.IsValid())
	//	//	{
	//	//		//if I already have a spring on me, then ignore.
	//	//		createSpring = false;
	//	//	}
	//	//	if (static_cast<idSkullsaver *>(other)->springPartner.IsValid())
	//	//	{
	//	//		if (static_cast<idSkullsaver *>(other)->springPartner.GetEntity() == this)
	//	//		{
	//	//			//if partner has a spring but it IS me, then ignore.
	//	//			createSpring = false;
	//	//		}
	//	//	}
	//	//
	//	//	if (createSpring)
	//	//	{
	//	//		//Ok, conditions are valid for making a spring.
	//	//		idDict args;
	//	//		args.Clear();
	//	//		args.SetVector("origin", GetPhysics()->GetOrigin());
	//	//		args.Set("ent1", this->GetName());
	//	//		args.Set("ent2", other->GetName());
	//	//		args.SetFloat("constant", 900.0f); //100
	//	//		args.SetFloat("damping", 10.0f); //10
	//	//		args.SetFloat("restlength", 0.0f); //0
	//	//		idEntity *newSpring = (idSpring *)gameLocal.SpawnEntityType(idSpring::Type, &args);
	//	//
	//	//		if (newSpring)
	//	//		{
	//	//			//spring successfully spawned.  Update my spring partner info.
	//	//			this->springPartner = other;
	//	//		}
	//	//	}
	//	//}
	//}
	//
	//return idMoveableItem::Collide(collision, velocity);
}

void idSkullsaver::Hide(void)
{
	idMoveableItem::Hide();

	if (itemLineHandle != -1)
	{
		gameRenderWorld->FreeEntityDef(itemLineHandle);
		itemLineHandle = -1;
	}

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}
	
	// SW 24th Feb 2025
	if (soundwaves.IsValid())
	{
		soundwaves.GetEntity()->Hide();
	}
}

void idSkullsaver::SetInEscapePod()
{
	state = SKULLSAVER_INESCAPEPOD;
}

//I've been stored in a lifeboat or escape pod or etc.
void idSkullsaver::StoreSkull()
{
	if (hasStoredSkull)
		return;

	hasStoredSkull = true;

	if (bodyOwner.IsValid())
	{
		gameLocal.GetLocalPlayer()->DoEliminationMessage(bodyOwner.GetEntity()->displayName.c_str(), true); //print elimination message.
		bodyOwner.GetEntity()->PostEventMS(&EV_Remove, 0);
	}
	else
	{
		common->Warning("Skullsaver '%s' has no bodyOwner.\n", this->GetName());
	}

	
	//Do some cleanup. Remove it from player inventory.
	//const char *weaponName = spawnArgs.GetString( "weapon" );
	//gameLocal.GetLocalPlayer()->RemoveCarryableFromInventory( weaponName );

	//Do cleanup. We only need to do this if the player is currently holding onto a skullsaver.
	if (gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(this))
	{
		gameLocal.GetLocalPlayer()->RemoveCarryableFromInventory(this);
	}



	//Determine whether we need to end the game.
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->OnEnemyStoredLifeboat();
}

bool idSkullsaver::DoFrob(int index, idEntity * frobber)
{
	if (index == ENEMY_FROBINDEX)
	{
		//Enemy touched the skullsaver. Shorten the convey delay.
		if (abs(gameLocal.time - conveyorDelayTime) > 500 && state == SKULLSAVE_IDLE)
		{
			conveyorDelayTime = gameLocal.time + 500;
		}

		return true;
	}

	//if (index == ENEMY_FROBINDEX)
	//{
	//	//Enemy has frobbed me.
	//
    //    //This is what attaches a skullsaver to an enemy.
	//
	//	if (frobber == NULL)
	//		return false;
	//
	//	if (!frobber->IsType(idAI::Type))
	//		return false;
	//
	//	jointHandle_t attachJoint;
	//	attachJoint = static_cast<idAI *>(frobber)->GetAnimator()->GetJointHandle("shoulders");
	//	if (attachJoint == INVALID_JOINT)
	//		return false;
	//
	//	idVec3 attachPos;
	//	idMat3 attachMat;
	//	static_cast<idAI *>(frobber)->GetAnimator()->GetJointTransform(attachJoint, gameLocal.time, attachPos, attachMat);
	//	attachPos = frobber->GetPhysics()->GetOrigin() + attachPos * frobber->GetPhysics()->GetAxis();
	//	SetAxis(mat3_identity);
	//
	//	idVec3 frobberForward = static_cast<idAI *>(frobber)->viewAxis.ToAngles().ToForward();
	//	SetOrigin(attachPos + frobberForward * -8);
	//	StartSound("snd_grab", SND_CHANNEL_ANY);
	//	BindToJoint(frobber, attachJoint, false);
	//
	//	state = SKULLSAVER_ENEMYCARRIED;
	//
	//	return true;
	//}

	bool playerFrobbed = idMoveableItem::DoFrob(index, frobber);
	if (playerFrobbed)
	{
		ResetConveyTime();
	}
	return playerFrobbed;
}

void idSkullsaver::DetachFromEnemy()
{
	Unbind();
	state = SKULLSAVE_IDLE;
	nextJumpTime = gameLocal.time + SKULL_JUMPMINTIME + gameLocal.random.RandomInt(SKULL_JUMPRANDOMTIME);
}

void idSkullsaver::ResetConveyTime()
{
	if (!gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin()))
	{
		GetPhysics()->SetGravity(gameLocal.GetGravity());
	}

	state = SKULLSAVE_IDLE;
	StopSound(SND_CHANNEL_ANY);
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_default")));
	conveyorDelayTime = gameLocal.time + spawnArgs.GetInt("convey_delay_time", "2000");

	for ( int i = 0; i < CONVEY_MAX_PATH; i++ )
	{
		pathBeamOrigin[i]->Hide();
	}

	if (regnerationParticle.IsValid()) {
		regnerationParticle.GetEntity()->SetActive(false);
	}
}

void idSkullsaver::Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location, const int materialType)
{
	//skullsaver does not take damage for a short grace period when it first spawns in world. This is to avoid/alleviate chain reactions of events immediately making the skullsaver get destroyed.
	if (gameLocal.time - this->spawnTime < ITEM_DROPINVINCIBLE_TIME)
		return;

	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	//when low health, make it spew smoke. So player has some warning that it's in danger.
	if (health <= 1 && !isLowHealthState)
	{
		isLowHealthState = true;
		damageParticle = static_cast<const idDeclParticle*>(declManager->FindType(DECL_PARTICLE, spawnArgs.GetString("model_damageparticle")));
		damageParticleFlyTime = gameLocal.time;
		
		if (damageEmitter.IsValid()) {
			damageEmitter.GetEntity()->SetActive(true);
		}
	}
}

void idSkullsaver::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	//skullsaver has been depleted to health zero.
	//we zap the skullsaver to the lost and found machine.	

	state = SKULLSAVER_LOSTANDFOUNDRESPAWN;	
}

void idSkullsaver::SetBodyOwner(idEntity* ent)
{
	if (ent == NULL)
		return;

	GetPhysics()->GetClipModel()->SetOwner(this); //so skullsaver doesn't collide into the body.
	bodyOwner = ent;

	Event_SetGuiParm("ownername", ent->displayName.c_str());
}

//BC 2-20-2025: yell when thrown in space.
void idSkullsaver::JustThrown()
{
	idMoveableItem::JustThrown();

	if (gameLocal.GetAirlessAtPoint(GetPhysics()->GetOrigin()))
	{
		//gameLocal.voManager.SayVO(this, "snd_vo_ejectspace", VO_CATEGORY_BARK);
		StartSound("snd_vo_ejectspace", SND_CHANNEL_VOICE);
		if (soundwaves.IsValid()) {
			soundwaves.GetEntity()->SetActive(true);
		}
	}
}
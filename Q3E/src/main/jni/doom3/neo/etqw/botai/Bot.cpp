// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Player.h"
#include "../rules/GameRules.h"
#include "../Game_local.h"
#include "../vehicles/Transport.h"
#include "../vehicles/VehicleView.h"
#include "../vehicles/VehicleControl.h"
#include "../../game/ContentMask.h"

#include "aas/ObstacleAvoidance.h"
#include "Bot.h"
#include "BotThreadData.h"
#include "BotThread.h"
#include "../WorldSpawn.h"


idCVar bot_testObstacleAvoidance( "bot_testObstacleAvoidance", "0", CVAR_GAME | CVAR_BOOL, "test obstacle avoidance" );

#ifdef BOT_MOVE_LOOKUP
// FeaRog: Statics for movement
float idBot::lastRunFwd = -1.0f;
float idBot::lastRunSide = -1.0f;
float idBot::lastRunBack = -1.0f;
float idBot::lastSprintFwd = -1.0f;
float idBot::lastSprintSide = -1.0f;
idBot::moveLookupArray_t idBot::runMoveForward;
idBot::moveLookupArray_t idBot::runMoveRight;
idBot::moveLookupArray_t idBot::sprintMoveForward;
idBot::moveLookupArray_t idBot::sprintMoveRight;
#endif

CLASS_DECLARATION( idPlayer, idBot )
END_CLASS


/*
================
idBot::idBot
================
*/
idBot::idBot() {
	firePistol = false;
	firePliers = false;
	lefty = false;
	moveJumpNow = false;
	leftyTimer = 5;
	jumpTimer = 10;
	nextAimUpdateTime = 0;
	movingForward = 0;
	proneDelay = 0;
	hornTime = 0;
	lowSkillAimPoint.Zero();
	decoyTime = 0;
	botNextWeapTime = 0;
	botNextSiegeCmdTime = 0;
	altAttackDelay = 0;
	updateAimTime = 0;
	botNextVehicleCmdTime = 0;
	oldBotGoalType = NULL_GOAL_TYPE;
	botViewAngles.Zero();
	botHackMoveAngles.Zero();
}

/*
================
idBot::~idBot
================
*/
idBot::~idBot() {
	assert( entityNumber >= 0 && entityNumber < MAX_CLIENTS );
	// add to remove list
	botThreadData.RemoveBot( entityNumber );
}

/*
================
idBot::Think

This is the start of the bot's thinking process, called
every game frame by the server.
================
*/
void idBot::Think() {

	if ( !gameLocal.isServer ) { //mal: for client prediction, just run the bot's latest ucmds.
        idPlayer::Think();  
		return;
	}

	const clientInfo_t& bot = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

#ifdef _XENON
 	if ( bot_pause.GetBool() || ( gameLocal.rules && gameLocal.rules->IsWarmup() ) || ( botThreadData.PlayerIsBeingBriefed() && !bot.isActor ) ) { //mal: if the bot is being paused......
#else
	if ( bot_pause.GetBool() || ( botThreadData.PlayerIsBeingBriefed() && !bot.isActor ) ) { //mal: if the bot is being paused......
#endif
		usercmd_t &ucmd = gameLocal.usercmds[ entityNumber ];
		Bot_ResetUcmd( ucmd );
		ucmd.gameTime = gameLocal.time;
		ucmd.gameFrame = gameLocal.framenum;
		ucmd.duplicateCount = 0;
		networkSystem->ServerSetBotUserCommand( entityNumber, gameLocal.framenum, ucmd );
		idPlayer::Think();
		return;
	}

//mal: pre player think AI
	if ( IsSpectator() ) { //mal: on map change, we can get bumped to spectators - if so, set our team/class back to what it was!
		Bot_ResetGameState();
	}

	CheckBotIngameMissionStatus();

	idPlayer::Think();
}

/*
================
idBotAI::ResetUcmd

Clears out the Bot's user cmd structure at the start of every frame, so that we
can build a fresh one each bot think.
================
*/
void idBot::Bot_ResetUcmd ( usercmd_t &ucmd ) {
	ucmd.forwardmove = 0;
	ucmd.rightmove = 0;
	ucmd.upmove = 0;
	ucmd.impulse = 0;
	ucmd.flags = 0;
	memset( &ucmd.buttons, 0, sizeof( ucmd.buttons ) );
	memset( &ucmd.clientButtons, 0, sizeof( ucmd.clientButtons ) );
}

idCVar bot_noTapOut( "bot_noTapOut", "0", CVAR_BOOL | CVAR_GAME, "makes bots not want to ever tap out, for debug purposes" );
idCVar bot_noRandomJump( "bot_noRandomJump", "0", CVAR_BOOL | CVAR_GAME, "makes bots not randomly jump" );


/*
================
idBotAI::Bot_InputToUserCmd

Sends the bot's user cmds to the server, updates bots cmd timers, etc.
================
*/
void idBot::Bot_InputToUserCmd() {
	usercmd_t &ucmd = gameLocal.usercmds[ entityNumber ];
	const botAIOutput_t& botOutput = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];
	clientInfo_t& botInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	
//mal: always keep these current!
	ucmd.gameTime = gameLocal.time;
	ucmd.gameFrame = gameLocal.framenum;
	ucmd.duplicateCount = 0;

	Bot_ResetUcmd( ucmd );

#ifdef _XENON
	if ( gameLocal.rules->IsWarmup() ) { //mal: dont think or do anything in warmup on 360
		return;
	}
#endif

	botThreadData.VOChat( botOutput.desiredChat, entityNumber, botOutput.desiredChatForced );

	if ( botOutput.botCmds.ackReset ) { //mal: the AI thread ack'd reseting its AI, so turn this flag off.
		botInfo.resetState = 0;
	}

	if ( botOutput.botCmds.ackSeatChange ) { //mal: the AI thread ack'd moving seats in the vehicle, so turn this flag off.
		botInfo.proxyInfo.clientChangedSeats = false;
	}

	if ( botOutput.botCmds.ackJustSpawned ) { //mal: the AI thread ack'd just spawning, so turn this flag off.
        botInfo.justSpawned = false;
	}

	if ( botOutput.botCmds.actorBriefedPlayer ) {
		botThreadData.actorMissionInfo.hasBriefedPlayer = true;
	}

	if ( botOutput.botCmds.actorSurrenderStatus ) {
		botInfo.isActor = false;
	}

	if ( botOutput.botCmds.actorIsBriefingPlayer ) { //mal_TODO: play the player anims for chatting. This has yet to be added. Play the chat here too.
		gameLocal.Printf("Blah ");
		botThreadData.GetGameWorldState()->clientInfo[ botThreadData.actorMissionInfo.targetClientNum ].briefingTime = gameLocal.time + 100;
	}

	if ( botOutput.tkReviveTime > 0 ) {
		botInfo.tkReviveTime = botOutput.tkReviveTime;
	}

	if ( botOutput.decayObstacleSpawnID != -1 ) {
		idEntity* obstacle = gameLocal.EntityForSpawnId( botOutput.decayObstacleSpawnID );
		if ( obstacle != NULL ) {
			sdTransport* transport = obstacle->Cast< sdTransport >();
			if ( transport != NULL ) {
				transport->Decayed();
			}
		}
	}

	if ( bot_debug.GetBool() ) {
		if ( botOutput.botCmds.hasNoGoals != false ) { //mal: oops! Either someone screwed up, or goals haven't been added to this map yet. Let the player know either way!
			gameLocal.DWarning("%s has no valid goals on this map! Bot can't function without at least 1 valid goal on map!! Also make sure there is a current AAS available!\n", userInfo.name.c_str() );
		}
	}

	botViewAngles = viewAngles;

	if ( InVehicle() ) {
		VehicleUcmds( ucmd );
		networkSystem->ServerSetBotUserCommand( entityNumber, gameLocal.framenum, ucmd );
		return;
	}

	if ( health <= 0 ) { //mal: if bot is dead, run very simple ucmd stuff, and leave.
		DeadUcmds( ucmd );
		networkSystem->ServerSetBotUserCommand( entityNumber, gameLocal.framenum, ucmd );
		return;
	}

	Bot_SetWeapon(); //mal: update the bot's weapon choice

	AngleUcmds( ucmd );

	MoveUcmds( ucmd );

	ActionUcmds( ucmd );

	// pass the bot's user cmds off to the engine, so that other clients can predict this bot
	networkSystem->ServerSetBotUserCommand( entityNumber, gameLocal.framenum, ucmd );
}

/*
================
idBotAI::Pain

If the bot gets hurt - lets check to see who did it. 
We might want to change our tactics, or just complain (if a stupid mate hit us).
================
*/
bool idBot::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {

	bool chatPain = false;

//mal: if in warmup, dont worry about being shot
	if ( gameLocal.rules->IsWarmup() ) {
		return idPlayer::Pain( inflictor, attacker, damage, dir, location, damageDecl);
	}

	//mal: the game now auto "complains" for us if a teammate hits us.
/*
	if ( attacker )	{
		if ( attacker != this && ( attacker->entityNumber > -1 && attacker->entityNumber < MAX_CLIENTS )) {		
			if ( GetEntityAllegiance( inflictor ) == TA_FRIEND )  {
				if ( inflictor != NULL ) {
					if ( !inflictor->IsType( sdTransport::Type ) ) { //mal: dont say hold your fire if our teammate is running us over! :-D
						chatPain = true;
					}
				}
	
				if ( chatPain ) {
                    botThreadData.VOChat( HOLDFIRE, entityNumber, false );
				}				
			}
		}
	}
*/

	return idPlayer::Pain( inflictor, attacker, damage, dir, location, damageDecl );
}

/*
==================
idBot::Cmd_AASStats_f
==================
*/
void idBot::Cmd_AASStats_f( const idCmdArgs &args ) {
	if ( botThreadData.IsThreadingEnabled() ) {
		gameLocal.Printf( "set bot_threading to 0 to use this command\n" );
		return;
	}

	botThreadData.ShowAASStats();
}

/*
================
idBotAI::Bot_ResetGameState

ETQW is wierd in that you can lose your class info when server changes map.
For bots, they can also lose team info. We reset the class/team info to what
the bot last set them to.

FIXME: this currently doesn't do what its supposed to, because we dont archive team info.
================
*/
void idBot::Bot_ResetGameState() {

    int botTeam, botClass, botGun;

	if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber].team == NOTEAM || botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].classType == NOCLASS ) {
		return;
	}

	if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team == GDF ) {
		botTeam = 1; //GDF!
	} else {
		botTeam = 2; //STROGG!
	}
		
	gameLocal.rules->SetClientTeam( this, botTeam, true, "" );

	botClass = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].classType;

	botGun = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.primaryWeapon;

	Bot_SetClassType( this, botClass, botGun );

	UserInfoChanged();
}

/*
================
idBotAI::Bot_SetClassType

ETQW is wierd in that you can lose your class info when server changes map.
For bots, they can also lose team info. We reset the class/team info to what
the bot last set them to.
================
*/
void idBot::Bot_SetClassType( idPlayer* player, int classType, int classWeapon ) {

	const sdDeclPlayerClass* pc;
	
	if ( botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].team == GDF ) { //GDF!

        if ( classType == 0 ) {
			pc = gameLocal.declPlayerClassType[ "medic" ];
		} else if ( classType == 1 ) {
			pc = gameLocal.declPlayerClassType[ "soldier" ];
		} else if ( classType == 2 ) {
			pc = gameLocal.declPlayerClassType[ "engineer" ];
		} else if ( classType == 3 ) {
			pc = gameLocal.declPlayerClassType[ "fieldops" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "covertops" ];
		}

	} else { //mal: must be an evile STROGG!

		if ( classType == 0 ) {
			pc = gameLocal.declPlayerClassType[ "technician" ];
		} else if ( classType == 1 ) {
			pc = gameLocal.declPlayerClassType[ "aggressor" ];
		} else if ( classType == 2 ) {
			pc = gameLocal.declPlayerClassType[ "constructor" ];
		} else if ( classType == 3 ) {
			pc = gameLocal.declPlayerClassType[ "oppressor" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "infiltrator" ];
		}
	}

	player->ChangeClass( pc, classWeapon );

	botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].cachedClassType = pc->GetPlayerClassNum();
}

/*
================
idBot::Bot_SetWeapon

Basically a ripoff of idPlayer::SelectWeapon, except that I don't worry
about showing any menus/hud stuff, and I can easily have the bot loop to a alt weapon with
one bool (instead of worrying about calling selectweapon twice).
================
*/
void idBot::Bot_SetWeapon() {

	int botWeapon;

//mal: dont try to switch guns if we're just spawning in.
	if ( !GetPhysics()->HasGroundContacts() && !physicsObj.HasJumped() && invulnerableEndTime > gameLocal.time ) {
		return;
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].idealWeaponSlot == NO_WEAPON || botThreadData.GetGameOutputState()->botOutput[ entityNumber ].idealWeaponSlot == RESET_WEAPON ) {
		
		if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].idealWeaponNum != NULL_WEAP ) {
			GetInventory().SelectWeaponByNumber( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].idealWeaponNum );
		}

		return;
	}

	clientInfo_t& ourInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	weaponBankTypes_t weaponChoice = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].idealWeaponSlot;

	bool altWeapon = ( weaponChoice == SPECIAL1_ALT || weaponChoice == GUN_ALT ) ? true : false;

	if ( weaponChoice == GUN && !ourInfo.weapInfo.primaryWeapHasAmmo ) { //mal: do a quick ammo check! NEVER auto select grenades.
		if ( ourInfo.weapInfo.sideArmHasAmmo ) {
			weaponChoice = SIDEARM;
		} else {
			weaponChoice = MELEE;
		}
	}

  	botWeapon = GetInventory().CycleWeaponByPosition( weaponChoice, true, altWeapon, true, altWeapon );

	if( botWeapon != -1 ) {
		GetInventory().SetSwitchingWeapon( botWeapon );
		AcceptWeaponSwitch( true );
	} else { //mal: check our current weapon - just in case the weap we want has no ammo!

		idWeapon* gun = weapon.GetEntity(); //mal: get some info on our current gun

		if ( gun ) {
			if ( !gun->ShotsAvailable( 0 ) || botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.weapon == KNIFE ) { //has no ammo - so get a diff weapon!
				NextWeapon( true );
			}
		}
	}
}

/*
================
idBot::FixUcmd

Takes a usercmd and turns it into a int, and sets its range
================
*/
int idBot::FixUcmd( float ucmd ) {
	int u = idMath::Ftob( ucmd + 128 ) - 128;
	return u;
}

/*
================
idBot::Bot_ChangeViewAngles

Turns the bot towards its idealAngle. Cheap, and produces very smooth angle changing
================
*/
void idBot::Bot_ChangeViewAngles( const idAngles &idealAngle, bool fast ) {

	int i;
	float move;
 	float angMod = ( fast == false ) ? ( 1.0f / 12.0f ) : ( 1.0f / 2.0f ); //mal: 1/12 WAS 1/8

	if ( botViewAngles.Compare( idealAngle, 1e-2f ) ) {
		return;
	}

	for( i = 0; i < 3; i++ ) {
        move = idMath::AngleDelta( idealAngle[ i ], botViewAngles[ i ] );
		botViewAngles[ i ] += ( move * angMod );
	}
}

/*
================
idBot::Bot_ClientAimAtEnemy

Has the bot aim towards its target - could be a vehicle (with player inside) or a player on foot. Used for 
weapon aiming. Varies the aim based on the bots aim skill.
================
*/
void idBot::Bot_ClientAimAtEnemy( int clientNum ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		return;
	}

	trace_t	tr;
	bool hasRocket = ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.weapon == ROCKET ) ? true : false;
	bool hasGrenade = ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.weapon == GRENADE ) ? true : false;
	bool hasSniperWeap = ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].weapInfo.weapon == SNIPERRIFLE ) ? true : false;
	float bodyShot = 16.0f;
    idPlayer *player = gameLocal.GetClient( clientNum );
	SetSwayScale( 1.0f ); //mal: default...

	if ( player == NULL ) {
		return;
	}

	idVec3 vec = GetPlayerViewPosition( clientNum ) - firstPersonViewOrigin;
	float dist = vec.LengthSqr();
	float aim;
	clientInfo_t& playerInfo = botThreadData.GetGameWorldState()->clientInfo[ clientNum ];
	float enemySpeed = playerInfo.xySpeed;

	int aimSkill = bot_aimSkill.GetInteger();

	if ( botThreadData.GetBotSkill() == BOT_SKILL_DEMO ) {
		if ( !playerInfo.isBot ) { //mal: in training mode, have horrible aim against humans..
			aimSkill = 0;
		} else {
			aimSkill = 1; //mal: nomal aim against other bots.
		}
	}

	if ( aimSkill <= 1 && updateAimTime > gameLocal.time && !playerInfo.isBot && InBotsSights( playerInfo.origin ) ) {
		return;
	}

	updateAimTime = ( gameLocal.time + ( aimSkill == 0 ) ? 700 : 300 );

	if ( hasRocket || hasGrenade ) {
		if ( aimSkill == 0 ) {
			aim = 0.50f; 
		} else if ( aimSkill == 1 ) {
		    aim = 0.20f;
		} else if ( aimSkill == 2 ) {
		    aim = 0.15f; 
		} else { 
			aim = 0.0f;
		}
	} else {
		if ( aimSkill == 0 ) {
		   aim = ( dist > Square( 200.0f ) ) ? -0.55f : -0.40f; 
		} else if ( aimSkill == 1 ) {
			aim = ( dist > Square( 500.0f ) ) ? 0.35f : 0.20f;
		} else if ( aimSkill == 2 ) {
			aim = ( dist > Square( 1100.0f ) ) ? 0.20f : 0.15f; 
		} else { 
			aim = ( dist > Square( 1900.0f ) ) ? 0.10f : 0.05f; //mal: holy bullet holes Batman!
		}
	}

	if ( dist < Square( 100.0f ) && aimSkill > 0 && !hasRocket ) {
		aim = 0.0f;
	} //mal: if your REALLY close, they dont overshoot!

	if ( hasSniperWeap ) {
		if ( aimSkill == 1 ) {
			SetSwayScale( 0.70f );
		} else if ( aimSkill == 2 ) { 
			SetSwayScale( 0.50f );
			aim -= 0.05f;
		} else if ( aimSkill == 3 ) { 
			SetSwayScale( 0.10f );
			aim -= 0.05f;
		}
	}
     
	bool isPlayer = ( player->InVehicle() ) ? false : true;
	idVec3 velocity = ( isPlayer ) ? player->GetPhysics()->GetLinearVelocity() : player->proxyEntity->GetPhysics()->GetLinearVelocity();
	vec = ( isPlayer ) ? GetPlayerViewPosition( clientNum ) : player->proxyEntity->GetPhysics()->GetOrigin();

	if ( !isPlayer ) { //mal: for vehicles, the origin can be kinda low, so up it a bit.
		proxyInfo_t enemyVehicleInfo;	
		GetVehicleInfo( player->GetProxyEntity()->entityNumber, enemyVehicleInfo );

		if ( playerInfo.proxyInfo.weapon == MINIGUN && ( enemyVehicleInfo.type == MCP || enemyVehicleInfo.type == TITAN ) ) {
			vec = GetPlayerViewPosition( clientNum );

			if ( enemySpeed <= WALKING_SPEED ) {
				vec.x += ( float ) botThreadData.random.RandomInt( 55 );
				vec.y += ( float ) botThreadData.random.RandomInt( 55 );
			}
		} else {
			float vehicleOffset = botThreadData.GetVehicleTargetOffset( enemyVehicleInfo.type );
			vec.z += ( enemyVehicleInfo.bbox[ 1 ][ 2 ] - enemyVehicleInfo.bbox[ 0 ][ 2 ] ) * vehicleOffset;
		}
	}

//mal: low skill bots have a degree of "jitter", and wont keep on target too much. We dont care about other bots for this, only humans.
	if ( aimSkill < 1 && botThreadData.GetGameWorldState()->clientInfo[ player->entityNumber ].isBot == false && isPlayer && !hasRocket && !hasGrenade ) {
		if ( velocity.LengthSqr() < Square( 84.0f ) && dist > Square( 300.0f ) ) { //mal: if the player is on foot, and isn't moving much, we'll add a bit of jitter so we dont kill them always.
			if ( nextAimUpdateTime < gameLocal.time ) {
				lowSkillAimPoint.x = ( float ) botThreadData.random.RandomInt( 55 );
				lowSkillAimPoint.y = ( float ) botThreadData.random.RandomInt( 55 );
				lowSkillAimPoint.z = -( float ) botThreadData.random.RandomInt( 155 );
				nextAimUpdateTime = gameLocal.time + 500;
			} else {
				vec += lowSkillAimPoint;
			}
		}
	}

	if ( isPlayer && !hasRocket && !hasGrenade ) { ///mal: no need to scale our shots down if your in a vehicle
        if ( aimSkill < 2 ) {
			if ( aimSkill == 0 ) { //mal: NO headshots on n00bie skill
				vec.z -= bodyShot; //mal: scale the shot down a bit, so that its not headshots.
			} else if ( ( botThreadData.random.RandomInt( 100 ) < 97 || ( dist < Square( 500.0f ) && dist < Square( 1500.0f ) ) ) && !playerInfo.isLeaning && !playerInfo.usingMountedGPMG ) {
		        vec.z -= bodyShot; //mal: scale the shot down a bit, so that its not headshots.
			}
		} else if ( aimSkill < 3 ) {
			if ( ( botThreadData.random.RandomInt( 100 ) < 90 || dist < Square( 500.0f ) ) && !playerInfo.isLeaning && !playerInfo.usingMountedGPMG ) { //mal: will get more headshots on experienced aim setting!
				vec.z -= bodyShot; //mal: scale the shot down a bit, so that its not headshots.
			}
		} else {
			if ( dist > Square( 500.0f ) ) {
				if ( botThreadData.random.RandomInt( 100 ) < 50 && !playerInfo.isLeaning && !playerInfo.usingMountedGPMG ) { //mal: will get lots of headshots on expert aim setting!
					vec.z -= bodyShot; //mal: scale the shot down a bit, so that its not headshots.
				}
			}
		}
	}

	if ( hasRocket && isPlayer ) {
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, firstPersonViewOrigin, player->GetPhysics()->GetOrigin(), BOT_VISIBILITY_TRACE_MASK, this );

		if ( dist < Square( 1500.0f ) && tr.fraction == 1.0f || tr.c.entityNum == clientNum ) {
			vec = player->GetPhysics()->GetOrigin(); //mal: aim near the feet if they're fairly close, and we have a clear shot.
		} else {
			vec = GetPlayerViewPosition( clientNum ); //mal: aim for the head if they're far away.
			if ( aimSkill > 0 ) {
				Bot_GetProjectileAimPoint( PROJECTILE_ROCKET, vec, -1 );
			}
		}
	}

	if ( hasGrenade ) {
		Bot_GetProjectileAimPoint( PROJECTILE_GRENADE, vec, -1 );
	}

	if ( InBotsSights( vec ) ) {
		vec += ( aim * velocity );
        vec -= firstPersonViewOrigin;
		botViewAngles = vec.ToAngles();
	} else {
		vec -= firstPersonViewOrigin;
		Bot_ChangeViewAngles( vec.ToAngles(), false );
	}
}

/*
================
idBotAI::InBotsSights

Checks to see if the passed origin is in the bot's sights
================
*/
bool idBot::InBotsSights( const idVec3 &origin ) {
	idVec3 dir = origin - GetPhysics()->GetOrigin();

	dir.NormalizeFast();

	if ( dir * GetViewAxis()[0] > 0.8f ) {
		return true;   //mal: have someone in our "sights".
	}
	return false;
}

/*
================
idBotAI::DeadUcmds

Just checks to see if a medic is nearby, or if the team respawn time is nearly up, and has the bot tap out.
If a medic is nearby, the bot will wait for him ( nothing worse then somebody tapping out just as you reach them ).
================
*/
void idBot::DeadUcmds( usercmd_t &ucmd ) {
    if ( bot_noTapOut.GetBool() ) {
        ucmd.upmove = -127;
	} else {
        if ( gameLocal.rules->IsWarmup() ) {
            ucmd.upmove = 127;
		} else {
			int respawnTime = ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].team == GDF ) ? botThreadData.GetGameWorldState()->gameLocalInfo.nextGDFRespawnTime : botThreadData.GetGameWorldState()->gameLocalInfo.nextStroggRespawnTime;

			if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].botCmds.hasMedicInFOV || respawnTime > 2 ) {
				ucmd.upmove = -127;
			} else {
                ucmd.upmove = 127;
            }
		}
	}
}

/*
================
idBotAI::AngleUcmds

Translates its desired angles into user cmds the game can use.
================
*/
void idBot::AngleUcmds( usercmd_t &ucmd ) {
	idVec3	vec;
	idVec3  targetOrg;

	if ( bot_skipThinkClient.GetInteger() == entityNumber ) {
		return;
	}

	idAngles desiredAngles = GetPhysics()->GetLinearVelocity().ToAngles(); //mal: default view is always the current one.

	const botAIOutput_t& botOutput = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];
	const clientInfo_t& botInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	if ( botInfo.weapInfo.covertToolInfo.entNum != 0 && botInfo.weapInfo.covertToolInfo.clientIsUsing == true && botInfo.team == STROGG ) {
		botViewAngles = botOutput.moveViewAngles;

		idAngles deltaViewAngles;

		sdScriptEntity* camera = remoteCamera.GetEntity()->Cast< sdScriptEntity >();
		if ( camera != NULL ) {
			deltaViewAngles = camera->GetDeltaViewAngles();
		} else {
			deltaViewAngles = GetDeltaViewAngles();
		}

		ucmd.angles[0] = ANGLE2SHORT( botViewAngles[0] - deltaViewAngles[0] );
		ucmd.angles[1] = ANGLE2SHORT( botViewAngles[1] - deltaViewAngles[1] );
		ucmd.angles[2] = ANGLE2SHORT( botViewAngles[2] - deltaViewAngles[2] );
		return;
	}

	if ( botOutput.turnType == AIM_TURN )  { //mal: this only aims at clients, both on foot or in vehicles.
		assert( botOutput.viewType == VIEW_ENTITY );
		assert( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_CLIENTS );

		if ( botOutput.viewType == VIEW_ENTITY && ( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_CLIENTS ) ) {
			if ( botInfo.usingMountedGPMG ) {
				idEntity* mountedGPMG = GetProxyEntity();
				idPlayer* player = gameLocal.GetClient( botOutput.viewEntityNum );

				if ( mountedGPMG != NULL && player != NULL ) {
					idVec3 playerOrigin = player->GetPhysics()->GetOrigin();
					botViewAngles = mountedGPMG->GetUsableInterface()->GetRequiredViewAngles( this, playerOrigin );
				} else {
					idEntity* proxy = GetProxyEntity();
					if ( proxy != NULL ) {
						PerformImpulse( UCI_USE_VEHICLE );
						assert( false );
						return;
					}
				}
			} else {
				Bot_ClientAimAtEnemy( botOutput.viewEntityNum );
			}
		}
	} else {
		if ( botOutput.viewType == VIEW_ANGLES ) {
			desiredAngles = botOutput.moveViewAngles; //mal: instantly turn toward our target.
		} else if ( botOutput.viewType == VIEW_ORIGIN ) {
			if ( botOutput.botCmds.throwNade == false ) {
				desiredAngles = ( botOutput.moveViewOrigin - firstPersonViewOrigin ).ToAngles();
			} else {
				idVec3 aimPoint = botOutput.moveViewOrigin;
				Bot_GetProjectileAimPoint( PROJECTILE_GRENADE, aimPoint, -1 );
				desiredAngles = ( aimPoint - firstPersonViewOrigin ).ToAngles();
			}
		} else if ( botOutput.viewType == VIEW_REVERSE ) {
			desiredAngles = botOutput.moveViewAngles; //mal: look behind us when moving.
			desiredAngles[ YAW ] -= 180.0f;
		} else if ( botOutput.viewType == VIEW_MOVEMENT ) {
			desiredAngles = GetPhysics()->GetLinearVelocity().ToAngles();
		} else if ( botOutput.viewType == VIEW_RANDOM ) {
			idAngles curAngles = botViewAngles;
			curAngles.yaw += ( 45.0f * ( gameLocal.random.RandomInt( 100 ) > 50 ) ? 1 : -1 );
			desiredAngles = curAngles;
		} else if ( botOutput.viewType == VIEW_ENTITY ) {
			if ( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_CLIENTS ) {
				idPlayer *player = gameLocal.GetClient( botOutput.viewEntityNum );

				if ( player ) { //mal: want the bot to look the player in the eye.....
					if ( player->InVehicle() ) {
						targetOrg = player->GetProxyEntity()->GetPhysics()->GetOrigin();
						targetOrg[ 2 ] += 32.0f;
						vec = targetOrg - firstPersonViewOrigin;
					} else {
						vec = GetPlayerViewPosition( player->entityNumber ) - firstPersonViewOrigin;
					}

					desiredAngles = vec.ToAngles();
				} 
			} else {
				if ( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_GENTITIES ) {
					idEntity *entity = gameLocal.entities[ botOutput.viewEntityNum ];

					if ( entity ) {
						vec = entity->GetPhysics()->GetOrigin() - firstPersonViewOrigin;
						desiredAngles = vec.ToAngles();
					}
				}
			}
		} else {
			assert( !"Unhandled viewType" );
		}

		if ( botOutput.turnType == INSTANT_TURN ) {
			botViewAngles = desiredAngles;
		} else if ( botOutput.turnType == SMOOTH_TURN ) {
			Bot_ChangeViewAngles( desiredAngles, false );
		} else if ( botOutput.turnType == FAST_TURN ) {
			Bot_ChangeViewAngles( desiredAngles, true );
		}
	}

    if ( botOutput.botCmds.lookUp != false ) {
        botViewAngles[ PITCH ] = -89; //-90; // look straight up
	} else if ( botOutput.botCmds.lookDown != false ) {
		botViewAngles[ PITCH ] = 45; //mal: look down a bit so that we can run into our packs easier
	}

    const idAngles &deltaViewAngles = GetDeltaViewAngles();

	ucmd.angles[0] = ANGLE2SHORT( botViewAngles[0] - deltaViewAngles[0] );
	ucmd.angles[1] = ANGLE2SHORT( botViewAngles[1] - deltaViewAngles[1] );
	ucmd.angles[2] = ANGLE2SHORT( botViewAngles[2] - deltaViewAngles[2] );
}

/*
================
idBotAI::MoveUcmds

Translates its desired movement into user cmds the game can use.
================
*/
void idBot::MoveUcmds( usercmd_t &ucmd ) {
	clientInfo_t& botInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];

	if ( botInfo.weapInfo.covertToolInfo.entNum != 0 && botInfo.weapInfo.covertToolInfo.clientIsUsing == true && botInfo.team == STROGG ) {
		ucmd.forwardmove = 127;
		botMoveFlags_t moveFlag = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag;

		if ( moveFlag == JUMP_MOVE ) {
			ucmd.upmove = 127;
		} else if ( moveFlag == CROUCH ) {
			ucmd.upmove = -127;
		}

		return;
	}

	bool disableRun = false;
	bool disableSprint = false;
	idVec2 xyDir = GetPhysics()->GetLinearVelocity().ToVec2();
	float xySpeed = xyDir.Normalize(); 
	idVec3 origin = GetPhysics()->GetOrigin();
	idVec3 moveGoal1 = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal;
	idVec3 moveGoal2 = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal2;
	botMoveFlags_t moveFlag = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag;
	botMoveTypes_t moveType = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType;
	botMoveTypes_t specialMoveType = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].specialMoveType;
	bool hasGroundContacts = GetPhysics()->HasGroundContacts();

	botHackMoveAngles = botViewAngles;

    if ( moveGoal1 != vec3_zero ) {
		idVec3 moveGoal = moveGoal1;
		float speed = 128.0f;

		if ( moveType == LOCATION_JUMP ) {
			if ( !hasGroundContacts ) {
				// if we're already in the air, then go to the end point
				moveGoal = moveGoal2;
			} else {
				idVec3 curToStart = ( moveGoal1 - origin );
				idVec3 curToEnd = ( moveGoal2 - origin );
				if ( curToEnd.LengthSqr() < curToStart.LengthSqr() ) {
					// we are on the ground and closer to our end point that the start point, this likely means we landed but the bot thread hasn't updated our goal yet
					moveGoal = moveGoal2;
					disableRun = true;
				} else {
					idVec2 startToEnd = ( moveGoal2 - moveGoal1 ).ToVec2();
					float jumpDist = startToEnd.Normalize();
					float distToEnd = curToEnd.ToVec2().Length();
					float distToStart = curToStart.ToVec2().Length();
					idPlane p;
					p.FromPoints( moveGoal1, moveGoal2, moveGoal1 + gameLocal.GetGravity() );
					// note: 60 and 20 picked at random, 16 is 1/2 a bounding box, distToStart * 0.5 causes the bot to 'funnel in' to a jump smoothly
					if ( distToStart > 60.0f && fabs( p.Distance( origin ) ) > 16.0f ) {
						// need to line up for the jump, move to a point in line with the jump, but some units back
						moveGoal = moveGoal1 - idVec3( startToEnd.x, startToEnd.y, 0.0f ) * Min( Max( 20.0f, jumpDist ), distToStart * 0.5f );
						float horizontalDist = ( moveGoal1 - origin ).ToVec2().Length();
						if ( horizontalDist < 80.0f ) {
							speed = 8.0f + horizontalDist;
							disableRun = true;
						}
					} else {
						moveGoal = moveGoal2;
						if ( distToEnd < ( jumpDist + 9.0f ) ) {
							moveJumpNow = !moveJumpNow;
							if ( moveJumpNow ) { //mal: dont hold down jump for too long.
								ucmd.upmove = 127;
							}
						}
					}
				}
			}
		} else if ( moveType == LOCATION_BARRIERJUMP ) {
			float horizontalDist = ( moveGoal1 - origin ).ToVec2().Length();
			if ( horizontalDist < 24.0f ) {
				moveJumpNow = !moveJumpNow;
				if ( moveJumpNow ) {
					ucmd.upmove = 127;
				}
			}
			if ( hasGroundContacts && horizontalDist < 80.0f ) {
				speed = 8.0f + horizontalDist;
				disableRun = true;
			} else {
				moveGoal = moveGoal2;
			}
		} else if ( moveType == LOCATION_WALKOFFLEDGE ) {
			float horizontalDist = ( moveGoal1 - origin ).ToVec2().Length();
			disableSprint = true;
			if ( horizontalDist < 128.0f ) {
				speed = 64.0f + horizontalDist * 0.5f;
				//disableRun = true;
			}
		}

		idVec3 moveDir;
		moveDir[0] = moveGoal[0] - origin[0];
		moveDir[1] = moveGoal[1] - origin[1];
		moveDir[2] = 0.0f;
		moveDir.Normalize();

		idVec3 viewForward, viewRight;

		const idVec3 &gravityNormal = GetPhysics()->GetGravityNormal();

		botViewAngles.ToVectors( &viewForward, NULL, NULL );
		viewRight = gravityNormal.Cross( viewForward );

		viewForward -= ( viewForward * gravityNormal ) * gravityNormal;
		viewRight -= ( viewRight * gravityNormal ) * gravityNormal;

		if ( GetPlayerPhysics().IsGrounded() ) {
			viewForward = idPhysics_Player::AdjustVertically( GetPlayerPhysics().GetGroundNormal(), viewForward );
			viewRight = idPhysics_Player::AdjustVertically( GetPlayerPhysics().GetGroundNormal(), viewRight );
		}

		viewForward.Normalize();
		viewRight.Normalize();


#ifdef BOT_MOVE_LOOKUP
		// forward and right move
		idVec2 relativeMoveDir;
		relativeMoveDir.x = viewForward * moveDir;
		relativeMoveDir.y = viewRight * moveDir;

		float moveForward, moveRight;
		MoveDirectionToInput( relativeMoveDir, moveFlag, moveForward, moveRight );
		ucmd.forwardmove = FixUcmd( moveForward * speed );
		ucmd.rightmove = FixUcmd( moveRight * speed );
#else
		// forward and right move
		idVec2 relativeMoveDir;
		relativeMoveDir.x = ( viewForward * moveDir ) * speed;
		relativeMoveDir.y = ( viewRight * moveDir ) * speed;

		idPhysics_Player::SetupUsercmdForDirection( relativeMoveDir, pm_runspeedforward.GetFloat(), pm_runspeedback.GetFloat(), pm_runspeedstrafe.GetFloat(), ucmd );
#endif

		if ( OnLadder() ) {
			ucmd.rightmove = 0;
			ucmd.forwardmove = 127;
			moveType = NULLMOVETYPE;
			moveFlag = RUN;
		}
	}

    switch ( moveType ) {

		case RANDOM_JUMP: {
			if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
                if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
                    ucmd.upmove = 127;
				}
			}
			break;
		}

		case QUICK_JUMP: {
            if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
				if ( xySpeed > WALKING_SPEED ) {
                    if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
						ucmd.upmove = 127;
					}
				}
			}
			break;
		}

		case STRAFE_RIGHT: {
            ucmd.rightmove = 127;
			break;
		}

		case STRAFE_LEFT: {
            ucmd.rightmove = -127;
			break;
		}
		
		case BACKSTEP: {
			ucmd.forwardmove = -127;
			break;
		}

		case RANDOM_JUMP_RIGHT: {
			if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
				if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
					ucmd.upmove = 127;
				}
			}

			ucmd.rightmove = 127;
			break;
		}

		case RANDOM_JUMP_LEFT: {
			if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
				if ( botThreadData.random.RandomInt( 100 ) > 95 ) {
					ucmd.upmove = 127;
				}
			}
			
			ucmd.rightmove = -127;
			break;
	   }

#ifndef _XENON
		case LEAN_LEFT: {
			ucmd.buttons.btn.leanLeft = true;
			break;
		}

		case LEAN_RIGHT: {
			ucmd.buttons.btn.leanRight = true;
			break;
		}
#endif
		
		case RANDOM_DIR_JUMP: {
			if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
				ucmd.upmove = 127;
			}

			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				ucmd.rightmove = 127;
			} else {
				ucmd.rightmove = -127;
			}

			if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				ucmd.forwardmove = 127;
			} else {
				ucmd.forwardmove = -127;
			}
		}

		default: {
			break;
		}
	}

    ucmd.buttons.btn.run = true; //mal: this will be true by default

	switch( moveFlag ) {
#ifndef _XENON
		case PRONE: {
			if ( !IsProne() ) {
				if ( proneDelay < gameLocal.time ) {
                    proneDelay = gameLocal.time + 700;
                    PerformImpulse( UCI_PRONE );
				}
			}
			break;
		}
#else 
		case PRONE: { //mal: on the consoles, any call to prone, becomes a call to crouch ( the anims are being removed to save memory ).
			ucmd.upmove = -127;
			break;
		}
#endif

		case CROUCH: {
            ucmd.upmove = -127;
			break;
		}

		case WALK: {
            ucmd.buttons.btn.run = false;
			break;
		}
		
		case SPRINT: {
			ucmd.buttons.btn.sprint = true;
			break;
		}

		case STRAFEJUMP: { //mal: moving like the l33t players.... 
            ucmd.buttons.btn.sprint = true;

			if ( xySpeed > SPRINTING_SPEED - 5.0f ) { //mal: give ourselves some time to pick up some speed...
                if ( hasGroundContacts ) {
					jumpTimer++;
				}

				leftyTimer++;
            
				if ( jumpTimer >= 10 ) {
					ucmd.upmove = 127;
					jumpTimer = 0;
				} else {
					ucmd.upmove = 0;
				}

				if ( leftyTimer >= 5 ) { //mal: this used to be 2.
					lefty = !lefty;
					leftyTimer = 0;
				}

				if ( lefty ) {
					ucmd.rightmove = -127;
				} else {
					ucmd.rightmove = 127;
				}
			}
			break;
		}
		
		case JUMP_MOVE: {
			if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
				ucmd.upmove = 127;
			}
			break;
		}
			
		default: {
            break;
		}
	}

	if ( specialMoveType == QUICK_JUMP ) {
		if ( !bot_noRandomJump.GetBool() && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
			if ( botThreadData.random.RandomInt( 100 ) > 75 ) {
				ucmd.upmove = 127;
			}
		}
	}

	if ( disableRun ) {
		ucmd.buttons.btn.sprint = false;
		ucmd.buttons.btn.run = false;
	}

	if ( disableSprint ) {
		ucmd.buttons.btn.sprint = false;
	}

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_debugSpeed.GetInteger() == entityNumber ) {
			gameLocal.Printf("Bot Speed = %f\n", botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].xySpeed );
		}
	}
}

#ifdef BOT_MOVE_LOOKUP

/*
================
idBot::MoveDirectionToInput
================
*/
void idBot::MoveDirectionToInput( const idVec2& localMoveDir, botMoveFlags_t moveFlags, float& moveForward, float& moveRight ) {

	if ( lastRunFwd == -1.0f ) {
		// the lookup tables haven't been setup yet. bad!
		moveForward = 0.0f;
		moveRight = 0.0f;
	} else {
		if ( moveFlags == PRONE || moveFlags == CROUCH || moveFlags == WALK ) {
			// directional movements are linear in walk, crouch & prone
			moveForward = localMoveDir.x;
			moveRight = localMoveDir.y;

			// scale it out from a circle to a square
			float absX = idMath::Fabs( localMoveDir.x );
			float absY = idMath::Fabs( localMoveDir.y );
			if ( absX > idMath::FLT_EPSILON && absX > absY ) {
				moveForward /= absX;
				moveRight /= absX;
			} else if ( absY > idMath::FLT_EPSILON && absY > absX ) {
				moveForward /= absY;
				moveRight /= absY;
			}
		} else if ( localMoveDir.x > idMath::FLT_EPSILON && ( moveFlags == SPRINT || moveFlags == STRAFEJUMP ) ) {
			// sprinting
			idVec3 moveDir( localMoveDir.x, localMoveDir.y, 0.0f );
			idAngles moveAngles = moveDir.ToAngles();
			moveAngles.Normalize360();

			int index = idMath::Ftoi( moveAngles.yaw * BOT_MOVE_LOOKUP_SCALE + 0.5f ) % BOT_MOVE_LOOKUP_STEPS;
			moveForward = sprintMoveForward[ index ] / 127.0f;
			moveRight = sprintMoveRight[ index ] / 127.0f;
		} else {
			// running
			idVec3 moveDir( localMoveDir.x, localMoveDir.y, 0.0f );
			idAngles moveAngles = moveDir.ToAngles();
			moveAngles.Normalize360();

			int index = idMath::Ftoi( moveAngles.yaw * BOT_MOVE_LOOKUP_SCALE + 0.5f ) % BOT_MOVE_LOOKUP_STEPS;
			moveForward = runMoveForward[ index ] / 127.0f;
			moveRight = runMoveRight[ index ] / 127.0f;
		}
	}
}

/*
================
idBot::BuildMoveLookups
================
*/
void idBot::BuildMoveLookups( float runFwd, float runSide, float runBack, float sprintFwd, float sprintSide ) {
	if ( runFwd != lastRunFwd || runSide != lastRunSide || runBack != lastRunBack ) {
		// run speed cvars have been updated
		BuildMoveLookup( runMoveForward, runMoveRight, runFwd, runSide, runBack );

		lastRunFwd = runFwd;
		lastRunSide = runSide;
		lastRunBack = runBack;
	}

	if ( sprintFwd != lastSprintFwd || sprintSide != lastSprintSide ) {
		// sprint speed cvars have been updated
		BuildMoveLookup( sprintMoveForward, sprintMoveRight, sprintFwd, sprintSide, runBack );

		lastSprintFwd = sprintFwd;
		lastSprintSide =  sprintSide;
	}
}

/*
================
idBot::BuildMoveLookup
================
*/
void idBot::BuildMoveLookup( moveLookupArray_t& moveForward, moveLookupArray_t& moveRight, 
							float fwdSpeed, float sideSpeed, float backSpeed ) {

	// init values to magic number that isn't possible
	for ( int i = 0; i < BOT_MOVE_LOOKUP_STEPS; i++ ) {
		moveForward[ i ] = 666;
		moveRight[ i ] = 666;
	}

	// don't bother with intermediate values - just the maxed out ones.
	// do the left & right sides
	for ( int fwd = -127; fwd <= 127; fwd++ ) {
		for ( int right = -127; right <= 127; right += 254 ) {

			idVec2 resultDirection;
			idPhysics_Player::CalcDesiredWalkMove( fwd, right, fwdSpeed, sideSpeed, backSpeed, resultDirection );

			// normalize to get resultant direction
			resultDirection.Normalize();

			// calculate the angle that this is from
			float angle = RAD2DEG( idMath::ATan( resultDirection.y, resultDirection.x ) );
			if ( angle < 0.0f ) {
				angle += 360.0f;
			}

			// stick it in the table
			int index = idMath::Ftoi( angle * BOT_MOVE_LOOKUP_SCALE + 0.5f ) % BOT_MOVE_LOOKUP_STEPS;
			moveForward[ index ] = fwd;
			moveRight[ index ] = right;
		}
	}

	// do the top & bottom sides
	for ( int right = -126; right <= 126; right++ ) {
		for ( int fwd = -127; fwd <= 127; fwd += 254 ) {

			idVec2 resultDirection;
			idPhysics_Player::CalcDesiredWalkMove( fwd, right, pm_runspeedforward.GetFloat(), pm_runspeedstrafe.GetFloat(), pm_runspeedback.GetFloat(), resultDirection );

			// normalize to get resultant direction
			resultDirection.Normalize();

			// calculate the angle that this is from
			float angle = RAD2DEG( idMath::ATan( resultDirection.y, resultDirection.x ) );
			if ( angle < 0.0f ) {
				angle += 360.0f;
			}

			// stick it in the table
			int index = idMath::Ftoi( angle * BOT_MOVE_LOOKUP_SCALE + 0.5f ) % BOT_MOVE_LOOKUP_STEPS;
			moveForward[ index ] = fwd;
			moveRight[ index ] = right;
		}
	}

	// see what angles aren't served
	for ( int i = 0; i < BOT_MOVE_LOOKUP_STEPS; i++ ) {
		if ( moveForward[ i ] == 666 || moveRight[ i ] == 666 ) {
			moveForward[ i ] = moveForward[ i - 1 ];
			moveRight[ i ] = moveRight[ i - 1 ];
		}
	}
}

#endif

/*
================
idBotAI::ActionUcmds

Translates its desired actions into user cmds the game can use.
================
*/
void idBot::ActionUcmds( usercmd_t &ucmd ) {
	bool weaponReady = false;
	bool hasIronSightsOn;
	idWeapon* realWeapon = weapon.GetEntity();
	clientInfo_t &clientInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];
	const botAIOutput_t &clientUcmd = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];

	if ( clientInfo.classType == COVERTOPS && ( clientInfo.weapInfo.primaryWeapon == SMG || clientInfo.weapInfo.primaryWeapon == SNIPERRIFLE ) ) {
		hasIronSightsOn = clientInfo.weapInfo.isScopeUp;
	} else if ( clientInfo.classType == SOLDIER && clientInfo.weapInfo.primaryWeapon == ROCKET ) {
		hasIronSightsOn = clientInfo.weapInfo.isScopeUp;
	} else {
        hasIronSightsOn = clientInfo.weapInfo.isIronSightsEnabled;
	}
    
	if ( realWeapon ) {
        if ( realWeapon->IsReady() ) {
            weaponReady = true;
		}
	}
	
	if ( clientUcmd.botCmds.attack ) {
		if ( clientUcmd.botCmds.constantFire || ( clientInfo.classType == SOLDIER && clientInfo.weapInfo.weapon == PISTOL && clientInfo.team == STROGG ) ) {
            ucmd.buttons.btn.attack = true;
		} else {
			if ( weaponReady && clientInfo.weapInfo.weapon != PISTOL && clientInfo.weapInfo.weapon != SCOPED_SMG ) {
                ucmd.buttons.btn.attack = true; 
			} else {
                firePistol = !firePistol;
				ucmd.buttons.btn.attack = firePistol; 
			}
		}
	}

#ifndef _XENON
	if ( clientUcmd.botCmds.topHat ) {
		ucmd.buttons.btn.tophat = true;
	}
#endif

	if ( clientUcmd.botCmds.launchPacks ) {
		if ( clientInfo.weapInfo.weapon == HEALTH || clientInfo.weapInfo.weapon == AMMO_PACK ) {
			firePistol = !firePistol;
			ucmd.buttons.btn.attack = firePistol;
		}
	}
	
	if ( clientUcmd.botCmds.reload ) {
		Reload();
	}

	if ( clientUcmd.botCmds.droppingSupplyCrate ) {
		botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].supplyCrateRequestTime = gameLocal.time + SUPPLY_CRATE_DELAY_TIME;
	}

	if ( clientUcmd.botCmds.destroySupplyCrate ) {
		if ( clientInfo.supplyCrate.entNum != 0 ) {
			idEntity *supplyCrate = gameLocal.entities[ clientInfo.supplyCrate.entNum ];

			if ( supplyCrate != NULL ) {
				supplyCrate->Damage( NULL, NULL, idVec3( 0.0f, 0.0f, 1.0f ), DAMAGE_FOR_NAME( "damage_grenade_frag_splash" ), 999.0f, NULL );
			}
		}
	}

	if ( clientUcmd.botCmds.honkHorn == true || hornTime > gameLocal.time ) {
		ucmd.buttons.btn.modeSwitch = true;

		if ( hornTime < gameLocal.time ) {
            hornTime = gameLocal.time + 1500;
		}
	}

	if ( clientUcmd.botCmds.shoveClient ) {
		PerformImpulse( UCI_USE_VEHICLE );
	}


	if ( clientUcmd.botCmds.dropDisguise ) {
		DropDisguise();
		const_cast<bool &>( clientUcmd.botCmds.dropDisguise ) = false;
	}

	if ( clientUcmd.botCmds.godMode ) {
		SetGodMode( true );
	}

    if ( clientUcmd.botCmds.activate != false && clientUcmd.botCmds.activateHeld == false ) {
        firePliers = !firePliers;
        ucmd.buttons.btn.activate = firePliers;
	} else if ( clientUcmd.botCmds.activateHeld != false ) {
        ucmd.buttons.btn.activate = true;
	}

	if ( IsProne() && clientUcmd.moveFlag != PRONE ) { //mal: on your feet soldier!
		ucmd.upmove = 127;
	}

	if ( clientUcmd.botCmds.suicide != false ) {
		const_cast<bool &>( clientUcmd.botCmds.suicide ) = false;
		botThreadData.GetBotOutputState()->botOutput[ entityNumber ].botCmds.suicide = false;
        Suicide();
	}

	if ( clientUcmd.botCmds.enterVehicle != false ) {
		idEntity* proxy = GetProxyEntity();
		if ( proxy == NULL ) {
			PerformImpulse( UCI_USE_VEHICLE );
		}            
	}

	if ( clientUcmd.botCmds.switchAwayFromSniperRifle ) {
		const sdDeclPlayerClass* pc;
		if ( clientInfo.team == GDF ) {
			pc = gameLocal.declPlayerClassType[ "covertops" ];
		} else {
			pc = gameLocal.declPlayerClassType[ "infiltrator" ];
		}

		ChangeClass( pc, 0 );
	}

	if ( clientUcmd.botCmds.exitVehicle != false && botNextVehicleCmdTime < gameLocal.time ) {
        idEntity* proxy = GetProxyEntity();

		if ( proxy != NULL ) {
			PerformImpulse( UCI_USE_VEHICLE );
			botNextVehicleCmdTime = gameLocal.time + NEXT_VEHICLE_CMD_TIME;
		}
	}

	if ( clientUcmd.botCmds.altFire != false ) {
		ucmd.buttons.btn.altAttack = true;
	}

	if ( clientUcmd.botCmds.altAttackOn != false && altAttackDelay < gameLocal.time ) {
		if ( !hasIronSightsOn ) {
			if ( realWeapon != NULL ) {
				if ( gameLocal.random.RandomInt( 100 ) > 50 ) {
					ucmd.buttons.btn.altAttack = true;
				}
				if ( clientInfo.weapInfo.weapon == SNIPERRIFLE ) {
                    ucmd.buttons.btn.attack = false; //mal: dont shoot till you get your scope up!
					altAttackDelay = gameLocal.time + 900;
				}
			}
		} 
	}

	bool needPause;

	if ( clientUcmd.deployInfo.deployableType != NULL_DEPLOYABLE && clientInfo.deployDelayTime < gameLocal.time ) {
		if ( !botThreadData.RequestDeployableAtLocation( entityNumber, needPause ) && !needPause ) {
			clientInfo.deployDelayTime = gameLocal.time + 10000;
		}
	}

	if ( hasIronSightsOn != false && ( clientUcmd.botCmds.altAttackOff == true || !clientUcmd.botCmds.altAttackOn ) && altAttackDelay < gameLocal.time ) {
        if ( realWeapon != NULL ) {
			ucmd.buttons.btn.altAttack = true;
			altAttackDelay = gameLocal.time + 500;
		}
	}

	if ( ucmd.buttons.btn.attack != false ) {
		if ( hasIronSightsOn != false && clientInfo.classType == COVERTOPS && clientInfo.weapInfo.weapon == SCOPED_SMG ) {
			if ( botThreadData.random.RandomInt( 100 ) > 70 ) {
				ucmd.buttons.btn.attack = false;
			}
		}
	}

	if ( clientInfo.needsParachute ) { //mal: OH NOES!1 WE'RE FALLING TO OUR DOOM!
		ucmd.buttons.btn.activate = true; //mal: trigger our chute.
	}

#ifndef _XENON
	if ( clientUcmd.moveType == LEAN_LEFT ) {
        ucmd.buttons.btn.leanLeft = true;
	}

	if ( clientUcmd.moveType == LEAN_RIGHT ) {
        ucmd.buttons.btn.leanRight = true;
	}
#endif
}

/*
================
idBotAI::VehicleUcmds

Translates its desired actions into user cmds the game can use. Vehicle specific
================
*/
void idBot::VehicleUcmds( usercmd_t &ucmd ) {

	proxyInfo_t vehicleInfo;
	clientInfo_t &clientInfo = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ];
	const botAIOutput_t &clientUcmd = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];

	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

	GetVehicleInfo( GetProxyEntity()->entityNumber, vehicleInfo );

	if ( vehicleInfo.type > ICARUS ) { 
		AirVehicleControls( ucmd );
	} else if ( vehicleInfo.type == ICARUS ) { //mal: special enough to get its own vehicle control scheme.
		IcarusVehicleControls( ucmd );
	} else {
		GroundVehicleControls( ucmd );
	}

	if ( clientUcmd.botCmds.attack ) {
		ucmd.buttons.btn.attack = true;
	}
	
	if ( clientUcmd.botCmds.honkHorn == true || hornTime > gameLocal.time ) {
		if ( gameLocal.random.RandomInt( 100 ) > 20 ) {
			ucmd.buttons.btn.modeSwitch = true;
		}

		if ( hornTime < gameLocal.time ) {
            hornTime = gameLocal.time + 1500;
		}
	}

	//mal: desecrators can slide side to side...
#ifndef _XENON
	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == LEAN_LEFT ) {
		ucmd.buttons.btn.leanLeft = true;
	} else if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == LEAN_RIGHT ) {
		ucmd.buttons.btn.leanRight = true;
	}
#endif

	if ( clientUcmd.botCmds.suicide != false ) {
		const_cast<bool &>( clientUcmd.botCmds.suicide ) = false;
		botThreadData.GetBotOutputState()->botOutput[ entityNumber ].botCmds.suicide = false;
        Suicide();
	}

	if ( clientUcmd.botCmds.becomeDriver ) {
		if ( transport->GetUsableInterface()->GetPlayerAtPosition( 0 ) == NULL ) { //mal: safety check!
			transport->ForcePlacement( this, 0, proxyPositionId, false );
		}
	}

#ifndef _XENON
	if ( clientUcmd.botCmds.topHat ) {
		ucmd.buttons.btn.tophat = true;
	}
#endif

	if ( clientUcmd.botCmds.becomeGunner ) {
		if ( transport->GetUsableInterface()->GetNumPositions() > 1 ) {
			if ( transport->GetUsableInterface()->GetPlayerAtPosition( 1 ) == NULL ) { //mal: safety check!
				transport->ForcePlacement( this, 1, proxyPositionId, false );
			}
		}
	}

	if ( clientUcmd.moveType == TAUNT_MOVE ) {
		if ( vehicleInfo.type == GOLIATH ) { //mal: add more for other vehicles? ( most other vehicles can't really express disdain for their enemy..... ).
			ucmd.upmove = 127;
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
		}
	}

	if ( clientUcmd.botCmds.switchVehicleWeap && botNextWeapTime < gameLocal.time ) {
		transport->NextWeapon( this );
		botNextWeapTime = gameLocal.time + 1000;
	}

	if ( clientUcmd.botCmds.exitVehicle && botNextVehicleCmdTime < gameLocal.time ) {
		idEntity* proxy = GetProxyEntity();

		if ( proxy != NULL ) {
			PerformImpulse( UCI_USE_VEHICLE );
			botNextVehicleCmdTime = gameLocal.time + NEXT_VEHICLE_CMD_TIME;
		}
	}

	if ( clientUcmd.botCmds.enterSiegeMode != false ) {
		if ( transport->GetVehicleControl() ) {
			if ( !transport->GetVehicleControl()->InSiegeMode() && botNextSiegeCmdTime < gameLocal.time ) {
                ucmd.upmove = -127;
				botNextSiegeCmdTime = gameLocal.time + 5000;
			}
		}
	}

	if ( clientUcmd.botCmds.launchDecoys == true && decoyTime < gameLocal.time ) {
		if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
			decoyTime = gameLocal.time + 5000;
			PerformImpulse( UCI_WEAP0 );
		}
	}

	if ( clientUcmd.botCmds.launchDecoysNow == true && decoyTime < gameLocal.time ) { //mal: a "I need decoys now!" version of the above.
		if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) {
			decoyTime = gameLocal.time + 1000;
			PerformImpulse( UCI_WEAP0 );
		}
	}

	if ( clientUcmd.botCmds.exitSiegeMode != false ) {
		if ( transport->GetVehicleControl() && transport->GetVehicleControl()->InSiegeMode() ) {
			ucmd.upmove = 127;
		}
	}

	if ( botThreadData.GetBotSkill() > BOT_SKILL_EASY && botThreadData.GetBotSkill() != BOT_SKILL_DEMO ) { //mal: stupid bots wont worry about flares.
		if ( clientInfo.enemyHasLockon || ( decoyTime != 0 && decoyTime > gameLocal.time - 5000 ) ) { //mal: if decide to fire one decoy, fire many, just to distract any more incoming missiles.
            if ( decoyTime == 0 ) {
				decoyTime = gameLocal.time + ( ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL || botThreadData.GetBotSkill() == BOT_SKILL_DEMO ) ? 900 : 600 /*350*/ ); //mal: add a bit of reaction time between when bot gets warning, to when it reacts.
			}
			if ( decoyTime < gameLocal.time ) {
                if ( botThreadData.GetBotSkill() == BOT_SKILL_NORMAL || botThreadData.GetBotSkill() == BOT_SKILL_DEMO ) {
					if ( botThreadData.random.RandomInt( 100 ) > 97 ) { //mal: sometimes, if we're only mid skilled, we'll "forget" to fire the decoy
						PerformImpulse( UCI_WEAP0 );
					}
				} else {
                    PerformImpulse( UCI_WEAP0 );
				}
			}
		} else {
			decoyTime = 0;
		}
	}
}

/*
==================
idBot::GetVehicleInfo

Returns all the info about a particular vehicle.
==================
*/
void idBot::GetVehicleInfo( int entNum, proxyInfo_t& vehicleInfo ) {

	int i;

	vehicleInfo.entNum = 0;

	for( i = 0; i < MAX_VEHICLES; i++ ) {

		if ( botThreadData.GetGameWorldState()->vehicleInfo[ i ].entNum != entNum ) {
			continue;
		}

		vehicleInfo = botThreadData.GetGameWorldState()->vehicleInfo[ i ];
		break;
	}
}

/*
================
idBotAI::GroundVehicleControls
================
*/
void idBot::GroundVehicleControls( usercmd_t &ucmd ) {
	bool goliathMustJump = false;
	bool useBrakes = ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == HAND_BRAKE || botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == FULL_STOP ) ? true : false;
	bool isBlocked = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].botCmds.isBlocked;
	bool turnInPlace = false;
	bool takeHardTurns = false;
	bool canHandleJumpsWell = true;
	bool canSprint = true;
	bool canSprintFast = true;
	bool shouldSprint = false;
	bool shouldMoveSlowly = ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].specialMoveType == SLOWMOVE ) ? true : false;
	bool aimFast = ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].turnType == AIM_TURN ) ? true : false;
	float lookAheadDist;

	bool useBoost = ( shouldMoveSlowly == false && useBrakes == false ) ? true : false; //mal: running the vehicles with boost now...

	proxyInfo_t vehicleInfo;
	const botAIOutput_t& botOutput = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];
	idVec3	vec, end;

	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

	if ( transport == NULL ) { //mal: paranoid safety check
		gameLocal.Warning("Bot client #: %i thinks its on a vehicle, but its not!!!", entityNumber );
		return;
	}

	GetVehicleInfo( transport->entityNumber, vehicleInfo );

//mal: setup some basic variables for vehicle handling.
	switch( vehicleInfo.type ) {   
		case HUSKY: 
			lookAheadDist = 100.0f;
			takeHardTurns = true;
			break; 

		case BADGER:
			lookAheadDist = 150.0f; 
			canHandleJumpsWell = false;
			canSprintFast = false;
			takeHardTurns = true;
			break;

		case TITAN:
			lookAheadDist = 250.0f;
			turnInPlace = true;
			break;

		case HOG:
			lookAheadDist = 175.0f;
			takeHardTurns = true;
			canHandleJumpsWell = false; 
			break;

		case GOLIATH:
			lookAheadDist = 250.0f;
			turnInPlace = true;
			canSprint = false;
			break;

		case DESECRATOR:
			lookAheadDist = 225.0f;
			turnInPlace = true;
			break;

		case MCP:
			lookAheadDist = 250.0f;
			turnInPlace = true;
			canSprint = false;
			break;

		case PLATYPUS: //mal: ok, so its a water vehicle, but it has the same control constraints, so.....
			lookAheadDist = 125.0f;
			break;

		case TROJAN:
			lookAheadDist = 175.0f;
			break;

		default:
			lookAheadDist = 100.0f;
			break;
	}
 
    if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveViewOrigin != vec3_zero ) {
		idVec3 aimPoint = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveViewOrigin;

		if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].botCmds.useTankAim ) {
			Bot_GetProjectileAimPoint( PROJECTILE_TANK_ROUND, aimPoint, -1 );
		}

		Bot_ChangeVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( aimPoint ), aimFast );
	} else {
        if ( botOutput.viewEntityNum > -1 && botOutput.viewEntityNum < MAX_CLIENTS ) {
        	idPlayer *player = gameLocal.GetClient( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum );

			if ( player != NULL ) {
				Bot_VehicleAimAtEnemy( player->entityNumber );
			}
		} else {
			if ( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_GENTITIES ) {
				idEntity *entity = gameLocal.entities[ botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum ];

				if ( entity != NULL ) {
					idVec3 aimPoint = entity->GetPhysics()->GetOrigin();

					if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].botCmds.useTankAim ) {
						aimPoint.z += ( entity->GetPhysics()->GetBounds()[ 1 ][ 2 ] - entity->GetPhysics()->GetBounds()[ 0 ][ 2 ] ) * 0.4f;
						Bot_GetProjectileAimPoint( PROJECTILE_TANK_ROUND, aimPoint, entity->entityNumber );
					}
		
					Bot_ChangeVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( aimPoint ), aimFast );
				}
			}
		}
	}

	const idAngles &deltaViewAngles = GetDeltaViewAngles();

 	ucmd.angles[0] = ANGLE2SHORT( botViewAngles[0] - deltaViewAngles[0] );
	ucmd.angles[1] = ANGLE2SHORT( botViewAngles[1] - deltaViewAngles[1] );
	ucmd.angles[2] = ANGLE2SHORT( botViewAngles[2] - deltaViewAngles[2] );

	ucmd.buttons.btn.run = true; //mal: this will be true by default
	
	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal != vec3_zero ) {
        idVec3 moveDir;
	
		idVec3 origin = transport->GetRenderEntity()->origin;
		idVec3 forward = transport->GetRenderEntity()->axis[ 0 ];
		idVec3 right = transport->GetRenderEntity()->axis[ 1 ] * -1;

		float moveRight, moveForward;

		moveDir[0] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[0] - origin[0];
		moveDir[1] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[1] - origin[1];
		moveDir[2] = 0.0f;

		moveDir.Normalize();

		moveForward = ( forward * moveDir ) * 128;
		moveRight = ( right * moveDir ) * 128;


		if ( InVehicleGunSights( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal, true ) ) {
			ucmd.rightmove = FixUcmd( moveRight );
		} else {
			if ( moveRight > 0 ) {
				ucmd.rightmove = 127;
			} else if ( moveRight < 0 ) { 
				ucmd.rightmove = -127; 
			}
		}

		if ( vehicleInfo.type == GOLIATH && vehicleInfo.inSiegeMode ) {
			goliathMustJump = true;
			ucmd.upmove = 127;
		}

		ucmd.forwardmove = FixUcmd( moveForward );

		if ( vehicleInfo.forwardSpeed < 0 && ucmd.forwardmove > 0 ) { 
			ucmd.forwardmove = 127;
		}			

		vec = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal - origin;

		end = transport->GetRenderEntity()->origin;
		end += ( lookAheadDist * transport->GetRenderEntity()->axis[ 0 ] ); 

		if ( isBlocked ) {
			if ( turnInPlace ) {
				if ( ucmd.rightmove > 0 ) {
					ucmd.rightmove = 127;
				} else if ( ucmd.rightmove < 0 ) {
					ucmd.rightmove = -127;
				}
				ucmd.forwardmove = 0; 
			}
		} else {  
			if ( takeHardTurns && movingForward > 60 ) {
				end = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal - transport->GetRenderEntity()->origin;
				end.NormalizeFast();

				if ( end * transport->GetRenderEntity()->axis[ 0 ] < 0.4f ) { 
					useBrakes = true;
				}
			}

			ucmd.forwardmove = 127; 
		}

		if ( !canHandleJumpsWell ) { 
			if ( vehicleInfo.isFlipped ) {
				ucmd.forwardmove = -127;
				useBrakes = true;
			}
		}

		if ( ucmd.forwardmove >= 127 && botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].xySpeed > 150.0f ) {
			movingForward++;
		} else {
			movingForward = 0;
		}

		if ( useBrakes && vehicleInfo.type != GOLIATH ) {//mal: hit the brakes!
			ucmd.upmove = 127;
		} else {
			if ( canSprint ) {
				if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag == SPRINT || botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag == SPRINT_ATTACK ) {
                    if ( vec.LengthSqr() > Square( 500.0f ) || botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag == SPRINT_ATTACK ) {
						shouldSprint = true;
					}
				}

				if ( ucmd.rightmove < 64 && ucmd.rightmove > -64 && shouldSprint && !isBlocked ) { //mal: if we're not actively turning, gun it!
					if ( canSprintFast ) {
                        ucmd.buttons.btn.sprint = true;
					} else {

						jumpTimer++;

						if ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].xySpeed < 1100.0f ) {
							if ( jumpTimer > 60 ) { 
                                ucmd.buttons.btn.sprint = true;
							}
						} else {
							jumpTimer = 0;
						}
					}
				}
			}
		}   

		if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag == REVERSE ) {
			ucmd.forwardmove = -127;
		}

		if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].specialMoveType == REVERSEMOVE ) {
			ucmd.forwardmove = -127;

//			if ( vehicleInfo.type != GOLIATH ) {
//				ucmd.rightmove = 127 * ( ucmd.rightmove > 0 ) ? -1 : 1;
//			}
		}

		if ( transport->GetVehicleControl() && transport->GetVehicleControl()->InSiegeMode() ) { //mal: if we're in siege mode, and have moving orders, get out of siege mode!
			ucmd.upmove = 127;
		}
	} else {
		if ( useBrakes && vehicleInfo.type != GOLIATH ) {
			ucmd.upmove = 127;
		}

        if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == FULL_STOP ) {
			if ( vehicleInfo.forwardSpeed > 50.0f ) {
				ucmd.forwardmove = -127;
			} else if ( vehicleInfo.forwardSpeed < -50.0f ) {
				ucmd.forwardmove = 127; //mal: do the opposite of whatever we're doing now, so we slow down faster.
			}
		}
	}

	if ( shouldMoveSlowly ) {
		if ( vehicleInfo.type == TITAN || vehicleInfo.type == DESECRATOR ) {
			ucmd.forwardmove = 84;
		} else if ( vehicleInfo.forwardSpeed > 250.0f ) {
			ucmd.forwardmove = 64;
			if ( vehicleInfo.forwardSpeed > 600.0f ) {
				ucmd.upmove = 127;
				ucmd.forwardmove = -127;
			}
		}
	}

	if ( !useBoost && botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveFlag != SPRINT_ATTACK ) {
		ucmd.buttons.btn.sprint = false;
	}

	if ( vehicleInfo.type == GOLIATH && !goliathMustJump ) {
		ucmd.upmove = 0;
	}
	
	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].specialMoveType == FULL_STOP ) {
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		ucmd.buttons.btn.sprint = false;

		if ( vehicleInfo.type != GOLIATH ) {
			ucmd.upmove = 127;
		}

		if ( vehicleInfo.forwardSpeed > 50.0f ) {
			ucmd.forwardmove = -127;
		} else if ( vehicleInfo.forwardSpeed < -50.0f ) {
			ucmd.forwardmove = 127; //mal: do the opposite of whatever we're doing now, so we slow down faster.
		}
	}

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_debugGroundVehicles.GetInteger() == entityNumber ) {
			common->Printf( "Brake = %i, Sprint = %i, Speed = %.0f, Blocked = %i, Forward = %i\n", useBrakes, ucmd.buttons.btn.sprint, botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].xySpeed, isBlocked, ucmd.forwardmove );
		}
	}
}

/*
================
idBot::Bot_ChangeVehicleViewAngles

Turns the bot towards its idealAngle. Cheap, and produces very smooth angle changing when in vehicles.
================
*/
void idBot::Bot_ChangeVehicleViewAngles( const idAngles &idealAngle, bool fast ) {

	int i;
	float move;
	float angMod = ( fast == true ) ? 1.0f / 2.0f : 1.0f / 12.0f;

	for( i = 0; i < 3; i++ ) {
        move = idMath::AngleDelta( idealAngle[ i ], botViewAngles[ i ] );
		botViewAngles[ i ] += ( move * angMod );
	}
}

/*
================
idBot::Bot_ChangeAirVehicleViewAngles

Turns the bot towards its idealAngle. Cheap, and produces very smooth angle changing when in air vehicles.
================
*/
void idBot::Bot_ChangeAirVehicleViewAngles( const idAngles &idealAngle, bool fast, bool inCombat ) {

	int i;
	float move;
	float angMod = ( fast == true ) ? 1.0f / 25.0f : 1.0f / 16.0f;

	if ( inCombat ) {
		angMod = 1.0f / 2.0f;
	}

	for( i = 0; i < 3; i++ ) {
		move = idMath::AngleDelta( idealAngle[ i ], botViewAngles[ i ] );
		botViewAngles[ i ] += ( move * angMod );
	}
}

/*
================
idBot::Bot_GetProjectileAimPoint

Adjusts the targetOrg we're aiming at, so that we can hit our targets if its far away.
================
*/
bool idBot::Bot_GetProjectileAimPoint( const projectileTypes_t projectileType, idVec3& targetOrg, int ignoreEntNum ) {
	bool shotClear = true;
	int n = 0;
	int i;
	float x, y, a, b, c, d, sqrtd, p[2];
	float speed, gravity;
	float time, angle;
	float t;
	float x2[ 2 ], y2[ 2 ];
	float pitch, zVelocity, s, k;
	trace_t tr;
	idVec3 gunOrg, tempVec;
	idVec3 points[ 3 ];
	idVec3 dir;

//mal_FIXME: get the gravity/speed values from the weapon .def files!
	if ( projectileType == PROJECTILE_TANK_ROUND ) {
		gunOrg = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].proxyInfo.weaponOrigin;
		gravity = -800.0f;
		speed = 8003.0f;
//		gravity = -1500.0f;
//		speed = 10000.0f;
	} else if ( projectileType == PROJECTILE_ROCKET ) {
		gunOrg = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].viewOrigin;
		gravity = -120.0f;
		speed = 2001.0f;
	} else { //mal: PROJECTILE_GRENDADE
		gunOrg = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].viewOrigin;
		gravity = -550.0f;
		speed = 950.0f;
	}


	x = ( targetOrg.ToVec2() - gunOrg.ToVec2() ).LengthFast();
	y = targetOrg[2] - gunOrg[2];

	a = 4.0f * y * y + 4.0f * x * x;
	b = -4.0f * speed * speed - 4.0f * y * gravity;
	c = gravity * gravity;

	d = b * b - 4.0f * a * c;
	
	if ( d <= 0.0f ) {
		return false;
	}
	
	sqrtd = idMath::Sqrt( d );
	p[0] = ( - b + sqrtd ) / ( 2.0f * a );
	p[1] = ( - b - sqrtd ) / ( 2.0f * a );

	for ( i = 0; i < 2; i++ ) {
		if ( p[i] <= 0.0f ) {
			continue;
		}

		d = idMath::Sqrt( p[i] );
		x2[ n ] = d * x / speed;
		y2[ n ] = 0.5f * ( 2.0f * y * p[i] - gravity ) / ( speed * d );
		n++;
	}

	if ( n == 0 ) {
		return false;
	}

	if ( n == 2 ) {
		if ( idMath::Fabs( y2[ 1 ] ) < idMath::Fabs( y2[ 0 ] ) ) {
			idSwap( x2[ 0 ], x2[ 1 ] );
			idSwap( y2[ 0 ], y2[ 1 ] );
		}
	}

	tempVec = targetOrg - gunOrg;
	tempVec[ 2 ] = 0.0f;
	tempVec.NormalizeFast();

	tempVec *= x2[ 0 ];
	tempVec[ 2 ] = y2[ 0 ];

//mal: this below sets up a test to make sure the new targetOrg is actually reachable.
//mal: Isn't super accurate - just want to make sure we're not going to OBVIOUSLY shoot into the world.
	angle = atan2( y2[ 0 ], x2[ 0 ] );
	time = x / ( cos( angle ) * speed );
	angle = idMath::AngleNormalize180( RAD2DEG( angle ) );

	t = time * 0.5f;

	pitch = DEG2RAD( angle );
    idMath::SinCos( pitch, s, k );
	dir = targetOrg - gunOrg;

	if ( dir.IsZero() ) {
		return false;
	}

	dir.z = 0.0f;
	dir *= k * idMath::InvSqrt( dir.LengthSqr() );
	dir.z = s;

	zVelocity = speed * dir.z;

	points[ 0 ] = gunOrg;

	points[ 1 ].ToVec2() = gunOrg.ToVec2() + ( targetOrg.ToVec2() - gunOrg.ToVec2() ) * 0.5f;
	points[ 1 ].z = gunOrg.z + t * zVelocity + 0.5f * gravity * t * t;

	points[ 2 ] = targetOrg;

	for( i = 0; i < 2; i++ ) {
        gameLocal.clip.TracePointExt( CLIP_DEBUG_PARMS tr, points[ i ], points[ i + 1 ], MASK_SOLID | MASK_OPAQUE | MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID, this, GetProxyEntity() );

//		gameRenderWorld->DebugLine( colorWhite, points[i], points[i+1], 16 );

		if ( tr.fraction < 1.0f && ( ignoreEntNum != -1 && tr.c.entityNum != ignoreEntNum ) ) {
			shotClear = false;
			break;
		}
	}

	if ( shotClear ) {
        targetOrg = gunOrg + tempVec * x;
		return true;
	}

	return false;
}

/*
================
idBot::Bot_VehicleAimAtEnemy

Has the bot aim towards its target - could be a vehicle (with player inside) or a player on foot. Used for 
weapon aiming. Varies the aim based on the bots aim skill. Only called when bot is in certain vehicles.
================
*/
void idBot::Bot_VehicleAimAtEnemy( int clientNum ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		return;
	}

	bool isPlayer;
	proxyInfo_t enemyVehicleInfo, vehicleInfo;
	idVec3 velocity;
    idPlayer *player = gameLocal.GetClient( clientNum );

	if ( player == NULL ) {
		gameLocal.DWarning("Bot client %i is trying to attack client %i, who doesn't exist!\n", entityNumber, clientNum );
		return;
	}

	idVec3 vec = GetPlayerViewPosition( clientNum ) - firstPersonViewOrigin;
	float dist = vec.LengthSqr();
	float aim;
	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

	if ( transport == NULL ) {
		gameLocal.DWarning("Bot client %i thinks its in a vehicle, when its not!\n", entityNumber );
		return;
	}

	GetVehicleInfo( GetProxyEntity()->entityNumber, vehicleInfo );

	if ( bot_aimSkill.GetInteger() == 0 ) {
        aim = -0.05f;			
	} else if ( bot_aimSkill.GetInteger() == 1 ) {
	    aim = -0.15f;
	} else if ( bot_aimSkill.GetInteger() == 2 ) {
	    aim = -0.20f;
	} else { 
		aim = -0.30f;
	}
	
	if ( dist < Square( 1200.0f ) && bot_aimSkill.GetInteger() > 1 ) {
		aim = 0.15f;
	} //mal: if your REALLY close, they dont overshoot as much!
     
	isPlayer = ( player->InVehicle() ) ? false : true;
	velocity = ( isPlayer ) ? player->GetPhysics()->GetLinearVelocity() : player->proxyEntity->GetPhysics()->GetLinearVelocity();
	vec = ( isPlayer ) ? GetPlayerViewPosition( clientNum ) : player->proxyEntity->GetPhysics()->GetOrigin();

	if ( !isPlayer ) { //mal: for vehicles, the origin can be kinda low, so up it a bit.
		GetVehicleInfo( player->GetProxyEntity()->entityNumber, enemyVehicleInfo );
		float vehicleOffset = botThreadData.GetVehicleTargetOffset( enemyVehicleInfo.type );
		vec.z += ( enemyVehicleInfo.bbox[ 1 ][ 2 ] - enemyVehicleInfo.bbox[ 0 ][ 2 ] ) * vehicleOffset;
	}

	if ( vehicleInfo.type == TITAN && bot_aimSkill.GetInteger() > 0 ) {
        Bot_GetProjectileAimPoint( PROJECTILE_TANK_ROUND, vec, -1 );
		botViewAngles = transport->GetViewForPlayer( this ).GetRequiredViewAngles( vec ); //mal: turn as fast as vehicle view allows.
	} else {
		botViewAngles = transport->GetViewForPlayer( this ).GetRequiredViewAngles( vec ); //mal: turn as fast as vehicle view allows.
//        Bot_ChangeVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( vec ), true );
	}
}

/*
================
idBotAI::AirVehicleControls
================
*/
void idBot::AirVehicleControls( usercmd_t &ucmd ) {
	bool useBrakes = ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == AIR_BRAKE || botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == FULL_STOP ) ? true : false;
	bool aimFast = ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].turnType == AIM_TURN ) ? true : false;
	bool isBlocked = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].botCmds.isBlocked;
	bool isBraking = false;
	bool lookingAtClient = false;
	bool moveGoalInFront = false;
	bool takingOff = ( botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].proxyInfo.time + 10000 > gameLocal.time ) ? true : false;
	bool needsFastAntiRoll;
	bool isPitchSensitive;
	bool takesHardTurnsWell;
	bool useMaxBoostConstraints;
	bool alwaysRun;
	bool mustBrakeIfCantBoost;
	bool alwaysBoost;
	int dangerousDownPitch = 20;
	int vehicleTime = botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].proxyInfo.time;
	float moveRight;
	float lookAheadDist;
	float vehicleYaw;
	float vehiclePitch;
	float vehicleRoll;
	float height;
	proxyInfo_t vehicleInfo;
	const botAIOutput_t& botOutput = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];
	idVec3 vec;
	idVec3 altitude = InchesToMetres( GetPhysics()->GetOrigin() );
	idVec3 moveDir;
	idVec3 origin;
	idVec3 forward;
	idVec3 right;
	
	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

	height = GetPhysics()->GetOrigin().z;

	if ( transport == NULL ) { //mal: paranoid safety check
		gameLocal.Warning("Bot client #: %i thinks its on a vehicle, but its not!!!", entityNumber );
		return;
	}

	vehicleYaw = transport->GetRenderEntity()->axis.ToAngles().yaw;
	vehiclePitch = transport->GetRenderEntity()->axis.ToAngles().pitch;
	vehicleRoll = transport->GetRenderEntity()->axis.ToAngles().roll;

	GetVehicleInfo( transport->entityNumber, vehicleInfo );

	switch( vehicleInfo.type ) {
        case ANANSI:
			alwaysBoost = false;
			lookAheadDist = 1500.0f;
			alwaysRun = false;
			takesHardTurnsWell = true;
			isPitchSensitive = true;
			mustBrakeIfCantBoost = false;
			needsFastAntiRoll = false;
			useMaxBoostConstraints = false;
			break; 

		case HORNET:
			alwaysBoost = false;  
			lookAheadDist = 1500.0f;
			alwaysRun = false;
			takesHardTurnsWell = false;
			isPitchSensitive = true;
			mustBrakeIfCantBoost = true;
			needsFastAntiRoll = true;
			useMaxBoostConstraints = false;
			break;

		case BUFFALO:
			alwaysBoost = true;
			lookAheadDist = 1500.0f;
			alwaysRun = true;
//			takesHardTurnsWell = true;
			takesHardTurnsWell = false;
			isPitchSensitive = false;
			mustBrakeIfCantBoost = false;
			needsFastAntiRoll = false;
			break;

		default:
			alwaysBoost = false;
			lookAheadDist = 500.0f;
			alwaysRun = false;
			takesHardTurnsWell = false;
			isPitchSensitive = true;
			mustBrakeIfCantBoost = true;
			needsFastAntiRoll = false;
			useMaxBoostConstraints = false;
			break;
	}

	if ( alwaysRun ) {   
        ucmd.buttons.btn.run = true; 
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal != vec3_zero ) {
        
		if ( height < botThreadData.GetGameWorldState()->gameLocalInfo.maxVehicleHeight && botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType != IGNORE_ALTITUDE ) {
            ucmd.forwardmove = 127;
		}

		ucmd.buttons.btn.run = true;

        origin = transport->GetRenderEntity()->origin;
		forward = transport->GetRenderEntity()->axis[ 0 ];
		right = transport->GetRenderEntity()->axis[ 1 ] * -1;

		moveDir[0] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[0] - origin[0];
		moveDir[1] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[1] - origin[1];
		moveDir[2] = 0.0f;

		moveDir.Normalize(); 
	
		moveRight = ( right * moveDir ) * 128;  

		if ( InVehicleGunSights( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal, false ) ) {
			moveGoalInFront = true;
			ucmd.rightmove = FixUcmd( moveRight );
		} else {
			if ( moveRight > 0 ) {
				ucmd.rightmove = 127;
			} else if ( moveRight < 0 ) { 
				ucmd.rightmove = -127; 
			}
		} 

		if ( altitude.z < 60.0f || vehicleTime + 5000 > gameLocal.time ) { //mal: make sure we're at a safe altitude before we start moving...
			if ( vehicleTime + 15000 > gameLocal.time ) {   
				return;
			}
		}

		if ( !useBrakes ) {       
	        if ( !alwaysBoost ) { 
				vec = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal - origin;
				if ( moveGoalInFront ) {
					if ( !useMaxBoostConstraints ) {
                        ucmd.buttons.btn.sprint = true;
					} else {
						jumpTimer++;
						if ( vehicleInfo.forwardSpeed < 2500.0f ) {
							if ( jumpTimer > 60 ) {
								ucmd.buttons.btn.sprint = true;
							}
						} else {
							jumpTimer = 0;
						}
					}
				} else {
					if ( vehicleInfo.forwardSpeed > 500.0f && mustBrakeIfCantBoost ) {
                        ucmd.buttons.btn.run = false;
						ucmd.buttons.btn.sprint = false;
						isBraking = true;
					} else if ( vehicleInfo.forwardSpeed < -50 ) {
						ucmd.buttons.btn.sprint = true;
					}
				}
			} else {
				ucmd.buttons.btn.sprint = true;
			}
		}

		if ( isBlocked ) {
			ucmd.buttons.btn.sprint = false;

			if ( vehicleInfo.type != BUFFALO ) {
				ucmd.buttons.btn.run = false;
			}

			isBraking = true;
			ucmd.forwardmove = 127;
		}
	}

    if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == FULL_STOP ) {
		if ( vehicleInfo.forwardSpeed > 50.0f ) {
			ucmd.buttons.btn.run = false;
			isBraking = true;
		} else if ( vehicleInfo.forwardSpeed < -50.0f ) {
			ucmd.buttons.btn.sprint = true;
		}
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == AIR_BRAKE ) {
 		ucmd.buttons.btn.run = false;
		ucmd.buttons.btn.sprint = false;
		isBraking = true;
	}

#ifndef _XENON
	if ( botOutput.botCmds.topHat ) {
		ucmd.buttons.btn.tophat = true;
	}
#endif

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == AIR_COAST ) {
		ucmd.buttons.btn.sprint = false;
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveType == LAND ) { 
		if ( vehicleInfo.forwardSpeed > 50.0f ) {
			ucmd.buttons.btn.run = false;
			isBraking = true;
		} else if ( vehicleInfo.forwardSpeed < -50.0f ) {
			ucmd.buttons.btn.sprint = true;
		}

		ucmd.forwardmove = -127; 
	}

	if ( takingOff ) {
		return;
	}

	if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveViewOrigin != vec3_zero ) {
		idVec3 moveViewOrigin = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveViewOrigin;
		vec	= moveViewOrigin - firstPersonViewOrigin;
		if ( vec.z > -100.0f || vec.z < 100.0f ) {
			moveViewOrigin.z -= vec.z;
		}

		Bot_ChangeAirVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveViewOrigin ), takesHardTurnsWell, false ); 

		if ( moveGoalInFront && !isBraking  ) { //mal: goal has to be in front, and we can't be braking else we'll stall, then we'll nose up to gain altitude.  
			if ( altitude.z < 50.0f ) {
				if ( vehiclePitch > -30 ) { 
                    ucmd.angles[ PITCH ] -= 1500;
				}
			} else if ( takesHardTurnsWell && vehiclePitch < dangerousDownPitch && !isPitchSensitive ) {
				botViewAngles[ PITCH ] += 1.5; //mal: pitch the nose down just a bit, for extra speed.
			}
		}
	} else {
        if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum > -1 && botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum < MAX_CLIENTS ) {
        	idPlayer *player = gameLocal.GetClient( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum );
			idVec3 targetDelta;
			idVec3 enemyOrg;

			if ( vehicleInfo.type == BUFFALO ) {
				if ( player != NULL ) {
					if ( !player->InVehicle() ) {
						enemyOrg = player->GetPhysics()->GetOrigin();
					} else {
						enemyOrg = player->GetProxyEntity()->GetPhysics()->GetAbsBounds().GetCenter();
					}
					origin = transport->GetRenderEntity()->origin;
					right = transport->GetRenderEntity()->axis[ 1 ] * -1;
					moveDir[0] = enemyOrg[0] - origin[0];
					moveDir[1] = enemyOrg[1] - origin[1];
					moveDir[2] = 0.0f;
					moveDir.Normalize(); 
					moveRight = ( right * moveDir ) * 128; 

					bool inFront = InVehicleGunSights( enemyOrg, false );

					if ( inFront ) {
						ucmd.rightmove = FixUcmd( moveRight );
					} else {
						if ( moveRight > 0 ) {
							ucmd.rightmove = 127;
						} else if ( moveRight < 0 ) { 
							ucmd.rightmove = -127; 
						}
					}

					if ( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal == vec3_zero ) {
#ifndef _XENON
						if ( vehiclePitch < 0 ) {
							ucmd.buttons.btn.tophat = false;
						}
#endif

						if ( inFront ) {
							Bot_ChangeAirVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( enemyOrg ), false, false );
						}

						moveDir = enemyOrg - origin;
						float enemyHeight = moveDir.z;
	
						if ( enemyHeight < -1000.0f ) {
							ucmd.forwardmove = -127;
						} else {
							ucmd.forwardmove = 127;
						}
					}
				}
			} else if ( vehicleInfo.type == ANANSI ) {
				if ( player != NULL ) {
					if ( !player->InVehicle() ) {
						enemyOrg = player->GetPhysics()->GetOrigin();
					} else {
						enemyOrg = player->GetProxyEntity()->GetPhysics()->GetAbsBounds().GetCenter();
					}

					targetDelta = enemyOrg - firstPersonViewOrigin;
					targetDelta.NormalizeFast();
					idAngles estTargetingAngles = targetDelta.ToAngles();
					idAngles currentAngles = firstPersonViewAxis[ 0 ].ToAngles();
					idAngles diffAngles = estTargetingAngles - currentAngles;
					diffAngles.Normalize180();
					Bot_ChangeAirVehicleViewAngles( diffAngles, aimFast, true );
					lookingAtClient = true;
				}
			} else {
				if ( player != NULL ) {
					if ( player->InVehicle() ) {
						Bot_ChangeAirVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( player->GetProxyEntity()->GetPhysics()->GetOrigin() ), aimFast, true );
						lookingAtClient = true;
					} else {
						Bot_ChangeAirVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( player->GetPhysics()->GetOrigin() ), aimFast, true );
						lookingAtClient = true;
					}
				}
			}
		} else {
			if ( botOutput.viewEntityNum >= 0 && botOutput.viewEntityNum < MAX_GENTITIES ) {
				idEntity *entity = gameLocal.entities[ botThreadData.GetGameOutputState()->botOutput[ entityNumber ].viewEntityNum ];
				if ( entity != NULL ) {
					if ( vehicleInfo.type == ANANSI ) {
						idVec3 enemyOrg = entity->GetPhysics()->GetOrigin();
						idVec3 targetDelta;
						targetDelta = enemyOrg - firstPersonViewOrigin;
						targetDelta.NormalizeFast();
						idAngles estTargetingAngles = targetDelta.ToAngles();
						idAngles currentAngles = firstPersonViewAxis[ 0 ].ToAngles();
						idAngles diffAngles = estTargetingAngles - currentAngles;
						diffAngles.Normalize180();
						Bot_ChangeAirVehicleViewAngles( diffAngles, aimFast, true );
						lookingAtClient = true;
					} else {
						Bot_ChangeAirVehicleViewAngles( transport->GetViewForPlayer( this ).GetRequiredViewAngles( entity->GetPhysics()->GetOrigin() ), aimFast, true );
						lookingAtClient = true;
					}
				}
			}
		}
	}

	const idAngles &deltaViewAngles = GetDeltaViewAngles();

	ucmd.angles[0] = ANGLE2SHORT( botViewAngles[0] - deltaViewAngles[0] ); 
	ucmd.angles[1] = ANGLE2SHORT( botViewAngles[1] - deltaViewAngles[1] );
	ucmd.angles[2] = ANGLE2SHORT( botViewAngles[2] - deltaViewAngles[2] ); 

//mal: bit of anti roll..
	if ( needsFastAntiRoll ) { 
		if ( vehicleRoll < -10.0f ) {
			ucmd.angles[ YAW ] += -1000;
		} else if ( vehicleRoll > 10.0f ) {
			ucmd.angles[ YAW ] += 1000;
		}
	} else { //mal:  anti roll that gets weaker the closer the bot gets to normal.
		if ( vehicleRoll < -20.0f ) {
			ucmd.angles[ YAW ] += -500; //-1000;
		} else if ( vehicleRoll < -10.0f ) {  
			ucmd.angles[ YAW ] += -100; //-500;
		} else if ( vehicleRoll > 20.0f ) {
			ucmd.angles[ YAW ] += 500; //1000;
		} else if ( vehicleRoll > 10.0f ) {
			ucmd.angles[ YAW ] += 100; //500;
		}
	}

//mal: anti pitch!
	if ( vehiclePitch > ( ( lookingAtClient ) ? 90 : dangerousDownPitch ) ) { 
		ucmd.angles[ PITCH ] -= 1500;
	}

	if ( botThreadData.AllowDebugData() ) {
		if ( bot_debugAirVehicles.GetInteger() == entityNumber ) {
			common->Printf( "Brake = %i, Boost = %i, Speed = %.0f, Blocked = %i, Forward = %i\n", ( ucmd.buttons.btn.run == false ) ? 1 : 0, ucmd.buttons.btn.sprint, botThreadData.GetGameWorldState()->clientInfo[ entityNumber ].xySpeed, isBlocked, ucmd.forwardmove );
		}
	}

//mal: HACK! This needs to be used because of the way the AI air control system was orignally designed. There is no time to re-do it now.
	viewAngles = botViewAngles;
}

/*
============
idBot::GetPlayerViewPosition
============
*/
idVec3 idBot::GetPlayerViewPosition( int clientNum ) const {
	if ( !botThreadData.ClientIsValid( clientNum ) ) {
		gameLocal.DWarning("Invalid client passed to \"GetPlayerViewPosition\"!\n");
		return vec3_zero;
	}

	idVec3 vec;
	idPlayer *player = gameLocal.GetClient( clientNum );

	if ( player == NULL ) {
		gameLocal.DWarning("Invalid client passed to \"GetPlayerViewPosition\"!\n");
		return vec3_zero;
	}

	if ( botThreadData.GetGameWorldState()->clientInfo[ clientNum ].weapInfo.covertToolInfo.entNum == 0 && botThreadData.GetGameWorldState()->clientInfo[ clientNum ].weapInfo.covertToolInfo.clientIsUsing == false ) {
		return player->firstPersonViewOrigin;
	}
	
	vec = player->GetPhysics()->GetOrigin();
	vec.z += player->eyeOffset.z;
	return vec;
}

/*
==================
idBot::InVehicleGunSights
==================
*/
bool idBot::InVehicleGunSights( const idVec3 &org, bool precise ) {
	float dotProductCheck = ( precise ) ? 0.90f : 0.30f;
	idVec3 dir;

	if ( !InVehicle() ) {
		gameLocal.DWarning("Called InVehicleGunSights while not in a vehicle!");
		return false;
	}

	dir = org - GetProxyEntity()->GetPhysics()->GetOrigin();

	dir.NormalizeFast();

	if ( dir * GetProxyEntity()->GetRenderEntity()->axis[ 0 ] > dotProductCheck ) {
		return true;//mal: have someone in front of us..
	} 
	
	return false;
}

/*
==================
idBot::IcarusVehicleControls
==================
*/
void idBot::IcarusVehicleControls( usercmd_t &ucmd ) {
	sdTransport* transport = GetProxyEntity()->Cast< sdTransport >();

	if ( transport == NULL ) { //mal: paranoid safety check
		gameLocal.Warning("Bot client #: %i thinks its on a vehicle, but its not!!!", entityNumber );
		return;
	}

	idVec3 origin = transport->GetRenderEntity()->origin;
	idVec3 forward = transport->GetRenderEntity()->axis[ 0 ];
	idVec3 right = transport->GetRenderEntity()->axis[ 1 ] * -1;

	idVec3 moveDir;
	moveDir[0] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[0] - origin[0];
	moveDir[1] = botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal[1] - origin[1];
	moveDir[2] = 0.0f;

	moveDir.Normalize();

	float moveRight = ( right * moveDir ) * 128; 
	float moveForward = ( forward * moveDir ) * 128.0f;
	
	ucmd.forwardmove = 127; //FixUcmd( moveForward );

	if ( InVehicleGunSights( botThreadData.GetGameOutputState()->botOutput[ entityNumber ].moveGoal, false ) ) {
		ucmd.rightmove = FixUcmd( moveRight );
	} else {
		if ( moveRight > 0 ) {
			ucmd.rightmove = 127;
		} else if ( moveRight < 0 ) { 
			ucmd.rightmove = -127; 
		}
	}

	proxyInfo_t vehicleInfo;	
	GetVehicleInfo( GetProxyEntity()->entityNumber, vehicleInfo );

	AngleUcmds( ucmd );		
	ActionUcmds( ucmd );

	const botAIOutput_t &clientUcmd = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];

	if ( clientUcmd.specialMoveType == ICARUS_BOOST ) {
		ucmd.buttons.btn.sprint = true;
	}
}

/*
==================
idBot::CheckBotIngameMissionStatus
==================
*/
void idBot::CheckBotIngameMissionStatus() {
	const botAIOutput_t &botUcmd = botThreadData.GetGameOutputState()->botOutput[ entityNumber ];

	if ( botUcmd.debugInfo.botGoalType == oldBotGoalType ) { //mal: no need to update if it hasn't changed.
		return;
	}

	taskHandle_t newTask;

	if ( botUcmd.debugInfo.botGoalType != DO_OBJECTIVE ) { //mal: easy enough.
		playerTaskList_t botTasks = GetTaskList();

		for( int i = 0; i < botTasks.Num(); i++ ) {
			sdPlayerTask* botTask = botTasks[ i ];
			if ( botTask->GetEntity() != gameLocal.world ) {
				if ( botTask->GetEntity()->entityNumber != botUcmd.debugInfo.botGoalTypeTarget ) {
					continue;
				}

				newTask = botTask->GetHandle();
				oldBotGoalType = botUcmd.debugInfo.botGoalType;
				break;
			} else {
				if ( botTask->GetInfo()->GetBotTaskType() != botUcmd.debugInfo.botGoalType ) {
					continue;
				}

				newTask = botTask->GetHandle();
				oldBotGoalType = botUcmd.debugInfo.botGoalType;
				break;
			}
		}
	}

	sdTaskManager::GetInstance().SelectTask( this, newTask );
}



#include "../../idlib/precompiled.h"
#pragma hdrstop

#ifdef MOD_BOTS

#include "../Game_local.h"

/*
=====================
botMoveState::botMoveState
TinMan: An extended idMoveState to keep track of more complex move types
=====================
*/
botMoveState::botMoveState()
{
    moveType			= MOVETYPE_ANIM;
    moveCommand			= MOVE_NONE;
    moveStatus			= MOVE_STATUS_DONE;
    moveDest.Zero();
    moveDir.Set( 1.0f, 0.0f, 0.0f );
    goalEntity			= NULL;
    goalEntityOrigin.Zero();
    toAreaNum			= 0;
    startTime			= 0;
    duration			= 0;
    speed				= 0.0f;
    range				= 0.0f;
    wanderYaw			= 0;
    nextWanderTime		= 0;
    blockTime			= 0;
    obstacle			= NULL;
    lastMoveOrigin		= vec3_origin;
    lastMoveTime		= 0;
    anim				= 0;

    secondaryMovePosition = vec3_zero;
    pathType			= 0;
}

/*
=====================
botMoveState::Save
=====================
*/
void botMoveState::Save( idSaveGame *savefile ) const
{
    savefile->WriteInt( (int)moveType );
    savefile->WriteInt( (int)moveCommand );
    savefile->WriteInt( (int)moveStatus );
    savefile->WriteVec3( moveDest );
    savefile->WriteVec3( moveDir );
    goalEntity.Save( savefile );
    savefile->WriteVec3( goalEntityOrigin );
    savefile->WriteInt( toAreaNum );
    savefile->WriteInt( startTime );
    savefile->WriteInt( duration );
    savefile->WriteFloat( speed );
    savefile->WriteFloat( range );
    savefile->WriteFloat( wanderYaw );
    savefile->WriteInt( nextWanderTime );
    savefile->WriteInt( blockTime );
    obstacle.Save( savefile );
    savefile->WriteVec3( lastMoveOrigin );
    savefile->WriteInt( lastMoveTime );
    savefile->WriteInt( anim );

    savefile->WriteVec3( secondaryMovePosition );
    savefile->WriteInt( pathType );
}

/*
=====================
botMoveState::Restore
=====================
*/
void botMoveState::Restore( idRestoreGame *savefile )
{
    savefile->ReadInt( (int &)moveType );
    savefile->ReadInt( (int &)moveCommand );
    savefile->ReadInt( (int &)moveStatus );
    savefile->ReadVec3( moveDest );
    savefile->ReadVec3( moveDir );
    goalEntity.Restore( savefile );
    savefile->ReadVec3( goalEntityOrigin );
    savefile->ReadInt( toAreaNum );
    savefile->ReadInt( startTime );
    savefile->ReadInt( duration );
    savefile->ReadFloat( speed );
    savefile->ReadFloat( range );
    savefile->ReadFloat( wanderYaw );
    savefile->ReadInt( nextWanderTime );
    savefile->ReadInt( blockTime );
    obstacle.Restore( savefile );
    savefile->ReadVec3( lastMoveOrigin );
    savefile->ReadInt( lastMoveTime );
    savefile->ReadInt( anim );

    savefile->ReadVec3( secondaryMovePosition );
    savefile->ReadInt( pathType );
}

/*
============
botAASFindAttackPosition::botAASFindAttackPosition
TinMan: Tweaked from idAI specific, a lot simpler though.
============
*/
botAASFindAttackPosition::botAASFindAttackPosition( const idPlayer *self, const idMat3 &gravityAxis, idEntity *target, const idVec3 &targetPos, const idVec3 &eyeOffset )
{
    int	numPVSAreas;

    this->target		= target;
    this->targetPos		= targetPos;
    this->eyeOffset		= eyeOffset;
    this->self			= self;
    this->gravityAxis	= gravityAxis;

    excludeBounds		= idBounds( idVec3( -64.0, -64.0f, -8.0f ), idVec3( 64.0, 64.0f, 64.0f ) );
    excludeBounds.TranslateSelf( self->GetPhysics()->GetOrigin() );

    // setup PVS
    idBounds bounds( targetPos - idVec3( 16, 16, 0 ), targetPos + idVec3( 16, 16, 64 ) );
    numPVSAreas = gameLocal.pvs.GetPVSAreas( bounds, PVSAreas, idEntity::MAX_PVS_AREAS );
    targetPVS	= gameLocal.pvs.SetupCurrentPVS( PVSAreas, numPVSAreas );
}

/*
============
botAASFindAttackPosition::~botAASFindAttackPosition
============
*/
botAASFindAttackPosition::~botAASFindAttackPosition()
{
    gameLocal.pvs.FreeCurrentPVS( targetPVS );
}

/*
============
botAASFindAttackPosition::TestArea
============
*/
bool botAASFindAttackPosition::TestArea( const idAAS *aas, int areaNum )
{
    idVec3	dir;
    idVec3	local_dir;
    idVec3	fromPos;
    idMat3	axis;
    idVec3	areaCenter;
    int		numPVSAreas;
    int		PVSAreas[ idEntity::MAX_PVS_AREAS ];

    idVec3	targetPos1;
    idVec3	targetPos2;
    trace_t		tr;
    idVec3 toPos;

    areaCenter = aas->AreaCenter( areaNum );
    areaCenter[ 2 ] += 1.0f;

    if ( excludeBounds.ContainsPoint( areaCenter ) )
    {
        // too close to where we already are
        return false;
    }

    numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( areaCenter ).Expand( 16.0f ), PVSAreas, idEntity::MAX_PVS_AREAS );
    if ( !gameLocal.pvs.InCurrentPVS( targetPVS, PVSAreas, numPVSAreas ) )
    {
        return false;
    }


    // calculate the world transform of the launch position
    dir = targetPos - areaCenter;
    gravityAxis.ProjectVector( dir, local_dir );
    local_dir.z = 0.0f;
    local_dir.ToVec2().Normalize();
    axis = local_dir.ToMat3();
    fromPos = areaCenter + eyeOffset * axis;

    if ( target->IsType( idActor::Type ) )
    {
        static_cast<idActor *>( target )->GetAIAimTargets( target->GetPhysics()->GetOrigin(), targetPos1, targetPos2 );
    }
    else
    {
        targetPos1 = target->GetPhysics()->GetAbsBounds().GetCenter();
        targetPos2 = targetPos1;
    }

    toPos = targetPos1;

    gameLocal.clip.TracePoint( tr, fromPos, toPos, MASK_SOLID, self );
    if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == target ) )
    {
        return true;
    }

    return false;
}

/*
===============================================================================

	botAi
	Base class to build bot on.

	TinMan: I was going to say something about this, but I've forgotten

===============================================================================
*/

const int botAi::BOT_MAX_BOTS		= 16;
const int botAi::BOT_START_INDEX	= 16;

botInfo_t botAi::bots[BOT_MAX_BOTS]; // TinMan: init bots array, must keep an eye on the blighters

const idEventDef BOT_SetNextState( "setNextState", "s" );
const idEventDef BOT_SetState( "setState", "s" );
const idEventDef BOT_GetState( "getState", NULL, 's' );
const idEventDef BOT_GetBody( "getBody", NULL, 'e' );

const idEventDef BOT_GetGameType( "getGameType", NULL, 'f' );

const idEventDef BOT_HasWeapon( "hasWeapon", "s", 'f' );
const idEventDef BOT_HasAmmoForWeapon( "hasAmmoForWeapon", "s", 'f' );
const idEventDef BOT_HasAmmo( "hasAmmo", "s", 'f' );

const idEventDef BOT_NextBestWeapon( "nextBestWeapon", NULL );

const idEventDef BOT_SetAimPosition( "setAimPosition", "v" );
const idEventDef BOT_GetAimPosition( "getAimPosition", NULL, 'v' );
const idEventDef BOT_SetAimDirection( "setAimDirection", "v" );
const idEventDef BOT_GetMovePosition( "getMovePosition", NULL, 'v' );
const idEventDef BOT_GetSecondaryMovePosition( "getSecondaryMovePosition", NULL, 'v' );
const idEventDef BOT_GetPathType( "getPathType", NULL, 'd' );
const idEventDef BOT_SetMoveDirection( "setMoveDirection", "vf" );

const idEventDef BOT_CanSeeEntity( "canSeeEntity", "Ed", 'd' );
const idEventDef BOT_CanSeePosition( "canSeePosition", "vd", 'd' );

const idEventDef BOT_GetEyePosition( "getEyePosition", NULL, 'v' );
const idEventDef BOT_GetViewPosition( "getViewPosition", NULL, 'v' );
const idEventDef BOT_GetAIAimTargets( "getAIAimTargets", "ef", 'v' );

const idEventDef BOT_FindEnemies( "findEnemies", "d", 'f' );
const idEventDef BOT_FindInRadius( "findInRadius", "vfs", 'f' );
const idEventDef BOT_FindItems( "findItems", NULL, 'f' );
const idEventDef BOT_GetEntityList( "getEntityList", "f", 'e' );
const idEventDef BOT_HeardSound( "heardSound", "d", 'e' );
const idEventDef BOT_SetEnemy( "setEnemy", "E" );
const idEventDef BOT_ClearEnemy( "clearEnemy" );
const idEventDef BOT_GetEnemy( "getEnemy", NULL, 'e' );
const idEventDef BOT_LocateEnemy( "locateEnemy" );

const idEventDef BOT_EnemyRange( "enemyRange", NULL, 'f' );
const idEventDef BOT_EnemyRange2D( "enemyRange2D", NULL, 'f' );
const idEventDef BOT_GetEnemyPos( "getEnemyPos", NULL, 'v' );
const idEventDef BOT_GetEnemyEyePos( "getEnemyEyePos", NULL, 'v' );
const idEventDef BOT_PredictEnemyPos( "predictEnemyPos", "f", 'v' );
const idEventDef BOT_CanHitEnemy( "canHitEnemy", NULL, 'd' );
const idEventDef BOT_EnemyPositionValid( "enemyPositionValid", NULL, 'd' );

const idEventDef BOT_MoveStatus( "moveStatus", NULL, 'd' );
const idEventDef BOT_StopMove( "stopMove" );
const idEventDef BOT_SaveMove( "saveMove" );
const idEventDef BOT_RestoreMove( "restoreMove" );

const idEventDef BOT_SetMoveToCover( "setMoveToCover" );
const idEventDef BOT_SetMoveToEnemy( "setMoveToEnemy" );
const idEventDef BOT_SetMoveOutOfRange( "setMoveOutOfRange", "ef" );
const idEventDef BOT_SetMoveToAttackPosition( "setMoveToAttackPosition", "e" );
const idEventDef BOT_SetMoveWander( "setMoveWander" );
const idEventDef BOT_SetMoveToEntity( "setMoveToEntity", "e" );
const idEventDef BOT_SetMoveToPosition( "setMoveToPosition", "v" );

const idEventDef BOT_CanReachPosition( "canReachPosition", "v", 'd' );
const idEventDef BOT_CanReachEntity( "canReachEntity", "E", 'd' );
const idEventDef BOT_CanReachEnemy( "canReachEnemy", NULL, 'd' );
const idEventDef BOT_GetReachableEntityPosition( "getReachableEntityPosition", "e", 'v' );

const idEventDef BOT_TravelDistanceToPoint( "travelDistanceToPoint", "v", 'f' );
const idEventDef BOT_TravelDistanceToEntity( "travelDistanceToEntity", "e", 'f' );
const idEventDef BOT_TravelDistanceBetweenPoints( "travelDistanceBetweenPoints", "vv", 'f' );
const idEventDef BOT_TravelDistanceBetweenEntities( "travelDistanceBetweenEntities", "ee", 'f' );

const idEventDef BOT_GetObstacle( "getObstacle", NULL, 'e' );
const idEventDef BOT_PushPointIntoAAS( "pushPointIntoAAS", "v", 'v' );

// TinMan: The old integration vs independant argument, the proper way would be to integrate the following script events into
const idEventDef BOT_GetHealth( "getActorHealth", "e", 'f' ); // TinMan: could go in actor
const idEventDef BOT_GetArmor( "getArmor", "e", 'f' ); // TinMan: could go in player

const idEventDef BOT_GetTeam( "getTeam", "e", 'f' ); // TinMan: could go in actor

const idEventDef BOT_GetEntityByNum( "getEntityByNum", "f", 'e' );
const idEventDef BOT_GetNumEntities( "getNumEntities", NULL, 'd' );

// TinMan: Could be in entity
const idEventDef BOT_GetClassName( "getClassName", "e", 's' );
const idEventDef BOT_GetClassType( "getClassType", "e", 's' );

const idEventDef BOT_Acos( "acos", "f", 'f' ); // TinMan: could go in script sys

const idEventDef BOT_GetFlag( "getFlag", "f", 'e' );
const idEventDef BOT_GetFlagStatus( "getFlagStatus", "f", 'f' );
const idEventDef BOT_GetFlagCarrier( "getFlagCarrier", "f", 'e' );
const idEventDef BOT_GetCapturePoint( "getCapturePoint", "f", 'e' );

const idEventDef BOT_IsUnderPlat( "isUnderPlat", "e", 'd' );
const idEventDef BOT_GetWaitPosition( "getWaitPosition", "e", 'v' );

const idEventDef BOT_GetCommandType( "getCommandType", NULL, 'd' );
const idEventDef BOT_GetCommandEntity( "getCommandEntity", NULL, 'E' );
const idEventDef BOT_GetCommandPosition( "getCommandPosition", NULL, 'v' );
const idEventDef BOT_ClearCommand( "clearCommand", NULL, NULL );

CLASS_DECLARATION( idEntity, botAi )
EVENT( BOT_SetNextState,					botAi::Event_SetNextState )
EVENT( BOT_SetState,						botAi::Event_SetState )
EVENT( BOT_GetState,						botAi::Event_GetState )
EVENT( BOT_GetBody,							botAi::Event_GetBody )

EVENT( BOT_GetGameType,						botAi::Event_GetGameType )

EVENT( BOT_GetHealth,						botAi::Event_GetHealth )
EVENT( BOT_GetArmor,						botAi::Event_GetArmor )

EVENT( BOT_GetTeam,							botAi::Event_GetTeam )

EVENT( BOT_HasWeapon,						botAi::Event_HasWeapon )
EVENT( BOT_HasAmmoForWeapon,				botAi::Event_HasAmmoForWeapon )
EVENT( BOT_HasAmmo,							botAi::Event_HasAmmo )

EVENT( BOT_NextBestWeapon,					botAi::Event_NextBestWeapon )

EVENT( BOT_SetAimPosition,					botAi::Event_SetAimPosition )
EVENT( BOT_GetAimPosition,					botAi::Event_GetAimPosition )
EVENT( BOT_SetAimDirection,					botAi::Event_SetAimDirection )
EVENT( BOT_GetMovePosition,					botAi::Event_GetMovePosition )
EVENT( BOT_GetSecondaryMovePosition,		botAi::Event_GetSecondaryMovePosition )
EVENT( BOT_GetPathType,						botAi::Event_GetPathType )
EVENT( BOT_SetMoveDirection,				botAi::Event_SetMoveDirection )

EVENT( BOT_CanSeeEntity,					botAi::Event_CanSeeEntity )
EVENT( BOT_CanSeePosition,					botAi::Event_CanSeePosition )

EVENT( BOT_GetEyePosition,					botAi::Event_GetEyePosition )
EVENT( BOT_GetViewPosition,					botAi::Event_GetViewPosition )
EVENT( BOT_GetAIAimTargets,					botAi::Event_GetAIAimTargets )

EVENT( BOT_FindEnemies,						botAi::Event_FindEnemies )
EVENT( BOT_FindInRadius,					botAi::Event_FindInRadius )
EVENT( BOT_FindItems,						botAi::Event_FindItems )
EVENT( BOT_GetEntityList,					botAi::Event_GetEntityList )
EVENT( BOT_HeardSound,						botAi::Event_HeardSound )
EVENT( BOT_SetEnemy,						botAi::Event_SetEnemy )
EVENT( BOT_ClearEnemy,						botAi::Event_ClearEnemy )
EVENT( BOT_GetEnemy,						botAi::Event_GetEnemy )
EVENT( BOT_GetEnemy,						botAi::Event_GetEnemy )
EVENT( BOT_LocateEnemy,						botAi::Event_LocateEnemy )

EVENT( BOT_EnemyRange,						botAi::Event_EnemyRange )
EVENT( BOT_EnemyRange2D,					botAi::Event_EnemyRange2D )
EVENT( BOT_GetEnemyPos,						botAi::Event_GetEnemyPos )
EVENT( BOT_GetEnemyEyePos,					botAi::Event_GetEnemyEyePos )
EVENT( BOT_PredictEnemyPos,					botAi::Event_PredictEnemyPos )
EVENT( BOT_CanHitEnemy,						botAi::Event_CanHitEnemy )
EVENT( BOT_EnemyPositionValid,				botAi::Event_EnemyPositionValid )

EVENT( BOT_MoveStatus,						botAi::Event_MoveStatus )
EVENT( BOT_StopMove,						botAi::Event_StopMove )
EVENT( BOT_SaveMove,						botAi::Event_SaveMove )
EVENT( BOT_RestoreMove,						botAi::Event_RestoreMove )

EVENT( BOT_SetMoveToCover,					botAi::Event_SetMoveToCover )
EVENT( BOT_SetMoveToEnemy,					botAi::Event_SetMoveToEnemy )
EVENT( BOT_SetMoveOutOfRange,				botAi::Event_SetMoveOutOfRange )
EVENT( BOT_SetMoveToAttackPosition,			botAi::Event_SetMoveToAttackPosition )
EVENT( BOT_SetMoveWander,					botAi::Event_SetMoveWander )
EVENT( BOT_SetMoveToEntity,					botAi::Event_SetMoveToEntity )
EVENT( BOT_SetMoveToPosition,				botAi::Event_SetMoveToPosition )

EVENT( BOT_CanReachPosition,				botAi::Event_CanReachPosition )
EVENT( BOT_CanReachEntity,					botAi::Event_CanReachEntity )
EVENT( BOT_CanReachEnemy,					botAi::Event_CanReachEnemy )
EVENT( BOT_GetReachableEntityPosition,		botAi::Event_GetReachableEntityPosition )

EVENT( BOT_TravelDistanceToPoint,			botAi::Event_TravelDistanceToPoint )
EVENT( BOT_TravelDistanceToEntity,			botAi::Event_TravelDistanceToEntity )
EVENT( BOT_TravelDistanceBetweenPoints,		botAi::Event_TravelDistanceBetweenPoints )
EVENT( BOT_TravelDistanceBetweenEntities,	botAi::Event_TravelDistanceBetweenEntities )

EVENT( BOT_GetObstacle,						botAi::Event_GetObstacle )
EVENT( BOT_PushPointIntoAAS,				botAi::Event_PushPointIntoAAS )

EVENT( BOT_GetEntityByNum,					botAi::Event_GetEntityByNum )
EVENT( BOT_GetNumEntities,					botAi::Event_GetNumEntities )

EVENT( BOT_GetClassName,					botAi::Event_GetClassName )
EVENT( BOT_GetClassType,					botAi::Event_GetClassType )

EVENT( BOT_Acos,							botAi::Event_Acos )

EVENT( BOT_GetFlag,							botAi::Event_GetFlag )
EVENT( BOT_GetFlagStatus,					botAi::Event_GetFlagStatus )
EVENT( BOT_GetFlagCarrier,					botAi::Event_GetFlagCarrier )
EVENT( BOT_GetCapturePoint,					botAi::Event_GetCapturePoint )

EVENT( BOT_IsUnderPlat,						botAi::Event_IsUnderPlat )
EVENT( BOT_GetWaitPosition,					botAi::Event_GetWaitPosition )
EVENT( BOT_GetCommandType,					botAi::Event_GetCommandType )
EVENT( BOT_GetCommandEntity,				botAi::Event_GetCommandEntity )
EVENT( BOT_GetCommandPosition,				botAi::Event_GetCommandPosition )
EVENT( BOT_ClearCommand,					botAi::Event_ClearCommand )
END_CLASS

/*
=====================
botAi::botAi
=====================
*/
botAi::botAi()
{
    fl.networkSync		= false; // TinMan: server side only
    //fl.networkSync		= true;

    botID				= 0;
    clientID			= 0;
    playerEnt			= NULL;
    physicsObject		= NULL;
    inventory			= NULL;

    viewDir.Zero();
    viewAngles			= ang_zero;
    moveDir.Zero();
    moveSpeed			= 0.0f;

    fovDot				= 0.0f;

    scriptThread		= NULL;		// id: initialized by ConstructScriptObject, which is called by idEntity::Spawn

    aas					= NULL;
    move.moveType		= MOVETYPE_ANIM;

    travelFlags			= TFL_WALK|TFL_AIR;
    // TFL_WALK|TFL_CROUCH|TFL_WALKOFFLEDGE|TFL_BARRIERJUMP|TFL_JUMP|TFL_LADDER|TFL_SWIM|TFL_WATERJUMP|TFL_TELEPORT|TFL_ELEVATOR|TFL_FLY|TFL_SPECIAL|TFL_WATER|TFL_AIR

    kickForce			= 2048.0f;
    ignore_obstacles	= false;
    blockedRadius		= 0.0f;
    blockedMoveTime		= 750;
}

/*
=====================
botAi::~botAi
=====================
*/
botAi::~botAi()
{
    ShutdownThreads();
}

/*
=====================
botAi::WriteUserCmdsToSnapshot
TinMan: A brian c and steven b production. You need to transmit usrcmds manually since the bot isn't a propper client so the engine doesn't handle it.
=====================
*/
void botAi::WriteUserCmdsToSnapshot( idBitMsg &msg )
{
    int i;
    int numBots;

    numBots = 0;
    // TODO: this loops through all the clients, really only need to loop starting at BOT_START_INDEX
    for ( i = 0; i < gameLocal.numClients; i++ )
    {
        if ( bots[i].inUse )
        {
            numBots++;
        }
    }
    // send the number of bots over the wire
    msg.WriteBits( numBots, 5 );

    for ( i = 0; i < gameLocal.numClients; i++ )
    {
        // write the bot number over the wire
        if ( bots[i].inUse )
        {
            // cusTom3 - the index in the usercmds array is i + BOT_START_INDEX
            msg.WriteBits( i + BOT_START_INDEX, 5 );
            usercmd_t &cmd = gameLocal.usercmds[i + BOT_START_INDEX];
            msg.WriteByte( cmd.buttons );
            msg.WriteShort( cmd.mx );
            msg.WriteShort( cmd.my );
            msg.WriteChar( cmd.forwardmove );
            msg.WriteChar( cmd.rightmove );
            msg.WriteChar( cmd.upmove );
            msg.WriteShort( cmd.angles[0] );
            msg.WriteShort( cmd.angles[1] );
            msg.WriteShort( cmd.angles[2] );
        }
    }
}

/*
=====================
botAi::ReadUserCmdsFromSnapshot
TinMan: A brian c and steven b production. The compatriot of above.
=====================
*/
void botAi::ReadUserCmdsFromSnapshot( const idBitMsg &msg )
{
    int i;
    int numBots;

    numBots = msg.ReadBits( 5 );
    for ( i = 0; i < numBots; i++ )
    {
        int clientNum = msg.ReadBits( 5 );

        usercmd_t &cmd = gameLocal.usercmds[clientNum]; // TinMan: Overwrite the usrcmds, as bot isn't propper client this will be null anyway.
        cmd.buttons     = msg.ReadByte();
        cmd.mx          = msg.ReadShort();
        cmd.my          = msg.ReadShort();
        cmd.forwardmove = msg.ReadChar();
        cmd.rightmove   = msg.ReadChar();
        cmd.upmove      = msg.ReadChar();
        cmd.angles[0]   = msg.ReadShort();
        cmd.angles[1]   = msg.ReadShort();
        cmd.angles[2]   = msg.ReadShort();
    }
}

/*
===================
botAi::Addbot_f
TinMan: Console command. Bring in teh b0tz!
*todo* set default def to something sensible
*todo* random name if default bot
*todo* get passed in args working for no added classname
===================
*/
void botAi::Addbot_f( const idCmdArgs &args )
{
    const char *key, *value;
    int			i;
    idVec3		org;
    idDict		dict;
    const char* name;
    idDict		userInfo;
    idEntity *	ent;

    int newBotID = 0;
    int newClientID = 0;

    if ( !gameLocal.isMultiplayer )
    {
        gameLocal.Printf( "You may only add a bot to a multiplayer game\n" );
        return;
    }

    if ( !gameLocal.isServer )
    {
        gameLocal.Printf( "Bots may only be added on server, only it has the powah!\n" );
        return;
    }

    // Try to find an ID in the bots list
    for ( i = 0; i < BOT_MAX_BOTS; i++ )
    {
        if ( bots[i].inUse )
        {
            // TinMan: make sure it isn't an orphaned spot
            if ( gameLocal.entities[ bots[i].clientID ] && gameLocal.entities[ bots[i].entityNum ]  )
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if ( i >= BOT_MAX_BOTS )
    {
        gameLocal.Printf("The maximum number of bots are already in the game.\n");
        return;
    }
    else
    {
        newBotID = i;
    }

    value = args.Argv( 1 );

    // TinMan: Check to see if valid def
    const idDeclEntityDef *botDef = gameLocal.FindEntityDef( value );
    const char *spawnclass = botDef->dict.GetString( "spawnclass" );
    idTypeInfo *cls = idClass::GetClass( spawnclass );
    if ( !cls || !cls->IsType( botAi::Type ) )
    {
        gameLocal.Printf( "def not of type botAi or no def name given\n" );
        return;
    }

    dict.Set( "classname", value );

    // TinMan: Add rest of key/values passed in
    for( i = 2; i < args.Argc() - 1; i += 2 )
    {
        key = args.Argv( i );
        value = args.Argv( i + 1 );

        dict.Set( key, value );
    }

    dict.Set( "name", va( "bot_%d", newBotID ) ); // TinMan: Set entity name for easier debugging
    dict.SetInt( "botID", newBotID ); // TinMan: Bot needs to know this before it's spawned so it can sync up with the client
    newClientID = BOT_START_INDEX + newBotID; // TinMan: client id, bots use >16

    //gameLocal.Printf("Spawning bot as client %d\n", newClientID);

    // Start up client
    gameLocal.ServerClientConnect(newClientID, NULL);

    gameLocal.ServerClientBegin(newClientID); // TinMan: spawn the fakeclient (and send message to do likewise on other machines)

    idPlayer * botClient = static_cast< idPlayer * >( gameLocal.entities[ newClientID ] ); // TinMan: Make a nice and pretty pointer to fakeclient
    // TinMan: Put our grubby fingerprints on fakeclient. *todo* may want to make these entity flags and make sure they go across to clients so we can rely on using them for clientside code
    botClient->spawnArgs.SetBool( "isBot", true );
    botClient->spawnArgs.SetInt( "botID", newBotID );

    // TinMan: Add client to bot list
    bots[newBotID].inUse	= true;
    bots[newBotID].clientID = newClientID;

    // TinMan: Spawn bot with our dict
    gameLocal.SpawnEntityDef( dict, &ent, false );
    botAi * newBot = static_cast< botAi * >( ent ); // TinMan: Make a nice and pretty pointer to bot

    // TinMan: Add bot to bot list
    bots[newBotID].entityNum = newBot->entityNumber;

    // TinMan: Give me your name, licence and occupation.
    name = newBot->spawnArgs.GetString( "npc_name" );
    userInfo.Set( "ui_name", va( "[BOT%d] %s", newBotID, name) ); // TinMan: *debug* Prefix [BOTn]

    // TinMan: I love the skin you're in.
    int skinNum = newBot->spawnArgs.GetInt( "mp_skin" );
    userInfo.Set( "ui_skin", ui_skinArgs[ skinNum ] );

    // TinMan: Set team
    if ( gameLocal.mpGame.IsGametypeTeamBased() )
    {
        userInfo.Set( "ui_team", newBot->spawnArgs.GetInt( "team" ) ? "Blue" : "Red" );
    }
    else if ( gameLocal.gameType == GAME_SP )
    {
        botClient->team = newBot->spawnArgs.GetInt( "team" );
    }

    // TinMan: Finish up connecting - Called in SetUserInfo
    //gameLocal.mpGame.EnterGame(newClientID);
    //gameLocal.Printf("Bot has been connected, and client has begun.\n");

    userInfo.Set( "ui_ready", "Ready" );

    gameLocal.SetUserInfo( newClientID, userInfo, false, true ); // TinMan: apply the userinfo *note* func was changed slightly in 1.3

    botClient->Spectate( false ); // TinMan: Finally done, get outa spectate

    cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", newClientID ) );
}

/*
===================
botAi::Removebot_f
TinMan: Console command. Bye bye botty.
===================
*/
void botAi::Removebot_f( const idCmdArgs &args )
{
    const char *value;
    int killBotID;

    if ( !gameLocal.isMultiplayer )
    {
        gameLocal.Printf( "This isn't multiplayer, so there no bots to remove, so yeah, you're mental.\n" );
        return;
    }

    if ( !gameLocal.isServer )
    {
        gameLocal.Printf( "Bots may only be removed on server, only it has the powah!\n" );
        return;
    }

    value = args.Argv( 1 );

    killBotID = atoi( value );

    //gameLocal.Printf( "[botAi::Removebot][Bot #%d]\n", killBotID ); // TinMan: *debug*

    if ( killBotID < BOT_MAX_BOTS && bots[killBotID].inUse && gameLocal.entities[ bots[killBotID].clientID ] && gameLocal.entities[ bots[killBotID].entityNum ] )
    {
        gameLocal.ServerClientDisconnect( bots[killBotID].clientID ); // TinMan: KaWhoosh! Youuure outa there!
    }
    else
    {
        gameLocal.Printf( "There is no spoon, I mean, bot #%d\n", killBotID );
        return;
    }
}

/*
================
botAi::Init
================
*/
void botAi::Init( void )
{
    state				= NULL;
    idealState			= NULL;

    enemy				= NULL;
    lastVisibleEnemyPos.Zero();
    lastVisibleEnemyEyeOffset.Zero();
    lastVisibleReachableEnemyPos.Zero();
    lastReachableEnemyPos.Zero();

    lastHitCheckResult	= false;
    lastHitCheckTime	= 0;

    AI_FORWARD			= false;
    AI_BACKWARD			= false;
    AI_STRAFE_LEFT		= false;
    AI_STRAFE_RIGHT		= false;
    AI_ATTACK_HELD		= false;
    AI_WEAPON_FIRED		= false;
    AI_JUMP				= false;
    AI_DEAD				= false;
    AI_CROUCH			= false;
    AI_ONGROUND			= true;
    AI_ONLADDER			= false;
    AI_RUN				= false;
    AI_HARDLANDING		= false;
    AI_SOFTLANDING		= false;
    AI_RELOAD			= false;
    AI_PAIN				= false;
    AI_TELEPORT			= false;
    AI_TURN_LEFT		= false;
    AI_TURN_RIGHT		= false;

    AI_ENEMY_VISIBLE	= false;
    AI_ENEMY_IN_FOV		= false;
    AI_ENEMY_DEAD		= false;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    AI_BLOCKED			= false;
    AI_OBSTACLE_IN_PATH = false;
    AI_PUSHED			= false;

    AI_WEAPON_FIRE		= false;

    BOT_COMMAND			= false;
    commandType			= NULL;
    commandEntity		= NULL;
    commandPosition.Zero();

    // id: init the move variables
    StopMove( MOVE_STATUS_DONE );

    numSearchListEntities = 0;
    memset( entitySearchList, 0, sizeof( entitySearchList ) );

    // id: reset the script object
    ConstructScriptObject();

    // id: execute the script so the script object's constructor takes effect immediately
    scriptThread->Execute();

    viewAngles		= playerEnt->viewAngles;
    aimPosition		= physicsObject->GetOrigin();
    aimRate			= spawnArgs.GetFloat( "aim_rate", "0.1" );
    if ( aimRate > 1.0f )
    {
        aimRate = 1.0f;
    }
    else if ( aimRate < 0.1f )
    {
        aimRate = 0.1f;
    }

}

/*
================
botAi::Spawn
================
*/
void botAi::Spawn( void )
{
    float			fovDegrees;

    // TinMan: Sync up with our client
    botID = spawnArgs.GetInt( "botID" );
    clientID = bots[botID].clientID;
    assert( clientID );

    playerEnt = static_cast< idPlayer * >( gameLocal.entities[ clientID ] ); // TinMan: Let the brain know what it's body is
    physicsObject = static_cast< idPhysics_Player * >( playerEnt->GetPhysics() );
    inventory = &playerEnt->inventory;

    /*
    // TinMan: *debug*
    if ( playerEnt->spawnArgs.GetBool( "isBot" ) ) {
    	gameLocal.Printf( "[botAi][client tagged as bot]\n" );
    }
    */

    spawnArgs.GetFloat( "fov", "90", fovDegrees );
    SetFOV( fovDegrees );

    state		= NULL;
    idealState	= NULL;

    move.blockTime = 0;

    SetAAS();

    LinkScriptVariables();

    Init();

    BecomeActive( TH_THINK );
    playerEnt->BecomeActive( TH_THINK ); // TinMan: *test* give your body a prod for good measure
}

/*
================
botAi::Save
TinMan: *todo* !!out of date!! look in class contructor for stuff to save
================
*/
void botAi::Save( idSaveGame *savefile ) const
{
    int i;

    savefile->WriteInt( botID );
    savefile->WriteInt( clientID );
    savefile->WriteObject( playerEnt );

    savefile->WriteAngles( viewAngles );

    savefile->WriteObject( scriptThread );

    idToken token;
    if ( idealState )
    {
        idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "botAi::Save" );

        src.ReadTokenOnLine( &token );
        src.ExpectTokenString( "::" );
        src.ReadTokenOnLine( &token );

        savefile->WriteString( token );
    }
    else
    {
        savefile->WriteString( "" );
    }

    savefile->WriteVec3( aimPosition );
    savefile->WriteFloat( aimRate );

    savefile->WriteInt( numSearchListEntities );
    for ( i = 0; i == numSearchListEntities; i++ )
    {
        savefile->WriteObject( entitySearchList[ i ] );
    }

    savefile->WriteInt( travelFlags );
    move.Save( savefile );
    savedMove.Save( savefile );
    savefile->WriteFloat( kickForce );
    savefile->WriteBool( ignore_obstacles );
    savefile->WriteFloat( blockedRadius );
    savefile->WriteInt( blockedMoveTime );

    enemy.Save( savefile );
    savefile->WriteVec3( lastVisibleEnemyPos );
    savefile->WriteVec3( lastVisibleEnemyEyeOffset );
    savefile->WriteVec3( lastVisibleReachableEnemyPos );
    savefile->WriteVec3( lastReachableEnemyPos );

    savefile->WriteBool( lastHitCheckResult );
    savefile->WriteInt( lastHitCheckTime );
}

/*
================
botAi::Restore
TinMan: *todo* !!out of date!!
================
*/
void botAi::Restore( idRestoreGame *savefile )
{
    int i;

    savefile->ReadInt( botID );
    savefile->ReadInt( clientID );
    savefile->ReadObject( reinterpret_cast<idClass *&>( playerEnt ) );

    physicsObject = static_cast< idPhysics_Player * >( playerEnt->GetPhysics() );
    inventory = &playerEnt->inventory; // TinMan: *todo* this may not be nice

    savefile->ReadAngles( viewAngles );
    //playerEnt->SetViewAngles( viewAngles );

    savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );

    idStr statename;
    savefile->ReadString( statename );
    if ( statename.Length() > 0 )
    {
        state = GetScriptFunction( statename );
    }

    savefile->ReadVec3( aimPosition );
    savefile->ReadFloat( aimRate );

    savefile->ReadInt( numSearchListEntities );
    for ( i = 0; i == numSearchListEntities; i++ )
    {
        savefile->ReadObject( reinterpret_cast<idClass *&>( entitySearchList[ i ] ) );
    }

    savefile->ReadInt( travelFlags );
    move.Restore( savefile );
    savedMove.Restore( savefile );
    savefile->ReadFloat( kickForce );
    savefile->ReadBool( ignore_obstacles );
    savefile->ReadFloat( blockedRadius );
    savefile->ReadInt( blockedMoveTime );

    enemy.Restore( savefile );
    savefile->ReadVec3( lastVisibleEnemyPos );
    savefile->ReadVec3( lastVisibleEnemyEyeOffset );
    savefile->ReadVec3( lastVisibleReachableEnemyPos );
    savefile->ReadVec3( lastReachableEnemyPos );

    savefile->ReadBool( lastHitCheckResult );
    savefile->ReadInt( lastHitCheckTime );

    SetAAS();

    LinkScriptVariables();
}

/*
===============
botAi::PrepareForRestart
TinMan: Get ready for map restart
================
*/
void botAi::PrepareForRestart( void )
{
    //gameLocal.Printf( "[PrepareForRestart]\n" );
    // we will be restarting program, clear the client entities from program-related things first
    ShutdownThreads();
}

/*
===============
botAi::Restart
TinMan: Re init after map restart
================
*/
void botAi::Restart( void )
{
    //gameLocal.Printf( "[Restart]\n" );

    Init();

    LinkScriptVariables();

    BecomeActive( TH_THINK );
}

/*
==============
botAi::LinkScriptVariables
==============
*/
void botAi::LinkScriptVariables( void )
{
    AI_FORWARD.LinkTo(			scriptObject, "AI_FORWARD" );
    AI_BACKWARD.LinkTo(			scriptObject, "AI_BACKWARD" );
    AI_STRAFE_LEFT.LinkTo(		scriptObject, "AI_STRAFE_LEFT" );
    AI_STRAFE_RIGHT.LinkTo(		scriptObject, "AI_STRAFE_RIGHT" );
    AI_ATTACK_HELD.LinkTo(		scriptObject, "AI_ATTACK_HELD" );
    AI_WEAPON_FIRED.LinkTo(		scriptObject, "AI_WEAPON_FIRED" );
    AI_JUMP.LinkTo(				scriptObject, "AI_JUMP" );
    AI_DEAD.LinkTo(				scriptObject, "AI_DEAD" );
    AI_CROUCH.LinkTo(			scriptObject, "AI_CROUCH" );
    AI_ONGROUND.LinkTo(			scriptObject, "AI_ONGROUND" );
    AI_ONLADDER.LinkTo(			scriptObject, "AI_ONLADDER" );
    AI_HARDLANDING.LinkTo(		scriptObject, "AI_HARDLANDING" );
    AI_SOFTLANDING.LinkTo(		scriptObject, "AI_SOFTLANDING" );
    AI_RUN.LinkTo(				scriptObject, "AI_RUN" );
    AI_PAIN.LinkTo(				scriptObject, "AI_PAIN" );
    AI_RELOAD.LinkTo(			scriptObject, "AI_RELOAD" );
    AI_TELEPORT.LinkTo(			scriptObject, "AI_TELEPORT" );
    AI_TURN_LEFT.LinkTo(		scriptObject, "AI_TURN_LEFT" );
    AI_TURN_RIGHT.LinkTo(		scriptObject, "AI_TURN_RIGHT" );


    AI_ENEMY_VISIBLE.LinkTo(	scriptObject, "AI_ENEMY_VISIBLE" );
    AI_ENEMY_IN_FOV.LinkTo(		scriptObject, "AI_ENEMY_IN_FOV" );
    AI_ENEMY_DEAD.LinkTo(		scriptObject, "AI_ENEMY_DEAD" );
    AI_MOVE_DONE.LinkTo(		scriptObject, "AI_MOVE_DONE" );
    AI_BLOCKED.LinkTo(			scriptObject, "AI_BLOCKED" );
    AI_DEST_UNREACHABLE.LinkTo( scriptObject, "AI_DEST_UNREACHABLE" );
    AI_OBSTACLE_IN_PATH.LinkTo(	scriptObject, "AI_OBSTACLE_IN_PATH" );
    AI_PUSHED.LinkTo(			scriptObject, "AI_PUSHED" );

    AI_WEAPON_FIRE.LinkTo(		scriptObject, "AI_WEAPON_FIRE" );

    BOT_COMMAND.LinkTo(			scriptObject, "BOT_COMMAND" );
}

/*
=====================
botAi::Think
=====================
*/
void botAi::Think( void )
{
    //gameLocal.Printf( "--------- Botthink ----------\n" ); // TinMan: *debug*
}

/*
================
botAi::GetBodyState
TinMan: Grab updated information from fakeclient, this will be info from last frame since bot thinks before fakeclient
================
*/
void botAi::GetBodyState( void )
{
    // TinMan: *todo* Encounted some wierdness if set scriptbool stright from playerEnts bool, it would be set in gamecode but not set for script.
    if ( playerEnt->AI_DEAD )
    {
        AI_DEAD	= true;
    }
    else
    {
        AI_DEAD = false;
    }

    if ( playerEnt->AI_PAIN )
    {
        AI_PAIN	= true;
    }
    else
    {
        AI_PAIN = false;
    }

    if ( playerEnt->AI_ONGROUND)
    {
        AI_ONGROUND	= true;
    }
    else
    {
        AI_ONGROUND = false;
    }

    if ( playerEnt->AI_TELEPORT )
    {
        AI_TELEPORT	= true;
    }
    else
    {
        AI_TELEPORT = false;
    }
}

/*
================
botAi::ClearInput
TinMan:
================
*/
void botAi::ClearInput( void )
{
    moveDir.Zero();
    moveSpeed = 0.0f;
    AI_FORWARD = false;
    AI_BACKWARD = false;
    AI_STRAFE_RIGHT = false;
    AI_STRAFE_LEFT = false;
    AI_JUMP = false;
    AI_CROUCH = false;
    AI_WEAPON_FIRE = false;
}

/*
================
botAi::UpdateViewAngles
TinMan: Aim towards aimDir
viewDir: Set by script - dir of where bot wants to aim
aimRate: Rate at which view changes to match aim - like the mouserate/speed a player can turn view
================
*/
void botAi::UpdateViewAngles( void )
{
    idVec3		viewPos;
    idMat3		axis;
    idAngles 	idealViewAngles;
    idAngles	delta;

    playerEnt->GetViewPos( viewPos, axis );
    viewAngles = playerEnt->viewAngles; // TinMan: Make sure we're cooking with the right ingredients

    viewDir.Normalize(); // TinMan: Viewdir is set by bot, there it should look
    idealViewAngles.pitch = -idMath::AngleNormalize180( viewDir.ToPitch() );
    idealViewAngles.yaw = idMath::AngleNormalize180( viewDir.ToYaw() );
    idealViewAngles.roll = 0;

    delta = idealViewAngles - viewAngles;

    // TinMan: Fix popping angles
    if ( ( delta.pitch > 180.0f ) || ( delta.pitch <= -180.0f ) )
    {
        delta.pitch = 360.0f - delta.pitch;
    }
    if ( delta.yaw > 180.0f )
    {
        delta.yaw -= 360.0f;
    }
    else if ( delta.yaw <= -180.0f )
    {
        delta.yaw += 360.0f;
    }

    viewAngles = viewAngles + delta * aimRate;

    //viewAngles = newAimAng; // TinMan: *test* no blend
}

/*
================
botAi::UpdateUserCmd
TinMan: Convert bot input to usrcmds for client, quite obviously inspired by q3bot
================
*/
void botAi::UpdateUserCmd( void )
{
    idAngles deltaViewAngles;
    int i;
    idVec3 forward, right;

    usercmd_t *usercmd;

    usercmd = &gameLocal.usercmds[ clientID ];
    memset( usercmd, 0, sizeof( usercmd_t ) );

    // TinMan: viewang to usrcmd, usrcmd is viewang - deltaviewang
    deltaViewAngles = playerEnt->GetDeltaViewAngles();
    for ( i = 0; i < 3; i++ )
    {
        usercmd->angles[i] = ANGLE2SHORT( viewAngles[i] - deltaViewAngles[i] );
    }

    // TinMan: Throw in buttons
    if ( AI_WEAPON_FIRE )   // TinMan: Bang bang, you're dead. No! I shot you first!
    {
        usercmd->buttons |= BUTTON_ATTACK;
    }

    if ( AI_RUN )
    {
        usercmd->buttons |= BUTTON_RUN;
    }

    moveDir.Normalize();

    if ( moveSpeed > 400 )
    {
        moveSpeed = 400;
    }
    else if ( moveSpeed < -400 )
    {
        moveSpeed = -400;
    }

    moveSpeed = moveSpeed * 127 / 400; // TinMan: Scale from [0, 400] to [0, 127]

    // TinMan: Grab vectors of viewangles for movement
    idAngles angles( viewAngles.pitch ? viewAngles.pitch : 0, viewAngles.yaw, NULL );
    angles.ToVectors( &forward, &right, NULL );

    // TinMan: Convert moveDir to usrcmd
    //usercmd->forwardmove = DotProduct( forward, moveDir ) * moveSpeed;
    usercmd->forwardmove = ( forward * moveDir ) * moveSpeed;
    //usercmd->rightmove	= DotProduct( right, moveDir ) * moveSpeed;
    usercmd->rightmove	= ( right * moveDir ) * moveSpeed;
    usercmd->upmove		= abs( forward.z ) * moveDir.z * moveSpeed;

    // TinMan: Manual movement
    if ( AI_FORWARD )
    {
        usercmd->forwardmove += 127.0f;
    }
    if ( AI_BACKWARD )
    {
        usercmd->forwardmove -= 127.0f;
    }

    if ( AI_STRAFE_RIGHT )
    {
        usercmd->rightmove += 127.0f;
    }
    if ( AI_STRAFE_LEFT )
    {
        usercmd->rightmove -= 127.0f;
    }

    if ( AI_JUMP )
    {
        usercmd->upmove += 127.0f;
    }
    if ( AI_CROUCH )
    {
        usercmd->upmove -= 127.0f;
    }

    //gameLocal.Printf( "[botAi::UpdateUserCmd][fwd: %d][right: %d][up: %d]\n", usercmd->forwardmove, usercmd->rightmove, usercmd->upmove );
}

/***********************************************************************

	script state management

***********************************************************************/

/*
================
botAi::ShutdownThreads
================
*/
void botAi::ShutdownThreads( void )
{
    if ( scriptThread )
    {
        scriptThread->EndThread();
        scriptThread->PostEventMS( &EV_Remove, 0 );
        delete scriptThread;
        scriptThread = NULL;
    }
}

/*
================
botAi::ConstructScriptObject

id: Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *botAi::ConstructScriptObject( void )
{
    const function_t *constructor;

    // make sure we have a scriptObject
    if ( !scriptObject.HasObject() )
    {
        gameLocal.Error( "No scriptobject set on '%s'.  Check the '%s' entityDef.", name.c_str(), GetEntityDefName() );
    }

    if ( !scriptThread )
    {
        // create script thread
        scriptThread = new idThread();
        scriptThread->ManualDelete();
        scriptThread->ManualControl();
        scriptThread->SetThreadName( name.c_str() );
    }
    else
    {
        scriptThread->EndThread();
    }

    // call script object's constructor
    constructor = scriptObject.GetConstructor();
    if ( !constructor )
    {
        gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject.GetTypeName(), name.c_str() );
    }

    // init the script object's data
    scriptObject.ClearObject();

    // just set the current function on the script.  we'll execute in the subclasses.
    scriptThread->CallFunction( this, constructor, true );

    return scriptThread;
}

/*
=====================
botAi::GetScriptFunction
=====================
*/
const function_t *botAi::GetScriptFunction( const char *funcname )
{
    const function_t *func;

    func = scriptObject.GetFunction( funcname );
    if ( !func )
    {
        scriptThread->Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
    }

    return func;
}

/*
=====================
botAi::SetState
=====================
*/
void botAi::SetState( const function_t *newState )
{
    if ( !newState )
    {
        gameLocal.Error( "botAi::SetState: Null state" );
    }

    if ( ai_debugScript.GetInteger() == entityNumber )
    {
        gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
    }

    state = newState;
    idealState = state;
    scriptThread->CallFunction( this, state, true );
}

/*
=====================
botAi::SetState
=====================
*/
void botAi::SetState( const char *statename )
{
    const function_t *newState;

    newState = GetScriptFunction( statename );
    SetState( newState );
}

/*
=====================
botAi::UpdateScript
=====================
*/
void botAi::UpdateScript( void )
{
    int	i;

    if ( ai_debugScript.GetInteger() == entityNumber )
    {
        scriptThread->EnableDebugInfo();
    }
    else
    {
        scriptThread->DisableDebugInfo();
    }

    // a series of state changes can happen in a single frame.
    // this loop limits them in case we've entered an infinite loop.
    for( i = 0; i < 20; i++ )
    {
        if ( idealState != state )
        {
            SetState( idealState );
        }

        // don't call script until it's done waiting
        if ( scriptThread->IsWaiting() )
        {
            break;
        }

        scriptThread->Execute();
        if ( idealState == state )
        {
            break;
        }
    }

    if ( i == 20 )
    {
        scriptThread->Warning( "botAi::UpdateScript: exited loop to prevent lockup" );
    }
}

/***********************************************************************

	navigation

***********************************************************************/

/*
============
botAi::KickObstacles
============
*/
void botAi::KickObstacles( const idVec3 &dir, float force, idEntity *alwaysKick )
{
    int i, numListedClipModels;
    idBounds clipBounds;
    idEntity *obEnt;
    idClipModel *clipModel;
    idClipModel *clipModelList[ MAX_GENTITIES ];
    int clipmask;
    idVec3 org;
    idVec3 forceVec;
    idVec3 delta;
    idVec2 perpendicular;

    org = physicsObject->GetOrigin();

    // find all possible obstacles
    clipBounds = physicsObject->GetAbsBounds();
    clipBounds.TranslateSelf( dir * 32.0f );
    clipBounds.ExpandSelf( 8.0f );
    clipBounds.AddPoint( org );
    clipmask = physicsObject->GetClipMask();
    numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipmask, clipModelList, MAX_GENTITIES );
    for ( i = 0; i < numListedClipModels; i++ )
    {
        clipModel = clipModelList[i];
        obEnt = clipModel->GetEntity();
        if ( obEnt == alwaysKick )
        {
            // we'll kick this one outside the loop
            continue;
        }

        if ( !clipModel->IsTraceModel() )
        {
            continue;
        }

        if ( obEnt->IsType( idMoveable::Type ) && obEnt->GetPhysics()->IsPushable() )
        {
            delta = obEnt->GetPhysics()->GetOrigin() - org;
            delta.NormalizeFast();
            perpendicular.x = -delta.y;
            perpendicular.y = delta.x;
            delta.z += 0.5f;
            delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
            forceVec = delta * force * obEnt->GetPhysics()->GetMass();
            obEnt->ApplyImpulse( playerEnt, 0, obEnt->GetPhysics()->GetOrigin(), forceVec );
        }
    }

    if ( alwaysKick )
    {
        delta = alwaysKick->GetPhysics()->GetOrigin() - org;
        delta.NormalizeFast();
        perpendicular.x = -delta.y;
        perpendicular.y = delta.x;
        delta.z += 0.5f;
        delta.ToVec2() += perpendicular * gameLocal.random.CRandomFloat() * 0.5f;
        forceVec = delta * force * alwaysKick->GetPhysics()->GetMass();
        alwaysKick->ApplyImpulse( playerEnt, 0, alwaysKick->GetPhysics()->GetOrigin(), forceVec );
    }
}

/*
============
ValidForBounds
TinMan: Odd that this should be orphaned like it was ( see idAI )
============
*/
bool botAi::ValidForBounds( const idAASSettings *settings, const idBounds &bounds )
{
    int i;

    for ( i = 0; i < 3; i++ )
    {
        if ( bounds[0][i] < settings->boundingBoxes[0][0][i] )
        {
            return false;
        }
        if ( bounds[1][i] > settings->boundingBoxes[0][1][i] )
        {
            return false;
        }
    }
    return true;
}

/*
=====================
botAi::SetAAS
=====================
*/
void botAi::SetAAS( void )
{
    idStr use_aas;

    //spawnArgs.GetString( "use_aas", NULL, use_aas );
    //aas = gameLocal.GetAAS( use_aas );
    aas = gameLocal.GetAAS( "aas48" ); // TinMan: Hard coded the bots aas size
    if ( aas )
    {
        const idAASSettings *settings = aas->GetSettings();
        if ( settings )
        {
            // TinMan: *todo* why isn't physics bounds returning something nice?
            /*if ( !ValidForBounds( settings, physicsObject->GetBounds() ) ) {
            	gameLocal.Error( "%s cannot use use_aas %s\n", name.c_str(), use_aas.c_str() );
            }*/
            float height = settings->maxStepHeight;
            physicsObject->SetMaxStepHeight( height );
            return;
        }
        else
        {
            aas = NULL;
        }
    }
    //gameLocal.Error( "WARNING: %s has no AAS file\n", name.c_str() );
    gameLocal.Error( "Bot cannot find AAS file for map\n" ); // TinMan: No aas, no play.
}

/*
=====================
botAi::DrawRoute
=====================
*/
void botAi::DrawRoute( void ) const
{
    if ( aas && move.toAreaNum && move.moveCommand != MOVE_NONE && move.moveCommand != MOVE_WANDER && move.moveCommand != MOVE_FACE_ENEMY && move.moveCommand != MOVE_FACE_ENTITY && move.moveCommand != MOVE_TO_POSITION_DIRECT )
    {
        if ( move.moveType == MOVETYPE_FLY )
        {
            aas->ShowFlyPath( physicsObject->GetOrigin(), move.toAreaNum, move.moveDest );
        }
        else
        {
            aas->ShowWalkPath( physicsObject->GetOrigin(), move.toAreaNum, move.moveDest );
        }
    }
}

/*
=====================
botAi::ReachedPos
=====================
*/
bool botAi::ReachedPos( const idVec3 &pos, const moveCommand_t moveCommand ) const
{
    if ( move.moveType == MOVETYPE_SLIDE )
    {
        idBounds bnds( idVec3( -4, -4.0f, -8.0f ), idVec3( 4.0f, 4.0f, 64.0f ) );
        bnds.TranslateSelf( physicsObject->GetOrigin() );
        if ( bnds.ContainsPoint( pos ) )
        {
            return true;
        }
    }
    else
    {
        if ( ( moveCommand == MOVE_TO_ENEMY ) || ( moveCommand == MOVE_TO_ENTITY ) )
        {
            if ( physicsObject->GetAbsBounds().IntersectsBounds( idBounds( pos ).Expand( 8.0f ) ) )
            {
                return true;
            }
        }
        else
        {
            idBounds bnds( idVec3( -16.0, -16.0f, -8.0f ), idVec3( 16.0, 16.0f, 64.0f ) );
            bnds.TranslateSelf( physicsObject->GetOrigin() );
            if ( bnds.ContainsPoint( pos ) )
            {
                return true;
            }
        }
    }
    return false;
}

/*
=====================
botAi::PointReachableAreaNum
=====================
*/
int botAi::PointReachableAreaNum( const idVec3 &pos, const float boundsScale ) const
{
    int areaNum;
    idVec3 size;
    idBounds bounds;

    if ( !aas )
    {
        return 0;
    }

    size = aas->GetSettings()->boundingBoxes[0][1] * boundsScale;
    bounds[0] = -size;
    size.z = 32.0f;
    bounds[1] = size;

    if ( move.moveType == MOVETYPE_FLY )
    {
        areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK | AREA_REACHABLE_FLY );
    }
    else
    {
        areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
    }

    return areaNum;
}

/*
=====================
botAi::PathToGoal
=====================
*/
bool botAi::PathToGoal( aasPath_t &path, int areaNum, const idVec3 &origin, int goalAreaNum, const idVec3 &goalOrigin ) const
{
    idVec3 org;
    idVec3 goal;

    if ( !aas )
    {
        return false;
    }

    org = origin;
    aas->PushPointIntoAreaNum( areaNum, org );
    if ( !areaNum )
    {
        return false;
    }

    goal = goalOrigin;
    aas->PushPointIntoAreaNum( goalAreaNum, goal );
    if ( !goalAreaNum )
    {
        return false;
    }

    if ( move.moveType == MOVETYPE_FLY )
    {
        return aas->FlyPathToGoal( path, areaNum, org, goalAreaNum, goal, travelFlags );
    }
    else
    {
        return aas->WalkPathToGoal( path, areaNum, org, goalAreaNum, goal, travelFlags );
    }
}

/*
=====================
botAi::TravelDistance

Returns the approximate travel distance from one position to the goal, or if no AAS, the straight line distance.

This is feakin' slow, so it's not good to do it too many times per frame.  It also is slower the further you
are from the goal, so try to break the goals up into shorter distances.
=====================
*/
float botAi::TravelDistance( const idVec3 &start, const idVec3 &end ) const
{
    int			fromArea;
    int			toArea;
    float		dist;
    idVec2		delta;
    aasPath_t	path;

    if ( !aas )
    {
        // no aas, so just take the straight line distance
        delta = end.ToVec2() - start.ToVec2();
        dist = delta.LengthFast();

        if ( ai_debugMove.GetBool() )
        {
            gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
            gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
        }

        return dist;
    }

    fromArea = PointReachableAreaNum( start );
    toArea = PointReachableAreaNum( end );

    if ( !fromArea || !toArea )
    {
        // can't seem to get there
        return -1;
    }

    if ( fromArea == toArea )
    {
        // same area, so just take the straight line distance
        delta = end.ToVec2() - start.ToVec2();
        dist = delta.LengthFast();

        if ( ai_debugMove.GetBool() )
        {
            gameRenderWorld->DebugLine( colorBlue, start, end, gameLocal.msec, false );
            gameRenderWorld->DrawText( va( "%d", ( int )dist ), ( start + end ) * 0.5f, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3() );
        }

        return dist;
    }

    idReachability *reach;
    int travelTime;
    if ( !aas->RouteToGoalArea( fromArea, start, toArea, travelFlags, travelTime, &reach ) )
    {
        return -1;
    }

    if ( ai_debugMove.GetBool() )
    {
        if ( move.moveType == MOVETYPE_FLY )
        {
            aas->ShowFlyPath( start, toArea, end );
        }
        else
        {
            aas->ShowWalkPath( start, toArea, end );
        }
    }

    return travelTime;
}

/*
=====================
botAi::StopMove
=====================
*/
void botAi::StopMove( moveStatus_t status )
{
    AI_MOVE_DONE		= true;
    //AI_FORWARD			= false;
    move.moveCommand	= MOVE_NONE;
    move.moveStatus		= status;
    move.toAreaNum		= 0;
    move.goalEntity		= NULL;
    move.moveDest		= physicsObject->GetOrigin();
    AI_DEST_UNREACHABLE	= false;
    AI_OBSTACLE_IN_PATH = false;
    AI_BLOCKED			= false;
    move.startTime		= gameLocal.time;
    move.duration		= 0;
    move.range			= 0.0f;
    move.speed			= 0.0f;
    move.anim			= 0;
    move.moveDir.Zero();
    move.lastMoveOrigin.Zero();
    move.lastMoveTime	= gameLocal.time;
}

/*
=====================
botAi::SetMoveToEnemy
=====================
*/
bool botAi::SetMoveToEnemy( void )
{
    int			areaNum;
    aasPath_t	path;
    idActor		*enemyEnt = enemy.GetEntity();

    if ( !enemyEnt )
    {
        StopMove( MOVE_STATUS_DEST_NOT_FOUND );
        return false;
    }

    if ( ReachedPos( lastVisibleReachableEnemyPos, MOVE_TO_ENEMY ) )
    {
        if ( !ReachedPos( lastVisibleEnemyPos, MOVE_TO_ENEMY ) || !AI_ENEMY_VISIBLE )
        {
            StopMove( MOVE_STATUS_DEST_UNREACHABLE );
            AI_DEST_UNREACHABLE = true;
            return false;
        }
        StopMove( MOVE_STATUS_DONE );
        return true;
    }

    idVec3 pos = lastVisibleReachableEnemyPos;

    move.toAreaNum = 0;
    if ( aas )
    {
        move.toAreaNum = PointReachableAreaNum( pos );
        aas->PushPointIntoAreaNum( move.toAreaNum, pos );

        areaNum	= PointReachableAreaNum( physicsObject->GetOrigin() );
        if ( !PathToGoal( path, areaNum, physicsObject->GetOrigin(), move.toAreaNum, pos ) )
        {
            AI_DEST_UNREACHABLE = true;
            return false;
        }
    }

    if ( !move.toAreaNum )
    {
        // if only trying to update the enemy position
        if ( move.moveCommand == MOVE_TO_ENEMY )
        {
            if ( !aas )
            {
                // keep the move destination up to date for wandering
                move.moveDest = pos;
            }
            return false;
        }

        if ( !NewWanderDir( pos ) )
        {
            StopMove( MOVE_STATUS_DEST_UNREACHABLE );
            AI_DEST_UNREACHABLE = true;
            return false;
        }
    }

    if ( move.moveCommand != MOVE_TO_ENEMY )
    {
        move.moveCommand	= MOVE_TO_ENEMY;
        move.startTime		= gameLocal.time;
    }

    move.moveDest		= pos;
    move.goalEntity		= enemyEnt;
    //move.speed			= fly_speed;
    move.moveStatus		= MOVE_STATUS_MOVING;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::SetMoveToEntity
=====================
*/
bool botAi::SetMoveToEntity( idEntity *ent )
{
    int			areaNum;
    aasPath_t	path;
    idVec3		pos;

    if ( !ent )
    {
        StopMove( MOVE_STATUS_DEST_NOT_FOUND );
        return false;
    }

    pos = ent->GetPhysics()->GetOrigin();
    if ( ( move.moveType != MOVETYPE_FLY ) && ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntityOrigin != pos ) ) )
    {
        ent->GetFloorPos( 64.0f, pos );
    }

    if ( ReachedPos( pos, MOVE_TO_ENTITY ) )
    {
        StopMove( MOVE_STATUS_DONE );
        return true;
    }

    move.toAreaNum = 0;
    if ( aas )
    {
        move.toAreaNum = PointReachableAreaNum( pos );
        aas->PushPointIntoAreaNum( move.toAreaNum, pos );

        areaNum	= PointReachableAreaNum( physicsObject->GetOrigin() );
        if ( !PathToGoal( path, areaNum, physicsObject->GetOrigin(), move.toAreaNum, pos ) )
        {
            AI_DEST_UNREACHABLE = true;
            return false;
        }
    }

    if ( !move.toAreaNum )
    {
        // if only trying to update the entity position
        if ( move.moveCommand == MOVE_TO_ENTITY )
        {
            if ( !aas )
            {
                // keep the move destination up to date for wandering
                move.moveDest = pos;
            }
            return false;
        }

        if ( !NewWanderDir( pos ) )
        {
            StopMove( MOVE_STATUS_DEST_UNREACHABLE );
            AI_DEST_UNREACHABLE = true;
            return false;
        }
    }

    if ( ( move.moveCommand != MOVE_TO_ENTITY ) || ( move.goalEntity.GetEntity() != ent ) )
    {
        move.startTime		= gameLocal.time;
        move.goalEntity		= ent;
        move.moveCommand	= MOVE_TO_ENTITY;
    }

    move.moveDest			= pos;

    move.goalEntityOrigin	= ent->GetPhysics()->GetOrigin();
    move.moveStatus			= MOVE_STATUS_MOVING;
    //move.speed				= fly_speed;
    AI_MOVE_DONE			= false;
    AI_DEST_UNREACHABLE		= false;
    //AI_FORWARD				= true;

    return true;
}

/*
=====================
botAi::SetMoveOutOfRange
=====================
*/
bool botAi::SetMoveOutOfRange( idEntity *ent, float range )
{
    int				areaNum;
    aasObstacle_t	obstacle;
    aasGoal_t		goal;
    idBounds		bounds;
    idVec3			pos;

    if ( !aas || !ent )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    const idVec3 &org = physicsObject->GetOrigin();
    areaNum	= PointReachableAreaNum( org );

    // consider the entity the monster is getting close to as an obstacle
    obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

    if ( ent == enemy.GetEntity() )
    {
        pos = lastVisibleEnemyPos;
    }
    else
    {
        pos = ent->GetPhysics()->GetOrigin();
    }

    idAASFindAreaOutOfRange findGoal( pos, range );
    if ( !aas->FindNearestGoal( goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal ) )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    if ( ReachedPos( goal.origin, move.moveCommand ) )
    {
        StopMove( MOVE_STATUS_DONE );
        return true;
    }

    move.moveDest		= goal.origin;
    move.toAreaNum		= goal.areaNum;
    move.goalEntity		= ent;
    move.moveCommand	= MOVE_OUT_OF_RANGE;
    move.moveStatus		= MOVE_STATUS_MOVING;
    move.range			= range;
    //move.speed			= fly_speed;
    move.startTime		= gameLocal.time;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::SetMoveToAttackPosition
=====================
*/
bool botAi::SetMoveToAttackPosition( idEntity *ent )
{
    int				areaNum;
    aasObstacle_t	obstacle;
    aasGoal_t		goal;
    idBounds		bounds;
    idVec3			pos;

    if ( !aas || !ent )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    const idVec3 &org = physicsObject->GetOrigin();
    areaNum	= PointReachableAreaNum( org );

    // consider the entity the monster is getting close to as an obstacle
    obstacle.absBounds = ent->GetPhysics()->GetAbsBounds();

    if ( ent == enemy.GetEntity() )
    {
        pos = lastVisibleEnemyPos;
    }
    else
    {
        pos = ent->GetPhysics()->GetOrigin();
    }

    botAASFindAttackPosition findGoal( playerEnt, physicsObject->GetGravityAxis(), ent, pos, playerEnt->EyeOffset() );
    if ( !aas->FindNearestGoal( goal, areaNum, org, pos, travelFlags, &obstacle, 1, findGoal ) )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    move.moveDest		= goal.origin;
    move.toAreaNum		= goal.areaNum;
    move.goalEntity		= ent;
    move.moveCommand	= MOVE_TO_ATTACK_POSITION;
    move.moveStatus		= MOVE_STATUS_MOVING;
    //move.speed			= fly_speed;
    move.startTime		= gameLocal.time;
    //move.anim			= attack_anim;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::SetMoveToPosition
=====================
*/
bool botAi::SetMoveToPosition( const idVec3 &pos )
{
    idVec3		org;
    int			areaNum;
    aasPath_t	path;

    if ( ReachedPos( pos, move.moveCommand ) )
    {
        StopMove( MOVE_STATUS_DONE );
        return true;
    }

    org = pos;
    move.toAreaNum = 0;
    if ( aas )
    {
        move.toAreaNum = PointReachableAreaNum( org );
        aas->PushPointIntoAreaNum( move.toAreaNum, org );

        areaNum	= PointReachableAreaNum( physicsObject->GetOrigin() );
        if ( !PathToGoal( path, areaNum, physicsObject->GetOrigin(), move.toAreaNum, org ) )
        {
            StopMove( MOVE_STATUS_DEST_UNREACHABLE );
            AI_DEST_UNREACHABLE = true;
            return false;
        }
    }

    if ( !move.toAreaNum && !NewWanderDir( org ) )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    move.moveDest		= org;
    move.goalEntity		= NULL;
    move.moveCommand	= MOVE_TO_POSITION;
    move.moveStatus		= MOVE_STATUS_MOVING;
    move.startTime		= gameLocal.time;
    //move.speed			= fly_speed;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::SetMoveToCover
=====================
*/
bool botAi::SetMoveToCover( idEntity *entity, const idVec3 &hideFromPos )
{
    int				areaNum;
    aasObstacle_t	obstacle;
    aasGoal_t		hideGoal;
    idBounds		bounds;

    if ( !aas || !entity )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    const idVec3 &org = physicsObject->GetOrigin();
    areaNum	= PointReachableAreaNum( org );

    // consider the entity the monster tries to hide from as an obstacle
    obstacle.absBounds = entity->GetPhysics()->GetAbsBounds();

    idAASFindCover findCover( hideFromPos );
    if ( !aas->FindNearestGoal( hideGoal, areaNum, org, hideFromPos, travelFlags, &obstacle, 1, findCover ) )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    if ( ReachedPos( hideGoal.origin, move.moveCommand ) )
    {
        StopMove( MOVE_STATUS_DONE );
        return true;
    }

    move.moveDest		= hideGoal.origin;
    move.toAreaNum		= hideGoal.areaNum;
    move.goalEntity		= entity;
    move.moveCommand	= MOVE_TO_COVER;
    move.moveStatus		= MOVE_STATUS_MOVING;
    move.startTime		= gameLocal.time;
    //move.speed			= fly_speed;
    AI_MOVE_DONE		= false;
    AI_DEST_UNREACHABLE = false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::WanderAround
=====================
*/
bool botAi::WanderAround( void )
{
    StopMove( MOVE_STATUS_DONE );

    move.moveDest = physicsObject->GetOrigin() + playerEnt->viewAxis[ 0 ] * physicsObject->GetGravityAxis() * 256.0f;
    if ( !NewWanderDir( move.moveDest ) )
    {
        StopMove( MOVE_STATUS_DEST_UNREACHABLE );
        AI_DEST_UNREACHABLE = true;
        return false;
    }

    move.moveCommand	= MOVE_WANDER;
    move.moveStatus		= MOVE_STATUS_MOVING;
    move.startTime		= gameLocal.time;
    //move.speed			= fly_speed;
    AI_MOVE_DONE		= false;
    //AI_FORWARD			= true;

    return true;
}

/*
=====================
botAi::MoveDone
=====================
*/
bool botAi::MoveDone( void ) const
{
    return ( move.moveCommand == MOVE_NONE );
}

/*
================
botAi::StepDirection
================
*/
bool botAi::StepDirection( float dir )
{
    predictedPath_t path;
    idVec3 org;

    move.wanderYaw = dir;
    move.moveDir = idAngles( 0, move.wanderYaw, 0 ).ToForward();

    org = physicsObject->GetOrigin();

    idAI::PredictPath( playerEnt, aas, org, move.moveDir * 48.0f, 1000, 1000, ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

    if ( path.blockingEntity && ( ( move.moveCommand == MOVE_TO_ENEMY ) || ( move.moveCommand == MOVE_TO_ENTITY ) ) && ( path.blockingEntity == move.goalEntity.GetEntity() ) )
    {
        // don't report being blocked if we ran into our goal entity
        return true;
    }

    if ( ( move.moveType == MOVETYPE_FLY ) && ( path.endEvent == SE_BLOCKED ) )
    {
        float z;

        move.moveDir = path.endVelocity * 1.0f / 48.0f;

        // trace down to the floor and see if we can go forward
        idAI::PredictPath( playerEnt, aas, org, idVec3( 0.0f, 0.0f, -1024.0f ), 1000, 1000, SE_BLOCKED, path );

        idVec3 floorPos = path.endPos;
        idAI::PredictPath( playerEnt, aas, floorPos, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
        if ( !path.endEvent )
        {
            move.moveDir.z = -1.0f;
            return true;
        }

        // trace up to see if we can go over something and go forward
        idAI::PredictPath( playerEnt, aas, org, idVec3( 0.0f, 0.0f, 256.0f ), 1000, 1000, SE_BLOCKED, path );

        idVec3 ceilingPos = path.endPos;

        for( z = org.z; z <= ceilingPos.z + 64.0f; z += 64.0f )
        {
            idVec3 start;
            if ( z <= ceilingPos.z )
            {
                start.x = org.x;
                start.y = org.y;
                start.z = z;
            }
            else
            {
                start = ceilingPos;
            }
            idAI::PredictPath( playerEnt, aas, start, move.moveDir * 48.0f, 1000, 1000, SE_BLOCKED, path );
            if ( !path.endEvent )
            {
                move.moveDir.z = 1.0f;
                return true;
            }
        }
        return false;
    }

    return ( path.endEvent == 0 );
}

/*
================
botAi::NewWanderDir
================
*/
bool botAi::NewWanderDir( const idVec3 &dest )
{
    float	deltax, deltay;
    float	d[ 3 ];
    float	tdir, olddir, turnaround;

    move.nextWanderTime = gameLocal.time + ( gameLocal.random.RandomFloat() * 500 + 500 );

    olddir = idMath::AngleNormalize360( ( int )( playerEnt->viewAngles.yaw / 45 ) * 45 );
    turnaround = idMath::AngleNormalize360( olddir - 180 );

    idVec3 org = physicsObject->GetOrigin();
    deltax = dest.x - org.x;
    deltay = dest.y - org.y;
    if ( deltax > 10 )
    {
        d[ 1 ]= 0;
    }
    else if ( deltax < -10 )
    {
        d[ 1 ] = 180;
    }
    else
    {
        d[ 1 ] = DI_NODIR;
    }

    if ( deltay < -10 )
    {
        d[ 2 ] = 270;
    }
    else if ( deltay > 10 )
    {
        d[ 2 ] = 90;
    }
    else
    {
        d[ 2 ] = DI_NODIR;
    }

    // try direct route
    if ( d[ 1 ] != DI_NODIR && d[ 2 ] != DI_NODIR )
    {
        if ( d[ 1 ] == 0 )
        {
            tdir = d[ 2 ] == 90 ? 45 : 315;
        }
        else
        {
            tdir = d[ 2 ] == 90 ? 135 : 215;
        }

        if ( tdir != turnaround && StepDirection( tdir ) )
        {
            return true;
        }
    }

    // try other directions
    if ( ( gameLocal.random.RandomInt() & 1 ) || abs( deltay ) > abs( deltax ) )
    {
        tdir = d[ 1 ];
        d[ 1 ] = d[ 2 ];
        d[ 2 ] = tdir;
    }

    if ( d[ 1 ] != DI_NODIR && d[ 1 ] != turnaround && StepDirection( d[1] ) )
    {
        return true;
    }

    if ( d[ 2 ] != DI_NODIR && d[ 2 ] != turnaround	&& StepDirection( d[ 2 ] ) )
    {
        return true;
    }

    // there is no direct path to the player, so pick another direction
    if ( olddir != DI_NODIR && StepDirection( olddir ) )
    {
        return true;
    }

    // randomly determine direction of search
    if ( gameLocal.random.RandomInt() & 1 )
    {
        for( tdir = 0; tdir <= 315; tdir += 45 )
        {
            if ( tdir != turnaround && StepDirection( tdir ) )
            {
                return true;
            }
        }
    }
    else
    {
        for ( tdir = 315; tdir >= 0; tdir -= 45 )
        {
            if ( tdir != turnaround && StepDirection( tdir ) )
            {
                return true;
            }
        }
    }

    if ( turnaround != DI_NODIR && StepDirection( turnaround ) )
    {
        return true;
    }

    // can't move
    StopMove( MOVE_STATUS_DEST_UNREACHABLE );
    return false;
}

/*
=====================
botAi::GetMovePos
=====================
*/
bool botAi::GetMovePos( idVec3 &seekPos )
{
    int			areaNum;
    aasPath_t	path;
    bool		result;
    idVec3		org;

    org = physicsObject->GetOrigin();
    seekPos = org;

    switch( move.moveCommand )
    {
    case MOVE_NONE :
        seekPos = move.moveDest;
        return false;
        break;

    case MOVE_FACE_ENEMY :
    case MOVE_FACE_ENTITY :
        seekPos = move.moveDest;
        return false;
        break;

    case MOVE_TO_POSITION_DIRECT :
        seekPos = move.moveDest;
        if ( ReachedPos( move.moveDest, move.moveCommand ) )
        {
            StopMove( MOVE_STATUS_DONE );
        }
        return false;
        break;

    case MOVE_SLIDE_TO_POSITION :
        seekPos = org;
        return false;
        break;
    }

    if ( move.moveCommand == MOVE_TO_ENTITY )
    {
        SetMoveToEntity( move.goalEntity.GetEntity() );
    }

    move.moveStatus = MOVE_STATUS_MOVING;
    result = false;
    if ( gameLocal.time > move.blockTime )
    {
        if ( move.moveCommand == MOVE_WANDER )
        {
            move.moveDest = org + playerEnt->viewAxis[ 0 ] * physicsObject->GetGravityAxis() * 256.0f;
        }
        else
        {
            if ( ReachedPos( move.moveDest, move.moveCommand ) )
            {
                StopMove( MOVE_STATUS_DONE );
                seekPos	= org;
                return false;
            }
        }

        if ( aas && move.toAreaNum )
        {
            areaNum	= PointReachableAreaNum( org );
            if ( PathToGoal( path, areaNum, org, move.toAreaNum, move.moveDest ) )
            {
                seekPos = path.moveGoal;
                result = true;
                move.nextWanderTime = 0;

                // TinMan: Save info for more complex navigation
                move.secondaryMovePosition = path.secondaryGoal;
                move.pathType = path.type;
            }
            else
            {
                AI_DEST_UNREACHABLE = true;
            }
        }
    }

    if ( !result )
    {
        // wander around
        if ( ( gameLocal.time > move.nextWanderTime ) || !StepDirection( move.wanderYaw ) )
        {
            result = NewWanderDir( move.moveDest );
            if ( !result )
            {
                StopMove( MOVE_STATUS_DEST_UNREACHABLE );
                AI_DEST_UNREACHABLE = true;
                seekPos	= org;
                return false;
            }
        }
        else
        {
            result = true;
        }

        seekPos = org + move.moveDir * 2048.0f;
        if ( ai_debugMove.GetBool() )
        {
            gameRenderWorld->DebugLine( colorYellow, org, seekPos, gameLocal.msec, true );
        }
    }
    else
    {
        AI_DEST_UNREACHABLE = false;
    }

    if ( result && ( ai_debugMove.GetBool() ) )
    {
        gameRenderWorld->DebugLine( colorCyan, physicsObject->GetOrigin(), seekPos );
    }

    return result;
}

/*
=====================
botAi::EntityCanSeePos
=====================
*/
bool botAi::EntityCanSeePos( idActor *actor, const idVec3 &actorOrigin, const idVec3 &pos )
{
    idVec3 eye, point;
    trace_t results;
    pvsHandle_t handle;

    handle = gameLocal.pvs.SetupCurrentPVS( actor->GetPVSAreas(), actor->GetNumPVSAreas() );

    if ( !gameLocal.pvs.InCurrentPVS( handle, GetPVSAreas(), GetNumPVSAreas() ) )
    {
        gameLocal.pvs.FreeCurrentPVS( handle );
        return false;
    }

    gameLocal.pvs.FreeCurrentPVS( handle );

    eye = actorOrigin + actor->EyeOffset();

    point = pos;
    point[2] += 1.0f;

    physicsObject->DisableClip();

    gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
    if ( results.fraction >= 1.0f /* || ( gameLocal.GetTraceEntity( results ) == playerEnt ) */ )
    {
        physicsObject->EnableClip();
        return true;
    }

    /*
    const idBounds &bounds = physicsObject->GetBounds();
    point[2] += bounds[1][2] - bounds[0][2];

    gameLocal.clip.TracePoint( results, eye, point, MASK_SOLID, actor );
    physicsObject->EnableClip();
    */
    //if ( results.fraction >= 1.0f /* || ( gameLocal.GetTraceEntity( results ) == playerEnt ) */ ) {
    //	return true;
    //}

    return false;
}

/*
=====================
botAi::BlockedFailSafe
=====================
*/
void botAi::BlockedFailSafe( void )
{
    if ( !ai_blockedFailSafe.GetBool() || blockedRadius < 0.0f )
    {
        return;
    }
    if ( !physicsObject->HasGroundContacts() || /*enemy.GetEntity() == NULL ||*/
            ( physicsObject->GetOrigin() - move.lastMoveOrigin ).LengthSqr() > Square( blockedRadius ) )
    {
        move.lastMoveOrigin = physicsObject->GetOrigin();
        move.lastMoveTime = gameLocal.time;
    }
    if ( move.lastMoveTime < gameLocal.time - blockedMoveTime )
    {
        //if ( lastAttackTime < gameLocal.time - blockedAttackTime ) {
        AI_BLOCKED = true;
        move.lastMoveTime = gameLocal.time;
        //}
    }
}

/*
=====================
botAi::CheckObstacleAvoidance
=====================
*/
void botAi::CheckObstacleAvoidance( const idVec3 &goalPos, idVec3 &newPos )
{
    idEntity		*obstacle;
    obstaclePath_t	path;
    idVec3			dir;
    float			dist;
    bool			foundPath;

    if ( ignore_obstacles )
    {
        newPos = goalPos;
        move.obstacle = NULL;
        return;
    }

    const idVec3 &origin = physicsObject->GetOrigin();

    obstacle = NULL;
    AI_OBSTACLE_IN_PATH = false;
    foundPath = idAI::FindPathAroundObstacles( playerEnt->GetPhysics(), aas, enemy.GetEntity(), origin, goalPos, path );
    if ( ai_showObstacleAvoidance.GetBool() )
    {
        gameRenderWorld->DebugLine( colorBlue, goalPos + idVec3( 1.0f, 1.0f, 0.0f ), goalPos + idVec3( 1.0f, 1.0f, 64.0f ), gameLocal.msec );
        gameRenderWorld->DebugLine( foundPath ? colorYellow : colorRed, path.seekPos, path.seekPos + idVec3( 0.0f, 0.0f, 64.0f ), gameLocal.msec );
    }

    if ( !foundPath )
    {
        // couldn't get around obstacles
        if ( path.firstObstacle )
        {
            AI_OBSTACLE_IN_PATH = true;
            if ( physicsObject->GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.firstObstacle->GetPhysics()->GetAbsBounds() ) )
            {
                obstacle = path.firstObstacle;
            }
        }
        else if ( path.startPosObstacle )
        {
            AI_OBSTACLE_IN_PATH = true;
            if ( physicsObject->GetAbsBounds().Expand( 2.0f ).IntersectsBounds( path.startPosObstacle->GetPhysics()->GetAbsBounds() ) )
            {
                obstacle = path.startPosObstacle;
            }
        }
        else
        {
            // Blocked by wall
            move.moveStatus = MOVE_STATUS_BLOCKED_BY_WALL;
        }
#if 0
    }
    else if ( path.startPosObstacle )
    {
        // check if we're past where the our origin was pushed out of the obstacle
        dir = goalPos - origin;
        dir.Normalize();
        dist = ( path.seekPos - origin ) * dir;
        if ( dist < 1.0f )
        {
            AI_OBSTACLE_IN_PATH = true;
            obstacle = path.startPosObstacle;
        }
#endif
    }
    else if ( path.seekPosObstacle )
    {
        // if the AI is very close to the path.seekPos already and path.seekPosObstacle != NULL
        // then we want to push the path.seekPosObstacle entity out of the way
        AI_OBSTACLE_IN_PATH = true;

        // check if we're past where the goalPos was pushed out of the obstacle
        dir = goalPos - origin;
        dir.Normalize();
        dist = ( path.seekPos - origin ) * dir;
        if ( dist < 1.0f )
        {
            obstacle = path.seekPosObstacle;
        }
    }

    // if we had an obstacle, set our move status based on the type, and kick it out of the way if it's a moveable
    if ( obstacle )
    {
        if ( obstacle->IsType( idActor::Type ) )
        {
            // monsters aren't kickable
            if ( obstacle == enemy.GetEntity() )
            {
                move.moveStatus = MOVE_STATUS_BLOCKED_BY_ENEMY;
            }
            else
            {
                move.moveStatus = MOVE_STATUS_BLOCKED_BY_MONSTER;
            }
        }
        else
        {
            // try kicking the object out of the way
            move.moveStatus = MOVE_STATUS_BLOCKED_BY_OBJECT;
        }
        newPos = obstacle->GetPhysics()->GetOrigin();
        //newPos = path.seekPos;
        move.obstacle = obstacle;
    }
    else
    {
        newPos = path.seekPos;
        move.obstacle = NULL;
    }
}

/*
=====================
botAi::GetMovePosition
=====================
*/
idVec3 botAi::GetMovePosition( void )
{
    idVec3				goalPos;
    idVec3				newDest;

    idVec3 oldorigin = physicsObject->GetOrigin();

    AI_BLOCKED = false;

    if ( move.moveCommand < NUM_NONMOVING_COMMANDS )
    {
        move.lastMoveOrigin.Zero();
        move.lastMoveTime = gameLocal.time;
    }

    move.obstacle = NULL;
    if ( GetMovePos( goalPos ) )
    {
        if ( move.moveCommand != MOVE_WANDER )
        {
            CheckObstacleAvoidance( goalPos, newDest );
            goalPos = newDest;
        }
    }

    if ( ai_debugMove.GetBool() )
    {
        gameRenderWorld->DebugLine( colorCyan, oldorigin, physicsObject->GetOrigin(), 5000 );
    }

    BlockedFailSafe();

    if ( ai_debugMove.GetBool() )
    {
        gameRenderWorld->DebugBounds( colorMagenta, physicsObject->GetBounds(), physicsObject->GetOrigin(), gameLocal.msec );
        gameRenderWorld->DebugBounds( colorMagenta, physicsObject->GetBounds(), move.moveDest, gameLocal.msec );
        gameRenderWorld->DebugLine( colorYellow, physicsObject->GetOrigin() + playerEnt->EyeOffset(), physicsObject->GetOrigin() + playerEnt->EyeOffset() + playerEnt->viewAxis[ 0 ] * physicsObject->GetGravityAxis() * 16.0f, gameLocal.msec, true );
        DrawRoute();
    }

    return goalPos;
}

/*
=====================
botAi::EnemyDead
=====================
*/
void botAi::EnemyDead( void )
{
    ClearEnemy();
    AI_ENEMY_DEAD = true;
}

/*
=====================
botAi::GetEnemy
=====================
*/
idActor	*botAi::GetEnemy( void ) const
{
    return enemy.GetEntity();
}

/*
=====================
botAi::ClearEnemy
=====================
*/
void botAi::ClearEnemy( void )
{
    if ( move.moveCommand == MOVE_TO_ENEMY )
    {
        StopMove( MOVE_STATUS_DEST_NOT_FOUND );
    }

    playerEnt->enemyNode.Remove();
    enemy				= NULL;
    AI_ENEMY_IN_FOV		= false;
    AI_ENEMY_VISIBLE	= false;
    AI_ENEMY_DEAD		= true;

    //SetChatSound();
}

/*
=====================
botAi::EnemyPositionValid
=====================
*/
bool botAi::EnemyPositionValid( void ) const
{
    trace_t	tr;
    idVec3	muzzle;
    idMat3	axis;

    if ( !enemy.GetEntity() )
    {
        return false;
    }

    if ( AI_ENEMY_VISIBLE )
    {
        return true;
    }

    gameLocal.clip.TracePoint( tr, playerEnt->GetEyePosition(), lastVisibleEnemyPos + lastVisibleEnemyEyeOffset, MASK_OPAQUE, playerEnt );
    if ( tr.fraction < 1.0f )
    {
        // can't see the area yet, so don't know if he's there or not
        return true;
    }

    return false;
}

/*
=====================
botAi::SetEnemyPosition
=====================
*/
void botAi::SetEnemyPosition( void )
{
    idActor		*enemyEnt = enemy.GetEntity();
    int			enemyAreaNum;
    int			areaNum;
    int			lastVisibleReachableEnemyAreaNum;
    aasPath_t	path;
    idVec3		pos;
    bool		onGround;

    if ( !enemyEnt )
    {
        return;
    }

    lastVisibleReachableEnemyPos = lastReachableEnemyPos;
    lastVisibleEnemyEyeOffset = enemyEnt->EyeOffset();
    lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();
    if ( move.moveType == MOVETYPE_FLY )
    {
        pos = lastVisibleEnemyPos;
        onGround = true;
    }
    else
    {
        onGround = enemyEnt->GetFloorPos( 64.0f, pos );
        if ( enemyEnt->OnLadder() )
        {
            onGround = false;
        }
    }

    if ( !onGround )
    {
        if ( move.moveCommand == MOVE_TO_ENEMY )
        {
            AI_DEST_UNREACHABLE = true;
        }
        return;
    }

    // when we don't have an AAS, we can't tell if an enemy is reachable or not,
    // so just assume that he is.
    if ( !aas )
    {
        lastVisibleReachableEnemyPos = lastVisibleEnemyPos;
        if ( move.moveCommand == MOVE_TO_ENEMY )
        {
            AI_DEST_UNREACHABLE = false;
        }
        enemyAreaNum = 0;
        areaNum = 0;
    }
    else
    {
        lastVisibleReachableEnemyAreaNum = move.toAreaNum;
        enemyAreaNum = PointReachableAreaNum( lastVisibleEnemyPos, 1.0f );
        if ( !enemyAreaNum )
        {
            enemyAreaNum = PointReachableAreaNum( lastReachableEnemyPos, 1.0f );
            pos = lastReachableEnemyPos;
        }
        if ( !enemyAreaNum )
        {
            if ( move.moveCommand == MOVE_TO_ENEMY )
            {
                AI_DEST_UNREACHABLE = true;
            }
            areaNum = 0;
        }
        else
        {
            const idVec3 &org = physicsObject->GetOrigin();
            areaNum = PointReachableAreaNum( org );
            if ( PathToGoal( path, areaNum, org, enemyAreaNum, pos ) )
            {
                lastVisibleReachableEnemyPos = pos;
                lastVisibleReachableEnemyAreaNum = enemyAreaNum;
                if ( move.moveCommand == MOVE_TO_ENEMY )
                {
                    AI_DEST_UNREACHABLE = false;
                }
            }
            else if ( move.moveCommand == MOVE_TO_ENEMY )
            {
                AI_DEST_UNREACHABLE = true;
            }
        }
    }

    if ( move.moveCommand == MOVE_TO_ENEMY )
    {
        if ( !aas )
        {
            // keep the move destination up to date for wandering
            move.moveDest = lastVisibleReachableEnemyPos;
        }
        else if ( enemyAreaNum )
        {
            move.toAreaNum = lastVisibleReachableEnemyAreaNum;
            move.moveDest = lastVisibleReachableEnemyPos;
        }
    }
}

/*
=====================
botAi::UpdateEnemyPosition
=====================
*/
void botAi::UpdateEnemyPosition( void )
{
    idActor *enemyEnt = enemy.GetEntity();
    int				enemyAreaNum;
    int				areaNum;
    aasPath_t		path;
    predictedPath_t predictedPath;
    idVec3			enemyPos;
    bool			onGround;

    if ( !enemyEnt )
    {
        return;
    }

    const idVec3 &org = physicsObject->GetOrigin();

    if ( move.moveType == MOVETYPE_FLY )
    {
        enemyPos = enemyEnt->GetPhysics()->GetOrigin();
        onGround = true;
    }
    else
    {
        onGround = enemyEnt->GetFloorPos( 64.0f, enemyPos );
        if ( enemyEnt->OnLadder() )
        {
            onGround = false;
        }
    }

    if ( onGround )
    {
        // when we don't have an AAS, we can't tell if an enemy is reachable or not,
        // so just assume that he is.
        if ( !aas )
        {
            enemyAreaNum = 0;
            lastReachableEnemyPos = enemyPos;
        }
        else
        {
            enemyAreaNum = PointReachableAreaNum( enemyPos, 1.0f );
            if ( enemyAreaNum )
            {
                areaNum = PointReachableAreaNum( org );
                if ( PathToGoal( path, areaNum, org, enemyAreaNum, enemyPos ) )
                {
                    lastReachableEnemyPos = enemyPos;
                }
            }
        }
    }

    AI_ENEMY_IN_FOV		= false;
    AI_ENEMY_VISIBLE	= false;

    if ( CanSee( enemyEnt, false ) )
    {
        AI_ENEMY_VISIBLE = true;
        if ( CheckFOV( enemyEnt->GetPhysics()->GetOrigin() ) )
        {
            AI_ENEMY_IN_FOV = true;
        }

        SetEnemyPosition();
    }
    else
    {
        // check if we heard any sounds in the last frame
        if ( enemyEnt == gameLocal.GetAlertEntity() )
        {
            float dist = ( enemyEnt->GetPhysics()->GetOrigin() - org ).LengthSqr();
            if ( dist < Square( AI_HEARING_RANGE ) )
            {
                SetEnemyPosition();
            }
        }
    }

    if ( ai_debugMove.GetBool() )
    {
        gameRenderWorld->DebugBounds( colorLtGrey, enemyEnt->GetPhysics()->GetBounds(), lastReachableEnemyPos, gameLocal.msec );
        gameRenderWorld->DebugBounds( colorWhite, enemyEnt->GetPhysics()->GetBounds(), lastVisibleReachableEnemyPos, gameLocal.msec );
    }
}

/*
=====================
botAi::SetEnemy
=====================
*/
void botAi::SetEnemy( idActor *newEnemy )
{
    int enemyAreaNum;

    if ( AI_DEAD )
    {
        ClearEnemy();
        return;
    }

    AI_ENEMY_DEAD = false;
    if ( !newEnemy )
    {
        ClearEnemy();
    }
    else if ( enemy.GetEntity() != newEnemy )
    {
        enemy = newEnemy;
        playerEnt->enemyNode.AddToEnd( newEnemy->enemyList );
        if ( newEnemy->health <= 0 )
        {
            EnemyDead();
            return;
        }
        // let the monster know where the enemy is
        newEnemy->GetAASLocation( aas, lastReachableEnemyPos, enemyAreaNum );
        SetEnemyPosition();
        //SetChatSound();

        lastReachableEnemyPos = lastVisibleEnemyPos;
        lastVisibleReachableEnemyPos = lastReachableEnemyPos;
        enemyAreaNum = PointReachableAreaNum( lastReachableEnemyPos, 1.0f );
        if ( aas && enemyAreaNum )
        {
            aas->PushPointIntoAreaNum( enemyAreaNum, lastReachableEnemyPos );
            lastVisibleReachableEnemyPos = lastReachableEnemyPos;
        }
    }
}

/***********************************************************************

	vision

***********************************************************************/

/*
=====================
botAi::SetFov
=====================
*/
void botAi::SetFOV( float fov )
{
    fovDot = (float)cos( DEG2RAD( fov * 0.5f ) );
}

/*
=====================
botAi::CheckFOV
=====================
*/
bool botAi::CheckFOV( const idVec3 &pos ) const
{
    if ( fovDot == 1.0f )
    {
        return true;
    }

    float	dot;
    idVec3	delta;
    idVec3	eyePos;
    idMat3	viewAxis;

    playerEnt->GetViewPos( eyePos, viewAxis );

    delta = pos - eyePos;

    // get our gravity normal
    const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

    // infinite vertical vision, so project it onto our orientation plane
    delta -= gravityDir * ( gravityDir * delta );

    delta.Normalize();
    dot = viewAxis[ 0 ] * delta;

    return ( dot >= fovDot );
}

/*
=====================
botAi::CanSee
TinMan: Same as idActor cansee at the moment, but may want to expand on it later.
=====================
*/
bool botAi::CanSee( idEntity *ent, bool useFov ) const
{
    trace_t		tr, tr2;
    idVec3		eye;
    idVec3		toPos;

    if ( ent->IsHidden() )
    {
        return false;
    }

    if ( ent->IsType( idActor::Type ) )
    {
        toPos = ( ( idActor * )ent )->GetEyePosition();
    }
    else
    {
        toPos = ent->GetPhysics()->GetOrigin();
    }

    if ( useFov && !CheckFOV( toPos ) )
    {
        return false;
    }

    eye = playerEnt->GetEyePosition();

    gameLocal.clip.TracePoint( tr, eye, toPos, CONTENTS_OPAQUE | CONTENTS_RENDERMODEL, this );
    // TinMan: *debug
    float t = 1;
    idBounds bounds = idBounds( tr.endpos ).Expand( 2 );

    if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) )
    {
        //gameRenderWorld->DebugLine( colorRed, eye, tr.endpos, t );
        //gameRenderWorld->DebugBounds( colorBlue, bounds, vec3_origin, t );
        return true;
    } /* else {
		gameRenderWorld->DebugLine( colorCyan, eye, toPos, t );
	} */

    return false;
}

#ifdef MOD_BOTS
/***********************************************************************

	Command System

***********************************************************************/

/*
=====================
botAi::ProcessCommand
TinMan: Process a chat string into bot commands
Note: chat lines come in several flavors
general chat: PlayerName: chattext
team chat: ( team ) PlayerName: chattext
ctf team chat: ( team ) PlayerName: [Location] chattext

TinMan: At the moment command system is simple and must follow strict rules:
notifyer command qualifier
notifier: must be "cmd" just an easy check to let function know this message is actually a command
command:
qualifier:
=====================
*/
void botAi::ProcessCommand( const char *text )
{
    idCmdArgs		args;
    idStr			message;
    idStr			commanderName;
    idStr			notifier;
    idStr			command;
    idStr			qualifier;

    idPlayer*		commander;

    goalType_t		type;
    idEntity *		entity;
    idVec3			position;

    trace_t			trace;

    type	= SABOT_GOAL_NONE;
    entity = NULL;
    position.Zero();

    message = text;
    message.RemoveColors();

    // TinMan: TODO: only have work for team chat?
    if ( message.Find( "( team ) " ) )   // TinMan: TODO: WTF why does it only work with the spaces?
    {
        message.StripLeading( "(team) " );
    }
    else
    {
        return;
    }

    //gameLocal.Printf( "ProcessCommand: [message][%s] \n", message.c_str() ); // TinMan: *debug*

    // TinMan: Grab who sent message as "commander"
    int pEnd = message.Find( ":" );
    //gameLocal.Printf( "ProcessCommand: [pend][%i] \n", pEnd ); // TinMan: *debug*
    message.Left( pEnd, commanderName );
    //gameLocal.Printf( "ProcessCommand: [commander][%s] \n", commanderName.c_str() ); // TinMan: *debug*

    message = message.Right( message.Length() - pEnd - 2 ); // TinMan: Trim everything before the "blahblah: "
    //gameLocal.Printf( "ProcessCommand: [message][%s] \n", message.c_str() ); // TinMan: *debug*

    args.TokenizeString( message, false ); // TinMan: Split the rest into args

    /*
    // TinMan: *debug* spit out args
    for( int icmd = 0; icmd < args.Argc(); ) {
    	//const char *cmd = args.Argv( icmd++ );
    	idStr cmd = args.Argv( icmd++ );

    	gameLocal.Printf( "ProcessCommand: [arg][%s] \n", cmd.c_str() );
    }
    */
    //gameLocal.Printf( "ProcessCommand: [argc:%i] \n", args.Argc() );

    // TinMan: notifyer command qualifier. Bug out if message too short.
    if ( args.Argc() < 3 )
    {
        //gameLocal.Printf( "ProcessCommand: message too short \n" ); // TinMan: *debug*
        return;
    }

    // TinMan: Check if this is actually a bot command (ie starts with "cmd") TODO: or cmdnext for ques
    notifier = args.Argv( 0 );
    if ( idStr::Icmp( notifier, "cmd" ) != 0 )
    {
        gameLocal.Printf( "ProcessCommand: unrecognised notifier \n" ); // TinMan: *debug*
        return;
    }

    command = args.Argv( 1 );
    command.ToLower();
    qualifier = args.Argv( 2 );
    qualifier.ToLower();

    commander = gameLocal.GetClientByName( commanderName );
    if ( !commander )
    {
        gameLocal.Printf( "ProcessCommand: Unknown commander: %s\n", commanderName.c_str() ); // TinMan: *debug*
        return;
    }
    else
    {
        trace = GetPlayerTrace( commander );
    }

    // TinMan: Process qualifier
    if ( qualifier == "here" )   // TinMan: Here means at commanders position
    {
        position = commander->GetPhysics()->GetOrigin();
    }
    else if ( qualifier == "there" )     // TinMan: There means where commander is looking
    {
        position = trace.endpos;
    }
    else if ( qualifier == "that" )     // TinMan: There means what commander is looking at
    {
        idEntity *ent = gameLocal.GetTraceEntity( trace );
        if ( ent )
        {
            entity = ent;
            gameLocal.Printf( "ProcessCommand: that = %s\n", entity->name.c_str() );
        }
    }
    else if ( qualifier == "me" )
    {
        entity = commander;
    }
    else if ( qualifier == "bot" )     // TinMan: get bot by botid, next arg should be botid
    {
        if ( args.Argc() < 4 )
        {
            //gameLocal.Printf( "ProcessCommand: message too short \n" ); // TinMan: *debug*
            return;
        }
        const char * botNum = args.Argv( 3 );
        int botID = atoi( botNum );
        if ( botID > BOT_MAX_BOTS )
        {
            return;
        }
        gameLocal.Printf( "ProcessCommand: botID: %i\n", botID );
        if ( !bots[ botID ].inUse )
        {
            gameLocal.DPrintf( "ProcessCommand: !botID: %i\n", botID );
            return;
        }
        else
        {
            entity = gameLocal.entities[ bots[ botID ].clientID ];
            assert( entity );
        }
        //} else if ( qualifier == "player" ) {	// TinMan: TODO: try and see if qualifier is a playername ( or part of one )
    }
    else
    {
        gameLocal.DPrintf( "ProcessCommand: unknown qualifier type %s\n", qualifier.c_str() );
        return;
    }

    // TinMan: Process command
    if ( command == "order" )
    {
        // TinMan: Find bot from ent
        if ( entity )
        {
            if ( entity->IsType( idPlayer::Type ) )
            {
                idPlayer * player = static_cast<idPlayer *>( entity );
                if ( player && player->spawnArgs.GetBool( "isBot" ) )
                {
                    int botNum = player->spawnArgs.GetBool( "botID" );
                    botAi * bot = static_cast<botAi *>( gameLocal.entities[ botAi::bots[ botNum ].entityNum ] );
                    bots[ botNum ].selected = true;
                    gameLocal.Printf( "ProcessCommand: selected bot %i\n", botNum );
                }
            }
        } /* else { // TinMan: select/deselect - *todo* not currently used as bots auto desel after cmd
			for ( int i = 0; i < BOT_MAX_BOTS; i++ ) {
				if ( qualifier == "clear" ) {
					bots[ i ].selected = false;
				} else if ( qualifier == "all" ) {
					bots[ i ].selected = true;
				}
			}
		} */

        return;	// TinMan: Done
    }
    else if ( command == "move" )
    {
        type = SABOT_GOAL_MOVE;
    }
    else if ( command == "hold" )
    {
        type = SABOT_GOAL_HOLD;
    }
    else if ( command == "follow" )
    {
        type = SABOT_GOAL_FOLLOW;
    }
    else if ( command == "attack" )
    {
        type = SABOT_GOAL_ATTACK;
    }
    else
    {
        gameLocal.DPrintf( "ProcessCommand: unknown command type %s\n", command.c_str() );
        return;
    }

    // TinMan: Send commands to all selected bots
    for ( int i = 0; i < BOT_MAX_BOTS; i++ )
    {
        if ( bots[ i ].selected )
        {
            botAi * commandBot = static_cast<botAi *>( gameLocal.entities[ botAi::bots[ i ].entityNum ] );
            if ( commandBot )
            {
                int cmdtype = type;
                commandBot->PostCommand( cmdtype, entity, position );
            }
        }
    }
}

/*
================
botAi::PostCommand
TinMan: Send processed command to selected bots
================
*/
void botAi::PostCommand( int type, idEntity * entity, idVec3 position )
{
    BOT_COMMAND = true;
    commandType = type;
    commandEntity = entity;
    commandPosition = position;
    //gameLocal.Printf( "[botAi::PostCommand][bot: %i][commandtype: %i][commandEntity: %s]\n", botID, type, entity->GetName() );
}


/*
================
botAi::GetPlayerTrace
TinMan:
================
*/
trace_t botAi::GetPlayerTrace( idPlayer * player )
{
    trace_t		tr;
    idVec3		eye;

    idVec3 dir;
    idVec3 pos;
    idMat3 axis;

    int maxRange = 4096; // TinMan:  Max a bounds trace can do is 4096.

    dir = player->viewAngles.ToForward();
    player->GetViewPos( eye, axis );

    gameLocal.clip.TracePoint( tr, eye, eye + ( dir * maxRange ), CONTENTS_OPAQUE | CONTENTS_RENDERMODEL, player );

    //idEntity * ent = gameLocal.GetTraceEntity( tr );

    /*
    if ( ent ) {
    	if ( ent->IsType( idPlayer::Type ) ) {

    	}
    }
    */

    return tr;
}

#endif

/***********************************************************************

	Events

***********************************************************************/

/*
=====================
botAi::Event_SetNextState
=====================
*/
void botAi::Event_SetNextState( const char *name )
{
    idealState = GetScriptFunction( name );
    if ( idealState == state )
    {
        state = NULL;
    }
}

/*
=====================
botAi::Event_SetState
=====================
*/
void botAi::Event_SetState( const char *name )
{
    idealState = GetScriptFunction( name );
    if ( idealState == state )
    {
        state = NULL;
    }
    scriptThread->DoneProcessing();
}

/*
=====================
botAi::Event_GetState
=====================
*/
void botAi::Event_GetState( void )
{
    if ( state )
    {
        idThread::ReturnString( state->Name() );
    }
    else
    {
        idThread::ReturnString( "" );
    }
}

/*
=====================
botAi::Event_GetBody
=====================
*/
void botAi::Event_GetBody( void )
{
    idThread::ReturnEntity( playerEnt );
}

/*
=====================
botAi::Event_GetGameType
=====================
*/
void botAi::Event_GetGameType( void )
{
    idThread::ReturnFloat( gameLocal.gameType );
}

/*
=====================
botAi::Event_GetHealth
=====================
*/
void botAi::Event_GetHealth( idEntity *ent )
{
    if ( !ent->IsType( idActor::Type ) )
    {
        gameLocal.Warning( "'%s' is not an idActor (player or ai controlled character)", ent->name.c_str() );
        idThread::ReturnFloat( -1 );
        return;
    }
    idActor * actor = static_cast<idActor *>( ent );
    idThread::ReturnFloat( actor->health );
}

/*
=====================
botAi::Event_GetArmor
=====================
*/
void botAi::Event_GetArmor( idEntity *ent )
{
    if ( !ent->IsType( idPlayer::Type ) )
    {
        gameLocal.Warning( "'%s' is not an idPlayer", ent->name.c_str() );
        idThread::ReturnFloat( -1 );
        return;
    }
    idPlayer * player = static_cast<idPlayer *>( ent );
    idThread::ReturnFloat( player->inventory.armor );
}

/*
=====================
botAi::Event_GetTeam
TinMan: *todo* could be moved to idActor
=====================
*/
void botAi::Event_GetTeam( idEntity *ent )
{
    if ( !ent->IsType( idActor::Type ) )
    {
        gameLocal.Warning( "[Event_GetTeam]['%s' is not an idActor (player or ai controlled character)]", ent->name.c_str() );
        idThread::ReturnFloat( -1 );
        return;
    }

    idActor * actor = static_cast<idActor *>( ent );
    idThread::ReturnFloat( actor->team );
}

/*
===============
botAi::Event_HasWeapon
===============
*/
void botAi::Event_HasWeapon( const char *name )
{
    const char *weap;
    int w = MAX_WEAPONS;

    /*
    // TinMan: better?
    	if ( num < MAX_WEAPONS ) {
    	if ( ( inventory->weapons & ( 1 << num ) ) != 0 ) {
    		idThread::ReturnFloat( true );
    		return;

    	}
    }
    */

    while ( w > 0 )
    {
        w--;
        weap = playerEnt->spawnArgs.GetString( va( "def_weapon%d", w ) );
        if ( !weap[ 0 ] || ( ( inventory->weapons & ( 1 << w ) ) == 0 ) )
        {
            continue;
        }

        if ( idStr::Icmp( name, weap ) == 0 )
        {
            //gameLocal.Printf("[botAi::Event_HasWeapon][has: %s]\n", weap ); // TinMan: *debug*
            idThread::ReturnFloat( true );
            return;
        }
    }

    idThread::ReturnFloat( false );
}

/*
===============
botAi::Event_HasAmmoForWeapon
TinMan: Ripped arpart from d3xp hasammo, takes ammo in weapon into account
===============
*/
void botAi::Event_HasAmmoForWeapon( const char *name )
{
    int ammoRequired;
    ammo_t ammo_i = inventory->AmmoIndexForWeaponClass( name, &ammoRequired );

    int ammoCount = inventory->HasAmmo( ammo_i, ammoRequired );
    ammoCount += inventory->clip[playerEnt->SlotForWeapon(name)];
    idThread::ReturnFloat( ammoCount );

    //int ammo = inventory->HasAmmo( name );
    //idThread::ReturnFloat( ammo );
}

/*
===============
botAi::Event_HasAmmo
===============
*/
void botAi::Event_HasAmmo( const char *name )
{
    //int ammoRequired;
    ammo_t ammo_i = inventory->AmmoIndexForAmmoClass( name );

    int ammoCount = inventory->HasAmmo( ammo_i, 1 );
    //ammoCount += inventory->clip[playerEnt->SlotForWeapon(name)];
    idThread::ReturnFloat( ammoCount );
}

/*
==================
botAi::Event_NextBestWeapon
==================
*/
void botAi::Event_NextBestWeapon( void )
{
    playerEnt->NextBestWeapon();
}

/*
================
botAi::Event_SetAimPosition
TinMan: Sets where bot should adjust view angles to
================
*/
void botAi::Event_SetAimPosition( const idVec3 &aimPos )
{
    aimPosition = aimPos;
}

/*
================
botAi::Event_GetAimPosition
TinMan: Get where bot has been told to aim
================
*/
void botAi::Event_GetAimPosition( void )
{
    idThread::ReturnVector( aimPosition );
}

/*
================
botAi::Event_SetAimDirection
TinMan: Sets where bot should adjust view angles to
================
*/
void botAi::Event_SetAimDirection( const idVec3 &dir )
{
    viewDir = dir;
}

/*
================
botAi::Event_GetMovePosition
TinMan: Current point to move to on set path
================
*/
void botAi::Event_GetMovePosition( void )
{
    idVec3 movePos = GetMovePosition();
    idThread::ReturnVector( movePos );
}

/*
================
botAi::Event_GetSecondaryMovePosition
TinMan: Current point to move to on set path
================
*/
void botAi::Event_GetSecondaryMovePosition( void )
{
    idThread::ReturnVector( move.secondaryMovePosition );
}

/*
=====================
botAi::Event_GetPathType
=====================
*/
void botAi::Event_GetPathType( void )
{
    idThread::ReturnInt( move.pathType );
}

/*
================
botAi::Event_SetMoveDirection
TinMan: Sets direction and speed for bot to move in
================
*/
void botAi::Event_SetMoveDirection( const idVec3 &dir, float speed )
{
    moveDir = dir;
    moveSpeed = speed;
}

/*
=====================
botAi::Event_CanSeeEntity
=====================
*/
void botAi::Event_CanSeeEntity( idEntity *ent, bool useFov )
{
    if ( !ent )
    {
        idThread::ReturnInt( false );
        return;
    }

    bool cansee = CanSee( ent, useFov );
    idThread::ReturnInt( cansee );
}

/*
=====================
botAi::Event_CanSeePosition
=====================
*/
void botAi::Event_CanSeePosition( const idVec3 &pos, bool useFov )
{
    if ( useFov && !CheckFOV( pos ) )
    {
        idThread::ReturnInt( false );
        return;
    }

    bool cansee = EntityCanSeePos( playerEnt, physicsObject->GetOrigin(), pos );
    idThread::ReturnInt( cansee );
}

/*
================
botAi::Event_GetEyePosition
================
*/
void botAi::Event_GetEyePosition( void )
{
    idThread::ReturnVector( playerEnt->GetEyePosition() );
}

/*
================
botAi::Event_GetViewPosition
================
*/
void botAi::Event_GetViewPosition( void )
{
    idVec3 pos;
    idMat3 axis;
    playerEnt->GetViewPos( pos, axis );
    idThread::ReturnVector( pos );
}

/*
================
botAi::Event_GetAIAimTargets
================
*/
void botAi::Event_GetAIAimTargets( idEntity *aimAtEnt, float location )
{
    idVec3	headPosition;
    idVec3	chestPosition;

    if ( aimAtEnt == enemy.GetEntity() )
    {
        static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( lastVisibleEnemyPos, headPosition, chestPosition );
    }
    else if ( aimAtEnt->IsType( idActor::Type ) )
    {
        static_cast<idActor *>( aimAtEnt )->GetAIAimTargets( aimAtEnt->GetPhysics()->GetOrigin(), headPosition, chestPosition );
    }
    else
    {
        headPosition = aimAtEnt->GetPhysics()->GetAbsBounds().GetCenter();
        chestPosition = headPosition;
    }

    if ( location == 1 )
    {
        idThread::ReturnVector( headPosition );
    }
    else
    {
        idThread::ReturnVector( chestPosition );
    }
}

/*
=====================
botAi::Event_FindEnemies
TinMan: Finds all visable enemies (AI and Player) and adds them to entitySearchList[].
=====================
*/
void botAi::Event_FindEnemies( int useFOV )
{
    int			i;
    idEntity	*ent;
    idActor		*actor;
    pvsHandle_t pvs;

    // TinMan: Clear existing list
    memset( entitySearchList, 0, sizeof( entitySearchList ) );
    numSearchListEntities = 0; // TinMan: Reset

    pvs = gameLocal.pvs.SetupCurrentPVS( playerEnt->GetPVSAreas(), playerEnt->GetNumPVSAreas() );

    // TinMan: Find players
    for ( i = 0; i < gameLocal.numClients ; i++ )
    {
        ent = gameLocal.entities[ i ];

        if ( !ent || !ent->IsType( idActor::Type ) )
        {
            continue;
        }

        // TinMan: Don't want to return self
        if ( ent == playerEnt )
        {
            continue;
        }

        actor = static_cast<idActor *>( ent );
        if ( ( actor->health <= 0 ) /* || !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) */ )
        {
            continue;
        }

        // TinMan: Friendly fire check, play nice!
        if ( gameLocal.mpGame.IsGametypeTeamBased() || gameLocal.gameType == GAME_SP )   // TinMan: Nice function added by threewave, was thinking about writing one myself when custom pointed it out. Except it says sp isn't team based, try and tell the monsters that, the'll be laughing at you.
        {
            if ( actor->team == playerEnt->team )
            {
                continue;
            }
        }

        if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) )
        {
            continue;
        }

        if ( CanSee( actor, useFOV != 0 ) )
        {
            //gameLocal.Printf("[idAI::Event_FindEnemies][Found a Player]\n" ); // TinMan: *debug*
            // TinMan: Add to new list
            entitySearchList[numSearchListEntities] = ent;
            numSearchListEntities++;
        }
    }

    // TinMan: Find AI
    for ( ent = gameLocal.activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() )
    {
        if ( ent->fl.hidden || ent->fl.isDormant || !ent->IsType( idActor::Type ) )
        {
            continue;
        }

        // TinMan: Players caught above^
        if ( ent->IsType( idPlayer::Type ) )
        {
            continue;
        }

        // TinMan: Don't want to return self
        if ( ent == playerEnt )
        {
            continue;
        }

        actor = static_cast<idActor *>( ent );
        if ( ( actor->health <= 0 ) /*|| !( ReactionTo( actor ) & ATTACK_ON_SIGHT ) */ )
        {
            continue;
        }

        // TinMan: Friendly fire check, play nice!
        if ( gameLocal.mpGame.IsGametypeTeamBased() || gameLocal.gameType == GAME_SP )   // TinMan: Nice function added by threewave, was thinking about writing one myself when custom pointed it out.
        {
            if ( actor->team == playerEnt->team )
            {
                continue;
            }
        }

        if ( !gameLocal.pvs.InCurrentPVS( pvs, actor->GetPVSAreas(), actor->GetNumPVSAreas() ) )
        {
            continue;
        }

        if ( CanSee( actor, useFOV != 0 ) )
        {
            //gameLocal.Printf("[idAI::Event_FindEnemies][Found an AI]\n" ); // TinMan: *debug*
            // TinMan: Add to new list
            entitySearchList[numSearchListEntities] = ent;
            numSearchListEntities++;
        }
    }

    gameLocal.pvs.FreeCurrentPVS( pvs );

    idThread::ReturnFloat( numSearchListEntities );
}

/*
============
botAi::Event_FindInRadius
TinMan: Finds all visable entities of specified type in radius and adds them to entitySearchList[].
TinMan: Interestingly gamelocal has a radius function but it seems incomplete, just doing the bounds cull.
============
*/
void botAi::Event_FindInRadius( const idVec3 &origin, float radius, const char *classname )
{
    float		dist;
    idEntity *	ent;
    idEntity *	entityList[ MAX_GENTITIES ];
    int			numListedEntities;
    idBounds	bounds;
    idBounds	playerBounds;
    idVec3 		v, dir;
    int			i, e;
    idVec3		entityOrigin, pushOrigin;

    // TinMan: Clear existing list
    memset( entitySearchList, 0, sizeof( entitySearchList ) );
    numSearchListEntities = 0; // TinMan: Reset

    if ( radius < 1 )
    {
        radius = 1;
    }

    bounds = idBounds( origin ).Expand( radius );
    //bounds = idBounds( origin - idVec3( radius, radius, 64 ), origin + idVec3( radius, radius, 128 ) ); // TinMan: *cheaphack* *todo* *rem* shifted into script
    //gameRenderWorld->DebugBounds( colorBlue, bounds, vec3_origin, 100 );


    // id: get all entities touching the bounds
    numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

    for ( e = 0; e < numListedEntities; e++ )
    {
        ent = entityList[ e ];
        assert( ent );

        // TinMan: If it's not of specidied classname skip it
        if ( idStr::Icmp( ent->GetClassname(), classname ) != 0 )
        {
            continue;
        }

        // TinMan: Pathtogoal and other routing pushes entity origin into aas, which is ok if bot doesn't need to touch entity or if bot can touch the ent from the point it's told to go, but if it isn't in an area to start with it will likely push into another area where is isn't actually possible to reach the entity from.
        // TinMan: Solution, as I don't want to touch routing functions I'll add filter here, basically check if item is lower than jump height
        // *todo* still isn't perfect due to only doing bounds test rather than clip model test
        // Note: There are of course cases where an item may be out of aas, but close enough for it to be trivial to navigate to, but thats a bigger kettle of fish.
        // *todo* hmm this would only be for entities you want to touch (mmm sexay).
        if ( aas && !ent->IsType( idPlat::Type )  /*&& ( ent->IsType( idItem::Type ) || ent->IsType( idItemPowerup::Type ) || ent->IsType( idMoveableItem::Type )) */ )
        {
            entityOrigin = ent->GetPhysics()->GetOrigin();
            pushOrigin = entityOrigin;
            int num = PointReachableAreaNum( entityOrigin ); // TinMan: *todo* need to be set accurately?
            aas->PushPointIntoAreaNum( num, pushOrigin );

            if ( pushOrigin != entityOrigin )
            {
                //gameLocal.Printf( "[Event_FindInRadius][pushOrigin != entityOrigin][%s]\n", ent->name.c_str() );

                //idBounds testBounds = idBounds( pushOrigin ).Expand( 2 );
                //gameRenderWorld->DebugBounds( colorRed, testBounds, vec3_origin, 10000 );

                // TinMan: pushPont could be at any height, we want a position on the floor below to test from
                trace_t result;
                float max_dist = playerEnt->spawnArgs.GetFloat( "pm_jumpheight" );
                gameLocal.clip.Translation( result, pushOrigin, pushOrigin + ent->GetPhysics()->GetGravityNormal() * max_dist,	ent->GetPhysics()->GetClipModel(), ent->GetPhysics()->GetAxis(), MASK_SOLID, ent );
                //gameRenderWorld->DebugLine( colorCyan, pushOrigin, pushOrigin + ent->GetPhysics()->GetGravityNormal() * max_dist, 10000 );
                //gameRenderWorld->DebugLine( colorRed, pushOrigin, result.endpos, 10000 );
                if ( result.fraction < 1.0f )
                {
                    // TinMan: Get where trace hit
                    pushOrigin = result.endpos;
                }
                else
                {
                    // TinMan: Trace longer than jump height so not reall gettable
                    //gameLocal.Printf( "[Event_FindItems][Item higher than jumpheight: %s]\n", ent->name.c_str() );
                    //gameLocal.Printf( "[Event_FindItems][Item not reachable (too high): %s]\n", ent->name.c_str() );
                    continue;
                }


                //entBounds = ent->GetPhysics()->GetAbsBounds();
                // TinMan: Set up bounds to test from
                playerBounds = playerEnt->GetPhysics()->GetBounds();
                playerBounds += pushOrigin;

                // TinMan: Does the playerbounds under the pushedpoint touch the item?
                if ( !playerBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
                {
                    // TinMan: Can't get the item via aas.
                    //gameLocal.Printf( "[Event_FindItems][Item not reachable: %s]\n", ent->name.c_str() );
                    //gameRenderWorld->DebugBounds( colorGreen, playerBounds, vec3_origin, 10000 );
                    //gameRenderWorld->DebugBounds( colorMagenta, ent->GetPhysics()->GetAbsBounds(), vec3_origin, 10000 );
                    continue;
                }

                /* *todo* *rem* cheaper check
                if ( !entBounds.ContainsPoint( pushOrigin ) ) {
                	gameLocal.Printf( "[Event_FindItems][Item not reachable: %s]\n", ent->name.c_str() );
                	gameRenderWorld->DebugBounds( colorMagenta, entBounds, vec3_origin, 10000 );
                	continue;
                }
                */
            }
        }


        // id: find the distance from the edge of the bounding box
        for ( i = 0; i < 3; i++ )
        {
            if ( origin[ i ] < ent->GetPhysics()->GetAbsBounds()[0][ i ] )
            {
                v[ i ] = ent->GetPhysics()->GetAbsBounds()[0][ i ] - origin[ i ];
            }
            else if ( origin[ i ] > ent->GetPhysics()->GetAbsBounds()[1][ i ] )
            {
                v[ i ] = origin[ i ] - ent->GetPhysics()->GetAbsBounds()[1][ i ];
            }
            else
            {
                v[ i ] = 0;
            }
        }

        // TinMan: If it's not in radius skip it
        dist = v.Length();
        if ( dist >= radius )
        {
            continue;
        }

        // TinMan: Add to new list
        entitySearchList[numSearchListEntities] = ent;
        numSearchListEntities++;

    }

    // DebugCircle( const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false ) = 0;
    //gameRenderWorld->DebugCircle( colorBlue, origin, vec3_origin, radius, 10, 100000 ); // TinMan: *debug*

    //void			DebugBounds( const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const int lifetime = 0 ) = 0;
    //gameRenderWorld->DebugBounds( colorBlue, bounds, vec3_origin, 1000 );

    idThread::ReturnFloat( numSearchListEntities );
}

/*
============
botAi::Event_FindItems
TinMan: Finds all items in level and adds them to entitySearchList[]. *todo* this is more of a listitems
============
*/
void botAi::Event_FindItems( void )
{
    idEntity *	ent;
    int			i;
    idVec3		entityOrigin, pushOrigin;
    idBounds	playerBounds;

    // TinMan: Clear existing list
    memset( entitySearchList, 0, sizeof( entitySearchList ) );
    numSearchListEntities = 0; // TinMan: Reset

    for ( i = 0; i < MAX_GENTITIES; i++ )
    {
        ent = gameLocal.entities[ i ];
        if ( ent )
        {
            if ( ent->IsType( idItem::Type ) || ent->IsType( idItemPowerup::Type ) )
            {
                // TinMan: Pathtogoal and other routing pushes entity origin into aas, which is ok if bot doesn't need to touch entity or if bot can touch the ent from the point it's told to go, but if it isn't in an area to start with it will likely push into another area where is isn't actually possible to reach the entity from.
                // TinMan: Solution, as I don't want to touch routing functions I'll add filter here, basically check if item is lower than jump height
                // *todo* still isn't perfect due to only doing bounds test rather than clip model test
                // Note: There are of course cases where an item may be out of aas, but close enough for it to be trivial to navigate to, but thats a bigger kettle of fish.
                if ( aas )
                {
                    entityOrigin = ent->GetPhysics()->GetOrigin();
                    pushOrigin = entityOrigin;
                    int num = PointReachableAreaNum( entityOrigin ); // TinMan: *todo* need to be set accurately?
                    aas->PushPointIntoAreaNum( num, pushOrigin );

                    if ( pushOrigin != entityOrigin )
                    {
                        //gameLocal.Printf( "[Event_FindItems][pushOrigin != entityOrigin][%s]\n", ent->name.c_str() );

                        //idBounds testBounds = idBounds( pushOrigin ).Expand( 2 );
                        //gameRenderWorld->DebugBounds( colorRed, testBounds, vec3_origin, 10000 );

                        // TinMan: pushPont could be at any height, we want a position on the floor below to test from
                        trace_t result;
                        float max_dist = playerEnt->spawnArgs.GetFloat( "pm_jumpheight" );
                        gameLocal.clip.Translation( result, pushOrigin, pushOrigin + ent->GetPhysics()->GetGravityNormal() * max_dist,	ent->GetPhysics()->GetClipModel(), ent->GetPhysics()->GetAxis(), MASK_SOLID, ent );
                        //gameRenderWorld->DebugLine( colorCyan, pushOrigin, pushOrigin + ent->GetPhysics()->GetGravityNormal() * max_dist, 10000 );
                        //gameRenderWorld->DebugLine( colorRed, pushOrigin, result.endpos, 10000 );
                        if ( result.fraction < 1.0f )
                        {
                            // TinMan: Get where trace hit
                            pushOrigin = result.endpos;
                        }
                        else
                        {
                            // TinMan: Trace longer than jump height so not reall gettable
                            //gameLocal.Printf( "[Event_FindItems][Item higher than jumpheight: %s]\n", ent->name.c_str() );
                            //gameLocal.Printf( "[Event_FindItems][Item not reachable (too high): %s]\n", ent->name.c_str() );
                            continue;
                        }


                        //entBounds = ent->GetPhysics()->GetAbsBounds();
                        // TinMan: Set up bounds to test from
                        playerBounds = playerEnt->GetPhysics()->GetBounds();
                        playerBounds += pushOrigin;

                        // TinMan: Does the playerbounds under the pushedpoint touch the item?
                        if ( !playerBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
                        {
                            // TinMan: Can't get the item via aas.
                            //gameLocal.Printf( "[Event_FindItems][Item not reachable: %s]\n", ent->name.c_str() );
                            //gameRenderWorld->DebugBounds( colorGreen, playerBounds, vec3_origin, 10000 );
                            //gameRenderWorld->DebugBounds( colorMagenta, ent->GetPhysics()->GetAbsBounds(), vec3_origin, 10000 );
                            continue;
                        }

                        /* *todo* *rem* cheaper check
                        if ( !entBounds.ContainsPoint( pushOrigin ) ) {
                        	gameLocal.Printf( "[Event_FindItems][Item not reachable: %s]\n", ent->name.c_str() );
                        	gameRenderWorld->DebugBounds( colorMagenta, entBounds, vec3_origin, 10000 );
                        	continue;
                        }
                        */

                    }
                }

                entitySearchList[numSearchListEntities] = ent;
                numSearchListEntities++;
            }
        }
    }

    idThread::ReturnFloat( numSearchListEntities );
}

/*
============
botAi::Event_GetEntityList
TinMan: Return entity from array built during one of the find functions
TinMan: *todo* Oh dear, nasty, check to see if not empty before barging in
============
*/
void botAi::Event_GetEntityList( float index )
{
    int i;
    i = index;
    idEntity *	ent;
    if ( i > numSearchListEntities )
    {
        gameLocal.Error( "[botAi::Event_GetEntityList][Invalid index]\n" );
    }
    ent = entitySearchList[ i ];

    if ( ent )
    {
        idThread::ReturnEntity( ent );
        return;
    }

    idThread::ReturnEntity( NULL );
}

/*
=====================
botAi::Event_HeardSound
TinMan: This function adapted from idAI is completely pointless since it was designed for singleplayer (just that, one player) so that means out of all player entities cuasing a ruckus only one will get through.
=====================
*/
void botAi::Event_HeardSound( int ignore_team )
{
    // check if we heard any sounds in the last frame
    idActor	*actor = gameLocal.GetAlertEntity();
    if ( actor )   //( !ignore_team || ( ReactionTo( actor ) & ATTACK_ON_SIGHT ) ) && gameLocal.InPlayerPVS( playerEnt ) ) { // TinMan: Don't use inplayerpvs check obviously
    {
        // TinMan: Don't want to return self, oh ho ho, this bug was giving me trouble
        if ( actor == playerEnt )
        {
            idThread::ReturnEntity( NULL );
            return;
        }

        // TinMan: Friendly fire check, play nice!
        if ( gameLocal.mpGame.IsGametypeTeamBased() || gameLocal.gameType == GAME_SP )   // TinMan: Nice function added by threewave, was thinking about writing one myself when custom pointed it out.
        {
            if ( actor->team == playerEnt->team && !ignore_team )
            {
                idThread::ReturnEntity( NULL );
                return;
            }
        }

        idVec3 pos = actor->GetPhysics()->GetOrigin();
        idVec3 org = physicsObject->GetOrigin();
        float dist = ( pos - org ).LengthSqr();
        if ( dist < Square( AI_HEARING_RANGE ) )
        {
            idThread::ReturnEntity( actor );
            return;
        }
    }

    idThread::ReturnEntity( NULL );
}

/*
=====================
botAi::Event_SetEnemy
=====================
*/
void botAi::Event_SetEnemy( idEntity *ent )
{
    if ( !ent )
    {
        ClearEnemy();
    }
    else if ( !ent->IsType( idActor::Type ) )
    {
        gameLocal.Error( "'%s' is not an idActor (player or ai controlled character)", ent->name.c_str() );
    }
    else
    {
        SetEnemy( static_cast<idActor *>( ent ) );
    }
}

/*
=====================
botAi::Event_ClearEnemy
=====================
*/
void botAi::Event_ClearEnemy( void )
{
    ClearEnemy();
}

/*
=====================
botAi::Event_GetEnemy
=====================
*/
void botAi::Event_GetEnemy( void )
{
    idThread::ReturnEntity( enemy.GetEntity() );
}

/*
================
botAi::Event_LocateEnemy
================
*/
void botAi::Event_LocateEnemy( void )
{
    idActor *enemyEnt;
    int areaNum;

    enemyEnt = enemy.GetEntity();
    if ( !enemyEnt )
    {
        return;
    }

    enemyEnt->GetAASLocation( aas, lastReachableEnemyPos, areaNum );
    SetEnemyPosition();
    UpdateEnemyPosition();
}

/*
=====================
botAi::Event_EnemyRange
=====================
*/
void botAi::Event_EnemyRange( void )
{
    float dist;
    idActor *enemyEnt = enemy.GetEntity();

    if ( enemyEnt )
    {
        dist = ( enemyEnt->GetPhysics()->GetOrigin() - physicsObject->GetOrigin() ).Length();
    }
    else
    {
        // Just some really high number
        dist = idMath::INFINITY;
    }

    idThread::ReturnFloat( dist );
}

/*
=====================
botAi::Event_EnemyRange2D
=====================
*/
void botAi::Event_EnemyRange2D( void )
{
    float dist;
    idActor *enemyEnt = enemy.GetEntity();

    if ( enemyEnt )
    {
        dist = ( enemyEnt->GetPhysics()->GetOrigin().ToVec2() - physicsObject->GetOrigin().ToVec2() ).Length();
    }
    else
    {
        // Just some really high number
        dist = idMath::INFINITY;
    }

    idThread::ReturnFloat( dist );
}

/*
=====================
botAi::Event_GetEnemyPos
=====================
*/
void botAi::Event_GetEnemyPos( void )
{
    idThread::ReturnVector( lastVisibleEnemyPos );
}

/*
=====================
botAi::Event_GetEnemyEyePos
=====================
*/
void botAi::Event_GetEnemyEyePos( void )
{
    idThread::ReturnVector( lastVisibleEnemyPos + lastVisibleEnemyEyeOffset );
}

/*
=====================
botAi::Event_PredictEnemyPos
=====================
*/
void botAi::Event_PredictEnemyPos( float time )
{
    predictedPath_t path;
    idActor *enemyEnt = enemy.GetEntity();

    // if no enemy set
    if ( !enemyEnt )
    {
        idThread::ReturnVector( physicsObject->GetOrigin() );
        return;
    }

    // predict the enemy movement
    idAI::PredictPath( enemyEnt, aas, lastVisibleEnemyPos, enemyEnt->GetPhysics()->GetLinearVelocity(), SEC2MS( time ), SEC2MS( time ), ( move.moveType == MOVETYPE_FLY ) ? SE_BLOCKED : ( SE_BLOCKED | SE_ENTER_LEDGE_AREA ), path );

    idThread::ReturnVector( path.endPos );
}

/*
=====================
botAi::Event_CanHitEnemy
=====================
*/
void botAi::Event_CanHitEnemy( void )
{
    trace_t	tr;
    idEntity *hit;

    idActor *enemyEnt = enemy.GetEntity();
    if ( !AI_ENEMY_VISIBLE || !enemyEnt )
    {
        idThread::ReturnInt( false );
        return;
    }

    // don't check twice per frame
    if ( gameLocal.time == lastHitCheckTime )
    {
        idThread::ReturnInt( lastHitCheckResult );
        return;
    }

    lastHitCheckTime = gameLocal.time;

    idVec3 toPos = enemyEnt->GetEyePosition();
    idVec3 eye = playerEnt->GetEyePosition();
    idVec3 dir;

    // expand the ray out as far as possible so we can detect anything behind the enemy
    dir = toPos - eye;
    dir.Normalize();
    toPos = eye + dir * MAX_WORLD_SIZE;
    gameLocal.clip.TracePoint( tr, eye, toPos, MASK_SHOT_BOUNDINGBOX, playerEnt );
    hit = gameLocal.GetTraceEntity( tr );
    if ( tr.fraction >= 1.0f || ( hit == enemyEnt ) )
    {
        lastHitCheckResult = true;
    } /*else if ( ( tr.fraction < 1.0f ) && ( hit->IsType( idAI::Type ) ) && ( static_cast<idAI *>( hit )->team != team ) ) {
		lastHitCheckResult = true;
	} */else
    {
        lastHitCheckResult = false;
    }

    idThread::ReturnInt( lastHitCheckResult );
}

/*
=====================
botAi::Event_EnemyPositionValid
=====================
*/
void botAi::Event_EnemyPositionValid( void )
{
    bool result;

    result = EnemyPositionValid();
    idThread::ReturnInt( result );
}

/*
=====================
botAi::Event_MoveStatus
=====================
*/
void botAi::Event_MoveStatus( void )
{
    idThread::ReturnInt( move.moveStatus );
}

/*
=====================
botAi::Event_StopMove
=====================
*/
void botAi::Event_StopMove( void )
{
    StopMove( MOVE_STATUS_DONE );
}

/*
=====================
botAi::Event_SaveMove
=====================
*/
void botAi::Event_SaveMove( void )
{
    savedMove = move;
}

/*
=====================
botAi::Event_RestoreMove
=====================
*/
void botAi::Event_RestoreMove( void )
{
    idVec3 goalPos;
    idVec3 dest;

    switch( savedMove.moveCommand )
    {
    case MOVE_NONE :
        StopMove( savedMove.moveStatus );
        break;

    case MOVE_TO_ENEMY :
        SetMoveToEnemy();
        break;

    case MOVE_TO_ENTITY :
        SetMoveToEntity( savedMove.goalEntity.GetEntity() );
        break;

    case MOVE_OUT_OF_RANGE :
        SetMoveOutOfRange( savedMove.goalEntity.GetEntity(), savedMove.range );
        break;

    case MOVE_TO_ATTACK_POSITION :
        SetMoveToAttackPosition( savedMove.goalEntity.GetEntity() );
        break;

    case MOVE_TO_COVER :
        SetMoveToCover( savedMove.goalEntity.GetEntity(), lastVisibleEnemyPos );
        break;

    case MOVE_TO_POSITION :
        SetMoveToPosition( savedMove.moveDest );
        break;

    case MOVE_WANDER :
        WanderAround();
        break;
    }

    if ( GetMovePos( goalPos ) )
    {
        CheckObstacleAvoidance( goalPos, dest );
    }
}

/*
=====================
botAi::Event_SetMoveToCover
=====================
*/
void botAi::Event_SetMoveToCover( void )
{
    idActor *enemyEnt = enemy.GetEntity();

    StopMove( MOVE_STATUS_DEST_NOT_FOUND );
    if ( !enemyEnt || !SetMoveToCover( enemyEnt, lastVisibleEnemyPos ) )
    {
        return;
    }
}

/*
=====================
botAi::Event_SetMoveToEnemy
=====================
*/
void botAi::Event_SetMoveToEnemy( void )
{
    StopMove( MOVE_STATUS_DEST_NOT_FOUND );
    if ( !enemy.GetEntity() || !SetMoveToEnemy() )
    {
        return;
    }
}

/*
=====================
botAi::Event_SetMoveOutOfRange
=====================
*/
void botAi::Event_SetMoveOutOfRange( idEntity *entity, float range )
{
    StopMove( MOVE_STATUS_DEST_NOT_FOUND );
    SetMoveOutOfRange( entity, range );
}

/*
=====================
botAi::Event_SetMoveToAttackPosition
=====================
*/
void botAi::Event_SetMoveToAttackPosition( idEntity *entity )
{
    StopMove( MOVE_STATUS_DEST_NOT_FOUND );

    SetMoveToAttackPosition( entity );
}

/*
=====================
botAi::Event_SetMoveToEntity
=====================
*/
void botAi::Event_SetMoveToEntity( idEntity *ent )
{
    StopMove( MOVE_STATUS_DEST_NOT_FOUND );
    if ( ent )
    {
        SetMoveToEntity( ent );
    }
}

/*
=====================
botAi::Event_SetMoveToPosition
=====================
*/
void botAi::Event_SetMoveToPosition( const idVec3 &pos )
{
    StopMove( MOVE_STATUS_DONE );
    SetMoveToPosition( pos );
}

/*
=====================
botAi::Event_SetMoveWander
=====================
*/
void botAi::Event_SetMoveWander( void )
{
    WanderAround();
}

/*
================
botAi::Event_CanReachPosition
================
*/
void botAi::Event_CanReachPosition( const idVec3 &pos )
{
    aasPath_t	path;
    int			toAreaNum;
    int			areaNum;

    toAreaNum = PointReachableAreaNum( pos );
    areaNum	= PointReachableAreaNum( physicsObject->GetOrigin() );
    if ( !toAreaNum || !PathToGoal( path, areaNum, physicsObject->GetOrigin(), toAreaNum, pos ) )
    {
        idThread::ReturnInt( false );
    }
    else
    {
        idThread::ReturnInt( true );
    }
}

/*
================
botAi::Event_CanReachEntity
================
*/
void botAi::Event_CanReachEntity( idEntity *ent )
{
    aasPath_t	path;
    int			toAreaNum;
    int			areaNum;
    idVec3		pos;

    if ( !ent )
    {
        idThread::ReturnInt( false );
        return;
    }

    if ( move.moveType != MOVETYPE_FLY )
    {
        if ( !ent->GetFloorPos( 64.0f, pos ) )
        {
            idThread::ReturnInt( false );
            return;
        }
        if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() )
        {
            idThread::ReturnInt( false );
            return;
        }
    }
    else
    {
        pos = ent->GetPhysics()->GetOrigin();
    }

    toAreaNum = PointReachableAreaNum( pos );
    if ( !toAreaNum )
    {
        idThread::ReturnInt( false );
        return;
    }

    const idVec3 &org = physicsObject->GetOrigin();
    areaNum	= PointReachableAreaNum( org );
    if ( !toAreaNum || !PathToGoal( path, areaNum, org, toAreaNum, pos ) )
    {
        idThread::ReturnInt( false );
    }
    else
    {
        idThread::ReturnInt( true );
    }
}

/*
================
botAi::Event_CanReachEnemy
================
*/
void botAi::Event_CanReachEnemy( void )
{
    aasPath_t	path;
    int			toAreaNum;
    int			areaNum;
    idVec3		pos;
    idActor		*enemyEnt;

    enemyEnt = enemy.GetEntity();
    if ( !enemyEnt )
    {
        idThread::ReturnInt( false );
        return;
    }

    if ( move.moveType != MOVETYPE_FLY )
    {
        if ( enemyEnt->OnLadder() )
        {
            idThread::ReturnInt( false );
            return;
        }
        enemyEnt->GetAASLocation( aas, pos, toAreaNum );
    }
    else
    {
        pos = enemyEnt->GetPhysics()->GetOrigin();
        toAreaNum = PointReachableAreaNum( pos );
    }

    if ( !toAreaNum )
    {
        idThread::ReturnInt( false );
        return;
    }

    const idVec3 &org = physicsObject->GetOrigin();
    areaNum	= PointReachableAreaNum( org );
    if ( !PathToGoal( path, areaNum, org, toAreaNum, pos ) )
    {
        idThread::ReturnInt( false );
    }
    else
    {
        idThread::ReturnInt( true );
    }
}

/*
================
botAi::Event_GetReachableEntityPosition
================
*/
void botAi::Event_GetReachableEntityPosition( idEntity *ent )
{
    int		toAreaNum;
    idVec3	pos;

    if ( !ent->GetFloorPos( 64.0f, pos ) )
    {
        idThread::ReturnInt( false );
        return;
    }
    if ( ent->IsType( idActor::Type ) && static_cast<idActor *>( ent )->OnLadder() )
    {
        idThread::ReturnInt( false );
        return;
    }

    if ( aas )
    {
        toAreaNum = PointReachableAreaNum( pos );
        aas->PushPointIntoAreaNum( toAreaNum, pos );
    }

    idThread::ReturnVector( pos );
}

/*
================
botAi::Event_TravelDistanceToPoint
================
*/
void botAi::Event_TravelDistanceToPoint( const idVec3 &pos )
{
    float time;

    time = TravelDistance( physicsObject->GetOrigin(), pos );
    idThread::ReturnFloat( time );
}

/*
================
botAi::Event_TravelDistanceToEntity
================
*/
void botAi::Event_TravelDistanceToEntity( idEntity *ent )
{
    float time;

    time = TravelDistance( physicsObject->GetOrigin(), ent->GetPhysics()->GetOrigin() );
    idThread::ReturnFloat( time );
}

/*
================
botAi::Event_TravelDistanceBetweenPoints
================
*/
void botAi::Event_TravelDistanceBetweenPoints( const idVec3 &source, const idVec3 &dest )
{
    float time;

    time = TravelDistance( source, dest );
    idThread::ReturnFloat( time );
}

/*
================
botAi::Event_TravelDistanceBetweenEntities
================
*/
void botAi::Event_TravelDistanceBetweenEntities( idEntity *source, idEntity *dest )
{
    float time;

    assert( source );
    assert( dest );
    time = TravelDistance( source->GetPhysics()->GetOrigin(), dest->GetPhysics()->GetOrigin() );
    idThread::ReturnFloat( time );
}

/*
================
botAi::Event_GetObstacle
================
*/
void botAi::Event_GetObstacle( void )
{
    idThread::ReturnEntity( move.obstacle.GetEntity() );
}

/*
================
botAi::Event_PushPointIntoAAS
================
*/
void botAi::Event_PushPointIntoAAS( const idVec3 &pos )
{
    int		areaNum;
    idVec3	newPos;

    areaNum = PointReachableAreaNum( pos );
    if ( areaNum )
    {
        newPos = pos;
        aas->PushPointIntoAreaNum( areaNum, newPos );
        idThread::ReturnVector( newPos );
    }
    else
    {
        idThread::ReturnVector( pos );
    }
}

/*
================
botAi::Event_Acos
TinMan: *todo* this belongs in sys scripting
================
*/
void botAi::Event_Acos( float a )
{
    idThread::ReturnFloat( idMath::ACos16( a ) );
}

/*
================
botAi::Event_GetEntityByNum
TinMan: Direct acess to entities array
================
*/
void botAi::Event_GetEntityByNum( float index )
{
    if ( index > MAX_GENTITIES || index > gameLocal.num_entities )
    {
        idThread::ReturnEntity( NULL );
        return;
    }
    int i = index;
    idThread::ReturnEntity( gameLocal.entities[ i ] );
}

/*
================
botAi::Event_GetNumEntiyes
TinMan: Get number of entities in entity array.
================
*/
void botAi::Event_GetNumEntities( void )
{
    idThread::ReturnFloat( gameLocal.num_entities );
}

/*
================
botAi::Event_GetClassName
TinMan: Added, simple as pimples *todo* rename?
================
*/
void botAi::Event_GetClassName( idEntity *ent )
{
    idThread::ReturnString( ent->spawnArgs.GetString( "classname" ) );
}

/*
================
botAi::Event_GetClassType
TinMan: Added, simple as pimples *todo* rename?
================
*/
void botAi::Event_GetClassType( idEntity *ent )
{
    idThread::ReturnString( ent->GetClassname() );
}

/*
================
botAi::Event_GetFlag
TinMan: *CTF* *todo* still thinking on how to handle ctf specific events, problem is at scripting end
================
*/
void botAi::Event_GetFlag( float team )
{
#ifdef CTF
    if ( team > 1 )
    {
        team = 1;
    }
    idEntity *ent = static_cast<idEntity *>( gameLocal.mpGame.GetTeamFlag( team ) );
    idThread::ReturnEntity( ent );
#else
    idThread::ReturnEntity( NULL );
#endif
}

/*
================
botAi::Event_GetFlagStatus
TinMan: *CTF*
================
*/
void botAi::Event_GetFlagStatus( float team )
{
#ifdef CTF
    if ( team > 1 )
    {
        team = 1;
    }
    idThread::ReturnFloat( gameLocal.mpGame.GetFlagStatus( team ) );
#else
    idThread::ReturnFloat( 0 );
#endif
}

/*
================
botAi::Event_GetFlagCarrier
TinMan: *CTF*
================
*/
void botAi::Event_GetFlagCarrier( float team )
{
#ifdef CTF
    if ( team > 1 )
    {
        team = 1;
    }
    idThread::ReturnEntity( gameLocal.entities[ gameLocal.mpGame.GetFlagCarrier( team ) ] );
#else
    idThread::ReturnEntity( NULL );
#endif
}

/*
================
botAi::Event_GetCapturePoint
TinMan: *CTF* well capture entity really
================
*/
void botAi::Event_GetCapturePoint( float team )
{
#ifdef CTF
    idEntity *	ent;

    if ( team > 1 )
    {
        team = 1;
    }

    for ( int i = 0; i < MAX_GENTITIES; i++ )
    {
        ent = gameLocal.entities[ i ];
        if ( ent )
        {
            if ( ent->IsType( idTrigger_Flag::Type ) )
            {
                if ( ent->spawnArgs.GetInt( "team" ) == team )
                {
                    //gameLocal.Printf( "[Event_GetCapturePoint][team: %i][capteam: %i]\n", playerEnt->team, ent->spawnArgs.GetInt( "team" ) );
                    idThread::ReturnEntity( ent );
                    return;
                }
            }
        }
    }
#endif
    idThread::ReturnEntity( NULL );
}

/*
================
botAi::Event_IsUnderPlat
custom3: Check to see if bot is under specified plat
================
*/
void botAi::Event_IsUnderPlat( idEntity *ent )
{
    if ( ent->IsType( idPlat::Type ) )
    {
        idPlat *plat = static_cast<idPlat *>(ent);
        // this will represent the volume under the plat
        idBounds floorToPlat = plat->GetPhysics()->GetAbsBounds();


        // adjust the mins z value to bottom pos
        floorToPlat[0].z = plat->GetPosition1().z;
        // adjust the maxs z value to top of plat minus arbitrary value to get below it
        floorToPlat[1].z = plat->GetPhysics()->GetAbsBounds()[1].z - 10;

        if ( ai_debugMove.GetBool() )
        {
            gameRenderWorld->DebugBounds( colorGreen, floorToPlat, vec3_origin, gameLocal.msec  );
        }
        // is player inside bounds just created?
        bool under =  floorToPlat.IntersectsBounds( physicsObject->GetAbsBounds() );
        if ( under && ai_debugMove.GetBool() )
        {
            gameRenderWorld->DebugBounds( colorYellow, physicsObject->GetAbsBounds(), vec3_origin, gameLocal.msec  );
            // Event_GetWaitPosition( ent ); was testing it, lol here only for visuals
        }
        idThread::ReturnInt( under );
        return;
    }
    idThread::ReturnInt( false );
}


/*
================
botAi::Event_GetWaitPosition

Find a position out from under plat
cusTom3 had this funny idea to draw a picture for the search arrays ;)

	4-----1-----5
	|			|
	|			|		|
	0			2		|Y
	|			|		|____
	|			|			X
	7-----3-----6
================
*/
void botAi::Event_GetWaitPosition( idEntity *ent )
{
    idVec3 result = physicsObject->GetOrigin();

    if ( ent->IsType( idPlat::Type ) )
    {
        idPlat *plat = static_cast<idPlat *>(ent);

        // expand 24 for player bounding box and another 6 to get out past it
        idBounds bounds = plat->GetPhysics()->GetAbsBounds().Expand( 30 );

        // rotate around the bounds looking for good spot to wait for plat
        idVec3 center = bounds.GetCenter();
        float x[8], y[8], z;
        x[0] = bounds[0][0];
        x[1] = center[0];
        x[2] = bounds[1][0];
        x[3] = center[0];
        x[4] = bounds[0][0];
        x[5] = bounds[1][0];
        x[6] = bounds[1][0];
        x[7] = bounds[0][0];
        y[0] = center[1];
        y[1] = bounds[1][1];
        y[2] = center[1];
        y[3] = bounds[0][1];
        y[4] = bounds[1][1];
        y[5] = bounds[1][1];
        y[6] = bounds[0][1];
        y[7] = bounds[0][1];
        z = playerEnt->GetEyePosition().z; // not sure what z value would be good, trying eye level

        idVec3 search;
        float closest = idMath::INFINITY; // bigger than big
        for ( int i = 0; i < 8; i++ )
        {
            search[0] = x[i];
            search[1] = y[1];
            search[2] = z;
            if ( PointReachableAreaNum( search ) )
            {
                // found a reachable spot, if it is closer than the last one we found use it instead
                float distance = ( search - physicsObject->GetOrigin() ).LengthSqr();
                if ( distance < closest )
                {
                    closest = distance;
                    result = search;
                }
            }
        }
    }
    if ( ai_debugMove.GetBool() )
    {
        gameRenderWorld->DebugArrow( colorCyan, physicsObject->GetOrigin(), result, 1);
    }
    idThread::ReturnVector( result );
}
#endif


/*
================
botAi::Event_GetCommandType
TinMan:
================
*/
void botAi::Event_GetCommandType( void )
{
    idThread::ReturnFloat( commandType );
}

/*
================
botAi::Event_GetCommandEntity
TinMan:
================
*/
void botAi::Event_GetCommandEntity( void )
{
    idThread::ReturnEntity( commandEntity );
}


/*
================
botAi::Event_GetCommandPosition
TinMan:
================
*/
void botAi::Event_GetCommandPosition( void )
{
    idThread::ReturnVector( commandPosition );
}

/*
================
botAi::Event_ClearCommand
TinMan:
================
*/
void botAi::Event_ClearCommand( void )
{
    commandType = NULL;
    commandEntity = NULL;
    commandPosition.Zero();
}

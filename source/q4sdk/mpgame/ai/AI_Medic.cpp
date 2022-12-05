#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AI_Medic.h"


const idEventDef AI_DisableHeal	( "disableHeal" );
const idEventDef AI_EnableHeal	( "enableHeal" );
const idEventDef AI_TakePatient ( "takePatient", "e" );

CLASS_DECLARATION( rvAITactical, rvAIMedic )
	EVENT( AI_DisableHeal,					rvAIMedic::Event_DisableHeal )
	EVENT( AI_EnableHeal,					rvAIMedic::Event_EnableHeal )
	EVENT( AI_TakePatient,					rvAIMedic::TakePatient )
	EVENT( AI_EnableMovement,				rvAIMedic::Event_EnableMovement )
	EVENT( AI_DisableMovement,				rvAIMedic::Event_DisableMovement )
END_CLASS

/*
================
rvAIMedic::rvAIMedic
================
*/
rvAIMedic::rvAIMedic ( void ) {
	patient = NULL;
	isTech = false;
	noAutoHeal = false;
	stationary = false;
	silent = false;
	healObeyTether = false;
	healing = false;
	lastPatientCheckTime = 0;
	emergencyOverride = false;
	//healedAmount = 0;
	healDisabled = false;
	wasAware = false;
	wasIgnoreEnemies = false;
	healDebounceTime = 0;
}

void rvAIMedic::InitSpawnArgsVariables( void )
{
	//NOTE: these shouldn't change from spawn values - maybe they don't need to be variables and saved/loaded?
	isTech = spawnArgs.GetBool( "tech" );
	noAutoHeal = spawnArgs.GetBool( "noAutoHeal" );
	healAmt = 200;//spawnArgs.GetInt( "healAmt", "25" );
	healObeyTether = spawnArgs.GetBool( "healObeyTether" );
	patientRange = spawnArgs.GetFloat( "patientRange", "640" );
	buddyRange = spawnArgs.GetFloat( "buddyRange", "640" );
	enemyRange = spawnArgs.GetFloat( "enemyRange", "1024" );
	healDebounceInterval = 0;//SEC2MS( spawnArgs.GetFloat( "healWait", "0" ) );

	/*
	// JTD: check for player being strogg, since the limits will be different in that case..
	idStr str = gameLocal.world->spawnArgs.GetString ( "player", "player_marine" );
	str.Strip( "player_" );
	// ...we'll end up with something like minMarineHeal...or minStroggHeal
	*/
	minHealValue = 35;//spawnArgs.GetInt( va( "min%sHeal", str.c_str()), "50" );
	maxHealValue = 0;//75;//spawnArgs.GetInt( va( "max%sHeal", str.c_str()), "75" );
}
/*
================
rvAIMedic::Spawn
================
*/
void rvAIMedic::Spawn ( void ) {
	InitSpawnArgsVariables();

	stationary = spawnArgs.GetBool( "stationary" );
	silent = spawnArgs.GetBool( "silent", "0");
	if ( spawnArgs.GetBool( "disableHeal" ) ) {
		Event_DisableHeal();
	}
	if ( spawnArgs.GetBool( "enableHeal" ) ) {
		Event_EnableHeal();
	}
	const char  *func;
	if ( spawnArgs.GetString( "script_postHeal", "", &func ) ) 
	{
		mPostHealScript.Init( func );
	}
}

/*
================
rvAIMedic::Show
================
*/
void rvAIMedic::Show( void ) {
	rvAITactical::Show();
	HideAttachment( spawnArgs.GetString("def_attach") );
}

/*
================
rvAIMedic::Save
================
*/
void rvAIMedic::Save( idSaveGame *savefile ) const {
	patient.Save( savefile );
	savefile->WriteBool( healing );
    savefile->WriteInt( lastPatientCheckTime );
	savefile->WriteBool( emergencyOverride );
	savefile->WriteBool( healDisabled );
	savefile->WriteBool( wasAware );
	savefile->WriteBool( wasIgnoreEnemies );
	//savefile->WriteInt( healedAmount );
	savefile->WriteInt( healDebounceTime );
	savefile->WriteBool( stationary );
	savefile->WriteBool( silent );

	//NOTE: every time these are used, they are set first, no point in saving/loading them!
	savefile->WriteInt( curHealValue );		
	savefile->WriteInt( maxPatientValue );
	mPostHealScript.Save( savefile );
}

/*
================
rvAIMedic::Restore
================
*/
void rvAIMedic::Restore( idRestoreGame *savefile ) {
	patient.Restore( savefile );
	savefile->ReadBool( healing );
	savefile->ReadInt( lastPatientCheckTime );
	savefile->ReadBool( emergencyOverride );
	savefile->ReadBool( healDisabled );
	savefile->ReadBool( wasAware );
	savefile->ReadBool( wasIgnoreEnemies );
	//savefile->ReadInt( healedAmount );
	savefile->ReadInt( healDebounceTime );
	savefile->ReadBool( stationary );
	savefile->ReadBool( silent );

	//NOTE: every time these are used, they are set first, no point in saving/loading them!
	savefile->ReadInt( curHealValue );		
	savefile->ReadInt( maxPatientValue );
	mPostHealScript.Restore( savefile );

	InitSpawnArgsVariables();
}

/*
=====================
rvAIMedic::Event_EnableHeal
=====================
*/
void rvAIMedic::Event_EnableHeal( void ) {
	healDisabled = false;
}

/*
=====================
rvAIMedic::Event_DisableHeal
=====================
*/
void rvAIMedic::Event_DisableHeal( void ) {
	healDisabled = true;
	/*
	if ( patient ) {
		//drop them right now - NOTE: should be done before any scripting?
		DropPatient();
	}
	*/
}

/*
==================
rvAIMedic::Event_EnableMovement
==================
*/
void rvAIMedic::Event_EnableMovement( void ) {	
	stationary = false;
}

/*
==================
rvAIMedic::Event_DisableMovement
==================
*/
void rvAIMedic::Event_DisableMovement ( void ) {	
	stationary = true;
}

/*
=====================
rvAIMedic::TalkTo
=====================
*/
void rvAIMedic::TalkTo( idActor *actor )  {
	if ( actor->IsType( idPlayer::GetClassType() ) )
	{
		idPlayer* player = dynamic_cast<idPlayer*>(actor);
		if ( player )
		{
			emergencyOverride = true;
			if ( AvailableToTakePatient() && CheckTakePatient( player ) )
			{
				return;
			}
			emergencyOverride = false;
		}
	}
	rvAITactical::TalkTo( actor );

	if ( !aifl.action && !aifl.scripted && !IsSpeaking() && !IsHidden() && !silent) {
		if ( GetEnemy() || patient ) {
			Speak( "lipsync_heal_busy_", true );
		} else {
			//Never got proper VO for this, disabling it... :/
			//Speak( "lipsync_heal_noheal_", true );
		}
	}
}

/*
================
rvAIMedic::GetDebugInfo
================
*/
void rvAIMedic::GetDebugInfo( debugInfoProc_t proc, void* userData ) {
	// Base class first
	rvAITactical::GetDebugInfo ( proc, userData );
	
	proc ( "rvAIMedic", "aifl.scripted",	aifl.scripted?"true":"false", userData );
	proc ( "rvAIMedic", "move.fl.disabled",	move.fl.disabled?"true":"false", userData );
	proc ( "rvAIMedic", "IsSpeaking",		IsSpeaking()?"true":"false", userData );
	proc ( "rvAIMedic", "healDisabled",		healDisabled?"true":"false", userData );
	proc ( "rvAIMedic", "noAutoHeal ",		noAutoHeal ?"true":"false", userData );
	proc ( "rvAIMedic", "stationary",		stationary?"true":"false", userData );
	proc ( "rvAIMedic", "silent",			silent?"true":"false", userData );
	proc ( "rvAIMedic", "healObeyTether",	healObeyTether?"true":"false", userData );
	proc ( "rvAIMedic", "patient",			patient==NULL?"<none>":patient->GetName(), userData );
	proc ( "rvAIMedic", "wasAware",			wasAware?"true":"false", userData );
	proc ( "rvAIMedic", "wasIgnoreEnemies",	wasIgnoreEnemies?"true":"false", userData );
	proc ( "rvAIMedic", "lastPatientCheckTime",va("%d",lastPatientCheckTime), userData );
	proc ( "rvAIMedic", "healDebounceTime",	va("%d",healDebounceTime), userData );
	proc ( "rvAIMedic", "healing",			healing?"true":"false", userData );
	proc ( "rvAIMedic", "emergencyOverride",healing?"true":"false", userData );
	//proc ( "rvAIMedic", "healedAmount",		va("%d",healedAmount), userData );
	proc ( "rvAIMedic", "minHealValue",		va("%d",minHealValue), userData );
	proc ( "rvAIMedic", "maxHealValue",		va("%d",maxHealValue), userData );
	proc ( "rvAIMedic", "healAmt",			va("%d",healAmt), userData );
	proc ( "rvAIMedic", "patientRange",		va("%f",patientRange), userData );
	proc ( "rvAIMedic", "buddyRange",		va("%f",buddyRange), userData );
	proc ( "rvAIMedic", "enemyRange",		va("%f",enemyRange), userData );
	proc ( "rvAIMedic", "tech",				isTech?"true":"false", userData );
}

/*
============
rvAIMedic::IsTethered
============
*/
bool rvAIMedic::IsTethered ( void ) const {
	if ( rvAITactical::IsTethered() ) {
		if ( !healObeyTether && patient ) {
			return false;
		}
		return true;
	}
	return false;
}

void rvAIMedic::TakePatient( idPlayer* pPatient )
{
	patient = pPatient;
	if ( emergencyOverride )
	{
		ClearEnemy();
	}

	wasAware = combat.fl.aware;
	wasIgnoreEnemies = combat.fl.ignoreEnemies;
	ProcessEvent( &AI_BecomePassive, true );
	combat.fl.ignoreEnemies = true;
	ClearEnemy();
	lookTarget = patient;
	healing = false;
	//healedAmount = 0;

	if ( pPatient && DistanceTo( pPatient ) > 250.0f && !silent ) 
	{
		//have enough time to say this before we get there...?
		//jshepard: tech/medic dependent speech
		if( isTech ) {
			Speak( "lipsync_call_player_tech_", true );
		} else {
			Speak( "lipsync_call_player_", true );
		}
	}

	if ( !stationary && !move.fl.disabled ) {
		MoveToEntity( patient, 42 );
	}
	SetState( "State_Medic" );
	PostState( "State_Combat" );

	//just in case
	if ( healDebounceInterval ) {
		healDebounceTime = gameLocal.GetTime() + healDebounceInterval;
	}
}

void rvAIMedic::DropPatient( void )
{
	/*
	if ( !spawnArgs.GetBool( "ignoreEnemies" ) )
	{
		combat.fl.ignoreEnemies = false;
	}
	*/
	/*
	if ( !aifl.scripted )
	{
		ProcessEvent( &AI_ForcePosture, AIPOSTURE_DEFAULT );
	}
	*/
	healing = false;
	if ( !aifl.scripted ) {
		ProcessEvent( &AI_BecomeAggressive );
		combat.fl.aware = wasAware;
		combat.fl.ignoreEnemies = wasIgnoreEnemies;
		lookTarget = NULL;
	} else if ( lookTarget == patient ) {
		//if scripted, clear the looktarget only if it's the patient?  This could be wrong, though....
		lookTarget = NULL;
	}

	patient = NULL;

	if ( healDebounceInterval ) {
		healDebounceTime = gameLocal.GetTime() + healDebounceInterval;
	}

	ForceTacticalUpdate();
	if ( !aifl.scripted ) {
		UpdateTactical ( 0 );
	}
	//FIXME: what if they stay in this state?
	HideAttachment( spawnArgs.GetString("def_attach") );
}

void rvAIMedic::SetHealValues( idPlayer* player )
{
	if ( !player )
	{
		return;
	}
	if ( isTech )
	{
		curHealValue = player->inventory.armor;
		maxPatientValue = player->inventory.maxarmor;
	}
	else
	{
		curHealValue = player->health;
		maxPatientValue = player->inventory.maxHealth;
	}
}

bool rvAIMedic::CheckTakePatient( idPlayer* player )
{
	if ( !player )
	{
		return false;
	}
	if ( player->IsHidden() )
	{
		return false;
	}
	if ( player->health <= 0 )
	{
		return false;
	}
	SetHealValues( player );

	if ( curHealValue < maxPatientValue )
	{//they are hurt
		if ( curHealValue <= minHealValue || (gameLocal.GetTime() >= healDebounceTime && emergencyOverride && (!maxHealValue || curHealValue < maxHealValue)) )
		{//patient needs healing or he requested a heal and it's been long enough and he's below the max heal level (if there is one)
			if ( DistanceTo( player ) < patientRange )
			{//close enough
				if ( (!move.fl.disabled && !stationary) || DistanceTo( player ) <= combat.meleeRange )
				{//either I am allowed to move or player is close enough that I don't have to
					//if ( !tether || tether->ValidateDestination( this, player->GetPhysics()->GetOrigin() ) )
					{//not tethered or patient is in our tether
						if ( emergencyOverride || CanSee( player, false ) )
						{//can see the patient - OR: just check PVS?
							TakePatient( player );
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool rvAIMedic::SituationAllowsPatient( void )
{
	if ( !aifl.scripted && !IsHidden() && !aifl.dead )
	{//not scripted right now
		if ( combat.fl.ignoreEnemies ) {
			return true;
		}
		bool enemyInPVS = false;
		bool enemyInRange = false;
		if ( GetEnemy() )
		{//sorry, we have to bail on you!
			enemyInRange = (DistanceTo( GetEnemy() ) < enemyRange);
			if ( enemyInRange ) {
				pvsHandle_t pvs;
				// Setup our local variables used in the search
				pvs	 = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
				// If this enemy isnt in the same pvps then use them as a backup
				if ( gameLocal.pvs.InCurrentPVS( pvs, GetEnemy()->GetPVSAreas(), GetEnemy()->GetNumPVSAreas() ) ) {
					enemyInPVS = true;
					emergencyOverride = false;
				}
				gameLocal.pvs.FreeCurrentPVS( pvs );
			}
		}
		if ( emergencyOverride )
		{//don't care how crazy it is, go for it!
			return true;
		}
		if ( (!GetEnemy() || !enemyInPVS || !enemyInRange ) && gameLocal.GetTime() - enemy.changeTime > 5000 )
		{//haven't had an enemy for 5 seconds
			if ( !aiManager.LocalTeamHasEnemies( this, buddyRange, enemyRange, true ) )
			{//local buddies don't have enemies
				//NOTE: which buddies and enemies are local can change as we head to our patient!
				return true;
			}
		}
	}
	return false;
}

bool rvAIMedic::AvailableToTakePatient( void )
{
	if ( !healDisabled )
	{//not forced to disabled
		if ( !aifl.action )
		{//not in the middle of an action
			if ( !patient )
			{//don't already have a patient
				if ( !IsSpeaking() )
				{//not talking
					return SituationAllowsPatient();
				}
			}
		}
	}
	return false;
}

/*
================
rvAIMedic::Think
================
*/
void rvAIMedic::Think ( void ) {
	rvAITactical::Think ( );

//	while( entMedic.getKey("alive") == "true" && entMedic.getKey("healer") == "1")	
//???
	if ( !noAutoHeal )
	{
		if ( gameLocal.GetTime() - lastPatientCheckTime > 1000 )
		{
			lastPatientCheckTime = gameLocal.GetTime();
			if ( !patient )
			{
				emergencyOverride = false;
			}
			if ( AvailableToTakePatient() )
			{
				idPlayer* player = gameLocal.GetLocalPlayer();

				if ( CheckTakePatient( player ) )
				{
					return;
				}
				//otherwise, check team?
				/*
				idActor* actor;
				for( actor = aiManager.GetAllyTeam ( (aiTeam_t)team ); actor; actor = actor->teamNode.Next() ) 	
				{
					if ( CheckTakePatient( actor ) )
					{
						return;
					}
				}
				*/
			}
		}
	}
}				

/*
=====================
rvAIMedic::OnStateThreadClear
=====================
*/
void rvAIMedic::OnStateThreadClear( const char *statename, int flags ) {
	if ( idStr::Icmp( statename, "State_Medic" ) ) {
		if ( patient ) {
			//BAH!  Someone changed our state on us!
			if ( lookTarget == patient ) {
				lookTarget = NULL;
			}
			patient = NULL;
			healing = false;
		}
	}
}

/*
============
rvAIMedic::OnStartMoving
============
*/
void rvAIMedic::OnStartMoving ( void ) {
	idAI::OnStartMoving();
	if ( patient )
	{//we were trying to heal!
		if ( move.moveCommand != MOVE_TO_ENTITY
			|| move.goalEntity != patient )
		{//being told to leave the patient
			//abort the heal, for now
			DropPatient();
			if ( !aifl.scripted ) {
				//only do this if you're not already scripted?
				ExecScriptFunction( mPostHealScript );
			}
		}
	}
}

/*
=====================
rvAIMedic::Pain
=====================
*/
bool rvAIMedic::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	bool retVal = idAI::Pain( inflictor, attacker, damage, dir, location );
	if ( retVal && patient ) {
		//if get hit while trying to heal, protect ourselves
		if ( !aifl.scripted ) {
			//only do this if you're not already scripted?
			StopMove ( MOVE_STATUS_DONE );
		}
		DropPatient();
		if ( !aifl.scripted ) {
			//only do this if you're not already scripted?
			ExecScriptFunction( mPostHealScript );
		}
	}
	return retVal;
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvAIMedic )
	STATE ( "State_Medic",			rvAIMedic::State_Medic )
END_CLASS_STATES

/*
================
idAI::State_Medic

Rush towards the patient to melee range and heal until they're okay
================
*/
stateResult_t rvAIMedic::State_Medic ( const stateParms_t& parms ) {	
	enum {
		STAGE_MOVE,			// Move towards the patient, speak & start anim
		STAGE_PRE_HEAL_ANIM_WAIT,// Wait for pre heal anim to finish, then start the normal heal anim
		STAGE_HEAL_START_WAIT,// Wait for start anim to finish
		STAGE_HEAL,			// Keep healing until no longer in melee range or they're fully healed
		STAGE_WAIT_FINISH	// Finish anim
	};	
	if ( !patient || patient->health <= 0 || !SituationAllowsPatient() )
	{//patient dead or situation is bad
		//NOTE: if patient still alive and situation is just bad, maybe we shouldn't break out altogether, maybe pause and resume?
		StopMove ( MOVE_STATUS_DONE );
		DropPatient();
		ExecScriptFunction( mPostHealScript );
		return SRESULT_DONE;
	}

	if ( !move.fl.done )
	{
		if ( move.moveCommand != MOVE_TO_ENTITY
			|| move.goalEntity != patient )
		{//something stomped our move, give it up, for now... :/
			DropPatient();
			ExecScriptFunction( mPostHealScript );
			return SRESULT_DONE;
		}
	}

	switch ( parms.stage ) {
		case STAGE_MOVE:
			// Attack when we have either stopped moving or are within melee range
			if ( move.fl.done )
			{
				if ( DistanceTo( patient ) > combat.meleeRange || !CanSee(patient,false) ) 
				{//wtf, we're not there yet, try again!
					if ( !stationary && !move.fl.disabled ) {
						MoveToEntity( patient, 42 );
					}
				}
				else
				{//we're there!
					if ( !healing )
					{
						SetHealValues( patient );
						if ( !IsSpeaking() && speakTime < (gameLocal.GetTime() - 2000)  && !silent )
						{//didn't speak in last couple seconds
							//jshepard: tech/medic dependent speech
							if( isTech ) {
								Speak( "lipsync_heal_start_tech_", true );
							} else {
								Speak( "lipsync_heal_start_", true );
							}
						}
					}
					StopMove ( MOVE_STATUS_DONE );
					healing = true;
					// check for preHeal anim key and if it's present, play it first.
					const char  *preHealAnim;
					if ( spawnArgs.GetString( "anim_preHeal", "", &preHealAnim )) 
					{
						PlayAnim( ANIMCHANNEL_TORSO, preHealAnim, 4 );
						return SRESULT_STAGE ( STAGE_PRE_HEAL_ANIM_WAIT );
					}
					else // otherwise just use the regular anim to start healing
					{
						PlayAnim( ANIMCHANNEL_TORSO, "medic_treating_player_start", 4 );
						return SRESULT_STAGE ( STAGE_HEAL_START_WAIT );
					}
				}
			}
			if ( !stationary && move.range != 42.0f )
			{
				//MCG: if you ever get this assert, please call me over so I can debug it!
				assert(move.range==42.0f);
				move.range = 42.0f;//shouldn't have to do this, but sometimes something is overriding the range to 8!  VERY VERY BAD... :_(
			}
			/*
			else if ( !CheckTacticalMove ( AITACTICAL_MEDIC ) && CanSee(patient,false) ) 
			{//we're here, just stop
				Speak( "lipsync_medic_arrive", true );
				StopMove ( MOVE_STATUS_DONE );
				return SRESULT_STAGE ( STAGE_HEAL );
			}
			*/

			// Perform actions on the way to the patient
			if ( UpdateAction ( ) ) {
				return SRESULT_WAIT;
			}

			// Update enemy, not tactical state
			if ( !emergencyOverride )
			{
				// Keep the enemy status up to date
				if ( !combat.fl.ignoreEnemies ) {
					// If we dont have an enemy or havent seen our enemy for a while just find a new one entirely
					if ( gameLocal.time - enemy.checkTime > 250 ) {
						CheckForEnemy ( true, true );
					} else if ( !IsEnemyRecentlyVisible ( ) ) {
						CheckForEnemy ( true );
					}
				}			
			}

		return SRESULT_WAIT;
	
		// intermediate state which may or may not exist...depends on the presence of pre heal anim key on this entity
		case STAGE_PRE_HEAL_ANIM_WAIT:
			const char  *preHealAnim;
			spawnArgs.GetString( "anim_preHeal", "", &preHealAnim );

			if ( AnimDone( ANIMCHANNEL_TORSO, 4 ) || idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), preHealAnim ))
			{//finished or interrupted
				PlayAnim( ANIMCHANNEL_TORSO, "medic_treating_player_start", 4 );
				return SRESULT_STAGE ( STAGE_HEAL_START_WAIT );
			}
			return SRESULT_WAIT;


		case STAGE_HEAL_START_WAIT:
			/*
			if ( patient->pfl.crouch && postureIdeal != AIPOSTURE_CROUCH )
			{
				ProcessEvent( &AI_ForcePosture, AIPOSTURE_CROUCH );
			}
			else if ( !patient->pfl.crouch && postureIdeal == AIPOSTURE_CROUCH )
			{
				ProcessEvent( &AI_ForcePosture, AIPOSTURE_STAND );
			}
			*/
			TurnToward ( patient->GetPhysics()->GetOrigin ( ) );

			if ( AnimDone( ANIMCHANNEL_TORSO, 4 ) || idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "medic_treating_player_start" ) )
			{//finished or interrupted
//				PlayCycle( ANIMCHANNEL_TORSO, "medic_treating_player", 4 );
				PlayAnim( ANIMCHANNEL_TORSO, "medic_treating_player", 4 );
				//show the tool, just in case the anim was interrupted
				ShowAttachment( spawnArgs.GetString("def_attach") );
				return SRESULT_STAGE ( STAGE_HEAL );
			}
			return SRESULT_WAIT;

		case STAGE_HEAL:
			{
				SetHealValues( patient );
				/*
				if ( curHealValue >= maxPatientValue 
					|| (curHealValue > minHealValue && healedAmount >= healAmt) )
				{//patient fully healed (or we've used up our allotment), we're done here
					Speak( "lipsync_heal_end_", true );
					PlayAnim( ANIMCHANNEL_TORSO, "medic_treating_player_end", 4 );	
					return SRESULT_STAGE ( STAGE_WAIT_FINISH );
				}
				*/

				// If we are out of melee range or lost sight of our patient then start moving again			
				/*
				if ( !stationary && !move.fl.disabled ) {
					if ( DistanceTo( patient ) > combat.meleeRange || !CanSee( patient, false ) ) {
						MoveToEntity( patient, 42 );
						Speak( "lipsync_heal_move_", true );
						SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
						return SRESULT_STAGE ( STAGE_MOVE );
					}
				}
				*/

				/*
				combat.fl.ignoreEnemies = true;
				*/
				/*
				if ( patient->pfl.crouch && postureIdeal != AIPOSTURE_CROUCH )
				{
					ProcessEvent( &AI_ForcePosture, AIPOSTURE_CROUCH );
				}
				else if ( !patient->pfl.crouch && postureIdeal == AIPOSTURE_CROUCH )
				{
					ProcessEvent( &AI_ForcePosture, AIPOSTURE_STAND );
				}
				*/

				// Always face patient when in melee range
				TurnToward ( patient->GetPhysics()->GetOrigin ( ) );

				// Perform actions while standing still
				/*
				if ( UpdateAction ( ) ) {
					return SRESULT_WAIT;
				}

				// Update enemy, not tactical state
				if ( !emergencyOverride )
				{
					combat.tacticalUpdateTime = gameLocal.GetTime();
					if ( UpdateTactical( 100000 ) ) {
						DropPatient();
						return SRESULT_DONE_WAIT;
					}
				}
				*/

				//jshepard: tech/medic dependent speech
				if ( AnimDone( ANIMCHANNEL_TORSO, 4 ) || idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "medic_treating_player" ) ) {
					if ( !isTech ) {
						patient->health = patient->health+healAmt>maxPatientValue?maxPatientValue:patient->health+healAmt;
						if( !silent) {
							Speak( "lipsync_heal_end_", true );
						}
					} else {
						patient->inventory.armor = patient->inventory.armor+healAmt>maxPatientValue?maxPatientValue:patient->inventory.armor+healAmt;;
						if( !silent) {
							Speak( "lipsync_heal_end_tech_", true );
						}
					}
					PlayAnim( ANIMCHANNEL_TORSO, "medic_treating_player_end", 4 );	
					return SRESULT_STAGE ( STAGE_WAIT_FINISH );
				}
				if ( gameLocal.random.RandomFloat() > 0.5f )
				{
					if ( !isTech )
					{
						if ( patient->health < maxPatientValue ) {
							patient->health++;
						}
					}
					else
					{
						if ( patient->inventory.armor < maxPatientValue ) {
							patient->inventory.armor++;
						}
					}
				}
			}
			return SRESULT_WAIT;

		case STAGE_WAIT_FINISH:
			if ( AnimDone( ANIMCHANNEL_TORSO, 4 ) || idStr::Icmp( animator.CurrentAnim( ANIMCHANNEL_TORSO )->AnimName(), "medic_treating_player_end" ) )
			{//finished or interrupted
				//turn off the tool, just in case we were interrupted
				DropPatient();
				ExecScriptFunction( mPostHealScript );
				return SRESULT_DONE;
			}
			return SRESULT_WAIT;

	}
	return SRESULT_ERROR;
}

/*
================

AI_Announcements.cpp

================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../Projectile.h"
#include "AI_Manager.h"

/*
=====================
idAI::AnnounceNewEnemy

Announce that we are going to shooting at a different enemy
=====================
*/
void idAI::AnnounceNewEnemy( void ) {
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	if ( !enemy.ent || !enemy.ent->IsType( idActor::GetClassType() ) ) {
		return;
	}

	if ( !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_NEWENEMY ) ) {
		return;
	}

	// Check to see if we should announce the new enemy as a sniper
	// jshepard: Disabled by request
	/*
	if ( aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SNIPER ) && enemy.ent->spawnArgs.GetBool ( "sniper" ) ) {
		// Should we announce the sniper as a high up sniper?
		if ( (enemy.lastKnownPosition - physicsObj.GetOrigin()) * physicsObj.GetGravityNormal() >= 250.0f ) {
			if ( Speak ( "lipsync_high_sniper", true ) ) {
				aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SNIPER, 10000 );
				return;
			}
			return;
		}
		
		// Just announce the sniper
		if ( Speak ( "lipsync_sniper", true ) ) {
			aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SNIPER, 10000 );
			return;
		}
	}
	*/

	idActor* newEnemyAct = static_cast<idActor*>(enemy.ent.GetEntity());
	bool	 result		 = false;

	//first see if the new enemy is behind our buddy closest to the enemy
	idActor* teammate = aiManager.NearestTeammateToPoint( this, newEnemyAct->GetPhysics()->GetOrigin(), false, 200.0f );
	if ( teammate ) {
		if ( aiManager.ActorIsBehindActor( newEnemyAct, teammate ) ) {
			result = Speak ( "lipsync_enemy_back", true );
		}
	}

	// If we havent spoken yet then find a teammat near us that we can tell about our enemy and
	// determine the best announcement to make based on their location
	if ( !result ) {
		teammate = aiManager.NearestTeammateToPoint( this, GetPhysics()->GetOrigin(), false, 300 );
		if ( teammate ) {
			if ( newEnemyAct->GetPhysics()->GetOrigin().z-GetPhysics()->GetOrigin().z >= 250.0f ) {
				result = Speak ( "lipsync_enemy_high", true );
			}
			
			// IF we still havent spoken and we're talking to the player, give him directional info
			if ( !result && teammate->IsType( idPlayer::GetClassType() ) ) {
				idPlayer* teamPlayer = static_cast<idPlayer*>(teammate);
				idVec3 dir = newEnemyAct->GetPhysics()->GetOrigin()-teamPlayer->GetPhysics()->GetOrigin();
				dir.z = 0;
				dir.NormalizeFast();
				idVec3 fwd = teamPlayer->viewAxis[0];
				fwd.z = 0.0f;
				fwd.NormalizeFast();
				idVec3 lt = teamPlayer->viewAxis[1];
				lt.z = 0.0f;
				lt.NormalizeFast();
				if ( fabs( dir * fwd ) < 0.4f )	{
					// more to the side than the front
					if ( dir * lt > 0 ) {
						result = Speak ( "lipsync_enemy_left", true );
					} else {
						result = Speak ( "lipsync_enemy_right", true );
					}
				}
			}
			
			// If we still havent spoken, just announce the enemy
			if ( !result ) {
				//FIXME: check right & left?
				//FIXME: check for surrounded?
				result = Speak ( "lipsync_enemy_default", true );
			}
		}
	}

	// If we spoke then set the timer
	if ( result ) {
		aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_NEWENEMY, 2000 );
	}
}

/*
=====================
idAI::AnnounceKill

Announce that we have killed our enemy
=====================
*/
void idAI::AnnounceKill( idActor* victim ) {
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	// If already speaking or one the same team, dont announce the kill
	if ( victim->team == team ) {
		return;
	}
	//jshepard: "Watch It" sounds have been cut. Replaced with Victory.

	// Generic annoucement of enemy's death
	Speak( "lipsync_victory", true );


	return;

/*
	// If the victim is targetting the player and was close to the player when he died
	// then announce to the player to be careful
	if ( victim->IsType( idAI::Type ) )	{
		idAI* vicAI = static_cast<idAI*>(victim);
		if ( vicAI && vicAI->GetEnemy() && vicAI->GetEnemy()->IsType ( idPlayer::GetClassType() ) ) {
			idPlayer* vicEnemyPlayer = static_cast<idPlayer*>(vicAI->GetEnemy());
			if ( vicEnemyPlayer->team == team ) {
				idVec3 diff = vicAI->GetPhysics()->GetOrigin() - vicEnemyPlayer->GetPhysics()->GetOrigin();
				if ( !vicEnemyPlayer->CheckFOV ( vicAI->GetPhysics()->GetOrigin() ) || diff.LengthSqr() < 300.0f * 300.0f ) {
					Speak( "lipsync_watchit", true );
					return;
				}
			}
		}
	}
	
	// Chance that we say "watch it" to the closest teammate to him if the guy that died was facing the teammate 
	// when he died (ie, was a possible threat)
	idActor* teammate = aiManager.NearestTeammateToPoint( this, victim->GetPhysics()->GetOrigin(), false, 300.0f );
	if ( teammate && victim->CheckFOV(teammate->GetPhysics()->GetOrigin()) ) {		
		if ( gameLocal.random.RandomInt(2) < 1 ) {
			Speak( "lipsync_watchit", true );
			return;
		}
	}
*/

}

/*
=====================
idAI::AnnounceTactical

Announce the changing of tactical status
=====================
*/
void idAI::AnnounceTactical( aiTactical_t newTactical ) {
	bool result = false;
	
	// If already speaking dont bother
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	// Make sure nobody on this team has announced a tactical change recently
	if ( !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_TACTICAL ) ) {
		return;
	}

	switch ( newTactical ) {
		case AITACTICAL_MELEE:
			if ( gameLocal.random.RandomFloat() < 0.2f ) {
				result = Speak( "lipsync_rush", true );
			}
			break;
/*
		case AITACTICAL_COVER:
			if ( gameLocal.random.RandomFloat() < 0.2f ) {
                result = Speak( "lipsync_cover", true );
			}
			break;
		case AITACTICAL_COVER_FLANK:
			result = Speak( "lipsync_flank", true );
			break;
		case AITACTICAL_COVER_ADVANCE:
			result = Speak( "lipsync_moveup", true );
			break;
		case AITACTICAL_COVER_RETREAT:
			result = Speak( "lipsync_fallback", true );
			break;
		case AITACTICAL_COVER_AMBUSH:
			break;
		case AITACTICAL_RANGED:
			break;
		case AITACTICAL_TURRET:
			break;
		case AITACTICAL_HIDE:
			result = Speak( "lipsync_cover", true );
			break;
*/
	}
	
	if ( result ) {
		aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_TACTICAL, 2000 );
	}
}

/*
=====================
idAI::AnnounceSuppressed

Announce that someone is using supressing fire on us
=====================
*/
void idAI::AnnounceSuppressed( idActor *suppressor ) {

	//jshepard: Suppressed and suppressing removed by request
	return;
/*
	// Dont bother if we are already speaking
	if ( !CanAnnounce ( ) ) {
		return;
	}

	// Make sure nobody on this team has announced a tactical change recently
	if ( !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SUPPRESSED ) ) {
		return;
	}

	//FIXME: check crossfire?
	if ( Speak( "lipsync_supressed", true ) ) {
		aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SUPPRESSED, 3000 );
	}
*/

}

/*
=====================
idAI::AnnounceSuppressing

Announce that we are about to use supressing fire
=====================
*/
void idAI::AnnounceSuppressing( void ) {

	//jshepard: Suppressed and suppressing removed by request
	return;
/*
	// Dont bother if already speaking
	if ( !CanAnnounce ( ) ) {
		return;
	}

	// Make sure nobody on this team has announced a tactical change recently
	if ( !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SUPPRESSING ) ) {
		return;
	}

	//Make the guy being shot at know this
	if ( enemy.ent->IsType( idAI::Type ) ) {
		idAI* enemyAI = dynamic_cast<idAI*>(enemy.ent.GetEntity());
		if ( enemyAI  ) {
			enemyAI->AnnounceSuppressed( this );
		}
	}

	if ( Speak( "lipsync_supressing", true ) ) {
		aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_SUPPRESSING, 5000 );
	}
*/
}

/*
=====================
idAI::AnnounceFlinch

Announce that an attack just missed us
=====================
*/
void idAI::AnnounceFlinch( idEntity *attacker ) {
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	idActor* attackActor = dynamic_cast<idActor*>(attacker);
	
	// Friendly fire?
	if ( attackActor && attackActor->team == team ) {
		if ( gameLocal.random.RandomFloat() < 0.2f ) {
			AnnounceFriendlyFire( attackActor );
		}
	} 
//jshepard: sniper announcement removed by request
/*
	else if ( attackActor->spawnArgs.GetBool ( "sniper" ) ) {
		//TEMP: static debounce timer
		static int lastPlayed2 = 0;
		if ( gameLocal.time - lastPlayed2 < 10000 ) {
			return;
		}
		lastPlayed2 = gameLocal.time;
		Speak( "lipsync_sniper", true );
	} else {
*/
	else {
		if ( gameLocal.random.RandomFloat() < 0.4f ) {
			return;
		}
		//TEMP: static debounce timer
		static int lastPlayed = 0;
		if ( gameLocal.time - lastPlayed < 5000 ) {
			return;
		}
		lastPlayed = gameLocal.time;
		Speak( "lipsync_closeone", true );
	}
}

/*
=====================
idAI::AnnounceInjured

Announce that we have been injured
=====================
*/
void idAI::AnnounceInjured( void ) {
	if ( !CanAnnounce ( 1.0f ) ) {
		return;
	}

	Speak( "lipsync_needhelp", true );
}

/*
=====================
idAI::AnnounceFriendlyFire

Announce that someone on our own team is shooting us
=====================
*/
void idAI::AnnounceFriendlyFire( idActor* attacker ) {
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	// Early outs
	if ( health <= 0 || attacker == this ) {
		return;
	}

	// Don't react to ff from other buddy AI
	if ( !attacker->IsType( idPlayer::GetClassType() ) ) {
		return;
	}

	// Make sure nobody on this team has announced a tactical change recently
	if ( !aiManager.CheckTeamTimer ( team, AITEAMTIMER_ANNOUNCE_FRIENDLYFIRE ) ) {
		return;
	}

	// Must be on same team for friendly fire
	if ( team != attacker->team ) {
		return;
	}
	
	// Must be close enough to hear it
	if ( DistanceTo ( attacker ) > 300.0f ) {
		return;
	}
	
	//FIXME: escalate?
	if ( Speak( "lipsync_checkfire", true ) ) {
		aiManager.SetTeamTimer ( team, AITEAMTIMER_ANNOUNCE_FRIENDLYFIRE, 1000 );
	}
}

/*
=====================
idAI::AnnounceGrenade
=====================
*/
void idAI::AnnounceGrenade( void ) {
	if ( !CanAnnounce ( 1 ) ) {
		return;
	}

	static int lastPlayed = 0;
	if ( gameLocal.time - lastPlayed < 5000 ) {
		return;
	}
	lastPlayed = gameLocal.time;
	//FIXME: escalate?
	Speak( "lipsync_grenade", true );
}

/*
=====================
idAI::AnnounceGrenadeThrow

Announce that we are throwing a grenade
=====================
*/
void idAI::AnnounceGrenadeThrow( void ) {
	if ( !CanAnnounce ( announceRate ) ) {
		return;
	}

	static int lastPlayed = 0;
	if ( gameLocal.time - lastPlayed < 1000 ) {
		return;
	}
	lastPlayed = gameLocal.time;
	//FIXME: escalate?
	Speak( "lipsync_throw_grenade", true );
}


/*
=====================
rvAIManager::AnnounceDeath

Announce through an ally of the victem that they have died
=====================
*/
void rvAIManager::AnnounceDeath( idAI* victim, idEntity* attacker ) {
	idActor* teammate;
	idAI*	 teammateAI;

	// Friendly fire kill?
	//MCG NOTE: This isn't even possible anymore...
	if ( attacker->IsType ( idPlayer::GetClassType() ) && static_cast<idPlayer*>(attacker)->team == victim->team ) {
		teammate   = NearestTeammateToPoint( static_cast<idActor*>(attacker), attacker->GetPhysics()->GetOrigin(), true, 500.0f );
		teammateAI = dynamic_cast<idAI*>(teammate);
		
		if ( teammateAI && teammateAI->CanAnnounce( teammateAI->announceRate ) ) {
			teammateAI->Speak( "lipsync_traitor", true );
			return;
		}
	}

	teammate   = NearestTeammateToPoint( victim, victim->GetPhysics()->GetOrigin(), true, 1000.0f );
	teammateAI = dynamic_cast<idAI*>(teammate);

	//jshepard: double check to make sure we don't call out our own death!
	if( teammateAI == victim )	{
		//MCG: note - NearestTeammateToPoint should *never* allow this, should never happen
		assert(0);
		return;
	}
	
	// Early out if we dont have a teammate or our teammate cant talk
	if ( !attacker || !teammateAI || !teammateAI->CanAnnounce ( teammateAI->announceRate ) ) {
		return;
	}
				
	// Announce sniper?
	// jshepard: sniper announcement removed by request
/*
	if ( attacker->spawnArgs.GetBool ( "sniper" ) ) {
		if ( !aiManager.CheckTeamTimer( teammateAI->team, AITEAMTIMER_ANNOUNCE_SNIPER ) ) {
			if ( teammateAI->Speak( "lipsync_sniper", true ) ) {
				aiManager.SetTeamTimer ( teammateAI->team, AITEAMTIMER_ANNOUNCE_SNIPER, 10000 );
				return;
			}
		} 
	}
*/
	// Annoucne specific death or just a generic death	
	const char* shortName;
	if ( !victim->spawnArgs.GetString ( "npc_shortname", "", &shortName ) || !*shortName ||
		 !teammateAI->Speak ( va("lipsync_%s_killed", shortName ), true ) ) {
		teammateAI->Speak( "lipsync_mandown", true );
	}
}

/*
=====================
idAI::AnnounceKill

Announces an ai being killed
=====================
*/
void rvAIManager::AnnounceKill ( idAI* victim, idEntity* attacker, idEntity* inflictor ) {
	idActor* teammate;
	idAI*	 teammateAI;
	
	// Friendly fire deaths are handled elsewhere
	if ( attacker->IsType ( idActor::GetClassType() ) && static_cast<idActor*>(attacker)->team == victim->team ) {
		return;
	}
	
	// If it was an AI guy that did the killing then just let him announce it
	if ( attacker->IsType( idAI::Type ) ) {
		//announce the kill
 		static_cast<idAI*>(attacker)->AnnounceKill( victim );
	} else if ( attacker->IsType( idPlayer::GetClassType() ) ) {
		idPlayer* attackerPlayer = static_cast<idPlayer*>(attacker);
		
		// If the guy who died is an AI guy who was targetting a buddy nearby, have him say "thanks!"
		// jshepard: these are cut unless we can get some tighter "thanks" quotes
/*		if ( victim->IsType( idAI::Type ) )	{
			idAI* victimAI = static_cast<idAI*>(victim);
			//if the victim's enemy is a teammate of mine, make the teammate say "thanks!"
			if ( victimAI && victimAI->GetEnemy() ) {
				idAI* victimEnemyAI = dynamic_cast<idAI*>(victimAI->GetEnemy());
				
				// See if the enemy of the guy who died is a teammate and wants to say thanks
				if ( victimEnemyAI && victimEnemyAI->CanAnnounce ( ) && victimEnemyAI->team == attackerPlayer->team ) {					
					float distSqr = (victimAI->GetPhysics()->GetOrigin() - victimEnemyAI->GetPhysics()->GetOrigin()).LengthSqr ( );
					if ( distSqr < Square ( 300.0f ) ) {
						//teammate was fighting him or close to him
						victimEnemyAI->Speak( "lipsync_thanks", true );
						return;
					}
				}
			}
		}	*/
		
		// Grab a nearby teammate of the player and say "nice shot!"
		teammate   = NearestTeammateToPoint( attackerPlayer, attacker->GetPhysics()->GetOrigin(), true, 500.0f, true );
		teammateAI = dynamic_cast<idAI*>(teammate);
		if ( teammateAI && teammateAI->CanAnnounce( teammateAI->announceRate ) ) {
			idProjectile* proj = dynamic_cast<idProjectile*>(inflictor);
			//killed them with a grenade?
			if ( proj && proj->spawnArgs.GetBool( "thrown" ) ) {
				teammateAI->Speak( "lipsync_nicetoss", true );
			// Or just shot them?
			} else {
				teammateAI->Speak( "lipsync_niceshot", true );
			}
			return;
		}
	}
}


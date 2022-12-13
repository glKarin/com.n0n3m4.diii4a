#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( hhMonsterAI, hhPossessedTommy )
END_CLASS

// TODO:  Set up a rhythmic damaging of this entity.

void hhPossessedTommy::Spawn() {
	physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL );
	nextDrop = 0;
}

void hhPossessedTommy::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( attacker->IsType( hhPlayer::Type ) ) { // Being attacked by a player, must be a spiritwalking player
		if ( possessedProxy.IsValid() ) {
			hhSpiritProxy *proxy = possessedProxy.GetEntity();

			Hide();
			
			proxy->SetOrigin( GetOrigin() );
			proxy->SetAxis( GetAxis() );
			proxy->Show();
			proxy->GetPhysics()->SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL );

			if ( proxy->GetPlayer() ) {
				proxy->GetPlayer()->Unpossess();
			}
		}

		hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );
		PostEventMS( &EV_Remove, 0 );

		return;
	}
	
	// OTHERWISE, KILL THIS ENTITY AND KILL THE PLAYER -- create a death proxy here, etc
	hhMonsterAI::Killed( inflictor, attacker, damage, dir, location );
	
	// Kill the player
	if (possessedProxy.IsValid() ) {
		if ( possessedProxy->GetPlayer() ) {
			possessedProxy->GetPlayer()->PossessKilled();
		}
	}
}

void hhPossessedTommy::Event_Remove(void) {
	if (!fl.hidden && possessedProxy.IsValid()) {
		Hide();

		hhSpiritProxy *proxy = possessedProxy.GetEntity();

		proxy->SetOrigin( GetOrigin() );
		proxy->SetAxis( GetAxis() );
		proxy->Show();
		proxy->GetPhysics()->SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL );

		if ( proxy->GetPlayer() ) {
			proxy->GetPlayer()->Unpossess();
		}
	}
	hhMonsterAI::Event_Remove();
}

void hhPossessedTommy::Think(void) {
	PROFILE_SCOPE("AI", PROFMASK_NORMAL|PROFMASK_AI);
	if (ai_skipThink.GetBool()) {
		return;
	}

	hhMonsterAI::Think();
	if (possessedProxy.IsValid() && gameLocal.time > nextDrop) {
		hhPlayer *player = possessedProxy->GetPlayer();
		if (player) {
			player->SetHealth(player->GetHealth() - 1);
			if (player->GetHealth() <= 0) {
				Hide();

				possessedProxy->SetOrigin( GetOrigin() );
				possessedProxy->SetAxis( GetAxis() );
				possessedProxy->Show();
				possessedProxy->GetPhysics()->SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP | CONTENTS_RENDERMODEL );

				player->Unpossess();

				player->Killed(this, this, 1, vec3_origin, INVALID_JOINT);
				PostEventMS(&EV_Remove, 0);
			}
		}

		float min = spawnArgs.GetFloat( "dda_drain_min" );
		float max = spawnArgs.GetFloat( "dda_drain_max" );
		nextDrop = gameLocal.time + min + (max - min) * (1.0f - gameLocal.GetDDAValue());
	}
}

void hhPossessedTommy::Save( idSaveGame *savefile ) const {
	possessedProxy.Save( savefile );
}

void hhPossessedTommy::Restore( idRestoreGame *savefile ) {
	possessedProxy.Restore( savefile );
	nextDrop = 0;
}

void hhPossessedTommy::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	hhMonsterAI::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
	// Turn to a different direction
	float adjust = 45.0f + (gameLocal.random.RandomFloat() * 30.0f);
	if (gameLocal.random.RandomInt(100) > 50) {
		adjust = -adjust;
	}
	gameLocal.Printf("Adjusting by %f\n", adjust);
	TurnToward(current_yaw + adjust);
	move.nextWanderTime = 0;
}


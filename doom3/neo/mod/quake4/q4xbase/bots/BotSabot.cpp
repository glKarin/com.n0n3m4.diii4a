#include "../../idlib/precompiled.h"
#pragma hdrstop

#ifdef MOD_BOTS

#include "../Game_local.h"

/*
===============================================================================

	SABot - Stupid Angry Bot - release alpha 8 - "I'm not a puppet! I'm a real boy!"

	botSabot
	SABot specific mojo and stuff not for the botAi base.

===============================================================================
*/

CLASS_DECLARATION( botAi, botSabot )
END_CLASS

/*
=====================
botSabot::botSabot
=====================
*/
botSabot::botSabot()
{
    travelFlags			= TFL_WALK|TFL_AIR|TFL_WALKOFFLEDGE|TFL_BARRIERJUMP|TFL_ELEVATOR|TFL_TELEPORT;
    // TFL_WALK|TFL_CROUCH|TFL_WALKOFFLEDGE|TFL_BARRIERJUMP|TFL_JUMP|TFL_LADDER|TFL_SWIM|TFL_WATERJUMP|TFL_TELEPORT|TFL_ELEVATOR|TFL_FLY|TFL_SPECIAL|TFL_WATER|TFL_AIR
}

/*
=====================
botSabot::~botSabot
=====================
*/
botSabot::~botSabot()
{
}

/*
=====================
botSabot::Think
TinMan: I think therefore I am stupid
=====================
*/
void botSabot::Think( void )
{
    if ( thinkFlags & TH_THINK )
    {
        //gameLocal.Printf( "--------- Botthink ----------\n" ); // TinMan: *debug*

        //gameLocal.Printf( "[Bot ClientId: %i]\n", clientID ); // TinMan: *debug*

        // TinMan: Clear previous input
        ClearInput();

        // TinMan: Update scriptvars
        GetBodyState();

        if ( playerEnt->spectating )   // TinMan: *todo* handle this diferently?
        {
            return;
        }

        if ( AI_DEAD )
        {
            if ( state && idStr::Icmp( state->Name(), "state_Killed" ) != 0 )   // TinMan: *todo* still a bit broken, keeps setting state, also will crash on maprestart (without if state check) if map has only one spawnpoint and two bots restart, state won't have been set at this point, maybe bot telefrags instantly and so is AI_DEAD before his first updatescript
            {
                state = GetScriptFunction( "state_Killed" );
                SetState( state );
            }

            UpdateScript();

            UpdateUserCmd();
        }
        else
        {
            // id: clear out the enemy when he dies or is hidden
            idActor *enemyEnt = enemy.GetEntity();
            if ( enemyEnt )
            {
                if ( enemyEnt->health <= 0 )
                {
                    EnemyDead();
                }
            }

            UpdateEnemyPosition();

            UpdateScript();

            UpdateViewAngles();

            UpdateUserCmd();
        }
    }
}

#endif
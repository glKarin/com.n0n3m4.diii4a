// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotAI::Enter_Run_And_Gun_Movement
================
*/
bool idBotAI::Enter_Run_And_Gun_Movement() {
 
	COMBAT_MOVEMENT_STATE = &idBotAI::Run_And_Gun_Movement;

	combatMoveType = RUN_N_GUN_ATTACK;

	lastMoveNode = "Run And Gun";

	return true;
}

/*
================
idBotAI::Run_And_Gun_Movement
================
*/
bool idBotAI::Run_And_Gun_Movement() {		//mal: like the name says, just a basic, run to the enemy and shoot him movement, occasionally jumping.

	float chaseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 500.0f : 300.0f;
	float tooCloseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 450.0f : 125.0f;

	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		if ( botInfo->weapInfo.isReloading ) {	//mal: what should the bot do while they're reloading their gun.
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		if ( botInfo->weapInfo.weapon == SNIPERRIFLE ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		if ( botInfo->weapInfo.weapon == KNIFE ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}
	}

	if ( enemyInfo.enemyDist > 1200.0f ) {
		Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );

        if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botThreadData.random.RandomInt( 100 ) > 80 ) ? RANDOM_JUMP : NULLMOVETYPE );
	} else if ( enemyInfo.enemyDist > chaseDist ) {

		if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
			if ( botThreadData.random.RandomInt( 100 ) > 90 && botInfo->weapInfo.isReloading != true ) {
				if ( Bot_CanProne( enemy ) && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
                    combatMoveFlag = PRONE;
				} else {
					combatMoveFlag = RUN;
				}				
			} else {
				combatMoveFlag = RUN;
			}
			combatMoveTime = botWorld->gameLocalInfo.time + 7000;
		}

		if ( combatMoveFlag == PRONE ) {
			Bot_MoveToGoal( vec3_zero, vec3_zero, PRONE, NULLMOVETYPE );
		} else {

            Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );

			if ( MoveIsInvalid() ) {
				Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
				Bot_ResetEnemy();
				return false;
			}
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( botThreadData.random.RandomInt( 100 ) > 80 ) ? RANDOM_JUMP : NULLMOVETYPE );
		}
	} else {
		if ( combatMoveFlag != PRONE ) {
			if ( enemyInfo.enemyDist < tooCloseDist ) {
                if ( Bot_CanMove( BACK, 100.0f, true )) {
  					Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
				} else {
					COMBAT_MOVEMENT_STATE = NULL;
				}
			}
		}
	}

	return true;
}

/*
================
idBotAI::Enter_Crazy_Jump_Attack_Movement
================
*/
bool idBotAI::Enter_Crazy_Jump_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Crazy_Jump_Attack_Movement;

	combatMoveType = CRAZY_JUMP_ATTACK;

	lastMoveNode = "Crazy Jump Attack";

	return true;
}

/*
================
idBotAI::Crazy_Jump_Attack_Movement
================
*/
bool idBotAI::Crazy_Jump_Attack_Movement() {

	float chaseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 900.0f : 500.0f;
	float tooCloseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 450.0f : 300.0f;

	if ( combatMoveFailedCount > 10 ) { //mal: move failed too many times, get out of here!
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

 	if ( enemyInfo.enemyDist < tooCloseDist ) {
        if ( Bot_CanMove( BACK, 150.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else if ( Bot_CanMove( RIGHT, 150.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else if ( Bot_CanMove( LEFT, 150.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else {
			combatMoveFailedCount++;
		}
	} else {
        if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
	        if ( combatMoveTime == -1 || combatMoveTime != 1 ) {
		        if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			} else {
				if ( combatMoveDir == LEFT ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			}
			combatMoveTime = botWorld->gameLocalInfo.time + 1700;		
		}

		if ( enemyInfo.enemyDist > chaseDist ) {
            if ( Bot_CanMove( combatMoveDir, 150.0f, true )) {
	
				Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );
			
				if ( MoveIsInvalid() ) {
					Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
					Bot_ResetEnemy();
					return false;
				}

				Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
			}
		} else {
			combatMoveTime = 1; //mal: move failed, try another!
			combatMoveFailedCount++;
		}
	}
	
	return true;
}

/*
================
idBotAI::Enter_Knife_Attack_Movement
================
*/
bool idBotAI::Enter_Knife_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Knife_Attack_Movement;

	combatMoveType = KNIFE_ATTACK;

	lastMoveNode = "Knife Attack";

	return true;
}

/*
================
idBotAI::Knife_Attack_Movement
================
*/
bool idBotAI::Knife_Attack_Movement() {

	if ( botInfo->weapInfo.weapon != KNIFE && !botWorld->gameLocalInfo.inWarmup ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( enemyInfo.enemyDist > 150.0f && enemyInfo.enemyFacingBot != false ) {
		if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
	        if ( combatMoveTime == -1 || combatMoveTime != 1 ) {
		        if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			} else {
				if ( combatMoveDir == LEFT ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			}

			combatMoveTime = botWorld->gameLocalInfo.time + 500;		
		}

		if ( Bot_CanMove( combatMoveDir, 50.0f, true )) {
            
			Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );
			
			if ( MoveIsInvalid() ) {
				Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
				Bot_ResetEnemy();
				return false;
			}

			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
		} else {
            combatMoveTime = 1; //mal: mark the move as having failed, so we know to try a diff dir next frame.
			combatMoveFailedCount++;
		}
	
		return true;
	}

//mal: hes close, or we're behind him, so just run up and stab the bastid!
	if ( enemyInfo.enemyDist > 65.0f ) {
        Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );


		if ( MoveIsInvalid() ) {
			Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
			Bot_ResetEnemy();
			return false;
		}
	
		Bot_MoveAlongPath( SPRINT );
	}

	return true;
}

/*
================
idBotAI::Enter_Prone_Attack_Movement
================
*/
bool idBotAI::Enter_Prone_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Prone_Attack_Movement;

	combatMoveType = PRONE_ATTACK;

	combatMoveFailedCount = 0;

	lastMoveNode = "Prone Attack";

	return true;
}

/*
================
idBotAI::Prone_Attack_Movement
================
*/
bool idBotAI::Prone_Attack_Movement() {

	idVec3 vec;

	if ( botInfo->weapInfo.weapon != SNIPERRIFLE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( !Bot_CanProne( enemy ) ) {
		combatMoveFailedCount++;

		if ( combatMoveFailedCount > MAX_MOVE_FAILED_COUNT ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}
	} else {
		combatMoveFailedCount = 0;
	}

	Bot_MoveToGoal( vec3_zero, vec3_zero, PRONE, NULLMOVETYPE );

	if ( combatDangerExists ) {
		Bot_SetupQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}

		return true;
	}

	if ( enemyInfo.enemyDist < 300.0f ) {
		if ( Bot_CanMove( BACK, 50.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
            COMBAT_MOVEMENT_STATE = NULL;
		}
		return false;
	}

	return true;
}

/*
================
idBotAI::Enter_Crouch_Attack_Movement
================
*/
bool idBotAI::Enter_Crouch_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Crouch_Attack_Movement;

	combatMoveType = CROUCH_ATTACK;

	combatMoveFailedCount = 0;

	lastMoveNode = "Crouch Attack";

	return true;
}

/*
================
idBotAI::Crouch_Attack_Movement
================
*/
bool idBotAI::Crouch_Attack_Movement() {

	int result;
	idVec3 vec;

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( !Bot_CanCrouch( enemy ) ) {
		combatMoveFailedCount++;

		if ( combatMoveFailedCount > MAX_MOVE_FAILED_COUNT ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}
	} else {
		combatMoveFailedCount = 0;
	}

	if ( botInfo->lastAttacker == enemy && botInfo->lastAttackerTime + 5000 > botWorld->gameLocalInfo.time && enemyInfo.enemyDist < 1500.0f && botInfo->weapInfo.weapon != SNIPERRIFLE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( combatDangerExists ) {
		Bot_SetupQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}

		return true;
	}

	if ( botInfo->weapInfo.weapon == SNIPERRIFLE ) {
		combatMoveDir = NULL_DIR;
	}

	if ( botInfo->weapInfo.isReloading && combatMoveTime < botWorld->gameLocalInfo.time && botInfo->weapInfo.weapon != SNIPERRIFLE ) {	//mal: what should the bot do while they're reloading their gun.
		if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
			combatMoveDir = LEFT;
		} else {
			combatMoveDir = RIGHT;
		}
		combatMoveTime = botWorld->gameLocalInfo.time + 5000;
	}

	if ( enemyInfo.enemyDist < 300.0f ) {
		if ( Bot_CanMove( BACK, 50.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
            COMBAT_MOVEMENT_STATE = NULL;
		}
		return false;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time && botInfo->weapInfo.weapon != SNIPERRIFLE ) {

		result = botThreadData.random.RandomInt( 3 );

		if ( result == 0 ) {
			combatMoveDir = RIGHT;
		} else if ( result == 1 ) {
			combatMoveDir = LEFT;
		} else {
			combatMoveDir = BACK; //mal: basically, do nothing, just crouch there.
		}

		combatMoveTime = botWorld->gameLocalInfo.time + 3000;
	}

	if ( combatMoveDir != NULL_DIR ) {
        if ( combatMoveDir == BACK ) {
			Bot_MoveToGoal( vec3_zero, vec3_zero, CROUCH, NULLMOVETYPE );
		} else {
			if ( Bot_CanMove( combatMoveDir, 100.0f, true ) ) {
				Bot_MoveToGoal( botCanMoveGoal, vec3_zero, CROUCH, ( combatMoveDir == RIGHT ) ? STRAFE_RIGHT : STRAFE_LEFT );
			} else {
				COMBAT_MOVEMENT_STATE = NULL;
			}
		}
	}

	return true;
}

/*
================
idBotAI::Enter_Circle_Strafe_Attack_Movement
================
*/
bool idBotAI::Enter_Circle_Strafe_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Circle_Strafe_Attack_Movement;

	combatMoveType = CIRCLE_STRAFE_ATTACK;

	lastMoveNode = "Circle Strafe";

	return true;
}

/*
================
idBotAI::Circle_Strafe_Attack_Movement
================
*/
bool idBotAI::Circle_Strafe_Attack_Movement() {

	if ( enemyInfo.enemyDist > 700.0f ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( enemyInfo.enemyHeight < -150 && enemyInfo.enemyDist > 700 ) { //mal: if we have the height advantage - stop moving!
		Bot_MoveToGoal( vec3_zero, vec3_zero, NULLMOVEFLAG, NULLMOVETYPE );
		return true;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
        if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            combatMoveDir = RIGHT;
		} else {
			combatMoveDir = LEFT;
		}
        combatMoveTime = botWorld->gameLocalInfo.time + 15000;		
	}

	if ( Bot_CanMove( combatMoveDir, 100.0f, true )) {
		Bot_MoveToGoal( vec3_zero, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? STRAFE_RIGHT : STRAFE_LEFT );
	} else {
		COMBAT_MOVEMENT_STATE = NULL;
	}

	return true;
}

/*
================
idBotAI::Enter_Side_Strafe_Attack_Movement
================
*/
bool idBotAI::Enter_Side_Strafe_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Side_Strafe_Attack_Movement;

	combatMoveType = SIDE_STRAFE_ATTACK;

	lastMoveNode = "Side Strafe";

	return true;
}

/*
================
idBotAI::Side_Strafe_Attack_Movement
================
*/
bool idBotAI::Side_Strafe_Attack_Movement() {

	float tooCloseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 450.0f : 300.0f;

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( enemyInfo.enemyDist < tooCloseDist ) {
		if ( Bot_CanMove( BACK, 100.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
            COMBAT_MOVEMENT_STATE = NULL;
		}
		return false;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time && combatMoveTime != -1 ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
        if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
            combatMoveDir = RIGHT;
		} else {
			combatMoveDir = LEFT;
		}

		combatMoveTime = botWorld->gameLocalInfo.time + 5000;
	}

	botMoveTypes_t botMove;

	if ( botThreadData.random.RandomInt( 100 ) > 60 ) {
		botMove = ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT;
	} else {
		botMove = ( combatMoveDir == RIGHT ) ? STRAFE_RIGHT : STRAFE_LEFT;
	}

	if ( Bot_CanMove( combatMoveDir, 50.0f, true )) {
		Bot_MoveToGoal( vec3_zero, vec3_zero, RUN, botMove );
	} else {
		combatMoveTime = 1; //mal: move failed - leave here. Side strafing is a one time deal...
	}

	return true;
}

/*
================
idBotAI::Enter_Hal_Strafe_Attack_Movement
================
*/
bool idBotAI::Enter_Hal_Strafe_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Hal_Strafe_Attak_Movement;

	combatMoveType = HAL_STRAFE_ATTACK;

	lastMoveNode = "Hal Strafe";

	return true;
}

/*
================
idBotAI::Hal_Strafe_Attak_Movement
================
*/
bool idBotAI::Hal_Strafe_Attak_Movement() {

	float chaseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 2500.0f : 1500.0f;
	float tooCloseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 450.0f : 300.0f;

	if ( botInfo->weapInfo.weapon == KNIFE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( enemyInfo.enemyDist < tooCloseDist ) {
		if ( Bot_CanMove( BACK, 100.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
            COMBAT_MOVEMENT_STATE = NULL;
		}
		return false;
	}

	if ( enemyInfo.enemyDist > chaseDist ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
		if ( combatMoveDir != RIGHT && combatMoveDir != LEFT ) {
            if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
				combatMoveDir = RIGHT;
			} else {
				combatMoveDir = LEFT;
			}
		} else {
			if ( combatMoveDir == RIGHT ) {
				combatMoveDir = LEFT;
			} else {
				combatMoveDir = RIGHT;
			}
		}
		combatMoveTime = botWorld->gameLocalInfo.time + 700;
	}

	if ( Bot_CanMove( combatMoveDir, 150.0f, true )) {
		Bot_MoveToGoal( vec3_zero, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? STRAFE_RIGHT : STRAFE_LEFT );
	} else {
		COMBAT_MOVEMENT_STATE = NULL;
	}

	return true;
}

/*
================
idBotAI::Enter_Stand_Ground_Attack_Movement
================
*/
bool idBotAI::Enter_Stand_Ground_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Stand_Ground_Attack_Movement;

	combatMoveType = STAND_GROUND_ATTACK;

	lastMoveNode = "Stand Ground";

	return true;
}

/*
================
idBotAI::Stand_Ground_Attack_Movement
================
*/
bool idBotAI::Stand_Ground_Attack_Movement() { 

	float tooCloseDist = ( botInfo->weapInfo.weapon == ROCKET ) ? 450.0f : 300.0f;
	idVec3 vec;

	if ( botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		if ( botInfo->weapInfo.weapon == KNIFE ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}
	}

	if ( combatDangerExists && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {
		Bot_SetupQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

		if ( MoveIsInvalid() ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		vec = botAAS.path.moveGoal - botInfo->origin;

		if ( vec.LengthSqr() > Square( 25.0f ) ) {
			Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, SPRINT, NULLMOVETYPE );
		}

		return true;
	}

	if ( botInfo->weapInfo.isReloading && botThreadData.GetBotSkill() > BOT_SKILL_EASY && botInfo->weapInfo.weapon != SNIPERRIFLE && botWorld->gameLocalInfo.botSkill != BOT_SKILL_DEMO ) {	//mal: what should the bot do while they're reloading their gun.
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( enemyInfo.enemyDist < tooCloseDist ) {
		if ( Bot_CanMove( BACK, 100.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, NULLMOVETYPE );
		} else {
            COMBAT_MOVEMENT_STATE = NULL;
		}
		return false;
	}

	return true;
}

/*
================
idBotAI::Enter_Grenade_Attack_Movement
================
*/
bool idBotAI::Enter_Grenade_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::Grenade_Attack_Movement;

	combatMoveType = GRENADE_ATTACK;

	lastMoveNode = "Grenade Attack";

	return true;
}

/*
================
idBotAI::Grenade_Attack_Movement
================
*/
bool idBotAI::Grenade_Attack_Movement() {

	float chaseDist = GRENADE_THROW_MAXDIST;
	float tooCloseDist = GRENADE_THROW_MINDIST;

	if ( combatMoveFailedCount > 10 ) { //mal: move failed too many times, get out of here!
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( botInfo->weapInfo.weapon != NADE ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

 	if ( enemyInfo.enemyDist < tooCloseDist ) {
        if ( Bot_CanMove( BACK, 150.0f, true )) {
  			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else if ( Bot_CanMove( RIGHT, 150.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else if ( Bot_CanMove( LEFT, 150.0f, true )) {
			Bot_MoveToGoal( botCanMoveGoal, vec3_zero, RUN, RANDOM_JUMP );
		} else {
			combatMoveFailedCount++;
		}
	} else {
        if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
	        if ( combatMoveTime == -1 || combatMoveTime != 1 ) {
		        if ( botThreadData.random.RandomInt( 100 ) > 50 ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			} else {
				if ( combatMoveDir == LEFT ) {
					combatMoveDir = RIGHT;
				} else {
					combatMoveDir = LEFT;
				}
			}
			combatMoveTime = botWorld->gameLocalInfo.time + 1700;		
		}

		if ( enemyInfo.enemyDist > chaseDist ) {
            if ( Bot_CanMove( combatMoveDir, 150.0f, true ) ) {
	
				Bot_SetupMove( vec3_zero, enemy, ACTION_NULL );
			
				if ( MoveIsInvalid() ) {
					Bot_IgnoreEnemy( enemy, ENEMY_IGNORE_TIME ); //mal: no valid path to this client for some reason - ignore him for a while
					Bot_ResetEnemy();
					return false;
				}

				Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
			} else {
				combatMoveTime = 1; //mal: move failed, try another!
				combatMoveFailedCount++;
			}
		} else {
			Bot_MoveToGoal( vec3_zero, vec3_zero, RUN, ( combatMoveDir == RIGHT ) ? RANDOM_JUMP_RIGHT : RANDOM_JUMP_LEFT );
		}
	}
	
	return true;
}

/*
================
idBotAI::Enter_Avoid_Danger_Movement
================
*/
bool idBotAI::Enter_Avoid_Danger_Movement() {
 
	COMBAT_MOVEMENT_STATE = &idBotAI::Avoid_Danger_Movement;

	combatMoveType = AVOID_DANGER_ATTACK;

	lastMoveNode = "Avoid Danger";

	combatMoveTime = botWorld->gameLocalInfo.time + 4000;

	return true;
}

/*
================
idBotAI::Avoid_Danger_Movement
================
*/
bool idBotAI::Avoid_Danger_Movement() {
	Bot_SetupQuickMove( botInfo->origin, false ); //mal: just path to itself, if its in an obstacle, it will freak and avoid it.

	if ( MoveIsInvalid() ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( combatMoveTime < botWorld->gameLocalInfo.time ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
	return true;
}

/*
================
idBotAI::Enter_Null_Move_Attack
================
*/
bool idBotAI::Enter_Null_Move_Attack() {
 
	COMBAT_MOVEMENT_STATE = &idBotAI::Null_Move_Attack;

	combatMoveType = NULL_MOVE_ATTACK;

	lastMoveNode = "Null Move";

	return true;
}

/*
================
idBotAI::Null_Move_Attack

This was one of the harder, most complex functions ever written, but somehow.... I managed. :-P
================
*/
bool idBotAI::Null_Move_Attack() {
	return true;
}

/*
================
idBotAI::Enter_MoveTo_Shield_Attack_Movement
================
*/
bool idBotAI::Enter_MoveTo_Shield_Attack_Movement() {

	COMBAT_MOVEMENT_STATE = &idBotAI::MoveTo_Shield_Attack_Movement;

	combatMoveType = MOVETO_SHIELD_ATTACK;

	lastMoveNode = "MoveTo Shield";

	return true;
}

/*
================
idBotAI::MoveTo_Shield_Attack_Movement
================
*/
bool idBotAI::MoveTo_Shield_Attack_Movement() {

	if ( !botInfo->weapInfo.isReloading && botInfo->weapInfo.isReady ) {
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	idVec3 shieldOrg;

	float shieldDistSqr = Bot_DistSqrToClosestForceShield( shieldOrg );

	if ( shieldDistSqr == -1.0f || shieldDistSqr > Square( SHIELD_CONSIDER_RANGE * 2.0f ) ) { //mal: make sure the shield didn't die, or that theres not another close one nearby...
		COMBAT_MOVEMENT_STATE = NULL;
		return false;
	}

	if ( shieldDistSqr > Square( 45.0f ) ) {
		Bot_SetupMove( shieldOrg, -1, ACTION_NULL );

		if ( MoveIsInvalid() ) {
			COMBAT_MOVEMENT_STATE = NULL;
			return false;
		}

		Bot_MoveToGoal( botAAS.path.moveGoal, vec3_zero, RUN, NULLMOVETYPE );
	}

	return true;
}
























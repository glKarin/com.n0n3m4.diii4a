// Bot_Input.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

/*
==============
rvmBot::BotInputToUserCommand
==============
*/
void rvmBot::BotInputToUserCommand(bot_input_t* bi, usercmd_t* ucmd, int time)
{
	idVec3 forward, right;

	short temp;
	int j;

	//clear the whole structure
//	memset(ucmd, 0, sizeof(usercmd_t));
	//
	//common->Printf("dir = %f %f %f speed = %f\n", bi->dir[0], bi->dir[1], bi->dir[2], bi->speed);
	//the duration for the user command in milli seconds
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP)
	{
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN)
	{
		ucmd->buttons = BUTTON_ATTACK;
	}
	if (bi->actionflags & ACTION_ATTACK)
	{
		ucmd->buttons |= BUTTON_ATTACK;
	}
	//if (bi->actionflags & ACTION_TALK) ucmd->buttons |= BUTTON_TALK;
	//if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	//if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK)
	{
		ucmd->buttons |= BUTTON_RUN;
	}
	//if (bi->actionflags & ACTION_AFFIRMATIVE) ucmd->buttons |= BUTTON_AFFIRMATIVE;
	//if (bi->actionflags & ACTION_NEGATIVE) ucmd->buttons |= BUTTON_NEGATIVE;
	//if (bi->actionflags & ACTION_GETFLAG) ucmd->buttons |= BUTTON_GETFLAG;
	//if (bi->actionflags & ACTION_GUARDBASE) ucmd->buttons |= BUTTON_GUARDBASE;
	//if (bi->actionflags & ACTION_PATROL) ucmd->buttons |= BUTTON_PATROL;
	//if (bi->actionflags & ACTION_FOLLOWME) ucmd->buttons |= BUTTON_FOLLOWME;
	//
	ucmd->impulse |= bi->weapon;
	if (bi->lastWeaponNum != bi->weapon)
	{
		//ucmd->flags = UCF_IMPULSE_SEQUENCE;
		bi->lastWeaponNum = bi->weapon;
	}
	else
	{
		//ucmd->flags = 0;
	}

	idAngles botViewAngles = viewAngles;

	{
		int i;
		float move;
		float angMod = (1.0f / 12.0f);

		for (i = 0; i < 3; i++) {
			move = idMath::AngleDelta(bi->viewangles[i], botViewAngles[i]);
			botViewAngles[i] += (move * angMod);
		}
	}

	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[0] = ANGLE2SHORT(botViewAngles[0] - deltaViewAngles[0]);
	ucmd->angles[1] = ANGLE2SHORT(botViewAngles[1] - deltaViewAngles[1]);
	ucmd->angles[2] = ANGLE2SHORT(botViewAngles[2] - deltaViewAngles[2]);

	bi->viewangles.ToVectors(&forward, &right, NULL);

	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	ucmd->forwardmove = idMath::ClampChar(DotProduct(forward, bi->dir) * bi->speed);
	ucmd->rightmove = idMath::ClampChar(DotProduct(right, bi->dir) * bi->speed);
	//ucmd->upmove = abs(forward[2]) * bi->dir[2] * bi->speed;

	//normal keyboard movement
	if (bi->actionflags & ACTION_MOVEFORWARD)
	{
		ucmd->forwardmove += 127;
	}
	if (bi->actionflags & ACTION_MOVEBACK)
	{
		ucmd->forwardmove -= 127;
	}
	if (bi->actionflags & ACTION_MOVELEFT)
	{
		ucmd->rightmove -= 127;
	}
	if (bi->actionflags & ACTION_MOVERIGHT)
	{
		ucmd->rightmove += 127;
	}

	//jump/moveup
	//if (bi->actionflags & ACTION_JUMP)
	//	ucmd->buttons |= BUTTON_JUMP;

	//	ucmd->upmove += 127;
	//
	////crouch/movedown
	//if (bi->actionflags & ACTION_CROUCH)
	//	ucmd->upmove -= 127;
	//
	//Com_Printf("forward = %d right = %d up = %d\n", ucmd.forwardmove, ucmd.rightmove, ucmd.upmove);
	//Com_Printf("ucmd->serverTime = %d\n", ucmd->serverTime);

	if( bi->respawn )
	{
		ucmd->buttons |= BUTTON_ATTACK;
	}
}

/*
================
rvmBot::ResetUcmd
================
*/
void rvmBot::Bot_ResetUcmd( usercmd_t& ucmd )
{
	ucmd.forwardmove = 0;
	ucmd.rightmove = 0;
	//ucmd.upmove = 0;
	ucmd.impulse = 0;
	//ucmd.flags = 0;
	memset( &ucmd.buttons, 0, sizeof( ucmd.buttons ) );
}


/*
========================
rvmBot::BotInputFrame
========================
*/
void rvmBot::BotInputFrame( void )
{
	usercmd_t& ucmd = (usercmd_t &)gameLocal.usercmds[entityNumber];

	ucmd.gameTime = gameLocal.time;
	ucmd.gameFrame = gameLocal.framenum;
	ucmd.duplicateCount = 0;

	Bot_ResetUcmd(ucmd);
	BotInputToUserCommand( &bs.botinput, &ucmd, gameLocal.time );

	// pass the bot's user cmds off to the engine, so that other clients can predict this bot
	networkSystem->ServerSetBotUserCommand(entityNumber, gameLocal.framenum, ucmd);
}

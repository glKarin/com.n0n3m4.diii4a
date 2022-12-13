

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#ifndef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
CLASS_DECLARATION(hhMonsterAI, hhMutate)
	EVENT( MA_OnProjectileLaunch,	hhMutate::Event_OnProjectileLaunch )
END_CLASS

void hhMutate::Event_OnProjectileLaunch(hhProjectile *proj) {
	const function_t *newstate = NULL;
	if ( !enemy.IsValid() || !AI_COMBAT ) {
		return;
	}

	float min = spawnArgs.GetFloat( "dda_dodge_min", "0.3" );
	float max = spawnArgs.GetFloat( "dda_dodge_max", "0.8" );
	float dodgeChance = 0.6f;

	dodgeChance = (min + (max-min)*gameLocal.GetDDAValue() );

	if ( ai_debugBrain.GetBool() ) {
		gameLocal.Printf( "%s dodge chance %f\n", GetName(), dodgeChance );
	}
	if ( gameLocal.random.RandomFloat() > dodgeChance ) {
		return;
	}

	//determine which side to dodge to
	idVec3 povPos, targetPos;
	povPos = enemy->GetPhysics()->GetOrigin();
	targetPos = GetPhysics()->GetOrigin();
	idVec3 povToTarget = targetPos - povPos;
	povToTarget.z = 0.f;
	povToTarget.Normalize();
	idVec3 povLeft, povUp;
	povToTarget.OrthogonalBasis(povLeft, povUp);
	povLeft.Normalize();

	idVec3 projVel = proj->GetPhysics()->GetLinearVelocity();
	projVel.Normalize();
	float dot = povLeft * projVel;
	if ( dot < 0 ) { 
		newstate = GetScriptFunction( "state_DodgeBackRight" );
	} else {
		newstate = GetScriptFunction( "state_DodgeBackLeft" );
	}

	if ( newstate ) {
		SetState( newstate );
		UpdateScript();
	}
}

#define LinkScriptVariable( name )	name.LinkTo( scriptObject, #name )
void hhMutate::LinkScriptVariables(void) {
	hhMonsterAI::LinkScriptVariables();
	LinkScriptVariable( AI_COMBAT );
}
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
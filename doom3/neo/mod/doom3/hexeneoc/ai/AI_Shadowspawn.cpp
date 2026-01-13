
#pragma hdrstop

#include "Game_local.h"

const int SHADOWSPAWN_RANGE_ATTACK_RATE		= 3;
const int SHADOWSPAWN_DRAIN_RATE			= 10;

#if 0
CLASS_DECLARATION( idAI, idAI_Shadowspawn )

END_CLASS

void idAI_Shadowspawn::regenerate() {
	float drainTime=0;
	idVec3 dir;
	float giveUp=DelayTime(5);

//	// regenerate 2 health every second
//	if ( regenTime > ( gameLocal.time / 1000 ) && getHealth() < getFloat("health") ) {
//		setHealth(getHealth() + 1);
//		regenTime = delaytime(1);
//	}

	//vec.Length(GetOrigin() - enemy.GetEyePos()) < 80

	if ( !enemy ) {
		return;
	}

	SetAnimState( ANIMCHANNEL_TORSO, "Torso_TelePull", 4 );
	stayPut=getWorldOrigin(); // shadowspawn should not be moved by projectiles, etc, might put player outside of level
	while ( 1 ) {
		setWorldOrigin(stayPut);

		if ( vec.Length(stayPut - enemy->GetPhysics()->GetOrigin()()) < 90 ) {
			break;
		} else if (( gameLocal.time / 1000 ) > giveUp) {
			SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 4 );
			return;
		}

		enemy->GetPhysics()->SetLinearVelocity( sys.vecNormalize(stayPut - enemy->GetPhysics()->GetOrigin()()) * 80);
		waitFrame();
	}

	
	enemy.bind(me);
	//enemyStayPut = enemy.getWorldOrigin();
	float tim=DelayTime(2);

	//vector turny = getAngles();

	SetAnimState( ANIMCHANNEL_TORSO, "Torso_AttackDrain", 4 );
	while ( 1 ) {
		setWorldOrigin(stayPut); // shadowspawn should not be moved by projectiles, etc, might put player outside of level
		//enemy.setWorldOrigin(enemyStayPut);
		//turnTo(turny.y + 180);
		if (drainTime < ( gameLocal.time / 1000 )) {
			drainTime = DelayTime(1);
			directDamage(enemy, "damage_shadowspawn_drain");
			setHealth(getHealth()+100);
		}

		if ( tim < ( gameLocal.time / 1000 ) ) {
			SetAnimState( ANIMCHANNEL_TORSO, "Torso_Repulse", 4 );
			waitAction( "repulse" );
			enemy.unbind();
			dir = sys.vecNormalize(enemy->GetPhysics()->GetOrigin()() - stayPut) * 1180;
			dir.z = 0;
			enemy->GetPhysics()->SetLinearVelocity( dir );
			break;
		}
		waitFrame();
	}
}

void idAI_Shadowspawn::telleport() {
	float	tDelay;
	idVec3	pV;
	float	dist;
	float	ang;
	idVec3	enemyOrigin = enemy->GetPhysics()->GetOrigin()();

	nextTelleport = ( gameLocal.time / 1000 ) + sys.random(4)+7;

	// lunge at player and disappear
	pV = sys.vecNormalize(enemy.GetEyePos() - GetOrigin());
	turnToEntity(enemy);
	tDelay = 1 + ( gameLocal.time / 1000 );
	GetPhysics()->SetLinearVelocity(pV * 1066);

	while (( gameLocal.time / 1000 ) < tDelay && vec.Length(GetOrigin() - enemy.GetEyePos()) > 20) {
		waitFrame();
	}

	hide();

	// delay
	tDelay = sys.random(3) + ( gameLocal.time / 1000 );
	while (( gameLocal.time / 1000 ) < tDelay) {
		waitFrame();
	}

	// reappear
	ang = sys.random(360);
	dist = sys.random(350)+150;
	pV = enemy.GetEyePos();
	pV.x = pV.x + dist * sys.sin(ang);
	pV.y = pV.y + dist * sys.cos(ang);
	//pV.z = enemyOrigin.z;
	SetOrigin(pV);
	show();

	doTelleport = false;
}
#endif

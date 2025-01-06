// Monster_Pigcop.h
//

#pragma once

enum LIZTROOP_IDLE_STATE
{
	LIZTROOP_IDLE_WAITINGTPLAYER = 0,
	LIZTROOP_IDLE_ROAR
};

//
// DnLiztroop
//
class DnLiztroop : public DnAI
{
	CLASS_PROTOTYPE(DnLiztroop);
public:
	stateResult_t				state_Begin(stateParms_t* parms);
	stateResult_t				state_Idle(stateParms_t* parms);
	stateResult_t				state_ApproachingEnemy(stateParms_t* parms);
	stateResult_t				state_ShootEnemy(stateParms_t* parms);
	stateResult_t				state_BeginDeath(stateParms_t* parms);
	stateResult_t				state_Killed(stateParms_t* parms);
private:
	const idSoundShader* troop_awake;
	const idSoundShader* fire_sound;
	const idSoundShader* death_sound;
};
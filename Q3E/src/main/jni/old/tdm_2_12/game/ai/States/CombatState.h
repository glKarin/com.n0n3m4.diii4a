/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __AI_COMBAT_STATE_H__
#define __AI_COMBAT_STATE_H__

#include "../AI.h"
#include "State.h"

namespace ai
{

#define STATE_COMBAT "Combat"

class CombatState :
	public State
{
protected:
	// The AI's enemy
	idEntityPtr<idActor> _enemy;
	//int _criticalHealth;
	bool _meleePossible;
	bool _rangedPossible;
	bool _unarmedMelee;		// grayman #3331
	bool _unarmedRanged;	// grayman #3331
	bool _armedMelee;		// grayman #3331
	bool _armedRanged;		// grayman #3331

	ECombatType _combatType;

	int _endTime;

	bool _endgame; // grayman #3848
	idVec3 _destination; // grayman #3848

	int _reactionEndTime; // grayman #3063

	// grayman #3331 - time to wait before proceeding with a state
	int _waitEndTime;

	// grayman #3331 - the first time you draw your weapon, you might need a small delay
	bool _needInitialDrawDelay;

	// grayman #3331 - keep track of whether you just drew a weapon
	bool _justDrewWeapon;
	
	// grayman #3063 - break Combat State into smaller bits
	// grayman #3331 - new state design

	enum ECombatSubState
	{
		EStateReaction,
		EStateDoOnce,
		EStateCheckWeaponState,
		EStateSheathingWeapon,
		EStateDrawWeapon,
		EStateDrawingWeapon,
		EStateCombatAndChecks,

		// grayman #3848
		EStateVictor1,
		EStateVictor2,
		EStateVictor3,
		EStateVictor4,
		EStateVictor5,
		EStateVictor6,
		EStateVictor7,
		EStateVictor8,
		EStateVictor9
	} _combatSubState;

public:
	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	// Override the alert functions
	virtual void OnTactileAlert(idEntity* tactEnt) override;
	virtual void OnVisualAlert(idActor* enemy) override;
	virtual bool OnAudioAlert(idStr soundName, bool addFuzziness, idEntity* maker) override; // grayman #3847 // grayman #3857

	virtual void OnActorEncounter(idEntity* stimSource, idAI* owner) override;
	virtual void OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead) override;

	// grayman #3317 - It's possible that an AI will enter Combat mode after encountering a
	// dead or KO'ed AI, but before the post processing for those events occurs. If that happens,
	// these methods will catch the post processing and abort it.
	virtual void Post_OnDeadPersonEncounter(idActor* person, idAI* owner) override;
	virtual void Post_OnUnconsciousPersonEncounter(idActor* person, idAI* owner) override;

	virtual void OnBlindStim(idEntity* stimSource, bool skipVisibilityCheck) override; // grayman #3431
	virtual void OnVisualStimBlood(idEntity* stimSource, idAI* owner) override; // grayman #3857
	
	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	static StatePtr CreateInstance();

protected:
	// Override base class method
	virtual bool CheckAlertLevel(idAI* owner) override;

	virtual void DelayedVisualStim( idEntity* stimSource, idAI* owner) override; // grayman #2924

	// Checks enemy status (dead, visible, not an enemy anymore).
	// Returns false if the enemy is not applicable anymore and the state has ended
	bool CheckEnemyStatus(idActor* enemy, idAI* owner);
};

} // namespace ai

#endif /* __AI_COMBAT_STATE_H__ */

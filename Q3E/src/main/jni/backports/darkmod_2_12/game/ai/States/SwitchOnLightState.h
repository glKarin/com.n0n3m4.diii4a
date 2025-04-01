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

#ifndef __AI_SWITCH_ON_LIGHT_H__
#define __AI_SWITCH_ON_LIGHT_H__

#include "State.h"

namespace ai
{

#define STATE_SWITCH_ON_LIGHT "SwitchOnLight"

class SwitchOnLightState :
	public State
{
private:
	// Default constructor
	SwitchOnLightState();

	idEntityPtr<idLight> _light;

	// time to wait after starting anim before the light is switched on
	int _waitEndTime;

	idEntity* _goalEnt;		// grayman #2603 - entity to walk toward when relighting a light
	float _standOff;		// grayman #2603 - get this close to relight
	idVec3 _relightSpot;	// grayman #2603 - where to stand to relight

	enum ERelightState	// grayman #2603
	{
		EStateSitting,
		EStateStarting,
		EStateApproaching,
		EStateTurningToward,
		EStateRelight,
		EStatePause,
		EStateFinal
	} _relightState;

public:
	// Constructor using light source as input parameter
	SwitchOnLightState(idLight* light);

	// Get the name of this state
	virtual const idStr& GetName() const override;

	// This is called when the state is first attached to the AI's Mind.
	virtual void Init(idAI* owner) override;

	// Gets called each time the mind is thinking
	virtual void Think(idAI* owner) override;

	// This is called when a State is destroyed
	virtual void Cleanup(idAI* owner) override; // grayman #3559
	
	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	static StatePtr CreateInstance();

private:
	float GetMaxReach(idAI* owner, idEntity* torch, idStr lightType); // grayman #2603

	//bool GetSwitchGoal(idAI* owner, CBinaryFrobMover* mySwitch, idVec3 &target); // grayman #2603

	bool CheckRelightPosition(idLight* light, idAI* owner, idVec3& pos); // grayman #2603

	void StartSwitchOn(idAI* owner, idLight* light); // Start switching on (stop move, start anim)

	void Wrapup(idAI* owner, /*idLight* light,*/ bool ignore); // grayman #2603
};

} // namespace ai

#endif /* __AI_SWITCH_ON_LIGHT_H__ */

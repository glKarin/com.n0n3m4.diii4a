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

#ifndef __GAME_BUTTON_STATE_TRACKER_H__
#define __GAME_BUTTON_STATE_TRACKER_H__

#include <map>

/**
* greebo: The ButtonStateTracker is a helper class keeping track
* of certain buttons. As soon as a tracked button is released, 
* this calls the method PerformButtonRelease() on the idPlayer class.
*
* Use the StartTracking() method to register an impulse for tracking.
*/

// Forward declaration
class idPlayer;

class ButtonStateTracker
{
private:
	// The class that is going to be notified on button release
	idPlayer* _owner;

	// ButtonTimeTable: maps buttons => hold time
	typedef std::map<int, int> ButtonHoldTimeMap;

	ButtonHoldTimeMap _buttons;

	int _lastCheckTime;

public:
	// Constructor, pass the owning, to-be-notified entity here
	ButtonStateTracker(idPlayer* owner);

	/**
	* greebo: Call this from the Think() method so that 
	* this class can update the state of the tracked buttons.
	*/
	void Update();

	/**
	* greebo: Register an impulse for tracking to consider it
	* during the update() routine. Released buttons get 
	* automatically de-registered, so no stopTracking() call is needed.
	*/
	void StartTracking(int impulse);

	/**
	* greebo: Returns TRUE if the given impulse button is currently 
	* held down by the user, FALSE otherwise.
	*/
	bool ButtonIsHeld(int impulse);

	/**
	* greebo: De-register an impulse for tracking. Calling this method 
	* does not trigger a PerformKeyReleased() event on the player,
	* so this is more or less a "cancellation".
	*/
	void StopTracking(int impulse);

}; // class ButtonStateTracker

#endif /* __GAME_BUTTON_STATE_TRACKER_H__ */

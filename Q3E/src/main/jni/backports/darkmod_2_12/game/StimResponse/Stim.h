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
#ifndef SR_STIM__H
#define SR_STIM__H

#include "StimResponse.h"
#include "StimResponseTimer.h"

/**
 * CStim is a base class for the stims. The constructor and destructors
 * are declared protected so that only the collection can actually create
 * destroy them.
 */
class CStim : 
	public CStimResponse
{
	friend class CStimResponseCollection;

protected:
	CStim(idEntity* owner, StimType type, int uniqueId);

public:
	virtual ~CStim();

	virtual void Save(idSaveGame *savefile) const override;
	virtual void Restore(idRestoreGame *savefile) override;

	/**
	 * Add a responseentity to the ignore list. If the response is already
	 * in the list, it is not entered again.
	 */
	void AddResponseIgnore(idEntity *);
	void RemoveResponseIgnore(idEntity *);
	bool CheckResponseIgnore(const idEntity *) const;

	void ClearResponseIgnoreList();

	/**
	 * greebo: Returns the current radius value. Note that stims can have
	 * their radius changing over time. This calculates the current value
	 * based on the timestamps and returns it.
	 */
	float GetRadius();

	/**
	* greebo: This adds/removes the stim timer to/from the list 
	*		  maintained by class gameLocal
	*/
	CStimResponseTimer* AddTimerToGame();
	void RemoveTimerFromGame();

	CStimResponseTimer *GetTimer() { return &m_Timer; };

protected:
	/**
	 * Timer for the stimulus. If no timer is set, then it is assumed
	 * that this stimulus is always working whenever it is applied.
	 */
	CStimResponseTimer	m_Timer;

public:
	/**
	 * This is the list of all responses that should be ignored for this stim.
	 * This is required for stims, which have a lifespan during which they
	 * can fire. Each response would fire on each frame as long as the stim is
	 * enabled. This is not really useful in most cases, so we can add a response,
	 * which already has fired, to the ignorelist. Until the response is removed
	 * it will no longer fire.
	 */
	idList< idEntityPtr<idEntity> >		m_ResponseIgnore;

	/**
	* If set to true, the stim uses the entity's bounds in the stim intersection test.
	* This makes it possible to get accurate stims with non-cube objects.
	* Note that radius further expands on these bounds.
	**/
	bool					m_bUseEntBounds;

	/**
	* If set to true, this stim is a collision-based stim and only checked when the physics 
	* clipmodel of the entity it's on collide with another entity.  
	* It is not checked unless it collides, so the entity has to be one that collides.
	**/
	bool					m_bCollisionBased;
	/**
	* Collision stim info:
	* m_bCollisionFired is set to true if the collision happened but stim is not processed yet
	* Reset in gameLocal::ProcessStimResponse
	**/
	bool					m_bCollisionFired;
	/**
	* List of entities with responses that the stimming entity collided with this frame
	* Reset in gameLocal::ProcessStimResponse
	**/
	idList<idEntity *>		m_CollisionEnts;

	/**
	* If set to true, this stim is a script-driven stim
	* and only checked after script calls Event_StimEmit
	**/
	bool					m_bScriptBased;
	/**
	* m_bScriptFired is set to true if Event_StimEmit was called but not processed yet
	* Reset in gameLocal::ProcessStimResponse
	**/
	bool					m_bScriptFired;
	/**
	* Radius override passed to Event_StimEmit (negative means none)
	* Reset in gameLocal::ProcessStimResponse
	**/
	float					m_ScriptRadiusOverride;
	/**
	* Position (origin) override passed to Event_StimEmit (exactly zero means none)
	* Reset in gameLocal::ProcessStimResponse
	**/
	idVec3					m_ScriptPositionOverride;

	/**
	* Milliseconds between interleaving for use with frame-based timer check (not StimTimer)
	**/
	int						m_TimeInterleave;

	/**
	* Timestamp used with time interleaving code.
	**/
	int						m_TimeInterleaveStamp;

	/**
	* greebo: The counter specifying how often the stim can be
	*		  be fired before it gets disabled.
	*		  Set this to -1 to allow infinite firing (default).
	*/
	int						m_MaxFireCount;

	/**
	 * Radius defines the radius the action can reach out
	 */
	float				m_Radius;

	/** 
	 * greebo: The final radius the stim is reaching after its duration time.
	 * A negative value is considered invalid (i.e. final radius is not set).
	 */
	float				m_RadiusFinal;

	/**
	* greebo: The stim bounds of this stim (can be used as alternative to
	*		  the stim radius). This has to be defined by passing two 
	*		  vectors via the spawnargs.
	*/
	idBounds			m_Bounds;

	/**
	* greebo: This is the velocity of this stim in units per second.
	*		  This affects the center of the stim only, not the entity that is
	*		  carrying this stim. This can be used to simulate more realistic
	*		  water arrows (where the water affects items below the impact after time).
	*/
	idVec3				m_Velocity;

	/**
	* Magnitude of stim, per stim firing.  This can be damage if the stim does damage,
	*	or healing if it's a healing stim, or more abstract things like amount of water,
	*	amount of energy transferred by heat, etc.
	**/
	float				m_Magnitude;

	/**
	* The Falloff shape of the magnitude in dependence of the distance.
	* 0 = constant (homogeneous) - magnitude is the same for all
	* 1 = linear
	* 2 = quadratic
	* 3 = etc..
	*/
	int					m_FallOffExponent;

	/**
	 * Defines the maximum number responses that can be applied to this particular
	 * stimulus at any given time. 0 means unlimited and is the default.
	 */
	int					m_MaxResponses;
	int					m_CurResponses;		// Already active responses

	/**
	 * Applytimer defines the number of times the stim can be used before the stim starts
	 * it's timer. For example a machinegun can be used for forty rounds before
	 * it has to cool down a certain time. 0 is unlimited.
	 */
	int					m_ApplyTimer;
	int					m_ApplyTimerVal;

	/**
	* This virtual member is called after the stimulus has been fired to allow the stim
	* to adjust itself according to any stim class specific logic.
	*
	* @param numResponses The number of responses triggered by the stim.  It may be 0 to
	* indicate there were no active responders present.
	*/
	virtual void PostFired (int numResponses);
};
typedef std::shared_ptr<CStim> CStimPtr;

#endif /* SR_STIM__H */

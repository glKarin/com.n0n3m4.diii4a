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
#ifndef SR_STIMRESPONSE__H
#define SR_STIMRESPONSE__H

#include "StimType.h"

extern const char *cStimType[];

enum StimState
{
	SS_DISABLED,		// Stim is disabled and can not be triggered
	SS_ENABLED,			// Stim is enabled and waits for activation
	SS_DEFAULT
};

/**
 * CStimResponse is the baseclass for stims and responses
 */
class CStimResponse {

	friend class CStimResponseCollection;

protected:
	CStimResponse(idEntity* owner, StimType type, int uniqueId, bool isStim);

public:
	virtual void Save(idSaveGame *savefile) const;
	virtual void Restore(idRestoreGame *savefile);

	void SetEnabled(bool enabled = true);

	// Shortcuts to SetEnabled
	void Enable() { SetEnabled(true); }
	void Disable() { SetEnabled(false); }

	/**
	 * greebo: Returns the unique ID used to identify this S/R after map load.
	 */
	int	GetUniqueId() const;

	/** 
	* greebo: This evaluates the m_Chance member variable against a random float.
	*
	* @returns: TRUE, if the S/R passed the check and can be fired
	*/
	bool CheckChance();

	/**
	* greebo: This retrieves the stim type id for the given stimName.
	*
	* @stimName: The name of the stim (e.g. "STIM_THIEF")
	* @returns: the according StimType (if the name is known), or ST_DEFAULT (== -1) if unknown
	*/
	static StimType GetStimType(const idStr& stimName);

public:
	// A unique ID as assigned by the StimResponseCollection. Used to identify 
	// this stim after map load.
	int					m_UniqueId;

	/**
	 * Id for the stimulus that uniquely identifies a stim, so they can
	 * be associated to each other.
	 */
	StimType			m_StimTypeId;

	/**
	 * stgatilov: true for CStim, false for CStimResponse
	 * (aka RTTI helper)
	 */
	bool				m_IsStim;

	// This is only populated with the Id as used in the entity definition. We
	// store the name here to reference the script action key.
	idStr				m_StimTypeName;

	/**
	 * If set to true, then the stim can be removed from an entity. This is mistly needed
	 * for an external app lication later on, so that the defauls can not be accidently
	 * removed.
	 */
	bool				m_Removable;

	/**
	 * State for the stim/response.
	 */
	StimState			m_State;

	/**
	 * Defines the chance that this stim/response works. Whenever the S/R is activated
	 * the chance determines whether it will do its job.
	 */
	float				m_Chance;

	/**
	 * Whenever the chance test failed, and the stim has a timer before it can be 
	 * reused again, the ChanceTimer determines whether the timer should be used (true)
	 * or not (default = -1).
	 * This can be used to create a stim that has a chance of failure but needs time 
	 * to recharge, bevore it can be used again, but the reuse timer may not always 
	 * be desirable to be applied.
	 */
	int					m_ChanceTimer;

	/**
	* greebo: This is the earliest time a next chance can be evaluated.
	*		  If a previous chance test failed, the next chance time is calculated.
	*		  If a previous chance test was passed, the time is set to -1;
	*/
	int					m_NextChanceTime;

	/**
	 * Default means that this is a stim which has been added as default to this entity.
	 * Thiw would also mainly be used for an editor.
	 */
	bool				m_Default;

	/**
	* Timestamp for stims/responses with finite duration after they're enabled (milliseconds)
	**/
	int						m_EnabledTimeStamp;

	/**
	* Stim or response duration after being enabled (in milliseconds).  
	* SR will automatically disable itself after this time.
	**/
	int						m_Duration;

	idEntityPtr<idEntity>	m_Owner;
};
typedef std::shared_ptr<CStimResponse> CStimResponsePtr;

#endif /* SR_STIMRESPONSE__H */

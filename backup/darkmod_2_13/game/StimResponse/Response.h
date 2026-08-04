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
#ifndef SR_RESPONSE__H
#define SR_RESPONSE__H

#include "StimResponse.h"

#include "ResponseEffect.h"
class CStim;

class CResponse : public CStimResponse {
	
	friend class CStimResponseCollection;

protected:
	CResponse(idEntity* owner, StimType type, int uniqueId);	

public:
	virtual ~CResponse();

	virtual void Save(idSaveGame *savefile) const override;
	virtual void Restore(idRestoreGame *savefile) override;

	/**
	* This method is called when the response should
	* make its script callback. It is virtual
	* so that the container can reach overriden
	* versions from a CStimResponse base pointer.
	*
	* @sourceEntity: This is the entity carrying the stim
	* @stim: This is the stim to retrieve stim properties like magnitude, etc.
	*		 This is an optional argument, pass NULL to fire responses without
	*		 a "real" stim (e.g. frobbing)
	*/
	virtual void TriggerResponse(idEntity *sourceEntity, const CStimPtr& stim = CStimPtr());

	/**
	 * Set the response script action.
	 */
	void SetResponseAction(idStr const &ActionScriptName);

	/**
	* Adds a response effect and returns the pointer to the new Effect object.
	*
	* @effectEntityDef: The entity definition where the target script is stored.
	*					The effect entity "effect_script" is treated specially.
	*
	* @effectPostfix:	The string that gets passed to the effect script (e.g. "1_2")
	*
	* @args:	The entity's spawnargs needed to query the script argument for the
	*			aforementioned special case of "effect_script".
	*/
	CResponseEffect* AddResponseEffect(const idStr& effectEntityDef, 
									   const idStr& effectPostfix,
									   const idDict& args);

protected:
	/**
	 * Scriptfunction that is to be executed when this response 
	 * is triggered.
	 */
	idStr				m_ScriptFunction;

	/**
	 * How much damage must be applied for this response?
	 */
	float				m_MinDamage;

	/**
	 * No more than this.
	 */
	float				m_MaxDamage;

	/**
	* If non-zero, this specifies the number of effects
	* that get fired on response. If this is set to 2 and 
	* 5 response effects are available, exactly 2 random
	* effects are fired. If only one effect is available,
	* this effect would get fired twice.
	*/
	int					m_NumRandomEffects;

	/**
	* The list of ResponseEffects
	*/
	idList<CResponseEffect*> m_ResponseEffects;
};
typedef std::shared_ptr<CResponse> CResponsePtr;

#endif /* SR_RESPONSE__H */

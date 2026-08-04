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
#include "precompiled.h"
#pragma hdrstop



#include "Response.h"
#include "Stim.h"

#include <algorithm>

/********************************************************************/
/*                   CResponse                                      */
/********************************************************************/
CResponse::CResponse(idEntity* owner, StimType type, int uniqueId) : 
	CStimResponse(owner, type, uniqueId, false)
{
	m_ScriptFunction = NULL;
	m_MinDamage = 0.0f;
	m_MaxDamage = 0;
	m_NumRandomEffects = 0;
}

CResponse::~CResponse()
{
	// Remove all the allocated response effects from the heap
	m_ResponseEffects.DeleteContents(true);
}

void CResponse::Save(idSaveGame *savefile) const
{
	CStimResponse::Save(savefile);

	savefile->WriteString(m_ScriptFunction.c_str());
	savefile->WriteFloat(m_MinDamage);
	savefile->WriteFloat(m_MaxDamage);
	savefile->WriteInt(m_NumRandomEffects);

	savefile->WriteInt(m_ResponseEffects.Num());
	for (int i = 0; i < m_ResponseEffects.Num(); i++)
	{
		m_ResponseEffects[i]->Save(savefile);
	}
}

void CResponse::Restore(idRestoreGame *savefile)
{
	CStimResponse::Restore(savefile);

	savefile->ReadString(m_ScriptFunction);
	savefile->ReadFloat(m_MinDamage);
	savefile->ReadFloat(m_MaxDamage);
	savefile->ReadInt(m_NumRandomEffects);

	int num;
	savefile->ReadInt(num);
	m_ResponseEffects.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_ResponseEffects[i] = new CResponseEffect(NULL, "", "", false);
		m_ResponseEffects[i]->Restore(savefile);
	}
}

void CResponse::TriggerResponse(idEntity *sourceEntity, const CStimPtr& stim)
{
	// Perform the probability check
	if (!CheckChance()) return;

	idEntity* owner = m_Owner.GetEntity();
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING(
		"TriggerResponse:  type %s  func %s  owner %s  source %s\r",
		stim ? stim->m_StimTypeName.c_str() : "[NULL]",
		m_ScriptFunction.c_str(),
		owner->GetName(),
		sourceEntity ? sourceEntity->GetName() : "[NULL]"
	);

	// Notify the owner entity
	owner->OnStim(stim, sourceEntity);

	const function_t* func = owner->scriptObject.GetFunction(m_ScriptFunction.c_str());
	if (func == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", m_ScriptFunction.c_str());
		func = gameLocal.program.FindFunction(m_ScriptFunction.c_str());
	}

	if (func != NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running ResponseScript %s\r", func->Name());
		idThread *pThread = new idThread(func);
		int n = pThread->GetThreadNum();
		pThread->CallFunctionArgs(func, true, "eed", owner, sourceEntity, n);
		pThread->DelayedStart(0);
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("ResponseActionScript not found! [%s]\r", m_ScriptFunction.c_str());
	}

	// Default magnitude (in case we have a NULL stim)
	float magnitude = 10;

	if (stim != NULL)
	{
		// We have a "real" stim causing this response, retrieve the properties

		// Calculate the magnitude of the stim based on the distance and the falloff model
		magnitude = stim->m_Magnitude;
		float distance = (owner->GetPhysics()->GetOrigin() - sourceEntity->GetPhysics()->GetOrigin()).LengthFast();
		
		using std::min;
		// greebo: Be sure to use GetRadius() to consider time-dependent radii
		float radius = stim->GetRadius();
		float base = 1 - min(radius, distance) / idMath::Fmax(radius, 1e-3f);
		
		// Calculate the falloff value (the magnitude is between [0, magnitude] for positive falloff exponents)
		magnitude *= pow(base, stim->m_FallOffExponent);
	}
	
	//DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Available Response Effects: %u\r", m_ResponseEffects.Num());
	if (m_NumRandomEffects > 0) {
		// Random effect mode, choose exactly m_NumRandomEffects to fire
		for (int i = 1; i <= m_NumRandomEffects; i++) {
			// Get a random effectIndex in the range of [0, m_ResponseEffects.Num()[
			int effectIndex = gameLocal.random.RandomInt(m_ResponseEffects.Num());
			m_ResponseEffects[effectIndex]->runScript(owner, sourceEntity, magnitude);
		}
	}
	else {
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Iterating through ResponseEffects: %d\r", m_ResponseEffects.Num());
		// "Normal" mode, all the effects get fired in order
		for (int i = 0; i < m_ResponseEffects.Num(); i++) {
			m_ResponseEffects[i]->runScript(owner, sourceEntity, magnitude);
		}
	}
}

void CResponse::SetResponseAction(idStr const &action)
{
	m_ScriptFunction = action;
}

CResponseEffect* CResponse::AddResponseEffect(const idStr& effectEntityDef, 
											  const idStr& effectPostfix, 
											  const idDict& args)
{
	CResponseEffect* returnValue = NULL;
	
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Seeking EffectEntity [%s]\r", effectEntityDef.c_str());
	// Try to locate the specified entity definition
	const idDict* dict = gameLocal.FindEntityDefDict(effectEntityDef.c_str(),true); // grayman #3391 - don't create a default 'dict'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message

	if (effectEntityDef == "effect_script")
	{
		// We have a script effect, this is a special case
		idStr key;
		sprintf(key, "sr_effect_%s_arg1", effectPostfix.c_str());
		
		// Get the script argument from the entity's spawnargs
		idStr scriptStr = args.GetString(key);

		if (scriptStr != "")
		{
			bool isLocalScript = true;

			const function_t* scriptFunc = m_Owner.GetEntity()->scriptObject.GetFunction(scriptStr.c_str());
			if (scriptFunc == NULL)
			{
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CResponse: %s not found in local space, checking for global.\r", scriptStr.c_str());
				scriptFunc = gameLocal.program.FindFunction(scriptStr.c_str());
				isLocalScript = false;
			}

			if (scriptFunc != NULL)
			{
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("CResponse: %s SCRIPTFUNC.\r", scriptFunc->Name());
				// Allocate a new effect object
				CResponseEffect* newEffect = new CResponseEffect(scriptFunc, effectPostfix, scriptStr, isLocalScript);
				// Add the item to the list
				m_ResponseEffects.Append(newEffect);

				returnValue = newEffect;
			}
			else
			{
				gameLocal.Printf("Warning: Script not found: %s!\n", scriptStr.c_str());
			}
		}
		else {
			gameLocal.Printf("Warning: Script argument not found!\n");
		}
	}
	else if (dict != NULL)
	{
		idStr scriptStr = dict->GetString("script");

		const function_t* scriptFunc = gameLocal.program.FindFunction(scriptStr.c_str());
		if (scriptFunc != NULL)
		{
			// Allocate a new effect object
			CResponseEffect* newEffect = new CResponseEffect(scriptFunc, effectPostfix, scriptStr, false);
			
			// Add the item to the list
			m_ResponseEffects.Append(newEffect);

			returnValue = newEffect;
		}
		else {
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Warning: Script Function not found: [%s]\r", scriptStr.c_str());
		}
	}
	else
	{
		// Entity not found, emit a warning
		gameLocal.Printf("Warning: EffectEntityDef not found: %s.\r", effectEntityDef.c_str());
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Warning: EffectEntityDef not found: %s\r", effectEntityDef.c_str());
	}

	//DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Items in the list: %u\r", m_ResponseEffects.Num());
	
	return returnValue;
}

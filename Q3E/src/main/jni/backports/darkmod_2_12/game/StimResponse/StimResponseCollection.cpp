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



#include "StimResponseCollection.h"

CStimResponseCollection::~CStimResponseCollection()
{
	m_Stims.Clear();
	m_Responses.Clear();
}

void CStimResponseCollection::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(m_Stims.Num());
	for (int i = 0; i < m_Stims.Num(); i++)
	{
		savefile->WriteInt(static_cast<int>(m_Stims[i]->m_StimTypeId));
		m_Stims[i]->Save(savefile);
	}

	savefile->WriteInt(m_Responses.Num());
	for (int i = 0; i < m_Responses.Num(); i++)
	{
		savefile->WriteInt(m_Responses[i]->m_StimTypeId);
		m_Responses[i]->Save(savefile);
	}
}

void CStimResponseCollection::Restore(idRestoreGame *savefile)
{
	int num;

	savefile->ReadInt(num);
	m_Stims.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		// Allocate a new stim class (according to the type info)
		int typeInt;
		savefile->ReadInt(typeInt);
		m_Stims[i] = CStimPtr(new CStim(NULL, static_cast<StimType>(typeInt), -1));
		m_Stims[i]->Restore(savefile);
	}

	savefile->ReadInt(num);
	m_Responses.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		// Allocate a new response class (according to the type info)
		int typeInt;
		savefile->ReadInt(typeInt);
		m_Responses[i] = CResponsePtr(new CResponse(NULL, static_cast<StimType>(typeInt), -1));
		m_Responses[i]->Restore(savefile);
	}
}

CStimPtr CStimResponseCollection::CreateStim(idEntity* p_owner, StimType type)
{
/*	grayman #3462 - undo the following change

	// grayman #2862 - don't create a visual stim for a door that's not marked shouldBeClosed

	if (( type == ST_VISUAL ) &&
		( idStr::Cmp( p_owner->spawnArgs.GetString("AIUse"), AIUSE_DOOR ) == 0 ) &&
		( !p_owner->spawnArgs.GetBool("shouldBeClosed","0" ) ) )
	{
		return CStimPtr(); // null result
	}
 */
	// Increase the counter to the next ID
	gameLocal.m_HighestSRId++;
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Creating Stim with ID: %d\r", gameLocal.m_HighestSRId);

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Creating CStim\r");
	
	return CStimPtr(new CStim(p_owner, type, gameLocal.m_HighestSRId));
}

CResponsePtr CStimResponseCollection::CreateResponse(idEntity* p_owner, StimType type)
{
	// Increase the counter to the next ID
	gameLocal.m_HighestSRId++;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Creating Response with ID: %d\r", gameLocal.m_HighestSRId);

	// Optimization: Set contents to include CONTENTS_RESPONSE
	p_owner->GetPhysics()->SetContents( p_owner->GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	//DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Creating CResponse\r");
	return CResponsePtr(new CResponse(p_owner, type, gameLocal.m_HighestSRId));
}

CStimPtr CStimResponseCollection::AddStim(idEntity *Owner, int Type, float fRadius, bool bRemovable, bool bDefault)
{
	CStimPtr stim;
	int i, n;

	n = m_Stims.Num();
	for(i = 0; i < n; i++)
	{
		if(m_Stims[i]->m_StimTypeId == Type)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Stim of that type is already in collection, returning it.\r");
			stim = m_Stims[i];
			break;
		}
	}

	if (stim == NULL)
	{
		// Create either type specific descended class, or the default base class
		stim = CreateStim(Owner, (StimType) Type);
		if ( stim != NULL ) // grayman #2862
		{
			m_Stims.Append(stim);
		}
	}

	if (stim != NULL)
	{
		stim->m_Default = bDefault;
		stim->m_Removable = bRemovable;
		stim->m_Radius = fRadius;
		stim->m_State = SS_DISABLED;

		gameLocal.AddStim(Owner);
	}

	return stim;
}

CResponsePtr CStimResponseCollection::AddResponse(idEntity *Owner, int type, bool bRemovable, bool bDefault)
{
	CResponsePtr rv;

	int n = m_Responses.Num();

	for (int i = 0; i < n; ++i)
	{
		if (m_Responses[i]->m_StimTypeId == type)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Response of that type is already in collection, returning it\r");
			rv = m_Responses[i];
			break;
		}
	}

	if (rv == NULL)
	{
		rv = CreateResponse(Owner, static_cast<StimType>(type));
		m_Responses.Append(rv);
	}

	if (rv != NULL)
	{
		rv->m_Default = bDefault;
		rv->m_Removable = bRemovable;

		gameLocal.AddResponse(Owner); 
	}

	// Optimization: Update clip contents to include contents_response

	return rv;
}

CStimPtr CStimResponseCollection::AddStim(const CStimPtr& stim)
{
	if (stim == NULL) return stim;

	CStimPtr rv;

	int n = m_Stims.Num();
	for (int i = 0; i < n; ++i)
	{
		if (m_Stims[i]->m_StimTypeId == stim->m_StimTypeId)
		{
			rv = m_Stims[i];
			break;
		}
	}

	if (rv == NULL)
	{
		rv = stim;
		m_Stims.Append(rv);

		gameLocal.AddStim(stim->m_Owner.GetEntity());
	}

	return rv;
}

CResponsePtr CStimResponseCollection::AddResponse(const CResponsePtr& response)
{
	CResponsePtr rv;

	if (response == NULL) return rv;

	int n = m_Responses.Num();
	for (int i = 0; i < n; ++i)
	{
		if (m_Responses[i]->m_StimTypeId == response->m_StimTypeId)
		{
			rv = m_Responses[i];
			break;
		}
	}

	if (rv == NULL)
	{
		rv = response;
		m_Responses.Append(rv);

		gameLocal.AddResponse(response->m_Owner.GetEntity());
	}

	return rv;
}


int CStimResponseCollection::RemoveStim(StimType type)
{
	int n = m_Stims.Num();

	for (int i = 0; i < n; ++i)
	{
		const CStimPtr& stim = m_Stims[i];

		if (stim->m_StimTypeId == type)
		{
			if (stim->m_Removable)
			{
				m_Stims.RemoveIndex(i);
			}

			break;
		}
	}

	return m_Stims.Num();
}

int CStimResponseCollection::RemoveResponse(StimType type)
{
	idEntity* owner = NULL;
	int n = m_Responses.Num();

	for (int i = 0; i < n; ++i)
	{
		const CResponsePtr& response = m_Responses[i];

		if (response->m_StimTypeId == type)
		{
			if (response->m_Removable)
			{
				owner = response->m_Owner.GetEntity();
				m_Responses.RemoveIndex(i);
			}

			break;
		}
	}

	// Remove the CONTENTS_RESPONSE flag if no more responses
	if (m_Responses.Num() <= 0 && owner != NULL)
	{
		owner->GetPhysics()->SetContents(owner->GetPhysics()->GetContents() & ~CONTENTS_RESPONSE);
	}

	return m_Responses.Num();
}

CStimPtr CStimResponseCollection::GetStimByType(StimType type)
{
	for (int i = 0; i < m_Stims.Num(); ++i)
	{
		if (m_Stims[i]->m_StimTypeId == type)
		{
			return m_Stims[i];
		}
	}

	return CStimPtr();
}

CResponsePtr CStimResponseCollection::GetResponseByType(StimType type)
{
	for (int i = 0; i < m_Responses.Num(); ++i)
	{
		if (m_Responses[i]->m_StimTypeId == type)
		{
			return m_Responses[i];
		}
	}

	return CResponsePtr();
}

bool CStimResponseCollection::ParseSpawnArg(const idDict& args, idEntity* owner, const char sr_class, int index)
{
	bool rc = false;
	idStr str;

	CStimPtr stim;
	CResponsePtr resp;
	CStimResponsePtr sr;

	StimState state( SS_DISABLED );
	StimType typeOfStim;
	
	// Check if the entity contains either a stim or a response.
	if (sr_class != 'S' && sr_class != 'R')
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_class value [%s]\r", str.c_str());
		goto Quit;
	}

	// Get the id of the stim/response type so we know what sub-class to create
	args.GetString(va("sr_type_%u", index), "-1", str);

	// This is invalid as an entity definition
	if(str == "-1")
	{
		sr.reset();
		goto Quit;
	}

	// If the first character is alphanumeric, we check if it 
	// is a known id and convert it.
	/* StimType */ typeOfStim = ST_DEFAULT;

	if((str[0] >= 'a' && str[0] <= 'z')
		|| (str[0] >= 'A' && str[0] <= 'Z'))
	{
		// Try to recognise the string as known Stim type
		typeOfStim = CStimResponse::GetStimType(str);
		
		// If the string hasn't been found, we have id == ST_DEFAULT.
		if (typeOfStim == ST_DEFAULT)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_type id [%s]\r", str.c_str());
			sr.reset();
			goto Quit;
		}
	}
	else if(str[0] >= '0' && str[0] <= '9') // Is it numeric?
	{	
		typeOfStim = (StimType) atol(str.c_str());
	}
	else		// neither a character nor a number, thus it is invalid.
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid sr_type id [%s]\r", str.c_str());
		sr.reset();
		goto Quit;
	}


	if (sr_class == 'S')
	{
		stim = CreateStim(owner, typeOfStim);
		if ( stim == NULL ) // grayman #2862
		{
			goto Quit; // nasty goto!!
		}
		sr = stim;
	}
	else if (sr_class == 'R')
	{
		resp = CreateResponse(owner, typeOfStim);
		sr = resp;
	}

	// Set stim response type
	sr->m_StimTypeId = typeOfStim;

	// Set stim response name string
	sr->m_StimTypeName = str;

	// Read stim response state from the def file
	state = static_cast<StimState>(args.GetInt(va("sr_state_%u", index), "1"));

	sr->SetEnabled(state == SS_ENABLED);
	
	sr->m_Chance = args.GetFloat(va("sr_chance_%u", index), "1.0");

	// A stim also may have a radius
	if(sr_class == 'S')
	{
		stim->m_Radius = args.GetFloat(va("sr_radius_%u", index), "0");
		stim->m_RadiusFinal = args.GetFloat(va("sr_radius_final_%u", index), "-1");

		stim->m_FallOffExponent = args.GetInt(va("sr_falloffexponent_%u", index), "0");
		stim->m_bUseEntBounds = args.GetBool(va("sr_use_bounds_%u", index), "0");
		stim->m_bCollisionBased = args.GetBool(va("sr_collision_%u", index), "0");
		stim->m_bScriptBased = args.GetBool(va("sr_script_%u", index), "0");

		stim->m_Velocity = args.GetVector(va("sr_velocity_%u", index), "0 0 0");

		stim->m_Bounds[0] = args.GetVector(va("sr_bounds_mins_%u", index), "0 0 0");
		stim->m_Bounds[1] = args.GetVector(va("sr_bounds_maxs_%u", index), "0 0 0");

		// set up time interleaving so the stim isn't fired every frame
		stim->m_TimeInterleave = args.GetInt(va("sr_time_interval_%u", index), "0");

		// stgatilov: add random offset in range [0 ... TI],
		// so that time taken by stim processing is spread approximately evenly across frames
		stim->m_TimeInterleaveStamp = gameLocal.time - int(stim->m_TimeInterleave * gameLocal.random.RandomFloat());

		// stgatilov #6223: wait at least one full period from game start until the first trigger
		// this is necessary for lights: they enable/disable stims during thread initialization, which can happen later than first stim check
		// (unfortunately, some lights use custom entityDef, and we can't disable their stims initially by spawnarg)
		if (stim->m_TimeInterleaveStamp < 0)
			stim->m_TimeInterleaveStamp += stim->m_TimeInterleave;

		// userfriendly stim duration time
		stim->m_Duration = args.GetInt(va("sr_duration_%u", index), "0");
		stim->m_Magnitude = args.GetFloat(va("sr_magnitude_%u", index), "1.0");

		stim->m_MaxFireCount = args.GetInt(va("sr_max_fire_count_%u", index), "-1");

		// Check if we have a timer on this stim.
		CreateTimer(args, stim, index);
	}
	else	// this is only for responses
	{
		sr->m_ChanceTimer = args.GetInt(va("sr_chance_timeout_%u", index), "-1");

		resp->m_NumRandomEffects = args.GetInt(va("sr_random_effects_%u", index), "0");

		// Get the name of the script function for processing the response
		args.GetString("sr_script_" + str, "", str);
		resp->m_ScriptFunction = str;

		// Try to identify the ResponseEffect spawnargs
		int effectIdx = 1;
		while (effectIdx > 0) {
			// Try to find a string like "sr_effect_2_1"
			args.GetString(va("sr_effect_%u_%u", index, effectIdx), "", str);

			if (str.IsEmpty())
			{
				// Set the index to negative values to end the loop
				effectIdx = -1;
			}
			else {
				// Assemble the postfix of this effect for later key/value lookup
				// This is passed to the effect script eventually
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Adding response effect\r");
				resp->AddResponseEffect(str, va("%u_%u", index, effectIdx), args);
				effectIdx++;
			}
		}
	}

	rc = true;

Quit:
	if(sr != NULL)
	{
		if(stim != NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Stim %s added to collection for %s\r", stim->m_StimTypeName.c_str(), owner->name.c_str());
			AddStim(stim);
			stim->m_State = state;
		}

		if(resp != NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Response %s added to collection for %s\r", resp->m_StimTypeName.c_str(), owner->name.c_str());
			AddResponse(resp);
		}
	}

	return rc;
}

void CStimResponseCollection::InitFromSpawnargs(const idDict& args, idEntity* owner)
{
	if (owner == NULL)
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Owner set to NULL is not allowed!\r");
		return;
	}

	idStr name;

	for (int i = 1; /* in-loop break */; ++i)
	{
		idStr name = va("sr_class_%u", i);
		DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Looking for %s\r", name.c_str());

		idStr str;
		if (!args.GetString(name, "X", str))
		{
			break;
		}

		char sr_class = str[0];

		if (ParseSpawnArg(args, owner, sr_class, i) == false)
		{
			break;
		}
	}
}


void CStimResponseCollection::CreateTimer(const idDict& args, const CStimPtr& stim, int index)
{
	CStimResponseTimer* timer = stim->GetTimer();

	timer->m_Reload = args.GetInt(va("sr_timer_reload_%u", index) , "-1");

	idStr str = args.GetString(va("sr_timer_type_%u", index), "");

	timer->m_Type = (str == "RELOAD") ? CStimResponseTimer::SRTT_RELOAD : CStimResponseTimer::SRTT_SINGLESHOT;
	
	args.GetString(va("sr_timer_time_%u", index), "0:0:0:0", str);

    TimerValue val = CStimResponseTimer::ParseTimeString(str);
	
	// if timer is actually set
	if (val.Hour || val.Minute || val.Second || val.Millisecond)
	{
		// TODO: Return a bool here so that the outer function knows not to add this to m_Stim in the collection?

		stim->AddTimerToGame();
		timer->SetTimer(val.Hour, val.Minute, val.Second, val.Millisecond);
		
		// timer starts on map startup by default, otherwise wait for start
		if (!args.GetBool(va("sr_timer_waitforstart_%u", index), "0"))
		{
			timer->Start(static_cast<unsigned int>(sys->GetClockTicks()));
		}
	}
}

bool CStimResponseCollection::HasStim()
{
	return m_Stims.Num() > 0;
}

bool CStimResponseCollection::HasResponse()
{
	return m_Responses.Num() > 0;
}

CStimResponsePtr CStimResponseCollection::FindStimResponse(int uniqueId)
{
	// Search the stims
	for (int i = 0; i < m_Stims.Num(); ++i)
	{
		if (m_Stims[i]->GetUniqueId() == uniqueId)
		{
			return m_Stims[i];
		}
	}

	// Search the responses
	for (int i = 0; i < m_Responses.Num(); i++)
	{
		if (m_Responses[i]->GetUniqueId() == uniqueId)
		{
			return m_Responses[i];
		}
	}

	// Nothing found
	return CStimResponsePtr();
}

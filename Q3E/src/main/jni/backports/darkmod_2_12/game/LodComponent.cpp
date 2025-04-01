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

#include "LodComponent.h"

static const float MIN_LOD_BIAS_DEFAULT = -1e+10f;
static const float MAX_LOD_BIAS_DEFAULT = 1e+10f;

LodComponent::LodComponent()
{
	m_dead = false;
	m_entity = nullptr;

	// by default no LOD to save memory and time
	m_LODHandle = 0;
	m_DistCheckTimeStamp = NOLOD;

	// by default disabled
	m_MinLODBias = MIN_LOD_BIAS_DEFAULT;
	m_MaxLODBias = MAX_LOD_BIAS_DEFAULT;

	m_LODLevel = m_ModelLODCur = m_SkinLODCur = 0xDEAFBEEF;	//to be set
	m_OffsetLODCur.Zero();
}

void LodComponent::Save( idSaveGame &savefile ) const
{
	//note: m_dead and m_entity handled in LodSystem

	savefile.WriteInt(m_LODHandle);
	savefile.WriteInt(m_DistCheckTimeStamp);
	savefile.WriteInt(m_LODLevel);
	savefile.WriteInt(m_ModelLODCur);
	savefile.WriteInt(m_SkinLODCur);
	savefile.WriteVec3(m_OffsetLODCur);

	// #3113
	savefile.WriteFloat(m_MinLODBias);
	savefile.WriteFloat(m_MaxLODBias);
	savefile.WriteString(m_HiddenSkin);
	savefile.WriteString(m_VisibleSkin);
}

void LodComponent::Restore( idRestoreGame &savefile )
{
	//note: m_dead and m_entity handled in LodSystem

	savefile.ReadUnsignedInt(m_LODHandle);
	savefile.ReadInt(m_DistCheckTimeStamp);
	savefile.ReadInt(m_LODLevel);
	savefile.ReadInt(m_ModelLODCur);
	savefile.ReadInt(m_SkinLODCur);
	savefile.ReadVec3(m_OffsetLODCur);

	// #3113
	savefile.ReadFloat(m_MinLODBias);
	savefile.ReadFloat(m_MaxLODBias);
	savefile.ReadString(m_HiddenSkin);
	savefile.ReadString(m_VisibleSkin);
}


/*
================
LodComponent::ParseLODSpawnargs

Tels: Look at dist_think_interval, lod_1_distance etc. and fill the m_LOD
data. The passed in dict is usually just this->spawnArgs, but can also
be from a completely different entity def so we can parse the spawnargs
without having to actually have the entity spawned. The fRandom
value is used to selectively hide some entities when hide_probability
is set, and should be between 0 and 1.0.
================
*/
lod_handle LodComponent::ParseLODSpawnargs( idEntity* entity, const idDict* dict, const float fRandom )
{
	m_LODHandle = 0;
	m_DistCheckTimeStamp = NOLOD;

	bool lod_disabled = dict->GetBool( "no_lod" ); // SteveL #3796: allow mappers to disable LOD explicitly on an entity

	// a quick check for LOD, to avoid looking at all lod_x_distance spawnargs:
	if ( lod_disabled) {// no LOD wanted
		return m_LODHandle;
	}

	float fHideDistance = dict->GetFloat( "hide_distance", "-1" );

	// by default these are at "ignored" values
	if ( !dict->GetFloat( "min_lod_bias", "", m_MinLODBias ) )
		m_MinLODBias = MIN_LOD_BIAS_DEFAULT;
	if ( !dict->GetFloat( "max_lod_bias", "", m_MaxLODBias ) )
		m_MaxLODBias = MAX_LOD_BIAS_DEFAULT;

	/* stgatilov TODO: for some reason, this code was in idEntity::Spawn BEFORE calling ParseLODSpawnargs
	* so it had no effect...
	if (renderEntity.customSkin && !idStr(spawnArgs.GetString("lod_hidden_skin")).IsEmpty())
	{
		m_VisibleSkin = renderEntity.customSkin->GetName();
		//		gameLocal.Printf ("%s: Storing current skin %s.\n", GetName(), m_VisibleSkin.c_str() );
	}*/

	m_HiddenSkin = dict->GetString( "lod_hidden_skin", "" );
	m_VisibleSkin = "";

	if (m_MinLODBias > m_MaxLODBias)
	{
		m_MinLODBias = m_MaxLODBias - 0.1f;
	}

	// Disable LOD if the LOD settings came with the entity def but the mapper has overridden the model 
	// without updating any LOD models #3912
	if ( const idDict* entDef = gameLocal.FindEntityDefDict( dict->GetString("classname"), false ) )
	{
		const bool inherited_model = *entDef->GetString("model") != '\0';
		const bool inherited_lod = entDef->GetFloat("dist_check_period") != 0.0f || entDef->GetFloat("hide_distance") >= 0.1f;
		const bool model_overriden = idStr::Icmp( dict->GetString("model"), entDef->GetString("model") ) != 0;
		bool lod_model_overridden = false;
		const idKeyValue* kv = NULL;
		while ( ( kv = dict->MatchPrefix( "model_lod_", kv ) ) != NULL )
		{
			if ( idStr::Icmp( kv->GetValue(), entDef->GetString( kv->GetKey() ) ) )
			{
				lod_model_overridden = true;
			}
		}
		if ( inherited_model && inherited_lod && model_overriden && !lod_model_overridden )
		{
			// Suppress LOD
			return m_LODHandle;
		}
	}

	// distance dependent LOD from this point on:
	lod_data_t lodData;
	lod_data_t *m_LOD = &lodData;	// don't want to edit all the code...

	// if interval not set, use twice per second
	m_LOD->DistCheckInterval = int( 1000.0f * dict->GetFloat( "dist_check_period", "0.5" ) );

	// SteveL #3744
	// If there are no LOD skin spawnargs set at all, then prevent LOD changing skins with a special
	// m_SkinLODCur value of -1
	if (dict->MatchPrefix("skin_lod_") == NULL)
	{
		m_SkinLODCur = -1;
	} else {
		m_SkinLODCur = 0;
	}

	m_ModelLODCur = 0;
	m_LODLevel = 0;

	// SteveL #4170: As with skins, disable LOD shadowcasting changes unless the mapper has set a noshadows_lod spawnarg
	if (dict->MatchPrefix("noshadows_lod_") == NULL)
	{
		m_LOD->noshadowsLOD = -1;
	} else {
		m_LOD->noshadowsLOD = dict->GetBool( "noshadows", "0" ) ? 1 : 0;	// the default value for level 0
	}

	// if > 0, if the entity is closer than this, lod_bias will be at minimum 1.0
	m_LOD->fLODNormalDistance = dict->GetFloat( "lod_normal_distance", "500" );
	if (m_LOD->fLODNormalDistance < 0.0f)
	{
		m_LOD->fLODNormalDistance = 0.0f;
	}

	idStr temp;
	m_LOD->OffsetLOD[0] = idVec3(0,0,0);			// assume there is no custom per-LOD model offset
	m_LOD->DistLODSq[0] = 0;

	// setup level 0 (aka "The one and only original")
	m_LOD->ModelLOD[0] = dict->GetString( "model", "" );

	idStr entityName = "[unknown]";
	if (entity)
	{
		entityName = entity->GetName();

		// For func_statics where name == model, use "" so they can share the same LOD data
		// even tho they all have different "models". (Their model spawnarg is never used.)
		if (entity->IsType( idStaticEntity::Type ) && m_LOD->ModelLOD[0] == entity->GetName() )
		{
			m_LOD->ModelLOD[0] = "";
		}

		// use whatever was set as skin, that can differ from spawnArgs.GetString("skin") due to random_skin:
		const renderEntity_t *rent = const_cast<idEntity*>(entity)->GetRenderEntity();
		if ( rent->customSkin )
		{
			m_LOD->SkinLOD[0] = rent->customSkin->GetName(); 
		}
		else
		{
			m_LOD->SkinLOD[0] = "";
		}
	}

	// start at 1, since 0 is "the original level" setup already above
	for (int i = 1; i < LOD_LEVELS; i++)
	{
		// set to 0,0,0 as default in case someone tries to use it
		m_LOD->OffsetLOD[i] = idVec3(0,0,0);

		if (i < LOD_LEVELS - 1)
		{
			// for i == LOD_LEVELS - 1, we use "hide_distance"
			sprintf(temp, "lod_%i_distance", i);
			m_LOD->DistLODSq[i] = dict->GetFloat( temp, "0.0" );

			// Tels: Fix #2635: if the LOD distance here is < fHideDistance, use hide distance-1 so the
			// entity gets really hidden.
			if (fHideDistance > 1.0f && m_LOD->DistLODSq[i] > fHideDistance)
			{
				m_LOD->DistLODSq[i] = fHideDistance - 1.0f;
			}
		}

		if (i == LOD_LEVELS - 1)
		{
			// last distance is named differently so you don't need to know how many levels the code supports:
			m_LOD->DistLODSq[i] = fHideDistance;

			// compute a random number and check it against the hide probability spawnarg
			// do this only once at setup time, so the setting is stable during runtime
			float fHideProbability = dict->GetFloat( "lod_hide_probability", "1.0" );
			if (fRandom > fHideProbability)
			{
				// disable hiding
				m_LOD->DistLODSq[i] = -1.0f;		// disable
				continue;
			}

			// do we have a lod_fadeout_range?
			m_LOD->fLODFadeOutRange = dict->GetFloat( "lod_fadeout_range", "0.0" );

			if (m_LOD->fLODFadeOutRange < 0)
			{
				gameLocal.Warning (" %s: lod_fadeout_range must be >= 0 but is %f. Ignoring it.", entityName.c_str(), m_LOD->fLODFadeOutRange);
				m_LOD->fLODFadeOutRange = 0.0f;
			}
			else
			{
				// square for easier comparisation with deltaSq
				m_LOD->fLODFadeOutRange *= m_LOD->fLODFadeOutRange;
			}

			// do we have a lod_fadein_range?
			m_LOD->fLODFadeInRange = dict->GetFloat( "lod_fadein_range", "0.0" );

			if (m_LOD->fLODFadeInRange < 0)
			{
				gameLocal.Warning (" %s: lod_fadein_range must be >= 0 but is %f. Ignoring it.", entityName.c_str(), m_LOD->fLODFadeInRange);
				m_LOD->fLODFadeInRange = 0.0f;
			}
			else if (m_LOD->fLODFadeInRange > 0 && m_LOD->fLODFadeInRange > m_LOD->DistLODSq[1])
			{
				gameLocal.Warning (" %s: lod_fadein_range must be <= lod_1_distance (%f) 0 but is %f. Ignoring it.", entityName.c_str(), m_LOD->DistLODSq[1], m_LOD->fLODFadeInRange);
				m_LOD->fLODFadeOutRange = 0.0f;
			}
			else
			{
				// square for easier comparisation with deltaSq
				m_LOD->fLODFadeInRange *= m_LOD->fLODFadeInRange;
			}

			//gameLocal.Printf (" %s: lod_fadeout_range %0.2f lod_fadein_range %0.2f.\n", GetName(), m_LOD->fLODFadeOutRange, m_LOD->fLODFadeInRange);
		}

		//		gameLocal.Printf (" %s: init LOD %i m_LOD->DistLODSq=%f\n", GetName(), i, m_LOD->DistLODSq[i]); 

		if (i > 0 && m_LOD->DistLODSq[i] > 0 && (m_LOD->DistLODSq[i] * m_LOD->DistLODSq[i]) < m_LOD->DistLODSq[i-1])
		{
			gameLocal.Warning (" %s: LOD %i m_DistLODSq %f < LOD %i m_DistLODSq=%f (this will not work!)\n",
				entityName.c_str(), i, m_LOD->DistLODSq[i] * m_LOD->DistLODSq[i], i-1, m_LOD->DistLODSq[i-1]); 
		}
		// -1 should stay -1 to signal "don't use this level"
		if (m_LOD->DistLODSq[i] > 0)
		{
			m_LOD->DistLODSq[i] *= m_LOD->DistLODSq[i];

			// the last level is "hide", so we don't need a model, skin, offset or noshadows there
			if (i < LOD_LEVELS - 1)
			{
				// not the last level
				sprintf(temp, "model_lod_%i", i);
				m_LOD->ModelLOD[i] = dict->GetString( temp );
				if (m_LOD->ModelLOD[i].Length() == 0) { m_LOD->ModelLOD[i] = m_LOD->ModelLOD[0]; }

				sprintf(temp, "skin_lod_%i", i);
				m_LOD->SkinLOD[i] = dict->GetString( temp );
				if (m_LOD->SkinLOD[i].Length() == 0) { m_LOD->SkinLOD[i] = m_LOD->SkinLOD[0]; }

				// set the right bit for noshadows
				sprintf(temp, "noshadows_lod_%i", i );  // 1, 2, 4, 8, 16 etc
				m_LOD->noshadowsLOD |= (dict->GetBool( temp, "0" ) ? 1 : 0) << i;

				//				// set the right bit for "standin". "standins" are models that always rotate in
				//				// XY to face the player, but unlike particles, don't tilt with the view
				//				sprintf(temp, "standin_lod_%i", i );
				//									  // 1, 2, 4, 8, 16 etc
				//				m_LOD->standinLOD |= (dict->GetBool( temp, "0" ) ? 1 : 0) << i;

				// setup the manual offset for this LOD stage (needed to align some models)
				sprintf(temp, "offset_lod_%i", i);
				m_LOD->OffsetLOD[i] = dict->GetVector( temp, "0,0,0" );
			}
			// else hiding needs no offset

			//gameLocal.Printf (" noshadowsLOD 0x%08x model %s skin %s\n", m_noshadowsLOD, m_ModelLOD[i].c_str(), m_SkinLOD[i].c_str() );
		}
		else
		{
			// initialize to empty in case someone tries to access them
			m_LOD->SkinLOD[i] = "";
			m_LOD->ModelLOD[i] = "";
			m_LOD->OffsetLOD[i] = idVec3(0,0,0);
		}
	}

	m_LOD->bDistCheckXYOnly = dict->GetBool( "dist_check_xy", "0" );

	// add some phase diversity to the checks so that they don't all run in one frame
	// make sure they all run on the first frame though
	m_DistCheckTimeStamp = gameLocal.time - (int) (m_LOD->DistCheckInterval * gameLocal.random.RandomFloat());

	// stgatilov #6359: drop LOD component if there are no settings which may affect anything at runtime
	bool lodProcessingNeeded = false;
	if (m_MinLODBias != MIN_LOD_BIAS_DEFAULT || m_MaxLODBias != MAX_LOD_BIAS_DEFAULT)
		lodProcessingNeeded = true;
	if (m_LOD->fLODFadeInRange != 0.0f || m_LOD->fLODFadeOutRange != 0.0f)
		lodProcessingNeeded = true;
	for (int i = 0; i < LOD_LEVELS; i++)
		if (m_LOD->DistLODSq[i] > 0.0f)
			lodProcessingNeeded = true;

	if (!lodProcessingNeeded)
		return m_LODHandle;	// disable LOD processing

	// register the data with the ModelGenerator and return the handle
	lod_handle h = gameLocal.m_ModelGenerator->RegisterLODData( m_LOD );

	m_entity = entity;
	m_LODHandle = h;

	return m_LODHandle;
}

/*
================
LodComponent::StopLOD

Tels: Permanently disable LOD checks, if doTeam is true, including on all of our attachements
================
*/
void LodComponent::StopLOD( idEntity *ent, bool doTeam )
{
	assert(ent);

	int idx = ent->lodIdx;
	if (idx >= 0) {
		LodComponent &self = gameLocal.lodSystem.Get(idx);

		// deregister our LOD struct
		if (self.m_LODHandle)
		{
			// gameLocal.Printf( "%s: Stopping LOD.\n", GetName() );
			gameLocal.m_ModelGenerator->UnregisterLODData( self.m_LODHandle );
			self.m_LODHandle = 0;
		}
		gameLocal.lodSystem.Remove( idx );
		ent->lodIdx = -1;
	}

	if (!doTeam) { return; }

	/* also all the bound entities in our team */
	idEntity* NextEnt = ent;

	idEntity* bindM = ent->GetBindMaster();
	if ( bindM ) { NextEnt = bindM;	}

	while ( NextEnt != NULL )
	{
		//gameLocal.Printf(" Looking at entity %s\n", NextEnt->name.c_str());
		idEntity *ent = static_cast<idEntity*>( NextEnt );
		if (ent)
		{
			StopLOD( ent, false );
		}
		/* get next Team member */
		NextEnt = NextEnt->GetNextTeamEntity();
	}
}

/*
================
LodComponent::DisableLOD

Tels: temp. disable LOD checks by setting the next check timestamp to negative.
================
*/
void LodComponent::DisableLOD( idEntity *ent, bool doTeam )
{
	assert(ent);

	int idx = ent->lodIdx;
	if (idx >= 0) {
		LodComponent &self = gameLocal.lodSystem.Get(idx);
		// negate check interval
		if (self.m_LODHandle && self.m_DistCheckTimeStamp != NOLOD)
		{
//			gameLocal.Printf( "%s: Temporarily disabling LOD.\n", GetName() );
			self.m_DistCheckTimeStamp = NOLOD;
		}
	}

	if (!doTeam) { return; }

	/* also all the bound entities in our team */
	idEntity* NextEnt = ent;
	idEntity* bindM = ent->GetBindMaster();
	if ( bindM ) { NextEnt = bindM; }

	while ( NextEnt != NULL )
	{
		//gameLocal.Printf(" Looking at entity %s\n", NextEnt->name.c_str());
		idEntity *ent = static_cast<idEntity*>( NextEnt );
		if (ent)
		{
			DisableLOD( ent, false );
		}
		/* get next Team member */
		NextEnt = NextEnt->GetNextTeamEntity();
	}
}

/*
================
LodComponent::EnableLOD

Tels: Enable LOD checks again (after DisableLOD) by setting the check interval to positive.
================
*/
void LodComponent::EnableLOD( idEntity *ent, bool doTeam )
{
	assert(ent);

	int idx = ent->lodIdx;
	if (idx >= 0) {
		LodComponent &self = gameLocal.lodSystem.Get(idx);
		// negate check interval
		if (self.m_LODHandle && self.m_DistCheckTimeStamp == NOLOD)
		{
//			gameLocal.Printf( "%s: Enabling LOD again.\n", GetName() );
			self.m_DistCheckTimeStamp = gameLocal.time;
		}
	}

	if (!doTeam) { return; }

	/* also all the bound entities in our team */
	idEntity* NextEnt = ent;
	idEntity* bindM = ent->GetBindMaster();
	if ( bindM ) { NextEnt = bindM; }

	while ( NextEnt != NULL )
	{
		//gameLocal.Printf(" Looking at entity %s\n", NextEnt->name.c_str());
		idEntity *ent = static_cast<idEntity*>( NextEnt );
		if (ent)
		{
			EnableLOD( ent, false );
		}
		/* get next Team member */
		NextEnt = NextEnt->GetNextTeamEntity();
	}
}

/***********************************************************************

Thinking about LOD

We pass a ptr to the current data, so that the SEED can let the spawned
entities think while still keeping their LOD data only once per class.

This routine will only modify:
* m_LODLevel
* m_DistCheckTimeStamp = gameLocal.time - 0.1;

It will also return the new alpha value for this entity, where 0.0f means
hidden and alpha > 0 means visible.

The caller is responsible for calling SetAlpha() and Hide/Show on the
proper entity (e.g. the entity thet the m_LOD is for) as well as
switching the model and skin.

***********************************************************************/
float LodComponent::ThinkAboutLOD( const lod_data_t *m_LOD, const float deltaSq ) 
{
	//nbohr1more: #4372: Allow lod_bias args for func_emitter entities
	float lodbias = cv_lod_bias.GetFloat();

	if ( lodbias < m_MinLODBias || lodbias > m_MaxLODBias ) {
		return 0;
	}

	// have no LOD
	if (NULL == m_LOD)
	{
		//switch to main model
		m_LODLevel = 0;
		// fully visible
		return 1.0f;
	}

	bool bWithinDist = false;

	// by default fully visible
	float fAlpha = 1.0f;

	//	gameLocal.Warning("%s: ThinkAboutLOD called with m_LOD %p deltaSq %0.2f", GetName(), m_LOD, deltaSq);

	// Tels: check in which LOD level we are 
	for (int i = 0; i < LOD_LEVELS; i++)
	{
		//		gameLocal.Printf ("%s considering LOD %i (distance %f)\n", GetName(), i, m_LOD->DistLODSq[i] );

		// skip this level (but not the first)
		if (m_LOD->DistLODSq[i] <= 0 && i > 0)
		{
			//			gameLocal.Printf ("%s skipping LOD %i (distance %f)\n", GetName(), i, m_LOD->DistLODSq[i] );
			continue;
		}

		// find the next usable level
		int nextLevel = i + 1;
		while (nextLevel < LOD_LEVELS && m_LOD->DistLODSq[nextLevel] <= 0 )
		{
			nextLevel++;
		}

		//		gameLocal.Printf ("%s ThinkAboutLOD deltaSq = %0.2f (this=%0.2f, nextLevel=%i, next=%0.2f, i=%i)\n", GetName(), deltaSq, m_LOD->DistLODSq[i],
		//					nextLevel, nextLevel < LOD_LEVELS ? m_LOD->DistLODSq[nextLevel] : -1, i );

		// found a usable next level, or the last level is -1 (means no hide)
		if (nextLevel < LOD_LEVELS)
		{
			bWithinDist = ((deltaSq > m_LOD->DistLODSq[i]) && (deltaSq <= m_LOD->DistLODSq[nextLevel]));
		}
		else
		{
			if (m_LOD->DistLODSq[ LOD_LEVELS - 1] < 0)
			{
				//				gameLocal.Printf ("%s no next level (last level is -1)\n", GetName() );
				bWithinDist = (deltaSq > m_LOD->DistLODSq[ i ]);
			}
			else
			{
				if (i < LOD_LEVELS - 1)
				{
					bWithinDist = (deltaSq < m_LOD->DistLODSq[ LOD_LEVELS - 1]);
				}
				else
				{
					// only hide if hiding isn't disabled
					// last usable level goes to infinity
					bWithinDist = m_LOD->DistLODSq[i] > 0 && (deltaSq > m_LOD->DistLODSq[i]);
				}

				// compute the alpha value of still inside the fade range
				if (bWithinDist)
				{
					if (m_LOD->fLODFadeOutRange > 0)
					{
						//						gameLocal.Printf ("%s outside hide_distance %0.2f (%0.2f) with fade %0.2f\n", GetName(), m_LOD->DistLODSq[i], deltaSq, m_LOD->fLODFadeOutRange);
						if (deltaSq > (m_LOD->DistLODSq[i] + m_LOD->fLODFadeOutRange))
						{
							fAlpha = 0.0f;
						}
						else
						{
							fAlpha = 1.0f - ( (deltaSq - m_LOD->DistLODSq[i]) / m_LOD->fLODFadeOutRange );
						}
						// set the timestamp so we think the next frame again to get a smooth blend:
						m_DistCheckTimeStamp = gameLocal.time - 0.1;
					}
					else
					{
						// just hide if outside
						fAlpha = 0.0f;
					}
					m_LODLevel = i;

					//					gameLocal.Printf (" %s returning LOD level %i, fAlpha = %0.2f", GetName(), i, fAlpha);

					// early out, we found the right level and switched
					return fAlpha;
				}
			}
		}

		//		gameLocal.Printf (" %s passed LOD %i distance check %f (%f), inside?: %i (old level %i, prel. alpha %0.2f)\n",
		//				GetName(), i, m_LOD->DistLODSq[i], deltaSq, bWithinDist, m_LODLevel, fAlpha);

		// do this even if we are already in the same level 
		// && m_LODLevel != i)
		if ( bWithinDist )
		{
			m_LODLevel = i;

			// LOD level number i
			// TODO: Hiding a LOD entity temp. completely would fail,
			//		 as the LOD levels < LAST_LOD_LEVEL would show it again.

			if (i == 0 && m_LOD->fLODFadeInRange > 0)
			{
				// do we need to hide the entity, or fade it?
				if (deltaSq < (m_LOD->DistLODSq[0] - m_LOD->fLODFadeInRange))
				{
					fAlpha = 0.0f;	// hide
				}
				else
				{
					fAlpha = (deltaSq - (m_LOD->DistLODSq[0] - m_LOD->fLODFadeInRange)) / m_LOD->fLODFadeOutRange;
					//				gameLocal.Printf ("%s fading in to %0.2f\n", GetName(), fAlpha);
				}
				// set the timestamp so we think the next frame again to get a smooth blend:
				m_DistCheckTimeStamp = gameLocal.time - 0.1;
			}
			else
			{
				// visible in all other levels
				fAlpha = 1.0f;	// show
			}

			//			gameLocal.Printf (" %s returning LOD level %i, fAlpha = %0.2f (step 2)", GetName(), i, fAlpha);

			// We found the right level and will switch to it
			return fAlpha;
		}

		// end for all LOD levels
	}

	//	gameLocal.Warning("%s: ThinkAboutLOD fall out of lod levels, using fAlpha = 1.0f", GetName() );

	return fAlpha;
}

/* Tels:

Call ThinkAboutLOD, then do the nec. things like calling Hide()/Show(), SetAlpha() etc.
*/
bool LodComponent::SwitchLOD()
{
	renderEntity_t *rent = m_entity->GetRenderEntity();

	// SteveL #3770: Moved the following if block and the derivation of deltaSq from idEntity::Think as it would 
	// have had to be repeated in many places. Left behind only a check that LOD is enabled before calling SwitchLOD
	const lod_data_t *m_LOD = nullptr;
	if ( m_LODHandle )
		m_LOD = gameLocal.m_ModelGenerator->GetLODDataPtr( m_LODHandle );
	//stgatilov #5683: m_LODHandle = 0 or m_LOD = NULL is only possible during hot-reload

	float deltaSq = -1;
	if ( m_LOD )
	{
		// If this entity has LOD, let it think about it:
		// Distance dependence checks
		if ( (m_LOD->DistCheckInterval > 0) && (gameLocal.time >= m_DistCheckTimeStamp) )
		{
			while (gameLocal.time >= m_DistCheckTimeStamp)
				m_DistCheckTimeStamp += m_LOD->DistCheckInterval;
			deltaSq = GetLODDistance( m_LOD, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), m_entity->GetPhysics()->GetOrigin(), rent->bounds.GetSize(), cv_lod_bias.GetFloat() );
		}
		else
		{
			return false;
		}
	}

	// remember the current level
	int oldLODLevel = m_LODLevel;
	float fAlpha = ThinkAboutLOD( m_LOD, deltaSq );

	// gameLocal.Printf("%s: Got fAlpha %0.2f\n", GetName(), fAlpha);

	if (fAlpha < 0.0001f)
	{
		if (!m_entity->fl.hidden)
		{
			// gameLocal.Printf( "%s Hiding\n", GetName() );
			m_entity->Hide();
		}
	}
	else
	{
		if (m_entity->fl.hidden)
		{
			// gameLocal.Printf("Showing %s again (%0.2f)\n", GetName(), fAlpha);
			m_entity->Show();
			m_entity->SetAlpha( fAlpha, true );
		}
		// Only set the alpha if we are actually fading, but skip if it is 1.0f
		else if (rent->shaderParms[ SHADERPARM_ALPHA ] != fAlpha)
		{
			// gameLocal.Printf("%s: Setting alpha %0.2f\n", GetName(), fAlpha);
			m_entity->SetAlpha( fAlpha, true );
		}
	}

	if (m_LODLevel != oldLODLevel)
	{
		if (m_ModelLODCur != m_LODLevel)
		{
			idStr newModelName = m_LOD ? m_LOD->ModelLOD[m_LODLevel].c_str() : m_entity->spawnArgs.GetString( "model" );
			// func_statics that have map geometry do not have a model, and their LOD data gets ""
			// as model name so they can all share the same data. However, we must not use "" when
			// setting a new model:
			if (!newModelName.IsEmpty())
			{
				m_entity->SwapLODModel( newModelName );
			}
			m_ModelLODCur = m_LODLevel;
			// Fix 1.04 blinking bug:
			// if the old LOD level had an offset, we need to revert this.
			// and if the new one has an offset, we need to add it:
			idVec3 offsetLODNew = m_LOD ? m_LOD->OffsetLOD[m_LODLevel] : idVec3( 0 );
			idVec3 originShift = offsetLODNew - m_OffsetLODCur;
			if (originShift.x != 0.0f || originShift.y != 0.0f || originShift.z != 0.0f)
			{
				m_entity->SetOrigin( rent->origin + originShift );
			}
			m_OffsetLODCur = offsetLODNew;
		}


		if (m_SkinLODCur != -1 && m_SkinLODCur != m_LODLevel) // SteveL #3744
		{
			// stgatilov #5683: m_LOD can be NULL only during hot-reload.
			// In such case, we ignore "random_skin" and default skin from modelDef
			idStr newSkinName = m_LOD ? m_LOD->SkinLOD[m_LODLevel].c_str() : m_entity->spawnArgs.GetString( "skin" );
			const idDeclSkin *skinD = declManager->FindSkin(newSkinName);
			if (skinD)
			{
				m_entity->SetSkin(skinD);
			}
			m_SkinLODCur = m_LODLevel;
		}

		bool newNoShadows;
		if ( m_LOD ) {
			// SteveL #3744 && #4170
			if ( m_LOD->noshadowsLOD != -1 )
				newNoShadows = (m_LOD->noshadowsLOD & (1 << m_LODLevel)) != 0;
			else
				newNoShadows = rent->noShadow;
		}
		else {
			//stgatilov #5683: this case only possible during hot-reload
			newNoShadows = m_entity->spawnArgs.GetBool( "noshadows" );
		}
		if ( newNoShadows != rent->noShadow )
		{
			rent->noShadow = newNoShadows;
		}


		// switched LOD
		return true;
	}

	// no switch done
	return false;
}

/*
================
LodComponent::GetLODDistance

Returns the distance that should be considered for LOD and hiding, depending on:

* the distance of the entity origin to the given player origin
(TODO: this should be actualy the distance to the closest point of the entity,
to aovid that very long entities get hidden when you are far from their origin,
but close to their corner)
* the lod-bias set in the menu
* some minimum and maximum distances based on entity size/importance

The returned value is the (virtual) distance squared, and rounded down to an integer.
================
*/
float LodComponent::GetLODDistance( const lod_data_t *m_LOD, const idVec3 &playerOrigin, const idVec3 &entOrigin, const idVec3 &entSize, const float lod_bias ) const
{
	idVec3 delta = playerOrigin - entOrigin;
	// enforce an absolute minimum of 500 units for entities w/o LOD
	float minDist = 0.0f;

	if( m_LOD && m_LOD->bDistCheckXYOnly)
	{
		// todo: allow passing in a different gravityNormal
		idVec3 vGravNorm = m_entity->GetPhysics()->GetGravityNormal();
		delta -= (vGravNorm * delta) * vGravNorm;
	}

	// let the mapper override it
	if ( m_LOD && m_LOD->fLODNormalDistance > 0)
	{
		//		gameLocal.Printf ("%s: Using %0.2f lod_normal_distance, delta %0.2f.\n", GetName(), m_LOD->fLODNormalDistance, deltaSq);
		minDist = m_LOD->fLODNormalDistance;
	}

	// multiply with the user LOD bias setting, and return the result:
	// floor the value to avoid inaccurancies leading to toggling when the player stands still:
	assert(lod_bias > 0.01f);
	float deltaSq = delta.LengthSqr();

	// if the entity is inside the "lod_normal_distance", simply ignore any LOD_BIAS < 1.0f
	// Tels: For v1.05 use at least 1.0f for lod_bias, so that any distance the mapper sets
	//       acts as the absolute minimum distance. Needs fixing later.
	//if (minDist > 0 && lod_bias < 1.0f && deltaSq < (minDist * minDist))
	if (lod_bias <= 1.0f)
	{
		deltaSq = idMath::Floor( deltaSq );
	}
	else
	{
		deltaSq = idMath::Floor( deltaSq / (lod_bias * lod_bias) );
	}

	// TODO: enforce minimum/maximum distances based on entity size/importance
	return deltaSq;
}

/*
================
LodComponent::Event_HideByLODBias

Tels: The menu setting "Object Detail" changed, so we need to check
if this entity is now hidden or visible.
*/
void LodComponent::Event_HideByLODBias( void )
{
	float lodbias = cv_lod_bias.GetFloat();

	// ignore worldspawn
	if (m_entity->IsType( idWorldspawn::Type ))
	{
		return;
	}

	if (lodbias < m_MinLODBias || lodbias > m_MaxLODBias)
	{
		// FuncPortals are closed instead of hidden
		if ( m_entity->IsType( idFuncPortal::Type ) )
		{
			//gameLocal.Printf ("%s: Closing portal due to lodbias %0.2f not being between %0.2f and %0.2f.\n",
			//		GetName(), cv_lod_bias.GetFloat(), m_MinLODBias, m_MaxLODBias);
			static_cast<idFuncPortal *>( m_entity )->ClosePortal();
		}
		else
		{
			// if a lod_hidden_skin is set, just set the new skin instead of hiding the entity
			if (!m_HiddenSkin.IsEmpty())
			{
				//				gameLocal.Printf ("%s: Setting hidden skin %s.\n", GetName(), m_HiddenSkin.c_str() );
				m_entity->Event_SetSkin( m_HiddenSkin.c_str() );
			}
			else
			{
				if (!m_entity->fl.hidden)
				{
					//					gameLocal.Printf ("%s: Hiding due to lodbias %0.2f not being between %0.2f and %0.2f.\n",
					//							GetName(), cv_lod_bias.GetFloat(), m_MinLODBias, m_MaxLODBias);
					// #4116: Post a Hide() event instead of hiding immediately as this routine is called during spawning
					m_entity->PostEventMS( &EV_Hide, 0 );
					// and make inactive
					m_entity->BecomeInactive(TH_PHYSICS|TH_THINK);
				}
			}
			// mark this entity as "hidden" for later show
			m_LODLevel = -1;
		}
		return;
	}	

	if ( m_entity->IsType( idFuncPortal::Type ) )
	{
		//gameLocal.Printf ("%s: Opening portal because lodbias %0.2f is between %0.2f and %0.2f.\n",
		//		GetName(), cv_lod_bias.GetFloat(), m_MinLODBias, m_MaxLODBias);
		static_cast<idFuncPortal *>( m_entity )->OpenPortal();
	}
	else
	{
		// do not "restore" the original skin during entity spawn
		if (!m_HiddenSkin.IsEmpty())
		{
			// just restore the orginal skin
			//			gameLocal.Printf ("%s: Restoring skin %s.\n", GetName(), m_VisibleSkin.c_str() );
			m_entity->Event_SetSkin( m_VisibleSkin.c_str() );
		}
		// avoid showing entities that where not hidden by LODBias
		else
		{
			if (m_entity->fl.hidden && m_LODLevel == -1 && m_DistCheckTimeStamp == NOLOD)
			{
				m_LODLevel = 0;
				//gameLocal.Printf ("%s: Showing due to lodbias %0.2f being between %0.2f and %0.2f.\n",
				//			GetName(), cv_lod_bias.GetFloat(), m_MinLODBias, m_MaxLODBias);
				m_entity->Show();
			}
		}
	}
}

//===================================================================================================================

void LodSystem::Clear()
{
	m_components.Clear();
}

void LodSystem::Save( idSaveGame &savefile ) const
{
	int k = 0;
	for (int i = 0; i < m_components.Num(); i++)
		k += !m_components[i].m_dead;
	savefile.WriteInt(k);

	for (int i = 0; i < m_components.Num(); i++) {
		if (m_components[i].m_dead)
			continue;
		savefile.WriteInt(m_components[i].m_entity->entityNumber);
		m_components[i].Save(savefile);
	}
}

void LodSystem::Restore( idRestoreGame &savefile )
{
	int k;
	savefile.ReadInt(k);
	m_components.SetNum(k, false);

	for (int i = 0; i < k; i++) {
		m_components[i].m_dead = false;
		int entityNum;
		savefile.ReadInt(entityNum);
		m_components[i].m_entity = gameLocal.entities[entityNum];
		m_components[i].Restore(savefile);
		m_components[i].m_entity->lodIdx = i;
	}
}

void LodSystem::AddToEnd(const LodComponent &component)
{
	assert(component.m_entity);
	assert(FindEntity(component.m_entity) < 0);
	int idx = m_components.AddGrow(component);
	component.m_entity->lodIdx = idx;
}

void LodSystem::Remove(int index)
{
	assert(index >= 0);
	LodComponent& comp = m_components[index];
	//mark as dead, so that this element is ignored during enumeration
	comp.m_dead = true;
	//reset most important members (just in case)
	comp.m_entity = NULL;
	comp.m_LODHandle = 0;
	comp.m_DistCheckTimeStamp = 0xDDDDDDDD;
}

int LodSystem::FindEntity(idEntity *entity) const
{
	for (int j = 0; j < m_components.Num(); j++)
	{
		const LodComponent &lodComp = m_components[j];
		if (lodComp.m_dead)
			continue;

		if (lodComp.m_entity == entity)
			return j;
	}
	return -1;
}

void LodSystem::ThinkAllLod()
{
	TRACE_CPU_SCOPE( "CheckLOD" )

	int gameTime = gameLocal.time;
	for (int j = 0; j < m_components.Num(); j++)
	{
		LodComponent &lodComp = m_components[j];
		if (lodComp.m_dead)
			continue;

		if (gameTime >= lodComp.m_DistCheckTimeStamp)
		{
			assert(lodComp.m_DistCheckTimeStamp != LodComponent::NOLOD);
			if (lodComp.SwitchLOD())
				lodComp.m_entity->BecomeActive( TH_UPDATEVISUALS );
		}
	}
}

void LodSystem::UpdateAfterLodBiasChanged()
{
	for (int j = 0; j < m_components.Num(); j++)
	{
		if (m_components[j].m_dead)
			continue;
		m_components[j].Event_HideByLODBias();
	}
}

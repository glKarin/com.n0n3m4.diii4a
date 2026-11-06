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



#include "Game_local.h"
#include "BloodMarker.h"
#include "StimResponse/Stim.h"

#include "game/LightEstimateSystem.h"

const idEventDef EV_GenerateBloodSplat("_TDM_GenerateBloodSplat", EventArgs(), EV_RETURNS_VOID, "internal");

CLASS_DECLARATION( idEntity, CBloodMarker )
	EVENT( EV_GenerateBloodSplat, CBloodMarker::Event_GenerateBloodSplat )
END_CLASS

void CBloodMarker::Event_GenerateBloodSplat()
{
	idVec3 dir = gameLocal.GetGravity();
	dir.Normalize();

	ProjectDecalParams params;
	params.origin = GetPhysics()->GetOrigin();
	params.dir = dir;
	params.depth = 3.0f;
	params.parallel = false;
	params.size = _size;
	params.angle = _angle;
	params.randomizeAngle = false;

	if (!_isFading)
	{
		// Read the stay duration from the material info
		const idMaterial* material = declManager->FindMaterial(_bloodSplat, false); // grayman #3391 - no default, please

		if (material != NULL)
		{
			params.material = _bloodSplat;
			gameLocal.ProjectDecal( params );

			PostEventMS(&EV_GenerateBloodSplat, material->GetDecalInfo().stayTime);
		}
		else 
		{
			gameLocal.Warning("Cannot find blood splat decal %s", _bloodSplat.c_str());
		}
	}
	else
	{
		// We're fading, just spawn one last decal and schedule our removal
		params.material = _bloodSplatFading;
		gameLocal.ProjectDecal( params );

		// grayman #3075 - notify the AI who spilled the blood that
		// we're going away.

		if ( _spilledBy.IsValid() )
		{
			_spilledBy.GetEntity()->SetBlood(NULL);
			_spilledBy = NULL;
		}

		PostEventMS(&EV_Remove, 1000);
	}

	if ( LightEstimateSystem *les = gameLocal.m_LightEstimateSystem ) {
		// generate explicit samples for LES
		idVec3 axisU, axisV;
		params.dir.NormalVectors( axisU, axisV );

		idList<idVec3> positions;
		for ( int i = 0; i <= 2; i++ ) {
			float radius = (params.size * 0.5f) * i / 2;
			int n = 1 + 6 * i;
			for ( int i = 0; i < n; i++ ) {
				float s, c;
				idMath::SinCos( idMath::TWO_PI * i / n, s, c);
				positions.Append( params.origin + radius * ( axisU * c + axisV * s ) );
			}
		}

		// transform to model coords
		for ( idVec3 &point : positions )
			point = ( point - GetPhysics()->GetOrigin() ) * GetPhysics()->GetAxis().Transpose();

		les->SetExplicitSamplingForEntity( this, &positions );
	}
}

CBloodMarker::CBloodMarker() {
	_angle = 0.0f;
	_size = 0.0f;
	_isFading = false;
	_spilledBy = nullptr;
}

void CBloodMarker::Init(const idStr& splat, const idStr& splatFading, float size, idAI* bleeder) // grayman #3075 - add who bled
{
	_bloodSplat = splat;
	_bloodSplatFading = splatFading;

	// randomly rotate the decal winding
	_angle = gameLocal.random.RandomFloat() * idMath::TWO_PI;
	_size = size;
	_isFading = false;

	// grayman #3075 - note who spilled this blood
	_spilledBy = bleeder;
	bleeder->SetBlood(this);

	AddResponse(ST_WATER);
	EnableResponse(ST_WATER);
}

void CBloodMarker::OnStim(const CStimPtr& stim, idEntity* stimSource)
{
	// Call the base class in any case
	idEntity::OnStim(stim, stimSource);

	if (stim->m_StimTypeId == ST_WATER)
	{
		_isFading = true;
	}
}

// grayman #3075

idAI* CBloodMarker::GetSpilledBy(void)
{
	return _spilledBy.GetEntity();
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Save( idSaveGame *savefile ) const
{
	savefile->WriteString(_bloodSplat);
	savefile->WriteString(_bloodSplatFading);
	savefile->WriteFloat(_angle);
	savefile->WriteFloat(_size);
	savefile->WriteBool(_isFading);
	_spilledBy.Save(savefile); // grayman #3075
}

//-----------------------------------------------------------------------------------

void CBloodMarker::Restore( idRestoreGame *savefile )
{
	savefile->ReadString(_bloodSplat);
	savefile->ReadString(_bloodSplatFading);
	savefile->ReadFloat(_angle);
	savefile->ReadFloat(_size);
	savefile->ReadBool(_isFading);
	_spilledBy.Restore(savefile); // grayman #3075
}

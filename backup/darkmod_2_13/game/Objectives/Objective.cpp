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



#include "Objective.h"
#include "MissionData.h"

/**==========================================================================
* CObjective
*==========================================================================**/

CObjective::CObjective()
{
	Clear();
}

CObjective::~CObjective()
{
	Clear();
}

void CObjective::Clear()
{
	m_ObjNum = -1;
	m_state = STATE_INCOMPLETE;
	m_text = "";
	m_bNeedsUpdate = false;
	m_bMandatory = false;
	m_bReversible = true;
	m_bLatched = false;
	m_bVisible = true;
	m_bOngoing = false;
	m_bApplies = true;
	m_handle = 0;
	m_Components.Clear();
	m_EnablingObjs.Clear();
	m_CompletionTarget.Clear();
	m_FailureTarget.Clear();
	m_CompletionScript.Clear();
	m_FailureScript.Clear();
	m_SuccessLogicStr.Clear();
	m_FailureLogicStr.Clear();
	m_SuccessLogic.Clear();
	m_FailureLogic.Clear();
}

// =============== Boolean Logic Parsing for Objective Failure/Success ==============

void CObjective::Save( idSaveGame *savefile ) const
{
	savefile->WriteString( m_text );
	savefile->WriteBool( m_bMandatory );
	savefile->WriteBool( m_bVisible );
	savefile->WriteBool( m_bOngoing );
	savefile->WriteBool( m_bApplies );
	savefile->WriteInt( m_ObjNum );
	savefile->WriteInt( m_handle );
	savefile->WriteInt( m_state );
	savefile->WriteBool( m_bNeedsUpdate );
	savefile->WriteBool( m_bReversible );
	savefile->WriteBool( m_bLatched );

	savefile->WriteInt( m_Components.Num() );
	for( int i=0; i < m_Components.Num(); i++ )
		m_Components[i].Save( savefile );

	savefile->WriteInt( m_EnablingObjs.Num() );
	for( int j=0; j < m_EnablingObjs.Num(); j++ )
		savefile->WriteInt( m_EnablingObjs[j] );

	savefile->WriteString(m_CompletionTarget);
	savefile->WriteString(m_FailureTarget);

	savefile->WriteString( m_CompletionScript );
	savefile->WriteString( m_FailureScript );
	savefile->WriteString( m_SuccessLogicStr );
	savefile->WriteString( m_FailureLogicStr );
}

void CObjective::Restore( idRestoreGame *savefile )
{
	int num(0), tempInt(0);
	savefile->ReadString( m_text );
	savefile->ReadBool( m_bMandatory );
	savefile->ReadBool( m_bVisible );
	savefile->ReadBool( m_bOngoing );
	savefile->ReadBool( m_bApplies );
	savefile->ReadInt( m_ObjNum );
	savefile->ReadInt( m_handle );
	savefile->ReadInt( tempInt );
	m_state = (EObjCompletionState) tempInt;
	savefile->ReadBool( m_bNeedsUpdate );
	savefile->ReadBool( m_bReversible );
	savefile->ReadBool( m_bLatched );

	savefile->ReadInt( num );
	m_Components.SetNum( num );
	for( int i=0; i < num; i++ )
		m_Components[i].Restore( savefile );

	savefile->ReadInt( num );
	m_EnablingObjs.SetNum( num );
	for( int j=0; j < num; j++ )
		savefile->ReadInt( m_EnablingObjs[j] );

	savefile->ReadString(m_CompletionTarget);
	savefile->ReadString(m_FailureTarget);

	savefile->ReadString( m_CompletionScript );
	savefile->ReadString( m_FailureScript );
	savefile->ReadString( m_SuccessLogicStr );
	savefile->ReadString( m_FailureLogicStr );

	// We have to re-parse the logic since the parse nodes involve raw pointer linkages
	ParseLogicStrs();
}

bool CObjective::CheckFailure()
{
	bool bTest(false);

	if( !m_FailureLogic.IsEmpty() )
		bTest = gameLocal.m_MissionData->EvalBoolLogic( &m_FailureLogic, false, m_ObjNum );
	else
	{
		// Default logic: If ANY components of an ongoing objective are false, the objective is failed
		if( m_bOngoing && !(m_state == STATE_INVALID) )
		{
			bTest = true;
			for( int j=0; j < m_Components.Num(); j++ )
			{
				bTest = bTest && m_Components[j].m_bState;
			}

			bTest = !bTest;
		}
	}
	return bTest;
}

bool CObjective::CheckSuccess()
{
	bool bTest(true);
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Check Success Called \r");

	if( !m_SuccessLogic.IsEmpty() )
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Evaluating custom success logic \r");
		bTest = gameLocal.m_MissionData->EvalBoolLogic( &m_SuccessLogic, false, m_ObjNum );
	}
	else
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Evaluating default success logic \r");
		// Default logic: All components must be true to succeed
		for( int j=0; j < m_Components.Num(); j++ )
		{
			bTest = bTest && m_Components[j].m_bState;
		}
	}
	return bTest;
}

bool CObjective::ParseLogicStrs()
{
	bool bReturnVal(true), bTemp(false);

	if( m_SuccessLogicStr != "" )
	{
		bReturnVal = gameLocal.m_MissionData->ParseLogicStr( &m_SuccessLogicStr, &m_SuccessLogic );
		
		if( !bReturnVal )
			gameLocal.Error("Objective success logic failed to parse \n");
	}

	if( m_FailureLogicStr != "" )
	{
		bTemp = gameLocal.m_MissionData->ParseLogicStr( &m_FailureLogicStr, &m_FailureLogic );
		
		if( !bTemp )
			gameLocal.Error("Objective failure logic failed to parse \n");

		bReturnVal = bReturnVal && bTemp;
	}

	return bReturnVal;
}

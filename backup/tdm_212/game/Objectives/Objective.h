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

#ifndef TDM_OBJECTIVE_H
#define TDM_OBJECTIVE_H

#include "precompiled.h"

#include "BoolParseNode.h"
#include "ObjectiveComponent.h"

/**
 * Objective completion states
 * NOTE: STATE_INVALID may also be used for initially deactivating objectives, 
 * then activating later by setting STATE_INCOMPLETE
 **/
enum EObjCompletionState
{
	STATE_INCOMPLETE,
	STATE_COMPLETE,
	STATE_INVALID, 
	STATE_FAILED
};

/**
* Class for storing objective data, containing
* all the objective components
*/
class CObjective
{
public:
	friend class CObjectiveComponent;
	friend class CMissionData;

	CObjective();
	virtual ~CObjective();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	void Clear();

	/**
	* Evaluate the boolean relationships for objective failure and success
	**/
	bool CheckFailure();
	bool CheckSuccess();

	/**
	* Parse m_SuccessLogicStr and m_FailureLogicStr into boolean evaluation
	* matrices to be evaluated by CheckFailure and CheckSuccess.
	* Returns false if the logic parse failed
	*
	* This should be run after CMissionData parsing sets those two strings
	**/
	bool ParseLogicStrs();

public:
	/** 
	* Text description of the objective in the objectives GUI
	**/
	idStr m_text;

	/** 
	* Set to false if an objective is optional
	**/
	bool m_bMandatory;

	/**
	* Sets whether the objective is shown in the objectives screen
	**/
	bool m_bVisible;
	
	/**
	* True if an objective is ongoing throughout the mission.
	* Will not be checked off as complete until the mission is over
	**/
	bool m_bOngoing;

	/**
	* True if this objective applies to the current skill level. Otherwise
	* the objective can be ignored.
	**/
	bool m_bApplies;

protected:

	/**
	* Integer index of this objective in the array
	**/
	int	m_ObjNum;

	/**
	* Handle for the FM author to refer to this objective (Not Yet Implemented)
	**/
	int m_handle;

	/**
	* Completion state.  Either COMP_INCOMPLETE, COMP_COMPLETE, COMP_FAILED or COMP_INVALID
	**/
	EObjCompletionState	m_state;

	/**
	* Set to true if one of the components changed this frame.  Test resets it to false.
	*/
	bool m_bNeedsUpdate;

	/**
	* Whether the objective may change state again once it initially changes to FAILED or SUCCESSFUL
	* Default is reversible.
	**/
	bool m_bReversible;

	/**
	* Set to true if the objective is irreversible and has latched into a state
	**/
	bool m_bLatched;

	/**
	* List of objective components (steal this, kill that, etc)
	**/
	idList<CObjectiveComponent> m_Components;

	/**
	* Other objectives that must be completed prior to the completion of this objective
	**/
	idList<int> m_EnablingObjs;

	/**
	 * greebo: These define the names of entities which should be triggered
	 * as soon as the objective completes or fails.
	 */
	idStr m_CompletionTarget;
	idStr m_FailureTarget;

	/**
	* String storing the script to call when this objective is completed
	* (optional)
	**/
	idStr m_CompletionScript;

	/**
	* String storing the script to call when this objective is failed
	* (optional)
	**/
	idStr m_FailureScript;
	
	/**
	* Success and failure logic strings.  Used to reload the parse matrix on save/restore
	**/
	idStr m_SuccessLogicStr;
	idStr m_FailureLogicStr;

	/**
	* Success and failure boolean parsing matrices
	**/
	SBoolParseNode m_SuccessLogic;
	SBoolParseNode m_FailureLogic;
};

#endif 

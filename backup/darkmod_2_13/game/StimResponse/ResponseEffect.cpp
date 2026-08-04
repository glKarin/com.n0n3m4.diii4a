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



#include "ResponseEffect.h"

/********************************************************************/
/*                 CResponseEffect                                  */
/********************************************************************/

CResponseEffect::CResponseEffect(
		const function_t* scriptFunction,
		const idStr& effectPostfix,
		const idStr& scriptName	,
		bool localScript) :
	_scriptFunction(scriptFunction),
	_scriptName(scriptName),
	_effectPostfix(effectPostfix),
	_localScript(localScript),
	_scriptFunctionValid(true)
{}

void CResponseEffect::runScript(idEntity* owner, idEntity* stimEntity, float magnitude) {
	if (!_scriptFunctionValid)
	{
		if (owner == NULL)
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Cannot restore scriptfunction, owner is NULL: %s\r", _scriptName.c_str());
			return;
		}

		_scriptFunctionValid = true;

		if (_localScript) {
			// Local scriptfunction
			_scriptFunction = owner->scriptObject.GetFunction(_scriptName.c_str());
		}
		else {
			// Global Method
			_scriptFunction = gameLocal.program.FindFunction(_scriptName.c_str());
		}
	}

	if ( _scriptFunction == NULL )
	{
		return;
	}

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Running ResponseEffect Script %s, effectPostfix = %s...\r", _scriptFunction->Name(), _effectPostfix.c_str());

	idThread *pThread = new idThread(_scriptFunction);
	int n = pThread->GetThreadNum();
	pThread->CallFunctionArgs(_scriptFunction, true, "eesfd", owner, stimEntity, _effectPostfix.c_str(), magnitude, n);
	pThread->DelayedStart(0);
}

void CResponseEffect::Save(idSaveGame *savefile) const
{
	savefile->WriteString(_effectPostfix.c_str());
	savefile->WriteString(_scriptName.c_str());
	savefile->WriteBool(_localScript);
}

void CResponseEffect::Restore(idRestoreGame *savefile)
{
	savefile->ReadString(_effectPostfix);
	savefile->ReadString(_scriptName);
	savefile->ReadBool(_localScript);

	// The script function pointer has to be restored after loading, set the dirty flag
	_scriptFunctionValid = false;
}

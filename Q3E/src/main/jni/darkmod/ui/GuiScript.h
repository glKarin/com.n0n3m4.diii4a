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
#ifndef __GUISCRIPT_H
#define __GUISCRIPT_H

#include "Winvar.h"
#include "Window.h"

struct idGSWinVar {
	idGSWinVar() {
		var = NULL;
		own = false;
	}
	void RelinkVar(idWinVar *newVar, bool newOwn) {
		if (own && var) {
			assert(var != newVar);
			delete var;
		}
		var = newVar;
		own = newOwn;
	}
	idWinVar* var;
	bool own;
};

class idGuiScriptList;

class idGuiScript {
	friend class idGuiScriptList;
	friend class idWindow;

public:
	idGuiScript();
	~idGuiScript();

	void SetSourceLocation(const idGuiSourceLocation &loc) { srcLocation = loc; }
	const idGuiSourceLocation &GetSourceLocation() const { return srcLocation; }
	idStr GetSrcLocStr() const;

	bool Parse(idParser *src, idWindow *win);
	void Execute(idWindow *win) {
		if (handler) {
			handler(this, win, &parms);
		}
	}
	void FixupParms(idWindow *win);
	size_t Size() {
		size_t sz = sizeof(*this);
		for (int i = 0; i < parms.Num(); i++) {
			sz += parms[i].var->Size();
		}
		return sz;
	}

	void WriteToSaveGame( idFile *savefile );
	void ReadFromSaveGame( idFile *savefile );

protected:
	int conditionReg;
	idGuiScriptList *ifList;
	idGuiScriptList *elseList;
	idList<idGSWinVar> parms;
	void (*handler) (idGuiScript *self, idWindow *window, idList<idGSWinVar> *src);
	//stgatilov: error reporting and debuggability
	idGuiSourceLocation srcLocation;	//points into owner's idWindow::sourceFilenamePool
};


class idGuiScriptList {
	idList<idGuiScript*> list;
public:
	idGuiScriptList() { list.SetGranularity( 4 ); };
	~idGuiScriptList() { list.DeleteContents(true); };
	void Execute(idWindow *win);
	void Append(idGuiScript* gs) {
		list.Append(gs);
	}
	size_t Size() {
		size_t sz = sizeof(*this);
		for (int i = 0; i < list.Num(); i++) {
			sz += list[i]->Size();
		}
		return sz;
	}
	void FixupParms(idWindow *win);
	void ReadFromDemoFile( class idDemoFile *f ) {};
	void WriteToDemoFile( class idDemoFile *f ) {};

	void WriteToSaveGame( idFile *savefile );
	void ReadFromSaveGame( idFile *savefile );
};

#endif // __GUISCRIPT_H

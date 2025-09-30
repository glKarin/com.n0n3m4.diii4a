/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Window.h"
#include "Winvar.h"
#include "GuiScript.h"
#include "UserInterfaceLocal.h"


/*
=========================
Script_Set
=========================
*/
void Script_Set(idWindow *window, idList<idGSWinVar> *src)
{
	idStr key, val;
	idWinStr *dest = dynamic_cast<idWinStr *>((*src)[0].var);

	if (dest) {
		if (idStr::Icmp(*dest, "cmd") == 0) {
			dest = dynamic_cast<idWinStr *>((*src)[1].var);
			int parmCount = src->Num();

			if (parmCount > 2) {
				val = dest->c_str();
				int i = 2;

				while (i < parmCount) {
					val += " \"";
					val += (*src)[i].var->c_str();
					val += "\"";
					i++;
				}

				window->AddCommand(val);
			} else {
				window->AddCommand(*dest);
			}

			return;
		}
	}

	(*src)[0].var->Set((*src)[1].var->c_str());
	(*src)[0].var->SetEval(false);
}

/*
=========================
Script_SetFocus
=========================
*/
void Script_SetFocus(idWindow *window, idList<idGSWinVar> *src)
{
	idWinStr *parm = dynamic_cast<idWinStr *>((*src)[0].var);

	if (parm) {
		drawWin_t *win = window->GetGui()->GetDesktop()->FindChildByName(*parm);

		if (win && win->win) {
			window->SetFocus(win->win);
		}
	}
}

/*
=========================
Script_ShowCursor
=========================
*/
void Script_ShowCursor(idWindow *window, idList<idGSWinVar> *src)
{
	idWinStr *parm = dynamic_cast<idWinStr *>((*src)[0].var);

	if (parm) {
		if (atoi(*parm)) {
			window->GetGui()->GetDesktop()->ClearFlag(WIN_NOCURSOR);
		} else {
			window->GetGui()->GetDesktop()->SetFlag(WIN_NOCURSOR);
		}
	}
}

/*
=========================
Script_RunScript

 run scripts must come after any set cmd set's in the script
=========================
*/
void Script_RunScript(idWindow *window, idList<idGSWinVar> *src)
{
	idWinStr *parm = dynamic_cast<idWinStr *>((*src)[0].var);

	if (parm) {
		idStr str = window->cmd;
		str += " ; runScript ";
		str += parm->c_str();
		window->cmd = str;
	}
}

/*
=========================
Script_LocalSound
=========================
*/
void Script_LocalSound(idWindow *window, idList<idGSWinVar> *src)
{
	idWinStr *parm = dynamic_cast<idWinStr *>((*src)[0].var);

	if (parm) {
		session->sw->PlayShaderDirectly(*parm);
	}
}

/*
=========================
Script_EvalRegs
=========================
*/
void Script_EvalRegs(idWindow *window, idList<idGSWinVar> *src)
{
	window->EvalRegs(-1, true);
}

/*
=========================
Script_EndGame
=========================
*/
void Script_EndGame(idWindow *window, idList<idGSWinVar> *src)
{
	cvarSystem->SetCVarBool("g_nightmare", true);
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "disconnect\n");
}

/*
=========================
Script_ResetTime
=========================
*/
void Script_ResetTime(idWindow *window, idList<idGSWinVar> *src)
{
	idWinStr *parm = dynamic_cast<idWinStr *>((*src)[0].var);
	drawWin_t *win = NULL;

	if (parm && src->Num() > 1) {
		win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		parm = dynamic_cast<idWinStr *>((*src)[1].var);
	}

	if (win && win->win) {
		win->win->ResetTime(atoi(*parm));
		win->win->EvalRegs(-1, true);
	} else {
		window->ResetTime(atoi(*parm));
		window->EvalRegs(-1, true);
	}
}

/*
=========================
Script_ResetCinematics
=========================
*/
void Script_ResetCinematics(idWindow *window, idList<idGSWinVar> *src)
{
	window->ResetCinematics();
}

/*
=========================
Script_Transition
=========================
*/
void Script_Transition(idWindow *window, idList<idGSWinVar> *src)
{
	// transitions always affect rect or vec4 vars
	if (src->Num() >= 4) {
		idWinRectangle *rect = NULL;
		idWinVec4 *vec4 = dynamic_cast<idWinVec4 *>((*src)[0].var);
		//
		//  added float variable
		idWinFloat *val = NULL;
#ifdef _RAVEN
        idWinFloatPtr* valp = NULL;
#endif

		//
		if (vec4 == NULL) {
			rect = dynamic_cast<idWinRectangle *>((*src)[0].var);

			//
			//  added float variable
			if (NULL == rect) {
				val = dynamic_cast<idWinFloat *>((*src)[0].var);
			}

			//
		}

#ifdef _RAVEN
        if (val == NULL)
        {
            valp = dynamic_cast<idWinFloatPtr*>((*src)[0].var);
        }
#endif

		idWinVec4 *from = dynamic_cast<idWinVec4 *>((*src)[1].var);
		idWinVec4 *to = dynamic_cast<idWinVec4 *>((*src)[2].var);
		idWinStr *timeStr = dynamic_cast<idWinStr *>((*src)[3].var);

		//
		//  added float variable
#ifdef _RAVEN
        if (!((vec4 || rect || val || valp) && from && to && timeStr))
#else
		if (!((vec4 || rect || val) && from && to && timeStr))
#endif
		{
			//
			common->Warning("Bad transition in gui %s in window %s\n", window->GetGui()->GetSourceFile(), window->GetName());
			return;
		}

		int time = atoi(*timeStr);
		float ac = 0.0f;
		float dc = 0.0f;

		if (src->Num() > 4) {
			idWinStr *acv = dynamic_cast<idWinStr *>((*src)[4].var);
			idWinStr *dcv = dynamic_cast<idWinStr *>((*src)[5].var);
			assert(acv && dcv);
			ac = atof(*acv);
			dc = atof(*dcv);
		}

		if (vec4) {
			vec4->SetEval(false);
			window->AddTransition(vec4, *from, *to, time, ac, dc);
			//
			//  added float variable
		} else if (val) {
			val->SetEval(false);
			window->AddTransition(val, *from, *to, time, ac, dc);
			//
		}
#ifdef _RAVEN
        else if (valp)
        {
            valp->SetEval(false);
            window->AddTransition(valp, *from, *to, time, ac, dc);
            //
        }
#endif
		else {
			rect->SetEval(false);
			window->AddTransition(rect, *from, *to, time, ac, dc);
		}

		window->StartTransition();
	}
}

#ifdef _RAVEN
/*
=========================
Script_NamedEvent
=========================
*/
// jmarshall - Quake 4 gui implementation
void Script_NamedEvent(idWindow* window, idList<idGSWinVar>* src)
{
    idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idStr parmStr = parm->c_str();

    int p = idStr::FindText(parm->c_str(), "::");
    if (p <= 0)
    {
        // window->RunNamedEvent(parm->c_str()); (version 1)
		window->GetGui()->HandleNamedEvent(parm->c_str()); //k: Q4D 2025
    }
    else
    {
        idStr windowName = parmStr.Left(p);
        idStr varName = parmStr.Right(parmStr.Length() - (p + 2));

        //k drawWin_t* childWindow = window->FindChildByName(windowName);
        drawWin_t* childWindow = window->GetGui()->GetDesktop()->FindChildByName(windowName);
        if (childWindow && childWindow->win/* support idSimpleWindow now */)
        {
            childWindow->win->RunNamedEvent(varName);
        }
        else
        {
            common->Warning("GUI: %s: unknown window %s for named event %s\n", window->GetName(), windowName.c_str(), varName.c_str());
        }
    }
}

/*
=========================
Script_StopTransitions
=========================
*/
void Script_StopTransitions(idWindow* window, idList<idGSWinVar>* src)
{
    idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idStr parmStr = parm->c_str();

    //k drawWin_t* childWindow = window->FindChildByName(parmStr);
    drawWin_t* childWindow = window->GetGui()->GetDesktop()->FindChildByName(parmStr);
    if (childWindow && childWindow->win/* support idSimpleWindow now */)
    {
        childWindow->win->ClearTransitions();
    }
}

/*
=========================
Script_ConsoleCmd
=========================
*/
void Script_ConsoleCmd(idWindow* window, idList<idGSWinVar>* src)
{
    idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idStr parmStr = parm->c_str();

    //cmdSystem->BufferCommandText(CMD_EXEC_NOW, parmStr.c_str()); //k: change to append, start game is work
    cmdSystem->BufferCommandText(CMD_EXEC_APPEND, parmStr.c_str());
}

/*
===================
Script_ResetVideo
===================
*/
void Script_ResetVideo(idWindow* window, idList<idGSWinVar>* src)
{
    idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idStr parmStr = parm->c_str();
    drawWin_t* childWindow = window->FindChildByName(parmStr);
    if (childWindow)
    {
        if (childWindow->win)
        {
            childWindow->win->ResetCinematics();
            childWindow->win->EvalRegs(-1, true);
        }
        else // if(childWindow->simp)
        {
            childWindow->simp->ResetCinematics();
        }
    }
    else
    {
        window->ResetCinematics();
        window->EvalRegs(-1, true);
    }
}

/*
===================
Script_NonInteractive
===================
*/
void Script_NonInteractive(idWindow* window, idList<idGSWinVar>* src)
{
    //idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idWinVar* parm = (*src)[0].var;
    int val = atoi(parm->c_str());

    window->GetGui()->SetInteractive(!(val != 0)); //k: jmarshall is (val == 1)
}
// jmarshall end

/*
=========================
Script_SetLightColor
=========================
*/
void Script_SetLightColor(idWindow* window, idList<idGSWinVar>* src)
{
    idWinVar* parm = (*src)[0].var;
	if(dynamic_cast<idWinStr*>(parm))
	{
		idStr parmStr = parm->c_str();
		int p = idStr::FindText(parmStr.c_str(), "::");
		if (p <= 0)
		{
			parm = window->GetWinVarByName(parmStr.c_str(), false);
		}
		else
		{
			idStr windowName = parmStr.Left(p);
			idStr varName = parmStr.Right(parmStr.Length() - (p + 2));

			//k drawWin_t* childWindow = window->FindChildByName(windowName);
			drawWin_t* childWindow = window->GetGui()->GetDesktop()->FindChildByName(windowName);
			if(childWindow)
			{
				if(childWindow->win)
					parm = childWindow->win->GetWinVarByName(varName, false);
				else if(childWindow->simp)
					parm = childWindow->win->GetWinVarByName(varName);
			}
		}
	}

    idWinVec4* parmV4 = dynamic_cast<idWinVec4*>(parm);
	if(parmV4)
		window->GetGui()->SetLightColor((idVec4)*parmV4);
}

#endif

#ifdef _HUMANHEAD
/*
=========================
Script_NamedEvent
=========================
*/
void Script_NamedEvent(idWindow* window, idList<idGSWinVar>* src)
{
    idWinStr* parm = dynamic_cast<idWinStr*>((*src)[0].var);
    idStr parmStr = parm->c_str();

    int p = idStr::FindText(parm->c_str(), "::");
    if (p <= 0)
    {
        window->RunNamedEvent(parm->c_str());
    }
    else
    {
        idStr windowName = parmStr.Mid(0, p);
        idStr varName = parmStr.Mid(p + 2, parmStr.Length() - (p + 2));

        //k drawWin_t* childWindow = window->FindChildByName(windowName);
        drawWin_t* childWindow = window->GetGui()->GetDesktop()->FindChildByName(windowName);
        if (childWindow && childWindow->win/* support idSimpleWindow now */)
        {
            childWindow->win->RunNamedEvent(varName);
        }
        else
        {
            common->Warning("GUI: %s: unknown window %s for named event %s\n", window->GetName(), windowName.c_str(), varName.c_str());
        }
    }
}

void Script_ResetCapture(idWindow* window, idList<idGSWinVar>* src)
{
	(void)window;
	(void)src;
}

void Script_Inc(idWindow* window, idList<idGSWinVar>* src)
{
	idStr key, val;

	int a = atoi((*src)[0].var->c_str());
	int b = atoi((*src)[1].var->c_str());

	(*src)[0].var->Set(va("%d", a + b));
	(*src)[0].var->SetEval(false);
}
#endif

typedef struct {
	const char *name;
	void (*handler)(idWindow *window, idList<idGSWinVar> *src);
	int mMinParms;
	int mMaxParms;
} guiCommandDef_t;

guiCommandDef_t commandList[] = {
	{ "set", Script_Set, 2, 999 },
	{ "setFocus", Script_SetFocus, 1, 1 },
	{ "endGame", Script_EndGame, 0, 0 },
	{ "resetTime", Script_ResetTime, 0, 2 },
	{ "showCursor", Script_ShowCursor, 1, 1 },
	{ "resetCinematics", Script_ResetCinematics, 0, 2 },
	{ "transition", Script_Transition, 4, 6 },
	{ "localSound", Script_LocalSound, 1, 1 },
	{ "runScript", Script_RunScript, 1, 1 },
	{ "evalRegs", Script_EvalRegs, 0, 0 }
#ifdef _RAVEN // quake4 gui cmd
// jmarshall - Quake 4 gui implementation
    , { "namedevent", Script_NamedEvent, 1, 1},
    { "stoptransitions", Script_StopTransitions, 1, 1},
    { "consolecmd", Script_ConsoleCmd, 1, 1},
    { "resetVideo", Script_ResetVideo, 1, 1},
    { "nonInteractive", Script_NonInteractive, 1, 1}
// jmarshall end
    , { "setlightcolor", Script_SetLightColor, 1, 1},
#endif

#ifdef _HUMANHEAD
    , { "namedevent", Script_NamedEvent, 1, 1},
    { "resetCapture", Script_ResetCapture, 1, 1},
    { "inc", Script_Inc, 2, 2},
#endif
};

int	scriptCommandCount = sizeof(commandList) / sizeof(guiCommandDef_t);


/*
=========================
idGuiScript::idGuiScript
=========================
*/
idGuiScript::idGuiScript()
{
	ifList = NULL;
	elseList = NULL;
	conditionReg = -1;
	handler = NULL;
	parms.SetGranularity(2);
}

/*
=========================
idGuiScript::~idGuiScript
=========================
*/
idGuiScript::~idGuiScript()
{
	delete ifList;
	delete elseList;
	int c = parms.Num();

	for (int i = 0; i < c; i++) {
		if (parms[i].own) {
			delete parms[i].var;
		}
	}
}

/*
=========================
idGuiScript::WriteToSaveGame
=========================
*/
void idGuiScript::WriteToSaveGame(idFile *savefile)
{
	int i;

	if (ifList) {
		ifList->WriteToSaveGame(savefile);
	}

	if (elseList) {
		elseList->WriteToSaveGame(savefile);
	}

	savefile->Write(&conditionReg, sizeof(conditionReg));

	for (i = 0; i < parms.Num(); i++) {
		if (parms[i].own) {
			parms[i].var->WriteToSaveGame(savefile);
		}
	}
}

/*
=========================
idGuiScript::ReadFromSaveGame
=========================
*/
void idGuiScript::ReadFromSaveGame(idFile *savefile)
{
	int i;

	if (ifList) {
		ifList->ReadFromSaveGame(savefile);
	}

	if (elseList) {
		elseList->ReadFromSaveGame(savefile);
	}

	savefile->Read(&conditionReg, sizeof(conditionReg));

	for (i = 0; i < parms.Num(); i++) {
		if (parms[i].own) {
			parms[i].var->ReadFromSaveGame(savefile);
		}
	}
}

/*
=========================
idGuiScript::Parse
=========================
*/
bool idGuiScript::Parse(idParser *src)
{
	int i;

	// first token should be function call
	// then a potentially variable set of parms
	// ended with a ;
	idToken token;

	if (!src->ReadToken(&token)) {
		src->Error("Unexpected end of file");
		return false;
	}

	handler	= NULL;

	for (i = 0; i < scriptCommandCount ; i++) {
		if (idStr::Icmp(token, commandList[i].name) == 0) {
			handler = commandList[i].handler;
			break;
		}
	}

	if (handler == NULL) {
		src->Error("Unknown script call %s", token.c_str());
	}

	// now read parms til ;
	// all parms are read as idWinStr's but will be fixed up later
	// to be proper types
	while (1) {
		if (!src->ReadToken(&token)) {
			src->Error("Unexpected end of file");
			return false;
		}

		if (idStr::Icmp(token, ";") == 0) {
			break;
		}

		if (idStr::Icmp(token, "}") == 0) {
			src->UnreadToken(&token);
			break;
		}

		idWinStr *str = new idWinStr();
		*str = token;
		idGSWinVar wv;
		wv.own = true;
		wv.var = str;
		parms.Append(wv);
	}

	//
	//  verify min/max params
	if (handler && (parms.Num() < commandList[i].mMinParms || parms.Num() > commandList[i].mMaxParms)) {
		src->Error("incorrect number of parameters for script %s", commandList[i].name);
	}

	//

	return true;
}

/*
=========================
idGuiScriptList::Execute
=========================
*/
void idGuiScriptList::Execute(idWindow *win)
{
	int c = list.Num();

	for (int i = 0; i < c; i++) {
		idGuiScript *gs = list[i];
		assert(gs);

		if (gs->conditionReg >= 0) {
			if (win->HasOps()) {
				float f = win->EvalRegs(gs->conditionReg);

				if (f) {
					if (gs->ifList) {
						win->RunScriptList(gs->ifList);
					}
				} else if (gs->elseList) {
					win->RunScriptList(gs->elseList);
				}
			}
		}

		gs->Execute(win);
	}
}

/*
=========================
idGuiScriptList::FixupParms
=========================
*/
void idGuiScript::FixupParms(idWindow *win)
{
	if (handler == &Script_Set) {
		bool precacheBackground = false;
		bool precacheSounds = false;
		idWinStr *str = dynamic_cast<idWinStr *>(parms[0].var);
		assert(str);
		idWinVar *dest = win->GetWinVarByName(*str, true);

		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;

			if (dynamic_cast<idWinBackground *>(dest) != NULL) {
				precacheBackground = true;
			}
		} else if (idStr::Icmp(str->c_str(), "cmd") == 0) {
			precacheSounds = true;
		}

		int parmCount = parms.Num();

		for (int i = 1; i < parmCount; i++) {
			idWinStr *str = dynamic_cast<idWinStr *>(parms[i].var);

			if (idStr::Icmpn(*str, "gui::", 5) == 0) {

				//  always use a string here, no point using a float if it is one
				//  FIXME: This creates duplicate variables, while not technically a problem since they
				//  are all bound to the same guiDict, it does consume extra memory and is generally a bad thing
				idWinStr *defvar = new idWinStr();
				defvar->Init(*str, win);
				win->AddDefinedVar(defvar);
				delete parms[i].var;
				parms[i].var = defvar;
				parms[i].own = false;

				//dest = win->GetWinVarByName(*str, true);
				//if (dest) {
				//	delete parms[i].var;
				//	parms[i].var = dest;
				//	parms[i].own = false;
				//}
				//
			} else if ((*str[0]) == '$') {
				//
				//  dont include the $ when asking for variable
				dest = win->GetGui()->GetDesktop()->GetWinVarByName((const char *)(*str) + 1, true);

				//
				if (dest) {
					delete parms[i].var;
					parms[i].var = dest;
					parms[i].own = false;
				}
			} else if (idStr::Cmpn(str->c_str(), STRTABLE_ID, STRTABLE_ID_LENGTH) == 0) {
				str->Set(common->GetLanguageDict()->GetString(str->c_str()));
			} else if (precacheBackground) {
				const idMaterial *mat = declManager->FindMaterial(str->c_str());
				mat->SetSort(SS_GUI);
			} else if (precacheSounds) {
				// Search for "play <...>"
				idToken token;
				idParser parser(LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT);
				parser.LoadMemory(str->c_str(), str->Length(), "command");

				while (parser.ReadToken(&token)) {
					if (token.Icmp("play") == 0) {
						if (parser.ReadToken(&token) && (token != "")) {
							declManager->FindSound(token.c_str());
						}
					}
#ifdef _HUMANHEAD
					else if (token.Icmp("play2") == 0) {
						if (parser.ReadToken(&token) && (token != "")) {
							declManager->FindSound(token.c_str());
						}
					}
#endif
				}
			}
		}
	} else if (handler == &Script_Transition) {
		if (parms.Num() < 4) {
			common->Warning("Window %s in gui %s has a bad transition definition", win->GetName(), win->GetGui()->GetSourceFile());
		}

		idWinStr *str = dynamic_cast<idWinStr *>(parms[0].var);
		assert(str);

		//
		drawWin_t *destowner;
		idWinVar *dest = win->GetWinVarByName(*str, true, &destowner);
		//

		if (dest) {
			delete parms[0].var;
			parms[0].var = dest;
			parms[0].own = false;
		} else {
			common->Warning("Window %s in gui %s: a transition does not have a valid destination var %s", win->GetName(), win->GetGui()->GetSourceFile(),str->c_str());
		}

		//
		//  support variables as parameters
		int c;

		for (c = 1; c < 3; c ++) {
			str = dynamic_cast<idWinStr *>(parms[c].var);

			idWinVec4 *v4 = new idWinVec4;
			parms[c].var = v4;
			parms[c].own = true;

			drawWin_t *owner;

			if ((*str[0]) == '$') {
				dest = win->GetWinVarByName((const char *)(*str) + 1, true, &owner);
			} else {
				dest = NULL;
			}

			if (dest) {
				idWindow *ownerparent;
				idWindow *destparent;

#ifdef _RAVEN
// jmarshall - quake 4 guis
                if ( owner && destowner)
// jmarshall end
#else
				if (owner)
#endif
                {
					ownerparent = owner->simp?owner->simp->GetParent():owner->win->GetParent();
					destparent  = destowner->simp?destowner->simp->GetParent():destowner->win->GetParent();

					// If its the rectangle they are referencing then adjust it
					if (ownerparent && destparent &&
					    (dest == (owner->simp?owner->simp->GetWinVarByName("rect"):owner->win->GetWinVarByName("rect")))) {
						idRectangle rect;
						rect = *(dynamic_cast<idWinRectangle *>(dest));
						ownerparent->ClientToScreen(&rect);
						destparent->ScreenToClient(&rect);
						*v4 = rect.ToVec4();
					} else {
						v4->Set(dest->c_str());
					}
				} else {
					v4->Set(dest->c_str());
				}
			} else {
				v4->Set(*str);
			}

			delete str;
		}

		//

	} else {
		int c = parms.Num();

		for (int i = 0; i < c; i++) {
			parms[i].var->Init(parms[i].var->c_str(), win);
		}
	}
}

/*
=========================
idGuiScriptList::FixupParms
=========================
*/
void idGuiScriptList::FixupParms(idWindow *win)
{
	int c = list.Num();

	for (int i = 0; i < c; i++) {
		idGuiScript *gs = list[i];
		gs->FixupParms(win);

		if (gs->ifList) {
			gs->ifList->FixupParms(win);
		}

		if (gs->elseList) {
			gs->elseList->FixupParms(win);
		}
	}
}

/*
=========================
idGuiScriptList::WriteToSaveGame
=========================
*/
void idGuiScriptList::WriteToSaveGame(idFile *savefile)
{
	int i;

	for (i = 0; i < list.Num(); i++) {
		list[i]->WriteToSaveGame(savefile);
	}
}

/*
=========================
idGuiScriptList::ReadFromSaveGame
=========================
*/
void idGuiScriptList::ReadFromSaveGame(idFile *savefile)
{
	int i;

	for (i = 0; i < list.Num(); i++) {
		list[i]->ReadFromSaveGame(savefile);
	}
}


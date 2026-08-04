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



#include "Window.h"
#include "SimpleWindow.h"
#include "Winvar.h"
#include "GuiScript.h"
#include "UserInterfaceLocal.h"


//stgatilov: additional debug output
static void ReportGuiCmd(idGuiScript *script, const char *fullCmd) {
	if (idStr::Cmp(fullCmd, "mainmenu_heartbeat;") == 0)
		return;	//stgatilov: suppress typical meaningless spam
	idStr location = script->GetSrcLocStr();
	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("CMD: %-30s (%s)", fullCmd, location.c_str());
}

/*
=========================
Script_Set
=========================
*/
void Script_Set(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idStr key, val;
	idWinStr *dest = dynamic_cast<idWinStr*>((*src)[0].var);
	if (dest) {
		if (idStr::Icmp(*dest, "cmd") == 0) {
			dest = dynamic_cast<idWinStr*>((*src)[1].var);
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
				ReportGuiCmd(self, val.c_str());
				window->AddCommand(val);
			} else {
				ReportGuiCmd(self, dest->c_str());
				window->AddCommand(*dest);
			}
			return;
		} 
	}
	(*src)[0].var->Set((*src)[1].var->c_str());
	// stgatilov: don't reset this variable automatically from register expressions
	(*src)[0].var->SetEval(false);
}

/*
=========================
Script_SetFocus
=========================
*/
void Script_SetFocus(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idWinStr *parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		drawWin_t win = window->GetGui()->GetDesktop()->FindChildByName(*parm);
		if (win.win) {
			window->SetFocus(win.win);
		}
	}
}

/*
=========================
Script_ShowCursor
=========================
*/
void Script_ShowCursor(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idWinBool *parm = dynamic_cast<idWinBool*>((*src)[0].var);
	if ( parm ) {
		if ( bool(parm) ) {
			window->GetGui()->GetDesktop()->ClearFlag( WIN_NOCURSOR );
		} else {
			window->GetGui()->GetDesktop()->SetFlag( WIN_NOCURSOR );
		}
	}
}

/*
=========================
Script_RunScript

 run scripts must come after any set cmd set's in the script
=========================
*/
void Script_RunScript(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idWinStr *parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		idStr str = "runScript ";
		str += parm->c_str();
		window->AddCommand(str);
	}
}

/*
=========================
Script_LocalSound
=========================
*/
void Script_LocalSound(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idWinStr *parm = dynamic_cast<idWinStr*>((*src)[0].var);
	if (parm) {
		session->sw->PlayShaderDirectly(*parm);
	}
}

/*
=========================
Script_EvalRegs
=========================
*/
void Script_EvalRegs(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	window->EvalRegs(-1, true);
}

/*
=========================
Script_EndGame
=========================
*/
void Script_EndGame(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
}

/*
=========================
Script_ResetTime
=========================
*/
void Script_ResetTime(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	idWinVar *target = nullptr;
	idWinVar *value = nullptr;
	if (src->Num() >= 2) {
		target = (*src)[0].var;
		value = (*src)[1].var;
	}
	else if (src->Num() == 1) {
		value = (*src)[0].var;
	}

	idWindow *targetWin = window;
	if (target) {
		drawWin_t win = window->GetGui()->GetDesktop()->FindChildByName(target->c_str());
		if (win.win)
			targetWin = win.win;
	}

	idWinInt *intVar = dynamic_cast<idWinInt*>(value);
	if (!intVar)
		return;	// no arguments / not integer: already reported in Parse
	int timeMoment = int(*intVar);

	targetWin->ResetTime(timeMoment);
	targetWin->EvalRegs(-1, true);
}

/*
=========================
Script_ResetCinematics
=========================
*/
void Script_ResetCinematics(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	window->ResetCinematics();
}

/*
=========================
Script_Transition
=========================
*/
void Script_Transition(idGuiScript *self, idWindow *window, idList<idGSWinVar> *src) {
	// transitions always affect rect or vec4 vars
	if (src->Num() >= 4) {
		idWinRectangle *rect = NULL;
		idWinVec4 *vec4 = dynamic_cast<idWinVec4*>((*src)[0].var);
		// 
		//  added float variable
		idWinFloat* val = NULL;
		// 
		if (vec4 == NULL) {
			rect = dynamic_cast<idWinRectangle*>((*src)[0].var);
			// 
			//  added float variable					
			if ( NULL == rect ) {
				val = dynamic_cast<idWinFloat*>((*src)[0].var);
			}
			// 
		}
		idWinVec4 *from = dynamic_cast<idWinVec4*>((*src)[1].var);
		idWinVec4 *to = dynamic_cast<idWinVec4*>((*src)[2].var);
		idWinInt *timeVal = dynamic_cast<idWinInt*>((*src)[3].var);
		// 
		//  added float variable					
		if (!((vec4 || rect || val) && from && to && timeVal)) {
			// 
			common->Warning("Bad transition in gui %s in window %s", window->GetGui()->GetSourceFile(), window->GetName());
			return;
		}
		int time = int(*timeVal);
		float ac = 0.0f;
		float dc = 0.0f;
		if (src->Num() > 4) {
			idWinFloat *acv = dynamic_cast<idWinFloat*>((*src)[4].var);
			idWinFloat *dcv = dynamic_cast<idWinFloat*>((*src)[5].var);
			assert(acv && dcv);
			ac = float(*acv);
			dc = float(*dcv);
		}
				
		if (vec4) {
			vec4->SetEval(false);
			window->AddTransition(vec4, *from, *to, time, ac, dc);
			// 
			//  added float variable					
		} else if ( val ) {
			val->SetEval ( false );
			window->AddTransition(val, *from, *to, time, ac, dc);
			// 
		} else {
			rect->SetEval(false);
			window->AddTransition(rect, *from, *to, time, ac, dc);
		}
		window->StartTransition();
	}
}

typedef struct {
	const char *name;
	void (*handler) (idGuiScript *self, idWindow *window, idList<idGSWinVar> *src);
	int mMinParms;
	int mMaxParms;
} guiCommandDef_t;

guiCommandDef_t commandList[] = {
	{ "set", Script_Set, 2, 2 },
	{ "setFocus", Script_SetFocus, 1, 1 },
	{ "endGame", Script_EndGame, 0, 0 },
	{ "resetTime", Script_ResetTime, 1, 2 },
	{ "showCursor", Script_ShowCursor, 1, 1 },
	{ "resetCinematics", Script_ResetCinematics, 0, 0 },
	{ "transition", Script_Transition, 4, 6 },
	{ "localSound", Script_LocalSound, 1, 1 },
	{ "runScript", Script_RunScript, 1, 1 },
	{ "evalRegs", Script_EvalRegs, 0, 0 }
};

int	scriptCommandCount = sizeof(commandList) / sizeof(guiCommandDef_t);


/*
=========================
idGuiScript::idGuiScript
=========================
*/
idGuiScript::idGuiScript() {
	ifList = NULL;
	elseList = NULL;
	conditionReg = -1;
	handler = NULL;
	parms.SetGranularity( 2 );
}

/*
=========================
idGuiScript::~idGuiScript
=========================
*/
idGuiScript::~idGuiScript() {
	delete ifList;
	delete elseList;
	int c = parms.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( parms[i].own ) {
			delete parms[i].var;
		}
	}
}

/*
=========================
idGuiScript::GetSrcLocStr
=========================
*/
idStr idGuiScript::GetSrcLocStr() const {
	return srcLocation.ToString();
}

/*
=========================
idGuiScript::WriteToSaveGame
=========================
*/
void idGuiScript::WriteToSaveGame( idFile *savefile ) {
	int i;

	if ( ifList ) {
		ifList->WriteToSaveGame( savefile );
	}
	if ( elseList ) {
		elseList->WriteToSaveGame( savefile );
	}

	savefile->Write( &conditionReg, sizeof( conditionReg ) );

	for ( i = 0; i < parms.Num(); i++ ) {
		if ( parms[i].own ) {
			parms[i].var->WriteToSaveGame( savefile );
		}
	}
}

/*
=========================
idGuiScript::ReadFromSaveGame
=========================
*/
void idGuiScript::ReadFromSaveGame( idFile *savefile ) {
	int i;

	if ( ifList ) {
		ifList->ReadFromSaveGame( savefile );
	}
	if ( elseList ) {
		elseList->ReadFromSaveGame( savefile );
	}

	savefile->Read( &conditionReg, sizeof( conditionReg ) );

	for ( i = 0; i < parms.Num(); i++ ) {
		if ( parms[i].own ) {
			parms[i].var->ReadFromSaveGame( savefile );
		}
	}
}

/*
=========================
idGuiScript::Parse
=========================
*/
bool idGuiScript::Parse(idParser *src, idWindow *win) {
	int i;

	// first token should be function call
	// then a potentially variable set of parms
	// ended with a ;
	idToken token;
	if ( !src->ReadToken(&token) ) {
		src->Error( "Unexpected end of file" );
		return false;
	}

	handler	= NULL;
	
	for ( i = 0; i < scriptCommandCount ; i++ ) {
		if ( idStr::Icmp(token, commandList[i].name) == 0 ) {
			handler = commandList[i].handler;
			break;
		}
	}

	if (handler == NULL) {
		src->Error("Wrong script command '%s' ignored", token.c_str());
	}
	// now read parms til ;
	// all parms are read as idWinStr's but will be fixed up later 
	// to be proper types
	while (1) {
		if ( !src->ReadToken(&token) ) {
			src->Error( "Unexpected end of file" );
			return false;
		}
		
		if (idStr::Icmp(token, ";") == 0) {
			break;
		}

		if (idStr::Icmp(token, "}") == 0) {
			src->UnreadToken(&token);
			break;
		}

		if (handler == Script_Set && parms.Num() == 1 && token.Icmp("(") == 0 && token.type == TT_PUNCTUATION) {
			// stgatilov #6028: allow window expressions as source argument to Set command
			// note that we need some trigger to distinguish it from ordinary assignment
			// so we require all expressions to be enclosed into parentheses
			idList<int> exprNodes;
			do {
				exprNodes.Append(win->ParseExpression(src, NULL));
				src->ReadToken(&token);
			} while (token.Icmp(",") == 0);

			if ( !(token.Icmp(")") == 0 && token.type == TT_PUNCTUATION) )
				src->Warning("Right side of Set with expressions ends with '%s' instead of ')'", token.c_str());

			if (exprNodes.Num() > 4)
				src->Warning("Too many components (%d) on right side of Set", exprNodes.Num());

			// decide which register type to use, create unnamed variable
			idWinVar *srcVar = nullptr;
			idRegister::REGTYPE regType = idRegister::NUMTYPES;
			if (exprNodes.Num() == 1) {
				srcVar = new idWinFloat();
				regType = idRegister::FLOAT;
			} else if (exprNodes.Num() == 2) {
				srcVar = new idWinVec2();
				regType = idRegister::VEC2;
			} else if (exprNodes.Num() == 3) {
				srcVar = new idWinVec3();
				regType = idRegister::VEC3;
			} else {
				srcVar = new idWinVec4();
				regType = idRegister::VEC4;
			}

			// add unnamed register with these expressions, link to new unnamed var
			win->RegList()->AddReg(nullptr, regType, exprNodes.Ptr(), srcVar);

			// add var to command parameters
			idGSWinVar wv;
			wv.own = true;
			wv.var = srcVar;
			parms.Append( wv );

			continue;
		}

		idWinStr *str = new idWinStr();
		*str = token;
		idGSWinVar wv;
		wv.own = true;
		wv.var = str;
		parms.Append( wv );
	}

	//  verify min/max params
	if ( handler && (parms.Num() < commandList[i].mMinParms || parms.Num() > commandList[i].mMaxParms ) ) {
		bool maybeMissedSemicolon = false;
		if ( parms.Num() > commandList[i].mMaxParms ) {
			// check if we have another command in arguments
			for ( int p = 0; p < parms.Num(); p++ )
				for ( int j = 0; j < scriptCommandCount; j++ )
					if ( idStr::Icmp( parms[p].var->c_str(), commandList[j].name ) == 0 ) {
						maybeMissedSemicolon = true;
					}
		}
		// stgatilov #5869: even though "set" command can technically process several arguments,
		// better complain about it, since otherwise missing semicolon at the end of any "set" command will silently swallow next command!
		src->Warning(
			"wrong number of arguments for script command %s: %d instead of [%d..%d]%s",
			commandList[i].name, parms.Num(), commandList[i].mMinParms, commandList[i].mMaxParms,
			(maybeMissedSemicolon ? ", maybe missing semicolon?" : "")
		);
	}
	// 

	return true;
}

/*
=========================
idGuiScriptList::Execute
=========================
*/
void idGuiScriptList::Execute(idWindow *win) {
	int c = list.Num();
	for (int i = 0; i < c; i++) {
		idGuiScript *gs = list[i];
		assert(gs);
		if (gs->conditionReg >= 0) {
			if (win->HasOps()) {
				float f = win->EvalRegs(gs->conditionReg);
				if (f) {
					if (gs->ifList) {
						win->RunScriptList(gs->ifList, NULL);
					}
				} else if (gs->elseList) {
					win->RunScriptList(gs->elseList, NULL);
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
void idGuiScript::FixupParms(idWindow *win) {
	if (handler == &Script_Set) {
		if (parms.Num() < 1)
			return;	// already warned in idGuiScript::Parse

		bool precacheBackground = false;
		bool precacheSounds = false;

		idWinStr *str = dynamic_cast<idWinStr*>(parms[0].var);
		assert(str);
		idWinVar *dest = win->GetAnyVarByName(*str);
		if (dest) {
			parms[0].RelinkVar(dest, false);

			if ( dynamic_cast<idWinBackground *>(dest) != NULL ) {
				precacheBackground = true;
			}
		} else if ( idStr::Icmp( str->c_str(), "cmd" ) == 0 ) {
			precacheSounds = true;
		} else {
			// stgatilov #5869: other destinations make no sense
			// I believe Script_Set would treat token as string and would assign new value straight into this token =)
			common->Warning("unknown destination '%s' of set command at %s", str->c_str(), GetSrcLocStr().c_str());
		}

		int parmCount = parms.Num();
		for (int i = 1; i < parmCount; i++) {
			idWinStr *str = dynamic_cast<idWinStr*>(parms[i].var);		

			if (str == nullptr) {
				// stgatilov #6028: this is expression (or several component expressions)
				if (!dest)
					return;

				// check that dimensions match or destination var is string
				idRegister::REGTYPE dstType = idRegister::RegTypeForVar(dest);
				idRegister::REGTYPE srcType = idRegister::RegTypeForVar(parms[i].var);
				int dstDim = idRegister::REGCOUNT[dstType];
				int srcDim = idRegister::REGCOUNT[srcType];
				if (dstType != idRegister::STRING && dstDim != srcDim) {
					common->Warning("destination var has type %s but source expression has %d components at %s", dest->GetTypeName(), srcDim, GetSrcLocStr().c_str());
				}

			} else if (idStr::Icmpn(*str, "gui::", 5) == 0) {

				//  always use a string here, no point using a float if it is one
				//  FIXME: This creates duplicate variables, while not technically a problem since they
				//  are all bound to the same guiDict, it does consume extra memory and is generally a bad thing
				idWinStr* defvar = new idWinStr();
				defvar->Init ( *str, win );
				win->AddDefinedVar ( defvar );
				parms[i].RelinkVar( defvar, false );

			} else if ((*str[0]) == '$') {
				// stgatilov: take window variable of specified name
				const char *varname = str->c_str() + 1;
				if (idStr::FindText(varname, "::") >= 0) {
					// contains window name, so search for it globally
					dest = win->GetGui()->GetDesktop()->GetAnyVarByName((const char*)(*str) + 1);
				} else {
					// does not contain window name, so find this variable in the current window
					dest = win->GetThisWinVarByName((const char*)(*str) + 1);
				}
				if (dest) {
					parms[i].RelinkVar(dest, false);
				}
				else {
					common->Warning("dollar source '%s' not found at %s", varname, GetSrcLocStr().c_str());
				}
			} else {
				// stgatilov: this is a plain string
				if (dest) {
					// stgatilov #5869: check compatible type of right value in assignment
					if (!dest->TestSet(str->c_str())) {
						common->Warning("set value '%s' is not compatible with type %s at %s", str->c_str(), dest->GetTypeName(), GetSrcLocStr().c_str());
					}
				}

				if ( idStr::Cmpn( str->c_str(), STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
					str->Set( common->Translate( str->c_str() ) );
				} else if ( precacheBackground ) {
					const idMaterial *mat = declManager->FindMaterial( str->c_str() );
					mat->SetSort( SS_GUI );
				} else if ( precacheSounds ) {
					// Search for "play <...>"
					idToken token;
					idParser parser( LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_ALLOWPATHNAMES );
					parser.LoadMemory(str->c_str(), str->Length(), "command");

					while ( parser.ReadToken(&token) ) {
						if ( token.Icmp("play") == 0 ) {
							if ( parser.ReadToken(&token) && ( token != "" ) ) {
								declManager->FindSound( token.c_str() );
							}
						}
					}
				}
			}
		}
	}
	else if (handler == &Script_Transition) {
		if (parms.Num() < 4)
			return;	// already warned in idGuiScript::Parse
		if (parms.Num() == 5) {
			common->Warning("transition has 5 arguments (must be 4 or 6) at %s", GetSrcLocStr().c_str());
		}
		idWinStr *str = dynamic_cast<idWinStr*>(parms[0].var);
		assert(str);

		// 
		drawWin_t destowner = {0};
		idWinVar *dest = win->GetWinVarByName(*str, &destowner, nullptr);
		// 

		if (dest) {
			parms[0].RelinkVar(dest, false);
		} else {
			common->Warning("Window '%s' transition has invalid destination var '%s' at %s", win->GetName(), str->c_str(), GetSrcLocStr().c_str());
		}

		// 
		//  support variables as parameters
		int c;
		for ( c = 1; c < 3; c ++ ) {
			str = dynamic_cast<idWinStr*>(parms[c].var);

			idWinVec4 *v4 = new idWinVec4;

			drawWin_t owner = {0};
			if ( (*str[0]) == '$' ) {
				dest = win->GetWinVarByName((const char*)(*str) + 1, &owner, nullptr);
			} else {
				dest = NULL;
			}

			// stgatilov #5869: print detailed warning if value is ill-suited
			auto VarSetWithWarning = [&](idWinVec4 *var, const char *value) {
				bool good = var->Set(value);
				if ( !good ) {
					common->Warning(
						"Window '%s' transition got wrong %s color '%s' at %s",
						win->GetName(), (c == 1 ? "1st" : "2nd"), value, GetSrcLocStr().c_str()
					);
				}
			};

			if ( dest ) {	
				idWindow* ownerparent;
				idWindow* destparent;
				if ( owner.win || owner.simp ) {
					ownerparent = owner.simp ? owner.simp->GetParent() : owner.win->GetParent();
					destparent  = destowner.simp ? destowner.simp->GetParent() : destowner.win->GetParent();

					// If its the rectangle they are referencing then adjust it 
					if ( ownerparent && destparent && 
						(dest == (owner.simp ? owner.simp->GetThisWinVarByName ( "rect" ) : owner.win->GetThisWinVarByName ( "rect" ) ) ) )
					{
						idRectangle rect;
						rect = *(dynamic_cast<idWinRectangle*>(dest));
						ownerparent->ClientToScreen ( &rect );
						destparent->ScreenToClient ( &rect );
						*v4 = rect.ToVec4 ( );
					} else {
						VarSetWithWarning(v4, dest->c_str());
					}
				} else {
					VarSetWithWarning(v4, dest->c_str());
				}
			} else {
				VarSetWithWarning(v4, *str);
			}			
			
			parms[c].RelinkVar(v4, true);
		}

		{
			idWinVar* &var = parms[3].var;
			idWinInt *intVar = new idWinInt();
			if (!intVar->Set(var->c_str())) {
				common->Warning("transition duration '%s' is not integer at %s", var->c_str(), GetSrcLocStr().c_str());
			}
			parms[3].RelinkVar(intVar, true);
		}

		if (parms.Num() >= 6) {
			for (int c = 4; c < 6; c++) {
				idWinVar* &var = parms[c].var;
				idWinFloat *floatVar = new idWinFloat();
				if (!floatVar->Set(var->c_str())) {
					common->Warning("transition %s time '%s' is not float at %s", (c == 4 ? "accel" : "decel"), var->c_str(), GetSrcLocStr().c_str());
				}
				parms[c].RelinkVar(floatVar, true);
			}
		}

	}
	else if (handler == &Script_ResetTime) {
		idWinVar *target = nullptr;
		idGSWinVar *value = nullptr;
		if (parms.Num() == 2) {
			target = parms[0].var;
			value = &parms[1];
		}
		else if (parms.Num() == 1) {
			value = &parms[0];
		}

		if (target) {
			drawWin_t match = win->GetGui()->GetDesktop()->FindChildByName(target->c_str());
			if (!match.win && !match.simp)
				common->Warning("resetTime target window '%s' not found at %s", target->c_str(), GetSrcLocStr().c_str());
			else if (!match.win)
				common->Warning("resetTime target window '%s' lacks time behavior at %s", target->c_str(), GetSrcLocStr().c_str());
		}
		if (value) {
			idWinInt *intVar = new idWinInt();
			if ( !intVar->Set(value->var->c_str()) ) {
				common->Warning("resetTime time value '%s' is not integer at %s", value->var->c_str(), GetSrcLocStr().c_str());
			}
			value->RelinkVar(intVar, true);
		}
	}
	else if (handler == &Script_ShowCursor) {
		if (parms.Num() < 1)
			return;
		idWinVar *value = parms[0].var;
		idWinBool *boolVar = new idWinBool();
		if (!boolVar->Set(value->c_str())) {
			common->Warning("showCursor value '%s' is not bool at %s", value->c_str(), GetSrcLocStr().c_str());
		}
		parms[0].RelinkVar(boolVar, true);
	}
	else if (handler == &Script_SetFocus) {
		if (parms.Num() < 1)
			return;
		idWinVar *target = parms[0].var;
		drawWin_t match = win->GetGui()->GetDesktop()->FindChildByName(target->c_str());
		if (!match.win && !match.simp)
			common->Warning("setFocus target window '%s' not found at %s", target->c_str(), GetSrcLocStr().c_str());
		else if (!match.win)
			common->Warning("setFocus target window '%s' lacks behavior at %s", target->c_str(), GetSrcLocStr().c_str());
	}
	else {
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
void idGuiScriptList::FixupParms(idWindow *win) {
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
void idGuiScriptList::WriteToSaveGame( idFile *savefile ) {
	int i;

	for ( i = 0; i < list.Num(); i++ ) {
		list[i]->WriteToSaveGame( savefile );
	}
}

/*
=========================
idGuiScriptList::ReadFromSaveGame
=========================
*/
void idGuiScriptList::ReadFromSaveGame( idFile *savefile ) {
	int i;

	for ( i = 0; i < list.Num(); i++ ) {
		list[i]->ReadFromSaveGame( savefile );
	}
}

/*
** thingdef_parse.cpp
**
**---------------------------------------------------------------------------
** Copyright 2017 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "actor.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_codeptr.h"
#include "thingdef/thingdef_parse.h"
#include "thingdef/thingdef_type.h"
#include "r_sprites.h"
#include "scanner.h"
#include "tmemory.h"
#include "w_wad.h"

enum DPARSER_CTL
{
	DPARSER_CTL_None = 0,

	DPARSER_CTL_Goto = 1,
	DPARSER_CTL_Loop = 2,
	DPARSER_CTL_Wait = 4,
	DPARSER_CTL_Fail = 8,
	DPARSER_CTL_Stop = 16,

	DPARSER_CTL_AnyStatement = (DPARSER_CTL_Stop | DPARSER_CTL_Fail | DPARSER_CTL_Wait | DPARSER_CTL_Loop | DPARSER_CTL_Goto)
};

FDecorateParser::FDecorateParser(int lumpNum) : sc(lumpNum), newClass(NULL)
{
	// Enable ed num warning if not in ecwolf.pk3 (set to true which
	// incidateds we've thrown this warning)
	thingEdNumWarning = Wads.GetLumpFile(lumpNum) == 0;
}

void FDecorateParser::Parse()
{
	while(sc.TokensLeft())
	{
		if(sc.CheckToken('#'))
		{
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("include") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected 'include' got '%s' instead.", sc->str.GetChars());
			sc.MustGetToken(TK_StringConst);

			int lmp = Wads.CheckNumForFullName(sc->str, true);
			if(lmp == -1)
				sc.ScriptMessage(Scanner::ERROR, "Could not find lump \"%s\".", sc->str.GetChars());
			FDecorateParser parser(lmp);
			parser.Parse();
			continue;
		}

		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("actor") == 0)
		{
			ParseActor();
		}
		else if(sc->str.CompareNoCase("const") == 0)
		{
			sc.MustGetToken(TK_Identifier);
			const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
			if(type == NULL)
				sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
			sc.MustGetToken(TK_Identifier);
			FName constName(sc->str);
			sc.MustGetToken('=');
			TUniquePtr<ExpressionNode> expr(ExpressionNode::ParseExpression(NATIVE_CLASS(Actor), TypeHierarchy::staticTypes, sc));
			ConstantSymbol *newSym = new ConstantSymbol(constName, type, expr->Evaluate(NULL));
			sc.MustGetToken(';');

			ClassDef::AddGlobalSymbol(newSym);
		}
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown thing section '%s'.", sc->str.GetChars());
	}
}

void FDecorateParser::ParseActor()
{
	// Read the header
	bool previouslyDefined = false;
	bool isNative = false;
	ParseActorHeader(previouslyDefined, isNative);

	if(previouslyDefined && !isNative && !newClass->tentative)
		sc.ScriptMessage(Scanner::ERROR, "Actor '%s' already defined.", newClass->name.GetChars());
	else
		newClass->tentative = false;

	if(!newClass->InitializeActorClass(isNative))
		sc.ScriptMessage(Scanner::ERROR, "Uninitialized default instance for '%s'.", newClass->GetName().GetChars());

	bool actionsSorted = true;
	sc.MustGetToken('{');

	while(!sc.CheckToken('}'))
	{
		if(sc.CheckToken('+') || sc.CheckToken('-'))
			ParseActorFlag();
		else
		{
			sc.MustGetToken(TK_Identifier);

			if(sc->str.CompareNoCase("states") == 0)
			{
				if(!actionsSorted)
					InitFunctionTable(&newClass->actions);
				ParseActorState();
			}
			else if(sc->str.CompareNoCase("action") == 0)
			{
				actionsSorted = false;
				ParseActorAction();
			}
			else if(sc->str.CompareNoCase("native") == 0)
				ParseActorNative();
			else
				ParseActorProperty();
		}
	}

	if(!actionsSorted)
		InitFunctionTable(&newClass->actions);

	newClass->FinalizeActorClass();
	newClass = NULL;
}

void FDecorateParser::ParseActorAction()
{
	sc.MustGetToken(TK_Identifier);
	if (sc->str.CompareNoCase("native") != 0)
		sc.ScriptMessage(Scanner::ERROR, "Custom actions not supported.");
	sc.MustGetToken(TK_Identifier);
	ActionInfo *funcInf = LookupFunction(sc->str, NULL);
	if (!funcInf)
		sc.ScriptMessage(Scanner::ERROR, "The specified function %s could not be located.", sc->str.GetChars());
	newClass->actions.Push(funcInf);
	sc.MustGetToken('(');
	if (!sc.CheckToken(')'))
	{
		bool optRequired = false;
		do
		{
			// If we have processed at least one argument, then we can take varArgs.
			if (funcInf->minArgs > 0 && sc.CheckToken(TK_Ellipsis))
			{
				funcInf->varArgs = true;
				break;
			}

			sc.MustGetToken(TK_Identifier);
			const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
			if (type == NULL)
				sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
			funcInf->types.Push(type);

			if (sc->str.CompareNoCase("class") == 0)
			{
				sc.MustGetToken('<');
				sc.MustGetToken(TK_Identifier);
				sc.MustGetToken('>');
			}
			sc.MustGetToken(TK_Identifier);
			if (optRequired || sc.CheckToken('='))
			{
				if (optRequired)
					sc.MustGetToken('=');
				else
					optRequired = true;

				CallArguments::Value defVal;
				defVal.isExpression = false;

				if (type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT) ||
					type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::FLOAT)
					)
				{
					TUniquePtr<ExpressionNode> node(ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc));
					const ExpressionNode::Value &val = node->Evaluate(NULL);
					if (type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT))
					{
						defVal.useType = CallArguments::Value::VAL_INTEGER;
						defVal.val.i = val.GetInt();
					}
					else
					{
						defVal.useType = CallArguments::Value::VAL_DOUBLE;
						defVal.val.d = val.GetDouble();
					}
				}
				else if (type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::BOOL))
				{
					sc.MustGetToken(TK_BoolConst);
					defVal.useType = CallArguments::Value::VAL_INTEGER;
					defVal.val.i = sc->boolean;
				}
				else if (type == TypeHierarchy::staticTypes.GetType(TypeHierarchy::STATE))
				{
					defVal.useType = CallArguments::Value::VAL_STATE;
					if (sc.CheckToken(TK_IntConst))
						sc.ScriptMessage(Scanner::ERROR, "State offsets not allowed for defaults.");
					else
					{
						sc.MustGetToken(TK_StringConst);
						defVal.label = StateLabel(sc->str, newClass);
					}
				}
				else
				{
					sc.MustGetToken(TK_StringConst);
					defVal.useType = CallArguments::Value::VAL_STRING;
					defVal.str = sc->str;
				}
				funcInf->defaults.Push(defVal);
			}
			else
				++funcInf->minArgs;
			++funcInf->maxArgs;
		} while (sc.CheckToken(','));
		sc.MustGetToken(')');
	}
	sc.MustGetToken(';');
}

// Returns the new class to operate on
void FDecorateParser::ParseActorHeader(bool &previouslyDefined, bool &isNative)
{
	sc.MustGetToken(TK_Identifier);

	ClassDef **classRef = ClassDef::ClassTable().CheckKey(sc->str);

	previouslyDefined = (classRef != NULL);

	if(!previouslyDefined)
	{
		newClass = new ClassDef();
		ClassDef::ClassTable()[sc->str] = newClass;
	}
	else
		newClass = *classRef;

	newClass->name = sc->str;

	ParseActorInheritance();
	ParseActorReplacements();

	if(sc.CheckToken(TK_IntConst))
	{
		if(const ClassDef *conflictClass = ClassDef::FindClass(sc->number))
		{
			sc.ScriptMessage(Scanner::WARNING, "'%s' overwrites deprecated editor number %d previously assigned to '%s'. This mod will soon break if not changed to 'replaces'!",
			                 newClass->GetName().GetChars(), sc->number, conflictClass->GetName().GetChars());

			if(newClass->replacee && newClass->replacee != conflictClass)
				sc.ScriptMessage(Scanner::WARNING, "Use of both editor number and 'replace' for '%s' can't be emulated. This mod is probably broken!", newClass->GetName().GetChars());
		}
		

		// Deprecated use of Doom Editor Number
		if(!thingEdNumWarning)
		{
			thingEdNumWarning = true;
			sc.ScriptMessage(Scanner::WARNING, "Deprecated use of editor number for class '%s'.", newClass->GetName().GetChars());
		}

		newClass->RegisterEdNum(sc->number);
	}

	if(sc.CheckToken(TK_Identifier))
	{
		if(sc->str.CompareNoCase("native") == 0)
			isNative = true;
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown keyword '%s'.", sc->str.GetChars());
	}
}


// Returns true if explicit inheritance was found; false otherwise
bool FDecorateParser::ParseActorInheritance()
{
	if(sc.CheckToken(':'))
	{
		sc.MustGetToken(TK_Identifier);

		const ClassDef *parent = ClassDef::FindClass(sc->str);

		if(parent == NULL || parent->tentative)
			sc.ScriptMessage(Scanner::ERROR, "Could not find parent actor '%s'", sc->str.GetChars());
		if(newClass->tentative && !parent->IsDescendantOf(newClass->parent))
			sc.ScriptMessage(Scanner::ERROR, "Parent for actor expected to be '%s'", newClass->parent->GetName().GetChars());

		newClass->parent = parent;

		return true;
	}
	else if(newClass != NATIVE_CLASS(Actor))
	{
		// If no class was specified to inherit from, inherit from AActor, but not for AActor.

		newClass->parent = NATIVE_CLASS(Actor);
	}

	return false;
}

// Returns true if the flag could be set/unset; false if it could not
bool FDecorateParser::ParseActorFlag()
{
	bool set = sc->token == '+';
	FString prefix;
	
	sc.MustGetToken(TK_Identifier);
	FString flagName = sc->str;
	
	if(sc.CheckToken('.'))
	{
		prefix = flagName;
		sc.MustGetToken(TK_Identifier);
		flagName = sc->str;
	}

	if(!ClassDef::SetFlag(newClass, (AActor*)newClass->defaultInstance, prefix, flagName, set))
	{
		sc.ScriptMessage(Scanner::WARNING, "Unknown flag '%s' for actor '%s'.", flagName.GetChars(), newClass->name.GetChars());
		return false;
	}

	return true;
}

void FDecorateParser::ParseActorNative()
{
	sc.MustGetToken(TK_Identifier);
	const Type *type = TypeHierarchy::staticTypes.GetType(sc->str);
	if (type == NULL)
		sc.ScriptMessage(Scanner::ERROR, "Unknown type %s.\n", sc->str.GetChars());
	sc.MustGetToken(TK_Identifier);
	FName varName(sc->str);
	const SymbolInfo *symInf = SymbolInfo::LookupSymbol(newClass, varName);
	if (symInf == NULL)
		sc.ScriptMessage(Scanner::ERROR, "Could not identify symbol %s::%s.\n", newClass->name.GetChars(), varName.GetChars());
	sc.MustGetToken(';');

	newClass->symbols.Push(new VariableSymbol(varName, type, symInf->offset));
}

void FDecorateParser::ParseActorProperty()
{
	FString className("actor");
	FString propertyName = sc->str;
	if (sc.CheckToken('.'))
	{
		className = propertyName;
		sc.MustGetToken(TK_Identifier);
		propertyName = sc->str;
	}

	if (!ClassDef::SetProperty(newClass, className, propertyName, sc))
	{
		do
		{
			sc.GetNextToken();
		} while (sc.CheckToken(','));
		sc.ScriptMessage(Scanner::WARNING, "Unknown property '%s' for actor '%s'.", propertyName.GetChars(), newClass->name.GetChars());
	}
}

// Returns true if a replacement was specified; false otherwise
bool FDecorateParser::ParseActorReplacements()
{
		// Handle class replacements
	if(sc.CheckToken(TK_Identifier))
	{
		if(sc->str.CompareNoCase("replaces") == 0)
		{
			sc.MustGetToken(TK_Identifier);

			if(sc->str.CompareNoCase(newClass->name) == 0)
				sc.ScriptMessage(Scanner::ERROR, "Actor '%s' attempting to replace itself!", sc->str.GetChars());

			ClassDef *replacee = const_cast<ClassDef *>(ClassDef::FindClassTentative(sc->str, NATIVE_CLASS(Actor)));
			replacee->replacement = newClass;
			newClass->replacee = replacee;

			return true;
		}
		else
			sc.Rewind();
	}

	return false;
}

void FDecorateParser::ParseActorState()
{
	TArray<StateDefinition> stateDefs;

	sc.MustGetToken('{');
	//sc.MustGetToken(TK_Identifier); // We should already have grabbed the identifier in all other cases.
	bool needIdentifier = true;
	bool infiniteLoopProtection = false;
	DPARSER_CTL controlStatement = DPARSER_CTL_None;

	while(sc->token != '}' && !sc.CheckToken('}'))
	{
		StateDefinition thisState;
		thisState.sprite[0] = thisState.sprite[4] = 0;
		thisState.duration = 0;
		thisState.randDuration = 0;
		thisState.offsetX = thisState.offsetY = 0;
		thisState.nextType = StateDefinition::NORMAL;

		if(needIdentifier)
			sc.MustGetToken(TK_Identifier);
		else
			needIdentifier = true;
		FString stateString = sc->str;
		if(sc.CheckToken(':'))
		{
			infiniteLoopProtection = false;
			thisState.label = stateString;
			// New state
			if(sc.CheckToken('}'))
				sc.ScriptMessage(Scanner::ERROR, "State defined with no frames.");
			sc.MustGetToken(TK_Identifier);

			controlStatement = (DPARSER_CTL)ParseActorStateControl(thisState, DPARSER_CTL_Stop | DPARSER_CTL_Goto);

			// If it's not set to DPARSER_CTL_None, then it will be either STOP or GOTO
			if(controlStatement != DPARSER_CTL_None && !sc.CheckToken('}'))
				sc.MustGetToken(TK_Identifier);

			stateString = sc->str;
		}

		if(thisState.nextType == StateDefinition::NORMAL &&
			(sc.CheckToken(TK_Identifier) || sc.CheckToken(TK_StringConst)))
		{
			bool invalidSprite = (stateString.Len() != 4);
			strncpy(thisState.sprite, stateString, 4);

			infiniteLoopProtection = false;
			if(invalidSprite) // We now know this is a frame so check sprite length
				sc.ScriptMessage(Scanner::ERROR, "Sprite name must be exactly 4 characters long.");

			R_LoadSprite(thisState.sprite);
			thisState.frames = sc->str;

			ParseActorStateDuration(thisState);

			thisState.functions[0].pointer = thisState.functions[1].pointer = NULL;

			// True return value indicates that the state is finished
			if(ParseActorStateFlags(thisState))
			{
				goto FinishState;
			}

			if(thisState.nextType == StateDefinition::NORMAL)
			{
				for(int func = 0;func <= 2;func++)
				{
					if(sc.CheckToken(':'))
					{
						// We have a state label!
						needIdentifier = false;
						sc.Rewind();
						break;
					}

					if(sc->str.Len() == 4 || func == 2)
					{
						controlStatement = (DPARSER_CTL)ParseActorStateControl(thisState, DPARSER_CTL_AnyStatement);

						if(controlStatement == DPARSER_CTL_None)
							needIdentifier = false;
						
						break;
					}
					else
					{
						if(sc->str.CompareNoCase("NOP") != 0)
							ParseActorStateAction(thisState, func);
					}

					if(!sc.CheckToken(TK_Identifier))
						break;
					else if(sc.CheckToken(':'))
					{
						needIdentifier = false;
						sc.Rewind();
						break;
					}
				}
			}
		}
		else
		{
			thisState.sprite[0] = 0;
			needIdentifier = false;
			if(infiniteLoopProtection)
				sc.ScriptMessage(Scanner::ERROR, "Malformed script.");
			infiniteLoopProtection = true;
		}
	FinishState:
		stateDefs.Push(thisState);
	}

	newClass->InstallStates(stateDefs);
}

void FDecorateParser::ParseActorStateAction(StateDefinition &thisState, int funcIdx)
{
	int specialNum = -1;
	const ActionInfo *funcInf = newClass->FindFunction(sc->str, specialNum);
	if(funcInf)
	{
		thisState.functions[funcIdx].pointer = *funcInf->func;

		CallArguments *&ca = thisState.functions[funcIdx].args;
		ca = new CallArguments();
		CallArguments::Value val;
		unsigned int argc = 0;

		// When using a line special we have to inject a parameter.
		if(specialNum >= 0)
		{
			val.useType = CallArguments::Value::VAL_INTEGER;
			val.isExpression = false;
			val.val.i = specialNum;
			ca->AddArgument(val);
			++argc;
		}
				
		if(sc.CheckToken('('))
		{
			if(funcInf->maxArgs == 0)
				sc.MustGetToken(')');
			else if(!(funcInf->minArgs == 0 && sc.CheckToken(')')))
			{
				do
				{
					val.isExpression = false;

					const Type *argType = funcInf->ArgType(argc);
					if(argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT) ||
						argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::FLOAT) ||
						argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::BOOL))
					{
						val.isExpression = true;
						if(argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::INT))
							val.useType = CallArguments::Value::VAL_INTEGER;
						else
							val.useType = CallArguments::Value::VAL_DOUBLE;
						val.expr = ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc);
					}
					else if(argType == TypeHierarchy::staticTypes.GetType(TypeHierarchy::STATE))
					{
						val.useType = CallArguments::Value::VAL_STATE;
						if(sc.CheckToken(TK_IntConst))
						{
							if(thisState.frames.Len() > 1)
								sc.ScriptMessage(Scanner::ERROR, "State offsets not allowed on multistate definitions.");
							FString label;
							label.Format("%d", sc->number);
							val.label = StateLabel(label, newClass);
						}
						else
						{
							sc.MustGetToken(TK_StringConst);
							val.label = StateLabel(sc->str, newClass);
						}
					}
					else
					{
						sc.MustGetToken(TK_StringConst);
						val.useType = CallArguments::Value::VAL_STRING;
						val.str = sc->str;
					}
					ca->AddArgument(val);
					++argc;

					// Check if we can or should take another argument
					if(!funcInf->varArgs && argc >= funcInf->maxArgs)
						break;
				}
				while(sc.CheckToken(','));
				sc.MustGetToken(')');
			}
		}
		if(argc < funcInf->minArgs)
			sc.ScriptMessage(Scanner::ERROR, "Too few arguments.");
		else
		{
			// Push unused defaults.
			while(argc < funcInf->maxArgs)
				ca->AddArgument(funcInf->defaults[(argc++)-funcInf->minArgs]);
		}
	}
	else
		sc.ScriptMessage(Scanner::WARNING, "Could not find function %s.", sc->str.GetChars());
}

//  Returns the state that it parsed (DPARSER_CTL_None if none), sets up thisState
int FDecorateParser::ParseActorStateControl(StateDefinition &thisState, int allowedStatements)
{
	DPARSER_CTL statement = DPARSER_CTL_None;

	if(sc->str.CompareNoCase("goto") == 0) statement = DPARSER_CTL_Goto;
	else if(sc->str.CompareNoCase("wait") == 0) statement = DPARSER_CTL_Wait;
	else if(sc->str.CompareNoCase("fail") == 0) statement = DPARSER_CTL_Fail;
	else if(sc->str.CompareNoCase("loop") == 0) statement = DPARSER_CTL_Loop;
	else if(sc->str.CompareNoCase("stop") == 0) statement = DPARSER_CTL_Stop;
	
	if(!(allowedStatements & statement)) return DPARSER_CTL_None;

	switch(statement)
	{
	case DPARSER_CTL_Goto:
		thisState.jumpLabel = StateLabel(sc, newClass, true);
		thisState.nextType = StateDefinition::GOTO;
		break;

	case DPARSER_CTL_Fail:
	case DPARSER_CTL_Wait:
		thisState.nextType = StateDefinition::WAIT;
		break;

	case DPARSER_CTL_Loop:
		thisState.nextType = StateDefinition::LOOP;
		break;

	case DPARSER_CTL_Stop:
		thisState.nextType = StateDefinition::STOP;
		break;

	default:
		break;
	}

	return statement;
}

void FDecorateParser::ParseActorStateDuration(StateDefinition &thisState)
{
	if(sc.CheckToken('-'))
	{
		sc.MustGetToken(TK_FloatConst);
		thisState.duration = -1;
	}
	else
	{
		if(sc.CheckToken(TK_FloatConst))
		{
			// Eliminate confusion about fractional frame delays
			if(!CheckTicsValid(sc->decimal))
				sc.ScriptMessage(Scanner::ERROR, "Fractional frame durations must be exactly .5!");

			thisState.duration = static_cast<int> (sc->decimal*2);
		}
		else if(stricmp(thisState.sprite, "goto") == 0)
		{
			thisState.nextType = StateDefinition::GOTO;
			thisState.nextArg = thisState.frames;
			thisState.frames = FString();
		}
		else if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("random") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected random frame duration.");

			sc.MustGetToken('(');
			sc.MustGetToken(TK_FloatConst);
			if(!CheckTicsValid(sc->decimal))
				sc.ScriptMessage(Scanner::ERROR, "Fractional frame durations must be exactly .5!");
			thisState.duration = static_cast<int> (sc->decimal*2);
			sc.MustGetToken(',');
			sc.MustGetToken(TK_FloatConst);
			if(!CheckTicsValid(sc->decimal))
				sc.ScriptMessage(Scanner::ERROR, "Fractional frame durations must be exactly .5!");
			thisState.randDuration = static_cast<int> (sc->decimal*2);
			sc.MustGetToken(')');
		}
		else
			sc.ScriptMessage(Scanner::ERROR, "Expected frame duration.");
	}
}

// Returns true if this state is finalized, false if it is not.
bool FDecorateParser::ParseActorStateFlags(StateDefinition &thisState)
{
	bool negate = false;
	thisState.fullbright = false;

	do
	{
		if(sc.CheckToken('}'))
			return true;
		else
			sc.MustGetToken(TK_Identifier);

		if(sc->str.CompareNoCase("bright") == 0)
			thisState.fullbright = true;
		else if(sc->str.CompareNoCase("offset") == 0)
		{
			sc.MustGetToken('(');

			negate = sc.CheckToken('-');
			sc.MustGetToken(TK_FloatConst);
			thisState.offsetX = FLOAT2FIXED((negate ? -1 : 1) * sc->decimal);

			sc.MustGetToken(',');

			negate = sc.CheckToken('-');
			sc.MustGetToken(TK_FloatConst);
			thisState.offsetY = FLOAT2FIXED((negate ? -1 : 1) * sc->decimal);

			sc.MustGetToken(')');
		}
		else
			break;
	}
	while(true);

	return false;
}

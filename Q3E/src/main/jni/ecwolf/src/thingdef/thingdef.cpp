/*
** thingdef.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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
#include "a_inventory.h"
#include "doomerrors.h"
#include "id_ca.h"
#include "lnspec.h"
#include "m_random.h"
#include "r_sprites.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_def.h"
#include "wl_draw.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_codeptr.h"
#include "thingdef/thingdef_expression.h"
#include "thingdef/thingdef_parse.h"
#include "thingdef/thingdef_type.h"
#include "thinker.h"
#include "templates.h"
#include "g_mapinfo.h"

#include <climits>

typedef DWORD flagstype_t;

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_FLAG(prefix, flag, type, variable) { NATIVE_CLASS(type), prefix##_##flag, #type, #flag, typeoffsetof(A##type,variable) }
const struct FlagDef
{
	public:
		const ClassDef * const &cls;
		const flagstype_t	value;
		const char* const	prefix;
		const char* const	name;
		const int			varOffset;
} flags[] =
{
	DEFINE_FLAG(FL, ALWAYSFAST, Actor, flags),
	DEFINE_FLAG(WF, ALWAYSGRIN, Weapon, weaponFlags),
	DEFINE_FLAG(IF, ALWAYSPICKUP, Inventory, itemFlags),
	DEFINE_FLAG(FL, AMBUSH, Actor, flags),
	DEFINE_FLAG(IF, AUTOACTIVATE, Inventory, itemFlags),
	DEFINE_FLAG(FL, BILLBOARD, Actor, flags),
	DEFINE_FLAG(FL, BRIGHT, Actor, flags),
	DEFINE_FLAG(FL, CANUSEWALLS, Actor, flags),
	DEFINE_FLAG(FL, COUNTITEM, Actor, flags),
	DEFINE_FLAG(FL, COUNTKILL, Actor, flags),
	DEFINE_FLAG(FL, COUNTSECRET, Actor, flags),
	DEFINE_FLAG(WF, DONTBOB, Weapon, weaponFlags),
	DEFINE_FLAG(FL, DONTRIP, Actor, flags),
	DEFINE_FLAG(FL, DROPBASEDONTARGET, Actor, flags),
	DEFINE_FLAG(IF, INVBAR, Inventory, itemFlags),
	DEFINE_FLAG(FL, ISMONSTER, Actor, flags),
	DEFINE_FLAG(FL, MISSILE, Actor, flags),
	DEFINE_FLAG(WF, NOALERT, Weapon, weaponFlags),
	DEFINE_FLAG(WF, NOAUTOFIRE, Weapon, weaponFlags),
	DEFINE_FLAG(WF, NOGRIN, Weapon, weaponFlags),
	DEFINE_FLAG(FL, OLDRANDOMCHASE, Actor, flags),
	DEFINE_FLAG(FL, PICKUP, Actor, flags),
	DEFINE_FLAG(FL, PLOTONAUTOMAP, Actor, flags),
	DEFINE_FLAG(FL, RANDOMIZE, Actor, flags),
	DEFINE_FLAG(FL, RIPPER, Actor, flags),
	DEFINE_FLAG(FL, REQUIREKEYS, Actor, flags),
	DEFINE_FLAG(FL, SHOOTABLE, Actor, flags),
	DEFINE_FLAG(FL, SOLID, Actor, flags)
};
extern const PropDef properties[];

////////////////////////////////////////////////////////////////////////////////

StateLabel::StateLabel(const FString &str, const ClassDef *parent, bool noRelative)
{
	Scanner sc(str.GetChars(), str.Len());
	sc.SetScriptIdentifier("StateLabel");
	Parse(sc, parent, noRelative);
}

StateLabel::StateLabel(Scanner &sc, const ClassDef *parent, bool noRelative)
{
	Parse(sc, parent, noRelative);
}

const Frame *StateLabel::Resolve() const
{
	return cls->FindStateInList(label) + offset;
}

const Frame *StateLabel::Resolve(AActor *owner, const Frame *caller, const Frame *def) const
{
	if(isRelative)
		return caller + offset;
	else if(isDefault)
		return def;

	const Frame *frame = owner->GetClass()->FindStateInList(label);
	if(frame)
		return frame + offset;
	return NULL;
}

void StateLabel::Parse(Scanner &sc, const ClassDef *parent, bool noRelative)
{
	cls = parent;

	// Empty string?
	if(!sc.TokensLeft())
	{
		isRelative = false;
		isDefault = false;
		label = "";
		return;
	}

	if(!noRelative && sc.CheckToken(TK_IntConst))
	{
		isRelative = true;
		offset = sc->number;
		return;
	}

	isRelative = false;
	isDefault = sc.CheckToken('*');
	if(isDefault)
		return;

	sc.MustGetToken(TK_Identifier);
	label = sc->str;
	if(sc.CheckToken(TK_ScopeResolution))
	{
		if(label.CompareNoCase("Super") == 0)
		{
			// This should never happen in normal use, but just in case.
			if(parent->parent == NULL)
				sc.ScriptMessage(Scanner::ERROR, "This actor does not have a super class.");
			cls = parent->parent;
		}
		else
		{
			do
			{
				cls = cls->parent;
				if(cls == NULL)
					sc.ScriptMessage(Scanner::ERROR, "%s is not a super class.", label.GetChars());
			}
			while(stricmp(cls->GetName().GetChars(), label.GetChars()) != 0);
		}

		sc.MustGetToken(TK_Identifier);
		label = sc->str;
	}

	while(sc.CheckToken('.'))
	{
		sc.MustGetToken(TK_Identifier);
		label = label + "." + sc->str;
	}

	if(sc.CheckToken('+'))
	{
		sc.MustGetToken(TK_IntConst);
		offset = sc->number;
	}
	else
		offset = 0;
}

////////////////////////////////////////////////////////////////////////////////

static TArray<const SymbolInfo *> *symbolPool = NULL;

SymbolInfo::SymbolInfo(const ClassDef *cls, const FName var, const int offset) :
	cls(cls), var(var), offset(offset)
{
	if(symbolPool == NULL)
		symbolPool = new TArray<const SymbolInfo *>();
	symbolPool->Push(this);
}

const SymbolInfo *SymbolInfo::LookupSymbol(const ClassDef *cls, FName var)
{
	for (unsigned int i = 0; i < symbolPool->Size(); ++i)
	{
		// I think the symbol pool will be small enough to do a
		// linear search on.
		if ((*symbolPool)[i]->cls == cls && (*symbolPool)[i]->var == var)
			return (*symbolPool)[i];
	}
	return NULL;
}

static int SymbolCompare(const void *s1, const void *s2)
{
	const Symbol * const sym1 = *((const Symbol **)s1);
	const Symbol * const sym2 = *((const Symbol **)s2);
	if(sym1->GetName() < sym2->GetName())
		return -1;
	else if(sym1->GetName() > sym2->GetName())
		return 1;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void ExprSin(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	out = double(finesine[(args[0]->Evaluate(self).GetInt()%360)*FINEANGLES/360])/FRACUNIT;
}

void ExprCos(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	out = double(finecosine[(args[0]->Evaluate(self).GetInt()%360)*FINEANGLES/360])/FRACUNIT;
}

void ExprRandom(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	int64_t min = args[0]->Evaluate(self).GetInt();
	int64_t max = args[1]->Evaluate(self).GetInt();
	if(min > max)
		out = max+(*rng)((int)(min-max+1));
	else
		out = min+(*rng)((int)(max-min+1));
}

void ExprFRandom(AActor *self, ExpressionNode::Value &out, ExpressionNode* const *args, FRandom *rng)
{
	static const unsigned int randomPrecision = 0x80000000;

	double min = args[0]->Evaluate(self).GetDouble();
	double max = args[1]->Evaluate(self).GetDouble();
	out = min+(double((*rng)(randomPrecision))/randomPrecision)*(max-min);
}

static const struct ExpressionFunction
{
	const char* const				name;
	int								ret;
	unsigned short					args;
	bool							takesRNG;
	FunctionSymbol::ExprFunction	function;
} functions[] =
{
	{ "cos",		TypeHierarchy::FLOAT,	1,	false,	ExprCos },
	{ "frandom",	TypeHierarchy::FLOAT,	2,	true,	ExprFRandom },
	{ "random",		TypeHierarchy::INT,		2,	true,	ExprRandom },
	{ "sin",		TypeHierarchy::FLOAT,	1,	false,	ExprSin },

	{ NULL, 0, 0, false, NULL }
};

////////////////////////////////////////////////////////////////////////////////

class MetaTable::Data
{
	public:
		Data(MetaTable::Type type, uint32_t id) : id(id), type(type), inherited(false), next(NULL) {}
		~Data()
		{
			SetType(MetaTable::INTEGER);
		}

		void	SetType(MetaTable::Type type)
		{
			// As soon as we try to change the value consider the meta data new
			inherited = false;
			if(this->type == type)
				return;

			if(this->type == MetaTable::STRING)
			{
				delete[] value.string;
				value.string = NULL;
			}

			this->type = type;
		}

		const Data &operator= (const Data &other)
		{
			id = other.id;
			SetType(other.type);
			inherited = true;

			switch(type)
			{
				case MetaTable::INTEGER:
					value.integer = other.value.integer;
					break;
				case MetaTable::STRING:
					value.string = new char[strlen(other.value.string)+1];
					strcpy(value.string, other.value.string);
					break;
				case MetaTable::FIXED:
					value.fixedPoint = other.value.fixedPoint;
					break;
			}

			return other;
		}

		uint32_t		id;
		MetaTable::Type	type;
		bool			inherited;
		union
		{
			int			integer;
			fixed		fixedPoint;
			char*		string;
		} value;
		Data			*next;
};

MetaTable::MetaTable() : head(NULL)
{
}

MetaTable::~MetaTable()
{
	FreeTable();
}

void MetaTable::CopyMeta(const MetaTable &other)
{
	Data *data = other.head;
	while(data)
	{
		Data *copyData = FindMetaData(data->id);
		*copyData = *data;

		data = data->next;
	}
}

MetaTable::Data *MetaTable::FindMeta(uint32_t id) const
{
	Data *data = head;

	while(data != NULL)
	{
		if(data->id == id)
			break;

		data = data->next;
	}

	return data;
}

MetaTable::Data *MetaTable::FindMetaData(uint32_t id)
{
	Data *data = FindMeta(id);
	if(data == NULL)
	{
		data = new MetaTable::Data(MetaTable::INTEGER, id);
		data->next = head;
		head = data;
	}

	return data;
}

void MetaTable::FreeTable()
{
	Data *data = head;
	while(data != NULL)
	{
		Data *prevData = data;
		data = data->next;
		delete prevData;
	}
}

int MetaTable::GetMetaInt(uint32_t id, int def) const
{
	Data *data = FindMeta(id);
	if(!data)
		return def;
	return data->value.integer;
}

fixed MetaTable::GetMetaFixed(uint32_t id, fixed def) const
{
	Data *data = FindMeta(id);
	if(!data)
		return def;
	return data->value.fixedPoint;
}

const char* MetaTable::GetMetaString(uint32_t id) const
{
	Data *data = FindMeta(id);
	if(!data)
		return NULL;
	return data->value.string;
}

bool MetaTable::IsInherited(uint32_t id)
{
	Data *data = FindMetaData(id);
	return data->inherited;	
}

void MetaTable::SetMetaInt(uint32_t id, int value)
{
	Data *data = FindMetaData(id);
	data->SetType(MetaTable::INTEGER);
	data->value.integer = value;
}

void MetaTable::SetMetaFixed(uint32_t id, fixed value)
{
	Data *data = FindMetaData(id);
	data->SetType(MetaTable::FIXED);
	data->value.fixedPoint = value;
}

void MetaTable::SetMetaString(uint32_t id, const char* value)
{
	Data *data = FindMetaData(id);
	if(data->type == MetaTable::STRING && data->value.string != NULL)
		delete[] data->value.string;
	else
		data->SetType(MetaTable::STRING);

	data->value.string = new char[strlen(value)+1];
	strcpy(data->value.string, value);
}

////////////////////////////////////////////////////////////////////////////////

static TMap<int, ClassDef *> EditorNumberTable, ConversationIDTable;
SymbolTable ClassDef::globalSymbols;
bool ClassDef::bShutdown = false;

// Minimize warning spam for deprecated feature in 1.4
static bool g_ThingEdNumWarning;

ClassDef::ClassDef() : tentative(false)
{
	defaultInstance = NULL;
	FlatPointers = Pointers = NULL;
	replacement = replacee = NULL;
}

ClassDef::~ClassDef()
{
	if(defaultInstance)
	{
		M_Free(defaultInstance);
	}
	for(unsigned int i = 0;i < symbols.Size();++i)
		delete symbols[i];
}

TMap<FName, ClassDef *> &ClassDef::ClassTable()
{
	static TMap<FName, ClassDef *> classTable;
	return classTable;
}

void ClassDef::AddGlobalSymbol(Symbol *sym)
{
	// We must insert the constant into the table at the proper place
	// now since the next const may try to reference it.
	if(globalSymbols.Size() > 0)
	{
		unsigned int min = 0;
		unsigned int max = globalSymbols.Size()-1;
		unsigned int mid = max/2;
		if(max > 0)
		{
			do
			{
				if(globalSymbols[mid]->GetName() > sym->GetName())
					max = mid-1;
				else if(globalSymbols[mid]->GetName() < sym->GetName())
					min = mid+1;
				else
					break;
				mid = (min+max)/2;
			}
			while(max >= min && max < globalSymbols.Size());
		}
		if(globalSymbols[mid]->GetName() <= sym->GetName())
			++mid;
		globalSymbols.Insert(mid, sym);
	}
	else
		globalSymbols.Push(sym);
}

const size_t ClassDef::POINTER_END = ~(size_t)0;
// [BL] Pulled from ZDoom more or less.
/*
** dobjtype.cpp
** Implements the type information class
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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
*/
// Create the FlatPointers array, if it doesn't exist already.
// It comprises all the Pointers from superclasses plus this class's own Pointers.
// If this class does not define any new Pointers, then FlatPointers will be set
// to the same array as the super class's.
void ClassDef::BuildFlatPointers()
{
	if (FlatPointers != NULL)
	{ // Already built: Do nothing.
		return;
	}
	else if (parent == NULL)
	{ // No parent: FlatPointers is the same as Pointers.
		if (Pointers == NULL)
		{ // No pointers: Make FlatPointers a harmless non-NULL.
			FlatPointers = &POINTER_END;
		}
		else
		{
			FlatPointers = Pointers;
		}
	}
	else
	{
		const_cast<ClassDef *>(parent)->BuildFlatPointers ();
		if (Pointers == NULL)
		{ // No new pointers: Just use the same FlatPointers as the parent.
			FlatPointers = parent->FlatPointers;
		}
		else
		{ // New pointers: Create a new FlatPointers array and add them.
			int numPointers, numSuperPointers;

			// Count pointers defined by this class.
			for (numPointers = 0; Pointers[numPointers] != POINTER_END; numPointers++)
			{ }
			// Count pointers defined by superclasses.
			for (numSuperPointers = 0; parent->FlatPointers[numSuperPointers] != POINTER_END; numSuperPointers++)
			{ }

			// Concatenate them into a new array
			size_t *flat = new size_t[numPointers + numSuperPointers + 1];
			if (numSuperPointers > 0)
			{
				memcpy (flat, parent->FlatPointers, sizeof(size_t)*numSuperPointers);
			}
			memcpy (flat + numSuperPointers, Pointers, sizeof(size_t)*(numPointers+1));
			FlatPointers = flat;
		}
	}
}

AActor *ClassDef::CreateInstance() const
{
	if(IsDescendantOf(NATIVE_CLASS(Actor)) && !((AActor*)defaultInstance)->SpawnState)
	{
		((AActor*)defaultInstance)->MeleeState = FindState(NAME_Melee);
		((AActor*)defaultInstance)->MissileState = FindState(NAME_Missile);
		((AActor*)defaultInstance)->PainState = FindState(NAME_Pain);
		((AActor*)defaultInstance)->PathState = FindState(NAME_Path);
		((AActor*)defaultInstance)->SpawnState = FindState(NAME_Spawn);
		((AActor*)defaultInstance)->SeeState = FindState(NAME_See);
	}

	AActor *newactor = (AActor *) M_Malloc(size);
	memcpy((void*)newactor, (void*)defaultInstance, size);
	ConstructNative(this, newactor);
	newactor->Init();
	return newactor;
}

// Perform any actions necessary after actor class is completely parsed
void ClassDef::FinalizeActorClass()
{
	// Sort the symbol table.
	qsort(&symbols[0], symbols.Size(), sizeof(symbols[0]), SymbolCompare);

	// Register conversation id into table if assigned
	if(int convid = Meta.GetMetaInt(AMETA_ConversationID))
		ConversationIDTable[convid] = this;
}


const ClassDef *ClassDef::FindClass(unsigned int ednum)
{
	ClassDef **ret = EditorNumberTable.CheckKey(ednum);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const ClassDef *ClassDef::FindClass(const FName &className)
{
	ClassDef **ret = ClassTable().CheckKey(className);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const ClassDef *ClassDef::FindConversationClass(unsigned int convid)
{
	ClassDef **ret = ConversationIDTable.CheckKey(convid);
	if(ret == NULL)
		return NULL;
	return *ret;
}

const ClassDef *ClassDef::FindClassTentative(const FName &className, const ClassDef *parent)
{
	const ClassDef *search = FindClass(className);
	if(search)
	{
		if(!search->parent->IsDescendantOf(parent))
			I_Error("%s does not inherit %s!", className.GetChars(), parent->GetName().GetChars());
		return search;
	}

	ClassDef *newClass = new ClassDef();
	ClassTable()[className] = newClass;

	newClass->tentative = true;
	newClass->name = className;
	newClass->parent = parent;
	return newClass;
}

const ActionInfo *ClassDef::FindFunction(const FName &function, int &specialNum) const
{
	Specials::LineSpecials special = Specials::LookupFunctionNum(function);
	if(special != Specials::NUM_POSSIBLE_SPECIALS)
	{
		specialNum = special;
		return FindFunction("A_CallSpecial", specialNum);
	}

	if(actions.Size() != 0)
	{
		ActionInfo *func = LookupFunction(function, &actions);
		if(func)
			return func;
	}
	if(parent)
		return parent->FindFunction(function, specialNum);
	return NULL;
}

const Frame *ClassDef::FindState(const FName &stateName) const
{
	const Frame *ret = FindStateInList(stateName);
	return ret;
}

const Frame *ClassDef::FindStateInList(const FName &stateName) const
{
	const unsigned int *ret = stateList.CheckKey(stateName);
	if(ret == NULL)
		return (!parent ? NULL : parent->FindStateInList(stateName));

	// Change the frameLists
	return ResolveStateIndex(*ret);
}

Symbol *ClassDef::FindSymbol(const FName &symbol) const
{
	unsigned int min = 0;
	unsigned int max = symbols.Size()-1;
	unsigned int mid = max/2;
	if(symbols.Size() > 0)
	{
		do
		{
			if(symbols[mid]->GetName() == symbol)
				return symbols[mid];

			if(symbols[mid]->GetName() > symbol)
				max = mid-1;
			else if(symbols[mid]->GetName() < symbol)
				min = mid+1;
			mid = (min+max)/2;
		}
		while(max >= min && max < symbols.Size());
	}

	if(parent)
		return parent->FindSymbol(symbol);
	else if(globalSymbols.Size() > 0)
	{
		// Search globals.
		min = 0;
		max = globalSymbols.Size()-1;
		mid = max/2;
		do
		{
			if(globalSymbols[mid]->GetName() == symbol)
				return globalSymbols[mid];

			if(globalSymbols[mid]->GetName() > symbol)
				max = mid-1;
			else if(globalSymbols[mid]->GetName() < symbol)
				min = mid+1;
			mid = (min+max)/2;
		}
		while(max >= min && max < globalSymbols.Size());
	}
	return NULL;
}

const ClassDef *ClassDef::GetReplacement(bool respectMapinfo) const
{
	return replacement ? replacement->GetReplacement(false) : this;
}

struct Goto
{
	Frame *frame;
	FString remapLabel; // Label: goto Label2
	StateLabel jumpLabel;
};
void ClassDef::InstallStates(const TArray<StateDefinition> &stateDefs)
{
	// We need to resolve gotos after we install the states.
	TArray<Goto> gotos;

	// Count the number of states we need so that we can allocate the memory
	// in one go and keep our pointers valid.
	unsigned int numStates = 0;
	for(unsigned int iter = 0;iter < stateDefs.Size();++iter)
	{
		if(!stateDefs[iter].label.IsEmpty() && stateDefs[iter].sprite[0] == 0)
			continue;
		numStates += (unsigned int)stateDefs[iter].frames.Len();
	}
	frameList.Resize(numStates);

	FString thisLabel;
	Frame *prevFrame = NULL;
	Frame *loopPoint = NULL;
	Frame *thisFrame = &frameList[0];
	for(unsigned int iter = 0;iter < stateDefs.Size();++iter)
	{
		const StateDefinition &thisStateDef = stateDefs[iter];

		// Special case, `Label: stop`, remove state.  Hmm... I wonder if ZDoom handles fall throughs on this.
		if(!thisStateDef.label.IsEmpty() && thisStateDef.sprite[0] == 0)
		{
			switch(thisStateDef.nextType)
			{
				case StateDefinition::STOP:
					stateList[thisStateDef.label] = INT_MAX;
					break;
				case StateDefinition::NORMAL:
					stateList[thisStateDef.label] = (unsigned int)(thisFrame - &frameList[0]);
					continue;
				case StateDefinition::GOTO:
				{
					Goto thisGoto;
					thisGoto.frame = NULL;
					thisGoto.remapLabel = thisStateDef.label;
					thisGoto.jumpLabel = thisStateDef.jumpLabel;
					gotos.Push(thisGoto);
					continue;
				}
				default:
					I_FatalError("Tried to use a loop on a frameless state.");
					break;
			}
			continue;
		}

		for(unsigned int i = 0;i < thisStateDef.frames.Len();++i)
		{
			if(i == 0 && !thisStateDef.label.IsEmpty())
			{
				stateList[thisStateDef.label] = (unsigned int)(thisFrame - &frameList[0]);
				loopPoint = thisFrame;
			}
			memcpy(thisFrame->sprite, thisStateDef.sprite, 4);
			thisFrame->frame = thisStateDef.frames[i]-'A';
			thisFrame->duration = thisStateDef.duration;
			thisFrame->randDuration = thisStateDef.randDuration;
			thisFrame->fullbright = thisStateDef.fullbright;
			thisFrame->offsetX = thisStateDef.offsetX;
			thisFrame->offsetY = thisStateDef.offsetY;
			thisFrame->action = thisStateDef.functions[0];
			thisFrame->thinker = thisStateDef.functions[1];
			thisFrame->next = NULL;
			thisFrame->index = (unsigned int)(thisFrame - &frameList[0]);
			thisFrame->spriteInf = 0;
			// Only free the action arguments if we are the last frame using them.
			thisFrame->freeActionArgs = i == thisStateDef.frames.Len()-1;
			if(i == thisStateDef.frames.Len()-1) // Handle nextType
			{
				if(thisStateDef.nextType == StateDefinition::WAIT)
					thisFrame->next = thisFrame;
				else if(thisStateDef.nextType == StateDefinition::LOOP)
					thisFrame->next = loopPoint;
				// Add to goto list
				else if(thisStateDef.nextType == StateDefinition::GOTO)
				{
					Goto thisGoto;
					thisGoto.frame = thisFrame;
					thisGoto.jumpLabel = thisStateDef.jumpLabel;
					gotos.Push(thisGoto);
				}
			}
			if(prevFrame != NULL)
				prevFrame->next = thisFrame;

			if(thisStateDef.nextType == StateDefinition::NORMAL || i != thisStateDef.frames.Len()-1)
				prevFrame = thisFrame;
			else
				prevFrame = NULL;
			//printf("Adding frame: %s %c %d\n", thisStateDef.sprite, thisFrame->frame, thisFrame->duration);
			++thisFrame;
		}
	}

	// Safe guard to make sure state counting stays in sync
	assert(thisFrame == &frameList[frameList.Size()]);

	// Resolve Gotos
	for(unsigned int iter = 0;iter < gotos.Size();++iter)
	{
		const Frame *result = gotos[iter].jumpLabel.Resolve();
		if(gotos[iter].frame)
			gotos[iter].frame->next = result;
		else
		{
			unsigned int frameIndex = result->index;
			const ClassDef *owner = this;
			while(!owner->IsStateOwner(result))
			{
				frameIndex += owner->frameList.Size();
				owner = owner->parent;
			}
			stateList[gotos[iter].remapLabel] = frameIndex;
		}
	}
}

bool ClassDef::IsDescendantOf(const ClassDef *parent) const
{
	const ClassDef *currentParent = this;
	while(currentParent != NULL)
	{
		if(currentParent == parent)
			return true;
		currentParent = currentParent->parent;
	}
	return false;
}

void ClassDef::LoadActors()
{
	printf("ClassDef: Loading actor definitions.\n");
	atterm(&ClassDef::UnloadActors);

	// First iterate through the native classes and fix their parent pointers
	// In order to keep things simple what I did was in DeclareNativeClass I
	// force a const ClassDef ** into the parent, so we just need to cast back
	// and get the value of the pointer.
	{
		TMap<FName, ClassDef *>::Iterator iter(ClassTable());
		TMap<FName, ClassDef *>::Pair *pair;
		while(iter.NextPair(pair))
		{
			ClassDef * const cls = pair->Value;
			if(cls->parent)
				cls->parent = *(const ClassDef **)cls->parent;
		}
	}

	InitFunctionTable(NULL);

	// Add function symbols
	const ExpressionFunction *func = functions;
	do
	{
		globalSymbols.Push(new FunctionSymbol(func->name, TypeHierarchy::staticTypes.GetType(TypeHierarchy::PrimitiveTypes(func->ret)), func->args, func->function, func->takesRNG));
	}
	while((++func)->name != NULL);
	qsort(&globalSymbols[0], globalSymbols.Size(), sizeof(globalSymbols[0]), SymbolCompare);

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("DECORATE", &lastLump)) != -1)
	{
		FDecorateParser parser(lump);
		parser.Parse();
	}

	ReleaseFunctionTable();
	delete symbolPool;
#if 0
	// Debug code - Dump actor tree visually.
	DumpClasses();
#endif

	R_InitSprites();

	{
		unsigned int index = 0;

		TMap<FName, ClassDef *>::Iterator iter(ClassTable());
		TMap<FName, ClassDef *>::Pair *pair;
		while(iter.NextPair(pair))
		{
			ClassDef * const cls = pair->Value;

			if(cls->tentative)
			{
				FString error;
				error.Format("The actor '%s' is referenced but never defined.", cls->GetName().GetChars());
				throw CFatalError(error);
			}

			cls->ClassIndex = index++;
			for(unsigned int i = 0;i < cls->frameList.Size();++i)
				cls->frameList[i].spriteInf = R_GetSprite(cls->frameList[i].sprite);
		}
	}
}

// Return true if we were able to initialize the actor with whatever sane defaults we may have; false if we were not
bool ClassDef::InitializeActorClass(bool isNative)
{
	if(!isNative) // Initialize the default instance to the nearest native class.
	{
		ConstructNative = parent->ConstructNative;
		size = parent->size;

		defaultInstance = (DObject *) M_Malloc(parent->size);
		memcpy((void*)defaultInstance, (void*)parent->defaultInstance, parent->size);
	}
	else
	{
		// This could happen if a non-native actor is declared native or
		// possibly in the case of a stuck dependency.
		if(!defaultInstance)
			return false;

		// Copy the parents defaults for native classes
		if(parent)
			memcpy((void*)defaultInstance, (void*)parent->defaultInstance, parent->size);
	}

	// Copy properties and flags.
	if(parent != NULL)
	{
		memcpy((void*)defaultInstance, (void*)parent->defaultInstance, parent->size);
		defaultInstance->Class = this;
		Meta = parent->Meta;
	}

	return true;
}

void ClassDef::RegisterEdNum(unsigned int ednum)
{
	if(ClassDef **conflictClass = EditorNumberTable.CheckKey(ednum))
	{
		if(!replacee)
		{
			// Treat as a replacement. This is the best compatibility we can do
			// for now that the engine doesn't use editor numbers.
			(*conflictClass)->replacement = this;
			replacee = (*conflictClass);
		}
	}
	EditorNumberTable[ednum] = this;
}

const Frame *ClassDef::ResolveStateIndex(unsigned int index) const
{
	if(index == INT_MAX) // Deleted state (Label: stop)
		return NULL;
	if(index > frameList.Size() && parent)
		return parent->ResolveStateIndex(index - frameList.Size());
	return &frameList[index];
}

bool ClassDef::SetFlag(const ClassDef *newClass, AActor *instance, const FString &prefix, const FString &flagName, bool set)
{
	int min = 0;
	int max = sizeof(flags)/sizeof(FlagDef) - 1;
	while(min <= max)
	{
		int mid = (min+max)/2;
		int ret = flagName.CompareNoCase(flags[mid].name);
		if(ret == 0 && !prefix.IsEmpty())
			ret = prefix.CompareNoCase(flags[mid].prefix);

		if(ret == 0)
		{
			if(!newClass->IsDescendantOf(flags[mid].cls))
				return false;

			if(set)
				*reinterpret_cast<flagstype_t *>((int8_t*)instance + flags[mid].varOffset) |= flags[mid].value;
			else
				*reinterpret_cast<flagstype_t *>((int8_t*)instance + flags[mid].varOffset) &= ~flags[mid].value;
			return true;
		}
		else if(ret < 0)
			max = mid-1;
		else
			min = mid+1;
	}
	return false;
}

bool ClassDef::SetProperty(ClassDef *newClass, const char* className, const char* propName, Scanner &sc)
{
	static unsigned int NUM_PROPERTIES = 0;
	if(NUM_PROPERTIES == 0)
	{
		// Calculate NUM_PROPERTIES if needed.
		while(properties[NUM_PROPERTIES++].name != NULL)
			;
	}

	int min = 0;
	int max = NUM_PROPERTIES - 1;
	while(min <= max)
	{
		int mid = (min+max)/2;
		int ret = stricmp(properties[mid].name, propName);
		if(ret == 0)
		{
			if(!newClass->IsDescendantOf(properties[mid].className) ||
				stricmp(properties[mid].prefix, className) != 0)
				sc.ScriptMessage(Scanner::ERROR, "Property %s.%s not available in this scope.\n", properties[mid].className->name.GetChars(), propName);

			PropertyParam* params = new PropertyParam[strlen(properties[mid].params)];
			// Key:
			//   K - Keyword (Identifier)
			//   I - Integer
			//   F - Float
			//   S - String
			bool optional = false;
			bool done = false;
			const char* p = properties[mid].params;
			unsigned int paramc = 0;
			if(*p != 0)
			{
				do
				{
					if(*p != 0)
					{
						while(*p == '_') // Optional
						{
							optional = true;
							p++;
						}

						bool negate = false;
						params[paramc].i = 0; // Try to default to 0

						switch(*p)
						{
							case 'K':
								if(!optional)
									sc.MustGetToken(TK_Identifier);
								else if(!sc.CheckToken(TK_Identifier))
								{
									done = true;
									break;
								}
								params[paramc].s = new char[sc->str.Len()+1];
								strcpy(params[paramc].s, sc->str);
								break;
							default:
							case 'I':
								if(sc.CheckToken('('))
								{
									params[paramc].isExpression = true;
									params[paramc].expr = ExpressionNode::ParseExpression(newClass, TypeHierarchy::staticTypes, sc, NULL);
									sc.MustGetToken(')');
									break;
								}
								else
									params[paramc].isExpression = false;

								if(sc.CheckToken('-'))
									negate = true;

								if(!optional) // Float also includes integers
									sc.MustGetToken(TK_FloatConst);
								else if(!sc.CheckToken(TK_FloatConst))
								{
									done = true;
									break;
								}
								params[paramc].i = (negate ? -1 : 1) * static_cast<int64_t> (sc->decimal);
								break;
							case 'F':
								if(sc.CheckToken('-'))
									negate = true;

								if(!optional)
									sc.MustGetToken(TK_FloatConst);
								else if(!sc.CheckToken(TK_FloatConst))
								{
									done = true;
									break;
								}
								params[paramc].f = (negate ? -1 : 1) * sc->decimal;
								break;
							case 'S':
								if(!optional)
									sc.MustGetToken(TK_StringConst);
								else if(!sc.CheckToken(TK_StringConst))
								{
									done = true;
									break;
								}
								params[paramc].s = new char[sc->str.Len()+1];
								strcpy(params[paramc].s, sc->str);
								break;
						}
						paramc++;
						p++;
					}
					else
						sc.GetNextToken();
				}
				while(sc.CheckToken(','));
			}
			if(!optional && *p != 0 && *p != '_')
				sc.ScriptMessage(Scanner::ERROR, "Not enough parameters.");

			properties[mid].handler(newClass, (AActor*)newClass->defaultInstance, paramc, params);

			// Clean up
			p = properties[mid].params;
			for (unsigned int i = 0; i < paramc; i++)
			{
				if(*p == 0)
					break;
				if(*p == 'S' || *p == 'K')
					delete[] params[i].s;
				p++;
			}
			delete[] params;
			return true;
		}
		else if(ret > 0)
			max = mid-1;
		else
			min = mid+1;
	}
	return false;
}

void ClassDef::UnloadActors()
{
	TMap<FName, ClassDef *>::Pair *pair;

	// Clean up the frames in case any expressions use the symbols
	for(TMap<FName, ClassDef *>::Iterator iter(ClassTable());iter.NextPair(pair);)
			pair->Value->frameList.Clear();

	// Also contains code from ZDoom

	bShutdown = true;

	TArray<size_t *> uniqueFPs;
	for(TMap<FName, ClassDef *>::Iterator iter(ClassTable());iter.NextPair(pair);)
	{
		ClassDef *type = pair->Value;
		if (type->FlatPointers != &POINTER_END && type->FlatPointers != type->Pointers)
		{
			// FlatPointers are shared by many classes, so we must check for
			// duplicates and only delete those that are unique.
			unsigned int j;
			for (j = 0; j < uniqueFPs.Size(); ++j)
			{
				if (type->FlatPointers == uniqueFPs[j])
				{
					break;
				}
			}
			if (j == uniqueFPs.Size())
			{
				uniqueFPs.Push(const_cast<size_t *>(type->FlatPointers));
			}
		}
		type->FlatPointers = NULL;

		delete type;
	}
	for (unsigned int i = 0; i < uniqueFPs.Size(); ++i)
	{
		delete[] uniqueFPs[i];
	}

	// Also clear globals
	// but first clear the damage expression table since it relies on some of the symbols.
	AActor::damageExpressions.Clear();
	for(unsigned int i = 0;i < globalSymbols.Size();++i)
		delete globalSymbols[i];
}

////////////////////////////////////////////////////////////////////////////////

void ClassDef::DumpClasses()
{
	struct ClassTree
	{
		public:
			ClassTree(const ClassDef *classType) : child(NULL), next(NULL), thisClass(classType)
			{
				ClassTree **nextChild = &child;
				TMap<FName, ClassDef *>::Pair *pair;
				for(TMap<FName, ClassDef *>::Iterator iter(ClassTable());iter.NextPair(pair);)
				{
					if(pair->Value->parent == classType)
					{
						*nextChild = new ClassTree(pair->Value);
						nextChild = &(*nextChild)->next;
					}
				}
			}

			~ClassTree()
			{
				if(child != NULL)
					delete child;
				if(next != NULL)
					delete next;
			}

			void Dump(int spacing)
			{
				for(int i = spacing;i > 0;--i)
					printf("  ");
				printf("%s\n", thisClass->name.GetChars());
				if(child != NULL)
					child->Dump(spacing+1);
				if(next != NULL)
					next->Dump(spacing);
			}

			ClassTree		*child;
			ClassTree		*next;
			const ClassDef	*thisClass;
	};

	ClassTree root(FindClass("Actor"));
	root.Dump(0);
}

////////////////////////////////////////////////////////////////////////////////

CallArguments::~CallArguments()
{
	for(unsigned int i = 0;i < args.Size();++i)
	{
		if(args[i].isExpression)
			delete args[i].expr;
	}
}

void CallArguments::AddArgument(const CallArguments::Value &val)
{
	args.Push(val);
}

void CallArguments::Evaluate(AActor *self)
{
	for(unsigned int i = 0;i < args.Size();++i)
	{
		if(args[i].isExpression)
		{
			const ExpressionNode::Value &val = args[i].expr->Evaluate(self);
			if(args[i].useType == Value::VAL_INTEGER)
				args[i].val.i = val.GetInt();
			else
				args[i].val.d = val.GetDouble();
		}
	}
}

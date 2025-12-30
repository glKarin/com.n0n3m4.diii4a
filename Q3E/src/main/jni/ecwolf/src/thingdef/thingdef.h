/*
** thingdef.h
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

#ifndef __THINGDEF_H__
#define __THINGDEF_H__

#include "actor.h"
#include "tarray.h"
#include "wl_def.h"
#include "zstring.h"

class ClassDef;
class Frame;
class Symbol;
class ExpressionNode;
class Type;
class Scanner;

class StateLabel
{
	public:
		StateLabel() : isDefault(false), isRelative(false) {}
		StateLabel(const FString &str, const ClassDef *parent, bool noRelative=false);
		StateLabel(Scanner &sc, const ClassDef *parent, bool noRelative=false);

		const Frame	*Resolve() const;
		const Frame	*Resolve(AActor *owner, const Frame *caller, const Frame *def=NULL) const;

	private:
		void	Parse(Scanner &sc, const ClassDef *parent, bool noRelative=false);

		const ClassDef	*cls;
		FString 		label;
		unsigned short	offset;
		bool			isDefault;
		bool			isRelative;
};

class CallArguments
{
	public:
		class Value
		{
			public:
				enum
				{
					VAL_INTEGER,
					VAL_DOUBLE,
					VAL_STRING,
					VAL_STATE
				} useType;
				bool isExpression;

				ExpressionNode	*expr;
				union
				{
					int64_t	i;
					double	d;
				}				val;
				FString			str;
				StateLabel		label;
		};

		~CallArguments();

		void		AddArgument(const Value &val);
		int			Count() const { return args.Size(); }
		void		Evaluate(AActor *self);
		const Value	&operator[] (unsigned int idx) const { return args[idx]; }
	private:
		TArray<Value> args;
};

class ActionInfo
{
	public:
		ActionInfo(ActionPtr func, const FName &name);

		const Type *ArgType(unsigned int n) const { return types[MIN(n, maxArgs-1)]; }

		ActionPtr func;
		const FName name;

		unsigned int					minArgs;
		unsigned int					maxArgs;
		bool							varArgs;
		TArray<CallArguments::Value>	defaults;
		TArray<const Type *>			types;
};

typedef TArray<ActionInfo *> ActionTable;

#define ACTION_FUNCTION(func) \
	bool __AF_##func(AActor *, AActor *, const Frame * const, const CallArguments &, struct ActionResult *); \
	static const ActionInfo __AI_##func(__AF_##func, #func); \
	bool __AF_##func(AActor *self, AActor *stateOwner, const Frame * const caller, const CallArguments &args, struct ActionResult *result)
#define ACTION_ALIAS(func, alias) \
	ACTION_FUNCTION(alias) \
	{ \
		return __AF_##func(self, stateOwner, caller, args, result); \
	}
#define CALL_ACTION(func, self) \
	bool __AF_##func(AActor *, AActor *, const Frame * const, const CallArguments &, struct ActionResult *); \
	__AF_##func(self, self, NULL, CallArguments(), NULL);
#define ACTION_PARAM_COUNT args.Count()
#define ACTION_PARAM_BOOL(name, num) \
	bool name = args[num].val.i ? true : false
#define ACTION_PARAM_INT(name, num) \
	int name = static_cast<int>(args[num].val.i)
#define ACTION_PARAM_DOUBLE(name, num) \
	double name = args[num].val.d
#define ACTION_PARAM_FIXED(name, num) \
	fixed name = static_cast<fixed>(args[num].val.d*FRACUNIT)
#define ACTION_PARAM_STRING(name, num) \
	FString name = args[num].str
#define ACTION_PARAM_STATE(name, num, def) \
	const Frame *name = args[num].label.Resolve(stateOwner, caller, def)

class SymbolInfo
{
	public:
		static const SymbolInfo *LookupSymbol(const ClassDef *cls, FName var);

		SymbolInfo(const ClassDef *cls, const FName var, const int offset);

		const ClassDef	* const cls;
		const FName		var;
		const int		offset;
};

#define DEFINE_SYMBOL(cls, var) \
	static const SymbolInfo __SI_##var(NATIVE_CLASS(cls), #var, typeoffsetof(A##cls,var));

struct StateDefinition;

struct PropertyParam
{
	bool isExpression;
	union
	{
		ExpressionNode	*expr;
		char			*s;
		double			f;
		int64_t			i;
	};
};
typedef void (*PropHandler)(ClassDef *info, AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params);
#define HANDLE_PROPERTY(property) void __Handler_##property(ClassDef *cls, AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params)
struct PropDef
{
	public:
		const ClassDef* const	&className;
		const char* const		prefix;
		const char* const		name;
		const char* const		params;
		PropHandler				handler;
};

typedef TArray<Symbol *> SymbolTable;

class MetaTable
{
	public:
		MetaTable();
		MetaTable(const MetaTable &other);
		~MetaTable();

		enum Type
		{
			INTEGER,
			FIXED,
			STRING
		};

		int			GetMetaInt(uint32_t id, int def=0) const;
		fixed		GetMetaFixed(uint32_t id, fixed def=0) const;
		const char*	GetMetaString(uint32_t id) const;
		bool		IsInherited(uint32_t id);
		void		SetMetaInt(uint32_t id, int value);
		void		SetMetaFixed(uint32_t id, fixed value);
		void		SetMetaString(uint32_t id, const char* value);

		const MetaTable &operator= (const MetaTable &other) { CopyMeta(other); return *this; }

	private:
		class Data;

		Data	*head;
		Data	*FindMeta(uint32_t id) const;
		Data	*FindMetaData(uint32_t id);

		void	CopyMeta(const MetaTable &other);
		void	FreeTable();
};

class ClassDef
{
	public:
		ClassDef();
		~ClassDef();

		AActor					*CreateInstance() const;
		bool					IsAncestorOf(const ClassDef *child) const { return child->IsDescendantOf(this); }
		bool					IsDescendantOf(const ClassDef *parent) const;

		/**
		 * Use with IMPLEMENT_CLASS to add a natively defined class.
		 */
		template<class T>
		static const ClassDef	*DeclareNativeClass(const char* className, const ClassDef **parent)
		{
			ClassDef **definitionLookup = ClassTable().CheckKey(className);
			ClassDef *definition = NULL;
			if(definitionLookup == NULL)
			{
				definition = new ClassDef();
				ClassTable()[className] = definition;
			}
			else
				definition = *definitionLookup;
			definition->Pointers = *T::__PointerOffsets == POINTER_END ? NULL : T::__PointerOffsets;
			definition->name = className;
			// We will get the real value of this later before we load the actors
			definition->parent = (const ClassDef *)parent;
			definition->size = sizeof(T);
			definition->defaultInstance = (DObject *) M_Malloc(definition->size);
			memset((void*)definition->defaultInstance, 0, definition->size);
			definition->ConstructNative = &T::__InPlaceConstructor;
			return definition;
		}

		typedef TMap<FName, ClassDef*>::ConstIterator	ClassIterator;
		typedef TMap<FName, ClassDef*>::ConstPair		ClassPair;
		static ClassIterator	GetClassIterator()
		{
			return ClassIterator(ClassTable());
		}
		static unsigned int		GetNumClasses() { return ClassTable().CountUsed(); }

		/**
		 * Prints the implemented classes in a tree.  This is not designed to 
		 * be fast since it's debug information more than anything.
		 */
		static void				DumpClasses();

		static const ClassDef	*FindClass(unsigned int ednum);
		static const ClassDef	*FindClass(const FName &className);
		static const ClassDef	*FindClassTentative(const FName &className, const ClassDef *parent);
		static const ClassDef	*FindConversationClass(unsigned int convid);
		const ActionInfo		*FindFunction(const FName &function, int &specialNum) const;
		const Frame				*FindState(const FName &stateName) const;
		Symbol					*FindSymbol(const FName &symbol) const;
		AActor					*GetDefault() const { return (AActor*)defaultInstance; }
		const FName				&GetName() const { return name; }
		const ClassDef			*GetParent() const { return parent; }
		const ClassDef			*GetReplacement(bool respectMapinfo=true) const;
		size_t					GetSize() const { return size; }
		const Frame				*GetState(unsigned int index) const { return &frameList[index]; }
		static void				LoadActors();
		bool					IsStateOwner(const Frame *frame) const { return frame >= &frameList[0] && frame < &frameList[frameList.Size()]; }
		static void				UnloadActors();

		unsigned int			ClassIndex;
		MetaTable				Meta;

		static bool	SetFlag(const ClassDef *newClass, AActor *instance, const FString &prefix, const FString &flagName, bool set);
	protected:
		friend class DObject;
		friend class StateLabel;
		friend class FDecorateParser;
		static const size_t POINTER_END;

		static bool SetProperty(ClassDef *newClass, const char* className, const char* propName, Scanner &sc);

		static void AddGlobalSymbol(Symbol *sym);
		void		BuildFlatPointers();
		const Frame *FindStateInList(const FName &stateName) const;
		void		FinalizeActorClass();
		bool		InitializeActorClass(bool isNative);
		void		InstallStates(const TArray<StateDefinition> &stateDefs);
		void		RegisterEdNum(unsigned int ednum);
		const Frame *ResolveStateIndex(unsigned int index) const;

		// We need to do this for proper initialization order.
		static TMap<FName, ClassDef *>	&ClassTable();
		static SymbolTable				globalSymbols;

		bool			tentative;
		FName			name;
		const ClassDef	*parent;
		size_t			size;

		const ClassDef	*replacement;
		const ClassDef	*replacee;

		TMap<FName, unsigned int> stateList;
		TArray<Frame> frameList;

		ActionTable		actions;
		SymbolTable		symbols;

		const size_t	*Pointers;		// object pointers defined by this class *only*
		const size_t	*FlatPointers;	// object pointers defined by this class and all its superclasses; not initialized by default

		DObject			*defaultInstance;
		DObject			*(*ConstructNative)(const ClassDef *, void *);

		static bool		bShutdown;
};

// Functions below are actually a part of dobject.h, but moved here for dependency reasons
inline bool DObject::IsSameKindOf (const ClassDef *base, const ClassDef *other) const
{
	const ClassDef *cls = GetClass();

	if(cls == other)
		return true;

	while(cls->GetParent() != base)
		cls = cls->GetParent();
	return cls->IsAncestorOf(other);
}

inline bool DObject::IsKindOf (const ClassDef *base) const
{
	return base->IsAncestorOf (GetClass ());
}

inline bool DObject::IsA (const ClassDef *type) const
{
	return (type == GetClass());
}

#endif /* __THINGDEF_H__ */

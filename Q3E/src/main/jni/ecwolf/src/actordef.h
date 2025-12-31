/*
** actordef.h
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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

#ifndef __ACTORDEF_H__
#define __ACTORDEF_H__

#include "dobject.h"
#include "tarray.h"

#define DECLARE_ABSTRACT_CLASS(name, parent) \
	friend class ClassDef; \
	private: \
		typedef parent Super; \
		typedef name ThisClass; \
	protected: \
		name(const ClassDef *classType) : parent(classType) {} \
		virtual const ClassDef *__StaticType() const { return __StaticClass; } \
	public: \
		static const ClassDef *__StaticClass; \
		static const size_t __PointerOffsets[];
#define DECLARE_CLASS(name, parent) \
	DECLARE_ABSTRACT_CLASS(name, parent) \
	protected: \
		static DObject *__InPlaceConstructor(const ClassDef *classType, void *mem); \
	public:
#define DECLARE_NATIVE_CLASS(name, parent) DECLARE_CLASS(A##name, A##parent)
#define HAS_OBJECT_POINTERS
#define __IMPCLS_ABSTRACT(cls, name) \
	const ClassDef *cls::__StaticClass = ClassDef::DeclareNativeClass<cls>(name, &Super::__StaticClass);
#define __IMPCLS(cls, name) \
	__IMPCLS_ABSTRACT(cls, name) \
	DObject *cls::__InPlaceConstructor(const ClassDef *classType, void *mem) { return new ((EInPlace *) mem) cls(classType); }
#define IMPLEMENT_ABSTRACT_CLASS(cls) \
	__IMPCLS_ABSTRACT(cls, #cls) \
	const size_t cls::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_INTERNAL_CLASS(cls) \
	__IMPCLS(cls, #cls) \
	const size_t cls::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_INTERNAL_POINTY_CLASS(cls) \
	__IMPCLS(cls, #cls) \
	const size_t cls::__PointerOffsets[] = {
#define IMPLEMENT_CLASS(name) \
	__IMPCLS(A##name, #name) \
	const size_t A##name::__PointerOffsets[] = { ~(size_t)0 };
#define IMPLEMENT_POINTY_CLASS(name) \
	__IMPCLS(A##name, #name) \
	const size_t A##name::__PointerOffsets[] = {
// Similar to typeoffsetof, but doesn't cast to int.
#define DECLARE_POINTER(ptr) \
	(size_t)&((ThisClass*)1)->ptr - 1,
#define END_POINTERS ~(size_t)0 };
#define NATIVE_CLASS(name) A##name::__StaticClass

class AActor;
class CallArguments;
class ExpressionNode;

typedef bool (*ActionPtr)(AActor *, AActor *, const class Frame * const, const CallArguments &, struct ActionResult *);

class Frame
{
	public:
		~Frame();
		int GetTics() const;

		union
		{
			char	sprite[4];
			uint32_t isprite;
		};
		uint8_t		frame;
		int			duration;
		unsigned	randDuration;
		bool		fullbright;
		fixed_t		offsetX;
		fixed_t		offsetY;
		class ActionCall
		{
			public:
				ActionPtr		pointer;
				CallArguments	*args;

				bool operator() (AActor *self, AActor *stateOwner, const Frame * const caller, struct ActionResult *result=NULL) const;
		} action, thinker;
		const Frame	*next;
		unsigned int	index;

		unsigned int	spriteInf;

		bool	freeActionArgs;
};
FArchive &operator<< (FArchive &arc, const Frame *&frame);

// This class allows us to store pointers into the meta table and ensures that
// the pointers get deleted when the game exits.
template<class T>
class PointerIndexTable
{
public:
	~PointerIndexTable()
	{
		Clear();
	}

	void Clear()
	{
		for(unsigned int i = 0;i < objects.Size();++i)
			delete objects[i];
		objects.Clear();
	}

	unsigned int Push(T *object)
	{
		return objects.Push(object);
	}

	T *operator[] (unsigned int index)
	{
		return objects[index];
	}
private:
	TArray<T*>	objects;
};

struct ActionResult
{
	const Frame *JumpFrame;
};

#endif

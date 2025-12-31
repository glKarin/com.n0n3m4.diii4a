/*
** thinker.cpp
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
** Thinkers are given priorities, one of which (TRAVEL) allows us to transfer
** actors between levels without it being collected.  This is similar to ZDoom's
** system, which in turn, is supposedly based off build.
**
*/

#include "farchive.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "wl_def.h"
#include "wl_game.h"
#include "wl_loadsave.h"

ThinkerList thinkerList;

ThinkerList::ThinkerList() : nextThinker(NULL)
{
}

ThinkerList::~ThinkerList()
{
	DestroyAll(static_cast<Priority>(0));
}

void ThinkerList::DestroyAll(Priority start)
{
	for(unsigned int i = start;i < NUM_TYPES;++i)
	{
		Iterator iter = thinkers[i].Head();
		while(iter)
		{
			Thinker *thinker = iter++;

			if(!(thinker->ObjectFlags & OF_EuthanizeMe))
				thinker->Destroy();
		}
	}
	GC::FullGC();
}

void ThinkerList::MarkRoots()
{
	for(unsigned int i = 0;i < NUM_TYPES;++i)
	{
		Iterator iter(thinkers[i]);
		while(iter.Next())
		{
			Thinker *thinker = iter;
			if(!(thinker->ObjectFlags & OF_EuthanizeMe))
			{
				GC::Mark(thinker);
				break;
			}
		}
	}
}

void ThinkerList::Tick()
{
	for(unsigned int i = FIRST_TICKABLE;i < NUM_TYPES;++i)
	{
		if(gamestate.victoryflag && i > VICTORY)
			break;

		Tick(static_cast<Priority>(i));
	}
}

void ThinkerList::Tick(Priority list)
{
	Iterator iter = thinkers[list].Head();
	while(iter)
	{
		Thinker *thinker = iter;
		nextThinker = ++iter;

		if(thinker->ObjectFlags & OF_JustSpawned)
		{
			thinker->ObjectFlags &= ~OF_JustSpawned;
			thinker->PostBeginPlay();
		}

		if(!(thinker->ObjectFlags & OF_EuthanizeMe))
		{
			thinker->Tick();
			GC::CheckGC();
		}

		iter = nextThinker;
	}
}

void ThinkerList::Serialize(FArchive &arc)
{
	if(arc.IsStoring())
	{
		for(unsigned int i = 0;i < NUM_TYPES;i++)
		{
			Iterator iter(thinkers[i]);
			while(iter.Next())
			{
				Thinker *thinker = iter;
				arc << thinker;
			}

			Thinker *terminator = NULL;
			arc << terminator; // Terminate list
		}
	}
	else
	{
		for(unsigned int i = 0;i < NUM_TYPES;i++)
		{
			Thinker *thinker;
			arc << thinker;
			while(thinker)
			{
				// FIXME: Remove this save compat hack in 1.4
				if(thinker->IsThinkerType<AActorProxy>())
				{
					Thinker *real = ((AActorProxy*)thinker)->actualObject;
					thinker->Destroy();
					thinker = real;
				}
				Register(thinker, static_cast<Priority>(i));
				arc << thinker;
			}
		}
	}
}

void ThinkerList::Register(Thinker *thinker, Priority type)
{
	thinkers[type].Push(thinker);
	thinker->thinkerPriority = type;

	Iterator head(thinker);
	if(head.Next())
	{
		GC::WriteBarrier(thinker, head);
		GC::WriteBarrier(head, thinker);
	}
	GC::WriteBarrier(thinker);
}

void ThinkerList::Deregister(Thinker *thinker)
{
	Thinker * const prev = static_cast<Thinker*>(thinker->elPrev);
	Thinker * const next = static_cast<Thinker*>(thinker->elNext);

	// If we're about to think this thinker then we should probably skip it.
	if(nextThinker == thinker)
		nextThinker = next;

	thinkers[thinker->thinkerPriority].Remove(thinker);
	if(prev && next)
	{
		GC::WriteBarrier(prev, next);
		GC::WriteBarrier(next, prev);
	}
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ABSTRACT_CLASS(Thinker)

Thinker::Thinker(ThinkerList::Priority priority)
{
	Activate(priority);
}

void Thinker::Activate(ThinkerList::Priority priority)
{
	thinkerList.Register(this, priority);
}

void Thinker::Deactivate()
{
	thinkerList.Deregister(this);
}

void Thinker::Destroy()
{
	if(IsThinking())
		thinkerList.Deregister(this);
	Super::Destroy();
}

void Thinker::Init()
{
	Super::Init();
	EmbeddedList<Thinker>::List::ValidateNode(this);
}

size_t Thinker::PropagateMark()
{
	if(IsThinking())
	{
		Thinker *next = static_cast<Thinker*>(elNext);
		Thinker *prev = static_cast<Thinker*>(elPrev);
		if(next)
		{
			assert(!(next->ObjectFlags & OF_EuthanizeMe));
			GC::Mark(next);
		}

		if(prev)
		{
			assert(!(prev->ObjectFlags & OF_EuthanizeMe));
			GC::Mark(prev);
		}
	}
	return Super::PropagateMark();
}

void Thinker::Serialize(FArchive &arc)
{
	if(GameSave::SaveVersion > 1451884199)
	{
		BYTE priority = thinkerPriority;
		arc << priority;
		thinkerPriority = static_cast<ThinkerList::Priority>(priority);
	}
	else
		thinkerPriority = ThinkerList::NORMAL;

	Super::Serialize(arc);
}

void Thinker::SetPriority(ThinkerList::Priority priority)
{
	Deactivate();
	Activate(priority);
}

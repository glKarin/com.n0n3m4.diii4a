/*
EventSystem.h -- event system implementation
Copyright(C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#pragma once
#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H

#include "Utils.h"

// Use these macros to set EventCallback, if no function pointer is available

// SET_EVENT_MULTI( event, callback )
// event -- CEventCallback object
// callback -- callback code block. Must contain a braces

// SET_EVENT( event, callback )
// same as MULTI, but does not require code block. Inteded for one-line events

#if defined(MY_COMPILER_SUCKS)
// WARN: can't rely on "item" name, because it can be something like "ptr->member"
// So we use something more valid for struct name
#define PASTE(x,y) __##x##_##y
#define PASTE2(x,y) PASTE(x,y)
#define EVNAME(x) PASTE2(x, __LINE__)

#define SET_EVENT_MULTI( event, callback ) \
	typedef struct                                             \
	{                                                          \
		static void __cb( CMenuBaseItem *pSelf, void *pExtra ) \
		callback                                               \
	} EVNAME( _ev ); (event) = EVNAME( _ev )::__cb

#else

#define SET_EVENT_MULTI( event, callback ) \
	(event) = [](CMenuBaseItem *pSelf, void *pExtra) callback
#endif

#define SET_EVENT( event, callback ) SET_EVENT_MULTI( event, { callback; } )

#define DECLARE_NAMED_EVENT_TO_ITEM_METHOD( className, method, eventName ) \
	static void eventName##Cb( CMenuBaseItem *pSelf, void * ) \
	{\
		((className *)pSelf)->method();\
	}

#define DECLARE_EVENT_TO_ITEM_METHOD( className, method ) \
	DECLARE_NAMED_EVENT_TO_ITEM_METHOD( className, method, method )

#ifdef _MSC_VER
#pragma pointers_to_members( full_generality, virtual_inheritance )  
#endif
class CMenuBaseItem;
class CMenuItemsHolder;

enum menuEvent_e
{
	QM_GOTFOCUS = 1,
	QM_LOSTFOCUS,
	QM_RELEASED,
	QM_CHANGED,
	QM_PRESSED,
	QM_IMRESIZED
};

typedef void (*EventCallback)( CMenuBaseItem *, void * ); // extradata
typedef void (*VoidCallback)( void ); // no extradata
typedef void (CMenuItemsHolder::*IHCallback)( void * ); // extradata
typedef void (CMenuItemsHolder::*VoidIHCallback)(); // no extradata

#define MenuCb( a ) static_cast<IHCallback>((a))
#define VoidCb( a ) static_cast<VoidIHCallback>((a))

class CEventCallback
{
public:
	CEventCallback();
	CEventCallback( EventCallback cb, void *ex = 0 );
	CEventCallback( IHCallback cb, void *ex = 0 );
	CEventCallback( VoidCallback cb );
	CEventCallback( VoidIHCallback cb );
	CEventCallback( int execute_now, const char *sz );

	void *pExtra;

	// convert to boolean for easy check in conditionals
	operator bool()
	{
		switch( type )
		{
		case CB_OLD_EXTRA: return u.cb != 0;
		case CB_OLD_VOID:  return u.voidCb != 0;
		case CB_IH_EXTRA:  return u.itemsHolderCb != 0;
		case CB_IH_VOID:   return u.voidItemsHolderCb != 0;
		}
		return false;
	}

	void operator() ( CMenuBaseItem *pSelf );

	// ItemCallback operator =( ItemCallback cb );
	IHCallback    operator=( IHCallback cb );
	VoidIHCallback operator=( VoidIHCallback cb );
	VoidCallback  operator=( VoidCallback cb );
	EventCallback operator=( EventCallback cb );

	// NULL assignment
	size_t        operator=( size_t null );
	void*         operator=( void *null );

	void Reset();

	void SetCommand( int execute_now, const char *sz );

	static void NoopCb( CMenuBaseItem *, void * ) {}
private:
	// matches type in union
	enum
	{
		CB_OLD_EXTRA = 0,
		CB_OLD_VOID,
		CB_IH_EXTRA,
		CB_IH_VOID
	} type;

	// event can be only one type at once,
	// so place in union for memory
	union
	{
		EventCallback cb;
		VoidCallback voidCb;
		IHCallback   itemsHolderCb;
		VoidIHCallback voidItemsHolderCb;
	} u;

	// to find event command by name(for items holder)
	const char *szName;

	static void CmdExecuteNextFrameCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		EngFuncs::ClientCmd( FALSE, (char *)pExtra );
	}

	static void CmdExecuteNowCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		EngFuncs::ClientCmd( TRUE, (char *)pExtra );
	}

	friend class CMenuItemsHolder;
};


#endif // EVENTSYSTEM_H

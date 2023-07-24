// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UIBinder.h"

#include "../../sys/sys_local.h"

SD_UI_IMPLEMENT_CLASS( sdUIBinder, sdUIWindow )

SD_UI_PUSH_CLASS_TAG( sdUIBinder )
const char* const sdUIBinder::eventNames[ BE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onBindComplete",	"",		"When a new key was bound to the bind command." ),
};
SD_UI_POP_CLASS_TAG

idHashMap< sdUITemplateFunction< sdUIBinder >* >	sdUIBinder::binderFunctions;
const char sdUITemplateFunctionInstance_IdentifierBinder[] = "sdUIBinderFunction";

/*
============
sdUIBinder::sdUIBinder
============
*/
sdUIBinder::sdUIBinder() :
	bindIndex( 0.0f ),
	currentKey( NULL ) {
	scriptState.GetProperties().RegisterProperty( "bindCommand", bindCommand );
	scriptState.GetProperties().RegisterProperty( "bindIndex", bindIndex );
	scriptState.GetProperties().RegisterProperty( "bindMessage", bindMessage );
}

/*
============
sdUIBinder::PostEvent
============
*/
bool sdUIBinder::PostEvent( const sdSysEvent* event ) {
	if ( bindCommand.GetValue().Length() == 0 || !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	bool consoleKey = sys->Keyboard().IsConsoleKey( *event );
	if ( consoleKey ) {
		return false;
	}

	if ( event->IsMouseEvent() ) {
		return false;
	}
	
	bool down;
	idKey* key = keyInputManager->GetKeyForEvent( *event, down );
	if ( !down || key == NULL ) {
		return false;
	}

	// those can be bound fine under Linux
#ifndef __linux__
	if ( event->GetKey() == K_LWIN || event->GetKey() == K_RWIN ) {
		return false;
	}
#endif

	currentKey = key;

	idStr binding = keyInputManager->GetBinding( gameLocal.GetDefaultBindContext(), *currentKey, NULL );
	if( !binding.IsEmpty() ) {
		idWStrList args( 2 );
		args.SetNum( 2 );
		currentKey->GetLocalizedText( args[ 0 ] );
		args[ 1 ] = va( L"%hs", binding.c_str() );
		bindMessage = common->LocalizeText( "guis/mainmenu/bindmessage", args );
	} else {
		idWStrList args( 1 );
		args.SetNum( 1 );
		currentKey->GetLocalizedText( args[ 0 ] );
		bindMessage = common->LocalizeText( "guis/mainmenu/unboundmessage", args );
	}

	return true;
}

/*
============
sdUIBinder::EnumerateEvents
============
*/
void sdUIBinder::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onBindComplete" ) ) {
		events.Append( sdUIEventInfo( BE_BIND_COMPLETE, 0 ) );
		return;
	}

	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIBinder::InitFunctions
============
*/
SD_UI_PUSH_CLASS_TAG( sdUIBinder )
void sdUIBinder::InitFunctions() {
	SD_UI_FUNC_TAG( applyBinding, "Apply the current key registered with the binder window." )
	SD_UI_END_FUNC_TAG
	binderFunctions.Set( "applyBinding", new sdUITemplateFunction< sdUIBinder >( 'v', "", &sdUIBinder::Script_ApplyBinding ) );

	SD_UI_FUNC_TAG( unbindBinding, "Unbind all keys bound to the bind command." )
	SD_UI_END_FUNC_TAG
	binderFunctions.Set( "unbindBinding", new sdUITemplateFunction< sdUIBinder >( 'v', "", &sdUIBinder::Script_UnbindBinding ) );
}
SD_UI_POP_CLASS_TAG


/*
============
sdUIBinder::FindFunction
============
*/
const sdUITemplateFunction< sdUIBinder >* sdUIBinder::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIBinder >** ptr;
	return binderFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIBinder::GetFunction
============
*/
sdUIFunctionInstance* sdUIBinder::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIBinder >* function = sdUIBinder::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIBinder, sdUITemplateFunctionInstance_IdentifierBinder >( this, function );
}

/*
============
sdUIBinder::RunNamedMethod
============
*/
bool sdUIBinder::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIBinder >* func = sdUIBinder::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIBinder::Script_ApplyBinding
============
*/
void sdUIBinder::Script_ApplyBinding( sdUIFunctionStack& stack ) {
	if( currentKey == NULL ) {
		return;
	}
	idStr binding( va( "%ls", bindCommand.GetValue().c_str() ) );

	int numKeys = 0;
	keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), binding.c_str(), numKeys, NULL );

	if( numKeys > 0 ) {
		idKey** keys = static_cast< idKey** >( _alloca( numKeys * sizeof( idKey* ) ) );
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), binding.c_str(), numKeys, keys );
		int index = idMath::Ftoi( bindIndex );
		if( index >= 0 && index < numKeys ) {
			keyInputManager->UnbindKey( gameLocal.GetDefaultBindContext(), *keys[ index ], NULL );
		}
	}

	keyInputManager->SetBinding( gameLocal.GetDefaultBindContext(), *currentKey, binding.c_str(), NULL );

	RunEvent( sdUIEventInfo( BE_BIND_COMPLETE, 0 ) );
	currentKey = NULL;
}

/*
============
sdUIBinder::Script_UnbindBinding
============
*/
void sdUIBinder::Script_UnbindBinding( sdUIFunctionStack& stack ) {
	if( bindCommand.GetValue().IsEmpty() ) {
		return;
	}
	idStr binding( va( "%ls", bindCommand.GetValue().c_str() ) );

	int numKeys = 0;
	keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), binding.c_str(), numKeys, NULL );

	if( numKeys > 0 ) {
		idKey** keys = static_cast< idKey** >( _alloca( numKeys * sizeof( idKey* ) ) );

		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), binding.c_str(), numKeys, keys );
		int index = idMath::Ftoi( bindIndex );
		if( index >= 0 && index < numKeys ) {
			keyInputManager->UnbindKey( gameLocal.GetDefaultBindContext(), *keys[ index ], NULL );
		}
	}
}

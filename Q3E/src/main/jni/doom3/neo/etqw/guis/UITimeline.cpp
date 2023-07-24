// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceLocal.h"
#include "UserInterfaceTypes.h"
#include "UITimeline.h"
#include "UIWindow.h"


const char	sdUITimelineEvent_Identifier[]				= "sdUITimelineEvent";
const char	sdUITemplateFunctionInstance_Identifier[]	= "sdUITemplateFunctionInstance";

/*
============
sdUITimelineManager::sdUITimelineManager
============
*/
sdUITimelineManager::sdUITimelineManager( sdUserInterfaceLocal& ui_, sdUserInterfaceScope& enclosingScope_, sdUIScript& script_ ) :
	enclosingScope( enclosingScope_ ),
	ui( ui_ ),
	script( script_ ) {
}

/*
============
sdUITimelineManager::CreateTimelines
============
*/
bool sdUITimelineManager::CreateTimelines( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl ) {
	for( int timelineIndex = 0; timelineIndex < timelineInfo.GetNumTimelines(); timelineIndex++ ) {
		const char* timelineName = timelineInfo.GetTimelineName( timelineIndex );

		sdUITimeline* newTimeline = new sdUITimeline( timelineName, this );
		timelines[ timelineName ] = newTimeline;
	}
	return true;
}


/*
============
sdUITimelineManager::CreateEvents
============
*/
bool sdUITimelineManager::CreateEvents( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl, idTokenCache& tokenCache ) {
	if( timelines.Num() != timelineInfo.GetNumTimelines() ) {
		assert( 0 );
		return false;
	}

	for( int timelineIndex = 0; timelineIndex < timelines.Num(); timelineIndex++ ) {
		sdUITimeline* newTimeline = timelines.FindIndex( timelineIndex )->second;

		newTimeline->CreateEvents( *timelineInfo.GetTimeline( timelineIndex ), guiDecl, tokenCache );
	}
	return true;
}

/*
============
sdUITimelineManager::CreateProperties
============
*/
bool sdUITimelineManager::CreateProperties( const sdDeclGUITimelineHolder& timelineInfo, const sdDeclGUI* guiDecl, const idTokenCache& tokenCache ) {
	if( timelines.Num() != timelineInfo.GetNumTimelines() ) {
		assert( 0 );
		return false;
	}

	for( int timelineIndex = 0; timelineIndex < timelines.Num(); timelineIndex++ ) {
		sdUITimeline* newTimeline = timelines.FindIndex( timelineIndex )->second;

		newTimeline->CreateProperties( *timelineInfo.GetTimeline( timelineIndex ), guiDecl, tokenCache );
	}
	return true;
}


/*
============
sdUITimelineManager::GetSubScope
============
*/
sdUserInterfaceScope* sdUITimelineManager::GetSubScope( const char* name ) {
	timelineHash_t::Iterator iter = timelines.Find( name );
	if( iter != timelines.End() ) {
		return iter->second;
	}
	return NULL;
}

/*
============
sdUITimelineManager::Clear
============
*/
void sdUITimelineManager::Clear() {
	timelines.DeleteValues();
	timelines.Clear();
	resetScopes.SetNum( 0, false );
	timelineEvents.DeleteContents( true );
}

/*
================
sdUITimelineManager::PostTimelineEvent
================
*/
void sdUITimelineManager::PostTimelineEvent( int offset, int index, sdUITimeline* scope ) {
	sdUITimelineEvent* event = new sdUITimelineEvent;
	event->index = index;
	event->timeline = scope;
	event->time	= ui.GetCurrentTime() + offset;

	timelineEvents.Append( event );
}

/*
================
sdUITimelineManager::ClearTimelineEvents
================
*/
void sdUITimelineManager::ClearTimelineEvents( sdUITimeline* scope ) {
	int i;
	for ( i = timelineEvents.Num() - 1; i >= 0; i-- ) {
		if ( timelineEvents[ i ]->timeline == scope ) {
			delete timelineEvents[ i ];
			timelineEvents.RemoveIndex( i );
		}
	}
}

/*
============
sdUITimelineManager::ResetAllTimelines
============
*/
void sdUITimelineManager::ResetAllTimelines() {
	for( int i = 0; i < timelines.Num(); i++ ) {
		sdUITimeline* timeline = timelines.FindIndex( i )->second;
		timeline->Reset();
	}
}

/*
============
sdUITimelineManager::ClearAllTimelines
============
*/
void sdUITimelineManager::ClearAllTimelines() {
	for( int i = 0; i < timelines.Num(); i++ ) {
		sdUITimeline* timeline = timelines.FindIndex( i )->second;
		timeline->Clear();
	}
}

/*
============
sdUITimelineManager::PushAllTimelines
============
*/
void sdUITimelineManager::PushAllTimelines() {
	for( int i = 0; i < timelines.Num(); i++ ) {
		sdUITimeline* timeline = timelines.FindIndex( i )->second;
		timeline->PushEvents();
	}
}


/*
============
sdUITimelineManager::OnSnapshotHitch
============
*/
void sdUITimelineManager::OnSnapshotHitch( int delta ) {
	for ( int i = 0; i < timelineEvents.Num(); i++ ) {
		sdUITimelineEvent* event = timelineEvents[ i ];
		event->time -= delta;
	}
}

/*
============
sdUITimelineManager::Run
============
*/
void sdUITimelineManager::Run( int time ) {
	for ( int i = 0; i < timelineEvents.Num(); ) {
		sdUITimelineEvent* event = timelineEvents[ i ];
		if ( time >= event->time ) {
			event->timeline->RunTimeEvent( event->index );
			if ( timelineEvents.Remove( event ) ) {
				delete event;
			}
			i = 0;
		} else {
			i++;
		}
	}
	for ( int i = 0; i < resetScopes.Num(); i++ ) {
		resetScopes[ i ]->Reset();
	}
	resetScopes.SetNum( 0, false );
}

const char sdUITemplateFunctionInstance_IdentifierTimeline[] = "sdUITimelineFunction";
idHashMap< sdUITemplateFunction< sdUITimeline >* >	sdUITimeline::timelineFunctions;

/*
============
sdUITimeline::sdUITimeline
============
*/
sdUITimeline::sdUITimeline( const char* name_, sdUITimelineManager* manager_ ) :
	name( name_ ),
	needsReset( false ),	
	resetTime( 0 ),
	active( 1.0f ),
	manager( manager_ ) {

	events.SetNumEvents( TE_NUM_EVENTS );
	
	properties.RegisterProperty( "active", active );
	properties.RegisterProperty( "name", name );

	name.SetReadOnly( true );

	UI_ADD_FLOAT_CALLBACK( active, sdUITimeline, OnActiveChanged );
}


/*
============
sdUITimeline::RunTimeEvent
============
*/
bool sdUITimeline::RunTimeEvent( int index ) {
	return script.RunEventHandle( events.GetEvent( sdUIEventInfo( TE_ONTIME, index ) ), this );
}

/*
============
sdUITimeline::GetProperty
============
*/
sdProperties::sdProperty* sdUITimeline::GetProperty( const char* name ) {
	using namespace sdProperties;
	sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if( !prop ) {
		return manager->GetScope().GetProperty( name );
	}
	return prop;
}

/*
============
sdUITimeline::GetProperty
============
*/
sdProperties::sdProperty* sdUITimeline::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	using namespace sdProperties;
	sdProperty* prop = properties.GetProperty( name, type, false );
	if( !prop ) {
		return manager->GetScope().GetProperty( name, type );
	}
	return prop;
}

/*
============
sdUITimeline::FindPropertyName
============
*/
const char* sdUITimeline::FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) {
	scope = this;
	return properties.NameForProperty( property );
}

/*
============
sdUITimeline::PushEvents
============
*/
void sdUITimeline::PushEvents() {
	Clear();

	if( active == 0.0f ) {
		return;
	}

	for( int i = 0; i < timeline.Num(); i++ ) {
		if( timeline[ i ] >= resetTime ) {
			manager->PostTimelineEvent( timeline[ i ] - resetTime, i, this );
		}
	}
}

/*
============
sdUITimeline::Clear
============
*/
void sdUITimeline::Clear() {
	manager->ClearTimelineEvents( this );
}

/*
============
sdUITimeline::GetFunction
============
*/
sdUIFunctionInstance* sdUITimeline::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUITimeline >* function = sdUITimeline::FindFunction( name );
	if( !function ) {		
		return manager->GetScope().GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUITimeline, sdUITemplateFunctionInstance_IdentifierTimeline >( this, function );
}

/*
============
sdUITimeline::FindFunction
============
*/
const sdUITemplateFunction< sdUITimeline >* sdUITimeline::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUITimeline >** ptr;
	return timelineFunctions.Get( name, &ptr ) ? *ptr : NULL;
}


/*
============
sdUITimeline::InitFunctions
============
*/
void sdUITimeline::InitFunctions() {
	timelineFunctions.Set( "resetTime",	new sdUITemplateFunction< sdUITimeline >( 'v', "f",	&sdUITimeline::Script_ResetTime ) );	
}

/*
============
sdUITimeline::OnActiveChanged
============
*/
void sdUITimeline::OnActiveChanged( const float oldValue, const float newValue ) {
	if( newValue == 0.0f ) {
		Clear();
	} else {
		DeferredReset();
	}
}

/*
============
sdUITimeline::Reset
============
*/
void sdUITimeline::Reset() {
	PushEvents();
}

/*
============
sdUITimeline::DeferredReset
============
*/
void sdUITimeline::DeferredReset() {
	manager->DeferredResetTimeline( this );
}

/*
============
sdUITimeline::CreateEvents
============
*/
bool sdUITimeline::CreateEvents( const sdDeclGUITimeline& timelineInfo, const sdDeclGUI* guiDecl, idTokenCache& tokenCache ) {
	const idList< sdDeclGUITimeEvent* > events = timelineInfo.GetEvents();
	const idList< sdDeclGUIProperty* > properties = timelineInfo.GetProperties();

	timeline.AssureSize( events.Num() );

	idList<unsigned short> constructorTokens;
	bool hasValues = sdDeclGUI::CreateConstructor( properties, constructorTokens, tokenCache );

	if ( hasValues ) {
		idLexer parser( sdDeclGUI::LEXER_FLAGS );
		parser.LoadTokenStream( constructorTokens, tokenCache, "sdUITimeline::CreateEvents" );

		sdUIEventInfo constructionEvent( TE_CONSTRUCTOR, 0 );
		script.ParseEvent( &parser, constructionEvent, this );
		script.RunEventHandle( GetEvent( constructionEvent ), this );
	}

	for( int eventIndex = 0; eventIndex < events.Num(); eventIndex++ ) {
		const sdDeclGUITimeEvent* eventInfo = events[ eventIndex ];

		sdUserInterfaceLocal::PushTrace( eventInfo->GetTextInfo() );

		idLexer parser( sdDeclGUI::LEXER_FLAGS );
		parser.LoadTokenStream( eventInfo->GetTokenIndices(), tokenCache, va( "onTime '%i'", eventInfo->GetStartTime() ) );
		//parser.LoadMemory( eventInfo->GetText(), eventInfo->GetTextLength(), va( "onTime '%i'", eventInfo->GetStartTime()) );

		script.ParseEvent( &parser, sdUIEventInfo( TE_ONTIME, eventIndex ), this );					

		timeline[ eventIndex ] = eventInfo->GetStartTime();

		sdUserInterfaceLocal::PopTrace();
	}
	return true;
}


/*
============
sdUITimeline::CreateProperties
============
*/
bool sdUITimeline::CreateProperties( const sdDeclGUITimeline& declProperties, const sdDeclGUI* guiDecl, const idTokenCache& tokenCache ) {
	sdUIWindow::SetupProperties( properties, declProperties.GetProperties(), manager->GetUI(), tokenCache );
	return true;
}

/*
============
sdUITimeline::Script_ResetTime
============
*/
void sdUITimeline::Script_ResetTime( sdUIFunctionStack& stack ) {
	stack.Pop( resetTime );
	DeferredReset();
}


/*
============
sdUITimeline::OnSnapshotHitch
============
*/
void sdUITimeline::OnSnapshotHitch( int delta ) {
	for( int i = 0; i < timeline.Num(); i++ ) {
		timeline[ i ] -= delta;
	}
}

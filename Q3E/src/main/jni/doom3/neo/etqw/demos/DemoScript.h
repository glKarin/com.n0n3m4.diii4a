// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DEMOS_DEMOSCRIPT_H__
#define __GAME_DEMOS_DEMOSCRIPT_H__

#include "DemoCamera.h"

class sdDemoScript {
public:
	class sdEvent {
	public:
		virtual				~sdEvent() {}

		virtual bool		Run( sdDemoScript& script ) const = 0;
		virtual bool		Parse( idParser& src ) = 0;
	};
	
									sdDemoScript() {}
									~sdDemoScript() {
										cameras.DeleteContents();
										timedEvents.DeleteContents( true );
									}

	static void						InitClass();
	static void						Shutdown();
	static sdEvent*					CreateEvent( const char* type ) { return eventFactory.CreateType( type ); }

	sdDemoCamera*					GetCamera( const char* name ) {
										sdDemoCamera** camera = NULL;
										if ( cameras.Get( name, &camera ) ) {
											return *camera;
										} else {
											return NULL;
										}
									}

	bool							Parse( const char* fileName );

	void							Reset() {
										lastEvent = -1;
									}
	void							RunFrame();

private:
	class sdOnTime {
	public:
							sdOnTime() :
								time( -1 ) {
							}
							~sdOnTime() {
								events.DeleteContents( true );
							}

		bool				Parse( idParser& src );

		int					GetTime() const { return time; }

		bool				RunEvents( sdDemoScript& script ) const {
								for ( int i = 0; i < events.Num(); i++ ) {
									if ( !events[ i ]->Run( script ) ) {
										return false;
									}
								}
								return true;
							}

	private:
		idList< sdEvent* >	events;
		int					time;
	};

	typedef sdFactory< sdEvent >	sdEventFactory;
	typedef sdOnTime*				sdOnTimePtr;

	bool							ParseTimeLine( idParser& src );

	static int						SortByTime( const sdOnTimePtr* a, const sdOnTimePtr* b ) { return ( *b )->GetTime() - ( *a )->GetTime(); }

private:
	idStr							fileName;

	static sdEventFactory			eventFactory;

	idHashMap< sdDemoCamera* >		cameras;
	idList< sdOnTime* >				timedEvents;

	int								lastEvent;
};

#endif // __GAME_DEMOS_DEMOSCRIPT_H__

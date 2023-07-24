// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_CROSSHAIRINFO_H__
#define __GAME_CROSSHAIRINFO_H__

#include "../cm/CollisionModel.h"

class idEntity;

typedef enum chInfoLineType_e {
	CI_BAR,
	CI_TEXT,
	CI_IMAGE,
} chInfoLineType_t;

typedef struct chInfoLine_s {
						chInfoLine_s() :
							material( NULL ) {
						}

	idWStr				text;
	const idMaterial*	material;

	idVec4				foreColor;

	idVec2				xy;
	float				frac;

	chInfoLineType_t	type;
} chInfoLine_t;

class sdCrosshairInfo {
public:
							sdCrosshairInfo( void );

	bool					IsValid( void ) const;
	bool					IsUseValid( void ) const;

	void					Validate( void );
	void					Invalidate( void );

	void					SetNumLines( int count ) { lines.SetNum( count, false ); }
	int						GetNumLines( void ) const { return lines.Num(); }

	chInfoLine_t&			GetLine( int index ) { return lines[ index ]; }
	const chInfoLine_t&		GetLine( int index ) const { return lines[ index ]; }

	float					GetAlpha( void ) const;

	void					SetTrace( const trace_t& _trace ) {memcpy(&trace,&_trace,sizeof(_trace));}
	const trace_t&			GetTrace() const { return trace; }

	const idVec3&			GetContactPoint() { return trace.endpos; }

	void					SetEntity( idEntity* entity );
	idEntity*				GetEntity( void ) const;

	void					SetStartTime( int _time ) { startTime = _time; }
	int						GetStartTime( void ) const { return startTime; }

	void					SetDistance( float _distance ) { distance = _distance; }
	float					GetDistance( void ) const { return distance; }

private:
	int						startTime;
	int						useTime;
	int						time;
	trace_t					trace;
	float					distance;

	idList< chInfoLine_t >	lines;
	idEntityPtr< idEntity >	owner;
};

#endif // __GAME_CROSSHAIRINFO_H__


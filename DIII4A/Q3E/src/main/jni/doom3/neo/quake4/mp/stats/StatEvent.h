//----------------------------------------------------------------
// StatEvent.h
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#ifndef __STATEVENT_H__
#define __STATEVENT_H__

/*
===============================================================================

Multiplayer statistics events

This file contains statistic event definitions.  rvStatManager exposes 
game-related functions (i.e. 'Kill', 'FlagCaptured') which create the 
appropriate statistic event and add it to the statQueue in rvStatManager.

The statQueue contains a complete picture of an MP game.  It can be parsed to
calculate accuracies, award end-game awards, etc.

Statistic events also have functionality to register in-game information for
on-the-fly statistics.

===============================================================================
*/

class rvPlayerStat;

/*
================
statType_t

Type identifiers for RTTI of stat events, rvStatTeam-derived events must go 
after ST_STAT_TEAM
================
*/
enum statType_t {
	// not an actual event, undefined marker
	ST_NONE = 0,
	// rvStat derived
	ST_BEGIN_GAME,
	ST_END_GAME,
	ST_CLIENT_CONNECT,
	ST_HIT,
	ST_KILL,
	ST_DEATH,
	ST_DAMAGE_DEALT,
	ST_DAMAGE_TAKEN,
	// not an actual event, team marker
	ST_STAT_TEAM,
	// rvStatTeam derived
	ST_CTF_FLAG_CAPTURE,
	ST_CTF_FLAG_DROP,
	ST_CTF_FLAG_RETURN,

	ST_COUNT
};

/*
================
rvStat

An individual event we want to know about for stats.
================
*/
class rvStat {

	friend class rvStatAllocator;

public:

	statType_t					GetType( void ) const { return type; };
	int							GetTimeStamp( void ) const { return timeStamp; };
	byte						GetPlayerClientNum( void ) const { return playerClientNum; };

protected:

	// moved to protected to prevent allocating these on the normal heap.
	// these should only be allocatd by rvStatAllocator.
	rvStat( int t ) { timeStamp = t; playerClientNum = -1; type = ST_NONE; };
	virtual						~rvStat( void ) {};

	virtual void				RegisterInGame( rvPlayerStat* stats ) {};

	// the entity number of the player associated with this statistic
	byte						playerClientNum;

	statType_t					type;
	int							timeStamp;

private:
	// because of the way memory is handled for these we want the compiler
	// on our side to find abuses.  note that we aren't defining these.
	// this change is propagated to all derived classes.
	rvStat();
	rvStat( const rvStat &rhs );
	const rvStat &operator=( const rvStat &rhs );
};

/*
================
rvStatTeam

A team-related rvStat
================
*/
class rvStatTeam : public rvStat {

	friend class rvStatAllocator;

public:
	virtual						~rvStatTeam( void ) {};

	int							GetTeam( void ) { return team; };
protected:

	// see comment in rvStat
	rvStatTeam( int t, int tm ) : rvStat( t ) { team = tm; };

	byte						team;

private:
	// see comment in rvStat
	rvStatTeam();
	rvStatTeam( const rvStatTeam &rhs );
	const rvStatTeam &operator=( const rvStat &rhs );

};

/*
===============================================================================

rvStat/rvStatTeam-derived classes

These are the individual events that get stored in the 
statManager's stat queue

===============================================================================
*/

/*
================
rvStatBeginGame

A game has begun
================
*/
class rvStatBeginGame : public rvStat {

	friend class rvStatAllocator;

protected:
	rvStatBeginGame( int t ) : rvStat( t ) { type = ST_BEGIN_GAME; };
};

/*
================
rvStatEndGame

The current game has ended
================
*/
class rvStatEndGame : public rvStat {

	friend class rvStatAllocator;

protected:
	rvStatEndGame( int t ) : rvStat( t ) { type = ST_END_GAME; };
};

/*
================
rvStatClientConnect

A player has connected
================
*/
class rvStatClientConnect : public rvStat {

	friend class rvStatAllocator;

protected:
	rvStatClientConnect( int t, int p ) : rvStat( t ) { type = ST_CLIENT_CONNECT; playerClientNum = p; };
};

/*
================
rvStatHit

A player hit another player
================
*/
class rvStatHit : public rvStat {

	friend class rvStatAllocator;

public:
	int GetVictimClientNum() const { return victimClientNum; }
	int GetWeapon() const { return weapon; }

protected:
	rvStatHit( int t, int p, int v, int w, bool countForAccuracy );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte		weapon;
	byte		victimClientNum;
	bool		trackAccuracy;
};

/*
================
rvStatKill

A player killed another player
================
*/
class rvStatKill : public rvStat {

	friend class rvStatAllocator;

protected:
	rvStatKill( int t, int p, int v, bool g, int mod );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte						methodOfDeath;
	byte						victimClientNum;
	bool						gibbed;
};

/*
================
rvStatDeath

A player died
================
*/
class rvStatDeath : public rvStat {

	friend class rvStatAllocator;

protected:
	rvStatDeath( int t, int p, int mod );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte						methodOfDeath;
};

/*
================
rvStatDamageDealt

A player damaged another player
================
*/
class rvStatDamageDealt : public rvStat {

	friend class rvStatAllocator;

public:
	int GetDamage() const { return damage; }

protected:
	rvStatDamageDealt( int t, int p, int w, int d );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte		weapon;
	short		damage;
};

/*
================
rvStatDamageTaken

A player took damage from another player
================
*/
class rvStatDamageTaken : public rvStat {

	friend class rvStatAllocator;

public:
	int GetDamage() const { return damage; }

protected:
	rvStatDamageTaken( int t, int p, int w, int d );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte		weapon;
	short		damage;
};

/*
================
rvStatFlagDrop

A player dropped the flag (CTF)
================
*/
class rvStatFlagDrop : public rvStatTeam {

	friend class rvStatAllocator;

protected:
	rvStatFlagDrop( int t, int p, int a, int tm );

	byte						attacker; // enemy who caused the flag drop

private:
	// see comment in rvStat
	rvStatFlagDrop();
	rvStatFlagDrop( const rvStatFlagDrop &rhs );
	const rvStatFlagDrop &operator=( const rvStatFlagDrop &rhs );
};

/*
================
rvStatFlagReturn

A player returned his teams flag
================
*/
class rvStatFlagReturn : public rvStatTeam {

	friend class rvStatAllocator;

protected:
	rvStatFlagReturn( int t, int p, int tm );

private:
	// see comment in rvStat
	rvStatFlagReturn();
	rvStatFlagReturn( const rvStatFlagReturn &rhs );
	const rvStatFlagReturn &operator=( const rvStatFlagReturn &rhs );
};

/*
================
rvStatFlagCapture

A player captured a flag (CTF)
================
*/
class rvStatFlagCapture : public rvStatTeam {

	friend class rvStatAllocator;

protected:
	rvStatFlagCapture( int t, int p, int f, int tm );
	virtual void RegisterInGame( rvPlayerStat* stats );

	byte					flagTeam; // team of flag was captured

private:
	// see comment in rvStat
	rvStatFlagCapture();
	rvStatFlagCapture( const rvStatFlagCapture &rhs );
	const rvStatFlagCapture &operator=( const rvStatFlagCapture &rhs );
};

#endif

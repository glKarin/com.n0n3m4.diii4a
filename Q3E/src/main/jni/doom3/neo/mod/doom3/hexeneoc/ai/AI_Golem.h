#ifndef __AI_GOLEM_
#define __AI_GOLEM_

#pragma hdrstop

#include "Game_local.h"

const idVec3 GOLEM_TRACE_MINS	= idVec3(-1,-1,-1);
const idVec3 GOLEM_TRACE_MAXS	= idVec3(-1,-1,-1);

class idAI_Golem : public idAI {
public:
	CLASS_PROTOTYPE( idAI_Golem );
	void		Spawn();
	void		Think();
	void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, idVec3 &iPoint );
	void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) { idVec3 zero; Damage(inflictor, attacker, dir, damageDefName, damageScale, location, zero); }
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

public:
	const char *	GetGolemType( void ) const;

private:
    float		totRocks;	// this IS the total number of rocks.
	float		nextFindRocks;	// time delay for finding new rocks
	int			nextBoneI;
	idStr		curBone;
	idStr		golemType;
	idList<idMoveable *>	rocks;
	bool		alive;
	int			onEnt;

private:
	void		findNewRocks( void );  // look for new rocks
    void		addRock( idMoveable *newRock );
    void		evalRocks( void ); // call this every frame to do all rock related junk
    void		nextBone( void );
	void		BlowUp( void );
	void		Event_SawEnemy( void );
	idMoveable	*FindGolemRock( void );
};


#endif // __AI_GOLEM_

#ifndef __GAME_AFS_H__
#define __GAME_AFS_H__

extern const idEventDef EV_CursorDrop;
extern const idEventDef EV_CursorDone;

/***********************************************************************

hhAFEntity

***********************************************************************/

class hhAFEntity : public idAFEntity_Generic {

public:
	CLASS_PROTOTYPE( hhAFEntity );

	void				Spawn();
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		DormantBegin();
	virtual void		DormantEnd();

	void				Event_EnableRagdoll( void );
	void				Event_DisableRagdoll( void );
};


/***********************************************************************

hhAFEntity_WithAttachedHead

***********************************************************************/

class hhAFEntity_WithAttachedHead : public idAFEntity_WithAttachedHead {

public:
	CLASS_PROTOTYPE( hhAFEntity_WithAttachedHead );

	void				Spawn();
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	virtual void		DormantBegin();
	virtual void		DormantEnd();

	void				Event_EnableRagdoll();
	void				Event_DisableRagdoll();
};


#endif	// __GAME_AFS_H__

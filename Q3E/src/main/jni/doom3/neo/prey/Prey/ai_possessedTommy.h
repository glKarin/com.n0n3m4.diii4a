#ifndef __PREY_AI_POSSESSED_TOMMY_H__
#define __PREY_AI_POSSESSED_TOMMY_H__

/***********************************************************************
  hhPossessedTommy.
***********************************************************************/
class hhPossessedTommy : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE( hhPossessedTommy );	

public:
	void			Spawn();
	virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void	Think(void);

	void			SetPossessedProxy( hhSpiritProxy *newProxy ) { possessedProxy = newProxy; }

	virtual void	Event_Remove(void);   

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual	void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
protected:
	idEntityPtr<hhSpiritProxy> possessedProxy;
	int nextDrop;
};

#endif //__PREY_AI_POSSESSED_TOMMY_H__
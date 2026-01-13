#ifndef __PREY_AI_MUTILATEDHUMAN_H__
#define __PREY_AI_MUTILATEDHUMAN_H__

class hhMutilatedHuman : public hhMonsterAI {
public:
	CLASS_PROTOTYPE(hhMutilatedHuman);
	void			Event_AlertFriends();
	void			Event_DropBinds();
	void			Event_DropProjectiles();
	int				ReactionTo( const idEntity *ent );
	void			Spawn();
	void			AnimMove();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	void			LinkScriptVariables();
protected:
	idScriptBool	AI_LIMB_FELL;
	int				damageFlag;
};


#endif //__PREY_AI_MUTILATEDHUMAN_H__
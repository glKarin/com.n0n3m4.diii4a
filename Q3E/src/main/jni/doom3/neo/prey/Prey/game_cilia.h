
#ifndef __GAME_CILIA_H
#define __GAME_CILIA_H

extern const idEventDef EV_TriggerNearby;
extern const idEventDef EV_StickOut;

class hhSphereCilia : public hhSpherePart {
public:
	CLASS_PROTOTYPE( hhSphereCilia );

	void		Spawn( void );
	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );
	void		Trigger( idEntity *activator );
	void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

protected:
	void		Event_Trigger( idEntity *activator );
	void		Event_TriggerNearby( void );
	void		Event_StickOut( void );
	void		Event_Idle( void ); // JRM
	void		Event_Touch( idEntity *other, trace_t *trace );

	void		ApplyEffect( void );

protected:	
	int			pullInAnim;
	int			stickOutAnim;
	float		nearbySize;
	float		idleDelay;	
	bool		bRetracted;
	bool		bAlreadyActivated;
};

#endif /* __GAME_CILIA_H */

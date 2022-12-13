
#ifndef __GAME_SPHEREPART_H__
#define __GAME_SPHEREPART_H__

extern const idEventDef EV_Pulse;
extern const idEventDef EV_PlayIdle;

class hhSpherePart : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE(hhSpherePart);

	virtual			~hhSpherePart();

	void			Spawn(void);
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

protected:
	void			Event_Pulse(void);
	void			Event_Trigger( idEntity *activator );
	void			Event_PlayIdle();

protected:
	float			pulseTime; // Time between pulses.  Slight randomness is added
	float			pulseRandom;

	int				idleAnim;
	int				painAnim;
	int				pulseAnim;
	int				triggerAnim;
};

/**********************************************************************

hhGenericAnimatedPart

**********************************************************************/
class hhGenericAnimatedPart: public hhAnimatedEntity {
	CLASS_PROTOTYPE( hhGenericAnimatedPart );

public:
	void				Spawn();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - networking
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void		ClientPredictionThink( void );

	virtual void		Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location );

	virtual int			PlayAnim( const char* animName, int channel ); 
	virtual void		CycleAnim( const char* animName, int channel );
	virtual void		ClearAllAnims();

	void				SetOwner( idEntity* pOwner );

protected:
	virtual void		LinkCombatModel( idEntity* self, const int renderModelHandle );

protected:
	void				Event_PostSpawn();

protected:
	idEntityPtr<idEntity> owner;
};

#endif /* __GAME_SPHEREPART_H__ */

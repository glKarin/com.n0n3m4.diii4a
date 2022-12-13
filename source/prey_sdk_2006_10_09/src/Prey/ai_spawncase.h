#ifndef ai_spawncase_H
#define ai_spawncase_H

//
// hhAISpawnCase
//
// Spawns a specific monster, attaches to model - waits for trigger before releasing monster
//
class hhAISpawnCase : public hhAnimated {
	
public:	
	CLASS_PROTOTYPE(hhAISpawnCase);

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	virtual void		Event_Trigger( idEntity *activator );
	virtual void		Event_AnimDone( int animIndex );

protected:
	void				Event_CreateAI( void );
	void				Event_AutoTrigger( void );

	void				CreateEntSpawnArgs( void );

	idEntityPtr<idAI>	encasedAI;				// The AI that is currently encased and waiting to come out
	bool				triggerToggle;			// Toggles back and forth to indicated which anim to play (open or close)
	int					aiSpawnCount;			// Current number of monsters spawned	
	bool				waitingForAutoTrigger;	// True if we are waiting for an auto-trigger msg to come in (from auto_retrigger_delay setting)
	int					triggerQueue;			// Number of triggers that have occurred during the delay period that must be re-sent
	idDict				entSpawnArgs;			// Spawn args to copy to spawned entities
};

#endif
#ifndef __PREY_AI_INSPECTOR_H__
#define __PREY_AI_INSPECTOR_H__

class hhAIInspector : public hhMonsterAI {

public:	
	CLASS_PROTOTYPE(hhAIInspector);	

public:
	void			Spawn();
	void			CheckReactions( idEntity* entity );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	static void			RestartInspector( const char* inspectClass, idEntity* newReaction, idPlayer* starter );
	static void			RestartInspector( idEntity* newReaction, idPlayer* starter );

public:
// Events.
	void			Event_PostSpawn();
	void			Event_CheckReactions( idEntity* entity );

protected:
	idEntityPtr<idEntity>					checkReaction;

};

#endif 

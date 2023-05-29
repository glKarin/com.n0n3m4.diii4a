#ifndef __PREY_AI_PASSAGENODE_H__
#define __PREY_AI_PASSAGENODE_H__



//
// hhAIPassageway
//
class hhAIPassageway : public hhAnimated
{
public:		
	CLASS_PROTOTYPE(hhAIPassageway);

#ifdef ID_DEMO_BUILD //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
	virtual void		PostSpawn() {};
	void				SetEnablePassageway(bool tf) {};
	void				Event_AnimDone( int animIndex ) {};
	void				Event_Trigger( idEntity *activator ) {};
	void				Event_EnablePassageway(void)	{}
	void				Event_DisablePassageway(void)	{}
#else

	hhAIPassageway()	{timeLastEntered = 0; lastEntered = NULL;}
	virtual void		Spawn();
	virtual void		PostSpawn();

	void				Save(idSaveGame *savefile) const;
	void				Restore(idRestoreGame *savefile);

	idVec3				GetExitPos(void);				// The point that a monster should be spawned upon exiting this passageway
	
	void				SetEnablePassageway(bool tf);
	bool				IsPassagewayEnabled(void)	const	{return enabled;}
	
	int					timeLastEntered;				// The time someone last entered this passage node
	idEntityPtr<idAI>	lastEntered;					// The ai that last entered this passage node
	bool				hasEntered;						// FALSE if the AI has not fully entered yet - waiting for one more trigger

protected:
	void				Event_AnimDone( int animIndex );
	void				Event_Trigger( idEntity *activator );
	void				Event_EnablePassageway(void)	{SetEnablePassageway(TRUE);}
	void				Event_DisablePassageway(void)	{SetEnablePassageway(FALSE);}

	bool				enabled;						// TRUE if we are a valid passagway
#endif //HUMANHEAD jsh PCF 5/26/06: code removed for demo build
};

#endif

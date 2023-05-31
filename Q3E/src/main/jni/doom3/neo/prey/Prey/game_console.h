
#ifndef __GAME_CONSOLE_H__
#define __GAME_CONSOLE_H__

extern const idEventDef EV_CallGuiEvent;

enum ETranslationState {
	TS_TRANSLATING,
	TS_TRANSLATED,
	TS_UNTRANSLATING,
	TS_UNTRANSLATED
};

class hhTalonTarget; // CJR

class hhConsole : public idStaticEntity {
public:
	CLASS_PROTOTYPE( hhConsole );

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			Think();
	virtual void			ConsoleActivated();
	virtual void			ClearTalonTargetType(); // CJR - Called when any command is sent to a gui.  This disables talon's squawking

	virtual void			Present( void );
	virtual bool			HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);
	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	void					Translate(bool bLanded);
	void					SetOnAllGuis(const char *key, float value);
	void					SetOnAllGuis(const char *key, int value);
	void					SetOnAllGuis(const char *key, bool value);
	void					SetOnAllGuis(const char *key, const char *value);
	virtual void			PlayerControls(usercmd_t *cmd) {}
	
	virtual void			Use(idAI *ai);			// JRM - An AI is starting to use this console
	bool					CanUse(idAI *ai);		// JRM - Returns TRUE if the given AI is allowed to use this console right now
	int						GetLastUsedTime(void)	{return aiLastUsedTime;}
	idAI*					GetLastUsedAI(void)		{return aiLastUsedBy.GetEntity();}
	virtual void			OnTriggeredByAI(idAI *ai); // JRM - Called when the AI actually "presses" the console
	void					UpdateUse(void);		// Called every tick

protected:
	void					Event_Activate(idEntity *activator);
	void					Event_TalonAction(idEntity *talon, bool landed);
	void					Event_CallGuiEvent(const char *eventName);
	void					Event_BecomeNonSolid( void );
	void					Event_PostSpawn();

protected:
	idInterpolate<float>	translationAlpha;
	ETranslationState		transState;
	bool					bTimeEventsAlways;
	bool					bUsesRand;

	hhTalonTarget			*perchSpot;				// Associated Talon perch spot with this console

	// JRM - AI data
	bool					aiCanUse;				// True if the AI is allowed to use this console
	int						aiUseCount;				// Number of times AI has used this console	
	int						aiMaxUses;				// Max number of times the ai can use this console
	int						aiReuseWaitTime;		// Time the AI has to wait before they can use this console again (ms)
	int						aiUseTime;				// How long once AI starts using this console, do they stay here? (total ms)
	int						aiTriggerWaitTime;		// How long once AI starts using this console until we fire this console's trigger? (ms)
	bool					aiWaitingToTrigger;		// TRUE if the ai is waiting to fire the triggers for this console
	int						aiLastTriggerTime;		// The time we last triggered this console
	int						aiRetriggerWait;		// -1 = Never retrigger, otherwise time (ms) that we should retrigger while using
	
	idEntityPtr<idAI>		aiCurrUsedBy;			// The AI is currently being using this console
	int						aiCurrUsedStartTime;	// The time the current user STARTED using this console
	
	idEntityPtr<idAI>		aiLastUsedBy;			// The AI that last used this console
	int						aiLastUsedTime;			// The time that aiLastUsedBy used finished using this console
	
};

class hhConsoleCountdown : public hhConsole {
	CLASS_PROTOTYPE( hhConsoleCountdown );

public:
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think();

	void					Reset();
	void					UpdateGUI(float curValue);
	void					SetGuiOctal(int value);
	void					SetCountdown(int count);
	void					StartCountdown();
	void					StopCountdown();

protected:
	void					Event_Activate(idEntity *activator);
	void					Event_SetCountdown(int count);
	void					Event_StartCountdown();
	void					Event_StopCountdown();

protected:
	bool					countingDown;			// Whether we are running or not
	float					countStart;				// Starting value of countdown
	float					countEnd;				// Ending value of countdown
	idInterpolate<float>	countdown;				// Interpolator that always carries the current count
};


class hhConsoleKeypad : public hhConsole {
	CLASS_PROTOTYPE( hhConsoleKeypad );

public:

	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	bool					HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);

protected:
	void					UpdateGui();

protected:
	idStr					keyBuffer;
	int						maxLength;
};


class hhConsoleAlarm : public hhConsole {
	CLASS_PROTOTYPE( hhConsoleAlarm );

public:
	void					Spawn();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	virtual void			ConsoleActivated();
	virtual void			Ticker();
protected:
    void					Event_SpawnMonster();
	idEntityPtr<idAI>		currentMonster;
	int						maxMonsters;
	int						numMonsters;
	bool					bSpawning;
private:
	bool					bAlarmActive;
};


#endif /* __GAME_CONSOLE_H__ */

/*
===============================================================================

  rvHealingStation

===============================================================================
*/
class rvHealingStation : public idAnimatedEntity {
public:

	CLASS_PROTOTYPE( rvHealingStation );

	virtual void			Think						( void );

	void					Spawn						( void );
	void					Save						( idSaveGame *savefile ) const;
	void					Restore						( idRestoreGame *savefile );
	
	void					BeginHealing				( idEntity *toHeal );
	void					EndHealing					( void );

protected:

	void					CreateFrame					( float station_health );

	stateResult_t			State_Healing				( const stateParms_t& parms );

	rvStateThread			stateThread;
	idEntityPtr<idEntity>	entityToHeal;
	int						nextHealTime;
	int						healFrequency;
	int						healAmount;
	int						healthDispensed;
	int						maxHealth;
	int						dispenseAnim;
	int						soundStartTime;
	int						soundLength;

private:

	bool					IsPlaying					( void );

	CLASS_STATES_PROTOTYPE ( rvHealingStation );
};


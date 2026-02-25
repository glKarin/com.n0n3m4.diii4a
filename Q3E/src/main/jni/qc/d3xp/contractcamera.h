
class idContractCamera : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE( idContractCamera );

	void					Spawn( void );
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );	

	void					OnFrob( idEntity* activator );

private:

	enum					{ OFF, UNFOLDING, PHOTOFLIPPING, PHOTOSNAPPING, PHOTOSNAPDONE, PHOTODONE, TAKEN };
	int						state;

	int						nextStateTime;
	int						maxcount;
	int						count;

	idEntity*				frobcube;
	idLight *				light;

	void					UpdateStates( void );

	void					Event_reset( void );
};


class idItemGate : public idStaticEntity
{
public:
							CLASS_PROTOTYPE( idItemGate );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );
	void					Spawn( void );

	virtual void			Think( void );

private:
	int						nextTriggerTime;
	idVec3					throwDir;
	idVec3					throwDestination;
	idStr					className;
	idStr					weaponName;
	idStr					callName;
	int						force;

	bool					hasFoundTarget;
	bool					isOn;

	void					Event_staticactivate( int value );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					repel(idEntity *ent);

	int						nextHitSoundTime;
};

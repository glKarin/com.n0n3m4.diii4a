#ifndef __RV_TRAM_GATE_H
#define __RV_TRAM_GATE_H

extern const idEventDef EV_OpenGate;
extern const idEventDef EV_CloseGate;

class rvTramGate : public idAnimatedEntity {
	CLASS_PROTOTYPE( rvTramGate );

public:
	void				Spawn();
	virtual				~rvTramGate();

	void				OpenGate();
	void				CloseGate();

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

protected:
	void				SpawnDoors();
	void				AdjustFrameRate();

	int					PlayAnim( int channel, const char* animName, int blendFrames = 0 );
	void				CycleAnim( int channel, const char* animName, int blendFrames = 0 );
	void				ClearAllAnims( int blendFrames = 0 );
	void				ClearAnim( int channel, int blendFrames = 0 );
	bool				AnimIsPlaying( int channel, int blendFrames = 0 );

	bool				IsOpen() const;
	bool				IsClosed() const;

	idDoor*				GetDoorMaster() const;
	bool				IsDoorMasterValid() const;
	
protected:
	void				Event_Touch( idEntity* other, trace_t* trace );
	void				Event_Activate( idEntity* activator );

	void				Event_OpenGate();
	void				Event_CloseGate();

	void				Event_IsOpen( void );
	void				Event_IsLocked( void );
	void				Event_Lock( int f );

protected:
	idList< idEntityPtr<idDoor> > doorList;
};

#endif

#ifndef __PREY_PROXDOOR_H__
#define __PREY_PROXDOOR_H__

class hhProxDoorSection;
typedef idEntityPtr<hhProxDoorSection> doorSectionPtr_t; //rww - so that the list of door sections can be safe.

/***********************************************************************
  hhProxDoor.
	The main door control.  Has a model associated with it (the frame).
***********************************************************************/
class hhProxDoor : public idEntity {
	CLASS_PROTOTYPE( hhProxDoor );

	public:
							hhProxDoor();
		virtual				~hhProxDoor();

		void				Spawn( void );
		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );
		void				SpawnSoundTrigger();

		//rww - netcode
		virtual void		ClientPredictionThink( void );
		virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

		virtual void		Ticker( void );
	
		//Events.
		void				Event_Touch( idEntity *other, trace_t *trace  );
		void				Event_PollForExit( void );
		virtual void		Event_PostSpawn( void );
		void				Event_Activate( idEntity* activator );

		//Portal Control.
		void				OpenPortal( void );
		void				ClosePortal( void );

		//AAS Control.
		void				SetAASAreaState( bool closed );

		//Lock Control
		bool				IsLocked( void );
		void				Lock( int f );

		bool				hasNetData; //rww - net stuff

protected:
	enum EProxState {
		PROXSTATE_Active,
		PROXSTATE_Inactive,
		PROXSTATE_GoingInactive,
	};

	enum EPDoorSound {
		PDOORSND_Opening,
		PDOORSND_Closing,
		PDOORSND_Closed,
		PDOORSND_Opened,
		PDOORSND_FullyOpened,
		PDOORSND_Stopped,
	};

	void SetDoorState( EProxState doorState );
	void UpdateSoundState( EPDoorSound newState );
	float PollClosestEntity();
	void CrushEntities();

protected:
	EProxState							proxState;			//our state
	EPDoorSound							doorSndState;		//state of our door sound
	float								lastAmount;			//last amount [mainly used for sound]
	idList<doorSectionPtr_t>			doorPieces;			//all of the pieces that make up the door
	idClipModel*						doorTrigger;		//trigger that wakes us up
	idClipModel*						sndTrigger;			//trigger for playing locked sound
	int									nextSndTriggerTime;	// next time to play door locked sound
	float								entDistanceSq;		//squared distance of the closest entity
	float								maxDistance;		//distance of trigger
	float								movementDistance;	//distance to start movement at
	float								lastDistance;		//previous distance
	float								stopDistance;		//distance to stop movement at
	float								damage;				// Amount of crush damage we do
	qhandle_t							areaPortal;			// 0 = no portal, > 0 = our portal we are by
	bool								doorLocked;
	bool								openForMonsters;
	bool								aas_area_closed;	

};

/***********************************************************************
  hhProxDoorSection.
	Base class for all of the pieces used to make up a hhProxDoor.
***********************************************************************/
class hhProxDoorSection : public idEntity {
	ABSTRACT_PROTOTYPE( hhProxDoorSection );

public:
	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const	{ savefile->WriteInt(sectionType); savefile->WriteFloat( proximity ); }
	void				Restore( idRestoreGame *savefile )	{ savefile->ReadInt((int&)sectionType); savefile->ReadFloat( proximity ); }

	//rww - netcode
	virtual void		ClientPredictionThink( void );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void		SetProximity( float prox ) = 0;

	idEntityPtr<idEntity>	proxyParent; //rww - for horrible network hacks

	bool				hasNetData; //rww - net stuff

	float				proximity;
protected:
	enum EProxSection {
		PROXSECTION_None,
		PROXSECTION_Rotator,
		PROXSECTION_Translator,
		PROXSECTION_RotatorMaster,
	};

	EProxSection		sectionType;
};

/***********************************************************************
  hhProxDoorRotator.
	A rotating door piece.  Uses an offset DoorRotMaster to bind to.
***********************************************************************/
class hhProxDoorRotator : public hhProxDoorSection {
	CLASS_PROTOTYPE( hhProxDoorRotator );
	
public:
	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - netcode
	virtual void		ClientPredictionThink( void );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void		SetProximity( float prox );

	virtual void		Event_PostSpawn( void );

protected:
	idEntityPtr<hhProxDoorSection>		bindParent;
};

/***********************************************************************
  hhProxDoorTranslator.
	A translating door piece.
***********************************************************************/
class hhProxDoorTranslator : public hhProxDoorSection {
	CLASS_PROTOTYPE( hhProxDoorTranslator );

public:
	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - netcode
	virtual void		ClientPredictionThink( void );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void		SetProximity( float prox );

protected:
	idVec3					baseOrigin;
	idVec3					targetOrigin;


};

/***********************************************************************
  hhProxDoorRotMaster
	The master that DoorRotators bind to.
***********************************************************************/
class hhProxDoorRotMaster : public hhProxDoorSection {
	CLASS_PROTOTYPE( hhProxDoorRotMaster );

public:
	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - netcode
	virtual void		ClientPredictionThink( void );
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual void		SetProximity( float prox );

protected:
	idVec3					rotVector;
	float					rotAngle;
	idMat3					baseAxis;
};


#endif //__PREY_PROXDOOR_H__
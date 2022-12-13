
#ifndef __PREY_GAME_HAND_H__
#define __PREY_GAME_HAND_H__

// Forward declar
class hhPlayer;

extern const idEventDef EV_Hand_DoRemove;
extern const idEventDef EV_Hand_DoAttach;
extern const idEventDef EV_Hand_Remove;
extern const idEventDef EV_Hand_Ready;
extern const idEventDef EV_Hand_Lowered;
extern const idEventDef EV_Hand_Raise;

typedef enum {
	HS_UNKNOWN,
	HS_READY,
	HS_LOWERING,
	HS_LOWERED,
	HS_RAISING
} handStatus_t;


class hhHand : public hhAnimatedEntity {

public: 

	CLASS_PROTOTYPE(hhHand);

   	virtual				~hhHand();

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void		ClientPredictionThink( void );

	enum {
		EVENT_REMOVEHAND = hhAnimatedEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};
	virtual bool		ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	static hhHand *		AddHand( hhPlayer *player, const char *classname, bool attachNow = false );

	static void			PrintHandInfo( idPlayer *player );

	virtual void		SetModel( const char *modelname );

	virtual void		Present();

	virtual void		Action( void ) { };
	virtual void		SetAction( const char* str ) {};	//HUMANHEAD bjk

	int					GetPriority( void ) { return( priority ); };

	int 				GetStatus( ) { return( status ); };

	void				Reraise( );

	bool				IsReady() { return( status == HS_READY ); }
	bool				IsLowering() { return( status == HS_LOWERING || status == HS_LOWERED ); }
	bool				IsLowered() { return( status == HS_LOWERED ); }
	bool				IsRaising() { return( status == HS_RAISING ); }
	
	virtual void		Raise();
	virtual void		PutAway();
	virtual void		Ready();
	
	void				Event_Ready();
	void				Event_Lowered();
	void				Event_Raise();
	

	void				PlayAnim( int channel, const char *animName, const idEventDef *animEvent = NULL );
	void				CycleAnim( int channel, const char *animname, int blendTime );

	bool				AttachHand( hhPlayer *player, bool attachNow = false );
	bool				RemoveHand( void );

	bool				IsAttached() { return attached; }

	int 				LowerHandOrWeapon( hhPlayer *player );
	void 				RaiseHandOrWeapon( hhPlayer *player );
	int					HandleWeapon( hhPlayer *player, hhHand *topHand, int weaponDelay = 0, bool doNow = false );

	// Replace the player hand with this hand.  Does a quick swap
	void				ReplaceHand( hhPlayer *player );
	
	int					GetAttachTime( ) { return( attachTime ); };

	// Functions that could be put in common base class
	int					GetAnimDoneTime() { return( animDoneTime ); };

	void				SetOwner( idActor *owner );
	void 				GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	
	virtual bool		IsValidFor( hhPlayer *who ) { return( true ); }

	// These used to be protected.  Call only if you know what you are doing!
	hhHand *			GetPreviousHand( hhPlayer *player = NULL );

	void				Event_DoHandAttach( idEntity *player );

	void 				ForceRemove( void );

protected:
	void				Event_RemoveHand( void );

	// Event only methods.  Should NOT be called directly
	void				Event_DoHandRemove( void );

	bool				DoHandAttach( idEntity *player );
	bool				DoHandRemove( void );


protected:
	bool				CheckHandAttach( hhPlayer *player );
	bool				CheckHandRemove( hhPlayer *player );

protected:
	// Who we are associated with
	idEntityPtr<idActor>	owner;

	// What is the priority of this hand?
	int					priority;

	// Previous hand.  Will restore on closing down!
	hhHand				*previousHand;

	// Should we lower the weapon for this hand?
	bool				lowerWeapon;
	
	// If we don't lower the weapon, do we put it aside?
	bool				asideWeapon;

	// DEBUG - We should never delete w/out being attached
	bool				attached;

	// Should we replace the previous hand instead of putting it on the hand stack?
	bool				replacePrevious;

	// What time do we attach?
	int					attachTime;
	
	// What hands do we represent?  (1 = right, 2 = left, 3 = both)
	int 				handedness;

	// When will we be done animating?
	int					animDoneTime;

	// How many frames should we blend?
	int 				animBlendFrames;
	
	// Status of the hand;
	int 				status;

	// What state we would like to be
	int					idealState;
	
	// Last event posted with an anim
	const idEventDef	* animEvent;

	hhPhysics_StaticWeapon	physicsObj;
};

#endif /* __PREY_GAME_HAND_H__ */


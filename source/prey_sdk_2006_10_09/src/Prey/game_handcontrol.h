
#ifndef __PREY_GAME_HAND_CONTROL_H__
#define __PREY_GAME_HAND_CONTROL_H__

#define HAND_MATRIX_WIDTH	3
#define HAND_MATRIX_HEIGHT	3

class hhControlHand : public hhHand {
public: 
	CLASS_PROTOTYPE(hhControlHand);

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void		ClientPredictionThink( void );

	void				UpdateControlDirection(idVec3 &dir);
	virtual void		Ready();
	virtual void		Raise();
	virtual void		PutAway();

protected:
	// Matrix of anims for up/down/normal & left/ceneter/right
	int					anims[ HAND_MATRIX_WIDTH ][ HAND_MATRIX_HEIGHT ];

	bool				bProcessControls;
	int					oldStatus;
};

#endif


#ifndef __PREY_GAME_GUIHAND_H__
#define __PREY_GAME_GUIHAND_H__

// Dumb forward decl
class hhPlayer;

class hhGuiHand : public hhHand {

public:

	CLASS_PROTOTYPE(hhGuiHand);

	void			Spawn();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void	ClientPredictionThink( void );

	virtual void	Action(void);			// Player clicked
	virtual void	SetAction(const char* str);	//HUMANHEAD bjk
	virtual bool	IsValidFor( hhPlayer *who );

protected:
	int				actionAnimDoneTime;
	const char*		action;
};


#endif /* __PREY_GAME_GUIHAND_H__ */


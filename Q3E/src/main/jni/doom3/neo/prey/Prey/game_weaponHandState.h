
#ifndef __PREY_GAME_WEAPONHANDSTATE_H__
#define __PREY_GAME_WEAPONHANDSTATE_H__

class hhWeaponHandState : public idClass {
	CLASS_PROTOTYPE( hhWeaponHandState );

 public:
	hhWeaponHandState( hhPlayer *player = NULL, 
					   bool trackWeapon = true, int weaponTransition = 0,
					   bool trackHand = true, int handTransistion = 0 );

	void		Save( idSaveGame *savefile ) const;
	void		Restore( idRestoreGame *savefile );

	//rww - network code
	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

	int			Archive( const char *newWeaponDef = NULL, int newWeaponTransition = 0,
					  const char *newHandDef = NULL, int newHandTransition = 0 );

	int			RestoreFromArchive();

	void		SetPlayer( hhPlayer *player );

	void		SetWeaponTransition( int trans ) { oldWeaponUp = trans; }

 protected:

	idEntityPtr<hhPlayer>	player;
	idEntityPtr<hhHand>		oldHand;		// Previous Hand.  NULL if don't care
	int						oldWeaponNum;
	int						oldHandUp;
	int						oldWeaponUp;

	bool					trackHand;
	bool					trackWeapon;
};

#endif /* __PREY_GAME_WEAPONHANDSTATE_H__ */

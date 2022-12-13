
#ifndef __GAME_SLOTS_H__
#define __GAME_SLOTS_H__

	#define SLOT_HEIGHT			100
	#define SLOTS_IN_REEL		10
	#define MINIMUM_REEL_RATE	(SLOT_HEIGHT/2)
	#define REEL_LENGTH			(SLOTS_IN_REEL*SLOT_HEIGHT)

	#define MASK_CHERRY		0x00000001
	#define MASK_ORANGE		0x00000002
	#define MASK_LEMON		0x00000004
	#define MASK_APPLE		0x00000008
	#define MASK_GRAPE		0x00000010
	#define MASK_MELON		0x00000020
	#define MASK_BAR		0x00000100
	#define MASK_BARBAR		0x00000200
	#define MASK_BARBARBAR	0x00000400

	#define MASK_ANYBAR		0x00000700
	#define MASK_ANY		0xffffffff

	typedef enum {
		FRUIT_CHERRY=0,
		FRUIT_ORANGE,
		FRUIT_LEMON,
		FRUIT_APPLE,
		FRUIT_GRAPE,
		FRUIT_MELON,
		FRUIT_BAR,
		FRUIT_BARBAR,
		FRUIT_BARBARBAR,
		NUM_FRUITS
	}fruit_t;

	typedef struct victory_s {
		int			f1, f2, f3;
		int			payoff;
	}victory_t;


class hhSlots : public hhConsole {
public:
	CLASS_PROTOTYPE( hhSlots );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			Spin();
	void			IncBet();
	void			DecBet();
	void			CheckVictory();

	void			Reset();
	void			UpdateView();
	bool			HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);	

protected:
	virtual void	Think( void );
	void			Event_Spin();

private:
	int				Bet;
	int				PlayerBet;
	int				PlayerCredits;
	int				result;
	int				creditsWon;
	int				victoryAmount;

	idStr			fruitTextures[NUM_FRUITS];

	fruit_t			reel1[SLOTS_IN_REEL];
	fruit_t			reel2[SLOTS_IN_REEL];
	fruit_t			reel3[SLOTS_IN_REEL];
	float			reelPos1, reelPos2, reelPos3;
	float			reelRate1, reelRate2, reelRate3;
	bool			bSpinning;

	bool			bCanSpin;
	bool			bCanIncBet;
	bool			bCanDecBet;
};


#endif /* __GAME_SLOTS_H__ */

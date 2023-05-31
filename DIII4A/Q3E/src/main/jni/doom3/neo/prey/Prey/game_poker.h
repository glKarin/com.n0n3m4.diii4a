
#ifndef __GAME_POKER_H__
#define __GAME_POKER_H__

extern const idEventDef EV_UpdateView;

#define PREQ_FLUSH		0x00000001
#define PREQ_STRAIGHT	0x00000002
#define PREQ_ROYALS		0x00000004
#define PREQ_4MATCH		0x00000008
#define PREQ_3MATCH		0x00000010
#define PREQ_2MATCH		0x00000020
#define PREQ_FULLHOUSE	0x00000040
#define PREQ_2PAIR		0x00000080

typedef enum pokerhand_e {
	HAND_ROYALFLUSH=0,
	HAND_STRAIGHTFLUSH,
	HAND_FOUROFKIND,
	HAND_FULLHOUSE,
	HAND_FLUSH,
	HAND_STRAIGHT,
	HAND_THREEOFKIND,
	HAND_TWOPAIR,
	HAND_PAIR,
	HAND_NOTHING,
	NUM_POKER_HANDS
}pokerhand_t;

typedef struct hand_s {
	pokerhand_t hand;
	int			payoff;
	int			requirement;
} hand_t;


class hhPokerHand : public idClass {
	CLASS_PROTOTYPE( hhPokerHand );

public:
	typedef struct valueHist_s {
		int	count;
	} valueHist_t;

	typedef struct suitHist_s {
		int	count;
	} suitHist_t;

			hhPokerHand();
	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );
	void	Clear();
	int		MaxCountOfValues();
	int		ValueOfMaxCount();
	int		GetLowValue();
	int		GetHighValue();
	bool	HasValue(int value);
	int		NumOfValue(int value);
	int		SuitOfMaxCount();
	int		NumUniqueValues();
	int		NumUniqueSuits();
	void	AddCard(hhCard card);
	bool	Search(hhCard &card);
	void	operator=(hhPokerHand &other);

public:
	suitHist_t	suits[NUM_SUITS];
	valueHist_t	values[NUM_CARD_VALUES];
	idList<hhCard>	cards;

};

class hhPoker : public hhConsole {
	CLASS_PROTOTYPE( hhPoker );
public:

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			Deal();
	void			Draw();
	void			Mark(int card);
	void			IncBet();
	void			DecBet();
	void			BestHand(float probability);
	void			Reset();

	void			EvaluateHand(bool score);
	void			UpdateView();
	bool			HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);

protected:
	void			Event_Deal();
	void			Event_Draw();
	void			Event_UpdateView();

private:
	static const hand_t *hands;
	hhDeck				deck;
	hhPokerHand			PlayerHand;

	bool				markedCards[5];
	int					Bet;
	int					PlayerBet;
	int					PlayerCredits;
	int					victoryAmount;
	int					currentHandIndex;
	int					creditsWon;

	bool bGameOver;
	bool bCanDeal;
	bool bCanIncBet;
	bool bCanDecBet;
	bool bCanDraw;
	bool bCanMark1;
	bool bCanMark2;
	bool bCanMark3;
	bool bCanMark4;
	bool bCanMark5;
};

#endif /* __GAME_POKER_H__ */

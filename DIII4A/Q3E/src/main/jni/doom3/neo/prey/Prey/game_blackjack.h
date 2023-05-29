
#ifndef __GAME_BLACKJACK_H__
#define __GAME_BLACKJACK_H__

extern const idEventDef EV_Deal;

typedef struct card_s {
	int Suit;
	int Value;
	char Name;
	unsigned char Visible;
}card_t;

class hhBlackJack : public hhConsole {
public:
	CLASS_PROTOTYPE( hhBlackJack );

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	void			Deal();
	void			Hit();
	void			Stay();
	void			Double();
	void			IncBet();
	void			DecBet();

	void			Reset();
	card_t			GetCard(bool visible);
	void			RetallyScores();
	void			AssessScores();
	void			EndGame();
	void			UpdateView();
	void			FollowDealerRules();
	void			DeterminePlayCommands();
	void			UpdateBetMechanics();
	bool			HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);

protected:
	void			Event_Deal();
	void			Event_Hit();
	void			Event_Stay();
	void			Event_Double();
	void			Event_UpdateView();

private:
	idList <card_t> PlayerHand;
	idList <card_t> DealerHand;

	int Bet;
	int PlayerBet;
	int DealerScore, PlayerScore;
	int DealerAces, PlayerAces;
	int PlayerCredits;
	int victoryAmount;
	int resultIndex;
	int creditsWon;

	bool bCanDeal;
	bool bCanIncBet;
	bool bCanDecBet;
	bool bCanHit;
	bool bCanStay;
	bool bCanDouble;
	bool bCanSplit;
};


#endif /* __GAME_BLACKJACK_H__ */

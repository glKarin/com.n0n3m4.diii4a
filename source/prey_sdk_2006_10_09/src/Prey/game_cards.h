#ifndef __GAME_CARDS_H__
#define __GAME_CARDS_H__

	enum suits_e {
		SUIT_DIAMONDS=0,
		SUIT_HEARTS,
		SUIT_SPADES,
		SUIT_CLUBS,
		NUM_SUITS
	};

	enum cards_e {
		CARD_DEUCE=0,
		CARD_THREE,
		CARD_FOUR,
		CARD_FIVE,
		CARD_SIX,
		CARD_SEVEN,
		CARD_EIGHT,
		CARD_NINE,
		CARD_TEN,
		CARD_JACK,
		CARD_QUEEN,
		CARD_KING,
		CARD_ACE,
		NUM_CARD_VALUES
	};

	class hhCard {
	public:
						hhCard();
						hhCard(int value, int suit);
		char			ValueName();
		char			SuitName();
		int				operator==(const hhCard &other) const;
		int				Value() const;
		int				Suit() const;

	private:
		int				value;
		int				suit;
	};


	class hhDeck : public idClass {
		CLASS_PROTOTYPE( hhDeck );

	public:
						hhDeck();
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );
		void			Generate();
		void			Shuffle();
		bool			HasCard(hhCard &card);
		hhCard			GetCard();
		hhCard			GetCard(int value, int suit);
		void			ReturnCard(hhCard card);

	private:
		hhStack<hhCard>	cards;
	};

#endif /* __GAME_CARDS_H__ */

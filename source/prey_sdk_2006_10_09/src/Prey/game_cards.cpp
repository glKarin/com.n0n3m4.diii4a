// game_cards.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


static char cardnames[] = { '2','3','4','5','6','7','8','9','T','J','Q','K','A' };
static char suitnames[] = { 'D','H','S','C' };


//==============================================================
// hhCard utility class
//==============================================================
hhCard::hhCard() {
	suit = SUIT_SPADES;
	value = CARD_ACE;
}

hhCard::hhCard(int value, int suit) {
	this->suit = suit;
	this->value = value;
}

int hhCard::operator==(const hhCard &other) const {
	return value==other.Value() && suit==other.Suit();
}

int hhCard::Value() const {
	return value;
}

int hhCard::Suit() const {
	return suit;
}

char hhCard::ValueName() {
	return cardnames[value];
}

char hhCard::SuitName() {
	return suitnames[suit];
}


//==============================================================
// hhDeck utility class
//==============================================================

CLASS_DECLARATION(idClass, hhDeck)
END_CLASS

hhDeck::hhDeck() {
	Generate();
	Shuffle();
}

void hhDeck::Save(idSaveGame *savefile) const {
	int i;
	savefile->WriteInt( cards.Num() );		// hhStack<hhCard>
	for (i=0; i<cards.Num(); i++) {
		savefile->Write(&cards[i], sizeof(hhCard));
	}
}

void hhDeck::Restore( idRestoreGame *savefile ) {
	int i, num;
	hhCard card;

	cards.Clear();							// hhStack<hhCard>
	savefile->ReadInt( num );
	cards.SetNum( num );
	for (i=0; i<num; i++) {
		savefile->Read(&card, sizeof(hhCard));
		cards[i] = card;
	}
}

void hhDeck::Generate() {
	int value, suit;
	cards.Clear();
	for (value=0; value<NUM_CARD_VALUES; value++) {
		for (suit=0; suit<NUM_SUITS; suit++) {
			hhCard card(value, suit);
			cards.Append(card);
		}
	}
}

void hhDeck::Shuffle() {
	int numCards = cards.Num();
	for (int ix=0; ix<5; ix++) {
		for (int c1=0; c1<numCards; c1++) {
			int c2 = gameLocal.random.RandomInt(numCards);
			idSwap(cards[c1], cards[c2]);
		}

		// Cut?
	}
}

bool hhDeck::HasCard(hhCard &card) {
	int numCards = cards.Num();
	for (int ix=0; ix<numCards; ix++) {
		if (cards[ix] == card) {
			return true;
		}
	}
	return false;
}

hhCard hhDeck::GetCard() {
	return cards.Pop();
}

hhCard hhDeck::GetCard(int value, int suit) {
	hhCard card(value, suit);
	assert(HasCard(card));

	// Find card and remove from deck
	cards.Remove(card);
	return card;
}

void hhDeck::ReturnCard(hhCard card) {
	cards.Push(card);
}


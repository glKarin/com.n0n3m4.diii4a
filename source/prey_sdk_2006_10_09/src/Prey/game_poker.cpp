// game_poker.cpp
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// Static declaration of our hands array
const hand_t localhands[NUM_POKER_HANDS] = {
	{	HAND_ROYALFLUSH,		5000,		PREQ_ROYALS|PREQ_FLUSH|PREQ_STRAIGHT	},
	{	HAND_STRAIGHTFLUSH,		1000,		PREQ_FLUSH|PREQ_STRAIGHT				},
	{	HAND_FOUROFKIND,		100,		PREQ_4MATCH								},
	{	HAND_FULLHOUSE,			50,			PREQ_FULLHOUSE							},
	{	HAND_FLUSH,				20,			PREQ_FLUSH								},
	{	HAND_STRAIGHT,			10,			PREQ_STRAIGHT							},
	{	HAND_THREEOFKIND,		5,			PREQ_3MATCH								},
	{	HAND_TWOPAIR,			3,			PREQ_2PAIR								},
	// Jacks or better is hardcoded at 2
	{	HAND_PAIR,				1,			PREQ_2MATCH								},
	{	HAND_NOTHING,			0,			0										}
};

// Assign reference to our local array to our static class
const hand_t * hhPoker::hands = localhands;


//==============================================================
// hhPokerHand utility class
//==============================================================

CLASS_DECLARATION(idClass, hhPokerHand)
END_CLASS

hhPokerHand::hhPokerHand() {
	Clear();
}

void hhPokerHand::Save(idSaveGame *savefile) const {
	savefile->Write(values, sizeof(suitHist_t)*NUM_SUITS);
	savefile->Write(suits, sizeof(valueHist_t)*NUM_CARD_VALUES);

	savefile->WriteInt(cards.Num());			// idList<hhCard>
	for (int i=0; i<cards.Num(); i++) {
		savefile->Write(&cards[i], sizeof(hhCard));
	}
}

void hhPokerHand::Restore( idRestoreGame *savefile ) {
	int i, num;
	hhCard card;

	savefile->Read(values, sizeof(suitHist_t)*NUM_SUITS);
	savefile->Read(suits, sizeof(valueHist_t)*NUM_CARD_VALUES);

	cards.Clear();
	savefile->ReadInt( num );
	cards.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->Read(&card, sizeof(hhCard));
		cards[i] = card;
	}
}

void hhPokerHand::operator=(hhPokerHand &other) {
	Clear();
	for (int ix=0; ix<other.cards.Num(); ix++) {
		AddCard(other.cards[ix]);
	}
}

void hhPokerHand::Clear() {
	int ix;
	cards.Clear();
	for (ix=0; ix<NUM_CARD_VALUES; ix++) {
		values[ix].count = 0;
	}
	for (ix=0; ix<NUM_SUITS; ix++) {
		suits[ix].count = 0;
	}
}

int hhPokerHand::MaxCountOfValues() {
	int maxcount = 0;
	for (int ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (values[ix].count > maxcount) {
			maxcount = values[ix].count;
		}
	}
	return maxcount;
}

int hhPokerHand::ValueOfMaxCount() {
	int maxcount = 0;
	int maxcountvalue = -1;
	for (int ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (values[ix].count > maxcount) {
			maxcount = values[ix].count;
			maxcountvalue = ix;
		}
	}
	return maxcountvalue;
}

int hhPokerHand::GetLowValue() {
	int first = -1;
	for (int ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (values[ix].count > 0) {
			first = ix;
			break;
		}
	}
	return first;
}

int hhPokerHand::GetHighValue() {
	int last = -1;
	for (int ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (values[ix].count > 0) {
			last = ix;
		}
	}
	return last;
}

bool hhPokerHand::HasValue(int value) {
	return values[value].count > 0;
}

int hhPokerHand::NumUniqueValues() {
	int numvalues = 0;
	for (int ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (values[ix].count > 0) {
			numvalues++;
		}
	}
	return numvalues;
}

int hhPokerHand::NumOfValue(int value) {
	assert(value>=0 && value<NUM_CARD_VALUES);
	return values[value].count;
}

int hhPokerHand::NumUniqueSuits() {
	int count = 0;
	for (int ix=0; ix<NUM_SUITS; ix++) {
		if (suits[ix].count > 0) {
			count++;
		}
	}
	return count;
}

int hhPokerHand::SuitOfMaxCount() {
	int maxcount = 0;
	int maxsuit = -1;
	for (int ix=0; ix<NUM_SUITS; ix++) {
		if (suits[ix].count > maxcount) {
			maxcount = suits[ix].count;
			maxsuit = ix;
		}
	}
	return maxsuit;
}

bool hhPokerHand::Search(hhCard &card) {
	for (int ix=0; ix<cards.Num(); ix++) {
		if (cards[ix] == card) {
			return true;
		}
	}
	return false;
}

void hhPokerHand::AddCard(hhCard card) {
	values[card.Value()].count++;
	suits[card.Suit()].count++;
	cards.Append(card);
}



//==============================================================
// hhPoker
//==============================================================

const idEventDef EV_Draw("Draw", NULL);
const idEventDef EV_UpdateView("<updateview>", NULL);

CLASS_DECLARATION(hhConsole, hhPoker)
	EVENT( EV_Deal,			hhPoker::Event_Deal)
	EVENT( EV_Draw,			hhPoker::Event_Draw)
	EVENT( EV_UpdateView,	hhPoker::Event_UpdateView)
END_CLASS

void hhPoker::Spawn() {

	Reset();
}

void hhPoker::Reset() {
	bCanDeal   = true;
	bCanIncBet = true;
	bCanDecBet = true;
	bCanDraw   = false;
	bGameOver = false;
	Bet = PlayerBet  = 1;
	memset(markedCards, 0, sizeof(markedCards));
	victoryAmount = spawnArgs.GetInt("victory");
	PlayerCredits = spawnArgs.GetInt("credits");
	currentHandIndex = -1;
	creditsWon = 0;

	bCanMark1 = bCanMark2 = bCanMark3 = bCanMark4 = bCanMark5 = false;
	PlayerHand.Clear();

	UpdateView();
}

void hhPoker::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject(deck);
	savefile->WriteStaticObject(PlayerHand);
	savefile->Write(markedCards, sizeof(bool)*5);
	savefile->WriteInt(Bet);
	savefile->WriteInt(PlayerBet);
	savefile->WriteInt(PlayerCredits);
	savefile->WriteInt(victoryAmount);
	savefile->WriteInt(currentHandIndex);

	savefile->WriteBool( bGameOver );
	savefile->WriteBool( bCanDeal );
	savefile->WriteBool( bCanIncBet );
	savefile->WriteBool( bCanDecBet );
	savefile->WriteBool( bCanDraw );
	savefile->WriteBool( bCanMark1 );
	savefile->WriteBool( bCanMark2 );
	savefile->WriteBool( bCanMark3 );
	savefile->WriteBool( bCanMark4 );
	savefile->WriteBool( bCanMark5 );
	savefile->WriteInt( creditsWon );
}

void hhPoker::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject(deck);
	savefile->ReadStaticObject(PlayerHand);
	savefile->Read(markedCards, sizeof(bool)*5);
	savefile->ReadInt(Bet);
	savefile->ReadInt(PlayerBet);
	savefile->ReadInt(PlayerCredits);
	savefile->ReadInt(victoryAmount);
	savefile->ReadInt(currentHandIndex);

	savefile->ReadBool( bGameOver );
	savefile->ReadBool( bCanDeal );
	savefile->ReadBool( bCanIncBet );
	savefile->ReadBool( bCanDecBet );
	savefile->ReadBool( bCanDraw );
	savefile->ReadBool( bCanMark1 );
	savefile->ReadBool( bCanMark2 );
	savefile->ReadBool( bCanMark3 );
	savefile->ReadBool( bCanMark4 );
	savefile->ReadBool( bCanMark5 );
	savefile->ReadInt( creditsWon );
}

void hhPoker::Deal() {

	// Shuffle
	deck.Generate();
	deck.Shuffle();

	Bet = PlayerBet;
	PlayerCredits -= Bet;
	creditsWon = 0;

	// Deal initial hand
	PlayerHand.Clear();
	PlayerHand.AddCard(deck.GetCard());
	PlayerHand.AddCard(deck.GetCard());
	PlayerHand.AddCard(deck.GetCard());
	PlayerHand.AddCard(deck.GetCard());
	PlayerHand.AddCard(deck.GetCard());

	EvaluateHand(false);

	bCanDeal = bCanIncBet = bCanDecBet = false;
	bCanDraw = bCanMark1 = bCanMark2 = bCanMark3 = bCanMark4 = bCanMark5 = true;
	for (int ix=0; ix<5; ix++) {
		markedCards[ix] = true;
	}

	UpdateView();
}

void hhPoker::Draw() {
	bCanDeal = bCanIncBet = bCanDecBet = true;
	bCanDraw = bCanMark1 = bCanMark2 = bCanMark3 = bCanMark4 = bCanMark5 = false;

	float luck = idMath::ClampFloat(0.0f, 1.0f, spawnArgs.GetFloat("luck"));
	if (luck) {
		BestHand(luck);
	}
	else {
		// Replace marked cards
		for (int ix=0; ix<5; ix++) {
			if (markedCards[ix]) {
				PlayerHand.cards[ix] = deck.GetCard();
				markedCards[ix] = false;
			}
		}
		hhPokerHand temp;
		temp = PlayerHand;
		PlayerHand = temp;		// Rebuild player hand for correct stats
	}

	EvaluateHand(true);
	UpdateView();
}

void hhPoker::Mark(int card) {
	markedCards[card] ^= 1;
	UpdateView();
}

void hhPoker::IncBet() {

	int amount = 1;
	idUserInterface *gui = renderEntity.gui[0];
	if (gui) {
		amount = gui->GetStateInt("increment");
	}

	if (bCanIncBet) {
		int oldBet = PlayerBet;
		PlayerBet = idMath::ClampInt(PlayerBet, PlayerCredits, PlayerBet+amount);
		PlayerBet = idMath::ClampInt(0, 999999, PlayerBet);
		if (PlayerBet != oldBet) {
			StartSound( "snd_betchange", SND_CHANNEL_ANY );
		}
	}
	UpdateView();
}

void hhPoker::DecBet() {
	int amount = 1;
	idUserInterface *gui = renderEntity.gui[0];
	if (gui) {
		amount = gui->GetStateInt("increment");
	}

	if (bCanDecBet) {
		int oldBet = PlayerBet;
		if (PlayerBet > amount) {
			PlayerBet -= amount;
		}
		else if (PlayerBet > 1) {
			PlayerBet = 1;
		}
		if (PlayerBet != oldBet) {
			StartSound( "snd_betchange", SND_CHANNEL_ANY );
		}
	}
	UpdateView();
}


/*
	hhPoker utility functions
*/

void hhPoker::EvaluateHand(bool score) {
	int ix;
	currentHandIndex = -1;

	int requirements = 0;
	int low = PlayerHand.GetLowValue();
	int high = PlayerHand.GetHighValue();
	int maxcount = PlayerHand.MaxCountOfValues();

	if (low >= CARD_TEN) {
		requirements |= PREQ_ROYALS;
	}

	if (PlayerHand.NumUniqueSuits() == 1) {
		requirements |= PREQ_FLUSH;
	}

	// Check for straight
	if (maxcount==1) {
		requirements |= PREQ_STRAIGHT;
		int cur = low;
		for (ix=1; ix<5; ix++) {
			int next = cur+1;
			if (low == CARD_DEUCE && next == CARD_SIX && PlayerHand.HasValue(CARD_ACE)) {
				// If looking for a '6', accept an 'Ace' also for Ace low straights
			}
			else if (!PlayerHand.HasValue(next)) {
				requirements &= ~PREQ_STRAIGHT;
				break;
			}
			cur = next;
		}
	}

	// Check for Pairs, etc.
	for (ix=0; ix<NUM_CARD_VALUES; ix++) {
		if (PlayerHand.values[ix].count == 4) {
			requirements |= PREQ_4MATCH;
		}
		else if (PlayerHand.values[ix].count == 3) {
			requirements |= PREQ_3MATCH;
		}
		else if (PlayerHand.values[ix].count == 2) {
			if (requirements & PREQ_2MATCH) {
				requirements |= PREQ_2PAIR;
			}
			requirements |= PREQ_2MATCH;
		}
	}
	if ((requirements & (PREQ_3MATCH|PREQ_2MATCH)) == (PREQ_3MATCH|PREQ_2MATCH)) {
		requirements |= PREQ_FULLHOUSE;
	}

	// Assess the best hand
	pokerhand_t hand;
	for (ix=0; ix<NUM_POKER_HANDS; ix++) {
		if ((hands[ix].requirement & requirements) == hands[ix].requirement) {
			hand = hands[ix].hand;
			break;
		}
	}

	currentHandIndex = hand;
	if (score) {

		// Handle Jacks or better
		int payoff = hands[hand].payoff;
		if (hand == HAND_PAIR && PlayerHand.ValueOfMaxCount() >= CARD_JACK) {
			payoff = 2;
		}

		creditsWon = Bet * payoff;
		PlayerCredits += creditsWon;
		PlayerCredits = idMath::ClampInt(0, 999999999, PlayerCredits);
		if (PlayerCredits < PlayerBet) {
			PlayerBet = PlayerCredits;
		}

		if (PlayerCredits <= 0) {
			bCanIncBet = bCanDecBet = bCanDraw = bCanDeal = bCanMark1 = bCanMark2 = bCanMark3 = bCanMark4 = bCanMark5 = false;
			bGameOver = true;
		}

		if (victoryAmount && PlayerCredits >= victoryAmount) {
			StartSound( "snd_victory", SND_CHANNEL_ANY );
			ActivateTargets( gameLocal.GetLocalPlayer() );
			victoryAmount = 0;
		}
		else if (payoff > 10) {
			StartSound( "snd_winbig", SND_CHANNEL_ANY );
		}
		else if (payoff > 0) {
			StartSound( "snd_win", SND_CHANNEL_ANY );
		}
		else {
			StartSound( "snd_lose", SND_CHANNEL_ANY );
		}
	}
}

void hhPoker::UpdateView() {
	// UpdateView() is posted as an event because sometimes we're already nested down in the gui handling code when it is called
	// and it in turn re-enters the gui handling code with HandleNamedEvent()
	CancelEvents(&EV_UpdateView);
	PostEventMS(&EV_UpdateView, 0);
}

void hhPoker::Event_UpdateView() {
	int ix;
	idUserInterface *gui = renderEntity.gui[0];

	if (gui) {

		bool atLeastOneCardKept = !(markedCards[0] && markedCards[1] && markedCards[2] && markedCards[3] && markedCards[4]);

		gui->SetStateBool("bgameover", bGameOver);
		gui->SetStateBool("bcanincbet", bCanIncBet);
		gui->SetStateBool("bcandecbet", bCanDecBet);
		gui->SetStateBool("bcandraw", bCanDraw);// && atLeastOneCardKept);
		gui->SetStateBool("bcandeal", bCanDeal);
		gui->SetStateBool("bcanmark1", bCanMark1);
		gui->SetStateBool("bcanmark2", bCanMark2);
		gui->SetStateBool("bcanmark3", bCanMark3);
		gui->SetStateBool("bcanmark4", bCanMark4);
		gui->SetStateBool("bcanmark5", bCanMark5);
		gui->SetStateInt("currentbet", PlayerBet);
		gui->SetStateInt("credits", PlayerCredits);
		gui->SetStateInt("hand", currentHandIndex);
		gui->SetStateInt("creditswon", creditsWon);

		for (ix=0; ix<5; ix++) {
			gui->SetStateInt(va("Player%d_Visible", ix+1), 0);
		}

		// Show player hand
		for (ix=0; ix<PlayerHand.cards.Num(); ix++) {
			gui->SetStateInt(va("Player%d_Visible", ix+1), 1);
			gui->SetStateInt(va("Player%d_Suit", ix+1), PlayerHand.cards[ix].Suit());
			char cardChar = PlayerHand.cards[ix].ValueName();
			if (cardChar == 'T') {
				gui->SetStateString(va("Player%d_Card", ix+1), "10");
			}
			else {
				gui->SetStateString(va("Player%d_Card", ix+1), va("%c", cardChar));
			}
			gui->SetStateInt(va("Player%d_Red", ix+1), PlayerHand.cards[ix].Suit()==SUIT_HEARTS || PlayerHand.cards[ix].Suit()==SUIT_DIAMONDS ? 1 : 0);
			gui->SetStateString(va("Player%d_Mark", ix+1), markedCards[ix] ? "X" : "");
			gui->SetStateBool(va("Player%d_Marked", ix+1), markedCards[ix] );
		}

		gui->StateChanged(gameLocal.time, true);
		CallNamedEvent("Update");
	}
}

bool hhPoker::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("deal") == 0) {
		Deal();
	}
	else if (token.Icmp("draw") == 0) {
		Draw();
	}
	else if (token.Icmp("mark1") == 0) {
		Mark(0);
	}
	else if (token.Icmp("mark2") == 0) {
		Mark(1);
	}
	else if (token.Icmp("mark3") == 0) {
		Mark(2);
	}
	else if (token.Icmp("mark4") == 0) {
		Mark(3);
	}
	else if (token.Icmp("mark5") == 0) {
		Mark(4);
	}
	else if (token.Icmp("incbet") == 0) {
		IncBet();
	}
	else if (token.Icmp("decbet") == 0) {
		DecBet();
	}
	else if (token.Icmp("reset") == 0) {
		Reset();
	}
	else if (token.Icmp("restart") == 0) {
		bCanDeal = bCanIncBet = bCanDecBet = true;
		bCanDraw = bCanMark1 = bCanMark2 = bCanMark3 = bCanMark4 = bCanMark5 = false;
		PlayerCredits = spawnArgs.GetInt("credits");
		bGameOver = false;
		Bet = PlayerBet = 1;
		UpdateView();
	}
	else {
		src->UnreadToken(&token);
		return false;
	}

	return true;
}

void hhPoker::Event_Deal() {
	Deal();
}

void hhPoker::Event_Draw() {
	Draw();
}

// Experiment: get best hand given discards
void hhPoker::BestHand(float probability) {
	int ix;

	hhPokerHand hand;
	hhPokerHand discards;
	hhPokerHand originalHand;

	hand.Clear();
	originalHand.Clear();
	discards.Clear();

	for (ix=0; ix<5; ix++) {
		if (markedCards[ix]) {
			discards.AddCard(PlayerHand.cards[ix]);
		}
		else {
			hand.AddCard(PlayerHand.cards[ix]);
			originalHand.AddCard(PlayerHand.cards[ix]);
		}
	}

	int possibilities = 0xFF;

	// Check for possibilities of requirements using hands
	for (ix=0; ix<hand.cards.Num(); ix++) {
		if (hand.cards[ix].Suit() != hand.cards[0].Suit()) {
			possibilities &= ~PREQ_FLUSH;
		}
		if (hand.cards[ix].Value() < CARD_TEN) {
			possibilities &= ~PREQ_ROYALS;
		}
	}

	// Check for possibilities of requirements
	int maxcount = hand.MaxCountOfValues();
	int first = hand.GetLowValue();
	int last = hand.GetHighValue();

	// Check for straight possibility
	if (maxcount > 1) {
		possibilities &= ~PREQ_STRAIGHT;
	}
	if (first != -1 && last != -1 && last-first>=5) {
		if (hand.HasValue(CARD_ACE)) {	// Extra check to allow for ace low straight
			last = first;
			first = -1;	// put ace at begining, find new last and try again
			for (ix=0; ix<CARD_ACE; ix++) {
				if (hand.values[ix].count > 0) {
					last = ix;
				}
			}
			if (last-first>=5) {
				possibilities &= ~PREQ_STRAIGHT;
			}
		}
		else {
			possibilities &= ~PREQ_STRAIGHT;
		}
	}

	// Check for full house possibility
	if (hand.NumUniqueValues() > 2) {
		possibilities &= ~PREQ_FULLHOUSE;
		possibilities &= ~PREQ_2PAIR;
	}

	// check for 'x of a kind' possibility
	if (maxcount + discards.cards.Num() < 4) {
		possibilities &= ~PREQ_4MATCH;
	}
	if (maxcount + discards.cards.Num() < 3) {
		possibilities &= ~PREQ_3MATCH;
	}
	if (maxcount + discards.cards.Num() < 2) {
		possibilities &= ~PREQ_2MATCH;
	}

	// Go through hands in decending order of greatness looking for possibilities
	for (ix=0; ix<NUM_POKER_HANDS; ix++) {
		int requirements = hands[ix].requirement;
		if ((possibilities & requirements)==requirements) {
			int value, suit;
			// Give cards to fullfill requirements
			for (int c=0; c<discards.cards.Num(); c++) {
				value = -1;		// If this is never set, give a random card because it doesn't matter
				if (requirements&PREQ_ROYALS) {
					value = CARD_TEN;
					while (hand.HasValue(value)) {
						value++;
					}
				}
				else if (requirements&PREQ_STRAIGHT) {
					if (hand.HasValue(CARD_ACE) && first <= CARD_FIVE) {	// ace low straight
						value = CARD_DEUCE;
					}
					else {
						value = first>CARD_TEN ? CARD_TEN : first;	// Start at ten or lower so it will fit
					}
					while (hand.HasValue(value)) {
						value++;
					}
				}

				int hotvalue;
				if (requirements&PREQ_FULLHOUSE) {
					hotvalue = hand.ValueOfMaxCount();
					if (hotvalue != -1) {
						if (hand.NumOfValue(hotvalue) < 3) {
							value = hotvalue;
						}
						else {
							value = (hotvalue==first) ? last : first;
						}
					}
				}

				if (requirements&PREQ_2PAIR) {
					if (hand.NumOfValue(first) < 2) {
						value = first;
					}
					else if (hand.NumOfValue(last) < 2) {
						value = last;
					}
					else {
						value = CARD_ACE;
						while (hand.HasValue(value)) {
							value--;
						}
					}
				}

				if ((hand.MaxCountOfValues()<4) && (requirements&PREQ_4MATCH)) {
					value = hand.ValueOfMaxCount();
				}
				if ((hand.MaxCountOfValues()<3) && (requirements&PREQ_3MATCH)) {
					value = hand.ValueOfMaxCount();
				}
				if ((hand.MaxCountOfValues()<2) && (requirements&PREQ_2MATCH)) {
					value = hand.ValueOfMaxCount();
				}

				// Determine suit properties
				if (requirements&PREQ_FLUSH) {
					suit = hand.SuitOfMaxCount();
					if (suit == -1) {
						suit = gameLocal.random.RandomInt() % NUM_SUITS;	// random fixed suit
					}
					if (value == -1) {	// Need a value
						value = CARD_ACE;
						while (!deck.HasCard(hhCard(value, suit))) {
							value--;
						}
					}
				}
				else if (value != -1) {
					// Choose a suit different from held values
					suit = 0;
					while ( !deck.HasCard(hhCard(value, suit)) ) {
						if (++suit >= NUM_SUITS) {
							break;
						}
					}
					if (suit >= NUM_SUITS) {	// Can't make our hand - set back to a used card and it will break out to next possibility below
						suit = SUIT_DIAMONDS;
					}
				}

				// Add card to hand
				if (value == -1) {	// This means any card is okay, give one from deck
					hand.AddCard(deck.GetCard());
				}
				else if (deck.HasCard(hhCard(value,suit))) {
					hand.AddCard(deck.GetCard(value, suit));
				}
				else {
					for (int jx=originalHand.cards.Num(); jx<hand.cards.Num(); jx++) {
						deck.ReturnCard(hand.cards[jx]);
					}
					hand = originalHand;
					break;
				}
			}
		}

		if (hand.cards.Num() == 5) {
			break;
		}
	}

	// Decide whether to give each card based on probability
	bool bHandCompromised = false;
	for (ix=originalHand.cards.Num(); ix<5; ix++) {
		if (!bHandCompromised && gameLocal.random.RandomFloat() > probability) {
			hand.cards[ix] = deck.GetCard();
			bHandCompromised = true;	// only compromise hand with one card
		}
	}

	// Copy into playerhand
	hhPokerHand temp;
	int drawcard = originalHand.cards.Num();
	for (ix=0; ix<5; ix++) {
		if (markedCards[ix]) {
			PlayerHand.cards[ix] = hand.cards[drawcard++];
			markedCards[ix] = false;
		}
		temp.AddCard(PlayerHand.cards[ix]);
	}
	PlayerHand.Clear();		// Rebuild player hand for correct stats
	for (ix=0; ix<5; ix++) {
		PlayerHand.AddCard(temp.cards[ix]);
	}
}


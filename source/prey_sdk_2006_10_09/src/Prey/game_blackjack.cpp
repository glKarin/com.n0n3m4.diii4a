// game_blackjack
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

enum {
	BJRESULT_BUST=0,
	BJRESULT_PUSH=1,
	BJRESULT_WIN=2,
	BJRESULT_LOSE=3,
	BJRESULT_BLACKJACK=4,
	BJRESULT_5CARD=5
};

const idEventDef EV_Deal("Deal", NULL);
const idEventDef EV_Hit("Hit", NULL);
const idEventDef EV_Stay("Stay", NULL);
const idEventDef EV_Double("Double", NULL);

CLASS_DECLARATION(hhConsole, hhBlackJack)
	EVENT( EV_Deal,			hhBlackJack::Event_Deal)
	EVENT( EV_Hit,			hhBlackJack::Event_Hit)
	EVENT( EV_Stay,			hhBlackJack::Event_Stay)
	EVENT( EV_Double,		hhBlackJack::Event_Double)
	EVENT( EV_UpdateView,	hhBlackJack::Event_UpdateView)
END_CLASS


void hhBlackJack::Spawn() {

	Reset();
}

void hhBlackJack::Reset() {
	bCanDeal   = 1;
	bCanIncBet = 1;
	bCanDecBet = 1;
	bCanHit    = 0;
	bCanStay   = 0;
	bCanDouble = 0;
	bCanSplit  = 0;
	PlayerBet = Bet = 1;
	DealerScore = PlayerScore = 0;
	DealerAces = PlayerAces = 0;
	PlayerCredits = spawnArgs.GetInt("credits");
	victoryAmount = spawnArgs.GetInt("victory");
	resultIndex = -1;
	creditsWon = 0;
	PlayerHand.Clear();
	DealerHand.Clear();

	UpdateView();
}

void hhBlackJack::Save(idSaveGame *savefile) const {
	int i;

	savefile->WriteInt( PlayerHand.Num() );		// Saving of idList<card_t>
	for( i = 0; i < PlayerHand.Num(); i++ ) {
		savefile->Write(&PlayerHand[i], sizeof(card_t));
	}
	savefile->WriteInt( DealerHand.Num() );		// Saving of idList<card_t>
	for( i = 0; i < DealerHand.Num(); i++ ) {
		savefile->Write(&DealerHand[i], sizeof(card_t));
	}

	savefile->WriteInt(Bet);
	savefile->WriteInt(PlayerBet);
	savefile->WriteInt(DealerScore);
	savefile->WriteInt(PlayerScore);
	savefile->WriteInt(DealerAces);
	savefile->WriteInt(PlayerAces);
	savefile->WriteInt(PlayerCredits);
	savefile->WriteInt(victoryAmount);
	savefile->WriteInt(resultIndex);
	savefile->WriteBool(bCanDeal);
	savefile->WriteBool(bCanIncBet);
	savefile->WriteBool(bCanDecBet);
	savefile->WriteBool(bCanHit);
	savefile->WriteBool(bCanStay);
	savefile->WriteBool(bCanDouble);
	savefile->WriteBool(bCanSplit);
	savefile->WriteInt(creditsWon);
}

void hhBlackJack::Restore( idRestoreGame *savefile ) {
	int i, num;

	PlayerHand.Clear();
	savefile->ReadInt( num );
	PlayerHand.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->Read(&PlayerHand[i], sizeof(card_t));
	}

	DealerHand.Clear();
	savefile->ReadInt( num );
	DealerHand.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->Read(&DealerHand[i], sizeof(card_t));
	}

	savefile->ReadInt(Bet);
	savefile->ReadInt(PlayerBet);
	savefile->ReadInt(DealerScore);
	savefile->ReadInt(PlayerScore);
	savefile->ReadInt(DealerAces);
	savefile->ReadInt(PlayerAces);
	savefile->ReadInt(PlayerCredits);
	savefile->ReadInt(victoryAmount);
	savefile->ReadInt(resultIndex);
	savefile->ReadBool(bCanDeal);
	savefile->ReadBool(bCanIncBet);
	savefile->ReadBool(bCanDecBet);
	savefile->ReadBool(bCanHit);
	savefile->ReadBool(bCanStay);
	savefile->ReadBool(bCanDouble);
	savefile->ReadBool(bCanSplit);
	savefile->ReadInt(creditsWon);
}

card_t hhBlackJack::GetCard(bool visible)
{
	card_t card;
	char names[] = { '2','3','4','5','6','7','8','9','T','J','Q','K','A'};
	int values[] = {  2 , 3,  4,  5,  6,  7,  8,  9,  10, 10, 10, 10, 11};

	int r = gameLocal.random.RandomInt(13);
	card.Name = names[r];
	card.Value = values[r];
	card.Suit = gameLocal.random.RandomInt(4);
	card.Visible = visible;
	return card;
}

void hhBlackJack::Deal()
{
	// Empty hands
	PlayerHand.Clear();
	DealerHand.Clear();

	Bet = PlayerBet;
	creditsWon = 0;

	// Deal initial hands
	PlayerHand.Append(GetCard(1));
	DealerHand.Append(GetCard(0));
	PlayerHand.Append(GetCard(1));
	DealerHand.Append(GetCard(1));

	RetallyScores();

	bCanIncBet = false;
	bCanDecBet = false;
	bCanDeal = false;
	DeterminePlayCommands();
	resultIndex = -1;

	if (PlayerScore == 21) {
		AssessScores();
		EndGame();
	}

	UpdateView();
}

void hhBlackJack::Hit()
{
	PlayerHand.Append(GetCard(1));
	RetallyScores();
	DeterminePlayCommands();

	if (PlayerScore >= 21 || PlayerHand.Num() == 5) {
		FollowDealerRules();
		AssessScores();
		EndGame();
	}

	UpdateView();
}

void hhBlackJack::Stay()
{
	FollowDealerRules();
	AssessScores();
	EndGame();
	UpdateView();
}

void hhBlackJack::Double()
{
	Bet *= 2;
	PlayerHand.Append(GetCard(1));
	RetallyScores();
	DeterminePlayCommands();

	// Force player to stay
	FollowDealerRules();
	AssessScores();
	EndGame();

	UpdateView();
}

void hhBlackJack::IncBet() {

	int amount = 1;
	idUserInterface *gui = renderEntity.gui[0];
	if (gui) {
		amount = gui->GetStateInt("increment");
	}

	int oldBet = PlayerBet;
	PlayerBet = idMath::ClampInt(PlayerBet, PlayerCredits, PlayerBet+amount);
	PlayerBet = idMath::ClampInt(0, 999999, PlayerBet);
	Bet = PlayerBet;
	if (PlayerBet != oldBet) {
		StartSound( "snd_betchange", SND_CHANNEL_ANY );
	}

	UpdateView();
}

void hhBlackJack::DecBet() {

	int amount = 1;
	idUserInterface *gui = renderEntity.gui[0];
	if (gui) {
		amount = gui->GetStateInt("increment");
	}

	int oldBet = PlayerBet;
	if (PlayerBet > amount) {
		PlayerBet -= amount;
	}
	else if (PlayerBet > 1) {
		PlayerBet = 1;
	}
	Bet = PlayerBet;
	if (PlayerBet != oldBet) {
		StartSound( "snd_betchange", SND_CHANNEL_ANY );
	}

	UpdateView();
}


/*
	Blackjack utility functions
*/

void hhBlackJack::RetallyScores()
{
	int ix;
	PlayerScore = DealerScore = PlayerAces = DealerAces = 0;

	for (ix=0; ix<PlayerHand.Num(); ix++) {
		PlayerScore += PlayerHand[ix].Value;
		PlayerAces += PlayerHand[ix].Value == 11 ? 1 : 0;
	}
	while (PlayerScore > 21 && PlayerAces) {
		PlayerScore -= 10;
		PlayerAces--;
	}
	for (ix=0; ix<DealerHand.Num(); ix++) {
		DealerScore += DealerHand[ix].Value;
		DealerAces += DealerHand[ix].Value == 11 ? 1 : 0;
	}
	while (DealerScore > 21 && DealerAces) {
		DealerScore -= 10;
		DealerAces--;
	}
}

void hhBlackJack::AssessScores()
{
	if (PlayerScore > 21) {
		creditsWon = -Bet;
		resultIndex = BJRESULT_BUST;
	}
	else if (DealerScore > 21) {
		creditsWon = Bet;
		resultIndex = BJRESULT_WIN;
	}
	else if (PlayerScore == 21 && PlayerHand.Num() == 2) {
		// BlackJack
		creditsWon = Bet * 2;
		resultIndex = BJRESULT_BLACKJACK;
	}
	else if (PlayerScore <= 21 && PlayerHand.Num() == 5) {
		creditsWon = Bet * 5;
		resultIndex = BJRESULT_5CARD;
	}
	else if (DealerScore > PlayerScore) {
		creditsWon = -Bet;
		resultIndex = BJRESULT_LOSE;
	}
	else if (PlayerScore > DealerScore) {
		creditsWon = Bet;
		resultIndex = BJRESULT_WIN;
	}
	else {
		// Push
		creditsWon = 0;
		resultIndex = BJRESULT_PUSH;
	}

	PlayerCredits += creditsWon;
	PlayerCredits = idMath::ClampInt(0, 999999999, PlayerCredits);
	Bet = PlayerBet;

	// Play victory/failure sound
	if (victoryAmount && PlayerCredits >= victoryAmount) {
		StartSound( "snd_victory", SND_CHANNEL_ANY, 0, true, NULL );
		ActivateTargets( gameLocal.GetLocalPlayer() );
		victoryAmount = 0;
	}
	else if (creditsWon > 0) {
		StartSound( "snd_win", SND_CHANNEL_ANY, 0, true, NULL );
	}
	else if (creditsWon < 0) {
		StartSound( "snd_lose", SND_CHANNEL_ANY, 0, true, NULL );
	}
}

void hhBlackJack::UpdateBetMechanics() {
	bCanIncBet = PlayerCredits > PlayerBet;
	bCanDecBet = PlayerBet > 0;
	if (PlayerCredits < PlayerBet) {
		PlayerBet = PlayerCredits;
	}
}

void hhBlackJack::EndGame() {
	bCanDeal   = 1;
	UpdateBetMechanics();
	bCanHit    = 0;
	bCanStay   = 0;
	bCanDouble = 0;
	bCanSplit  = 0;
}

void hhBlackJack::UpdateView() {
	// UpdateView() is posted as an event because sometimes we're already nested down in the gui handling code when it is called
	// and it in turn re-enters the gui handling code with HandleNamedEvent()
	CancelEvents(&EV_UpdateView);
	PostEventMS(&EV_UpdateView, 0);
}

void hhBlackJack::Event_UpdateView() {
	int ix;
	bool bGameOver = false;
	idUserInterface *gui = renderEntity.gui[0];

	if (gui) {
		if (PlayerCredits <= 0) {
			bCanIncBet = bCanDecBet = bCanHit = bCanStay = bCanDouble = bCanSplit = bCanDeal = false;
			bGameOver = true;
		}

		gui->SetStateBool("bgameover", bGameOver);
		gui->SetStateBool("bcanincbet", bCanIncBet);
		gui->SetStateBool("bcandecbet", bCanDecBet);
		gui->SetStateBool("bcanhit", bCanHit);
		gui->SetStateBool("bcanstay", bCanStay);
		gui->SetStateBool("bcandouble", bCanDouble);
		gui->SetStateBool("bcansplit", bCanSplit);
		gui->SetStateBool("bcandeal", bCanDeal);
		gui->SetStateInt("credits", PlayerCredits);
		gui->SetStateInt("currentbet", Bet);
		gui->SetStateInt("result", resultIndex);
		gui->SetStateInt("creditswon", creditsWon);

		// Clear
		for (ix=0; ix<6; ix++) {
			gui->SetStateInt(va("Dealer%d_Visible", ix+1), 0);
			gui->SetStateInt(va("Player%d_Visible", ix+1), 0);
			gui->SetStateBool(va("Dealer%d_Flipped", ix+1), 0);
			gui->SetStateBool(va("Player%d_Flipped", ix+1), 0);
		}

		// Show dealer hand
		for (ix=0; ix<DealerHand.Num(); ix++) {
			gui->SetStateInt(va("Dealer%d_Visible", ix+1), 1);
			if (DealerHand[ix].Visible) {
				gui->SetStateBool(va("Dealer%d_Flipped", ix+1), 1);
				gui->SetStateInt(va("Dealer%d_Suit", ix+1), DealerHand[ix].Suit);
				char cardChar = DealerHand[ix].Name;
				if (cardChar == 'T') {
					gui->SetStateString(va("Dealer%d_Card", ix+1), "10");
				}
				else {
					gui->SetStateString(va("Dealer%d_Card", ix+1), va("%c", cardChar));
				}
				gui->SetStateInt(va("Dealer%d_Red", ix+1), DealerHand[ix].Suit==SUIT_HEARTS || DealerHand[ix].Suit==SUIT_DIAMONDS ? 1 : 0);
			}
		}

		// Show player hand
		for (ix=0; ix<PlayerHand.Num(); ix++) {
			gui->SetStateInt(va("Player%d_Visible", ix+1), 1);
			if (PlayerHand[ix].Visible) {
				gui->SetStateBool(va("Player%d_Flipped", ix+1), 1);
				gui->SetStateInt(va("Player%d_Suit", ix+1), PlayerHand[ix].Suit);
				char cardChar = PlayerHand[ix].Name;
				if (cardChar == 'T') {
					gui->SetStateString(va("Player%d_Card", ix+1), "10");
				}
				else {
					gui->SetStateString(va("Player%d_Card", ix+1), va("%c", cardChar));
				}
				gui->SetStateInt(va("Player%d_Red", ix+1), PlayerHand[ix].Suit==SUIT_HEARTS || PlayerHand[ix].Suit==SUIT_DIAMONDS ? 1 : 0);
			}
		}

		gui->StateChanged(gameLocal.time, true);
		CallNamedEvent("Update");
	}
}

void hhBlackJack::FollowDealerRules() {
	// Flip up cards
	for (int i=0; i<DealerHand.Num(); i++)
		DealerHand[i].Visible = 1;

	// Only take cards if player is NOT busted, not 5 carded, and non blackjacked
	if ( PlayerScore <= 21 && PlayerHand.Num() < 5 && (PlayerScore != 21 || PlayerHand.Num() != 2) ) {
		while (DealerScore <= 16 && DealerHand.Num() < 5) {
			DealerHand.Append(GetCard(1));
			RetallyScores();
		}
	}

	UpdateView();
}

void hhBlackJack::DeterminePlayCommands() {
	bCanStay =		1;
	bCanHit =		PlayerScore < 21;
	bCanDouble =	(PlayerHand.Num() == 2) && (Bet*2 <= PlayerCredits);
//	bCanSplit =		PlayerHand.Num() == 2 && (PlayerHand[0].Name == PlayerHand[1].Name);
}

bool hhBlackJack::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

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
	else if (token.Icmp("hit") == 0) {
		Hit();
	}
	else if (token.Icmp("stay") == 0) {
		Stay();
	}
	else if (token.Icmp("double") == 0) {
		Double();
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
		PlayerCredits = spawnArgs.GetInt("credits");
		Bet = PlayerBet = 1;
		EndGame();
		UpdateView();
	}
	else {
		src->UnreadToken(&token);
		return false;
	}

	return true;
}

void hhBlackJack::Event_Deal() {
	Deal();
}

void hhBlackJack::Event_Hit() {
	Hit();
}

void hhBlackJack::Event_Stay() {
	Stay();
}

void hhBlackJack::Event_Double() {
	Double();
}

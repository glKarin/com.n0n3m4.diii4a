// hhSlots
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

enum {
	SLOTRESULT_NONE=0,
	SLOTRESULT_LOSE=1,
	SLOTRESULT_WIN=2
};

const idEventDef EV_Spin("spin", NULL);

CLASS_DECLARATION(hhConsole, hhSlots)
	EVENT( EV_Spin,			hhSlots::Event_Spin)
END_CLASS


void hhSlots::Spawn() {
	int ix;

	fruitTextures[FRUIT_CHERRY]		= spawnArgs.GetString("mtr_cherry");
	fruitTextures[FRUIT_ORANGE]		= spawnArgs.GetString("mtr_orange");
	fruitTextures[FRUIT_LEMON]		= spawnArgs.GetString("mtr_lemon");
	fruitTextures[FRUIT_APPLE]		= spawnArgs.GetString("mtr_apple");
	fruitTextures[FRUIT_GRAPE]		= spawnArgs.GetString("mtr_grape");
	fruitTextures[FRUIT_MELON]		= spawnArgs.GetString("mtr_melon");
	fruitTextures[FRUIT_BAR]		= spawnArgs.GetString("mtr_bar");
	fruitTextures[FRUIT_BARBAR]		= spawnArgs.GetString("mtr_barbar");
	fruitTextures[FRUIT_BARBARBAR]	= spawnArgs.GetString("mtr_barbarbar");

	for (ix=0; ix<SLOTS_IN_REEL; ix++) {
		reel1[ix] = (fruit_t)(ix % NUM_FRUITS);
		reel2[ix] = (fruit_t)(ix % NUM_FRUITS);
		reel3[ix] = (fruit_t)(ix % NUM_FRUITS);
	}

	// Shuffle reels
	int f1;
	for (ix=0; ix<5; ix++) {
		for (f1=0; f1<SLOTS_IN_REEL; f1++) {
			idSwap(reel1[f1], reel1[gameLocal.random.RandomInt(SLOTS_IN_REEL)]);
			idSwap(reel2[f1], reel2[gameLocal.random.RandomInt(SLOTS_IN_REEL)]);
			idSwap(reel3[f1], reel3[gameLocal.random.RandomInt(SLOTS_IN_REEL)]);
		}
	}

	Reset();
}

void hhSlots::Reset() {
	bCanSpin   = 1;
	bCanIncBet = 1;
	bCanDecBet = 1;
	Bet = PlayerBet = 1;
	PlayerCredits = spawnArgs.GetInt("credits");
	victoryAmount = spawnArgs.GetInt("victory");
	result = SLOTRESULT_NONE;
	creditsWon = 0;
	bSpinning = false;
	reelRate1 = reelRate2 = reelRate3 = 0;
	reelPos1 = reelPos2 = reelPos3 = 0;
	UpdateView();
}

void hhSlots::Save(idSaveGame *savefile) const {
	int i;
	savefile->WriteInt( Bet );
	savefile->WriteInt( PlayerBet );
	savefile->WriteInt( PlayerCredits );
	savefile->WriteInt( result );
	savefile->WriteInt( creditsWon );
	savefile->WriteInt( victoryAmount );
	for (i=0; i<NUM_FRUITS; i++) {
		savefile->WriteString( fruitTextures[i] );
	}

	for (i=0; i<SLOTS_IN_REEL; i++) {
		savefile->WriteInt( reel1[i] );
		savefile->WriteInt( reel2[i] );
		savefile->WriteInt( reel3[i] );
	}

	savefile->WriteFloat( reelPos1 );
	savefile->WriteFloat( reelPos2 );
	savefile->WriteFloat( reelPos3 );

	savefile->WriteFloat( reelRate1 );
	savefile->WriteFloat( reelRate2 );
	savefile->WriteFloat( reelRate3 );
	savefile->WriteBool( bSpinning );
	savefile->WriteBool( bCanSpin );
	savefile->WriteBool( bCanIncBet );
	savefile->WriteBool( bCanDecBet );
}

void hhSlots::Restore( idRestoreGame *savefile ) {
	int i;
	savefile->ReadInt( Bet );
	savefile->ReadInt( PlayerBet );
	savefile->ReadInt( PlayerCredits );
	savefile->ReadInt( result );
	savefile->ReadInt( creditsWon );
	savefile->ReadInt( victoryAmount );
	for (i=0; i<NUM_FRUITS; i++) {
		savefile->ReadString( fruitTextures[i] );
	}

	for (i=0; i<SLOTS_IN_REEL; i++) {
		savefile->ReadInt( (int&)reel1[i] );
		savefile->ReadInt( (int&)reel2[i] );
		savefile->ReadInt( (int&)reel3[i] );
	}

	savefile->ReadFloat( reelPos1 );
	savefile->ReadFloat( reelPos2 );
	savefile->ReadFloat( reelPos3 );

	savefile->ReadFloat( reelRate1 );
	savefile->ReadFloat( reelRate2 );
	savefile->ReadFloat( reelRate3 );
	savefile->ReadBool( bSpinning );
	savefile->ReadBool( bCanSpin );
	savefile->ReadBool( bCanIncBet );
	savefile->ReadBool( bCanDecBet );
}

void hhSlots::Spin() {
	bSpinning = true;
	Bet = PlayerBet;
	bCanSpin = bCanIncBet = bCanDecBet = false;
	reelRate1 = 2000+gameLocal.random.RandomFloat()*100;
	reelRate2 = 3000+gameLocal.random.RandomFloat()*100;
	reelRate3 = 4000+gameLocal.random.RandomFloat()*100;
	result = SLOTRESULT_NONE;
	creditsWon = 0;
}

void hhSlots::IncBet() {
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

void hhSlots::DecBet() {
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

int MaskForFruit(fruit_t fruit) {
	int mask = -1;
	switch(fruit) {
		case FRUIT_CHERRY:		mask = MASK_CHERRY;			break;
		case FRUIT_ORANGE:		mask = MASK_ORANGE;			break;
		case FRUIT_LEMON:		mask = MASK_LEMON;			break;
		case FRUIT_APPLE:		mask = MASK_APPLE;			break;
		case FRUIT_GRAPE:		mask = MASK_GRAPE;			break;
		case FRUIT_MELON:		mask = MASK_MELON;			break;
		case FRUIT_BAR:			mask = MASK_BAR;			break;
		case FRUIT_BARBAR:		mask = MASK_BARBAR;			break;
		case FRUIT_BARBARBAR:	mask = MASK_BARBARBAR;		break;
	}
	assert(mask != -1);
	return mask;
}

int ReelPos2Slot(int rpos) {
	return ((rpos + REEL_LENGTH) % REEL_LENGTH) / SLOT_HEIGHT;
}

int ReelPos2SlotPos(int rpos) {
	return ((rpos + REEL_LENGTH) % REEL_LENGTH) % SLOT_HEIGHT;
}


void hhSlots::UpdateView() {
	bool bGameOver = false;
	idUserInterface *gui = renderEntity.gui[0];

	if (gui) {
		if (PlayerCredits <= 0) {
			bCanIncBet = bCanDecBet = bCanSpin = false;
			bGameOver = true;
		}

		gui->SetStateBool("bgameover", bGameOver);
		gui->SetStateBool("bcanincbet", bCanIncBet);
		gui->SetStateBool("bcandecbet", bCanDecBet);
		gui->SetStateBool("bcanspin", bCanSpin);
		gui->SetStateInt("currentbet", PlayerBet);
		gui->SetStateInt("credits", PlayerCredits);
		gui->SetStateInt("result", result);
		gui->SetStateInt("creditswon", creditsWon);

		gui->SetStateInt("reel1pos", (int)reelPos1);
		gui->SetStateInt("reel2pos", (int)reelPos2);
		gui->SetStateInt("reel3pos", (int)reelPos3);

		gui->SetStateInt("reel1rate", (int)reelRate1);
		gui->SetStateInt("reel2rate", (int)reelRate2);
		gui->SetStateInt("reel3rate", (int)reelRate3);

		// Clear
		int fruit1, fruit2, fruit3;

		// Reel 1
		fruit1 = reel1[ ReelPos2Slot( reelPos1-SLOT_HEIGHT	)];
		fruit2 = reel1[ ReelPos2Slot( reelPos1				)];
		fruit3 = reel1[ ReelPos2Slot( reelPos1+SLOT_HEIGHT	)];
		gui->SetStateString("r1s1_texture", fruitTextures[fruit1]);
		gui->SetStateString("r1s2_texture", fruitTextures[fruit2]);
		gui->SetStateString("r1s3_texture", fruitTextures[fruit3]);

		// Reel 2
		fruit1 = reel2[ ReelPos2Slot( reelPos2-SLOT_HEIGHT	)];
		fruit2 = reel2[ ReelPos2Slot( reelPos2				)];
		fruit3 = reel2[ ReelPos2Slot( reelPos2+SLOT_HEIGHT	)];
		gui->SetStateString("r2s1_texture", fruitTextures[fruit1]);
		gui->SetStateString("r2s2_texture", fruitTextures[fruit2]);
		gui->SetStateString("r2s3_texture", fruitTextures[fruit3]);

		// Reel 3
		fruit1 = reel3[ ReelPos2Slot( reelPos3-SLOT_HEIGHT	)];
		fruit2 = reel3[ ReelPos2Slot( reelPos3				)];
		fruit3 = reel3[ ReelPos2Slot( reelPos3+SLOT_HEIGHT	)];
		gui->SetStateString("r3s1_texture", fruitTextures[fruit1]);
		gui->SetStateString("r3s2_texture", fruitTextures[fruit2]);
		gui->SetStateString("r3s3_texture", fruitTextures[fruit3]);

		gui->StateChanged(gameLocal.time, true);
		CallNamedEvent("Update");
	}
}

void hhSlots::CheckVictory() {

	#define NUM_VICTORIES	16
	static victory_t victoryTable[NUM_VICTORIES] = {
		{	MASK_BARBARBAR,		MASK_BARBARBAR,		MASK_BARBARBAR,		10000},
		{	MASK_BARBAR,		MASK_BARBAR,		MASK_BARBAR,		1000},
		{	MASK_BAR,			MASK_BAR,			MASK_BAR,			500},
		{	MASK_ANYBAR,		MASK_ANYBAR,		MASK_ANYBAR,		100},

		{	MASK_MELON,			MASK_MELON,			MASK_MELON,			60},
		{	MASK_GRAPE,			MASK_GRAPE,			MASK_GRAPE,			50},
		{	MASK_APPLE,			MASK_APPLE,			MASK_APPLE,			40},
		{	MASK_LEMON,			MASK_LEMON,			MASK_LEMON,			30},
		{	MASK_ORANGE,		MASK_ORANGE,		MASK_ORANGE,		20},
		{	MASK_CHERRY,		MASK_CHERRY,		MASK_CHERRY,		10},

		{	MASK_CHERRY,		MASK_CHERRY,		MASK_ANY,			5},
		{	MASK_ANY,			MASK_CHERRY,		MASK_CHERRY,		5},
		{	MASK_CHERRY,		MASK_ANY,			MASK_CHERRY,		5},

		{	MASK_CHERRY,		MASK_ANY,			MASK_ANY,			2},
		{	MASK_ANY,			MASK_CHERRY,		MASK_ANY,			2},
		{	MASK_ANY,			MASK_ANY,			MASK_CHERRY,		2}
	};

	PlayerCredits -= Bet;
	result = SLOTRESULT_LOSE;
	creditsWon = 0;
	for (int ix=0; ix<NUM_VICTORIES; ix++) {
		int fruitmask1 = MaskForFruit(reel1[ReelPos2Slot(reelPos1)]);
		int fruitmask2 = MaskForFruit(reel2[ReelPos2Slot(reelPos2)]);
		int fruitmask3 = MaskForFruit(reel3[ReelPos2Slot(reelPos3)]);

		if ((fruitmask1 & victoryTable[ix].f1) &&
			(fruitmask2 & victoryTable[ix].f2) &&
			(fruitmask3 & victoryTable[ix].f3) ) {

			result = SLOTRESULT_WIN;
			creditsWon = Bet * victoryTable[ix].payoff;
			PlayerCredits += creditsWon;
			PlayerCredits = idMath::ClampInt(0, 999999999, PlayerCredits);

			// Play victory sound
			if (victoryAmount && PlayerCredits >= victoryAmount) {
				StartSound( "snd_victory", SND_CHANNEL_ANY );
				ActivateTargets( gameLocal.GetLocalPlayer() );
				victoryAmount = 0;
			}
			else if (victoryTable[ix].payoff > 5) {
				StartSound( "snd_winbig", SND_CHANNEL_ANY );
			}
			else {
				StartSound( "snd_win", SND_CHANNEL_ANY );
			}
			break;
		}
	}

	PlayerBet = idMath::ClampInt(0, PlayerCredits, PlayerBet);
}


bool hhSlots::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("spin") == 0) {
		BecomeActive(TH_MISC3);
		StartSound( "snd_spin", SND_CHANNEL_ANY );
		Spin();
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
		bCanSpin   = 1;
		bCanIncBet = 1;
		bCanDecBet = 1;
		PlayerCredits = spawnArgs.GetInt("credits");
		Bet = PlayerBet = 1;
		UpdateView();
	}
	else {
		src->UnreadToken(&token);
		return false;
	}

	return true;
}

void hhSlots::Think() {
	hhConsole::Think();

	if (thinkFlags & TH_MISC3) {
		if (bSpinning) {

			float deltaTime = MS2SEC(gameLocal.msec);
			if (reelRate1 > 0.0f) {
				reelPos1 = ((int)(reelPos1 - reelRate1 * deltaTime) + REEL_LENGTH) % REEL_LENGTH;
				reelRate1 *= 0.98f;
				if (reelRate1 < MINIMUM_REEL_RATE) {
					reelRate1 = 0.0f;
					StartSound( "snd_stop", SND_CHANNEL_ANY );
				}
			}
			if (reelRate2 > 0.0f) {
				reelPos2 = ((int)(reelPos2 - reelRate2 * deltaTime) + REEL_LENGTH) % REEL_LENGTH;
				reelRate2 *= 0.98f;
				if (reelRate2 < MINIMUM_REEL_RATE) {
					reelRate2 = 0.0f;
					StartSound( "snd_stop", SND_CHANNEL_ANY );
				}
			}
			if (reelRate3 > 0.0f) {
				reelPos3 = ((int)(reelPos3 - reelRate3 * deltaTime) + REEL_LENGTH) % REEL_LENGTH;
				reelRate3 *= 0.98f;
				if (reelRate3 < MINIMUM_REEL_RATE) {
					reelRate3 = 0.0f;
					StartSound( "snd_stop", SND_CHANNEL_ANY );
				}
			}

			if (reelRate1==0.0f && reelRate2==0.0f && reelRate3==0.0f) {
				CheckVictory();
				bSpinning = false;
				bCanSpin = bCanIncBet = bCanDecBet = true;
				BecomeInactive(TH_MISC3);
			}

			UpdateView();
		}
	}
}

void hhSlots::Event_Spin() {
	Spin();
}
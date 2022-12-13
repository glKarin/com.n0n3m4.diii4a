#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define DEFAULT_HIGH_SCORE			76535
#define DEFAULT_HIGH_SCORE_NAME		"Teacher"
#define FRUIT_DELAY					15000

const idEventDef EV_GameStart("<pmgamestart>", NULL);
const idEventDef EV_PlayerRespawn("<pmplayerspawn>", NULL);
const idEventDef EV_MonsterRespawn("<pmmonsterspawn>", "d");
const idEventDef EV_ToggleFruit("<pmtogglefruit>", NULL);
const idEventDef EV_GameOver("<pmgameover>", NULL);
const idEventDef EV_NextMap("<pmnextmap>", NULL);

CLASS_DECLARATION(hhConsole, hhArcadeGame)
	EVENT( EV_GameStart,			hhArcadeGame::Event_GameStart)
	EVENT( EV_PlayerRespawn,		hhArcadeGame::Event_PlayerRespawn)
	EVENT( EV_MonsterRespawn,		hhArcadeGame::Event_MonsterRespawn)
	EVENT( EV_ToggleFruit,			hhArcadeGame::Event_ToggleFruit)
	EVENT( EV_GameOver,				hhArcadeGame::Event_GameOver)
	EVENT( EV_NextMap,				hhArcadeGame::Event_NextMap)
	EVENT( EV_CallGuiEvent,			hhArcadeGame::Event_CallGuiEvent)
END_CLASS


hhArcadeGame::hhArcadeGame() {
}

hhArcadeGame::~hhArcadeGame() {
}

void hhArcadeGame::Spawn() {
	highscore = DEFAULT_HIGH_SCORE;
	highscoreName = DEFAULT_HIGH_SCORE_NAME;

	GetMazeForLevel(1);
	victoryAmount = spawnArgs.GetInt("victory");
	bPlayerMoving = false;
	playerMove.Set(1,0);
	bGameOver = true;
	ResetMap();
	ResetPlayerAndMonsters();
	ResetGame();
	UpdateView();
}

void hhArcadeGame::GetMazeForLevel(int lev) {
	numMazes = idMath::ClampInt(1, 999, spawnArgs.GetInt("numMazes"));
	const char *maze = spawnArgs.GetString(va("maze%d", (lev-1)%numMazes+1));
	assert(idStr::Length(maze) == (ARCADE_GRID_WIDTH*ARCADE_GRID_HEIGHT));
	memset(startingGrid, 0, ARCADE_GRID_WIDTH*ARCADE_GRID_HEIGHT);
	for (int y=0; y<ARCADE_GRID_HEIGHT; y++) {
		for (int x=0; x<ARCADE_GRID_WIDTH; x++) {
			startingGrid[x][y] = maze[y*ARCADE_GRID_WIDTH+x] - '0';
		}
	}
}

void hhArcadeGame::Save(idSaveGame *savefile) const {
	int i,j;
	savefile->Write(startingGrid, ARCADE_GRID_WIDTH*ARCADE_GRID_HEIGHT);
	for (i=0; i<ARCADE_GRID_WIDTH; i++) {
		for (j=0; j<ARCADE_GRID_HEIGHT; j++) {
			grid[i][j].Save(savefile);
		}
	}
	for (i=0; i<4; i++) {
		monsters[i].Save(savefile);
	}
	player.Save(savefile);
	fruit.Save(savefile);
	savefile->WriteVec2(playerMove);
	savefile->WriteBool(bPlayerMoving);
	savefile->WriteBool(bSimulating);
	savefile->WriteBool(bGameOver);
	savefile->WriteBool(bPoweredUp);
	savefile->WriteInt(score);
	savefile->WriteInt(lives);
	savefile->WriteInt(level);
	savefile->WriteInt(nextScoreBoost);
	savefile->WriteInt(nextPelletTime);
	savefile->WriteInt(victoryAmount);
	savefile->WriteInt(numMazes);
	savefile->WriteInt(highscore);
	savefile->WriteString(highscoreName);
}

void hhArcadeGame::Restore( idRestoreGame *savefile ) {
	int i,j;
	savefile->Read(startingGrid, ARCADE_GRID_WIDTH*ARCADE_GRID_HEIGHT);
	for (i=0; i<ARCADE_GRID_WIDTH; i++) {
		for (j=0; j<ARCADE_GRID_HEIGHT; j++) {
			grid[i][j].Restore(savefile);
		}
	}
	for (i=0; i<4; i++) {
		monsters[i].Restore(savefile);
	}
	player.Restore(savefile);
	fruit.Restore(savefile);
	savefile->ReadVec2(playerMove);
	savefile->ReadBool(bPlayerMoving);
	savefile->ReadBool(bSimulating);
	savefile->ReadBool(bGameOver);
	savefile->ReadBool(bPoweredUp);
	savefile->ReadInt(score);
	savefile->ReadInt(lives);
	savefile->ReadInt(level);
	savefile->ReadInt(nextScoreBoost);
	savefile->ReadInt(nextPelletTime);
	savefile->ReadInt(victoryAmount);
	savefile->ReadInt(numMazes);
	savefile->ReadInt(highscore);
	savefile->ReadString(highscoreName);
}

void hhArcadeGame::ConsoleActivated() {
	BecomeActive(TH_MISC3);
}

void hhArcadeGame::StartGame() {
	if (bGameOver) {
		ResetMap();
		ResetPlayerAndMonsters();
		StartSound("snd_intro", SND_CHANNEL_ANY, 0, true, NULL);
		ResetGame();
		StartSound("snd_coin", SND_CHANNEL_ANY, 0, true, NULL);
		PostEventMS(&EV_GameStart, 2000);
		bGameOver = false;
		UpdateView();
	}
}

void hhArcadeGame::FindType(int type, int &xOut, int &yOut, int skipX, int skipY) {
	xOut = 0;
	yOut = 0;
	for (int x=0; x<ARCADE_GRID_WIDTH; x++) {
		for (int y=0; y<ARCADE_GRID_HEIGHT; y++) {
			if (x != skipX || y != skipY) {
				if (grid[x][y].GetType() == type) {
					xOut = x;
					yOut = y;
					return;
				}
			}
		}
	}
}

void hhArcadeGame::ResetMap() {
	int x, y;

	// Make a copy of the grid that we can modify it, transposing in the process
	for (x=0; x<ARCADE_GRID_WIDTH; x++) {
		for (y=0; y<ARCADE_GRID_HEIGHT; y++) {
			grid[x][y].SetType(startingGrid[x][y]);
			grid[x][y].SetVisible(true);
		}
	}

	FindType(ARCADE_FRUIT, x, y);
	fruit.Set(x, y, false);
}

void hhArcadeGame::ResetPlayerAndMonsters() {
	int x, y;

	FindType(ARCADE_PLAYERSTART, x, y);
	player.moveDelay = 200;
	player.nextMoveTime = gameLocal.time;
	player.isPlayer = true;
	player.Set(x, y);
	player.dying = false;
	bPoweredUp = false;
	StopSound(SND_CHANNEL_ITEM, true);

	FindType(ARCADE_MONSTERSTART, x, y);
	monsters[0].Set(x, y);
	monsters[1].Set(x, y);
	monsters[2].Set(x, y);
	monsters[3].Set(x, y);
	for (int ix=0; ix<4; ix++) {
		monsters[ix].SetVisible(true);
		monsters[ix].moveDelay = 300;
		monsters[ix].nextMoveTime = gameLocal.time;
		monsters[ix].vulnerableTimeRemaining = 0;
	}

}

void hhArcadeGame::ResetGame() {
	level = 1;
	score = 0;
	lives = 3;
	nextScoreBoost = 10000;
	bSimulating = false;
	nextPelletTime = gameLocal.time;
	bPoweredUp = false;
}

idVec2 hhArcadeGame::SmoothMovement(MovingGamePiece &piece) {
	idVec2 curPos;
	idVec2 lastGuiPos;
	idVec2 lastGuiMove;
	curPos.x = piece.x;
	curPos.y = piece.y;
	lastGuiPos.x = (curPos.x - piece.lastMove.x) * ARCADE_CELL_WIDTH;
	lastGuiPos.y = (curPos.y - piece.lastMove.y) * ARCADE_CELL_HEIGHT;
	lastGuiMove.x = piece.lastMove.x * ARCADE_CELL_WIDTH;
	lastGuiMove.y = piece.lastMove.y * ARCADE_CELL_HEIGHT;

	float alpha = (gameLocal.time - piece.lastMoveTime) / (float)piece.moveDelay;
	alpha = idMath::ClampFloat(0.0f, 1.0f, alpha);
	return lastGuiPos + lastGuiMove * alpha;
}

void hhArcadeGame::UpdateView() {
	int ix;
	idVec2 guiPos;
	idUserInterface *gui = renderEntity.gui[0];

	if (gui) {

		if (player.Changed()) {
			guiPos = SmoothMovement(player);
			gui->SetStateInt("player_x", guiPos.x);
			gui->SetStateInt("player_y", guiPos.y);
			gui->SetStateBool("player_vis", player.IsVisible());
			gui->SetStateBool("player_moving", bPlayerMoving);
			gui->SetStateFloat("player_angle", player.angle);
			gui->SetStateBool("player_dying", player.dying);
		}

		for (ix=0; ix<4; ix++) {
			if (monsters[ix].Changed()) {
				guiPos = SmoothMovement(monsters[ix]);
				gui->SetStateInt(va("monster%d_x", ix+1), guiPos.x);
				gui->SetStateInt(va("monster%d_y", ix+1), guiPos.y);
				gui->SetStateBool(va("monster%d_vis", ix+1), monsters[ix].IsVisible());
				gui->SetStateInt(va("monster%d_pu", ix+1), monsters[ix].vulnerableTimeRemaining);
				gui->SetStateFloat(va("monster%d_angle", ix+1), monsters[ix].angle);
			}
		}

		// Traverse grid, telling gui about state of the pellets and powerups
		int curPowerup = 1;
		int curPellet = 1;
		for (int x=0; x<ARCADE_GRID_WIDTH; x++) {
			for (int y=0; y<ARCADE_GRID_HEIGHT; y++) {
				if (grid[x][y].Changed()) {
					if (grid[x][y].GetType() == ARCADE_PELLET) {
						gui->SetStateInt(va("pellet%d_x", curPellet), x * ARCADE_CELL_WIDTH);
						gui->SetStateInt(va("pellet%d_y", curPellet), y * ARCADE_CELL_HEIGHT);
						gui->SetStateBool(va("pellet%d_vis", curPellet), grid[x][y].IsVisible());
						curPellet++;
					}
					else if (grid[x][y].GetType() == ARCADE_POWERUP) {
						gui->SetStateInt(va("powerup%d_x", curPowerup), x * ARCADE_CELL_WIDTH);
						gui->SetStateInt(va("powerup%d_y", curPowerup), y * ARCADE_CELL_HEIGHT);
						gui->SetStateBool(va("powerup%d_vis", curPowerup), grid[x][y].IsVisible());
						curPowerup++;
					}
				}
			}
		}

		if (fruit.Changed()) {
			gui->SetStateInt("fruit_x", fruit.x * ARCADE_CELL_WIDTH);
			gui->SetStateInt("fruit_y", fruit.y * ARCADE_CELL_HEIGHT);
			gui->SetStateBool("fruit_vis", fruit.IsVisible());
		}

		if (score > highscore) {
			highscore = score;
			highscoreName = cvarSystem->GetCVarString("ui_name");
		}

		gui->SetStateInt("maze", (level-1)%numMazes+1);
		gui->SetStateInt("level", level);
		gui->SetStateInt("score", score);
		gui->SetStateInt("lives", lives);
		gui->SetStateInt("highscore", highscore);
		gui->SetStateString("highscorename", highscoreName.c_str());
		gui->SetStateBool("gameover", bGameOver);
		gui->StateChanged(gameLocal.time, true);
	}
}

bool hhArcadeGame::HandleSingleGuiCommand(idEntity *entityGui, idLexer *src) {

	idToken token;

	if (!src->ReadToken(&token)) {
		return false;
	}

	if (token == ";") {
		return false;
	}

	if (token.Icmp("startgame") == 0) {
		// Trigger this entity, used for tip system
		idEntity *agTrig = gameLocal.FindEntity( "arcadegame_used" );
		if( agTrig ) {
			agTrig->PostEventMS( &EV_Activate, 0, this );
		}

		BecomeActive(TH_MISC3);
		StartGame();
	}
	else if (token.Icmp("reset") == 0) {
		BecomeInactive(TH_MISC3);
		bSimulating = false;
		bGameOver = true;
		bPlayerMoving = false;
		playerMove.Set(1,0);
		ResetMap();
		ResetPlayerAndMonsters();
		ResetGame();
		highscore = 1000;
		highscoreName = DEFAULT_HIGH_SCORE_NAME;

		UpdateView();
	}
	else {
		src->UnreadToken(&token);
		return false;
	}

	return true;
}

void hhArcadeGame::PlayerControls(usercmd_t *cmd) {
	if (bGameOver && (cmd->buttons & BUTTON_ATTACK)) {
		StartGame();
	}

	// These come from the player every tick
	if (cmd->rightmove != 0 || cmd->forwardmove != 0) {
		playerMove.x = cmd->rightmove > 0 ? 1 : cmd->rightmove < 0 ? -1 : 0;
		playerMove.y = cmd->forwardmove < 0 ? 1 : cmd->forwardmove > 0 ? -1 : 0;
	}
}

idVec2 MoveForDirection(int dir) {
	switch(dir) {
		case PAC_MOVE_LEFT:		return idVec2(-1, 0);
		case PAC_MOVE_RIGHT:	return idVec2(1, 0);
		case PAC_MOVE_UP:		return idVec2(0, -1);
		case PAC_MOVE_DOWN:		return idVec2(0, 1);
	}
	return idVec2(0,0);
}

bool hhArcadeGame::MoveIsValid(idVec2 &move, MovingGamePiece &piece) {
	int newX = piece.x + move.x;
	int newY = piece.y + move.y;
	int cell = grid[newX][newY].GetType();

	if (cell == ARCADE_CLIPPLAYER) {
		// Hack: disallow reentry into ghost cage.  Assumes the monster start is adjacent to the playerclip
		int sourceCell = grid[piece.x][piece.y].GetType();
		if (sourceCell != ARCADE_MONSTERSTART) {
			return false;
		}
	}
	return (cell != ARCADE_SOLID && (cell != ARCADE_CLIPPLAYER || !piece.isPlayer ) );
}

void hhArcadeGame::GrowPellets() {
	if (!bSimulating) {
		return;
	}

	int growth = 0;
	int i, j;
	int x, y;
	int startX = gameLocal.random.RandomInt(ARCADE_GRID_WIDTH);
	int startY = gameLocal.random.RandomInt(ARCADE_GRID_HEIGHT);

	StartSound("snd_grow", SND_CHANNEL_ANY, 0, true, NULL);

	for (i=0; i<ARCADE_GRID_WIDTH; i++) {
		for (j=0; j<ARCADE_GRID_HEIGHT; j++) {
			x = (startX + i) % ARCADE_GRID_WIDTH;
			y = (startY + j) % ARCADE_GRID_HEIGHT;
			// For each valid pellet, make all adjacent pellets visible
			if (grid[x][y].GetType() == ARCADE_PELLET && grid[x][y].IsVisible()) {
				if (grid[x-1][y].GetType() == ARCADE_PELLET && !grid[x-1][y].IsVisible()) {
					grid[x-1][y].SetVisible(true);
					growth++;
				}
				if (grid[x+1][y].GetType() == ARCADE_PELLET && !grid[x+1][y].IsVisible()) {
					grid[x+1][y].SetVisible(true);
					growth++;
				}
				if (grid[x][y-1].GetType() == ARCADE_PELLET && !grid[x][y-1].IsVisible()) {
					grid[x][y-1].SetVisible(true);
					growth++;
				}
				if (grid[x][y+1].GetType() == ARCADE_PELLET && !grid[x][y+1].IsVisible()) {
					grid[x][y+1].SetVisible(true);
					growth++;
				}

				if (growth > 4) {
					return;
				}
			}
		}
	}
}

void hhArcadeGame::CheckForCollisions(MovingGamePiece &piece) {
	int x = piece.x;
	int y = piece.y;
	int ix;
	int type;
	int destX, destY;

	switch(grid[x][y].GetType()) {
		case ARCADE_TELEPORT:
			FindType(ARCADE_TELEPORT, destX, destY, x, y);
			piece.Set(destX, destY);
			break;

		case ARCADE_PELLET:
			if (piece.isPlayer && grid[x][y].IsVisible()) {
				grid[x][y].SetVisible(false);
				AddScore(1);
				StartSound("snd_pellet", SND_CHANNEL_ANY, 0, true, NULL);
			}
			break;

		case ARCADE_POWERUP:
			if (piece.isPlayer && grid[x][y].IsVisible()) {
				AddScore(50);
				grid[x][y].SetVisible(false);
				monsters[0].vulnerableTimeRemaining += 5000 / level;
				monsters[1].vulnerableTimeRemaining += 5000 / level;
				monsters[2].vulnerableTimeRemaining += 5000 / level;
				monsters[3].vulnerableTimeRemaining += 5000 / level;
				bPoweredUp = true;
				StopSound(SND_CHANNEL_ITEM, true);
				StartSound("snd_powerup", SND_CHANNEL_ANY, 0, true, NULL);
				StartSound("snd_poweruploop", SND_CHANNEL_ITEM, 0, true, NULL);
			}
			break;
	}

	if (piece.isPlayer) {
		if (fruit.IsVisible() && piece.x==fruit.x && piece.y==fruit.y) {
			fruit.SetVisible(false);
			AddScore(1000*level);
			StartSound("snd_fruit", SND_CHANNEL_ANY, 0, true, NULL);
			CancelEvents(&EV_ToggleFruit);
			PostEventMS(&EV_ToggleFruit, FRUIT_DELAY);
		}
	}

	// player / monster collisions
	for (ix=0; ix<4; ix++) {
		if (monsters[ix].IsVisible() && player.x==monsters[ix].x && player.y==monsters[ix].y) {
			if (monsters[ix].vulnerableTimeRemaining > 0) {
				monsters[ix].SetVisible(false);
				StartSound("snd_eatmonster", SND_CHANNEL_ANY, 0, true, NULL);
				AddScore(500);
				PostEventMS(&EV_MonsterRespawn, 3000, ix);
			}
			else if (!gameLocal.GetLocalPlayer()->godmode) {
				bSimulating = false;
				player.SetVisible(false);
				player.dying = true;
				StartSound("snd_die", SND_CHANNEL_ANY, 0, true, NULL);
				if (lives <= 0) {
					PostEventMS(&EV_GameOver, 0);
				}
				else {
					lives--;
					PostEventMS(&EV_PlayerRespawn, 2000);
				}
				return;
			}
		}
	}

	// Check for life bonus
	if (score >= nextScoreBoost) {
		lives++;
		nextScoreBoost *= 2;
		StartSound("snd_lifeboost", SND_CHANNEL_ANY, 0, true, NULL);
	}

	// Check for level completion
	if (piece.isPlayer) {
		bool levelCompleted = true;
		for (x=0; levelCompleted && x<ARCADE_GRID_WIDTH; x++) {
			for (y=0; levelCompleted && y<ARCADE_GRID_HEIGHT; y++) {
				type = grid[x][y].GetType();
				if (grid[x][y].IsVisible() && (type == ARCADE_PELLET || type == ARCADE_POWERUP)) {
					levelCompleted = false;
				}
			}
		}

		if (levelCompleted) {
			AddScore(2000);
			bSimulating = false;
			if (victoryAmount > 0 && (level+1 >= victoryAmount)) {
				ActivateTargets( gameLocal.GetLocalPlayer() );
				StartSound( "snd_victory", SND_CHANNEL_ANY );
				victoryAmount = 0;
			}
			StartSound("snd_leveldone", SND_CHANNEL_ANY, 0, true, NULL);
			PostEventMS(&EV_NextMap, 2000);
		}
	}
}

bool hhArcadeGame::DoMove(idVec2 &move, MovingGamePiece &piece) {

	bool bMoved = move.x || move.y;
	piece.Move(move);

	if (piece.isPlayer) {
		// Acount for player movement direction
		bPlayerMoving = bMoved;
	}

	CheckForCollisions(piece);

	return bMoved;
}

void hhArcadeGame::DoPlayerMove() {
	bPlayerMoving = false;
	if (!bSimulating) {
		return;
	}

	bool bValidXMove = playerMove.x && MoveIsValid(idVec2(playerMove.x, 0), player);
	bool bValidYMove = playerMove.y && MoveIsValid(idVec2(0, playerMove.y), player);

	// Allow move direction to alternate between X and Y
	if (player.lastMove.x) {
		if (bValidYMove) {
			DoMove(idVec2(0, playerMove.y), player);
		}
		else if (bValidXMove) {
			DoMove(idVec2(playerMove.x, 0), player);
		}
		else if (MoveIsValid(player.lastMove, player)) {
			DoMove(player.lastMove, player);
		}
	}
	else {
		if (bValidXMove) {
			DoMove(idVec2(playerMove.x, 0), player);
		}
		else if (bValidYMove) {
			DoMove(idVec2(0, playerMove.y), player);
		}
		else if (MoveIsValid(player.lastMove, player)) {
			DoMove(player.lastMove, player);
		}
	}
}

void hhArcadeGame::DoMonsterAI(MovingGamePiece &monster, int index) {
	if (!bSimulating) {
		return;
	}

	if (monster.vulnerableTimeRemaining > 0) {
		monster.vulnerableTimeRemaining -= monster.moveDelay;
		monster.vulnerableTimeRemaining = idMath::ClampInt(0, 100000, monster.vulnerableTimeRemaining);
	}

	idVec2 move;
	idVec2 toPlayer;
	idVec2 toPlayerMove;
	int playerDirX, playerDirY;
	toPlayer.Set(player.x - monster.x, player.y - monster.y);
	playerDirX = toPlayer.x < 0 ? PAC_MOVE_LEFT : PAC_MOVE_RIGHT;
	playerDirY = toPlayer.y < 0 ? PAC_MOVE_UP   : PAC_MOVE_DOWN;
	toPlayerMove.Set(
		toPlayer.x > 0 ? 1 : toPlayer.x < 0 ? -1 : 0,
		toPlayer.y > 0 ? 1 : toPlayer.y < 0 ? -1 : 0
		);
	idVec2 illegalMove = -monster.lastMove;
	bool bGoTowardPlayer = gameLocal.random.RandomInt(4) >= index;
	int choice;
	int startingChoice = index;	// Makes each monster unique
	int j;

	// Keep moving in direction of last move until there is an intersection
	// At intersection, choose between one of the directions, favoring one going towards
	// the player.  Never go back in the direction you came from unless at a dead end.

	// Go towards player if possible
	if (bGoTowardPlayer) {
		move = MoveForDirection(playerDirX);
		if (MoveIsValid(move, monster) && move != illegalMove) {
			DoMove(move, monster);
			return;
		}
		move = MoveForDirection(playerDirY);
		if (MoveIsValid(move, monster) && move != illegalMove) {
			DoMove(move, monster);
			return;
		}
	}

	// Otherwise move in one of the other valid directions
	for (j=0; j<4; j++) {
		choice = (startingChoice + j) % 4;
		move = MoveForDirection(choice);
		if (MoveIsValid(move, monster) && move != illegalMove) {
			DoMove(move, monster);
			return;
		}
	}

	// If still haven't found a valid move, must be at a dead end, now allow illegalMove
	for (j=0; j<4; j++) {
		choice = (startingChoice + j) % 4;
		move = MoveForDirection(choice);
		if (MoveIsValid(move, monster)) {
			DoMove(move, monster);
			return;
		}
	}
}

void hhArcadeGame::Think() {
	hhConsole::Think();

	if (thinkFlags & TH_MISC3) {
		if (bSimulating) {
			// Player
			if (gameLocal.time > player.nextMoveTime) {
				player.nextMoveTime = gameLocal.time + player.moveDelay;
				if (playerMove.LengthSqr() > 0.0f) {
					DoPlayerMove();
				}
			}

			// AI
			for (int ix=0; ix<4; ix++) {
				if (gameLocal.time > monsters[ix].nextMoveTime) {
					monsters[ix].nextMoveTime = gameLocal.time + monsters[ix].moveDelay;
					DoMonsterAI(monsters[ix], ix);
				}
			}

			// See if power up sound should end
			if (bPoweredUp) {
				bPoweredUp = false;
				for (int ix=0; ix<4; ix++) {
					if (monsters[ix].vulnerableTimeRemaining > 0) {
						bPoweredUp = true;
						break;
					}
				}
				if (!bPoweredUp) {
					StopSound(SND_CHANNEL_ITEM, true);
				}
			}

			if (gameLocal.time > nextPelletTime) {
				nextPelletTime = gameLocal.time + 3500;
				//GrowPellets();
			}

			UpdateView();
		}
	}
}

void hhArcadeGame::AddScore(int amount) {
	if (!gameLocal.GetLocalPlayer()->godmode) {
		score += amount;
	}
}

void hhArcadeGame::Event_GameStart() {
	bSimulating = true;
	PostEventMS(&EV_ToggleFruit, FRUIT_DELAY);
}

void hhArcadeGame::Event_GameOver() {		// Player ran out of lives
	CancelEvents(&EV_ToggleFruit);
	StopSound(SND_CHANNEL_ITEM, true);
	bSimulating = false;
	bGameOver = true;
	CallNamedEvent("gameover");
	UpdateView();
}

void hhArcadeGame::Event_NextMap() {		// Transition to next map
	CancelEvents(&EV_ToggleFruit);
	StopSound(SND_CHANNEL_ITEM, true);
	GetMazeForLevel(++level);
	ResetMap();
	ResetPlayerAndMonsters();
	StartSound("snd_intro", SND_CHANNEL_ANY, 0, true, NULL);
	PostEventMS(&EV_GameStart, 2000);
	UpdateView();
}

void hhArcadeGame::Event_PlayerRespawn() {	// Player died, respawn
	CancelEvents(&EV_ToggleFruit);
	ResetPlayerAndMonsters();
	StartSound("snd_intro", SND_CHANNEL_ANY, 0, true, NULL);
	player.SetVisible(true);
	player.dying = false;
	PostEventMS(&EV_GameStart, 3000);
	UpdateView();
}

void hhArcadeGame::Event_MonsterRespawn(int index) {	// Monster died, respawn
	int x,y;
	FindType(ARCADE_MONSTERSTART, x, y);
	monsters[index].SetVisible(true);
	monsters[index].vulnerableTimeRemaining = 0;
	monsters[index].Set(x,y);
	monsters[index].nextMoveTime = gameLocal.time + 3000;
}

void hhArcadeGame::Event_ToggleFruit() {
	if (fruit.IsVisible()) {
		fruit.SetVisible(false);
		PostEventMS(&EV_ToggleFruit, FRUIT_DELAY);
	}
	else {
		fruit.SetVisible(true);
		PostEventMS(&EV_ToggleFruit, 5000);
	}
}

void hhArcadeGame::Event_CallGuiEvent(const char *eventName) {

	if (!idStr::Icmp(eventName, "turnoff")) {
		// Release any players routing commands to us
		for( int i = 0; i < gameLocal.numClients; i++ ) {
			if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType(hhPlayer::Type) ) {
				hhPlayer *player = static_cast<hhPlayer*>(gameLocal.entities[i]);
				if (player->guiWantsControls.IsValid() && player->guiWantsControls.GetEntity() == this) {
					player->guiWantsControls = NULL;
				}
			}
		}
	}

	CallNamedEvent(eventName);
}

void hhArcadeGame::LockedGuiReleased(hhPlayer *player) {
	idEntity *agTrig = gameLocal.FindEntity( "arcadegame_released" );
	if( agTrig ) {
		agTrig->PostEventMS( &EV_Activate, 0, this );
	}
}


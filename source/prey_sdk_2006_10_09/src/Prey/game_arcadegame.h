
#ifndef __GAME_ARCADEGAME_H__
#define __GAME_ARCADEGAME_H__

#define ARCADE_GRID_WIDTH	19
#define ARCADE_GRID_HEIGHT	21
#define ARCADE_CELL_WIDTH	20
#define ARCADE_CELL_HEIGHT	20

#define ARCADE_EMPTY		0
#define ARCADE_SOLID		1
#define ARCADE_PELLET		2
#define ARCADE_POWERUP		3
#define ARCADE_CLIPPLAYER	4
#define ARCADE_TELEPORT		5
#define ARCADE_FRUIT		6
#define ARCADE_PLAYERSTART	7
#define ARCADE_MONSTERSTART	8

enum {
	PAC_MOVE_LEFT=0,
	PAC_MOVE_RIGHT=1,
	PAC_MOVE_UP=2,
	PAC_MOVE_DOWN=3
};

class GamePiece {
public:
			GamePiece() { type = ARCADE_EMPTY; visible = true; changed = true; }
	virtual ~GamePiece() { }
	void	SetType(char t) { type = t; changed = true; }
	void	SetVisible(bool v) { visible = v; changed = true; }
	char	GetType() { return type; }
	bool	IsVisible() { return visible; }
	bool	Changed() { return changed; }
	virtual void	Save( idSaveGame *savefile ) const {
		savefile->WriteByte(type);
		savefile->WriteBool(visible);
		savefile->WriteBool(changed);
	}
	virtual void	Restore( idRestoreGame *savefile ) {
		savefile->ReadByte(type);
		savefile->ReadBool(visible);
		savefile->ReadBool(changed);
	}

protected:
	byte	type;
	bool	visible;
	bool	changed;
};

class MovingGamePiece : public GamePiece {
public:
	MovingGamePiece() {
		x = y = 0; isPlayer = false; dying = false;
		lastMove.Zero();
		lastMoveTime = 0;
		angle = 0.0f;
		moveDelay = 250;
		nextMoveTime = 0;
		vulnerableTimeRemaining = 0;
	}
	void	Set(int nx, int ny, bool nvis=true) {
		x=nx; y=ny; visible = nvis;
		lastMove.Zero();
		lastMoveTime = gameLocal.time;
		changed = true;
	}
	void	Move(idVec2 move) {
		x += move.x;
		y += move.y;
		lastMove = move;
		lastMoveTime = gameLocal.time;
		angle = 
			lastMove.x > 0 ? 0.0f :
			lastMove.x < 0 ? 180.0f :
			lastMove.y < 0 ? 90.0f :
			lastMove.y > 0 ? 270.0f : angle;

		changed = true;
	}
	virtual void	Save( idSaveGame *savefile ) const {
		GamePiece::Save(savefile);
		savefile->WriteInt(x);
		savefile->WriteInt(y);
		savefile->WriteBool(isPlayer);
		savefile->WriteFloat(angle);
		savefile->WriteBool(dying);
		savefile->WriteVec2(lastMove);
		savefile->WriteInt(lastMoveTime);
		savefile->WriteInt(moveDelay);
		savefile->WriteInt(nextMoveTime);
		savefile->WriteInt(vulnerableTimeRemaining);
	}
	virtual void	Restore( idRestoreGame *savefile ) {
		GamePiece::Restore(savefile);
		savefile->ReadInt(x);
		savefile->ReadInt(y);
		savefile->ReadBool(isPlayer);
		savefile->ReadFloat(angle);
		savefile->ReadBool(dying);
		savefile->ReadVec2(lastMove);
		savefile->ReadInt(lastMoveTime);
		savefile->ReadInt(moveDelay);
		savefile->ReadInt(nextMoveTime);
		savefile->ReadInt(vulnerableTimeRemaining);
	}

	int		x,y;
	bool	isPlayer;
	bool	dying;						// used for player only
	idVec2	lastMove;
	float	angle;
	int		lastMoveTime;
	int		moveDelay;					// Delay in MS between moves
	int		nextMoveTime;				// Next time piece is allowed to move
	int		vulnerableTimeRemaining;	// monsters only, time before powerup wears off
};


class hhArcadeGame : public hhConsole {
public:
	CLASS_PROTOTYPE( hhArcadeGame );

					hhArcadeGame();
	virtual			~hhArcadeGame();

	void			Spawn( void );
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );
	virtual void	Think();
	virtual bool	HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);
	virtual void	PlayerControls(usercmd_t *cmd);
	virtual void	ConsoleActivated();
	virtual void	LockedGuiReleased(hhPlayer *player);

	void			StartGame();
	bool			MoveIsValid(idVec2 &move, MovingGamePiece &piece);
	bool			DoMove(idVec2 &move, MovingGamePiece &piece);
	void			CheckForCollisions(MovingGamePiece &piece);
	void			ResetMap();
	void			ResetPlayerAndMonsters();
	void			ResetGame();
	void			GrowPellets();
	void			DoPlayerMove();
	void			DoMonsterAI(MovingGamePiece &monster, int index);
	idVec2			SmoothMovement(MovingGamePiece &piece);
	void			UpdateView();
	void			FindType(int type, int &x, int &y, int skipX=-1, int skipY=-1);
	void			GetMazeForLevel(int lev);
	void			AddScore(int amount);

protected:
	void			Event_GameStart();
	void			Event_GameOver();
	void			Event_PlayerRespawn();
	void			Event_MonsterRespawn(int index);
	void			Event_ToggleFruit();
	void			Event_NextMap();
	void			Event_CallGuiEvent(const char *eventName);

protected:
	char			startingGrid[ARCADE_GRID_WIDTH][ARCADE_GRID_HEIGHT];
	GamePiece		grid[ARCADE_GRID_WIDTH][ARCADE_GRID_HEIGHT];
	MovingGamePiece	monsters[4];
	MovingGamePiece	player;
	MovingGamePiece	fruit;
	idVec2			playerMove;
	idStr			highscoreName;
	bool			bPlayerMoving;
	bool			bSimulating;
	bool			bGameOver;
	bool			bPoweredUp;
	int				score;
	int				highscore;
	int				lives;
	int				level;
	int				numMazes;
	int				nextScoreBoost;
	int				nextPelletTime;

private:
	int victoryAmount;
};


#endif

/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


//this is the gem cat game.
//move the gemcat paddle to collect gems.



//BC TODO:
//3 gems = triple ball.
//6 gems + 2+ balls in play = bigpaddle + replenish tripleballs.
//9 gems + bigpaddle + 2+ balls in play = launch a billion balls

//Drops: gems, smallpaddle.





#include "sys/platform.h"
#include "framework/Session_local.h"
#include "renderer/Image.h"
#include "sound/sound.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"
#include "idlib/LangDict.h"

#include "ui/GameBustOutWindow.h"
#include "Game_local.h"

#define BALL_RADIUS		12.f
#define BALL_SPEED		300.f //BC was 250
#define BALL_MAXSPEED	600.f //BC was 450

#define S_UNIQUE_CHANNEL	6

#define POWERUP_FALLSPEED			192.f //BC was 64
#define POWERUP_FALLSPEED_CURSE		96.f //BC cursed powerups move slower.

//BC
#define CURSE_MINTIME		3
#define CURSE_VARIATIONTIME	4
#define FREELIFE_THRESHOLD	100

#define MULTIBALL_THRESHOLD	3

#define DIAL_WIDTH 70.f

#define DEFAULT_HIGHSCORE 0

/*
*****************************************************************************
* BOEntity
****************************************************************************
*/
BOEntity::BOEntity(idGameBustOutWindow* _game) {
	game = _game;
	visible = true;

	materialName = "";
	material = NULL;
	width = height = 8;
	color = colorWhite;
	powerup = POWERUP_NONE;

	position.Zero();
	velocity.Zero();

	removed = false;
	fadeOut = 0;
}

BOEntity::~BOEntity() {
}

/*
======================
BOEntity::WriteToSaveGame
======================
*/
void BOEntity::WriteToSaveGame( idSaveGame *savefile ) const {
	savefile->WriteMiscPtr( CastWriteVoidPtrPtr(game) );

	savefile->Write( &visible, sizeof(visible) );

	savefile->WriteString( materialName );
	savefile->WriteMaterial( material );

	savefile->Write( &width, sizeof(width) );
	savefile->Write( &height, sizeof(height) );

	savefile->Write( &color, sizeof(color) );
	savefile->Write( &position, sizeof(position) );
	savefile->Write( &velocity, sizeof(velocity) );

	savefile->Write( &powerup, sizeof(powerup) );
	savefile->Write( &removed, sizeof(removed) );
	savefile->Write( &fadeOut, sizeof(fadeOut) );
}

/*
======================
BOEntity::ReadFromSaveGame
======================
*/
void BOEntity::ReadFromSaveGame( idRestoreGame *savefile, idGameBustOutWindow* _game ) {
	savefile->ReadMiscPtr( CastReadVoidPtrPtr(game) );

	game = _game;

	savefile->Read( &visible, sizeof(visible) );

	savefile->ReadString( materialName );
	savefile->ReadMaterial( material );

	savefile->Read( &width, sizeof(width) );
	savefile->Read( &height, sizeof(height) );

	savefile->Read( &color, sizeof(color) );
	savefile->Read( &position, sizeof(position) );
	savefile->Read( &velocity, sizeof(velocity) );

	savefile->Read( &powerup, sizeof(powerup) );
	savefile->Read( &removed, sizeof(removed) );
	savefile->Read( &fadeOut, sizeof(fadeOut) );
}

/*
======================
BOEntity::SetMaterial
======================
*/
void BOEntity::SetMaterial(const char* name) {
	materialName = name;
	material = declManager->FindMaterial( name );
	material->SetSort( SS_GUI );
}

/*
======================
BOEntity::SetSize
======================
*/
void BOEntity::SetSize( float _width, float _height ) {
	width = _width;
	height = _height;
}

/*
======================
BOEntity::SetVisible
======================
*/
void BOEntity::SetColor( float r, float g, float b, float a ) {
	color.x = r;
	color.y = g;
	color.z = b;
	color.w = a;
}

/*
======================
BOEntity::SetVisible
======================
*/
void BOEntity::SetVisible( bool isVisible ) {
	visible = isVisible;
}

/*
======================
BOEntity::Update
======================
*/
void BOEntity::Update( float timeslice, int guiTime ) {

	if ( !visible ) {
		return;
	}

	// Move the entity
	position += velocity * timeslice;

	// Fade out the ent
	if ( fadeOut ) {
		color.w -= timeslice * 10;

		if ( color.w <= 0.f ) {
			color.w = 0.f;
			removed = true;
		}
	}
}

/*
======================
BOEntity::Draw
======================
*/
void BOEntity::Draw(idDeviceContext *dc) {
	if ( visible ) {
		dc->DrawMaterialRotated( position.x, position.y, width, height, material, color, 1.0f, 1.0f, DEG2RAD(0.f) );
	}
}

/*
*****************************************************************************
* BOBrick
****************************************************************************
*/
BOBrick::BOBrick( void ) {
	ent = NULL;
	x = y = width = height = 0;
	powerup = POWERUP_NONE;
	isBroken = false;
}

BOBrick::BOBrick( BOEntity *_ent, float _x, float _y, float _width, float _height ) {
	ent = _ent;
	x = _x;
	y = _y;
	width = _width;
	height = _height;
	powerup = POWERUP_NONE;

	isBroken = false;

	ent->position.x = x;
	ent->position.y = y;
	ent->SetSize( width, height );
	ent->SetMaterial( "game/bustout/brick" );

	ent->game->entities.Append( ent );
}

BOBrick::~BOBrick( void ) {
}

/*
======================
BOBrick::WriteToSaveGame
======================
*/
void BOBrick::WriteToSaveGame( idSaveGame *savefile ) const {
	savefile->Write( &x, sizeof(x) );
	savefile->Write( &y, sizeof(y) );
	savefile->Write( &width, sizeof(width) );
	savefile->Write( &height, sizeof(height) );

	savefile->Write( &powerup, sizeof(powerup) );
	savefile->Write( &isBroken, sizeof(isBroken) );

	int index = ent->game->entities.FindIndex( ent );
	savefile->Write( &index, sizeof(index) );
}

/*
======================
BOBrick::ReadFromSaveGame
======================
*/
void BOBrick::ReadFromSaveGame( idRestoreGame *savefile, idGameBustOutWindow *game ) {
	savefile->Read( &x, sizeof(x) );
	savefile->Read( &y, sizeof(y) );
	savefile->Read( &width, sizeof(width) );
	savefile->Read( &height, sizeof(height) );

	savefile->Read( &powerup, sizeof(powerup) );
	savefile->Read( &isBroken, sizeof(isBroken) );

	int index;
	savefile->Read( &index, sizeof(index) );
	ent = game->entities[index];
}

/*
======================
BOBrick::SetColor
======================
*/
void BOBrick::SetColor( idVec4 bcolor ) {
	ent->SetColor( bcolor.x, bcolor.y, bcolor.z, bcolor.w );
}

/*
======================
BOBrick::checkCollision
======================
*/
collideDir_t BOBrick::checkCollision( idVec2 pos, idVec2 vel ) {
	idVec2	ptA, ptB;
	float	dist;

	collideDir_t	result = COLLIDE_NONE;

	if ( isBroken ) {
		return result;
	}

	// Check for collision with each edge
	idVec2 vec;

	// Bottom
	ptA.x = x;
	ptA.y = y + height;

	ptB.x = x + width;
	ptB.y = y + height;

	if ( vel.y < 0 && pos.y > ptA.y ) {
		if( pos.x > ptA.x && pos.x < ptB.x ) {
			dist = pos.y - ptA.y;

			if ( dist < BALL_RADIUS ) {
				result = COLLIDE_DOWN;
			}
		} else {
			if ( pos.x <= ptA.x ) {
				vec = pos - ptA;
			} else {
				vec = pos - ptB;
			}

			if ( (idMath::Fabs(vec.y) > idMath::Fabs(vec.x)) && (vec.LengthFast() < BALL_RADIUS) ) {
				result = COLLIDE_DOWN;
			}
		}
	}

	if ( result == COLLIDE_NONE ) {
		// Top
		ptA.y = y;
		ptB.y = y;

		if ( vel.y > 0 && pos.y < ptA.y ) {
			if( pos.x > ptA.x && pos.x < ptB.x ) {
				dist = ptA.y - pos.y;

				if ( dist < BALL_RADIUS ) {
					result = COLLIDE_UP;
				}
			} else {
				if ( pos.x <= ptA.x ) {
					vec = pos - ptA;
				} else {
					vec = pos - ptB;
				}

				if ( (idMath::Fabs(vec.y) > idMath::Fabs(vec.x)) && (vec.LengthFast() < BALL_RADIUS) ) {
					result = COLLIDE_UP;
				}
			}
		}

		if ( result == COLLIDE_NONE ) {
			// Left side
			ptA.x = x;
			ptA.y = y;

			ptB.x = x;
			ptB.y = y + height;

			if ( vel.x > 0 && pos.x < ptA.x ) {
				if( pos.y > ptA.y && pos.y < ptB.y ) {
					dist = ptA.x - pos.x;

					if ( dist < BALL_RADIUS ) {
						result = COLLIDE_LEFT;
					}
				} else {
					if ( pos.y <= ptA.y ) {
						vec = pos - ptA;
					} else {
						vec = pos - ptB;
					}

					if ( (idMath::Fabs(vec.x) >= idMath::Fabs(vec.y)) && (vec.LengthFast() < BALL_RADIUS) ) {
						result = COLLIDE_LEFT;
					}
				}
			}

			if ( result == COLLIDE_NONE ) {
				// Right side
				ptA.x = x + width;
				ptB.x = x + width;

				if ( vel.x < 0 && pos.x > ptA.x ) {
					if( pos.y > ptA.y && pos.y < ptB.y ) {
						dist = pos.x - ptA.x;

						if ( dist < BALL_RADIUS ) {
							result = COLLIDE_LEFT;
						}
					} else {
						if ( pos.y <= ptA.y ) {
							vec = pos - ptA;
						} else {
							vec = pos - ptB;
						}

						if ( (idMath::Fabs(vec.x) >= idMath::Fabs(vec.y)) && (vec.LengthFast() < BALL_RADIUS) ) {
							result = COLLIDE_LEFT;
						}
					}
				}

			}
		}
	}

	return result;
}

/*
*****************************************************************************
* idGameBustOutWindow
****************************************************************************
*/
idGameBustOutWindow::idGameBustOutWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	highScore = DEFAULT_HIGHSCORE;
	gameLocal.persistentLevelInfo.GetInt("bustOutHighScore", "0", highScore);
	dc = d;
	gui = g;
	CommonInit();
}

idGameBustOutWindow::idGameBustOutWindow(idUserInterfaceLocal *g) : idWindow(g) {
	highScore = DEFAULT_HIGHSCORE;
	gameLocal.persistentLevelInfo.GetInt("bustOutHighScore", "0", highScore);
	gui = g;
	CommonInit();
}

idGameBustOutWindow::~idGameBustOutWindow() {
	entities.DeleteContents(true);

	Mem_Free( levelBoardData );
}

/*
=============================
idGameBustOutWindow::WriteToSaveGame
=============================
*/
void idGameBustOutWindow::WriteToSaveGame( idSaveGame *savefile ) const {
	idWindow::WriteToSaveGame( savefile );

	int numberOfEnts = entities.Num();
	savefile->WriteInt( numberOfEnts ); // idList<BOEntity*> entities
	for (int i=0; i<numberOfEnts; i++ ) {
		entities[i]->WriteToSaveGame( savefile );
	}

	gamerunning.WriteToSaveGame( savefile ); // idWinBool gamerunning
	onFire.WriteToSaveGame( savefile ); // idWinBool onFire
	onContinue.WriteToSaveGame( savefile ); // idWinBool onContinue
	onNewGame.WriteToSaveGame( savefile ); // idWinBool onNewGame
	onNewLevel.WriteToSaveGame( savefile ); // idWinBool onNewLevel

	savefile->WriteFloat( timeSlice ); // float timeSlice
	savefile->WriteBool( gameOver ); // bool gameOver

	savefile->WriteInt( numLevels ); // int numLevels
	//boardDataLoaded = false; LoadBoardFiles();  //byte * levelBoardData 
	savefile->WriteBool( boardDataLoaded ); // bool boardDataLoaded

	savefile->WriteInt( numBricks ); // int numBricks
	savefile->WriteInt( currentLevel ); // int currentLevel

	savefile->WriteBool( updateScore ); // bool updateScore
	savefile->WriteInt( gameScore ); // int gameScore
	savefile->WriteInt( nextBallScore ); // int nextBallScore

	savefile->WriteInt( bigPaddleTime ); // int bigPaddleTime
	savefile->WriteFloat( paddleVelocity ); // float paddleVelocity

	savefile->WriteFloat( ballSpeed ); // float ballSpeed
	savefile->WriteInt( ballsRemaining ); // int ballsRemaining
	savefile->WriteInt( ballsInPlay ); // int ballsInPlay
	savefile->WriteBool( ballHitCeiling ); // bool ballHitCeiling

	numberOfEnts = balls.Num(); // idList<BOEntity*> balls
	savefile->WriteInt( numberOfEnts );
	for (int i=0; i<numberOfEnts; i++ ) {
		int entIdx = entities.FindIndex( balls[i] );
		savefile->WriteInt( entIdx );
	}

	numberOfEnts = powerUps.Num(); // idList<BOEntity*> powerUps
	savefile->WriteInt( numberOfEnts );
	for (int i=0; i<numberOfEnts; i++ ) {
		int entIdx = entities.FindIndex( powerUps[i] );
		savefile->WriteInt( entIdx );
	}

	paddle->WriteToSaveGame( savefile );// BOBrick * paddle

	savefile->WriteInt(BOARD_ROWS); // idList<BOBrick*> board[BOARD_ROWS]
	for (int row=0; row<BOARD_ROWS; row++ ) {
		numberOfEnts = board[row].Num();
		savefile->WriteInt( numberOfEnts );
		for (int i=0; i<numberOfEnts; i++ ) {
			board[row][i]->WriteToSaveGame( savefile );
		}
	}

	savefile->WriteInt( nextTurboMultiballTime ); // int nextTurboMultiballTime
	savefile->WriteInt( smallPaddleTime ); // int smallPaddleTime
	savefile->WriteInt( curseCounter ); // int curseCounter
	savefile->WriteInt( curseMaxCounter ); // int curseMaxCounter
	savefile->WriteInt( gemCounter ); // int gemCounter
	savefile->WriteInt( multiGemCounter ); // int multiGemCounter
	savefile->WriteBool( bigpaddleEligible ); // bool bigpaddleEligible
	savefile->WriteInt( dialBlinktimer ); // int dialBlinktimer

	dial->WriteToSaveGame( savefile );// BOBrick * dial

	savefile->WriteInt( highScore ); // int highScore

	savefile->WriteCheckSizeMarker();
}

/*
=============================
idGameBustOutWindow::ReadFromSaveGame
=============================
*/
void idGameBustOutWindow::ReadFromSaveGame( idRestoreGame *savefile ) {
	idWindow::ReadFromSaveGame( savefile );

	int numberOfEnts;
	savefile->ReadInt( numberOfEnts ); // idList<BOEntity*> entities
	entities.SetNum(numberOfEnts);
	for (int i=0; i<numberOfEnts; i++ ) {
		entities[i]= new BOEntity(this);
		entities[i]->ReadFromSaveGame( savefile, this );
	}

	gamerunning.ReadFromSaveGame( savefile ); // idWinBool gamerunning
	onFire.ReadFromSaveGame( savefile ); // idWinBool onFire
	onContinue.ReadFromSaveGame( savefile ); // idWinBool onContinue
	onNewGame.ReadFromSaveGame( savefile ); // idWinBool onNewGame
	onNewLevel.ReadFromSaveGame( savefile ); // idWinBool onNewLevel

	savefile->ReadFloat( timeSlice ); // float timeSlice
	savefile->ReadBool( gameOver ); // bool gameOver

	savefile->ReadInt( numLevels ); // int numLevels
	//boardDataLoaded = false; LoadBoardFiles();  //byte * levelBoardData 
	savefile->ReadBool( boardDataLoaded ); // bool boardDataLoaded

	savefile->ReadInt( numBricks ); // int numBricks
	savefile->ReadInt( currentLevel ); // int currentLevel

	savefile->ReadBool( updateScore ); // bool updateScore
	savefile->ReadInt( gameScore ); // int gameScore
	savefile->ReadInt( nextBallScore ); // int nextBallScore

	savefile->ReadInt( bigPaddleTime ); // int bigPaddleTime
	savefile->ReadFloat( paddleVelocity ); // float paddleVelocity

	savefile->ReadFloat( ballSpeed ); // float ballSpeed
	savefile->ReadInt( ballsRemaining ); // int ballsRemaining
	savefile->ReadInt( ballsInPlay ); // int ballsInPlay
	savefile->ReadBool( ballHitCeiling ); // bool ballHitCeiling

	savefile->ReadInt( numberOfEnts ); // idList<BOEntity*> balls
	balls.SetNum( numberOfEnts );
	for (int i=0; i<numberOfEnts; i++ ) {
		int entIdx;
		savefile->ReadInt( entIdx );
		balls[i] = entities[entIdx];
		balls[i]->ReadFromSaveGame( savefile, this );
	}

	savefile->ReadInt( numberOfEnts ); // idList<BOEntity*> powerUps
	powerUps.SetNum(numberOfEnts);
	for (int i=0; i<numberOfEnts; i++ ) {
		int entIdx;
		savefile->ReadInt( entIdx );
		powerUps[i] = entities[entIdx];
		powerUps[i]->ReadFromSaveGame( savefile, this );
	}

	paddle->ReadFromSaveGame( savefile, this );// BOBrick * paddle

	int rowNum; // idList<BOBrick*> board[BOARD_ROWS]
	savefile->ReadInt(rowNum);
	for ( int row=0 ; row<rowNum ; row++ ) {
		savefile->ReadInt( numberOfEnts );
		for (int i=0; i<numberOfEnts; i++ ) {

			BOBrick *brick = new BOBrick();
			brick->ReadFromSaveGame( savefile, this );
			board[row].Append( brick );
		}
	}

	savefile->ReadInt( nextTurboMultiballTime ); // int nextTurboMultiballTime
	savefile->ReadInt( smallPaddleTime ); // int smallPaddleTime
	savefile->ReadInt( curseCounter ); // int curseCounter
	savefile->ReadInt( curseMaxCounter ); // int curseMaxCounter
	savefile->ReadInt( gemCounter ); // int gemCounter
	savefile->ReadInt( multiGemCounter ); // int multiGemCounter
	savefile->ReadBool( bigpaddleEligible ); // bool bigpaddleEligible
	savefile->ReadInt( dialBlinktimer ); // int dialBlinktimer

	dial->ReadFromSaveGame( savefile, this );// BOBrick * dial

	savefile->ReadInt( highScore ); // int highScore

	savefile->ReadCheckSizeMarker();
}

/*
=============================
idGameBustOutWindow::ResetGameState
=============================
*/
void idGameBustOutWindow::ResetGameState() {

	idRandom rnd(gui->GetTime());

	gamerunning = false;
	gameOver = false;
	onFire = false;
	onContinue = false;
	onNewGame = false;
	onNewLevel = false;

	// Game moves forward 16 milliseconds every frame
	timeSlice = 0.016f;
	ballsRemaining = 2;
	ballSpeed = BALL_SPEED;
	ballsInPlay = 0;
	updateScore = false;
	numBricks = 0;
	currentLevel = 1;
	gameScore = 0;
	bigPaddleTime = 0;
	nextBallScore = gameScore + 10000;

	//BC
	smallPaddleTime = 0;
	nextTurboMultiballTime = 0;
	curseCounter = 0;
	curseMaxCounter = CURSE_MINTIME + rnd.RandomInt(CURSE_VARIATIONTIME); //Set next curse counter to be a random value.
	gemCounter = 0;
	multiGemCounter = 0;
	bigpaddleEligible = true;
	dialBlinktimer = 0;

	ClearBoard();

	gui->SetStateInt("highscore", highScore); //for the title screen high score text.
}

/*
=============================
idGameBustOutWindow::CommonInit
=============================
*/
void idGameBustOutWindow::CommonInit() {
	BOEntity *ent;

	BOEntity *dialEnt;

	// Precache images
	declManager->FindMaterial( "game/bustout/ball" );
	declManager->FindMaterial( "game/bustout/doublepaddle" );
	declManager->FindMaterial( "game/bustout/powerup_bigpaddle" );
	declManager->FindMaterial( "game/bustout/powerup_multiball" );
	declManager->FindMaterial( "game/bustout/brick" );

	//BC
	declManager->FindMaterial("game/bustout/dial0");
	declManager->FindMaterial("game/bustout/dial1");
	declManager->FindMaterial("game/bustout/dial2");
	declManager->FindMaterial("game/bustout/dial3");

	// Precache sounds
	declManager->FindSound( "arcade_ballbounce" );
	declManager->FindSound( "arcade_brickhit" );
	declManager->FindSound( "arcade_missedball" );
	declManager->FindSound( "arcade_extraball" );
	declManager->FindSound( "arcade_powerup" );

	ResetGameState();

	numLevels = 0;
	boardDataLoaded = false;
	levelBoardData = NULL;

	// Create Paddle
	ent = new BOEntity( this );
	paddle = new BOBrick( ent, 260.f, 440.f, 96.f, 24.f );
	paddle->ent->SetMaterial( "game/bustout/paddle" );


	dialEnt = new BOEntity(this);
	dial = new BOBrick(dialEnt, 260.f, 463.f, DIAL_WIDTH, 16.f);
	dial->ent->SetMaterial("game/bustout/dial0");

	
}

// SW 3rd March 2025:
// We pass through a special event from idGameBlock::Think so that the game only updates when the game block is capable of thinking.
// This means that the game should pause correctly whenever the world is paused.
void idGameBustOutWindow::RunNamedEvent(const char* namedEvent)
{
	idWindow::RunNamedEvent(namedEvent);

	if (idStr::Icmp(namedEvent, "updateGame") == 0)
	{
		UpdateGame();
	}
}

/*
=============================
idGameBustOutWindow::HandleEvent
=============================
*/
const char *idGameBustOutWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	int key = event->evValue;

	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = idWindow::HandleEvent(event, updateVisuals);

	if ( event->evType == SE_KEY ) {

		if ( !event->evValue2 ) {
			return ret;
		}
		if ( key == K_MOUSE1) {
			// Mouse was clicked
			if ( ballsInPlay <= 0 && ballsRemaining >= 0 && !gameOver) //Only allow left click if I have lives in the hopper
			{
				BOEntity *ball = CreateNewBall();

				ball->SetVisible( true );
				ball->position.x = paddle->ent->position.x + 48.f; //BC TODO: change according to paddle size.
				ball->position.y = 430.f;

				ball->velocity.x = ballSpeed;
				ball->velocity.y = -ballSpeed*2.f;
				ball->velocity.NormalizeFast();
				ball->velocity *= ballSpeed;

				//BC play sound. Click to fire the ball.
				session->sw->PlayShaderDirectly("arcade_ballbounce", S_UNIQUE_CHANNEL);

				gui->HandleNamedEvent("balllaunch");
			}
		} else {
			return ret;
		}
	}

	return ret;
}

/*
=============================
idGameBustOutWindow::ParseInternalVar
=============================
*/
bool idGameBustOutWindow::ParseInternalVar(const char *_name, idParser *src) {
	if ( idStr::Icmp(_name, "gamerunning") == 0 ) {
		gamerunning = src->ParseBool();
		return true;
	}
	if ( idStr::Icmp(_name, "onFire") == 0 ) {
		onFire = src->ParseBool();
		return true;
	}
	if ( idStr::Icmp(_name, "onContinue") == 0 ) {
		onContinue = src->ParseBool();
		return true;
	}
	if ( idStr::Icmp(_name, "onNewGame") == 0 ) {
		onNewGame = src->ParseBool();
		return true;
	}
	if ( idStr::Icmp(_name, "onNewLevel") == 0 ) {
		onNewLevel = src->ParseBool();
		return true;
	}
	if ( idStr::Icmp(_name, "numLevels") == 0 ) {
		numLevels = src->ParseInt();

		// Load all the level images
		LoadBoardFiles();
		return true;
	}

	return idWindow::ParseInternalVar(_name, src);
}

/*
=============================
idGameBustOutWindow::GetWinVarByName
=============================
*/
idWinVar *idGameBustOutWindow::GetWinVarByName(const char *_name, bool winLookup, drawWin_t** owner) {
	idWinVar *retVar = NULL;

	if ( idStr::Icmp(_name, "gamerunning") == 0 ) {
		retVar = &gamerunning;
	} else	if ( idStr::Icmp(_name, "onFire") == 0 ) {
		retVar = &onFire;
	} else	if ( idStr::Icmp(_name, "onContinue") == 0 ) {
		retVar = &onContinue;
	} else	if ( idStr::Icmp(_name, "onNewGame") == 0 ) {
		retVar = &onNewGame;
	} else	if ( idStr::Icmp(_name, "onNewLevel") == 0 ) {
		retVar = &onNewLevel;
	}

	if(retVar) {
		return retVar;
	}

	return idWindow::GetWinVarByName(_name, winLookup, owner);
}

/*
=============================
idGameBustOutWindow::PostParse
=============================
*/
void idGameBustOutWindow::PostParse() {
	idWindow::PostParse();
}

/*
=============================
idGameBustOutWindow::Draw
=============================
*/
void idGameBustOutWindow::Draw(int time, float x, float y) {
	int i;

	for( i = entities.Num()-1; i >= 0; i-- ) {
		entities[i]->Draw(dc);
	}
}

/*
=============================
idGameBustOutWindow::UpdateScore
=============================
*/
void idGameBustOutWindow::UpdateScore() {

	if ( gameOver ) {
		gui->HandleNamedEvent( "GameOver" );

		//Game has ended.
		//Do the high score logic.

		idStr highscorestringToDisplay = "";

		if (gemCounter > highScore)
		{
			//Player has a new high score.
			highscorestringToDisplay = common->GetLanguageDict()->GetString("#str_gui_arcade_newhighscore");
			highScore = gemCounter;
			gameLocal.persistentLevelInfo.SetInt("bustOutHighScore", highScore);
		}
		else
		{
			highscorestringToDisplay =  idStr::Format(common->GetLanguageDict()->GetString("#str_gui_arcade_currenthighscore"), highScore);
		}

		gui->SetStateString("highscoretext", highscorestringToDisplay.c_str());
		gui->SetStateInt("highscore", highScore); //for the title screen high score text.
		return;
	}

	// Check for level progression
	if ( numBricks == 0 ) {
		ClearBalls();

		gui->HandleNamedEvent( "levelComplete" );
	}

	// Check for new ball score
	if ( gameScore >= nextBallScore ) {
		ballsRemaining++;
		gui->HandleNamedEvent( "extraBall" );

		// Play sound
		session->sw->PlayShaderDirectly( "arcade_extraball", S_UNIQUE_CHANNEL );

		nextBallScore = gameScore + 10000;
	}

	gui->SetStateString( "player_score", va("%i", gameScore ) );
	gui->SetStateString( "balls_remaining", va("%i", ballsRemaining ) );
	gui->SetStateString( "current_level", va("%i", currentLevel+1  ) ); //BC add +1 so the level fanfare shows correct level
	gui->SetStateString( "next_ball_score", va("%i", nextBallScore ) );
}

/*
=============================
idGameBustOutWindow::ClearBoard
=============================
*/
void idGameBustOutWindow::ClearBoard( void ) {
	int i,j;

	ClearPowerups();

	ballHitCeiling = false;

	for ( i=0; i<BOARD_ROWS; i++ ) {
		for ( j=0; j<board[i].Num(); j++ ) {

			BOBrick *brick = board[i][j];
			brick->ent->removed = true;
		}

		board[i].DeleteContents( true );
	}
}

/*
=============================
idGameBustOutWindow::ClearPowerups
=============================
*/
void idGameBustOutWindow::ClearPowerups( void ) {
	while ( powerUps.Num() ) {
		powerUps[0]->removed = true;
		powerUps.RemoveIndex( 0 );
	}

	//Reset paddle size.
	smallPaddleTime = 0;
	bigPaddleTime = 0;
}

/*
=============================
idGameBustOutWindow::ClearBalls
=============================
*/
void idGameBustOutWindow::ClearBalls( void ) {
	while ( balls.Num() ) {
		balls[0]->removed = true;
		balls.RemoveIndex( 0 );
	}

	ballsInPlay = 0;
}

/*
=============================
idGameBustOutWindow::LoadBoardFiles
=============================
*/
void idGameBustOutWindow::LoadBoardFiles( void ) {
	int i;
	int w,h;
	ID_TIME_T time;
	int boardSize;
	byte *currentBoard;

	if ( boardDataLoaded ) {
		return;
	}

	boardSize = 9 * 12 * 4;
	levelBoardData = (byte*)Mem_Alloc( boardSize * numLevels );

	currentBoard = levelBoardData;

	for ( i=0; i<numLevels; i++ ) {
		byte *pic;
		idStr	name = "guis/assets/bustout/level";
		name += (i+1);
		name += ".tga";

		R_LoadImage( name, &pic, &w, &h, &time, false );

		if ( pic != NULL ) {
			if ( w != 9 || h != 12 ) {
				common->DWarning( "Hell Bust-Out level image not correct dimensions! (%d x %d)", w, h );
			}

			memcpy( currentBoard, pic, boardSize );
			Mem_Free(pic);
		}

		currentBoard += boardSize;
	}

	boardDataLoaded = true;
}

/*
=============================
idGameBustOutWindow::SetCurrentBoard
=============================
*/
void idGameBustOutWindow::SetCurrentBoard( void ) {
	int i,j;
	int realLevel = ((currentLevel-1) % numLevels);
	int boardSize;
	byte *currentBoard;
	float	bx = 11.f;
	float	by = 24.f;
	float	stepx = 619.f / 9.f;
	float	stepy = ( 256 / 12.f );

	boardSize = 9 * 12 * 4;
	currentBoard = levelBoardData + ( realLevel * boardSize );

	for ( j=0; j<BOARD_ROWS; j++ ) {
		bx = 11.f;

		for ( i=0; i<9; i++ ) {
			int pixelindex = (j*9*4) + (i*4);

			if ( currentBoard[pixelindex + 3] ) {
				idVec4 bcolor;
				float pType = 0.f;

				BOEntity *bent = new BOEntity( this );
				BOBrick *brick = new BOBrick( bent, bx, by, stepx, stepy );

				bcolor.x = currentBoard[pixelindex + 0] / 255.f;
				bcolor.y = currentBoard[pixelindex + 1] / 255.f;
				bcolor.z = currentBoard[pixelindex + 2] / 255.f;
				bcolor.w = 1.f;
				brick->SetColor( bcolor );

				//BC No pre-populated powerup bricks.
				//pType = currentBoard[pixelindex + 3] / 255.f;
				//if ( pType > 0.f && pType < 1.f )
				//{
				//	if ( pType < 0.5f )
				//	{
				//		brick->powerup = POWERUP_BIGPADDLE;
				//	}
				//	else
				//	{
				//		brick->powerup = POWERUP_MULTIBALL;
				//	}
				//}

				pType = currentBoard[pixelindex + 3] / 255.f;
				if (pType > 0.f)
				{
					brick->powerup = POWERUP_MULTIBALL;
				}

				board[j].Append( brick );
				numBricks++;
			}

			bx += stepx;
		}

		by += stepy;
	}
}

/*
=============================
idGameBustOutWindow::CreateNewBall
=============================
*/
BOEntity * idGameBustOutWindow::CreateNewBall( void ) {
	BOEntity *ball;

	ball = new BOEntity( this );
	ball->position.x = 300.f;
	ball->position.y = 416.f;
	ball->SetMaterial( "game/bustout/ball" );
	ball->SetSize( BALL_RADIUS*2.f, BALL_RADIUS*2.f );
	ball->SetVisible( false );

	ballsInPlay++;

	balls.Append( ball );
	entities.Append( ball );

	return ball;
}

/*
=============================
idGameBustOutWindow::CreatePowerup
=============================
*/
BOEntity * idGameBustOutWindow::CreatePowerup( BOBrick *brick )
{
	if (numBricks <= 1) //BC if last brick, then don't spawn powerup.
		return NULL;

	BOEntity *powerEnt = new BOEntity( this );

	powerEnt->position.x = brick->x;
	powerEnt->position.y = brick->y;
	powerEnt->velocity.x = 0.f;	

	powerEnt->powerup = brick->powerup;

	switch( powerEnt->powerup ) {
		case POWERUP_BIGPADDLE:
			powerEnt->SetMaterial( "game/bustout/powerup_bigpaddle" );
			powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
			break;
		case POWERUP_MULTIBALL:
			powerEnt->SetMaterial( "game/bustout/powerup_multiball" );
			powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
			break;
		case POWERUP_SMALLPADDLE:
			powerEnt->SetMaterial("game/bustout/powerup_smallpaddle");
			powerEnt->velocity.y = POWERUP_FALLSPEED_CURSE; //BC
			break;
		case POWERUP_GEM:
			powerEnt->SetMaterial("game/bustout/powerup_gem");
			powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
			break;
		default:
			powerEnt->SetMaterial( "textures/common/nodraw" );
			break;
	}

	powerEnt->SetSize( 619/9, 256/12 );
	powerEnt->SetVisible( true );

	powerUps.Append( powerEnt );
	entities.Append( powerEnt );

	return powerEnt;
}



//BC split this off into its own function so it can be called from different places.
void idGameBustOutWindow::ActivateMultiball()
{
	BOEntity *ball;

	for (int b = 0; b<2; b++)
	{
		ball = CreateNewBall();
		ball->position = balls[0]->position;
		ball->velocity = balls[0]->velocity;

		if (b == 0) {
			ball->velocity.x -= 35.f;
		}
		else {
			ball->velocity.x += 35.f;
		}
		ball->velocity.NormalizeFast();
		ball->velocity *= ballSpeed;

		ball->SetVisible(true);
	}

	session->sw->PlayShaderDirectly("arcade_extraball", 5);
	
}

/*
=============================
idGameBustOutWindow::UpdatePowerups
=============================
*/
void idGameBustOutWindow::UpdatePowerups( void ) {
	idVec2 pos;

	for ( int i=0; i < powerUps.Num(); i++ ) {
		BOEntity *pUp = powerUps[i];

		// Check for powerup falling below screen
		if ( pUp->position.y > 480 ) {

			powerUps.RemoveIndex( i );
			pUp->removed = true;
			continue;
		}

		// Check for the paddle catching a powerup
		pos.x = pUp->position.x + ( pUp->width / 2 );
		pos.y = pUp->position.y + ( pUp->height / 2 );

		collideDir_t collision = paddle->checkCollision( pos, pUp->velocity );
		if ( collision != COLLIDE_NONE )
		{
			//BOEntity *ball;

			// Give the powerup to the player
			switch( pUp->powerup ) {
				case POWERUP_BIGPADDLE:
					bigPaddleTime = gui->GetTime() + 600000;
					smallPaddleTime = 0;
					session->sw->PlayShaderDirectly("arcade_powerup", S_UNIQUE_CHANNEL);
					break;
				case POWERUP_MULTIBALL:
					// Create 2 new balls in the spot of the existing ball
					ActivateMultiball();
					session->sw->PlayShaderDirectly("arcade_powerup", S_UNIQUE_CHANNEL);
					break;
				case POWERUP_SMALLPADDLE:
					smallPaddleTime = gui->GetTime() + 10000; //BC how long small paddle curse lasts.
					bigPaddleTime = 0;
					session->sw->PlayShaderDirectly("arcade_curse", S_UNIQUE_CHANNEL);
					break;
				case POWERUP_GEM:
					//BC has captured a gem. Make things happen if threshold numbers are met.
					gemCounter++;
					multiGemCounter++;

					bigpaddleEligible = true;

					//BC
					gui->SetStateString("gemcounter", va("%i", gemCounter));

					//BC If hit threshold, reward a life.
					if (gemCounter % FREELIFE_THRESHOLD == 0)
					{
						//Life rewarded.
						ballsRemaining++;
						gui->HandleNamedEvent("extraBall");

						// Play sound
						session->sw->PlayShaderDirectly("arcade_extraball", S_UNIQUE_CHANNEL);
					}

					//BC
					if (multiGemCounter >= MULTIBALL_THRESHOLD)
					{
						//Activate multiball.
						multiGemCounter = 0;
						ActivateMultiball();

						//gui->HandleNamedEvent("multiBall");
						dialBlinktimer = gui->GetTime() + 400;
					}
					
					

					session->sw->PlayShaderDirectly("arcade_getgem", S_UNIQUE_CHANNEL);

					if (gemCounter >= 50 && common->g_SteamUtilities && common->g_SteamUtilities->IsSteamInitialized())
					{
						common->g_SteamUtilities->SetAchievement("ach_gemcat");
					}

					break;
				default:
					break;
			}

			
			

			// Remove it
			powerUps.RemoveIndex( i );
			pUp->removed = true;
		}
	}
}

/*
=============================
idGameBustOutWindow::UpdatePaddle
=============================
*/
void idGameBustOutWindow::UpdatePaddle( void ) {
	idVec2 cursorPos;
	float  oldPos = paddle->x;

	cursorPos.x = gui->CursorX();
	cursorPos.y = gui->CursorY();

	if ( bigPaddleTime > gui->GetTime() )
	{
		paddle->x = cursorPos.x - 80.f;
		paddle->width = 160;
		paddle->ent->width = 160;
		paddle->ent->SetMaterial( "game/bustout/doublepaddle" );
	}
	else if (smallPaddleTime > gui->GetTime()) //BC small paddle curse.
	{
		paddle->x = cursorPos.x - 24.f;
		paddle->width = 48;
		paddle->ent->width = 48;
		paddle->ent->SetMaterial("game/bustout/smallpaddle");
	}
	else
	{
		paddle->x = cursorPos.x - 48.f;
		paddle->width = 96;
		paddle->ent->width = 96;
		paddle->ent->SetMaterial( "game/bustout/paddle" );
	}
	paddle->ent->position.x = paddle->x;

	paddleVelocity = (paddle->x - oldPos);

	dial->ent->position.x = gui->CursorX() - (DIAL_WIDTH/2.f);
}

/*
=============================
idGameBustOutWindow::UpdateBall
=============================
*/
void idGameBustOutWindow::UpdateBall( void ) {
	int ballnum,i,j;
	bool playSoundBounce = false;
	bool playSoundBrick = false;
	static int bounceChannel = 1;

	if ( ballsInPlay == 0 ) {
		return;
	}

	for ( ballnum = 0; ballnum < balls.Num(); ballnum++ ) {
		BOEntity *ball = balls[ballnum];

		// Check for ball going below screen, lost ball
		if ( ball->position.y > 480.f ) {
			ball->removed = true;
			continue;
		}

		// Check world collision
		if ( ball->position.y < 20 && ball->velocity.y < 0 ) {
			ball->velocity.y = -ball->velocity.y;

			// Increase ball speed when it hits ceiling
			if ( !ballHitCeiling ) {
				ballSpeed *= 1.25f;
				ballHitCeiling = true;
			}
			playSoundBounce = true;
		}

		if ( ball->position.x > 608 && ball->velocity.x > 0 ) {
			ball->velocity.x = -ball->velocity.x;
			playSoundBounce = true;
		} else if ( ball->position.x < 8 && ball->velocity.x < 0 ) {
			ball->velocity.x = -ball->velocity.x;
			playSoundBounce = true;
		}

		// Check for Paddle collision
		idVec2 ballCenter = ball->position + idVec2( BALL_RADIUS, BALL_RADIUS );
		collideDir_t collision = paddle->checkCollision( ballCenter, ball->velocity );

		if ( collision == COLLIDE_UP ) {
			if ( ball->velocity.y > 0 ) {
				idVec2	paddleVec( paddleVelocity*2, 0 );
				float	centerX;

				if ( bigPaddleTime > gui->GetTime() )
				{
					centerX = paddle->x + 80.f;
				}
				else if (smallPaddleTime > gui->GetTime())
				{
					centerX = paddle->x + 24.f; //BC
				}
				else
				{
					centerX = paddle->x + 48.f;
				}

				ball->velocity.y = -ball->velocity.y;

				paddleVec.x += (ball->position.x - centerX) * 2;

				ball->velocity += paddleVec;
				ball->velocity.NormalizeFast();
				ball->velocity *= ballSpeed;

				playSoundBounce = true;
			}
		} else if ( collision == COLLIDE_LEFT || collision == COLLIDE_RIGHT ) {
			if ( ball->velocity.y > 0 ) {
				ball->velocity.x = -ball->velocity.x;
				playSoundBounce = true;
			}
		}

		//BC if brick count is ONE, then do a turbo multiball
		//if ((collision == COLLIDE_UP || collision == COLLIDE_LEFT || collision == COLLIDE_RIGHT) && numBricks <= 1 && gui->GetTime() > nextTurboMultiballTime)
		//{
		//	nextTurboMultiballTime = gui->GetTime() + 300; //Limit how often the turbo multiball happens.
		//	ActivateMultiball();
		//}


		collision = COLLIDE_NONE;

		// Check for collision with bricks
		for ( i=0; i<BOARD_ROWS; i++ ) {
			int num = board[i].Num();

			for ( j=0; j<num; j++ ) {
				BOBrick *brick = (board[i])[j];

				collision = brick->checkCollision( ballCenter, ball->velocity );
				if ( collision ) {
					// Now break the brick if there was a collision
					brick->isBroken = true;
					brick->ent->fadeOut = true;

					//if ( brick->powerup > POWERUP_NONE )
					//{
					//	CreatePowerup( brick );
					//
					//	session->sw->PlayShaderDirectly("arcade_powerup_spawn", S_UNIQUE_CHANNEL);				//BC play sound when powerup spawns.		
					//}

					//BC new powerup system.
					CreatePowerup2(brick);

					numBricks--;
					gameScore += 100;
					updateScore = true;

					// Go ahead an forcibly remove the last brick, no fade
					if ( numBricks == 0 ) {
						brick->ent->removed = true;
					}
					board[i].Remove( brick );
					break;
				}
			}

			if ( collision ) {
				playSoundBrick = true;
				break;
			}
		}

		if ( collision == COLLIDE_DOWN || collision == COLLIDE_UP ) {
			ball->velocity.y *= -1;
		} else if ( collision == COLLIDE_LEFT || collision == COLLIDE_RIGHT ) {
			ball->velocity.x *= -1;
		}

		if ( playSoundBounce )
		{
			session->sw->PlayShaderDirectly( "arcade_ballbounce", bounceChannel );
		}
		else if ( playSoundBrick )
		{
			if (numBricks <= 0)
			{
				session->sw->PlayShaderDirectly("arcade_nextlevel", S_UNIQUE_CHANNEL); //BC Play special sound when next level happens
			}
			else
			{
				session->sw->PlayShaderDirectly("arcade_brickhit", bounceChannel);
			}
		}

		if ( playSoundBounce || playSoundBrick ) {
			bounceChannel++;
			if ( bounceChannel == 4 ) {
				bounceChannel = 1;
			}
		}
	}

	// Check to see if any balls were removed from play
	for ( ballnum=0; ballnum<balls.Num(); ballnum++ ) {
		if ( balls[ballnum]->removed ) {
			ballsInPlay--;
			balls.RemoveIndex( ballnum );
		}
	}

	// If all the balls were removed, update the game accordingly
	if ( ballsInPlay == 0 )
	{
		if ( ballsRemaining <= 0 )
		{
			gameOver = true;

			// Game Over sound
			session->sw->PlayShaderDirectly( "arcade_gameover", S_UNIQUE_CHANNEL );
		}
		else
		{
			ballsRemaining--;

			// Ball was lost, but game is not over
			session->sw->PlayShaderDirectly( "arcade_missedball", S_UNIQUE_CHANNEL );
		}

		ClearPowerups();
		updateScore = true;
	}
}

/*
=============================
idGameBustOutWindow::UpdateGame
=============================
*/
void idGameBustOutWindow::UpdateGame() {
	int i;

	if ( onNewGame ) {
		ResetGameState();

		// Create Board
		SetCurrentBoard();

		gamerunning = true;
	}
	if ( onContinue ) {
		gameOver = false;
		ballsRemaining = 3;

		onContinue = false;
	}
	if ( onNewLevel ) {
		currentLevel++;

		ClearBoard();
		SetCurrentBoard();

		
		//ballSpeed = BALL_SPEED * ( 1.f + ((float)currentLevel/5.f) );

		//BC ballspeed function
		ballSpeed = BALL_SPEED * (1.f + ((float)currentLevel / 10.f));
		if ( ballSpeed > BALL_MAXSPEED ) {
			ballSpeed = BALL_MAXSPEED;
		}
		updateScore = true;
		onNewLevel = false;
	}

	if(gamerunning == true) {

		UpdatePaddle();
		UpdateBall();
		UpdatePowerups();

		for( i = 0; i < entities.Num(); i++ ) {
			entities[i]->Update( timeSlice, gui->GetTime() );
		}

		// Delete entities that need to be deleted
		for( i = entities.Num()-1; i >= 0; i-- ) {
			if( entities[i]->removed ) {
				BOEntity* ent = entities[i];
				delete ent;
				entities.RemoveIndex(i);
			}
		}

		if ( updateScore ) {
			UpdateScore();
			updateScore = false;
		}

		//BC
		if (dialBlinktimer > gui->GetTime())
		{
			dial->ent->SetMaterial("game/bustout/dial3_blink");
		}
		else
		{
			dial->ent->SetMaterial(va("game/bustout/dial%i", multiGemCounter));
		}
	}
}


//BC this is the new powerup creation function.
BOEntity * idGameBustOutWindow::CreatePowerup2(BOBrick *brick)
{
	if (numBricks <= 1) //BC if last brick, then don't spawn powerup.
		return NULL;

	BOEntity *powerEnt = new BOEntity(this);

	powerEnt->position.x = brick->x;
	powerEnt->position.y = brick->y;
	powerEnt->velocity.x = 0.f;

	//Decide which powerup to generate.

	if (gemCounter > 0 && gemCounter % 10 == 0 && bigpaddleEligible && bigPaddleTime < gui->GetTime() /*Don't spawn this powerup if player already has it*/)
	{
		powerEnt->powerup = POWERUP_BIGPADDLE;
		curseCounter = 0;
		bigpaddleEligible = false;
	}
	else if (curseCounter >= curseMaxCounter)
	{
		idRandom rnd(gui->GetTime());

		powerEnt->powerup = POWERUP_SMALLPADDLE;
		curseMaxCounter = CURSE_MINTIME + rnd.RandomInt(CURSE_VARIATIONTIME); //Set next curse counter to be a random value.
		curseCounter = 0;
	}
	else
	{
		powerEnt->powerup = POWERUP_GEM;
		curseCounter++; //Fired whenever a brick is hit.
	}

	switch (powerEnt->powerup) {
	case POWERUP_BIGPADDLE:
		powerEnt->SetMaterial("game/bustout/powerup_bigpaddle");
		powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
		break;
	//case POWERUP_MULTIBALL:
	//	powerEnt->SetMaterial("game/bustout/powerup_multiball");
	//	powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
	//	break;
	case POWERUP_SMALLPADDLE:
		powerEnt->SetMaterial("game/bustout/powerup_smallpaddle");
		powerEnt->velocity.y = POWERUP_FALLSPEED_CURSE; //BC
		break;
	case POWERUP_GEM:
		powerEnt->SetMaterial("game/bustout/powerup_gem");
		powerEnt->velocity.y = POWERUP_FALLSPEED; //BC
		break;
	default:
		powerEnt->SetMaterial("textures/common/nodraw");
		break;
	}

	powerEnt->SetSize(619 / 9, 256 / 12);
	powerEnt->SetVisible(true);

	powerUps.Append(powerEnt);
	entities.Append(powerEnt);

	return powerEnt;
}
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

#include "sys/platform.h"
#include "framework/Session_local.h"
#include "sound/sound.h"

#include "idlib/LangDict.h"

#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"

#include "ui/GameBearShootWindow.h"
#include "Game_local.h"

#define BEAR_GRAVITY 240
#define BEAR_SIZE 24.f
#define BEAR_SHRINK_TIME 600.f //bc WAS 2000.f

//#define MAX_WINDFORCE 100.f

//BC
#define BEAR_SHRINK_TIME_FAIL 200.f //delay after banana fails to hit target

#define PRIMATE_FALLSPEED 500.f
#define PRIMATE_BOBSPEED 40.f
#define BOMB_BOBSPEED 70.f
#define BOMBSIZE 48.f

#define FORCE_MIN 100.f
#define FORCE_MAX 500.f

#define DEFAULT_HIGHSCORE 0

idCVar bearTurretAngle( "bearTurretAngle", "0", CVAR_FLOAT, "" );
idCVar bearTurretForce( "bearTurretForce", "200", CVAR_FLOAT, "" );

/*
*****************************************************************************
* BSEntity
****************************************************************************
*/
BSEntity::BSEntity(idGameBearShootWindow* _game) {
	game = _game;
	visible = true;

	entColor = colorWhite;
	materialName = "";
	material = NULL;
	width = height = 8;
	rotation = 0.f;
	rotationSpeed = 0.f;
	fadeIn = false;
	fadeOut = false;

	position.Zero();
	velocity.Zero();
}

BSEntity::~BSEntity() {
}

/*
======================
BSEntity::WriteToSaveGame
======================
*/
void BSEntity::WriteToSaveGame( idSaveGame *savefile ) const {
	//savefile->WriteMiscPtr( CastWriteVoidPtrPtr(game) );

	savefile->WriteString( materialName );

	savefile->Write( &width, sizeof(width) );
	savefile->Write( &height, sizeof(height) );
	savefile->Write( &visible, sizeof(visible) );

	savefile->Write( &entColor, sizeof(entColor) );
	savefile->Write( &position, sizeof(position) );
	savefile->Write( &rotation, sizeof(rotation) );
	savefile->Write( &rotationSpeed, sizeof(rotationSpeed) );
	savefile->Write( &velocity, sizeof(velocity) );

	savefile->Write( &fadeIn, sizeof(fadeIn) );
	savefile->Write( &fadeOut, sizeof(fadeOut) );
}

/*
======================
BSEntity::ReadFromSaveGame
======================
*/
void BSEntity::ReadFromSaveGame( idRestoreGame *savefile, idGameBearShootWindow* _game ) {

	//savefile->ReadMiscPtr( CastReadVoidPtrPtr(game) );

	game = _game;

	savefile->ReadString( materialName );
	SetMaterial( materialName );

	savefile->Read( &width, sizeof(width) );
	savefile->Read( &height, sizeof(height) );
	savefile->Read( &visible, sizeof(visible) );

	savefile->Read( &entColor, sizeof(entColor) );
	savefile->Read( &position, sizeof(position) );
	savefile->Read( &rotation, sizeof(rotation) );
	savefile->Read( &rotationSpeed, sizeof(rotationSpeed) );
	savefile->Read( &velocity, sizeof(velocity) );

	savefile->Read( &fadeIn, sizeof(fadeIn) );
	savefile->Read( &fadeOut, sizeof(fadeOut) );
}

/*
======================
BSEntity::SetMaterial
======================
*/
void BSEntity::SetMaterial(const char* name) {
	materialName = name;
	material = declManager->FindMaterial( name );
	material->SetSort( SS_GUI );
}

/*
======================
BSEntity::SetSize
======================
*/
void BSEntity::SetSize( float _width, float _height ) {
	width = _width;
	height = _height;
}

/*
======================
BSEntity::SetVisible
======================
*/
void BSEntity::SetVisible( bool isVisible ) {
	visible = isVisible;
}

/*
======================
BSEntity::Update
======================
*/
void BSEntity::Update( float timeslice ) {

	if ( !visible ) {
		return;
	}

	// Fades
	if ( fadeIn && entColor.w < 1.f ) {
		entColor.w += 1 * timeslice;
		if ( entColor.w >= 1.f ) {
			entColor.w = 1.f;
			fadeIn = false;
		}
	}
	if ( fadeOut && entColor.w > 0.f ) {
		entColor.w -= 1 * timeslice;
		if ( entColor.w <= 0.f ) {
			entColor.w = 0.f;
			fadeOut = false;
		}
	}

	// Move the entity
	position += velocity * timeslice;

	// Rotate Entity
	rotation += rotationSpeed * timeslice;
}

/*
======================
BSEntity::Draw
======================
*/
void BSEntity::Draw(idDeviceContext *dc) {
	if ( visible ) {
		dc->DrawMaterialRotated( position.x, position.y, width, height, material, entColor, 1.0f, 1.0f, DEG2RAD(rotation) );
	}
}

/*
*****************************************************************************
* idGameBearShootWindow
****************************************************************************
*/
idGameBearShootWindow::idGameBearShootWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	highScore = DEFAULT_HIGHSCORE;
	gameLocal.persistentLevelInfo.GetInt("bearShootHighScore", "0", highScore);
	dc = d;
	gui = g;
	CommonInit();
}

idGameBearShootWindow::idGameBearShootWindow(idUserInterfaceLocal *g) : idWindow(g) {
	highScore = DEFAULT_HIGHSCORE;
	gameLocal.persistentLevelInfo.GetInt("bearShootHighScore", "0", highScore);
	gui = g;
	CommonInit();
}

idGameBearShootWindow::~idGameBearShootWindow() {
	entities.DeleteContents(true);
}

/*
=============================
idGameBearShootWindow::WriteToSaveGame
=============================
*/
void idGameBearShootWindow::WriteToSaveGame( idSaveGame *savefile ) const {
	idWindow::WriteToSaveGame( savefile );

	gamerunning.WriteToSaveGame( savefile ); // idWinBool gamerunning
	onFire.WriteToSaveGame( savefile ); // idWinBool onFire
	onContinue.WriteToSaveGame( savefile ); // idWinBool onContinue
	onNewGame.WriteToSaveGame( savefile ); // idWinBool onNewGame

	savefile->WriteFloat( timeSlice ); // float timeSlice
	savefile->WriteBool( gameOver ); // bool gameOver

	savefile->WriteInt( currentLevel ); // int currentLevel
	savefile->WriteBool( updateScore ); // bool updateScore
	savefile->WriteBool( bearHitTarget ); // bool bearHitTarget

	savefile->WriteFloat( bearScale ); // float bearScale
	savefile->WriteBool( bearIsShrinking ); // bool bearIsShrinking
	savefile->WriteInt( bearShrinkStartTime ); // int bearShrinkStartTime

	savefile->WriteFloat( turretAngle ); // float turretAngle
	savefile->WriteFloat( turretForce ); // float turretForce

	int numberOfEnts = entities.Num(); // idList<BSEntity*> entities
	savefile->WriteInt( numberOfEnts );
	for ( int i=0; i<numberOfEnts; i++ ) {
		entities[i]->WriteToSaveGame( savefile );
	}

	savefile->WriteInt( entities.FindIndex( turret ) ); // BSEntity * turret
	savefile->WriteInt( entities.FindIndex( bear ) ); // BSEntity * bear
	savefile->WriteInt( entities.FindIndex( helicopter ) ); // BSEntity * helicopter
	savefile->WriteInt( entities.FindIndex( goal ) );// BSEntity * goal
	savefile->WriteInt( entities.FindIndex( gunblast ) ); // BSEntity * gunblast
	//savefile->WriteInt( entities.FindIndex( wind ) );

	savefile->WriteInt( bananasLeft ); // int bananasLeft
	savefile->WriteInt( baseEnemyY ); // int baseEnemyY

	savefile->WriteInt( highScore ); // int highScore

	savefile->WriteCheckSizeMarker();
}

/*
=============================
idGameBearShootWindow::ReadFromSaveGame
=============================
*/
void idGameBearShootWindow::ReadFromSaveGame( idRestoreGame *savefile ) {
	idWindow::ReadFromSaveGame( savefile );

	gamerunning.ReadFromSaveGame( savefile ); // idWinBool gamerunning
	onFire.ReadFromSaveGame( savefile ); // idWinBool onFire
	onContinue.ReadFromSaveGame( savefile ); // idWinBool onContinue
	onNewGame.ReadFromSaveGame( savefile ); // idWinBool onNewGame

	savefile->ReadFloat( timeSlice ); // float timeSlice
	savefile->ReadBool( gameOver ); // bool gameOver

	savefile->ReadInt( currentLevel ); // int currentLevel
	savefile->ReadBool( updateScore ); // bool updateScore
	savefile->ReadBool( bearHitTarget ); // bool bearHitTarget

	savefile->ReadFloat( bearScale ); // float bearScale
	savefile->ReadBool( bearIsShrinking ); // bool bearIsShrinking
	savefile->ReadInt( bearShrinkStartTime ); // int bearShrinkStartTime

	savefile->ReadFloat( turretAngle ); // float turretAngle
	savefile->ReadFloat( turretForce ); // float turretForce

	int numberOfEnts; // idList<BSEntity*> entities
	savefile->ReadInt( numberOfEnts );
	for ( int i=0; i<numberOfEnts; i++ ) {
		entities[i]->ReadFromSaveGame( savefile, this );
	}


	int index;
	savefile->ReadInt( index ); // BSEntity * turret
	turret = entities[index];
	savefile->ReadInt( index ); // BSEntity * bear
	bear = entities[index];
	savefile->ReadInt( index ); // BSEntity * helicopter
	helicopter = entities[index];
	savefile->ReadInt( index ); // BSEntity * goal
	goal = entities[index];
	savefile->ReadInt( index ); // BSEntity * gunblast
	gunblast = entities[index];
	//savefile->ReadInt( index );
	//wind = entities[index];

	savefile->ReadInt( bananasLeft ); // int bananasLeft
	savefile->ReadInt( baseEnemyY ); // int baseEnemyY

	savefile->ReadInt( highScore ); // int highScore

	savefile->ReadCheckSizeMarker();
}

/*
=============================
idGameBearShootWindow::ResetGameState
=============================
*/
void idGameBearShootWindow::ResetGameState() {
	gamerunning = false;
	gameOver = false;
	onFire = false;
	onContinue = false;
	onNewGame = false;

	// Game moves forward 16 milliseconds every frame
	timeSlice = 0.016f;
	//timeRemaining = 60.f;
	//goalsHit = 0;
	updateScore = false;
	bearHitTarget = false;
	currentLevel = 0;
	turretAngle = 0.f;
	turretForce = 200.f;
	//windForce = 0.f;
	//windUpdateTime = 0;

	bearIsShrinking = false;
	bearShrinkStartTime = 0;
	bearScale = 1.f;

	//BC
	bananasLeft = 2;

	gui->SetStateInt("highscore", highScore); //for the title screen high score text.
}

/*
=============================
idGameBearShootWindow::CommonInit
=============================
*/
void idGameBearShootWindow::CommonInit() {
	BSEntity *			ent;

	// Precache sounds
	
	
	
	

	// Precache dynamically used materials
	//declManager->FindMaterial( "game/bearshoot/helicopter_broken" );
	//declManager->FindMaterial( "game/bearshoot/goal_dead" );
	//declManager->FindMaterial( "game/bearshoot/gun_blast" );

	ResetGameState();

	ent = new BSEntity( this );
	turret = ent;
	ent->SetMaterial( "game/bearshoot/turret" );
	//ent->SetSize( 160, 160 ); //BC was 272 144
	//ent->position.x = 16; //BC
	//ent->position.y = 260;
	ent->SetSize(80, 80);
	ent->position.x = 56;
	ent->position.y = 300;
	entities.Append( ent );

	ent = new BSEntity( this );
	ent->SetMaterial( "game/bearshoot/turret_base" );
	ent->SetSize( 64, 64 ); //BC was 144 160
	ent->position.x = 64; //BC
	ent->position.y = 310; //BC
	entities.Append( ent );

	ent = new BSEntity( this );
	bear = ent;
	ent->SetMaterial( "game/bearshoot/bear" );
	ent->SetSize( BEAR_SIZE, BEAR_SIZE );
	ent->SetVisible( false );
	ent->position.x = 0;
	ent->position.y = 0;
	entities.Append( ent );

	ent = new BSEntity( this );
	helicopter = ent;
	ent->SetMaterial( "game/bearshoot/helicopter" );
	ent->SetSize( 64, 64 );
	ent->position.x = 550;
	ent->position.y = 100;
	entities.Append( ent );

	ent = new BSEntity( this );
	goal = ent;
	ent->SetMaterial( "game/bearshoot/powerup" ); //BC banana powerup
	ent->SetSize( 64, 64 );
	ent->position.x = 550;
	ent->position.y = 164;
	entities.Append( ent );

	//ent = new BSEntity( this );
	//wind = ent;
	//ent->SetMaterial( "game/bearshoot/wind" );
	//ent->SetSize( 100, 40 );
	//ent->position.x = 500;
	//ent->position.y = 430;
	//entities.Append( ent );

	ent = new BSEntity( this );
	gunblast = ent;
	ent->SetMaterial( "game/bearshoot/bomb" );
	ent->SetSize(BOMBSIZE, BOMBSIZE);
	ent->SetVisible( false );
	entities.Append( ent );
}

// SW 3rd March 2025:
// We pass through a special event from idGameBlock::Think so that the game only updates when the game block is capable of thinking.
// This means that the game should pause correctly whenever the world is paused.
void idGameBearShootWindow::RunNamedEvent(const char* namedEvent)
{
	idWindow::RunNamedEvent(namedEvent);

	if (idStr::Icmp(namedEvent, "updateGame") == 0)
	{
		UpdateGame();
	}
}

/*
=============================
idGameBearShootWindow::HandleEvent
=============================
*/
const char *idGameBearShootWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	int key = event->evValue;

	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = idWindow::HandleEvent(event, updateVisuals);

	if ( event->evType == SE_KEY ) {

		if ( !event->evValue2 ) {
			return ret;
		}
		if ( key == K_MOUSE1) {
			// Mouse was clicked
		} else {
			return ret;
		}
	}

	return ret;
}

/*
=============================
idGameBearShootWindow::ParseInternalVar
=============================
*/
bool idGameBearShootWindow::ParseInternalVar(const char *_name, idParser *src) {
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

	return idWindow::ParseInternalVar(_name, src);
}

/*
=============================
idGameBearShootWindow::GetWinVarByName
=============================
*/
idWinVar *idGameBearShootWindow::GetWinVarByName(const char *_name, bool winLookup, drawWin_t** owner) {
	idWinVar *retVar = NULL;

	if ( idStr::Icmp(_name, "gamerunning") == 0 ) {
		retVar = &gamerunning;
	} else	if ( idStr::Icmp(_name, "onFire") == 0 ) {
		retVar = &onFire;
	} else	if ( idStr::Icmp(_name, "onContinue") == 0 ) {
		retVar = &onContinue;
	} else	if ( idStr::Icmp(_name, "onNewGame") == 0 ) {
		retVar = &onNewGame;
	}

	if(retVar) {
		return retVar;
	}

	return idWindow::GetWinVarByName(_name, winLookup, owner);
}

/*
=============================
idGameBearShootWindow::PostParse
=============================
*/
void idGameBearShootWindow::PostParse() {
	idWindow::PostParse();
}

/*
=============================
idGameBearShootWindow::Draw
=============================
*/
void idGameBearShootWindow::Draw(int time, float x, float y) {
	int i;

	for( i = entities.Num()-1; i >= 0; i-- ) {
		entities[i]->Draw(dc);
	}
}

/*
=============================
idGameBearShootWindow::UpdateTurret
=============================
*/
void idGameBearShootWindow::UpdateTurret() {
	idVec2	pt;
	idVec2	turretOrig;
	idVec2	right;
	float	dot, angle;

	pt.x = gui->CursorX();
	pt.y = gui->CursorY();
	turretOrig.Set( 80.f, 348.f ); //BC was 80 348

	pt = pt - turretOrig;
	pt.NormalizeFast();

	right.x = 1.f;
	right.y = 0.f;

	dot = pt * right;

	angle = RAD2DEG( acosf( dot ) );

	turretAngle = idMath::ClampFloat( 0.f, 90.f, angle );
}

/*
=============================
idGameBearShootWindow::UpdateBear
=============================
*/
void idGameBearShootWindow::UpdateBear() {
	int time = gui->GetTime();
	bool startShrink = false;

	// Apply gravity
	bear->velocity.y += BEAR_GRAVITY * timeSlice;

	// Apply wind
	//bear->velocity.x += windForce * timeSlice;

	// Check for collisions
	if ( !bearHitTarget && !gameOver )
	{
		idVec2 bearCenter;
		bool	collision = false;

		bearCenter.x = bear->position.x + bear->width/2;
		bearCenter.y = bear->position.y + bear->height/2;

		//BC removing this hardcoded stuff.
		//if ( bearCenter.x > (helicopter->position.x + 16) && bearCenter.x < (helicopter->position.x + helicopter->width - 29) )
		//{
		//	if ( bearCenter.y > (helicopter->position.y + 12) && bearCenter.y < (helicopter->position.y + helicopter->height - 7) )
		//	{
		//		collision = true;
		//	}
		//}


		//BC check free-banana collision.
		if ((bearCenter.x > goal->position.x) && (bearCenter.x < goal->position.x + goal->width)
			&& (bearCenter.y > goal->position.y) && (bearCenter.y < goal->position.y + goal->height)
			&& goal->visible)
		{
			//Get free banana.
			goal->SetVisible(false);
			bananasLeft += 1;
			gui->SetStateString("bananasLeft", va("%i", bananasLeft));

			session->sw->PlayShaderDirectly("arcade_powerup_spawn", 4);
			
		}


		//BC check bomb collision.
		if ((bearCenter.x > gunblast->position.x) && (bearCenter.x < gunblast->position.x + gunblast->width)
			&& (bearCenter.y > gunblast->position.y) && (bearCenter.y < gunblast->position.y + gunblast->height)
			&& gunblast->visible)
		{
			//Hit bomb.
			gunblast->SetVisible(false);
			bear->velocity.Zero();
			bear->velocity.y = PRIMATE_FALLSPEED;

			session->sw->PlayShaderDirectly("arcade_missedball", 3);
		}


		//BC simple collision. just use the sprite size.
		if ((bearCenter.x > helicopter->position.x)  && (bearCenter.x < helicopter->position.x + helicopter->width)
			&& (bearCenter.y > helicopter->position.y) && (bearCenter.y < helicopter->position.y + helicopter->height))
		{
			collision = true;
		}


		if ( collision ) {
			// balloons pop and bear tumbles to ground
			//helicopter->SetMaterial( "game/bearshoot/helicopter_broken" );
			helicopter->rotation = -90; //BC make primate lie down
			helicopter->velocity.y = PRIMATE_FALLSPEED;
			
			//goal->velocity.y = 230.f;

			session->sw->PlayShaderDirectly( "arcade_getgem" , 3);

			bear->SetVisible( false );
			if ( bear->velocity.x > 0 ) {
				bear->velocity.x *= -1.f;
			}
			bear->velocity *= 0.666f;
			bearHitTarget = true;
			updateScore = true;
			startShrink = true;
		}
	}

	// Check for ground collision
	if ( bear->position.y > 480 ) //BC was 380
	{
		bear->position.y = 480;

		startShrink = true;


		//if ( bear->velocity.Length() < 25 ) {
		//	bear->velocity.Zero();
		//} else {
		//	startShrink = true;
		//
		//	bear->velocity.y *= -1.f;
		//	bear->velocity *= 0.5f;
		//
		//	if ( bearScale ) {
		//		session->sw->PlayShaderDirectly( "arcade_balloonpop" );
		//	}
		//}
	}

	// Bear rotation is based on velocity
	float angle;
	idVec2 dir;

	dir = bear->velocity;
	dir.NormalizeFast();

	angle = RAD2DEG( atan2( dir.x, dir.y ) );
	bear->rotation = angle - 90;

	// Update Bear scale
	if ( bear->position.x > 650 )
	{
		startShrink = true; //BC if bullet leaves right side of screen, then start its shrink process.
	}

	if ( !bearIsShrinking && bearScale && startShrink ) {
		bearShrinkStartTime = time;
		bearIsShrinking = true;

		if (!bearHitTarget)
		{
			session->sw->PlayShaderDirectly("arcade_curse", 3);
		}
	}

	if ( bearIsShrinking ) {
		if ( bearHitTarget ) {
			bearScale = 1 - ( (float)(time - bearShrinkStartTime) / BEAR_SHRINK_TIME );
		} else {
			bearScale = 1 - ( (float)(time - bearShrinkStartTime) / BEAR_SHRINK_TIME_FAIL);
		}
		bearScale *= BEAR_SIZE;
		bear->SetSize( bearScale, bearScale );

		if ( bearScale < 0 )
		{
			if (!bearHitTarget)
			{
				//BC lose banana if banana fails to hit target.
				bananasLeft--;
				if (bananasLeft < 0)
				{
					gui->SetStateString("bananasLeft", "0");
				}
				else
				{
					gui->SetStateString("bananasLeft", va("%i", bananasLeft));
				}
			}

			//BC check for game over.
			if (bananasLeft < 0)
			{
				gui->HandleNamedEvent("GameOver");
				DoHighscoreLogic();
				return;
			}

			

			gui->HandleNamedEvent( "EnableFireButton" ); //BC reenable the fire button.
			bearIsShrinking = false;
			bearScale = 0.f;

			if ( bearHitTarget )
			{
				//Reset the game objects.

				//goal->SetMaterial( "game/bearshoot/bear" ); //BC Banana texture.
				goal->position.x = 550;
				goal->position.y = 164;
				goal->velocity.Zero();
				goal->velocity.y = 0;
				goal->entColor.w = 0.f;
				goal->fadeIn = true;
				goal->fadeOut = false;

				helicopter->SetVisible( true );
				helicopter->SetMaterial( "game/bearshoot/helicopter" );
				helicopter->position.x = 550;
				helicopter->position.y = 100;
				helicopter->velocity.Zero();
				helicopter->velocity.y = 0;
				helicopter->entColor.w = 0.f;
				helicopter->fadeIn = true;
				helicopter->fadeOut = false;
				helicopter->rotation = 0; //BC

				InitPositions();

				session->sw->PlayShaderDirectly("arcade_nextlevel", 3);				
			}
		}
	}
}

void idGameBearShootWindow::InitPositions()
{
	idRandom rnd(gui->GetTime());
	

	//BC set the game object positions.
	bear->position.x = 320;
	bear->position.y = 240;


	//Freebanana location.
	goal->position.x = 200 + rnd.RandomInt(150);
	goal->position.y = 100 + rnd.RandomInt(150);

	//The enemy primate.
	helicopter->position.x = 400 + rnd.RandomInt(170);
	helicopter->position.y = 50 + rnd.RandomInt(250);
	baseEnemyY = helicopter->position.y;

	

	if (currentLevel > 1 && currentLevel % 2 == 0)
	{
		helicopter->velocity.y = PRIMATE_BOBSPEED;
	}

	//Handle powerup spawning.
	if (currentLevel <= 1)
	{
		//First level. don't spawn powerup.
		goal->SetVisible(false);
	}
	else if (bananasLeft <= 0)
	{
		//if player has no more bananas left, always spawn the powerup.
		goal->SetVisible(true);
	}
	else if (currentLevel % 3 == 0)
	{
		//guarantee powerup every XX levels.
		goal->SetVisible(true);
	}
	else
	{
		goal->SetVisible(false);
	}

	if (!goal->visible && currentLevel % 2 == 0)
	{
		//The bomb.
		gunblast->position.x = 200 + rnd.RandomInt(150);
		gunblast->position.y = 100 + rnd.RandomInt(150);
		gunblast->velocity.y = BOMB_BOBSPEED;
		gunblast->SetVisible(true);
	}
	else
	{
		gunblast->SetVisible(false);
	}
}

/*
=============================
idGameBearShootWindow::UpdateHelicopter
=============================
*/
void idGameBearShootWindow::UpdateHelicopter() {

	if ( bearHitTarget && bearIsShrinking )
	{
		//BC if target is hit, then ...

		if ( helicopter->velocity.y != 0 && helicopter->position.y > 500 /*bc was 264*/ )
		{
			helicopter->velocity.y = 0;
			//goal->velocity.y = 0;

			//helicopter->SetVisible( false );
			//goal->SetMaterial( "game/bearshoot/goal_dead" );
			//session->sw->PlayShaderDirectly( "arcade_beargroan", 1 );

			//helicopter->fadeOut = true;
			//goal->fadeOut = true;
		}
	}
	else if ( currentLevel > 1 && currentLevel %2 == 0 )
	{
		//BC bob up and down on even-numbered levels.
	
		int height = helicopter->position.y;
		//float speed = (currentLevel-1) * 30;
		float speed = PRIMATE_BOBSPEED;
	
		if ( height > baseEnemyY + 50) {
			helicopter->velocity.y = -speed;
			//goal->velocity.y = -speed;
		} else if ( height < baseEnemyY - 50) {
			helicopter->velocity.y = speed;
			//goal->velocity.y = speed;
		}
	}

	if (gunblast->visible)
	{
		//Handle bomb movement.
		if (gunblast->position.y < 75)
		{
			gunblast->velocity.y = BOMB_BOBSPEED;
		}
		else if (gunblast->position.y > 250)
		{
			gunblast->velocity.y = -BOMB_BOBSPEED;
		}
	}
}

/*
=============================
idGameBearShootWindow::UpdateButtons
=============================
*/
void idGameBearShootWindow::UpdateButtons() {

	if ( onFire )
	{
		idVec2 vec;

		gui->HandleNamedEvent( "DisableFireButton" );
		session->sw->PlayShaderDirectly( "arcade_brickhit" , 3);

		bear->SetVisible( true );
		bearScale = 1.f;
		bear->SetSize( BEAR_SIZE, BEAR_SIZE );

		vec.x = idMath::Cos( DEG2RAD(turretAngle) );
		vec.x += ( 1 - vec.x ) * 0.18f;
		vec.y = -idMath::Sin( DEG2RAD(turretAngle) );

		turretForce = bearTurretForce.GetFloat();

		bear->position.x = 80 + ( 96 * vec.x );
		bear->position.y = 334 + ( 96 * vec.y );
		bear->velocity.x = vec.x * turretForce;
		bear->velocity.y = vec.y * turretForce;

		//gunblast->position.x = 55 + ( 96 * vec.x );
		//gunblast->position.y = 310 + ( 100 * vec.y );
		////gunblast->SetVisible( true ); //BC
		//gunblast->entColor.w = 1.f;
		//gunblast->rotation = turretAngle;
		//gunblast->fadeOut = true;

		bearHitTarget = false;

		onFire = false;

		
		
	}
}

/*
=============================
idGameBearShootWindow::UpdateScore
=============================
*/
void idGameBearShootWindow::UpdateScore() {

	if ( gameOver ) {
		gui->HandleNamedEvent( "GameOver" );
		DoHighscoreLogic();
		return;
	}

	//goalsHit++;
	//gui->SetStateString( "player_score", va("%i", goalsHit ) );

	// Check for level progression
	//if ( !(goalsHit % 5) )
	currentLevel++;

	gui->SetStateString("current_level", va("%i", currentLevel));	
	gui->HandleNamedEvent("levelComplete");

	

	//BC remove old level system.
	//if (goalsHit % 5 == 0)
	//{
	//	currentLevel++;
	//	//timeRemaining += 30;
	//}

	if (currentLevel >= 25 && common->g_SteamUtilities && common->g_SteamUtilities->IsSteamInitialized())
	{
		common->g_SteamUtilities->SetAchievement("ach_primatez");
	}
}

/*
=============================
idGameBearShootWindow::UpdateGame
=============================
*/
void idGameBearShootWindow::UpdateGame() {
	int i;

	if ( onNewGame )
	{
		//On start new game.

		ResetGameState();

		goal->position.x = 550;
		goal->position.y = 164;
		goal->velocity.Zero();
		helicopter->position.x = 550;
		helicopter->position.y = 100;
		helicopter->velocity.Zero();
		bear->SetVisible( false );

		bearTurretAngle.SetFloat( 0.f );
		bearTurretForce.SetFloat( 200.f );

		InitPositions();

		gamerunning = true;
	}
	if ( onContinue )
	{
		gameOver = false;
		//timeRemaining = 60.f;

		onContinue = false;
	}

	if (gamerunning == true)
	{
		int current_time = gui->GetTime();
		idRandom rnd( current_time );

		// Check for button presses
		UpdateButtons();

		if ( bear ) {
			UpdateBear();
		}
		if ( helicopter && goal ) {
			UpdateHelicopter();
		}


		

		//common->Printf("cursorX %f  heliX %f   cursorY %f heliY%f\n", gui->CursorX(), helicopter->position.x, gui->CursorY(), helicopter->position.y);

		

		//BC no more wind.
		// Update Wind
		//if ( windUpdateTime < current_time ) {
		//	float	scale;
		//	int		width;
		//
		//	windForce = rnd.CRandomFloat() * ( MAX_WINDFORCE * 0.75f );
		//	if (windForce > 0) {
		//		windForce += ( MAX_WINDFORCE * 0.25f );
		//		wind->rotation = 0;
		//	} else {
		//		windForce -= ( MAX_WINDFORCE * 0.25f );
		//		wind->rotation = 180;
		//	}
		//
		//	scale = 1.f - (( MAX_WINDFORCE - idMath::Fabs(windForce) ) / MAX_WINDFORCE);
		//	width = 100*scale;
		//
		//	if ( windForce < 0 ) {
		//		wind->position.x = 500 - width + 1;
		//	} else {
		//		wind->position.x = 500;
		//	}
		//	wind->SetSize( width, 40 );
		//
		//	windUpdateTime = current_time + 7000 + rnd.RandomInt(5000);
		//}

		// Update turret rotation angle
		if ( turret )
		{
			float forceLerp;

			turretAngle = bearTurretAngle.GetFloat();
			turret->rotation = turretAngle;

			//BC resize sprite according to force.
			turretForce = bearTurretForce.GetFloat();
			forceLerp = (turretForce - FORCE_MIN) / (FORCE_MAX - FORCE_MIN);

			turret->SetSize(idMath::Lerp(80,200,forceLerp), idMath::Lerp(80, 200, forceLerp));
			turret->position.x = idMath::Lerp(56, -4, forceLerp);
			turret->position.y = idMath::Lerp(300, 240, forceLerp);
		}

		for( i = 0; i < entities.Num(); i++ ) {
			entities[i]->Update( timeSlice );
		}

		// Update countdown timer
		//timeRemaining -= timeSlice;
		//timeRemaining = idMath::ClampFloat( 0.f, 99999.f, timeRemaining );
		//gui->SetStateString( "time_remaining", va("%2.1f", timeRemaining ) );

		//if ( timeRemaining <= 0.f && !gameOver ) {
		//	gameOver = true;
		//	updateScore = true;
		//}

		if ( updateScore ) {
			UpdateScore();
			updateScore = false;
		}
	}
}


void idGameBearShootWindow::DoHighscoreLogic()
{
	idStr highscorestringToDisplay = "";

	if (currentLevel > highScore)
	{
		//Player has a new high score.
		highscorestringToDisplay = common->GetLanguageDict()->GetString("#str_gui_arcade_newhighscore");
		highScore = currentLevel;
		gameLocal.persistentLevelInfo.SetInt("bearShootHighScore", highScore);
	}
	else
	{
		highscorestringToDisplay = idStr::Format(common->GetLanguageDict()->GetString("#str_gui_arcade_currenthighscore"), highScore);
	}

	gui->SetStateString("highscoretext", highscorestringToDisplay.c_str());

	gui->SetStateInt("highscore", highScore); //for the title screen high score text.
}
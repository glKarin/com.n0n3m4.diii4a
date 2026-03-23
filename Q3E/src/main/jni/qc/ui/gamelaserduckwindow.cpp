
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../framework/Session_local.h"

#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "gamelaserduckwindow.h"


#define BEAR_GRAVITY 240
#define BEAR_SIZE 24.f
#define BEAR_SHRINK_TIME 2000.f

#define MAX_WINDFORCE 100.f


/*
*****************************************************************************
* LaserDuckEntity	
****************************************************************************
*/
LaserDuckEntity::LaserDuckEntity(idGameLaserDuckWindow* _game) {
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

LaserDuckEntity::~LaserDuckEntity() {
}

/*
======================
LaserDuckEntity::WriteToSaveGame
======================
*/
void LaserDuckEntity::WriteToSaveGame( idFile *savefile ) {

	game->WriteSaveGameString( materialName, savefile );

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
LaserDuckEntity::ReadFromSaveGame
======================
*/
void LaserDuckEntity::ReadFromSaveGame( idFile *savefile, idGameLaserDuckWindow* _game ) {
	game = _game;

	game->ReadSaveGameString( materialName, savefile );
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
LaserDuckEntity::SetMaterial
======================
*/
void LaserDuckEntity::SetMaterial(const char* name)
{
	materialName = name;
	material = declManager->FindMaterial( name );
	material->SetSort( SS_GUI );
}

/*
======================
LaserDuckEntity::SetSize
======================
*/
void LaserDuckEntity::SetSize( float _width, float _height ) {
	width = _width;
	height = _height;
}

/*
======================
LaserDuckEntity::SetVisible
======================
*/
void LaserDuckEntity::SetVisible( bool isVisible ) {
	visible = isVisible;
}

/*
======================
LaserDuckEntity::Update
======================
*/
void LaserDuckEntity::Update( float timeslice )
{	
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
LaserDuckEntity::Draw
======================
*/
void LaserDuckEntity::Draw(idDeviceContext *dc)
{
	if ( visible )
	{
		dc->DrawMaterialRotated( position.x, position.y, width, height, material, entColor, 1.0f, 1.0f, DEG2RAD(rotation) );
	}
}




idGameLaserDuckWindow::idGameLaserDuckWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idGameLaserDuckWindow::idGameLaserDuckWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idGameLaserDuckWindow::~idGameLaserDuckWindow() {
	entities.DeleteContents(true);
}


void idGameLaserDuckWindow::WriteToSaveGame( idFile *savefile )
{
}


void idGameLaserDuckWindow::ReadFromSaveGame( idFile *savefile )
{
}


void idGameLaserDuckWindow::ResetGameState() {
	gamerunning = false;
	onNewGame = false;


}


void idGameLaserDuckWindow::CommonInit() {
	LaserDuckEntity *			ent;

	// Precache sounds
	//declManager->FindSound( "arcade_beargroan" );
	//declManager->FindMaterial( "game/bearshoot/helicopter_broken" );

	declManager->FindMaterial( "game/laserduck/duck" );

	ResetGameState();

	ent = new LaserDuckEntity( this );
	duck = ent;
	ent->SetMaterial( "game/laserduck/duck" );
	ent->SetSize( BEAR_SIZE, BEAR_SIZE );
	ent->SetVisible( true );
	ent->position.x = 32;
	ent->position.y = 32;
	entities.Append( ent );
}



const char *idGameLaserDuckWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals)
{
	int key = event->evValue;

	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = idWindow::HandleEvent(event, updateVisuals);

	if ( event->evType == SE_KEY )
	{
		if ( !event->evValue2 )
		{
			return ret;
		}

		if ( key == K_MOUSE1)
		{
			// Mouse was clicked	
		}
		else
		{
			return ret;
		}
	}

	return ret;
}


bool idGameLaserDuckWindow::ParseInternalVar(const char *_name, idParser *src) {
	if ( idStr::Icmp(_name, "gamerunning") == 0 )
	{
		gamerunning = src->ParseBool();
		return true;
	}
		
	if ( idStr::Icmp(_name, "onNewGame") == 0 )
	{
		onNewGame = src->ParseBool();
		return true;
	}

	return idWindow::ParseInternalVar(_name, src);
}



idWinVar *idGameLaserDuckWindow::GetWinVarByName(const char *_name, bool winLookup, drawWin_t** owner)
{
	idWinVar *retVar = NULL;

	if ( idStr::Icmp(_name, "gamerunning") == 0 )
	{
		retVar = &gamerunning;
	}
	else if ( idStr::Icmp(_name, "onNewGame") == 0 )
	{
		retVar = &onNewGame;
	}

	if(retVar)
	{
		return retVar;
	}

	return idWindow::GetWinVarByName(_name, winLookup, owner);
}



void idGameLaserDuckWindow::PostParse()
{
	idWindow::PostParse();
}



void idGameLaserDuckWindow::Draw(int time, float x, float y)
{
	int i;

	//Update the game every frame before drawing
	UpdateGame();

	for( i = entities.Num()-1; i >= 0; i-- )
	{
		entities[i]->Draw(dc);
	}
}



const char *idGameLaserDuckWindow::Activate(bool activate)
{
	return "";
}



void idGameLaserDuckWindow::UpdateButtons()
{	
}



void idGameLaserDuckWindow::UpdateGame()
{

	//this->duck->position.x = this->duck->position.x + 1;

}

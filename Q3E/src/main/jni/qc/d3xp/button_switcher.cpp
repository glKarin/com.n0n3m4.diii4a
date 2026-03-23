
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

const idEventDef EV_ButtonSwitcher_activate( "buttonswitcheractivate", "f" );

CLASS_DECLARATION( idAnimated, idButtonSwitcher )
	EVENT( EV_ButtonSwitcher_activate,			idButtonSwitcher::Event_buttonswitcheractivate)
END_CLASS

void idButtonSwitcher::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteBool(active);
}

void idButtonSwitcher::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadBool(active);
}

void idButtonSwitcher::Spawn( void )
{
	GetPhysics()->SetContents( 0 );
	
	this->noGrab = true;
	this->isFrobbable = false;

	state = 0;
	active = false;
}

void idButtonSwitcher::Event_buttonswitcheractivate( int value )
{
	Event_PlayAnim( "turn", 4);

	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->isFrobbable = true;
	active = true;
}



void idButtonSwitcher::OnFrob( idEntity* activator )
{
	if (!active)
		return;

	StartSound( "snd_press", SND_CHANNEL_BODY, 0, false, NULL );

	if (state == 0)
	{
		state = 1;

		Event_PlayAnim( "_0_to_1", 4);
		CallScript("script1");
		SetSkin(declManager->FindSkin( "skins/button_switcher/skin_1" ));
	}
	else if (state == 1)
	{
		state = 2;

		Event_PlayAnim( "_1_to_2", 4);
		CallScript("script2");
		SetSkin(declManager->FindSkin( "skins/button_switcher/skin_2" ));
	}
	else
	{
		state = 0;

		Event_PlayAnim( "_2_to_0", 4);
		CallScript("script0");
		SetSkin(declManager->FindSkin( "skins/button_switcher/skin" ));
	}
}

void idButtonSwitcher::CallScript(const char* name)
{
	idStr scriptName = spawnArgs.GetString( name );

	if (scriptName.Length() <= 0)
		return;

	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction( scriptName );

	if ( !scriptFunction )
		return;

	idThread *thread;
	thread = new idThread( scriptFunction );
	thread->DelayedStart( 0 );	
}

/*
void idLever::Think( void )
{
	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}*/


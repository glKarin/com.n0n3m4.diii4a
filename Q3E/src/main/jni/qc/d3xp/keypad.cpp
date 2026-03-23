
//  viola, the keypad
//  0 QZ    1 ABC   2 DEF
//  3 GHI   4 JKL   5 MNO
//  6 PRS   7 TUV   8 WXY


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define PRESSTIME	100

const idEventDef EV_keypadopen(		"keypadopen", "d" );

CLASS_DECLARATION( idAnimatedEntity, idKeypad )
	EVENT( EV_keypadopen,		idKeypad::Event_keypadopen)
	
END_CLASS

void idKeypad::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(counter);
	savefile->WriteInt(nextStateTime);
	savefile->WriteObject(frobcubeMain);

	for (int i = 0; i < 9; i++)
	{
		savefile->WriteObject(frobcubes[i]);
		savefile->WriteSkin(skin_glow[i]);
		savefile->WriteInt(transitions[i]);
	}

	savefile->WriteInt( keys.Num() );
	for (int i = 0; i < keys.Num(); i++ )
	{
		savefile->WriteString( keys[ i ] );
	}

	for (int i = 0; i < 4; i++)
	{
		savefile->WriteInt(keycode[i]);
		savefile->WriteInt(input[i]);
	}

	savefile->WriteInt(keyIndex);
	savefile->WriteObject(bluebox);
}

void idKeypad::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(counter);
	savefile->ReadInt(nextStateTime);
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcubeMain));

	for (int i = 0; i < 9; i++)
	{
		savefile->ReadObject(reinterpret_cast<idClass *&>(frobcubes[i]));
		savefile->ReadSkin(skin_glow[i]);
		savefile->ReadInt(transitions[i]);
	}

	int keyAmount;
	savefile->ReadInt( keyAmount );
	for (int i = 0; i < keyAmount; i++ )
	{
		idStr keyStr;
		savefile->ReadString( keyStr );
		keys.Append( keyStr );
	}

	for (int i = 0; i < 4; i++)
	{
		savefile->ReadInt(keycode[i]);
		savefile->ReadInt(input[i]);
	}

	savefile->ReadInt(keyIndex);
	savefile->ReadObject(reinterpret_cast<idClass *&>(bluebox));
}

void idKeypad::Event_keypadopen( int value )
{
	if (state == CONFIRM_SUCCESS)
		return;

	if (value >= 1)
	{
		int i;


		GenerateKey();

		Event_PlayAnim( "opening" , 4 );

		state = ACTIVE;

		for (i = 0; i < 9; i++)
		{
			this->frobcubes[i]->isFrobbable = true;
		}

		this->frobcubeMain->GetPhysics()->SetContents(0);
	}
	else
	{
		this->frobcubeMain->GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
		this->frobcubeMain->GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

		if (state == OFF)
		{
			return;
		}

		int i;
		Event_PlayAnim( "closing" , 4);

		state = OFF;

		for (i = 0; i < 9; i++)
		{
			this->frobcubes[i]->isFrobbable = false;
		}

		bluebox->Hide();
	}
}

//call this once.
void idKeypad::GenerateDictionary( void )
{
	void *buffer;
	idStr rawDictionary;
	int i;

	if (fileSystem->ReadFile( "keycodes.txt", &buffer) > 0)
	{
		rawDictionary = (char *) buffer;
		fileSystem->FreeFile( buffer );
	}
	else
	{
		gameLocal.Error("cannot load keycodes.txt");
		return;
	}

	rawDictionary.StripTrailingWhitespace(); //remove whitespace.
	rawDictionary.StripLeading( ' ' );		//remove whitespace.
	rawDictionary.ToUpper();

	keys.Clear();
	for (i = rawDictionary.Length(); i >= 0; i--)
	{
		if (rawDictionary[i] == '\n' || rawDictionary[i] == '\r')
		{
			if (rawDictionary.Mid(i+1, rawDictionary.Length()).IsEmpty())
			{
				rawDictionary = rawDictionary.Left(i);
				continue;
			}

			keys.AddUnique(rawDictionary.Mid(i+1, rawDictionary.Length()   ));

			rawDictionary = rawDictionary.Left(i); //strip.
		}

		if (i <= 0)
		{
			keys.AddUnique(rawDictionary.Mid(0, rawDictionary.Length()   ));
		}
	}

	
}

void idKeypad::GenerateKey( void )
{
	int i;
	keyIndex = gameLocal.random.RandomInt(keys.Num());

	for (i = 0; i < 4; i++)
	{
		if (keys[keyIndex][i] == 'Q' || keys[keyIndex][i] == 'Z')
		{
			keycode[i] = 0;
		}
		else if (keys[keyIndex][i] == 'A' || keys[keyIndex][i] == 'B' || keys[keyIndex][i] == 'C')
		{
			keycode[i] = 1;
		}
		else if (keys[keyIndex][i] == 'D' || keys[keyIndex][i] == 'E' || keys[keyIndex][i] == 'F')
		{
			keycode[i] = 2;
		}
		else if (keys[keyIndex][i] == 'G' || keys[keyIndex][i] == 'H' || keys[keyIndex][i] == 'I')
		{
			keycode[i] = 3;
		}
		else if (keys[keyIndex][i] == 'J' || keys[keyIndex][i] == 'K' || keys[keyIndex][i] == 'L')
		{
			keycode[i] = 4;
		}
		else if (keys[keyIndex][i] == 'M' || keys[keyIndex][i] == 'N' || keys[keyIndex][i] == 'O')
		{
			keycode[i] = 5;
		}
		else if (keys[keyIndex][i] == 'P' || keys[keyIndex][i] == 'R' || keys[keyIndex][i] == 'S')
		{
			keycode[i] = 6;
		}
		else if (keys[keyIndex][i] == 'T' || keys[keyIndex][i] == 'U' || keys[keyIndex][i] == 'V')
		{
			keycode[i] = 7;
		}
		else
		{
			keycode[i] = 8;
		}
	}

	this->bluebox->GetRenderEntity()->gui[0]->SetStateString("gui_parm0", keys[keyIndex]);
}

void idKeypad::Spawn( void )
{
	int i;
	idVec3 forward, right, up;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//spawn the buttons.
	for (i = 0; i < 9; i++)
	{
		idDict args;
		idVec3 pos = this->GetPhysics()->GetOrigin() + (forward * 1.7f);

		if (i <= 0)		 { pos += (up * 2) + (right * 2); }
		else if (i == 1) { pos += (up * 2); }
		else if (i == 2) { pos += (up * 2) + (right * -2); }
		else if (i == 3) { pos += (right * 2); }
		else if (i == 4) {	}
		else if (i == 5) { pos += (right * -2); }
		else if (i == 6) { pos += (up * -2) + (right * 2); }
		else if (i == 7) { pos += (up * -2); }
		else if (i == 8) { pos += (up * -2) + (right * -2); }

		args.SetVector( "origin", pos );
		args.Set( "model", "models/frob/cube_2x2.ase" );
		args.SetInt( "frobbable", 1 );
		args.SetInt( "corpse", 1 );
		args.SetInt( "noGrab", 1 );
		args.Set( "owner", this->GetName() );
		args.SetInt( "index", i );
		this->frobcubes[i] = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
		this->frobcubes[i]->isFrobbable = false;
		this->frobcubes[i]->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	}

	skin_glow[0] = declManager->FindSkin( "skins/keypad/qz_glow" );
	skin_glow[1] = declManager->FindSkin( "skins/keypad/abc_glow" );
	skin_glow[2] = declManager->FindSkin( "skins/keypad/def_glow" );
	skin_glow[3] = declManager->FindSkin( "skins/keypad/ghi_glow" );
	skin_glow[4] = declManager->FindSkin( "skins/keypad/jkl_glow" );
	skin_glow[5] = declManager->FindSkin( "skins/keypad/mno_glow" );
	skin_glow[6] = declManager->FindSkin( "skins/keypad/prs_glow" );
	skin_glow[7] = declManager->FindSkin( "skins/keypad/tuv_glow" );
	skin_glow[8] = declManager->FindSkin( "skins/keypad/wxy_glow" );


	idDict args1;
	args1.SetVector( "origin", this->GetPhysics()->GetOrigin() );
	args1.Set( "model", "models/keypad/tris_cm.ase" );
	args1.SetInt( "frobbable", 1 );
	args1.SetInt( "corpse", 1 );
	args1.SetInt( "noGrab", 1 );
	args1.Set( "owner", this->GetName() );
	args1.SetInt( "index", 16 );
	this->frobcubeMain = gameLocal.SpawnEntityType( idStaticEntity::Type, &args1 );
	this->frobcubeMain->isFrobbable = true;
	this->frobcubeMain->SetAngles(this->GetPhysics()->GetAxis().ToAngles());

	args1.Clear();
	args1.SetVector("origin", this->GetPhysics()->GetOrigin() );
	args1.Set("classname", "moveable_bluebox");
	args1.Set("gui", "guis/bluebox_pw.gui");
	gameLocal.SpawnEntityDef(args1, &bluebox);
	bluebox->Hide();

	for (i = 0; i < 4; i++)
	{
		input[i] = 0;
	}

	GenerateDictionary();
	

	

	this->nextStateTime = 0;
	this->counter = 0;
	this->state = OFF;
	BecomeActive( TH_THINK );
}

idStr idKeypad::GetJointViaIndex( int index )
{
	switch ( index )
	{
		case 0:		return "qz"; break;
		case 1:		return "abc"; break;
		case 2:		return "def"; break;
		case 3:		return "ghi"; break;
		case 4:		return "jkl"; break;
		case 5:		return "mno"; break;
		case 6:		return "prs"; break;
		case 7:		return "tuv"; break;
		default:	return "wxy"; break;
	}
}

void idKeypad::OnFrob( idEntity* activator )
{
	int index;
	
	if (!activator->spawnArgs.GetInt( "index", "", index))
	{
		return;
	}

	//common->Printf("%d\n", index);

	if (index == 16)
	{
		this->StartSound( "snd_error" , SND_CHANNEL_ANY, 0, false, NULL );
		return;
	}

	//PostEventSec( &EV_Turret_muzzleflashoff, MUZZLEFLASHTIME);
	if (index >= 0 && index <= 8)
	{
		jointHandle_t joint;

		joint = animator.GetJointHandle( GetJointViaIndex(index) );
		animator.SetJointPos(joint, JOINTMOD_LOCAL, idVec3(-0.8f, 0, 0) );

		transitions[index] = gameLocal.time + PRESSTIME;
	}


	StartSound( "snd_press", SND_CHANNEL_ANY, 0, false, NULL );
	
	if (counter <= 0)
	{
		Event_PlayAnim("marker0", 4);
	}
	else if (counter == 1)
	{
		Event_PlayAnim("marker1", 4);
	}
	else if (counter == 2)
	{
		Event_PlayAnim("marker2", 4);
	}
	else if (counter == 3)
	{
		Event_PlayAnim("marker3", 4);
	}

	input[counter] = index;

	counter++;

	if (counter >= 4)
	{
		int i;		
		for (i = 0; i < 9; i++)
		{
			this->frobcubes[i]->isFrobbable = false;
		}
		counter = 0;

		//TODO if successful.
		// gameLocal.GetLocalPlayer()->Event_useAmmo("ammo_hacktool", 1)
		if ( input[0] == keycode[0] && input[1] == keycode[1] && input[2] == keycode[2] && input[3] == keycode[3] )
		{
			//done success.
			bluebox->Hide();
			ActivateTargets( this );

			state = CONFIRM_SUCCESS;
			int doneTime = Event_PlayAnim("success", 4);
			
			nextStateTime = doneTime;

			SetSkin(declManager->FindSkin("skins/keypad/green"));	

			//add hud message.

			StartSound( "snd_deactivated", SND_CHANNEL_ANY, 0, false, NULL);

			return;
		}
		else
		{
			//fail.
			state = CONFIRM_FAIL;
			int doneTime = Event_PlayAnim("fail", 4);
			nextStateTime = doneTime;

			SetSkin(declManager->FindSkin("skins/keypad/red"));
			return;
		}
	}
}




void idKeypad::UpdateStates( void )
{
	if (state == CONFIRM_SUCCESS)
	{
		if (gameLocal.time > nextStateTime)
		{
			int i;
			

			//close ALL keypads.
			state = READY_TO_CLOSE;

			//Event_keypadopen( 0 );
			for ( i = 0; i < gameLocal.num_entities; i++ )
			{
				if ( !gameLocal.entities[ i ] )
					continue;

				if (!gameLocal.entities[i]->IsType(idKeypad::Type))
					continue;

				//skip self.
				//if (gameLocal.entities[i]->entityNumber == this->entityNumber)
				//	continue;
				
				static_cast<idKeypad *>( gameLocal.entities[i] )->Event_keypadopen(0);				
			}

			SetSkin(0);		
			
			return;
		}
	}
	else if (state == CONFIRM_FAIL)
	{
		if (gameLocal.time > nextStateTime)
		{
			//close up.
			int i;		
			for (i = 0; i < 9; i++)
			{
				this->frobcubes[i]->isFrobbable = true;
			}

			state = ACTIVE;
			SetSkin(0);

			return;
		}
	}
}

void idKeypad::Think( void )
{
	int i;

	//glows.
	if (state == ACTIVE)
	{
		if (gameLocal.GetLocalPlayer()->focusFrobEnt)
		{
			bool isHover = false;
			int i;
			int hoverIndex = gameLocal.GetLocalPlayer()->focusFrobEnt->entityNumber;

			for (i = 0; i < 9; i++)
			{
				if (hoverIndex == frobcubes[i]->entityNumber)
				{
					SetSkin(skin_glow[i]);
					isHover = true;
				}
			}

			if (!isHover)
			{
				SetSkin(0); //default.
				bluebox->Hide();
			}
			else
			{
				//show bluebox.
				bluebox->Show();

				idAngles remoteAng = gameLocal.GetLocalPlayer()->viewAngles;
				remoteAng.yaw += 180 + 30;
				remoteAng.pitch *= -1;
				remoteAng.pitch += 3 * idMath::Sin(gameLocal.time * 0.0005f); //sway
				remoteAng.yaw += 3 + 3 * idMath::Sin(gameLocal.time * 0.0003f);  //sway
				bluebox->SetAngles( remoteAng );

				idVec3	up, right;
				remoteAng.ToVectors( NULL, &right, &up );

				idVec3 finalPos = gameLocal.GetLocalPlayer()->GetEyePosition() 
					+ (gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 12.0f) 
					+ (up * -5.0f)
					+ (right * 7.0f);

				bluebox->GetPhysics()->SetOrigin(finalPos);
				//bluebox->GetPhysics()->SetOrigin(
			}
		}
		else
		{
			SetSkin(0); //default.

			//hide bluebox.
			bluebox->Hide();
		}
	}

	//handle the button animations.
	for (i = 0; i < 9; i++)
	{
		if (gameLocal.time > transitions[i])
		{
			jointHandle_t joint;
			joint = animator.GetJointHandle( GetJointViaIndex(i) );
			animator.ClearJoint(joint);
		}
	}

	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}




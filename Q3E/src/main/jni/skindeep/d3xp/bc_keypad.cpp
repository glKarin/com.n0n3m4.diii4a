
//  viola, the keypad
//  0 QZ    1 ABC   2 DEF
//  3 GHI   4 JKL   5 MNO
//  6 PRS   7 TUV   8 WXY

#include "Entity.h"
#include "Misc.h"
#include "Player.h"
//#include "Game_local.h"

#include "framework/DeclEntityDef.h"
#include "Fx.h"

#include "bc_maintpanel.h"
#include "bc_frobcube.h"
#include "bc_keypad.h"

#define PRESSTIME	100

//const idEventDef EV_keypadopen(		"keypadopen", "d" );

CLASS_DECLARATION(idAnimatedEntity, idKeypad)
//	EVENT( EV_keypadopen,		idKeypad::Event_keypadopen)
	EVENT(EV_PostSpawn, idKeypad::Event_PostSpawn)
END_CLASS

const int FROBINDEX_COVER = 16;

const int CLOSE_ANIMATIONTIME = 600; //ms animation time, to close lid.

idKeypad::idKeypad(void)
{
	zapParticle = NULL;
}

idKeypad::~idKeypad(void)
{
	for (int i = 0; i < 9; i++)
	{
		delete this->frobcubes[i];
	}

	if (zapParticle != NULL) {
		zapParticle->PostEventMS(&EV_Remove, 0);
		zapParticle = nullptr;
	}

	//delete frobcubeMain;
}

void idKeypad::Spawn(void)
{
	int i;
	idVec3 forward, right, up;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//spawn the buttons.
	for (i = 0; i < 9; i++)
	{
		idDict args;
		idVec3 pos = this->GetPhysics()->GetOrigin() + (forward * 1.7f);

		if (i <= 0) { pos += (up * 2) + (right * 2); }
		else if (i == 1) { pos += (up * 2); }
		else if (i == 2) { pos += (up * 2) + (right * -2); }
		else if (i == 3) { pos += (right * 2); }
		else if (i == 4) {}
		else if (i == 5) { pos += (right * -2); }
		else if (i == 6) { pos += (up * -2) + (right * 2); }
		else if (i == 7) { pos += (up * -2); }
		else if (i == 8) { pos += (up * -2) + (right * -2); }

		//Spawn frobcube buttons.
		//args.SetVector("origin", pos);
		//args.Set("model", spawnArgs.GetString("model_buttonfrob"));
		//args.SetInt("frobbable", 1);
		//args.SetInt("corpse", 1);
		//args.SetInt("noGrab", 1);
		//args.Set("owner", this->GetName());
		//args.SetInt("index", i);
		//this->frobcubes[i] = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		//this->frobcubes[i]->isFrobbable = false;
		//this->frobcubes[i]->SetAngles(this->GetPhysics()->GetAxis().ToAngles());

		args.Clear();
		args.SetVector("origin", pos);
		args.Set("model", spawnArgs.GetString("model_buttonfrob"));
		args.Set("owner", this->GetName());
		args.SetInt("index", i);
		args.SetInt("corpse", 1);
		args.SetMatrix("rotation", GetPhysics()->GetAxis());
		args.SetVector("cursoroffset", idVec3(1, 0, -.3f));
		//args.Set("displayname", " ");
		frobcubes[i] = gameLocal.SpawnEntityType(idFrobcube::Type, &args);		
		frobcubes[i]->GetPhysics()->GetClipModel()->SetOwner(this);
		frobcubes[i]->Bind(this, false);
		static_cast<idFrobcube*>(frobcubes[i])->SetIndex(i);
	}

	//Array of the keypad button glow skins.
	skin_glow[0] = declManager->FindSkin("skins/keypad/qz_glow");
	skin_glow[1] = declManager->FindSkin("skins/keypad/abc_glow");
	skin_glow[2] = declManager->FindSkin("skins/keypad/def_glow");
	skin_glow[3] = declManager->FindSkin("skins/keypad/ghi_glow");
	skin_glow[4] = declManager->FindSkin("skins/keypad/jkl_glow");
	skin_glow[5] = declManager->FindSkin("skins/keypad/mno_glow");
	skin_glow[6] = declManager->FindSkin("skins/keypad/prs_glow");
	skin_glow[7] = declManager->FindSkin("skins/keypad/tuv_glow");
	skin_glow[8] = declManager->FindSkin("skins/keypad/wxy_glow");


	//idDict args1;
	//args1.SetVector("origin", this->GetPhysics()->GetOrigin());
	//args1.Set("model", spawnArgs.GetString("model_clipmodel"));
	//args1.SetInt("frobbable", 1);
	//args1.SetInt("corpse", 1);
	//args1.SetInt("noGrab", 1);
	//args1.Set("owner", this->GetName());
	//args1.SetInt("index", FROBINDEX_COVER);
	//this->frobcubeMain = gameLocal.SpawnEntityType(idStaticEntity::Type, &args1);
	//this->frobcubeMain->isFrobbable = true;
	//this->frobcubeMain->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	
	for (i = 0; i < 4; i++)
	{
		input[i] = 0;
	}


	this->nextStateTime = 0;
	this->counter = 0;
	this->state = OFF;
	BecomeActive(TH_THINK);

	Event_keypadopen(1);

	team = TEAM_ENEMY;

	StartSound("snd_ambient", SND_CHANNEL_AMBIENT);


	PostEventMS(&EV_PostSpawn, 100);
}

void idKeypad::Event_PostSpawn(void)
{
	//The particle that points toward the maintpanel.

	if (targets.Num() <= 0)
	{
		gameLocal.Error("keypad '%s' has no target.", GetName());
		return;
	}

	if (!targets[0].IsValid())
	{
		gameLocal.Error("keypad '%s' has invalid target.", GetName());
		return;
	}

	idVec3 maintpanelForward;
	targets[0].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToVectors(&maintpanelForward, NULL, NULL);
	idVec3 maintpanelPos = targets[0].GetEntity()->GetPhysics()->GetOrigin() + (maintpanelForward * 4);
	

	idVec3 particleVec = maintpanelPos - GetAntennaPosition();
	particleVec.Normalize();

	idAngles particleAng = particleVec.ToAngles();
	particleAng.pitch += 90;	

	float distToPanel = (maintpanelPos - GetPhysics()->GetOrigin()).Length();

	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString(distToPanel > 32 ? "model_signalprt" : "model_signalprt_short"));
	args.SetVector("origin", GetAntennaPosition());
	args.SetMatrix("rotation", particleAng.ToMat3());
	
	zapParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
}

void idKeypad::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( counter ); // int counter
	savefile->WriteInt( nextStateTime ); // int nextStateTime
	SaveFileWriteArray( frobcubes, 9, WriteObject ); // idEntity* frobcubes[9]
	SaveFileWriteArray( skin_glow, 9, WriteSkin ); // const idDeclSkin * skin_glow[9];
	SaveFileWriteArray( transitions, 9, WriteInt ); // int transitions[9];

	SaveFileWriteArray( keys, keys.Num(), WriteString ); // idStrList keys

	SaveFileWriteArray( keycode, 4, WriteInt ); // int keycode[4]
	SaveFileWriteArray( input, 4, WriteInt ); // int input[4]

	savefile->WriteInt( keyIndex ); // int keyIndex

	savefile->WriteObject( zapParticle ); // idFuncEmitter * zapParticle
}

void idKeypad::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( counter ); // int counter
	savefile->ReadInt( nextStateTime ); // int nextStateTime
	SaveFileReadArray( frobcubes, ReadObject ); // idEntity* frobcubes[9]
	SaveFileReadArray( skin_glow, ReadSkin ); // const idDeclSkin * skin_glow[9];
	SaveFileReadArray( transitions, ReadInt ); // int transitions[9];

	SaveFileReadList( keys, ReadString ); // idStrList keys

	SaveFileReadArray( keycode, ReadInt ); // int keycode[4]
	SaveFileReadArray( input, ReadInt ); // int input[4]

	savefile->ReadInt( keyIndex ); // int keyIndex

	savefile->ReadObject( CastClassPtrRef(zapParticle) ); // idFuncEmitter * zapParticle
}



void idKeypad::Event_keypadopen( int value )
{
	if (state == CONFIRM_SUCCESS)
		return;

	if (value >= 1)
	{
		int i;

		//GenerateKeyFromDictionary();
		GenerateKeyFromKeyvalue(spawnArgs.GetString("code")); //get code from spawnarg.

		Event_PlayAnim( "opening" , 4 );

		state = ACTIVE;

		for (i = 0; i < 9; i++)
		{
			this->frobcubes[i]->isFrobbable = true;
		}

		//this->frobcubeMain->GetPhysics()->SetContents(0);
	}
	else
	{
		//this->frobcubeMain->GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
		//this->frobcubeMain->GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

		//BC
		//this->frobcubeMain->isFrobbable = false;
		//this->frobcubeMain->GetPhysics()->SetContents(0);
		//this->frobcubeMain->Hide();
		//this->frobcubeMain->isFrobbable = false;


		if (state == OFF)
		{
			return;
		}

		//int i;
		Event_PlayAnim( "closing" , 1);

		//state = OFF;


	}
}

void idKeypad::SetCode(idStr code)
{
	spawnArgs.Set("code", code.c_str());
	GenerateKeyFromKeyvalue(code);
}

//Generate key from 'code' spawnarg.
void idKeypad::GenerateKeyFromKeyvalue(idStr userkey)
{
	userkey.ToUpper();
	userkey.StripTrailingWhitespace();
	userkey.StripLeading(' ');
	
	if (userkey.Length() <= 0)
	{
		gameLocal.Error("keypad '%s' is missing 'code' keyvalue.", GetName());
	}
	else  if (userkey.Length() != 4)
	{
		gameLocal.Error("keypad '%s' invalid code: '%s'", GetName(), userkey.c_str());
	}

	int i;
	for (i = 0; i < 4; i++)
	{
		if (userkey[i] == 'Q' || userkey[i] == 'Z')
		{
			keycode[i] = 0;
		}
		else if (userkey[i] == 'A' || userkey[i] == 'B' || userkey[i] == 'C')
		{
			keycode[i] = 1;
		}
		else if (userkey[i] == 'D' || userkey[i] == 'E' || userkey[i] == 'F')
		{
			keycode[i] = 2;
		}
		else if (userkey[i] == 'G' || userkey[i] == 'H' || userkey[i] == 'I')
		{
			keycode[i] = 3;
		}
		else if (userkey[i] == 'J' || userkey[i] == 'K' || userkey[i] == 'L')
		{
			keycode[i] = 4;
		}
		else if (userkey[i] == 'M' || userkey[i] == 'N' || userkey[i] == 'O')
		{
			keycode[i] = 5;
		}
		else if (userkey[i] == 'P' || userkey[i] == 'R' || userkey[i] == 'S')
		{
			keycode[i] = 6;
		}
		else if (userkey[i] == 'T' || userkey[i] == 'U' || userkey[i] == 'V')
		{
			keycode[i] = 7;
		}
		else if (userkey[i] == 'W' || userkey[i] == 'X' || userkey[i] == 'Y')
		{
			keycode[i] = 8;
		}
		else
		{
			gameLocal.Error("keypad '%s' invalid code: '%s'", GetName(), userkey.c_str());
			return;
		}
	}
}

void idKeypad::GenerateKeyFromDictionary( void )
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

bool idKeypad::DoFrob(int index, idEntity * frobber)
{
	if (index == FROBINDEX_COVER)
	{
		this->StartSound( "snd_error" , SND_CHANNEL_ANY, 0, false, NULL );		
		return true;
	}

	
	//frob buttons.
	if (index >= 0 && index <= 8)
	{
		jointHandle_t joint;

		joint = animator.GetJointHandle( GetJointViaIndex(index) );
		animator.SetJointPos(joint, JOINTMOD_LOCAL, idVec3(-0.8f, 0, 0) );

		transitions[index] = gameLocal.time + PRESSTIME;
	}

	StartSound( "snd_press", SND_CHANNEL_ANY, 0, false, NULL );

	if (state != ACTIVE && state != CONFIRM_FAIL)
		return true;
	
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
		counter = 0;

		if ( input[0] == keycode[0] && input[1] == keycode[1] && input[2] == keycode[2] && input[3] == keycode[3] )
		{
			//Success!
			DoCodeSuccess();
			return true;
		}
		else
		{
			//fail.
			state = CONFIRM_FAIL;
			int doneTime = Event_PlayAnim("fail", 4);
			nextStateTime = doneTime;

			SetSkin(declManager->FindSkin("skins/keypad/red"));

			idVec3 forward, up;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
			idVec3 interestpos = GetPhysics()->GetOrigin() + (forward * 1.5f) + (up * 3.5f);
			gameLocal.SpawnInterestPoint(this, interestpos, spawnArgs.GetString("def_errornoise"));
			return true;
		}
	}

	return true;
}

void idKeypad::DoCodeSuccess()
{
	team = TEAM_FRIENDLY;

	//done success.
	//ActivateTargets(this);

	if (targets.Num() > 0)
	{
		for (int i = 0; i < targets.Num(); i++)
		{
			if (!targets[i].IsValid())
				continue;

			if (targets[i].GetEntity()->IsType(idMaintPanel::Type))
			{
				static_cast<idMaintPanel *>(targets[i].GetEntity())->Unlock(true);
			}
		}
	}


	state = CONFIRM_SUCCESS;
	int doneTime = Event_PlayAnim("success", 4);

	nextStateTime = doneTime;

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_success")));

	zapParticle->SetActive(false);
	StopSound(SND_CHANNEL_AMBIENT);

	//add hud message.

	StartSound("snd_deactivated", SND_CHANNEL_ANY, 0, false, NULL);

	gameLocal.RunMapScriptArgs(spawnArgs.GetString("call", ""), gameLocal.GetLocalPlayer(), this);
}




void idKeypad::UpdateStates( void )
{
	if (state == CONFIRM_SUCCESS)
	{
		if (gameLocal.time > nextStateTime)
		{
			int i;
			for (i = 0; i < 9; i++)
			{
				this->frobcubes[i]->isFrobbable = false;
			}

			//int i;

			//close ALL keypads.
			state = READY_TO_CLOSE;
			nextStateTime = gameLocal.time + CLOSE_ANIMATIONTIME;			

			Event_keypadopen(0); //close the keypad.

			//SetSkin(0);
			//idEntityFx::StartFx("fx/explosion_small", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);			
			//PostEventMS(&EV_Remove, 0);
			
			return;
		}
	}
	else if (state == READY_TO_CLOSE)
	{
		if (gameLocal.time > nextStateTime)
		{
			state = FULLY_CLOSED;

			for (int i = 0; i < 9; i++)
			{
				this->frobcubes[i]->isFrobbable = false;
			}

			StopSound(SND_CHANNEL_AMBIENT);
			gameLocal.DoParticle(spawnArgs.GetString("model_detachprt"), GetPhysics()->GetOrigin());
			GetPhysics()->SetContents(0);
			Hide();
			PostEventMS(&EV_Remove, 0);

			//Spawn the moveable.

			idVec3 forward;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL,NULL);			
			
			const idDeclEntityDef *itemDef;
			itemDef = gameLocal.FindEntityDef(spawnArgs.GetString("def_detachent"), false);
			
			if (itemDef)
			{
				idEntity *itemEnt;
				gameLocal.SpawnEntityDef(itemDef->dict, &itemEnt, false);
			
				if (itemEnt)
				{
					itemEnt->GetPhysics()->SetOrigin(GetPhysics()->GetOrigin() + forward * 0.2f);
					itemEnt->GetPhysics()->SetLinearVelocity((forward * 12) + idVec3(0, 0, 12));
					itemEnt->GetPhysics()->SetAngularVelocity(idVec3(16,0,0));

					if (itemEnt->IsType(idMoveableItem::Type))
					{
						//BC 2-21-2025: make a little explosion effect.
						idEntityFx::StartFx(spawnArgs.GetString("fx_explosion"), GetPhysics()->GetOrigin(), mat3_identity);

						static_cast<idMoveableItem *>(itemEnt)->SetJustDropped(true);						
					}
				}
			}			
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

void idKeypad::DetachFromWall()
{
	if (state == FULLY_CLOSED)
		return;

	state = READY_TO_CLOSE;
	nextStateTime = 0;
}

void idKeypad::Think( void )
{
	int i;

	//glows.
	if (state == ACTIVE)
	{
		if (gameLocal.GetLocalPlayer()->GetFrobEnt() != nullptr)
		{
			bool isHover = false;
			int i;
			int hoverIndex = gameLocal.GetLocalPlayer()->GetFrobEnt()->entityNumber;

			for (i = 0; i < 9; i++)
			{
				if (hoverIndex == frobcubes[i]->entityNumber)
				{
					//make keypad button glow when frob-hovering on it
					SetSkin(skin_glow[i]);
					isHover = true;
				}
			}
		}
		else
		{
			SetSkin(0); //default.
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


void idKeypad::DoHack()
{
	if (state != ACTIVE)
		return;

	gameLocal.AddEventLog("#str_def_gameplay_keypad_hacked", GetPhysics()->GetOrigin());
	DoCodeSuccess();
}

idVec3 idKeypad::GetAntennaPosition()
{
	idVec3 forward, right, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	return GetPhysics()->GetOrigin() + (forward * 1) + (right * -4.5f)  + (up * 8);
}

//#include "sys/platform.h"
//#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
//#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"

#include "SmokeParticles.h"
#include "bc_mech.h"

const int MECHANIMIDX_IDLE = 1;
const int MECHANIMIDX_FORWARD = 5;

const int COCKPIT_TRANSITIONFRAMES = 4;

const int DEATHTIME_MAX = 10000;

CLASS_DECLARATION(idAI, idMech)

END_CLASS


//TODO: Add missile circus.



void idMech::Spawn(void)
{
	mounted = false;
	animState = MECHSTATE_IDLE;
	strafeState = MECHSTRAFE_NONE;

	deathCountdownState = MECHCOUNTDOWN_NONE;
	deathTimer = 0;
	aiState = AISTATE_IDLE;

	fireParticles = nullptr;
	fireParticlesFlyTime = 0;
	countdownGui = nullptr;
	fireLight = nullptr;

	// Get projectile info
	playerProjectile = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_projectile_player" ), false );
	if ( !playerProjectile ) {
		gameLocal.Warning( "Invalid player projectile on idMech." );
	}

	spawnArgs.GetFloat( "player_firerate", "500", playerWeaponFireDelay );
	playerWeaponLastFireTime = 0.0f;
	spawnArgs.GetFloat( "player_reload_duration", "2000", playerWeaponReloadDuration );
	playerWeaponReloadFinishTime = -1.0f;
	spawnArgs.GetFloat( "player_rack_duration", "1000", playerWeaponRackDuration );
	playerWeaponRackFinishTime = -1.0f;

	// Get the firing sound
	idStr soundName;
	spawnArgs.GetString( "snd_player_fire", "", soundName );
	playerSoundFireWeapon = declManager->FindSound( soundName );

	spawnArgs.GetString( "snd_player_dryfire", "", soundName );
	playerSoundDryFire = declManager->FindSound( soundName );

	spawnArgs.GetString( "snd_player_lastshot", "", soundName );
	playerSoundLastShot = declManager->FindSound( soundName );

	spawnArgs.GetString( "snd_player_reload", "", soundName );
	playerSoundReload = declManager->FindSound( soundName );

	spawnArgs.GetString( "snd_player_rack", "", soundName );
	playerSoundRack = declManager->FindSound( soundName );

	spawnArgs.GetInt( "player_ammo", "10", playerAmmo );
	spawnArgs.GetInt( "player_clip_size", "5", playerClipSize );

	// Load up default clip
	playerClip = playerClipSize;
	playerAmmo -= playerClipSize;
	playerAmmo = Max( 0, playerAmmo );

	playerChambered = true;

	SetDrawBall(true);
}

void idMech::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( mounted ); // bool mounted

	savefile->WriteInt( animState ); // int animState

	savefile->WriteInt( strafeState ); // int strafeState

	savefile->WriteInt( deathCountdownState ); // int deathCountdownState
	savefile->WriteInt( deathTimer ); // int deathTimer

	savefile->WriteParticle( fireParticles ); // const idDeclParticle * fireParticles
	savefile->WriteInt( fireParticlesFlyTime ); // int fireParticlesFlyTime

	savefile->WriteObject( countdownGui ); // idEntity* countdownGui

	savefile->WriteObject( fireLight ); // idLight * fireLight
	savefile->WriteFloat( playerWeaponLastFireTime ); // float playerWeaponLastFireTime
	savefile->WriteFloat( playerWeaponFireDelay ); // float playerWeaponFireDelay
	savefile->WriteFloat( playerWeaponReloadDuration ); // float playerWeaponReloadDuration
	savefile->WriteFloat( playerWeaponReloadFinishTime ); // float playerWeaponReloadFinishTime
	savefile->WriteFloat( playerWeaponRackDuration ); // float playerWeaponRackDuration
	savefile->WriteFloat( playerWeaponRackFinishTime ); // float playerWeaponRackFinishTime

	// const idDict* playerProjectile; // const idDict* playerProjectile
	//playerProjectile = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_projectile_player" ), false );
	//if ( !playerProjectile ) {
	//	gameLocal.Warning( "Invalid player projectile on idMech." );
	//}
	savefile->WriteSoundShader( playerSoundFireWeapon ); // const idSoundShader* playerSoundFireWeapon
	savefile->WriteSoundShader( playerSoundLastShot ); // const idSoundShader* playerSoundLastShot
	savefile->WriteSoundShader( playerSoundDryFire ); // const idSoundShader* playerSoundDryFire
	savefile->WriteSoundShader( playerSoundReload ); // const idSoundShader* playerSoundReload
	savefile->WriteSoundShader( playerSoundRack ); // const idSoundShader* playerSoundRack
	savefile->WriteInt( playerAmmo ); // int playerAmmo
	savefile->WriteInt( playerClip ); // int playerClip
	savefile->WriteInt( playerClipSize ); // int playerClipSize
	savefile->WriteBool( playerChambered ); // bool playerChambered
}
void idMech::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( mounted ); // bool mounted

	savefile->ReadInt( animState ); // int animState

	savefile->ReadInt( strafeState ); // int strafeState

	savefile->ReadInt( deathCountdownState ); // int deathCountdownState
	savefile->ReadInt( deathTimer ); // int deathTimer

	savefile->ReadParticle( fireParticles ); // const idDeclParticle * fireParticles
	savefile->ReadInt( fireParticlesFlyTime ); // int fireParticlesFlyTime

	savefile->ReadObject( countdownGui ); // idEntity* countdownGui

	savefile->ReadObject( CastClassPtrRef(fireLight) ); // idLight * fireLight
	savefile->ReadFloat( playerWeaponLastFireTime ); // float playerWeaponLastFireTime
	savefile->ReadFloat( playerWeaponFireDelay ); // float playerWeaponFireDelay
	savefile->ReadFloat( playerWeaponReloadDuration ); // float playerWeaponReloadDuration
	savefile->ReadFloat( playerWeaponReloadFinishTime ); // float playerWeaponReloadFinishTime
	savefile->ReadFloat( playerWeaponRackDuration ); // float playerWeaponRackDuration
	savefile->ReadFloat( playerWeaponRackFinishTime ); // float playerWeaponRackFinishTime

	// const idDict* playerProjectile
	playerProjectile = gameLocal.FindEntityDefDict( spawnArgs.GetString( "def_projectile_player" ), false );
	if ( !playerProjectile ) {
		gameLocal.Warning( "Invalid player projectile on idMech." );
	}
	savefile->ReadSoundShader( playerSoundFireWeapon ); // const idSoundShader* playerSoundFireWeapon
	savefile->ReadSoundShader( playerSoundLastShot ); // const idSoundShader* playerSoundLastShot
	savefile->ReadSoundShader( playerSoundDryFire ); // const idSoundShader* playerSoundDryFire
	savefile->ReadSoundShader( playerSoundReload ); // const idSoundShader* playerSoundReload
	savefile->ReadSoundShader( playerSoundRack ); // const idSoundShader* playerSoundRack
	savefile->ReadInt( playerAmmo ); // int playerAmmo
	savefile->ReadInt( playerClip ); // int playerClip
	savefile->ReadInt( playerClipSize ); // int playerClipSize
	savefile->ReadBool( playerChambered ); // bool playerChambered
}

void idMech::Think(void)
{
	if (deathCountdownState == MECHCOUNTDOWN_ACTIVE)
	{
		float remainingTime = (deathTimer - gameLocal.time) / 1000.0f;

		if (mounted)
		{
			//Update the countdown timer hud.			
			gameLocal.GetLocalPlayer()->hud->SetStateString("corecountdown", idStr::Format("%.1f\n", remainingTime));
		}
		else
		{
			this->team = TEAM_NEUTRAL;
		}

		//Draw number above my head.
		//gameRenderWorld->DrawText(idStr::Format("%.1f\n", remainingTime), GetPhysics()->GetOrigin() + idVec3(0, 0, 140), 1.0f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10, true);
		//countdownGui->Event_SetGuiParm("gui_parm0", idStr::Format("%.1f\n", remainingTime));
		countdownGui->Event_SetGuiInt("gui_parm0", (int)(remainingTime + .8f));

		//Handle the fire particle fx.
		if (fireParticles != NULL && fireParticlesFlyTime && !IsHidden())
		{
			jointHandle_t topJoint;
			idVec3 topPos;
			idMat3 topMat;
			idVec3 dir = idVec3(0, 1, 0);

			topJoint = this->GetAnimator()->GetJointHandle("topofhead");
			this->GetJointWorldTransform(topJoint, gameLocal.time, topPos, topMat);

			if (!gameLocal.smokeParticles->EmitSmoke(fireParticles, fireParticlesFlyTime, gameLocal.random.RandomFloat(), topPos, dir.ToMat3(), timeGroup))
			{
				fireParticlesFlyTime = gameLocal.time;
			}
		}

		if (gameLocal.time >= deathTimer)
		{
			const char *splashDmgDef;
			idStr fxBreak;

			//THE MECH COUNTDOWN HAS EXPIRED. BLOW UP NOW!!!!!
			Hide();
			physicsObj.SetContents(0);

			deathCountdownState = MECHCOUNTDOWN_EXPIRED;
			StopSound(SND_CHANNEL_ANY, false);
			gameLocal.GetLocalPlayer()->mechCockpit->StopSound(SND_CHANNEL_ANY, false);

			if (mounted)
			{
				gameLocal.GetLocalPlayer()->ExitMech(true);
			}

			fxBreak = spawnArgs.GetString("fx_explode");
			if (fxBreak.Length())
			{
				idVec3 fxPos = GetPhysics()->GetAbsBounds().GetCenter(); //Center the explosion at gas cylinder middle, not origin.
				idMat3 fxMat = mat3_identity;
				idEntityFx::StartFx(fxBreak, &fxPos, &fxMat, NULL, false);
			}

			//Do explosion.
			splashDmgDef = spawnArgs.GetString("def_splash_damage", "damage_explosion");
			if (splashDmgDef && *splashDmgDef)
			{
				gameLocal.RadiusDamage(GetPhysics()->GetAbsBounds().GetCenter(), this, this, this, this, splashDmgDef);
				gameLocal.ThrowShrapnel(GetPhysics()->GetAbsBounds().GetCenter(), "projectile_shrapnel", this);
			}

			gameLocal.SpawnInterestPoint(this, GetPhysics()->GetAbsBounds().GetCenter(), "interest_explosion");

			idAI::Killed(this, this, 0, vec3_zero, 0);
			idAI::Gib(vec3_zero, "damage_suicide");

			fireLight->FadeOut(1.0f);
		}

		idAI::Think();
	}
	else if (mounted)
	{
		idAngles jointAng;
		idVec3 playerViewForward;
		idPlayer* player = gameLocal.GetLocalPlayer();

		this->team = TEAM_FRIENDLY;

		// SW: hijacking AI_JUMP to tell the animation script whether to play our special 'space flail' anim
		AI_JUMP = gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin());

		// Check for trying to fire
		if ( player->usercmd.buttons & BUTTON_ATTACK && ( gameLocal.time > playerWeaponLastFireTime + playerWeaponFireDelay ) && playerWeaponReloadFinishTime < 0.0f )
		{
			if ( playerClip > 0 && playerChambered )
			{
				// FIRE!
				idEntity		*ent;
				idProjectile	*proj;
				idBounds		projBounds;
				idVec3			dir;
				idVec3			muzzleOrigin;
				idMat3			muzzleAxis;

				gameLocal.SpawnEntityDef( *playerProjectile, &ent );
				if ( !ent || !ent->IsType( idProjectile::Type ) ) {
					const char *projectileName = spawnArgs.GetString( "def_player_projectile" );
					gameLocal.Error( "'%s' is not an idProjectile", projectileName );
				}

				player->GetViewPos( muzzleOrigin, muzzleAxis );

				//muzzleOrigin += ( muzzleAxis[0] * 128 );
				//muzzleOrigin -= ( muzzleAxis[2] * 20 );

				dir = muzzleAxis[0];

				proj = static_cast< idProjectile * >( ent );
				proj->Create( this, muzzleOrigin, dir );

				projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

				proj->Launch( muzzleOrigin, dir, vec3_origin );
				const idSoundShader* fireSound = playerClip == 1 ? playerSoundLastShot : playerSoundFireWeapon;
				StartSoundShader( fireSound, SND_CHANNEL_WEAPON, SSF_GLOBAL, false, NULL );
				gameLocal.SpawnInterestPoint(this, muzzleOrigin, "interest_weaponfire");
				playerClip--;
			}
			else
			{
				// Dry fire
				StartSoundShader( playerSoundDryFire, SND_CHANNEL_WEAPON, SSF_GLOBAL, false, NULL );
			}

			playerWeaponLastFireTime = gameLocal.time;
		}

		// Check for reload request
		if ( player->usercmd.buttons & BUTTON_RELOAD && !( player->oldButtons & BUTTON_RELOAD ) && playerWeaponReloadFinishTime < 0.0f && playerAmmo > 0 && playerClip == 0 )
		{
			playerChambered = false;
			playerWeaponReloadFinishTime = gameLocal.time + playerWeaponReloadDuration;
			StartSoundShader( playerSoundReload, SND_CHANNEL_WEAPON, SSF_GLOBAL, false, NULL );
		}

		// Check for reload finished
		if ( playerWeaponReloadFinishTime > 0.0f && gameLocal.time > playerWeaponReloadFinishTime )
		{
			playerClip = Min( playerAmmo, playerClipSize );
			playerAmmo -= playerClipSize;
			playerAmmo = Max( 0, playerAmmo );
			playerWeaponReloadFinishTime = -1.0f;
		}

		// Check for rack request
		if ( player->usercmd.buttons & BUTTON_RACKSLIDE && !( player->oldButtons & BUTTON_RACKSLIDE ) && !Player_IsReloading() && Player_NeedsRack() )
		{
			playerWeaponRackFinishTime = gameLocal.time + playerWeaponRackDuration;
			StartSoundShader( playerSoundRack, SND_CHANNEL_WEAPON, SSF_GLOBAL, false, NULL );
		}

		// Check for rack finished
		if ( playerWeaponRackFinishTime > 0.0f && gameLocal.time > playerWeaponRackFinishTime )
		{
			playerChambered = true;
			playerWeaponRackFinishTime = -1.0f;
		}

		//forwardmove ranges from -127 to 127
		
		if ( player->usercmd.rightmove < 0)
		{
			if (animState != MECHSTATE_LEFT)
			{
				animState = MECHSTATE_LEFT;
				AI_FORWARD = false;
				AI_BACKWARD = false;
				AI_LEFT = true;
				AI_RIGHT = false;
			}

			if ( player->usercmd.forwardmove > 0 && strafeState != MECHSTRAFE_LEFTFORWARD)
			{
				strafeState = MECHSTRAFE_LEFTFORWARD;
				player->mechCockpit->Event_PlayAnim("forward_left", COCKPIT_TRANSITIONFRAMES, true);
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
			else if ( player->usercmd.forwardmove < 0 && strafeState != MECHSTRAFE_LEFTBACKWARD)
			{
				strafeState = MECHSTRAFE_LEFTBACKWARD;
				player->mechCockpit->Event_PlayAnim("backward_left", COCKPIT_TRANSITIONFRAMES, true);
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
			else if (player->usercmd.forwardmove == 0 && strafeState != MECHSTRAFE_LEFT)
			{
				strafeState = MECHSTRAFE_LEFT;
				player->mechCockpit->Event_PlayAnim("strafe_left", COCKPIT_TRANSITIONFRAMES, true);
				gameLocal.GetLocalPlayer()->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
		}
		else if (player->usercmd.rightmove > 0)
		{
			if (animState != MECHSTATE_RIGHT)
			{
				animState = MECHSTATE_RIGHT;
				AI_FORWARD = false;
				AI_BACKWARD = false;
				AI_LEFT = false;
				AI_RIGHT = true;
			}

			if (player->usercmd.forwardmove > 0 && strafeState != MECHSTRAFE_RIGHTFORWARD)
			{
				strafeState = MECHSTRAFE_RIGHTFORWARD;
				player->mechCockpit->Event_PlayAnim("forward_right", COCKPIT_TRANSITIONFRAMES, true);
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
			else if (player->usercmd.forwardmove < 0 && strafeState != MECHSTRAFE_RIGHTBACKWARD)
			{
				strafeState = MECHSTRAFE_RIGHTBACKWARD;
				player->mechCockpit->Event_PlayAnim("backward_right", COCKPIT_TRANSITIONFRAMES, true);
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
			else if (player->usercmd.forwardmove == 0 && strafeState != MECHSTRAFE_RIGHT)
			{
				strafeState = MECHSTRAFE_RIGHT;
				player->mechCockpit->Event_PlayAnim("strafe_right", COCKPIT_TRANSITIONFRAMES, true);
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
		}
		else if (player->usercmd.forwardmove > 0 )
		{
			if (animState != MECHSTATE_FORWARD)
			{
				animState = MECHSTATE_FORWARD;
				//legsAnim.SetState("Legs_Walk", 4);
				//legsAnim.animBlendFrames = 8;
				//legsAnim.CycleAnim(5);
				AI_FORWARD = true;
				AI_BACKWARD = false;
				AI_LEFT = false;
				AI_RIGHT = false;

				player->mechCockpit->Event_PlayAnim("forward", COCKPIT_TRANSITIONFRAMES, true);
				strafeState = MECHSTRAFE_NONE;
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
		}
		else if (player->usercmd.forwardmove < 0)
		{
			if (animState != MECHSTATE_BACKWARD)
			{
				animState = MECHSTATE_BACKWARD;
				//legsAnim.SetState("Legs_Walk", 4);
				//legsAnim.animBlendFrames = 8;
				//legsAnim.CycleAnim(5);
				AI_FORWARD = false;
				AI_BACKWARD = true;				
				AI_LEFT = false;
				AI_RIGHT = false;

				player->mechCockpit->Event_PlayAnim("backward", COCKPIT_TRANSITIONFRAMES, true);
				strafeState = MECHSTRAFE_NONE;
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
		}
		else
		{
			if (animState != MECHSTATE_IDLE)
			{
				animState = MECHSTATE_IDLE;
				//legsAnim.animBlendFrames = 8;
				//legsAnim.SetState("Legs_Idle", 4);
				AI_FORWARD = false;
				AI_BACKWARD = false;
				AI_LEFT = false;
				AI_RIGHT = false;

				player->mechCockpit->Event_PlayAnim("idle", COCKPIT_TRANSITIONFRAMES, false);
				strafeState = MECHSTRAFE_NONE;
				player->StartSound("snd_cockpitmove", SND_CHANNEL_ANY, 0, false, NULL);
			}
		}

		lookAng = idAngles( gameLocal.GetLocalPlayer()->viewAngles.pitch , 0, 0);
		lookAng.Normalize180();

		TouchTriggers(); // for various awkward reasons, this doesn't fire in idAI::Think
		
		idAI::Think();
	}
	else
	{
		// SW: hijacking AI_JUMP to tell the animation script whether to play our special 'space flail' anim
		AI_JUMP = gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetOrigin());

		this->team = TEAM_NEUTRAL;
		idAI::Think();
	}
	
}


void idMech::Dismount(void)
{
	isFrobbable = true;

	if (health > 0 && deathCountdownState == MECHCOUNTDOWN_NONE)
	{
		move.moveType = MOVETYPE_ANIM;
	}

	mounted = false;
	AI_FORWARD = false;
	AI_BACKWARD = false;
	AI_LEFT = false;
	AI_RIGHT = false;

	lasersightbeam->Show();
	lasersightbeamTarget->Show();
	laserdot->Show();

	SetDrawBall(true);
	Unbind();


	//physicsObj.SetContents(CONTENTS_BODY);
	//physicsObj.SetClipMask(MASK_MONSTERSOLID);
	//SetPhysics(&physicsObj);
	//physicsObj.Activate();

	// SW 19th Feb 2025: replaced hardcoded integer with GetAnim call
	torsoAnim.PlayAnim(GetAnim(ANIMCHANNEL_TORSO, "flap_close")); //Play flap close anim.

	UpdateGravity();
}

void idMech::SetDrawBall(bool enabled)
{
	if (enabled)
	{
		this->currentskin = declManager->FindSkin(spawnArgs.GetString("skin_default"));
		this->damageFlashSkin = declManager->FindSkin(spawnArgs.GetString("skin_damageflash"));
	}
	else
	{
		this->currentskin = declManager->FindSkin(spawnArgs.GetString("skin_hideball"));
		this->damageFlashSkin = declManager->FindSkin(spawnArgs.GetString("skin_hideball_damageflash"));
	}

	SetSkin(this->currentskin);
}

bool idMech::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
		return false;

	if (frobber != gameLocal.GetLocalPlayer())
		return false;

	if (!mounted)
	{
		//Player has frobbed mech and wants to enter it.

		//float mechYaw = this->GetPhysics()->GetAxis().ToAngles().yaw;

		mounted = true;
		isFrobbable = false;
		move.moveType = MOVETYPE_PUPPET;

		// SW 19th Feb 2025: replaced hardcoded integer with GetAnim call
		torsoAnim.PlayAnim(GetAnim(ANIMCHANNEL_TORSO, "flap_open")); //Play flap open anim.
		AI_FORWARD = false;
		AI_BACKWARD = false;
		AI_LEFT = false;
		AI_RIGHT = false;

		//fl.takedamage = false;
		//GetPhysics()->SetContents(0); //make nonsolid.

		lasersightbeam->Hide();
		lasersightbeamTarget->Hide();
		laserdot->Hide();

		gameLocal.GetLocalPlayer()->EnterMech(this);
	}

	return false;
}

void idMech::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	//idAI::Killed(inflictor, attacker, damage, dir, location);

	if (deathCountdownState == MECHCOUNTDOWN_NONE)
	{
		idDict args;

		//Health has dipped at or below zero, start the death countdown here...
		

		//Start particles.
		fireParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, "fire_medium_loop.prt"));
		fireParticlesFlyTime = gameLocal.time;		

		args.Clear();
		args.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0,0,135));
		args.Set("texture", "lights/flames_ambient");
		args.SetInt("noshadows", 1);
		args.Set("_color", "1 .3 0 1");
		fireLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &args);
		fireLight->SetRadius(160);
		fireLight->Bind(this, false);

		lasersightbeam->Hide();
		lasersightbeamTarget->Hide();
		laserdot->Hide();

		deathCountdownState = MECHCOUNTDOWN_ACTIVE;

		StopSound(SND_CHANNEL_ANY, false);
		EndAttack();		
		StopMove(MOVE_STATUS_DONE);
		ClearEnemy();

		turnRate = 0; //Prevent AI from turning.

		

		AI_FORWARD = false;
		AI_BACKWARD = false;
		AI_LEFT = false;
		AI_RIGHT = false;
		move.moveType = MOVETYPE_STATIC;

		deathTimer = gameLocal.time + DEATHTIME_MAX;
		StartSound("snd_deathalarm", SND_CHANNEL_ANY, 0, false, NULL);

		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("fatalcore");

		args.Clear();
		args.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0,0,90));
		args.Set("model", "env_healthtube");
		args.Set("gui", "guis/game/fatalcore.gui");
		args.Set("start_anim", "spin");
		countdownGui = gameLocal.SpawnEntityType(idAnimated::Type, &args);
		countdownGui->Bind(this, false);

		state = GetScriptFunction("State_Killed");
		SetState(state);
		SetWaitState("");
	}
}

void idMech::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idAI::Damage(inflictor, attacker, dir, damageDefName, damageScale, location, materialType);

	gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("mechdamage");
}
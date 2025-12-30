#include "g_shared/a_deathcam.h"
#include "g_shared/a_playerpawn.h"
#include "actor.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_vh.h"
#include "language.h"
#include "gamemap.h"
#include "g_mapinfo.h"
#include "thingdef/thingdef.h"
#include "thinker.h"
#include "v_video.h"
#include "wl_act.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"

IMPLEMENT_POINTY_CLASS(DeathCam)
	DECLARE_POINTER(actor)
	DECLARE_POINTER(killer)
END_POINTERS

/*
===============
=
= CheckPosition
=
===============
*/

static bool CheckPosition (AActor *ob)
{
	int x,y,xl,yl,xh,yh;
	MapSpot check;

	xl = MAX<int>(0, (ob->x-ob->radius) >> TILESHIFT);
	yl = MAX<int>(0, (ob->y-ob->radius) >> TILESHIFT);

	xh = MIN<int>(map->GetHeader().width, (ob->x+ob->radius) >> TILESHIFT);
	yh = MIN<int>(map->GetHeader().height, (ob->y+ob->radius) >> TILESHIFT);

	//
	// check for solid walls
	//
	for (y=yl;y<=yh;y++)
	{
		for (x=xl;x<=xh;x++)
		{
			check = map->GetSpot(x, y, 0);
			if (check->tile)
				return false;
		}
	}

	return yl <= yh && xl <= xh;
}

void ADeathCam::SetupDeathCam(AActor *actor, AActor *killer)
{
	camState = CAM_STARTED;
	this->actor = actor;
	this->killer = killer;

	SetPriority(ThinkerList::VICTORY);
	actor->SetPriority(ThinkerList::VICTORY);
}

void ADeathCam::Tick()
{
	if(camState == ADeathCam::CAM_FINISHED)
	{
		Destroy();
		return;
	}

	if(gamestate.victoryflag)
	{
		if(killer->player)
			barrier_cast<APlayerPawn *>(killer)->TickPSprites();
	}

	Super::Tick();
}

ACTION_FUNCTION(A_FinishDeathCam)
{
	ADeathCam *cam = (ADeathCam *)self;
	if(cam->camState != ADeathCam::CAM_STARTED)
	{
		cam->camState = ADeathCam::CAM_FINISHED;
		CALL_ACTION(A_BossDeath, cam->actor);
		return true;
	}

	cam->x = cam->actor->killerx;
	cam->y = cam->actor->killery;
	cam->radius = cam->killer->radius;

	FinishPaletteShifts();

	gamestate.victoryflag = true;
	cam->camState = ADeathCam::CAM_ACTIVE;

	FizzleFadeStart();

	double fadex = 0, fadey, fadew = 320, fadeh;
	if(viewsize == 21)
	{
		fadey = 0;
		fadeh = 200;
	}
	else
	{
		fadey = StatusBar->GetHeight(true);
		fadeh = 200-StatusBar->GetHeight(false) - fadey + 1;
	}
	screen->VirtualToRealCoords(fadex, fadey, fadew, fadeh, 320, 200, true, true);
	VWB_DrawFill(TexMan(levelInfo->GetBorderTexture()), 0., fadey, screenWidth, fadeh);

	word width, height;
	VW_MeasurePropString(IntermissionFont, language["STR_SEEAGAIN"], width, height);
	px = 160 - (width/2);
	py = ((200 - StatusBar->GetHeight(false) - StatusBar->GetHeight(true)) - height)/2;
	VWB_DrawPropString(IntermissionFont, language["STR_SEEAGAIN"], CR_UNTRANSLATED);

	FizzleFade(0, static_cast<unsigned int>(fadey), screenWidth, static_cast<unsigned int>(fadeh), 70, false);

	A_Face(cam, cam->actor);

	//
	// try to position as close as possible without being in a wall
	//
	fixed dist = FixedMul(cam->actor->radius, 0x1E79E); // Approximate original distance of 0x14000l
	do
	{
		fixed xmove = FixedMul(dist,finecosine[cam->angle>>ANGLETOFINESHIFT]);
		fixed ymove = -FixedMul(dist,finesine[cam->angle>>ANGLETOFINESHIFT]);

		cam->x = cam->actor->x - xmove;
		cam->y = cam->actor->y - ymove;
		dist += 0x1000;

	}
	while(!CheckPosition(cam));

	IN_UserInput(300);

	players[0].camera = cam;
	players[0].SetPSprite(cam->FindState(NAME_Ready), player_t::ps_weapon);
	cam->actor->SetState(cam->actor->FindState(NAME_Death));

	DrawPlayScreen();

	fizzlein = true;
	return true;
}

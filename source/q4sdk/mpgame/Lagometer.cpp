#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define LAGO_TOP	380
#define	LAGO_LEFT	10
#define LAGO_WIDTH	64
#define LAGO_HEIGHT	44


idLagometer::idLagometer()
{
	memset(aheadOfServer, 0, sizeof(int) * LAGOMETER_HISTORY);
	memset(dupeUserCmds, 0, sizeof(byte) * LAGOMETER_HISTORY);

	frameIndex = 0;
}

void idLagometer::Draw()
{
	//renderSystem->DrawStretchPic(x, y, 1, h, 0, 0, 0, 0, lagometerBack);

	const idVec4 colorYellow(1.0f, 1.0f, 0.0f, 1.0f);
	const idVec4 colorGreen(0.0f, 1.0f, 0.0f, 1.0f);
	const idVec4 colorRed(1.0f, 0.0f, 0.0f, 1.0f);
	const idVec4 colorWhite(1.0f, 1.0f, 1.0f, 1.0f);

	const idMaterial *whiteMaterial = declManager->FindMaterial("_white");

	for (int x=0; x<LAGOMETER_HISTORY; x++)
	{
		int i = (frameIndex + 1 + x) % LAGOMETER_HISTORY;

		if (aheadOfServer[i] > 0)
		{
			renderSystem->SetColor(colorGreen);
			renderSystem->DrawStretchPic(LAGO_LEFT + x, LAGO_TOP + 10 - aheadOfServer[i], 1, aheadOfServer[i], 0, 0, 0, 0, whiteMaterial);
		}
		else if (aheadOfServer[i] < 0)
		{
			renderSystem->SetColor(colorYellow);
			renderSystem->DrawStretchPic(LAGO_LEFT + x, LAGO_TOP + 10, 1, -aheadOfServer[i], 0, 0, 0, 0, whiteMaterial);
		}

		renderSystem->SetColor(colorYellow);
		renderSystem->DrawStretchPic(LAGO_LEFT + x, LAGO_TOP + LAGO_HEIGHT - dupeUserCmds[i], 1, dupeUserCmds[i], 0, 0, 0, 0, whiteMaterial);
	}
}

void idLagometer::Update(int ahead, int dupe)
{
	frameIndex = (frameIndex + 1) % LAGOMETER_HISTORY;

	aheadOfServer[frameIndex] = 2 * idMath::ClampInt(-10, 5, 
									floorf( (float)ahead / net_clientLagOMeterResolution.GetInteger() ));

	dupeUserCmds[frameIndex] = 2 * Min( 6, dupe );
} 
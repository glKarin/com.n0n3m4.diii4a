
#include "Entity.h"
#include "Actor.h"
#include "Player.h"
#include "bc_spectatenode.h"

CLASS_DECLARATION(idEntity, idSpectateNode)
END_CLASS

idSpectateNode::idSpectateNode()
{
	spectatenodeNode.SetOwner(this);
	spectatenodeNode.AddToEnd(gameLocal.spectatenodeEntities);
}

idSpectateNode::~idSpectateNode()
{
	spectatenodeNode.Remove();
}

void idSpectateNode::Spawn(void)
{
	text = spawnArgs.GetString("text");


	int timeMS = spawnArgs.GetInt("time");
	timestamp = gameLocal.ParseTimeDetailedMS(timeMS);
	
	//BecomeActive(TH_THINK);
	//GetPhysics()->SetContents(CONTENTS_CORPSE);
	//gameRenderWorld->DebugArrowSimple(GetPhysics()->GetOrigin());
}


void idSpectateNode::Save(idSaveGame* savefile) const
{
	savefile->WriteString( text ); // idString text
	savefile->WriteString( timestamp ); // idString timestamp
}
void idSpectateNode::Restore(idRestoreGame* savefile)
{
	savefile->ReadString( text ); // idString text
	savefile->ReadString( timestamp ); // idString timestamp
}

void idSpectateNode::Think(void)
{	
	idEntity::Think();
}

void idSpectateNode::Draw()
{
	#define ICON_VERTICALOFFSET 17
	#define RIGHTOFFSET 7
	#define TEXT_VERTICALOFFSET -15
	idVec3 viewRight;
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(NULL, &viewRight, NULL);

	idVec3 iconPos = GetPhysics()->GetOrigin() + idVec3(0, 0, ICON_VERTICALOFFSET) + (viewRight * RIGHTOFFSET);
	idVec2 textPos = gameLocal.GetLocalPlayer()->GetWorldToScreen(iconPos);

	textPos.y += TEXT_VERTICALOFFSET;

	gameLocal.GetLocalPlayer()->hud->DrawArbitraryText(text.c_str(), .65f, idVec4(1, 1, 1, 1), textPos.x, textPos.y, "sofia", idVec4(0, 0, 0, 1), idDeviceContext::ALIGN_LEFT);

	gameLocal.GetLocalPlayer()->hud->DrawArbitraryText(timestamp.c_str(), .45f, idVec4(1, 1, 1, 1), textPos.x, textPos.y + 16, "sofia", idVec4(0, 0, 0, 1), idDeviceContext::ALIGN_LEFT, 2);
}

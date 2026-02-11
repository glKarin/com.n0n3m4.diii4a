#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"


#include "bc_searchnode.h"

CLASS_DECLARATION(idEntity, idSearchNode)
END_CLASS

idSearchNode::idSearchNode(void)
{
}

idSearchNode::~idSearchNode(void)
{
	//aiSearchNodes.Remove();
}

void idSearchNode::Save(idSaveGame *savefile) const
{
    savefile->WriteInt( lastTimeUsed ); // int lastTimeUsed
    savefile->WriteInt( lastNodeAnimTime ); // int lastNodeAnimTime
    SaveFileWriteArray( nodeAnimList, nodeAnimList.Num(), WriteString ); // idList<idStr> nodeAnimList
}

void idSearchNode::Restore(idRestoreGame *savefile)
{
    savefile->ReadInt( lastTimeUsed ); // int lastTimeUsed
    savefile->ReadInt( lastNodeAnimTime ); // int lastNodeAnimTime
    SaveFileReadList( nodeAnimList, ReadString ); // idList<idStr> nodeAnimList
}

void idSearchNode::Spawn(void)
{
	lastTimeUsed = -60000;
	lastNodeAnimTime = -60000;

	//aiSearchNodes.SetOwner(this);
	//aiSearchNodes.AddToEnd(gameLocal.searchnodeEntities);


	//check if reachable within AAS.
	idAAS *aas = gameLocal.GetAAS(0);
	if (aas)
	{
		int value = aas->PointReachableAreaNum(this->GetPhysics()->GetOrigin(), idBounds(idVec3(-16, -16, 0), idVec3(16, 16, 60)), AREA_REACHABLE_WALK);
		
		if (value <= 0)
		{
			common->Warning("searchnode '%s' is not within an AAS-reachable area.", this->GetName());

			gameRenderWorld->DrawText(idStr::Format("'%s' not within AAS reachable area.", this->GetName()), this->GetPhysics()->GetOrigin() + idVec3(0, 0, 70), 1.5f, idVec4(1, 0, 0, 1), mat3_identity, 1, 90000);
			gameRenderWorld->DebugArrow(colorRed, this->GetPhysics()->GetOrigin() + idVec3(0, 0, 64), this->GetPhysics()->GetOrigin(), 4, 90000);
		}
	}

    idStr animRaw = spawnArgs.GetString("anim");
    if (animRaw[0] == '\0')
    {
		//No anims. Exit here.
    }    
    else if (animRaw.Find(";") >= 0)
    {
        //Parse the anims.
        while (animRaw.Length())
        {
            idStr singleAnimName;
            int n = animRaw.Find(";");
            if (n >= 0)
            {
                singleAnimName = animRaw.Left(n);
                animRaw = animRaw.Right(animRaw.Length() - n - 1);
            }
            else
            {
                singleAnimName = animRaw;
                animRaw = "";
            }
            
            nodeAnimList.Append(singleAnimName);
        }
    }
    else
    {
        nodeAnimList.Append(animRaw); //No semicolons.
    }    
}

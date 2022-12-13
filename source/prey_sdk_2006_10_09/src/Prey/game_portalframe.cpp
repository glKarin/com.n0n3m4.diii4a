// hhPortalFrame
//
// Frame for our portal which accepts commands from GUIs which change shader parm5
// based on the command, as well as trigger some given entities each level, and finally
// trigger all it's own targets upon the victory command


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION(idEntity, hhPortalFrame)
END_CLASS


void hhPortalFrame::Spawn() {
	GetPhysics()->SetContents(CONTENTS_SOLID);
}


// DnItem.cpp
//

#include "../Gamelib/Game_local.h"

CLASS_DECLARATION(idEntity, DnItem)
	EVENT(EV_Touch, DnItem::Event_Touch)
END_CLASS

/*
================
DnItem::Spawn
================
*/
void DnItem::Spawn(void) {
	// create a trigger for item pickup
	GetPhysics()->GetClipModel()->LoadModel(idTraceModel(idBounds(vec3_origin).Expand(32)));
	GetPhysics()->GetClipModel()->Link(gameLocal.clip);
	GetPhysics()->SetContents(CONTENTS_TRIGGER);

	BecomeActive(TH_THINK);

	orgOrigin = GetPhysics()->GetOrigin();

	//renderEntity.hModel = renderEntity.hModel->InstantiateDynamicModel(nullptr, nullptr, nullptr);
}


/*
================
DnItem::Think
================
*/
void DnItem::Think(void) {
	idAngles	ang;
	idVec3		org;

	ang.pitch = ang.roll = 0.0f;
	ang.yaw = (gameLocal.time & 4095) * 360.0f / -4096.0f;
	SetAngles(ang);

	float scale = 0.005f + entityNumber * 0.00001f;

	org = orgOrigin;
	org.z += 4.0f + cos((gameLocal.time + 2000) * scale) * 4.0f;
	SetOrigin(org);

	Present();
}

/*
================
DnItem::Event_Touch
================
*/
void DnItem::Event_Touch(idEntity* other, trace_t* trace) {
	DukePlayer* player = other->Cast<DukePlayer>();

	if (!player)
	{
		return;
	}

	TouchEvent(player, trace);
}

/*
================
DnItem::TouchEvent
================
*/
void DnItem::TouchEvent(DukePlayer* player, trace_t* trace) {
	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents(0);

	// hide the model
	Hide();

	BecomeInactive(TH_THINK);
}

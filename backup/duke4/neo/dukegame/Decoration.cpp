// Decoration.cpp
//

#include "Gamelib/Game_local.h"

CLASS_DECLARATION(idMover, dnDecoration)
END_CLASS

/*
================
dnDecoration::Spawn
================
*/
void dnDecoration::Spawn(void) {
	BecomeActive(TH_THINK);
	BecomeActive(TH_UPDATEVISUALS);
}

/*
================
dnDecoration::Think
================
*/
void dnDecoration::Think(void) {
	idEntity::Think();
}

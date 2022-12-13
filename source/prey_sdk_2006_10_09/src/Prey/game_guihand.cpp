
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


CLASS_DECLARATION( hhHand, hhGuiHand )
END_CLASS


void hhGuiHand::Spawn(void) {
	actionAnimDoneTime = 0;
	fl.networkSync = true;
}

void hhGuiHand::Save(idSaveGame *savefile) const {
	savefile->WriteInt( actionAnimDoneTime );
}

void hhGuiHand::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( actionAnimDoneTime );
}

void hhGuiHand::WriteToSnapshot( idBitMsgDelta &msg ) const
{
	hhHand::WriteToSnapshot(msg);

	msg.WriteBits(actionAnimDoneTime, 32);
}

void hhGuiHand::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hhHand::ReadFromSnapshot(msg);

	actionAnimDoneTime = msg.ReadBits(32);
}

void hhGuiHand::ClientPredictionThink( void ) {
	hhHand::ClientPredictionThink();
}

void hhGuiHand::Action(void) {
	PlayAnim( -1, action, &EV_Hand_Ready );
}

void hhGuiHand::SetAction(const char* str) {
	action = str;
}

bool hhGuiHand::IsValidFor( hhPlayer *who ) { 
	return( who->InGUIMode() ); 
}

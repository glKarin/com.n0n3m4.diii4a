/*
game_spring.cpp
*/
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"


/***********************************************************************

  hhSpring
	
***********************************************************************/

const idEventDef EV_HHLinkSpring( "linkspring", "" );
const idEventDef EV_HHUnlinkSpring( "unlinkspring", "" );

CLASS_DECLARATION( idEntity, hhSpring )
	EVENT( EV_HHLinkSpring,		hhSpring::Event_LinkSpring )
	EVENT( EV_HHUnlinkSpring,	hhSpring::Event_UnlinkSpring )
END_CLASS


/*
================
hhSpring::Spawn
================
*/
void hhSpring::Spawn( void ) {
	float Kstretch, damping, restLength, Kcompress;

	spawnArgs.GetString( "ent1", "", name1 );
	spawnArgs.GetString( "ent2", "", name2 );
	spawnArgs.GetInt( "id1", "0", id1 );
	spawnArgs.GetInt( "id2", "0", id2 );
	spawnArgs.GetVector( "point1", "0 0 0", p1 );
	spawnArgs.GetVector( "point2", "0 0 0", p2 );
	spawnArgs.GetFloat( "constant", "100.0f", Kstretch );
	spawnArgs.GetFloat( "damping", "10.0f", damping );
	spawnArgs.GetFloat( "restlength", "0.0f", restLength );
	spawnArgs.GetFloat( "compress",	"0.0f", Kcompress );

	// HUMANHEAD: Added compression constant
	spring.InitSpring( Kstretch, Kcompress, damping, restLength );

	PostEventMS( &EV_HHLinkSpring, 0 );
}

void hhSpring::Save(idSaveGame *savefile) const {
	savefile->WriteString( name1 );
	savefile->WriteString( name2 );
	physics1.Save( savefile );
	physics2.Save( savefile );
	savefile->WriteInt( id1 );
	savefile->WriteInt( id2 );
	savefile->WriteVec3( p1 );
	savefile->WriteVec3( p2 );
	savefile->WriteStaticObject( spring );
}

void hhSpring::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( name1 );
	savefile->ReadString( name2 );
	physics1.Restore( savefile );
	physics2.Restore( savefile );
	savefile->ReadInt( id1 );
	savefile->ReadInt( id2 );
	savefile->ReadVec3( p1 );
	savefile->ReadVec3( p2 );
	savefile->ReadStaticObject( spring );
}

/*
================
hhSpring::Think
	HUMANHEAD
================
*/
#define TEST_SPRINGS
void hhSpring::Think() {
	if (thinkFlags & TH_THINK) {
		spring.Evaluate( gameLocal.time );

#ifdef TEST_SPRINGS
		idVec3 start, end, origin;
		idMat3 axis;

		start = p1;
		if ( physics1.IsValid() ) {
			axis = physics1->GetPhysics()->GetAxis(id1);
			origin = physics1->GetPhysics()->GetOrigin(id1);
			start = origin + p1 * axis;
		}

		end = p2;
		if ( physics2.IsValid() ) {
			axis = physics2->GetPhysics()->GetAxis(id2);
			origin = physics2->GetPhysics()->GetOrigin(id2);
			end = origin + p2 * axis;
		}
		
		gameRenderWorld->DebugLine( colorYellow, start, end, 0, true );
#endif
	}
}

/*
================
hhSpring::LinkSpring
	HUMANHEAD
================
*/
void hhSpring::LinkSpring(idEntity *ent1, idEntity *ent2 ) {
	LinkSpringAll(ent1, id1, p1, ent2, id2, p2);
}

/*
================
hhSpring::LinkSpringIDs
	HUMANHEAD
================
*/
void hhSpring::LinkSpringIDs(idEntity *ent1, int bodyID1, idEntity *ent2, int bodyID2) {
	id1 = bodyID1;
	id2 = bodyID2;
	LinkSpringAll(ent1, id1, p1, ent2, id2, p2);
}

/*
================
hhSpring::LinkSpringAll
	HUMANHEAD
================
*/
void hhSpring::LinkSpringAll(idEntity *ent1, int bodyID1, idVec3 &offset1,
							 idEntity *ent2, int bodyID2, idVec3 &offset2) {

	physics1 = ent1;
	physics2 = ent2;
	spring.SetPosition( physics1.GetEntity(), bodyID1, offset1, physics2.GetEntity(), bodyID2, offset2 );
	BecomeActive(TH_THINK);
}
							
/*
================
hhSpring::UnLinkSpring
	HUMANHEAD
================
*/
void hhSpring::UnLinkSpring() {
	BecomeInactive(TH_THINK);
}

/*
================
hhSpring::SpringSettings
	HUMANHEAD
================
*/
void hhSpring::SpringSettings(float kStretch, float kCompress, float damping, float restLength) {
	spring.InitSpring(kStretch, kCompress, damping, restLength);
}

/*
================
hhSpring::Event_LinkSpring
================
*/
void hhSpring::Event_LinkSpring( void ) {
	idEntity *ent1, *ent2;

	ent1 = gameLocal.FindEntity( name1 );
	if ( !ent1 ) {
		ent1 = gameLocal.entities[ENTITYNUM_WORLD];
	}
	ent2 = gameLocal.FindEntity( name2 );
	if ( !ent2 ) {
		ent2 = gameLocal.entities[ENTITYNUM_WORLD];
	}
	LinkSpring(ent1, ent2);
}

/*
================
hhSpring::Event_UnlinkSpring
================
*/
void hhSpring::Event_UnlinkSpring( void ) {
	UnLinkSpring();
}

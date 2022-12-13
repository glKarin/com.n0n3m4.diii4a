#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_EnableNode( "enableNode", NULL );
const idEventDef EV_DisableNode( "disableNode", NULL );

CLASS_DECLARATION( idStaticEntity, hhEnergyNode )
	EVENT( EV_EnableNode,			hhEnergyNode::Event_Enable )
	EVENT( EV_DisableNode,			hhEnergyNode::Event_Disable )
END_CLASS

hhEnergyNode::hhEnergyNode(void)
:disabled(false)
{}

hhEnergyNode::~hhEnergyNode(void) {
	Event_Disable();
}


void hhEnergyNode::Save(idSaveGame *savefile) const {
	savefile->WriteBool( disabled );
	savefile->WriteVec3( leechPoint );

	energyFx.Save( savefile );
}

void hhEnergyNode::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( disabled );
	savefile->ReadVec3( leechPoint );

	energyFx.Restore( savefile );
}

void hhEnergyNode::Spawn( void ) {
	fl.networkSync = true;
	GetPhysics()->SetContents( CONTENTS_SOLID );

	leechPoint = GetAxis()*spawnArgs.GetVector("leechPoint")+GetOrigin();

	fl.clientEvents = true;
	PostEventMS(&EV_EnableNode, 1);
}

void hhEnergyNode::LeechTrigger(idEntity *activator, const char* type) {
	idStr name = spawnArgs.GetString(type);
	if ( name != "" ) {
		idEntity* ent = gameLocal.FindEntity(name);
		if ( ent ) {
			if ( ent->RespondsTo( EV_Activate ) || ent->HasSignal( SIG_TRIGGER ) ) {
				ent->Signal( SIG_TRIGGER );
				ent->ProcessEvent( &EV_Activate, activator );
			}
			ent->TriggerGuis();
		}
	}
}

void hhEnergyNode::Finish() {
	Event_Disable();

	if ( spawnArgs.GetInt("infinite","0") || gameLocal.isMultiplayer ) { // delay before reenabling -cjr
		PostEventSec( &EV_EnableNode, spawnArgs.GetFloat( "reenableDelay", "20" ) );	
	}
}

void hhEnergyNode::Event_Enable() {
	disabled = false;

	const idDict *energyDef = NULL;
	const char* str = spawnArgs.GetString( "def_energy" );
	if ( str && str[0] ) {
		energyDef = gameLocal.FindEntityDefDict( str );
	}

	if ( energyDef ) {
		hhFxInfo fxInfo;
		if (IsBound() || spawnArgs.GetBool("force_bind")) {
			fxInfo.SetEntity( this );	// Only bind if we will be moving
		}
		energyFx = SpawnFxLocal( energyDef->GetString("fx_node"), leechPoint, GetAxis(), &fxInfo, true );

		idVec3 color = energyDef->GetVector("nodeColor");
		SetShaderParm( SHADERPARM_RED, color.x );
		SetShaderParm( SHADERPARM_GREEN, color.y );
		SetShaderParm( SHADERPARM_BLUE, color.z );

		StartSound( "snd_activate", SND_CHANNEL_ANY );
		StartSound( "snd_idle", SND_CHANNEL_BODY );
	}
}

void hhEnergyNode::Event_Disable() {
	disabled = true;
	if( energyFx.IsValid() ) {
		SAFE_REMOVE(energyFx);
	}

	SetShaderParm( SHADERPARM_RED, 0 );
	SetShaderParm( SHADERPARM_GREEN, 0 );
	SetShaderParm( SHADERPARM_BLUE, 0 );

	StopSound( SND_CHANNEL_BODY );
}

void hhEnergyNode::WriteToSnapshot( idBitMsgDelta &msg ) const {
	idStaticEntity::WriteToSnapshot(msg);

	msg.WriteBits(disabled, 1);
}

void hhEnergyNode::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idStaticEntity::ReadFromSnapshot(msg);

	bool snapDisabled = !!msg.ReadBits(1);
	if (snapDisabled != disabled) {
		if (snapDisabled) {
			Event_Disable();
		}
		else {
			Event_Enable();
		}
	}
}

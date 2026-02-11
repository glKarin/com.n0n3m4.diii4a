// HEXEN : Zeroth

#pragma hdrstop

#include "Game_local.h"
#include "Item.h"
#include "Player.h"
#include "gamesys/SysCvar.h"

int idPlayer::FindArtifact( int art ) {
	for (int belt=0; belt<Artifact.Num(); belt++) {
		if (Artifact[belt]->GetInt("artifact") == art) {
			return belt;
		}
	}
	return -1;
}

bool idPlayer::ActiveArtifact( int art ) {
	idItem *item;
	int hash, i;

	hash = gameLocal.entypeHash.GenerateKey( idItem::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idItem::Type ) ) {
			item = static_cast< idItem* >( gameLocal.entities[i] );

			if ( item->GetOwner() != this ) {
				item = NULL;
				continue;
			}

			if ( !item->ArtifactActive ) {
				item = NULL;
				continue;
			}

			if ( item->spawnArgs.GetInt("artifact") != art) {
				item = NULL;
				continue;
			}

			return true;
		}
	}

	return false;	
}

bool idPlayer::ActiveArtifact( const char *art ) {
	idDict tmp;
	tmp.Set("art", art);
	return ActiveArtifact(tmp.GetInt("art"));
}

void idPlayer::ArtifactExec( int belt, char * funcName, bool remove ) {
	CleanupArtifactItems();

	if (!ArtifactVerify(belt)) {
		UpdateHudArtifacts();
		return;
	}

	if (funcName == NULL) {
		return;
	}

	const char *name=Artifact[belt]->GetString("name");

	if (ActiveArtifact(Artifact[belt]->GetInt("artifact"))) {
		return;
	}

	idItem *item;
	int hash, i;

	hash = gameLocal.entypeHash.GenerateKey( idItem::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idItem::Type ) ) {
			item = static_cast< idItem* >( gameLocal.entities[i] );

			if ( item->GetOwner() !=  this ) {
				continue;
			}

			if ( idStr::Icmp(name, item->spawnArgs.GetString("inv_name")) ) {
				continue;
			}

			if ( item->Processing ) {
				continue;
			}

			if ( item->DeleteMe ) {
				continue;
			}

			item->CallFunc(funcName);
			return;
		}
	}
}

void idPlayer::SortArtifacts( void ) {
	int a,b;
	idDict *temp;

	for (a=0; a<Artifact.Num(); a++) {
		for (b=a+1; b<Artifact.Num(); b++) {
			if ( Artifact[a]->GetInt("artifact") < Artifact[b]->GetInt("artifact")) {
				continue;
			}

			temp=Artifact[a];
			Artifact[a]=Artifact[b];
			Artifact[b]=temp;
			temp=NULL;

			if ( BeltSelection == a) {
					BeltSelection = b;
			} else if ( BeltSelection == b) {
					BeltSelection = a;
			}

			// we don't callUpdateHudArtifacts(); because SortArtifacts is only called by GiveInventoryItem(), and that takes care of it.
		}
	}
}

void idPlayer::UpdateHudArtifacts( void ) {
	int	belt,art;
	int	pos; // belt item position marker
	int	onScreen=((7-1)/2); //number of on-screen artifacts to the left of the center. (belt_width - 1) / 2; belt_width must be odd (the middle number is center of belt)
	int	tileWidth=60; //width in pixels of an artifact on the belt
	char	itemStr[22];

	ArtifactValidateSelection();

	// disappear all artifacts
	for ( art=0; art < NUM_UNIQUE_ARTIFACTS; art++) {
		sprintf(itemStr, "eoc_Artifact%iVis", art);
		hud->SetStateBool( itemStr, false );

		sprintf(itemStr, "eoc_Artifact%iRotate", art);
		hud->SetStateFloat( itemStr, 0.0f );
	}

	hud->SetStateInt( "eoc_Artifacts",Artifact.Num() );
	hud->SetStateInt( "eoc_BeltSelection",BeltSelection );

	// determine tile position and visibility 
	pos=0;
	for (belt=0; belt < Artifact.Num(); belt++) {
		art=Artifact[belt]->GetInt("artifact");
		if (InventoryItemQty(belt) > 0) {
			sprintf(itemStr, "eoc_Artifact%iPos", art);
			hud->SetStateInt( itemStr, pos );

			sprintf(itemStr, "eoc_Artifact%iQty", art);
			hud->SetStateInt( itemStr,InventoryItemQty(belt) );

			sprintf(itemStr, "eoc_Artifact%iVis", art);
			if ((belt<BeltSelection-onScreen) || (belt>BeltSelection+onScreen)) {
				hud->SetStateBool( itemStr, false ); // Hide artifacts that are off-screen
			}else{
				hud->SetStateBool( itemStr, true );
			}

			sprintf(itemStr, "eoc_Artifact%iRotate", art);
			if (BeltSelection == belt) //ArtifactValidateSelection(); sould be called once before this in the same function
			{
				BeltPosition=pos;
				hud->SetStateFloat( itemStr, 0.05f );
			}else{
				hud->SetStateFloat( itemStr, 0.0f );
			}
			
			pos+=tileWidth; //up the position for the next visible item
		}else{
			//the artifact is no longer in the inventory, but is in use
			sprintf(itemStr, "eoc_Artifact%iVis", art);
			hud->SetStateBool( itemStr, false );

			sprintf(itemStr, "eoc_Artifact%iRotate", art);
			hud->SetStateFloat( itemStr, 0.0f );
		}
	}

	BeltPosition= -(BeltPosition)+(tileWidth*onScreen); // add onScreen*tielWidth to get to center of belt, negate to scroll in opposite (correct) direction
	hud->SetStateInt( "eoc_BeltPosition",BeltPosition ); // set scrolled position of inventory

	// update selected artifact
	if (BeltSelection < 0 ) {
		hud->SetStateString( "eoc_BeltSelectionQty", "" );
	} else {
		hud->SetStateInt( "eoc_BeltSelectionQty", InventoryItemQty(BeltSelection)  );
	}
	UpdateArtifactHudDescription();
}

void idPlayer::UpdateHudActiveArtifacts() {
	int		i;
	int		numActive=0;
	int		pos=0; // active artifact position marker
	int		tileWidth=60; //width in pixels of an artifact on the belt
	char		itemStr[30];
	idItem*		item;
	bool		active[NUM_UNIQUE_ARTIFACTS];
	bool		cooling[NUM_UNIQUE_ARTIFACTS];
	int		totArtifacts=NUM_UNIQUE_ARTIFACTS; //Z.TODO this should be read from a def file or something

	// initialize local variables
	for (i=0; i < totArtifacts; i++) {
		active[i]=false;
		cooling[i]=false;
	}

	// figure out which artifacts are active or cooling
	int hash;

	hash = gameLocal.entypeHash.GenerateKey( idItem::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idItem::Type ) ) {
			item = static_cast< idItem* >( gameLocal.entities[i] );

			if ( item->GetOwner() !=  this ) {
				continue;
			}

			if ( !item->ArtifactActive ) {
				continue;
			}

 			active[item->spawnArgs.GetInt("artifact")] = true;

			if ( item->Cooling ) {
				cooling[item->spawnArgs.GetInt("artifact")] = true;
			}
		}
	}


	for (i=0; i < totArtifacts; i++ ) {
		if ( active[i] ) {
			sprintf(itemStr, "eoc_Artifact%iPosA", i);
			hud->SetStateInt( itemStr, pos );

			sprintf(itemStr, "eoc_Artifact%iVisA", i);
			hud->SetStateBool( itemStr, true );

			// artifact must be active for cooldown, but effects should be turned off at this point via script
			sprintf(itemStr, "eoc_Artifact%iCoolDownVisA", i);
			hud->SetStateBool( itemStr, cooling[i] );

			pos+=tileWidth; //up the position for the next visible item
			numActive++;
		}else{
			//no need to change pos

			sprintf(itemStr, "eoc_Artifact%iVisA", i);
			hud->SetStateBool( itemStr, false );

			sprintf(itemStr, "eoc_Artifact%iCoolDownVisA", i);
			hud->SetStateBool( itemStr, false );
		}
	}

	hud->SetStateInt("eoc_ActiveArtifacts", numActive);

}

bool idPlayer::ArtifactVerify( int belt ) {
	int art;
	char itemStr[22];
        //int iteration=rand();

	if ( belt == -1 ) {
		return false;
	}

	if ( belt >= Artifact.Num() ) {
		return false;
	}

	art=Artifact[belt]->GetInt("artifact");

	sprintf(itemStr,Artifact[belt]->GetString("name"));

	if ( art == -1 ) {
		return true; //?
	}
	
	if ( InventoryItemQty(itemStr) > 0 ) {
		return true;
	}

	// remove the artifact, we have zero items of it's type
	sprintf(itemStr, "eoc_Artifact%iPos", art);
	hud->SetStateInt( itemStr, 0 );
	sprintf(itemStr, "eoc_Artifact%iVis", art);
	hud->SetStateBool( itemStr, false );
	sprintf(itemStr, "eoc_Artifact%iQty", art);
	hud->SetStateInt( itemStr, 0 );
	sprintf(itemStr, "eoc_Artifact%iRotate", art);
	hud->SetStateFloat( itemStr, 0.0f );

	if (BeltSelection > belt) {
		BeltSelection--;
	}

	delete Artifact[belt];
	Artifact.RemoveIndex(belt);
	Artifact.Condense();

	return false;
}

void idPlayer::ArtifactValidateSelection( void ) {
	int belt;

	if ( gameLocal.inCinematic || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	if (BeltSelection == -1) {
		ArtifactScrollRight(1);
		return;
	}

	if ( ArtifactVerify(BeltSelection) ) {
		return;
	}

	belt=BeltSelection;
	ArtifactScrollRight(0); // we start scrolling right from where we are because idList removes gaps between list items.

	if ( belt == BeltSelection ) {
		ArtifactScrollLeft(0);
	}

	if ( belt == BeltSelection ) {
		BeltSelection = -1;
	}
}

void idPlayer::ArtifactScrollRight( int startfrom ) {
	int belt;

	if ( gameLocal.inCinematic || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	for (belt=BeltSelection+startfrom; belt <Artifact.Num(); belt++) {
		if (InventoryItemQty(belt) > 0 ) {
			BeltSelection = belt;
			UpdateHudArtifacts();
			return;
		}
	}
}

void idPlayer::ArtifactScrollLeft( int startfrom ) {
	int belt;

	if ( gameLocal.inCinematic || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	for (belt=BeltSelection-startfrom; belt > -1; belt--) {
		if (InventoryItemQty(belt) > 0 ) {
			BeltSelection = belt;
			UpdateHudArtifacts();
			return;
		}
	}
}

void idPlayer::ArtifactDrop( int belt, bool randomPos ) {

	CleanupArtifactItems();

	if ( !ArtifactVerify(belt) ) {
		UpdateHudArtifacts();
		return;
	}

	const char *name=Artifact[belt]->GetString("name");

	idEntity*	ent;
	idItem*		item;

	for (int i = 0; i < MAX_GENTITIES; i++ ) {
		ent = gameLocal.entities[i];
		if ( !ent ) {
			continue;
		}
		
		if ( !ent->IsType( idItem::Type ) ) {
			continue;
		}
		
		item = static_cast< idItem* >( ent );

		if ( item->GetOwner() != this ) {
			continue;
		}
		
		if ( idStr::Icmp(name, item->spawnArgs.GetString("inv_name") ) ) {
			continue;
		}

		if ( item->Processing ) {
			continue;
		}

		idAngles use=viewAngles;
		float	dist=80;
		if ( randomPos ) {
			use.yaw = gameLocal.random.CRandomFloat() * 360.0f;
			dist = 40 + gameLocal.random.CRandomFloat() * 40;
		}

		idVec3 org = GetPhysics()->GetOrigin() + idAngles( 0, use.yaw, 0 ).ToForward() * dist + idVec3( 0, 0, 1 );

		idDict dict;
		dict.Set( "classname", item->spawnArgs.GetString("classname") );
		dict.SetVector( "origin", org );
		//dict.SetAngles( "angles", viewAngles.yaw + 180 );

		idEntity *newItem;
		gameLocal.SpawnEntityDef( dict, &newItem );
		static_cast< idItem* >(newItem)->PickupDelayTime = gameLocal.time + 3000;

		RemoveInventoryItem(name);
		item->DeleteMe = true;
		item->SetOwner(NULL);
		UpdateHudArtifacts();

		break;
	}

	return;
}

bool idPlayer::ArtifactRemove( int belt ) {

	CleanupArtifactItems();

	if ( !ArtifactVerify(belt) ) {
		UpdateHudArtifacts();
		return false;
	}

	const char *name=Artifact[belt]->GetString("name");

	idItem *item;
	int hash, i;

	hash = gameLocal.entypeHash.GenerateKey( idItem::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idItem::Type ) ) {
			item = static_cast< idItem* >( gameLocal.entities[i] );

			if ( item->GetOwner() !=  this ) {
				continue;
			}

			if ( idStr::Icmp(name, item->spawnArgs.GetString("inv_name")) ) {
				continue;
			}

			if ( item->Processing ) {
				continue;
			}

			RemoveInventoryItem(name);
			item->DeleteMe = true;
			item->SetOwner(NULL);
			UpdateHudArtifacts();
			return true;
		}
	}
	return false;
}

void idPlayer::ShowArtifactHud( void ) {
	hud->HandleNamedEvent( "eoc_BeltShow" );
}

void idPlayer::UpdateArtifactHudDescription( void ) {
	if ( g_noArtifactDescriptions.GetBool() ) {
		hud->SetStateBool( "artifactDescShow", 0.0f );
		hud->SetStateString( "artifactDesc", "" );
		return;
	}

	CleanupArtifactItems();

	if (!ArtifactVerify(BeltSelection)) {
		hud->SetStateString( "artifactDesc", "" );
		return;
	}

	hud->SetStateBool( "artifactDescShow", 1.0f );

	idStr		desc="";

	const idDict *item = gameLocal.FindEntityDefDict( Artifact[BeltSelection]->GetString("defname") );

	if ( !item ) {
		desc = "";
	} else {
		desc = item->GetString("eoc_description");

		if ( desc == "" ) {
			switch( inventory.Class ) {
				case CLERIC:	desc = "eoc_description_cleric";	break;
				case MAGE:		desc = "eoc_description_mage";		break;
				case FIGHTER:	desc = "eoc_description_fighter";	break;
			}
			desc = item->GetString( desc.c_str() );
		}

		desc = "\n" + desc;
		desc = Artifact[BeltSelection]->GetString("name") + desc;
	}

	hud->SetStateString( "artifactDesc", desc.c_str() );

}

// z.todo: this COULD be done with a post event ...
void idPlayer::CleanupArtifactItems() {
	idItem *item;
	int hash, i;
	bool upd=false;

	hash = gameLocal.entypeHash.GenerateKey( idItem::Type.classname, true );

	for ( i = gameLocal.entypeHash.First( hash ); i != -1; i = gameLocal.entypeHash.Next( i ) ) {
		if ( gameLocal.entities[i] && gameLocal.entities[i]->IsType( idItem::Type ) ) {
			item = static_cast< idItem* >( gameLocal.entities[i] );

			if ( item->GetOwner() !=  this ) {
				continue;
			}

			if ( !item->DeleteMe ) {
				continue;
			}

			delete item;
			item=NULL;
			upd=true;
		}
	}

	if ( upd ) {
		UpdateHudArtifacts();
	}
}

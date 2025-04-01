/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"

#include "EntityList.h"
#include "Entity.h"

template<class Entity> idEntityList<Entity>::idEntityList(int Entity::*idxMember) : idxMember(idxMember) {}
template<class Entity> idEntityList<Entity>::~idEntityList() {
	Clear();
}

template<class Entity> void idEntityList<Entity>::Clear() {
	for (int i = 0; i < order.Num(); i++)
		if (order[i])
			order[i]->*idxMember = -1;
	order.Clear();
}

template<class Entity> void idEntityList<Entity>::AddToEnd(Entity *ent) {
	assert(!order.Find(ent));
	int idx = order.Append(ent);
	ent->*idxMember = idx;
}

template<class Entity> bool idEntityList<Entity>::Remove(Entity *ent) {
	int idx = ent->*idxMember;
	if (idx < 0) {
		assert(!order.Find(ent));
		return false;
	}
	assert(order[idx] == ent);
	order[idx] = nullptr;
	ent->*idxMember = -1;
	return true;
}

template<class Entity> const idList<Entity*> &idEntityList<Entity>::ToList() {
	int k = 0;
	for (int i = 0; i < order.Num(); i++)
		if (order[i])
			order[k++] = order[i];
	order.SetNum(k, false);

	for (int i = 0; i < order.Num(); i++)
		order[i]->*idxMember = i;

	return order;
}
template<class Entity> void idEntityList<Entity>::FromList(idList<Entity*> &arr) {
	Clear();
	order.Swap(arr);
	for (int i = 0; i < order.Num(); i++)
		order[i]->*idxMember = i;
}

template<class Entity> void idEntityList<Entity>::Save(idSaveGame &savegame) {
	const idList<Entity*> &arr = ToList();
	savegame.WriteInt( arr.Num() );
	for (int i = 0; i < arr.Num(); i++ ) {
		savegame.WriteObject( arr[i] );
	}
}
template<class Entity> void idEntityList<Entity>::Restore(idRestoreGame &savegame) {
	int num;
	savegame.ReadInt( num );
	idList<Entity*> arr;
	for( int i = 0; i < num; i++ ) {
		Entity *ent;
		savegame.ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			arr.AddGrow( ent );
		}
	}
	FromList( arr );
}

//list of template instantiations used
template class idEntityList<idEntity>;
template class idEntityList<idLight>;

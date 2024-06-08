// Duke_cliententity.cpp
//

#include "Gamelib/Game_local.h"

/*
===================
dnGameLocal::ClientEntityJob_t
===================
*/
void dnGameLocal::ClientEntityJob_t(void* params) {
	while (true)
	{
		gameLocal.clientGamePhysicsMutex.Lock();

		for (int i = 0; i < gameLocal.clientEntityThreadWork.Num(); i++) {
			gameLocal.clientEntityThreadWork[i]->RunThreadedPhysics(ENTITYNUM_CLIENT);
		}

		gameLocal.clientEntityThreadWork.Clear();

		gameLocal.clientGamePhysicsMutex.Unlock();

		Sleep(1);
	}
}

/*
===================
dnGameLocal::SpawnClientEntityDef

Finds the spawn function for the client entity and calls it,
returning false if not found
===================
*/
bool dnGameLocal::SpawnClientEntityDef(const idDict& args, rvClientEntity** cent, bool setDefaults, const char* spawn) {
	const char* classname;
	idTypeInfo* cls;
	idClass* obj;
	idStr		error;
	const char* name;

	if (cent) {
		*cent = NULL;
	}

	spawnArgs = args;

	if (spawnArgs.GetBool("nospawn")) {
		//not meant to actually spawn, just there for some compiling process
		return false;
	}

	if (spawnArgs.GetString("name", "", &name)) {
		error = va(" on '%s'", name);
	}

	spawnArgs.GetString("classname", NULL, &classname);

	const idDeclEntityDef* def = FindEntityDef(classname, false);
	if (!def) {
		// RAVEN BEGIN
		// jscott: a NULL classname would crash Warning()
		if (classname) {
			Warning("Unknown classname '%s'%s.", classname, error.c_str());
		}
		// RAVEN END
		return false;
	}

	spawnArgs.SetDefaults(&def->dict);

	// check if we should spawn a class object
	if (spawn == NULL) {
		spawnArgs.GetString("spawnclass", NULL, &spawn);
	}

	if (spawn) {

		cls = idClass::GetClass(spawn);
		if (!cls) {
			Warning("Could not spawn '%s'.  Class '%s' not found%s.", classname, spawn, error.c_str());
			return false;
		}

		obj = cls->CreateInstance();
		if (!obj) {
			Warning("Could not spawn '%s'. Instance could not be created%s.", classname, error.c_str());
			return false;
		}

		obj->CallSpawn();

		if (cent && obj->IsType(rvClientEntity::Type)) {
			*cent = static_cast<rvClientEntity*>(obj);
		}

		return true;
	}

	Warning("%s doesn't include a spawnfunc%s.", classname, error.c_str());
	return false;
}

/*
===================
dnGameLocal::RegisterClientEntity
===================
*/
void dnGameLocal::RegisterClientEntity(rvClientEntity* cent) {
	int entityNumber;

	assert(cent);

	if (clientSpawnCount >= (1 << (32 - CENTITYNUM_BITS))) {
		//		Error( "idGameLocal::RegisterClientEntity: spawn count overflow" );
		clientSpawnCount = INITIAL_SPAWN_COUNT;
	}

	// Find a free entity index to use
	while (clientEntities[firstFreeClientIndex] && firstFreeClientIndex < MAX_CENTITIES) {
		firstFreeClientIndex++;
	}

	if (firstFreeClientIndex >= MAX_CENTITIES) {
		cent->PostEventMS(&EV_Remove, 0);
		Warning("idGameLocal::RegisterClientEntity: no free client entities");
		return;
	}

	entityNumber = firstFreeClientIndex++;

	// Add the client entity to the lists
	clientEntities[entityNumber] = cent;
	clientSpawnIds[entityNumber] = clientSpawnCount++;
	cent->entityNumber = entityNumber;
	cent->spawnNode.AddToEnd(clientSpawnedEntities);
	cent->spawnArgs.TransferKeyValues(spawnArgs);

	if (entityNumber >= num_clientEntities) {
		num_clientEntities++;
	}
}

/*
===================
dnGameLocal::UnregisterClientEntity
===================
*/
void dnGameLocal::UnregisterClientEntity(rvClientEntity* cent) {
	assert(cent);

	// No entity number then it failed to register
	if (cent->entityNumber == -1) {
		return;
	}

	cent->spawnNode.Remove();
	cent->bindNode.Remove();

	if (clientEntities[cent->entityNumber] == cent) {
		clientEntities[cent->entityNumber] = NULL;
		clientSpawnIds[cent->entityNumber] = -1;
		if (cent->entityNumber < firstFreeClientIndex) {
			firstFreeClientIndex = cent->entityNumber;
		}
		cent->entityNumber = -1;
	}
}

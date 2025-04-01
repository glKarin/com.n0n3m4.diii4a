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
#include "LoadStack.h"
#include "renderer/resources/Image.h"
#include "DeclManager.h"
#include "MapFile.h"
#include "../sound/snd_local.h"
#include "../ui/Window.h"

//we store string values in this pool
idStrPool LoadStack::pool;
#define STRPOOL (LoadStack::pool)



idCVar decl_stack( "decl_stack", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "when enabled, print load stack after every warning about missing asset" );

void LoadStack::Clear() {
	for (int i = 0; i < MAX_LEVELS; i++)
		levels[i].Clear();
}

LoadStack::~LoadStack() {
	Clear();
}

LoadStack::LoadStack(const LoadStack &src) {
	memcpy(this, &src, sizeof(*this));
	for (int i = 0; i < MAX_LEVELS; i++)
		for (int j = 0; j < 2; j++)
			if (const idPoolStr* &s = levels[i].values[j])
				s = STRPOOL.CopyString(s);
}

template<LoadStack::Type type, class T> static LoadStack::Level CreateLevel(T *ptr) {
	LoadStack::Level res;
	res.type = type;
	res.ptr = ptr;
	return res;
}
template<> LoadStack::Level LoadStack::LevelOf(idDecl *ptr) {
	return CreateLevel<tDecl>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idImageAsset *ptr) {
	return CreateLevel<tImage>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idSoundSample *ptr) {
	return CreateLevel<tSoundSample>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idRenderModel *ptr) {
	return CreateLevel<tModel>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idEntity *ptr) {
	return CreateLevel<tEntity>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idMapEntity *ptr) {
	return CreateLevel<tMapEntity>(ptr);
}
template<> LoadStack::Level LoadStack::LevelOf(idWindow *ptr) {
	return CreateLevel<tWindow>(ptr);
}

void LoadStack::Append(const Level &lvl) {
	int i;
	for (i = 0; i < MAX_LEVELS; i++)
		if (levels[i].type == tNone)
			break;
	if (i == MAX_LEVELS)
		return;	//overflow, save top-level part
	levels[i] = lvl;
	//fetch string from pointer and save them immediately
	levels[i].SaveStrings();
}

void LoadStack::Rollback(void *ptr) {
	int i;
	for (i = MAX_LEVELS - 1; i >= 0; i--)
		if (levels[i].ptr == ptr)
			break;
	if (i < 0)
		return;	//such level does not exist
	for (; i < MAX_LEVELS; i++)
		levels[i].Clear();
}

void LoadStack::PrintStack(int indent, const Level &addLastLevel) const {
	if (!decl_stack.GetBool())
		return;	//disabled
	for (int i = 0; i < MAX_LEVELS; i++) {
		if (levels[i].type == tNone || levels[i].ptr == nullptr)
			continue;
		levels[i].Print(indent);
	}
	if (addLastLevel.type != tNone) {
		//temporarily set string for printing
		Level temp = addLastLevel;
		temp.SaveStrings();
		temp.Print(indent);
		temp.FreeStrings();
	}
}
void LoadStack::Level::Print(int indent) const {
	if (!decl_stack.GetBool())
		return;	//disabled
	char spaces[256];
	memset(spaces, ' ', indent);
	spaces[indent] = 0;
	if (type == tNone)
		common->Printf("%s[none]\n", spaces);
	else if (type == tDecl)
		common->Printf("%s[decl: %s in %s]\n", spaces, values[0]->c_str(), values[1]->c_str());
	else if (type == tImage)
		common->Printf("%s[image: %s]\n", spaces, values[0]->c_str());
	else if (type == tSoundSample)
		common->Printf("%s[sound: %s]\n", spaces, values[0]->c_str());
	else if (type == tModel)
		common->Printf("%s[model: %s]\n", spaces, values[0]->c_str());
	else if (type == tMapEntity)
		common->Printf("%s[map entity: %s]\n", spaces, values[0]->c_str());
	else if (type == tEntity)
		common->Printf("%s[game entity: %s]\n", spaces, values[0]->c_str());
	else if (type == tWindow)
		common->Printf("%s[window: %s]\n", spaces, values[0]->c_str());
	else
		common->Printf("%s[corrupted: %d]\n", spaces, type);
}

void LoadStack::Level::SaveStrings() {
	FreeStrings();

	if (type == tDecl) {
		values[0] = STRPOOL.AllocString(decl->GetName());
		values[1] = STRPOOL.AllocString(decl->GetFileName());
	}
	else if (type == tImage)
		values[0] = STRPOOL.AllocString(image->imgName.c_str());
	else if (type == tSoundSample)
		values[0] = STRPOOL.AllocString(soundSample->name.c_str());
	else if (type == tModel)
		values[0] = STRPOOL.AllocString(model->Name());
	else if (type == tMapEntity)
		values[0] = STRPOOL.AllocString(mapEntity->epairs.GetString("name"));
	else if (type == tEntity)
		values[0] = STRPOOL.AllocString(entity->GetName());
	else if (type == tWindow)
		values[0] = STRPOOL.AllocString(window->GetName());
}

void LoadStack::Level::FreeStrings() {
	for (int j = 0; j < 2; j++)
		if (const idPoolStr* &s = values[j]) {
			STRPOOL.FreeString(s);
			s = nullptr;
		}
}
void LoadStack::Level::Clear() {
	FreeStrings();
	type = tNone;
	ptr = nullptr;
}

void LoadStack::ShowMemoryUsage_f( const idCmdArgs &args ) {
	idLib::common->Printf( "%5zu KB in %d loadstack strings\n", pool.Size() >> 10, pool.Num() );
}
void LoadStack::ListStrings_f( const idCmdArgs &args ) {
	pool.PrintAll("loadStack");
}

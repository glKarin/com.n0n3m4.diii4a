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
#pragma hdrstop



#include "MaterialDef.h"


/**
* Constructor.
*/
MaterialDef::MaterialDef(void) {
	type = 0;
	quotes = false;
}

/**
* Destructor.
*/
MaterialDef::~MaterialDef(void) {
}

/**
* Returns view specific data associated with the material definition.
*/
DWORD MaterialDef::GetViewData(const char* viewName) {
	DWORD* value = NULL;
	viewData.Get(viewName, &value);
	return *value;
}

/**
* Sets view specific data for the material definition.
*/
void MaterialDef::SetViewData(const char* viewName, DWORD value) {
	viewData.Set(viewName, value);
}

#define MATERIAL_DEF_FILE "MaterialEditorDefs.med"

MaterialDefList MaterialDefManager::materialDefs[MaterialDefManager::MATERIAL_DEF_NUM];


/**
* Loads the material definition file instatiates MaterialDef objects for each definition
* and groups the definitions.
*/
void MaterialDefManager::InitializeMaterialDefLists() {
	
	char	*buffer;
	int length = fileSystem->ReadFile( MATERIAL_DEF_FILE, (void **)&buffer);

	if ( length == -1 ) {
		common->Error( "Couldn't load material editor definition: %s", MATERIAL_DEF_FILE );
		return;
	}

	idLexer src;
	if ( !src.LoadMemory( buffer, length, MATERIAL_DEF_FILE ) ) {
		common->Error( "Couldn't parse %s", MATERIAL_DEF_FILE );
		fileSystem->FreeFile(buffer);
	}


	InitializeMaterialDefList(&src, "materialprops", &materialDefs[MATERIAL_DEF_MATERIAL]);
	InitializeMaterialDefList(&src, "stageprops", &materialDefs[MATERIAL_DEF_STAGE]);
	InitializeMaterialDefList(&src, "specialmapstageprops", &materialDefs[MATERIAL_DEF_SPECIAL_STAGE]);

	fileSystem->FreeFile(buffer);
}

/**
* Loads a single type of material attributes and adds them to the supplied MaterialDefList object.
* @param src The idLexer object that contains the file.
* @param typeName The name of the attribute grouping to search for in the file.
* @param list The MaterialDefList object to append the MaterialDef instances to.
*/
void MaterialDefManager::InitializeMaterialDefList(idLexer* src, const char* typeName, MaterialDefList* list) {

	idToken token;

	src->Reset();
	src->SkipUntilString(typeName);
	src->SkipUntilString("{");

	while(1) {
		if ( !src->ExpectAnyToken( &token ) ) {
			//Todo: Add some error checking here
			return;
		}

		if ( token == "}" ) {
			break;
		}

		MaterialDef* newProp = new MaterialDef();

		if(!token.Icmp("TYPE_GROUP")) {
			newProp->type = MaterialDef::MATERIAL_DEF_TYPE_GROUP;
		} else if(!token.Icmp("TYPE_BOOL")) {
			newProp->type = MaterialDef::MATERIAL_DEF_TYPE_BOOL;
		} else if(!token.Icmp("TYPE_STRING")) {
			newProp->type = MaterialDef::MATERIAL_DEF_TYPE_STRING;
		} else if(!token.Icmp("TYPE_FLOAT")) {
			newProp->type = MaterialDef::MATERIAL_DEF_TYPE_FLOAT;
		} else if(!token.Icmp("TYPE_INT")) {
			newProp->type = MaterialDef::MATERIAL_DEF_TYPE_INT;
		}

		//Skip the ,
		src->ReadToken(&token);

		//Read Dict Name
		src->ReadToken(&token);
		newProp->dictName = token;

		//Skip the ,
		src->ReadToken(&token);

		//Read Display Name
		src->ReadToken(&token);
		newProp->displayName = token;

		//Skip the ,
		src->ReadToken(&token);

		//Read Display Info
		src->ReadToken(&token);
		newProp->displayInfo = token;

		//Type Specific Data
		if(newProp->type == MaterialDef::MATERIAL_DEF_TYPE_STRING) {

			newProp->quotes = false;

			//Skip the ,
			src->ReadToken(&token);

			//Read validate flag
			src->ReadToken(&token);
			if(token == "1") {
				newProp->quotes = true;
			}
		}

		src->SkipRestOfLine();

		list->Append(newProp);
	}
}

/**
* Destroys all MaterialDef instances and clears the material attribute grouping lists.
*/
void MaterialDefManager::DestroyMaterialDefLists() {
	
	for(int i = 0; i < MATERIAL_DEF_NUM; i++) {
		for(int j = 0; j < materialDefs[i].Num(); j++) {
			delete materialDefs[i][j];
		}
		materialDefs[i].ClearFree();
	}
}

/**
* Returns the MaterialDefList for the specified attribute grouping.
* @param type The attribute grouping for which to retreive the attribute list.
*/
MaterialDefList* MaterialDefManager::GetMaterialDefs(int type) {
	if(type >= 0 && type < MATERIAL_DEF_NUM) {
		return &materialDefs[type];
	}
	return NULL;
}
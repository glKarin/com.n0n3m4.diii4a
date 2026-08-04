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
#pragma once

/**
* Represents a single attribute in a material. Represents material, stage
* and special stage attributes. The MaterialDef manager loads these
* definitions from the material definition file as the editor
* is being initialized.
*/
class MaterialDef {

public:
	/**
	* Defines possible attribute types.
	*/
	enum {
		MATERIAL_DEF_TYPE_GROUP,
		MATERIAL_DEF_TYPE_BOOL,
		MATERIAL_DEF_TYPE_STRING,
		MATERIAL_DEF_TYPE_FLOAT,
		MATERIAL_DEF_TYPE_INT
	};
	
	int					type;
	idStr				dictName;
	idStr				displayName;
	idStr				displayInfo;
	bool				quotes;
	idHashTable<DWORD>	viewData;
	
public:
	
	MaterialDef(void);
	virtual ~MaterialDef(void);

	DWORD	GetViewData(const char* viewName);
	void	SetViewData(const char* viewName, DWORD value);
};

/**
* A list of material attributes. Material, stage, and special stage attributes
* are grouped together during the load process for use by the different view and
* MaterialDoc.
*/
typedef idList<MaterialDef*> MaterialDefList;

/**
* This class contains static utility functions that view and MaterialDoc use
* to access the MaterialDef and MaterialDefList data that is loaded. This class
* is also responsible for loading and destroying the MaterialDef instances.
*/
class MaterialDefManager {

public:

	/**
	* Defines the groupings of material attributes.
	*/
	enum {
		MATERIAL_DEF_MATERIAL = 0,
		MATERIAL_DEF_STAGE,
		MATERIAL_DEF_SPECIAL_STAGE,
		MATERIAL_DEF_NUM
	};

	static void 				InitializeMaterialDefLists();
	static void					InitializeMaterialDefList(idLexer* src, const char* typeName, MaterialDefList* list);
	
	static void					DestroyMaterialDefLists();

	static MaterialDefList*		GetMaterialDefs(int type);


protected:
	static MaterialDefList		materialDefs[MATERIAL_DEF_NUM];	
};

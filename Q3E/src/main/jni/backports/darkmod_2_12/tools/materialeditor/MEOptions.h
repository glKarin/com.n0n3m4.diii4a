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

#include "../common/registryoptions.h"

/**
* Wrapper class that is responsible for reading and writing Material Editor
* settings to the registry. Settings are written to 
* Software\\id Software\\DOOM3\\Tools\\MaterialEditor
*/
class MEOptions {

public:
	MEOptions();
	~MEOptions();

	bool				Save (void);
	bool				Load (void);

	/**
	* Sets the flag that determines if the settings need to be saved because
	* they where modified.
	*/
	void				SetModified(bool mod = true) { modified = mod; };
	/**
	* Get the flag that determines if the settings need to be saved because
	* they where modified.
	*/
	bool				GetModified() { return modified; };

	void				SetWindowPlacement		( const char* name, HWND hwnd );
	bool				GetWindowPlacement		( const char* name, HWND hwnd );

	void				SetMaterialTreeWidth(int width);
	int					GetMaterialTreeWidth();

	void				SetStageWidth(int width);
	int					GetStageWidth();

	void				SetPreviewPropertiesWidth(int width);
	int					GetPreviewPropertiesWidth();

	void				SetMaterialEditHeight(int height);
	int					GetMaterialEditHeight();

	void				SetMaterialPropHeadingWidth(int width);
	int					GetMaterialPropHeadingWidth();

	void				SetPreviewPropHeadingWidth(int width);
	int					GetPreviewPropHeadingWidth();

protected:
	rvRegistryOptions	registry;

	bool				modified;

	int					materialTreeWidth;
	int					stageWidth;
	int					previewPropertiesWidth;
	int					materialEditHeight;
	int					materialPropHeadingWidth;
	int					previewPropHeadingWidth;
};


ID_INLINE void MEOptions::SetWindowPlacement ( const char* name, HWND hwnd ) {
	registry.SetWindowPlacement ( name, hwnd );
}

ID_INLINE bool MEOptions::GetWindowPlacement ( const char* name, HWND hwnd ) {
	return registry.GetWindowPlacement ( name, hwnd );
}

ID_INLINE void MEOptions::SetMaterialTreeWidth(int width) {
	materialTreeWidth = width;
	SetModified(true);
}

ID_INLINE int MEOptions::GetMaterialTreeWidth() {
	return materialTreeWidth;
}

ID_INLINE void MEOptions::SetStageWidth(int width) {
	stageWidth = width;
	SetModified(true);
}

ID_INLINE int MEOptions::GetStageWidth() {
	return stageWidth;
}

ID_INLINE void MEOptions::SetPreviewPropertiesWidth(int width) {
	previewPropertiesWidth = width;
	SetModified(true);
}

ID_INLINE int MEOptions::GetPreviewPropertiesWidth() {
	return previewPropertiesWidth;
}

ID_INLINE void MEOptions::SetMaterialEditHeight(int height) {
	materialEditHeight = height;
	SetModified(true);
}

ID_INLINE int MEOptions::GetMaterialEditHeight() {
	return materialEditHeight;
}

ID_INLINE void MEOptions::SetMaterialPropHeadingWidth(int width) {
	materialPropHeadingWidth = width;
	SetModified(true);
}

ID_INLINE int MEOptions::GetMaterialPropHeadingWidth() {
	return materialPropHeadingWidth;
}

ID_INLINE void MEOptions::SetPreviewPropHeadingWidth(int width) {
	previewPropHeadingWidth = width;
	SetModified(true);
}

ID_INLINE int MEOptions::GetPreviewPropHeadingWidth() {
	return previewPropHeadingWidth;
}
// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_GUIS_USERINTERFACECINEMATIC_H__
#define __GAME_GUIS_USERINTERFACECINEMATIC_H__

#include "UserInterfaceTypes.h"

/*
============
sdUICinematic
============
*/
class sdUICinematic :
	public sdUIWindow {
public:
	SD_UI_DECLARE_ABSTRACT_CLASS( sdUICinematic )
				
							sdUICinematic();
	virtual					~sdUICinematic();

protected:
	virtual void			DrawLocal();

	void					OnSoundShaderChanged( const idStr& oldValue, const idStr& newValue );
	void					OnActiveChanged( const float oldValue, const float newValue );

protected:
	SD_UI_PROPERTY_TAG(
	title					= "1. Common/SoundShader";
	desc					= "Cinematic's sound shader.";
	editor					= "edit";
	datatype				= "string";
	alias					= "soundshader";
	)
	sdStringProperty		soundShaderName;

	SD_UI_PROPERTY_TAG(
	title					= "1. Common/active";
	desc					= "Cinematic is active.";
	editor					= "edit";
	datatype				= "float";
	)
	sdFloatProperty 		active;

	sdFloatProperty			looping;

	const idSoundShader*	soundShader;

	idSoundWorld*			sw;
	idSoundEmitter*			soundEmitter;
};

#endif // ! __GAME_GUIS_USERINTERFACECINEMATIC_H__

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

#ifndef __GAME_SOUND_H__
#define __GAME_SOUND_H__

/*
===============================================================================

  Generic sound emitter.

===============================================================================
*/

class idSound : public idEntity {
public:
	CLASS_PROTOTYPE( idSound );

					idSound( void );

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	virtual void	UpdateChangeableSpawnArgs( const idDict *source ) override;

	void			Spawn( void );

	void			ToggleOnOff( idEntity *other, idEntity *activator );
	virtual void	Think( void ) override;
	void			SetSound( const char *sound, int channel = SND_CHANNEL_ANY );

	virtual void	ShowEditingDialog( void ) override;

private:
	float			lastSoundVol;
	float			soundVol;
	float			random;
	float			wait;
	bool			timerOn;
	idVec3			shakeTranslate;
	idAngles		shakeRotate;
	int				playingUntilTime;

	void			Event_Trigger( idEntity *activator );
	void			Event_Timer( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			DoSound( bool play );
};

#endif /* !__GAME_SOUND_H__ */

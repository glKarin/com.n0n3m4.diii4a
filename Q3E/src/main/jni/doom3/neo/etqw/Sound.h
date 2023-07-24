// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_SOUND_H__
#define __GAME_SOUND_H__

/*
===============================================================================

  Generic sound emitter.

===============================================================================
*/

class sdSoundBroadcastData : public sdEntityStateNetworkData {
public:
	sdSoundBroadcastData() {
		isOn = false;
	}

	virtual void		MakeDefault( void );

	virtual void		Write( idFile* file ) const;
	virtual void		Read( idFile* file );

	bool				isOn;
};

class idSound : public idEntity {
public:
	CLASS_PROTOTYPE( idSound );

					idSound( void );

	virtual void	UpdateChangeableSpawnArgs( const idDict *source );

	static void		OnNewMapLoad( void );
	static void		OnMapClear( void );
	static void		FreeMapSounds( void );

	void			Spawn( void );

	void			Think( void );
	void			SetSound( const char *sound, int channel = SND_ANY );

	virtual void	ShowEditingDialog( void );

	virtual bool	StartSynced( void ) const { return spawnArgs.GetBool( "dynamic" ) == 1; }

	// networking methods
	virtual void	ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void	ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void	WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

private:
	float			lastSoundVol;
	float			soundVol;
	float			random;
	float			wait;
	bool			timerOn;
	bool			soundOn;
	int				playingUntilTime;

	void			Event_Trigger( idEntity *activator );
	void			Event_Timer( void );
	void			Event_On( void );
	void			Event_Off( void );
	void			DoSound( bool play );

	static idList< idSoundEmitter* > s_soundEmitters;
};

#endif /* !__GAME_SOUND_H__ */

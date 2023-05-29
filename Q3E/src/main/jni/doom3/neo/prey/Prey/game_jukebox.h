
#ifndef __GAME_JUKEBOX_H__
#define __GAME_JUKEBOX_H__


class hhJukeBox : public hhConsole {
public:
	CLASS_PROTOTYPE( hhJukeBox );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);
	virtual void		ConsoleActivated();
	virtual void		Think();

	void				ClearSpectrum();
	void				SetTrack(int track);
	void				PlayCurrentTrack();
	void				StopCurrentTrack();
	void				UpdateView();
	void				UpdateVolume();
	void				UpdateEntityVolume(idEntity *ent);

	void				Event_SetNumTracks(int newNumTracks);
	void				Event_SetTrack(int newTrack);
	void				Event_PlaySelected();
	void				Event_TrackOver();
	void				Event_SetVolume(float vol);

protected:
	float				volume;
	int					track;
	int					numTracks;
	int					currentHistorySample;
	idList<idEntity*>	speakers;
};


class hhJukeBoxSpeaker : public hhSound {
public:
	CLASS_PROTOTYPE( hhJukeBoxSpeaker );
};


#endif

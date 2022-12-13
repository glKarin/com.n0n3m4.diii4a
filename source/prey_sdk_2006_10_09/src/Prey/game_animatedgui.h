
#ifndef __GAME_ANIMATEDGUI_H__
#define __GAME_ANIMATEDGUI_H__


class hhAnimatedGui : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE(hhAnimatedGui);

	void			Spawn(void);
					hhAnimatedGui();
	virtual			~hhAnimatedGui();
	virtual void	Think();
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

protected:
	void			FadeInGui();
	void			FadeOutGui();

	void			Event_Trigger( idEntity *activator );
	void			Event_PlayIdle();

protected:
	bool					bOpen;
	int						idleOpenAnim;
	int						idleCloseAnim;
	int						openAnim;
	int						closeAnim;
	idInterpolate<float>	guiScale;
	idEntity *				attachedConsole;
};

#endif /* __GAME_ANIMATEDGUI_H__ */

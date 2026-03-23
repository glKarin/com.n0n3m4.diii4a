class idGameLaserDuckWindow;

class LaserDuckEntity {
public:
	const idMaterial *		material;
	idStr					materialName;
	float					width, height;
	bool					visible;

	idVec4					entColor;
	idVec2					position;
	float					rotation;
	float					rotationSpeed;
	idVec2					velocity;

	bool					fadeIn;
	bool					fadeOut;

	idGameLaserDuckWindow *	game;
	
public:
						LaserDuckEntity(idGameLaserDuckWindow* _game);
	virtual				~LaserDuckEntity();

	virtual void		WriteToSaveGame( idFile *savefile );
	virtual void		ReadFromSaveGame( idFile *savefile, idGameLaserDuckWindow* _game );

	void				SetMaterial(const char* name);
	void				SetSize( float _width, float _height );
	void				SetVisible( bool isVisible );

	virtual void		Update( float timeslice );
	virtual void		Draw(idDeviceContext *dc);

private:
};


class idGameLaserDuckWindow : public idWindow {
public:
	idGameLaserDuckWindow(idUserInterfaceLocal *gui);
	idGameLaserDuckWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	~idGameLaserDuckWindow();

	virtual void		WriteToSaveGame( idFile *savefile );
	virtual void		ReadFromSaveGame( idFile *savefile );

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual const char*	Activate(bool activate);
	virtual idWinVar *	GetWinVarByName	(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

private:
	void				CommonInit();
	void				ResetGameState();

	
	void				UpdateButtons();
	void				UpdateGame();


	virtual bool		ParseInternalVar(const char *name, idParser *src);

private:

	idWinBool			gamerunning;
	idWinBool			onNewGame;
	
	idList<LaserDuckEntity*>	entities;

	LaserDuckEntity			*duck;

};

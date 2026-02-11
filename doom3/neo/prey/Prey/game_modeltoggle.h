#ifndef __GAME_MODELTOGGLE_H__
#define __GAME_MODELTOGGLE_H__

typedef struct ResourceSet_s{
	idStr			model;
	idList<idStr>	animList;
	idList<idStr>	animFileList;
	const idDict *	args;
} ResourceSet;


class hhViewedModel : public hhAnimatedEntity {
public:
	CLASS_PROTOTYPE( hhViewedModel );

	void					Spawn();
	void					SetRotationAmount(float amount);

	idPhysics_Parametric	physicsObj;
	float					rotationAmount;

protected:
	virtual void			Ticker();
};


class hhModelToggle : public hhConsole {
public:
	CLASS_PROTOTYPE( hhModelToggle );

	void					Spawn( void );
	virtual bool			HandleSingleGuiCommand(idEntity *entityGui, idLexer *src);

protected:
	void					NextModel(void);
	void					PrevModel(void);
	void					NextAnim(void);
	void					PrevAnim(void);
	void					SetTargetsToModel(const char *modelname);
	void					SetTargetsToAnim(const char *animname);
	void					FillResourceList();
	void					UpdateGUIValues();
	void					Update();
	void					SetTargetsToRotate(bool bRotate);

	void					Event_SetInitialAnims();
	void					Event_RequireTargets();

protected:
	idList<idStr>			defList;
	idList<ResourceSet>		resources;

	int						currentModel;
	int						currentAnim;
	bool					bTranslation;
	bool					bRotation;
	bool					bCycle;

};


#endif /* __GAME_MODELTOGGLE_H__ */

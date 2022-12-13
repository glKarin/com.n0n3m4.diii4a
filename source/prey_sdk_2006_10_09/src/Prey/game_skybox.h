
#ifndef __GAME_SKYBOX_H__
#define __GAME_SKYBOX_H__

class hhSkybox : public idEntity {
public:
	CLASS_PROTOTYPE( hhSkybox );

	void		Spawn(void);

	virtual void Present( void );
};

#endif /* __GAME_SKYBOX_H__ */

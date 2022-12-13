
#ifndef __PREY_SPIRITBRIDGE_H__
#define __PREY_SPIRITBRIDGE_H__

//class hhPlayer;

class hhSpiritBridge : public idEntity {
public:
	CLASS_PROTOTYPE( hhSpiritBridge );

	void		Spawn( void );

protected:
	void		Event_Activate( idEntity *activator );
};

#endif /* __PREY_SPIRITBRIDGE_H__ */

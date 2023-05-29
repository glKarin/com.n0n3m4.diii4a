
#ifndef __PREY_SPIRITSECRET_H__
#define __PREY_SPIRITSECRET_H__

class hhSpiritSecret : public idEntity {
public:
	CLASS_PROTOTYPE( hhSpiritSecret );

	void		Spawn( void );

protected:
	void		Event_Activate( idEntity *activator );
};

#endif /* __PREY_SPIRITSECRET_H__ */

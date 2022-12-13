#ifndef __HH_ITEM_AUTOMATIC_H
#define __HH_ITEM_AUTOMATIC_H

/***********************************************************************

hhItemAutomatic

***********************************************************************/

class hhItemAutomatic : public idEntity {
	CLASS_PROTOTYPE( hhItemAutomatic );
	
public:
	void			Spawn();
	virtual void	Think();


protected:
	void		SpawnItem();
	idStr		GetNewItem();
	float		FindAmmoNearby( const char *ammoName );
};

#endif
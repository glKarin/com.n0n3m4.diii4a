// DnItem.h
//

class DukePlayer;

//
// DnItem
//
class DnItem : public idEntity
{
public:
	CLASS_PROTOTYPE(DnItem);

	void Spawn(void);
	void Think(void);

	void Event_Touch(idEntity* other, trace_t* trace);

	idVec3 orgOrigin;
protected:
	virtual void  TouchEvent(DukePlayer* player, trace_t* trace);	
};

#include "DnItemShotgun.h"
#ifndef __A_KEYS_H__
#define __A_KEYS_H__

#include "actor.h"
#include "g_shared/a_inventory.h"

class AKey : public AInventory
{
	DECLARE_NATIVE_CLASS(Key, Inventory)

	public:
		unsigned int	KeyNumber;
};

void P_InitKeyMessages();
void P_DeinitKeyMessages();
bool P_CheckKeys (AActor *owner, int keynum, bool remote);
bool P_GiveKeys (AActor *owner, int keynum);

#endif
 

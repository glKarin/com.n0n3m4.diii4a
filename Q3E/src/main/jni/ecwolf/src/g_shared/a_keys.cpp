#include "id_sd.h"
#include "scanner.h"
#include "tarray.h"
#include "v_palette.h"
#include "w_wad.h"
#include "wl_iwad.h"
#include "zstring.h"
#include "thingdef/thingdef.h"
#include "g_shared/a_inventory.h"
#include "g_shared/a_keys.h"

IMPLEMENT_CLASS(Key)

class AKeyGiver : public AInventory
{
	DECLARE_NATIVE_CLASS(KeyGiver, Inventory)

	public:
		bool TryPickup(AActor *toucher)
		{
			bool pickedup = true;

			DropList *list = GetDropList();
			DropList::Iterator item = list->Head();
			while(item)
			{
				const ClassDef *cls = ClassDef::FindClass(item->className);
				++item;

				if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Key)))
				{
					pickedup = false;
					continue;
				}

				AInventory *item = static_cast<AInventory *>(AActor::Spawn(cls, 0, 0, 0, 0));
				item->RemoveFromWorld();
				if(!item->CallTryPickup(toucher))
				{
					pickedup = false;
					item->Destroy();
				}
				else
					GoAwayAndDie();
			}

			return pickedup;
		}
};
IMPLEMENT_CLASS(KeyGiver)

////////////////////////////////////////////////////////////////////////////////
//
// From ZDoom a_keys.cpp below this line
//
////////////////////////////////////////////////////////////////////////////////

struct OneKey
{
	const ClassDef *key;
	int count;

	bool check(AActor * owner)
	{
		// P_GetMapColorForKey() checks the key directly
		if (owner->IsKindOf (NATIVE_CLASS(Key)))
			return owner->IsA(key);
		// Other calls check an actor that may have a key in its inventory.
		else
			return !!owner->FindInventory(key);
	}
};

struct Keygroup
{
	TArray<OneKey> anykeylist;

	bool check(AActor *owner)
	{
		for(unsigned int i=0;i<anykeylist.Size();i++)
		{
			if (anykeylist[i].check(owner)) return true;
		}
		return false;
	}
};

struct Lock
{
	TArray<Keygroup *> keylist;
	TArray<SoundIndex> locksound;
	FString Message;
	FString RemoteMsg;
	int	rgb;

	Lock()
	{
		rgb=0;
	}

	~Lock()
	{
		for(unsigned int i=0;i<keylist.Size();i++) delete keylist[i];
		keylist.Clear();
	}

	bool check(AActor * owner)
	{
		// An empty key list means that any key will do
		if (!keylist.Size())
		{
			for (AInventory * item = owner->inventory; item != NULL; item = item->inventory)
			{
				if (item->IsKindOf (NATIVE_CLASS(Key)))
				{
					return true;
				}
			}
			return false;
		}
		else for(unsigned int i=0;i<keylist.Size();i++)
		{
			if (!keylist[i]->check(owner)) return false;
		}
		return true;
	}
};

static Lock *locks[256];		// all valid locks
static bool keysdone=false;		// have the locks been initialized?
static int currentnumber;		// number to be assigned to next key
static bool ignorekey;			// set to true when the current lock is not being used

static void ClearLocks();

static const char * keywords_lock[]={
	"ANY",
	"MESSAGE",
	"REMOTEMESSAGE",
	"MAPCOLOR",
	"LOCKEDSOUND",
	NULL
};
static int MatchString(const FString &token, const char* keywords[])
{
	int i = 0;
	do
	{
		if(token.CompareNoCase(*keywords) == 0)
			return i;
		++i;
	}
	while(*++keywords != NULL);
	return -1;
}

//===========================================================================
//
//
//===========================================================================

static void AddOneKey(Keygroup *keygroup, const ClassDef *mi, Scanner &sc)
{
	if (mi)
	{
		// Any inventory item can be used to unlock a door
		if (mi->IsDescendantOf(NATIVE_CLASS(Inventory)))
		{
			OneKey k = {mi,1};
			keygroup->anykeylist.Push (k);

			//... but only keys get key numbers!
			if (mi->IsDescendantOf(NATIVE_CLASS(Key)))
			{
				if (!ignorekey &&
					static_cast<AKey*>(mi->GetDefault())->KeyNumber == 0)
				{
					static_cast<AKey*>(mi->GetDefault())->KeyNumber=++currentnumber;
				}
			}
		}
		else
		{
			sc.ScriptMessage(Scanner::ERROR, "'%s' is not an inventory item", sc->str.GetChars());
		}
	}
	else
	{
		sc.ScriptMessage(Scanner::ERROR, "Unknown item '%s'", sc->str.GetChars());
	}
}


//===========================================================================
//
//
//===========================================================================

static Keygroup * ParseKeygroup(Scanner &sc)
{
	Keygroup * keygroup;
	const ClassDef * mi;

	sc.MustGetToken('{');
	keygroup = new Keygroup;
	while (!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		mi = ClassDef::FindClass(sc->str);
		AddOneKey(keygroup, mi, sc);
	}
	if (keygroup->anykeylist.Size() == 0)
	{
		delete keygroup;
		return NULL;
	}
	keygroup->anykeylist.ShrinkToFit();
	return keygroup;
}

//===========================================================================
//
//
//===========================================================================

static void ParseLock(Scanner &sc)
{
	int i,r,g,b;
	int keynum;
	Lock sink;
	Lock * lock=&sink;
	Keygroup * keygroup;
	const ClassDef * mi;

	sc.MustGetToken(TK_IntConst);
	keynum = sc->number;

	if (!sc.CheckToken('{'))
	{
		sc.MustGetToken(TK_Identifier);
		if (!IWad::CheckGameFilter(sc->str)) keynum = -1;
		sc.MustGetToken('{');
	}

	ignorekey = true;
	if (keynum > 0 && keynum < 255) 
	{
		lock = new Lock;
		if (locks[keynum])
		{
			delete locks[keynum];
		}
		locks[keynum] = lock;
		locks[keynum]->locksound.Push(SoundInfo.FindSound("*keytry"));
		locks[keynum]->locksound.Push(SoundInfo.FindSound("misc/keytry"));
		ignorekey=false;
	}
	else if (keynum != -1)
	{
		sc.ScriptMessage(Scanner::ERROR, "Lock index %d out of range", keynum);
	}

	while (!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		switch(i = MatchString(sc->str, keywords_lock))
		{
		case 0:	// Any
			keygroup = ParseKeygroup(sc);
			if (keygroup)
			{
				lock->keylist.Push(keygroup);
			}
			break;

		case 1:	// message
			sc.MustGetToken(TK_StringConst);
			lock->Message = sc->str;
			break;

		case 2: // remotemsg
			sc.MustGetToken(TK_StringConst);
			lock->RemoteMsg = sc->str;
			break;

		case 3:	// mapcolor
			sc.MustGetToken(TK_IntConst);
			r = sc->number;
			sc.MustGetToken(TK_IntConst);
			g = sc->number;
			sc.MustGetToken(TK_IntConst);
			b = sc->number;
			lock->rgb = MAKERGB(r,g,b);
			break;

		case 4:	// locksound
			lock->locksound.Clear();
			for (;;)
			{
				sc.MustGetToken(TK_StringConst);
				lock->locksound.Push(SoundInfo.FindSound(sc->str));
				if (!sc.CheckToken(','))
				{
					break;
				}
			}
			break;

		default:
			mi = ClassDef::FindClass(sc->str);
			if (mi) 
			{
				keygroup = new Keygroup;
				AddOneKey(keygroup, mi, sc);
				if (keygroup) 
				{
					keygroup->anykeylist.ShrinkToFit();
					lock->keylist.Push(keygroup);
				}
			}
			break;
		}
	}
	// copy the messages if the other one does not exist
	if (lock->RemoteMsg.IsEmpty() && lock->Message.IsNotEmpty())
	{
		lock->RemoteMsg = lock->Message;
	}
	if (lock->Message.IsEmpty() && lock->RemoteMsg.IsNotEmpty())
	{
		lock->Message = lock->RemoteMsg;
	}
	lock->keylist.ShrinkToFit();
}

//===========================================================================
//
// Clears all key numbers so the parser can assign its own ones
// This ensures that only valid keys are considered by the key cheats
//
//===========================================================================

static void ClearLocks()
{
	ClassDef::ClassPair *pair;
	ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
	while(iter.NextPair(pair))
	{
		if (pair->Value->IsDescendantOf(NATIVE_CLASS(Key)))
		{
			AKey *key = static_cast<AKey*>(pair->Value->GetDefault());
			if (key != NULL)
			{
				key->KeyNumber = 0;
			}
		}
	}
	for(int i=0;i<256;i++)
	{
		if (locks[i]!=NULL) 
		{
			delete locks[i];
			locks[i]=NULL;
		}
	}
	currentnumber=0;
	keysdone=false;
}

//===========================================================================
//
// P_InitKeyMessages
//
//===========================================================================

void P_InitKeyMessages()
{
	int lastlump, lump;

	lastlump = 0;

	ClearLocks();
	while ((lump = Wads.FindLump ("LOCKDEFS", &lastlump)) != -1)
	{
		Scanner sc(lump);
		do
		{
			sc.MustGetToken(TK_Identifier);
			if (sc->str.CompareNoCase("LOCK") == 0) 
			{
				ParseLock(sc);
			}
			else if (sc->str.CompareNoCase("CLEARLOCKS") == 0)
			{
				// clear all existing lock definitions and key numbers
				ClearLocks();
			}
			else
			{
				sc.ScriptMessage(Scanner::ERROR, "Unknown command %s in LockDef", sc->str.GetChars());
			}
		}
		while(sc.TokensLeft());
	}
	keysdone = true;
}

//===========================================================================
//
// P_DeinitKeyMessages
//
//===========================================================================

void P_DeinitKeyMessages()
{
	ClearLocks();
}

//===========================================================================
//
// P_CheckKeys
//
// Returns true if the actor has the required key. If not, a message is
// shown if the actor is also the consoleplayer's camarea, and false is
// returned.
//
//===========================================================================

bool P_CheckKeys (AActor *owner, int keynum, bool remote)
{
	const char *failtext = NULL;
	SoundIndex *failsound;
	int numfailsounds;

	if (owner == NULL) return false;
	if (keynum<=0 || keynum>255) return true;
	// Just a safety precaution. The messages should have been initialized upon game start.
	if (!keysdone) P_InitKeyMessages();

	SoundIndex failage[2] = { SoundInfo.FindSound("*keytry"), SoundInfo.FindSound("misc/keytry") };

	if (!locks[keynum]) 
	{
		/*if (keynum == 103 && (gameinfo.flags & GI_SHAREWARE))
			failtext = "$TXT_RETAIL_ONLY";
		else*/
			failtext = "$TXT_DOES_NOT_WORK";

		failsound = failage;
		numfailsounds = countof(failage);
	}
	else
	{
		if (locks[keynum]->check(owner)) return true;
		failtext = remote? locks[keynum]->RemoteMsg : locks[keynum]->Message;
		failsound = &locks[keynum]->locksound[0];
		numfailsounds = locks[keynum]->locksound.Size();
	}

	// If we get here, that means the actor isn't holding an appropriate key.

	/*if (owner == players[consoleplayer].camera)
	{
		PrintMessage(failtext);

		// Play the first defined key sound.
		for (int i = 0; i < numfailsounds; ++i)
		{
			if (failsound[i] != 0)
			{
				int snd = S_FindSkinnedSound(owner, failsound[i]);
				if (snd != 0)
				{
					S_Sound (owner, CHAN_VOICE, snd, 1, ATTN_NORM);
					break;
				}
			}
		}
	}*/

	return false;
}

bool P_GiveKeys (AActor *owner, int keynum)
{
	if (owner == NULL) return false;
	if (keynum<=0 || keynum>255) return false;

	Lock *lock = locks[keynum];
	if(!lock)
		return false;

	for(unsigned int i = lock->keylist.Size();i-- > 0;)
	{
		for(unsigned int j = lock->keylist[i]->anykeylist.Size();j-- > 0;)
		{
			OneKey &key = lock->keylist[i]->anykeylist[j];
			if(!key.check(owner))
			{
				AKey *newKey = (AKey*) AActor::Spawn(key.key, 0, 0, 0, 0);
				newKey->RemoveFromWorld();
				if(!newKey->CallTryPickup(owner))
					newKey->Destroy();
			}
		}
	}
	return true;
}

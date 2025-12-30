/*
** lnspec.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "doomerrors.h"
#include "id_ca.h"
#include "id_sd.h"
#include "g_conversation.h"
#include "lnspec.h"
#include "actor.h"
#include "m_random.h"
#include "sndseq.h"
#include "thinker.h"
#include "wl_act.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "wl_play.h"
#include "g_mapinfo.h"
#include "g_shared/a_keys.h"
#include "thingdef/thingdef.h"
using namespace Specials;

static FRandom pr_teleport("Teleport");

#define DEFINE_SPECIAL(name,num,argc) \
	static int LN_##name(MapSpot spot, const int args[], MapTrigger::Side direction, AActor *activator);
#define FUNC(name) \
	static int LN_##name(MapSpot spot, const int args[], MapTrigger::Side direction, AActor *activator)

DEFINE_SPECIAL(NOP, 0, 0)
#include "lnspecials.h"
#undef DEFINE_SPECIAL

LineSpecialFunction lnspecFunctions[NUM_POSSIBLE_SPECIALS] =
{
	LN_NOP,

#define DEFINE_SPECIAL(name,num,argc) LN_##name,
#include "lnspecials.h"
#undef DEFINE_SPECIAL
};

#define DEFINE_SPECIAL(name,num,args) { #name, num, args},
const struct LineSpecialMeta
{
	const char* const		name;
	const unsigned int		num;
	const unsigned short	args;
} lnspecMeta[] = {
#include "lnspecials.h"
	{ NULL, 0, 0 }
};

LineSpecials Specials::LookupFunctionNum(const char* const function)
{
	const LineSpecialMeta *func = lnspecMeta;
	do
	{
		if(stricmp(func->name, function) == 0)
			return static_cast<LineSpecials> (func->num);
	}
	while((++func)->name != NULL);
	return NUM_POSSIBLE_SPECIALS;
}
LineSpecialFunction Specials::LookupFunction(LineSpecials function)
{
	if(function >= NUM_POSSIBLE_SPECIALS)
		return LN_NOP;

	return lnspecFunctions[function];
}

////////////////////////////////////////////////////////////////////////////////

FUNC(NOP)
{
	return 0;
}

class EVDoor : public Thinker
{
	DECLARE_CLASS(EVDoor, Thinker)

	public:
		EVDoor(MapSpot spot, unsigned int speed, int opentics, bool direction, unsigned int style) : Thinker(ThinkerList::WORLD),
			state(Closed), spot(spot), amount(0), opentics(opentics), direction(direction)
		{
			if(spot->tile->soundSequence == NAME_None)
				seqname = gameinfo.DoorSoundSequence;
			else
				seqname = spot->tile->soundSequence;
			sndseq = NULL;

			spot->slideStyle = style;
			if(spot->slideAmount[direction] == 0 && spot->slideAmount[direction+2] == 0)
				ChangeState(Opening);
			else
			{
				// This should occur if we have a negative duration door
				// allow manual closing.
				amount = spot->slideAmount[direction];
				ChangeState(Closing);
			}
			spot->thinker = this;
			this->speed = 64*speed;
		}

		void Destroy()
		{
			if(sndseq)
				delete sndseq;
			if(spot->thinker == this)
				spot->thinker = NULL;
			Super::Destroy();
		}

		bool IsClosing() const
		{
			return state == Closing || state == Closed;
		}

		void Tick()
		{
			if(sndseq)
				sndseq->Tick();

			switch(state)
			{
				default:
					break;
				case Opening:
					// Connect sound
					if(amount == 0)
					{
						const MapZone *zone1 = spot->GetAdjacent(MapTile::Side(direction))->zone;
						const MapZone *zone2 = spot->GetAdjacent(MapTile::Side(direction), true)->zone;
						map->LinkZones(zone1, zone2, true);

						if(map->CheckLink(zone1, players[ConsolePlayer].mo->GetZone(), true))
							sndseq = new SndSeqPlayer(SoundSeq(seqname, SEQ_OpenNormal), spot);
					}

					if(amount < 0xffff)
						amount += speed;
					if(amount >= 0xffff)
					{
						amount = 0xffff;
						if(opentics < 0) // Negative delay means stay open
							Destroy();
						else
							ChangeState(Opened);
					}
					spot->slideAmount[direction] = spot->slideAmount[direction+2] = amount;
					break;
				case Opened:
					if(wait == 0)
					{
						if(CheckJammed(false))
						{
							// Might as well reset the door timer, chances are the actor isn't going any where
							wait = opentics;
							break;
						}
						ChangeState(Closing);
					}
					else
						--wait;
					break;
				case Closing:
					if(amount > 0)
						amount -= speed;
					if(amount <= 0)
					{
						amount = 0;
						Destroy();

						// Disconnect Sound
						const MapZone *zone1 = spot->GetAdjacent(MapTile::Side(direction))->zone;
						const MapZone *zone2 = spot->GetAdjacent(MapTile::Side(direction), true)->zone;
						map->LinkZones(zone1, zone2, false);
					}
					spot->slideAmount[direction] = spot->slideAmount[direction+2] = amount;
					break;
			}
		}

		// forceclose is a temporary workaround for the issue where monsters can
		// jam elevator doors and breaking levels. TODO: Replace with ROTT's
		// crushing/gibbing behavior.
		bool Reactivate(AActor *activator, bool ismonster, bool forceclose=false)
		{
			switch(state)
			{
				case Opened:
					if(ismonster) // Monsters can reset the door
					{
						wait = opentics;
						return false;
					}
				default:
					if(ismonster || CheckJammed(forceclose))
						break;
					return ChangeState(Closing);
				case Closing:
					return ChangeState(Opening);
			}
			return false;
		}

		void Serialize(FArchive &arc)
		{
			BYTE state = this->state;
			arc << state;
			this->state = static_cast<State>(state);

			arc << spot
				<< speed
				<< amount
				<< wait
				<< direction;
			if(GameSave::SaveVersion > 1410810515)
				arc << sndseq << seqname << opentics;

			Super::Serialize(arc);
		}

	private:
		enum State { Opening, Opened, Closing, Closed };

		// Returns true if the actor isn't blocking the door.
		bool CheckClears(AActor *obj) const
		{
			if(direction == 0)
			{
				if(spot->GetY() == obj->tiley &&
					abs((((fixed)spot->GetX()<<FRACBITS)+(FRACUNIT/2))-obj->x) < FRACUNIT/2 + obj->radius)
				{
					return false;
				}
			}
			else
			{
				if(spot->GetX() == obj->tilex &&
					abs((((fixed)spot->GetY()<<FRACBITS)+(FRACUNIT/2))-obj->y) < FRACUNIT/2 + obj->radius)
				{
					return false;
				}
			}
			return true;
		}

		bool CheckJammed(bool onlysolid) const
		{
			for(AActor::Iterator iter = AActor::GetIterator();iter.Next();)
			{
				if(!CheckClears(iter))
				{
					if(!onlysolid || (iter->flags & FL_SOLID))
						return true;
				}
			}
			return false;
		}

		bool ChangeState(State st)
		{
			if(st == state)
				return false;
			state = st;

			switch(state)
			{
				default:
					break;
				case Opened:
					wait = opentics;
					if(sndseq)
					{
						delete sndseq;
						sndseq = NULL;
					}
					break;
				case Closing:
					if(map->CheckLink(spot->GetAdjacent(MapTile::Side(direction))->zone, players[ConsolePlayer].mo->GetZone(), true))
						sndseq = new SndSeqPlayer(SoundSeq(seqname, SEQ_CloseNormal), spot);
					break;
			}
			return true;
		}

		State state;
		MapSpot spot;

		SndSeqPlayer *sndseq;
		FName seqname;

		unsigned int speed;
		int32_t amount;
		int32_t opentics;
		unsigned int wait;
		bool direction;
};
IMPLEMENT_INTERNAL_CLASS(EVDoor)

FUNC(Door_Open)
{
	static const unsigned int DOOR_TYPE_DIRECTION = 0x1;

	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
	}

	if(activator->player || (activator->flags & FL_REQUIREKEYS))
	{
		if(args[3] != 0)
		{
			if(!P_CheckKeys(activator, args[3], false))
				return 0;
		}
	}

	if(args[0] == 0)
	{
		if(spot->thinker)
		{
			if(spot->thinker->IsThinkerType<EVDoor>())
			{
				return barrier_cast<EVDoor *>(spot->thinker)->Reactivate(activator, activator && (activator->flags & FL_ISMONSTER));
			}
			return 0;
		}

		new EVDoor(spot, args[1], args[2], args[4]&DOOR_TYPE_DIRECTION, args[4]>>1);
	}
	else
	{
		bool activated = false;
		MapSpot door = NULL;
		while((door = map->GetSpotByTag(args[0], door)))
		{
			if(door->thinker)
			{
				if(door->thinker->IsThinkerType<EVDoor>())
				{
					return barrier_cast<EVDoor *>(door->thinker)->Reactivate(activator, activator && (activator->flags & FL_ISMONSTER));
				}
				continue;
			}

			activated = true;
			new EVDoor(door, args[1], args[2], args[4]&DOOR_TYPE_DIRECTION, args[4]>>1);
		}
		return activated;
	}
	return 1;
}

class EVElevator : public Thinker
{
	DECLARE_CLASS(EVElevator, Thinker)
public:
	EVElevator(AActor *activator, MapSpot spot, MapSpot door, MapSpot next, unsigned int elevTag, unsigned int callSpeed) :
		Thinker(ThinkerList::WORLD), activator(activator), sndseq(NULL),
		spot(spot), door(door), next(next), nextDoor(NULL), elevTag(elevTag),
		callSpeed(callSpeed)
	{
		state = Moving;

		// Try to close the door if it is open.
		if(door->thinker)
		{
			if(door->thinker->IsThinkerType<EVDoor>())
			{
				state = ClosingDoor;

				EVDoor *doorThinker = barrier_cast<EVDoor *>(door->thinker);
				if(!doorThinker->IsClosing())
				{
					if(!doorThinker->Reactivate(activator, activator && (activator->flags & FL_ISMONSTER), true))
					{
						Destroy();
						return;
					}
				}
			}
			else
			{
				Destroy();
				return;
			}
		}

		if(state == Moving)
			StartSoundSequence();

		// Remove elevator position since we're off!
		map->elevatorPosition[elevTag] = NULL;
	}

	void Destroy()
	{
		delete sndseq;
		Super::Destroy();
	}

	void Tick()
	{
		if(sndseq)
			sndseq->Tick();

		switch(state)
		{
			default:
				Destroy();
				break;
			case ClosingDoor:
				if(door->thinker)
					break;

				state = Moving;
				StartSoundSequence();
				// Fall through
			case Moving:
				if(--callSpeed == 0)
				{
					state = Shaking;
					callSpeed = 20; // ROTT uses 10 tics @ 35Hz
					if(sndseq)
					{
						sndseq->SetSource(next);
						sndseq->Stop();
					}

					map->elevatorPosition[elevTag] = next;

					// Find elevator trigger on destination
					for(unsigned int i = 0;i < next->triggers.Size();++i)
					{
						if(next->triggers[i].action == Elevator_SwitchFloor)
						{
							nextDoor = map->GetSpotByTag(next->triggers[i].arg[1], NULL);
						}
					}
					if(!nextDoor)
					{
						Printf("Failed to perform elevator teleport.\n");
						state = Finished;
						break;
					}

					if(activator)
					{
						/* Teleport the player, trying to rotate according to the
						* orientation of the detination. ROTT only changed the
						* location of the player, so this could be an issue. In
						* that case we just need to remove the angle change and
						* the fracx/y change since ROTT use elevator things and
						* not the switch as the reference point.
						*/
						const int ddeltax = clamp<int>(nextDoor->GetX() - next->GetX(), -1, 1);
						const int ddeltay = clamp<int>(nextDoor->GetY() - next->GetY(), -1, 1);
						const int sdeltax = clamp<int>(door->GetX() - spot->GetX(), -1, 1);
						const int sdeltay = clamp<int>(door->GetY() - spot->GetY(), -1, 1);
						int relx = spot->GetX()-activator->tilex;
						int rely = spot->GetY()-activator->tiley;
						fixed fracx = activator->fracx;
						fixed fracy = activator->fracy;
						angle_t angle = 0;
						if((ddeltax && ddeltax == -sdeltax) || (ddeltay && ddeltay == -sdeltay))
						{
							relx *= -1;
							rely *= -1;
							angle = ANGLE_180;
							fracx = (FRACUNIT-fracx);
							fracy = (FRACUNIT-fracy);
						}
						else if(ddeltax == 0 && sdeltax)
						{
							int tmp = rely;
							if(ddeltay < 0)
							{
								rely = -relx;
								relx = tmp;
								angle = ANGLE_90;

								tmp = (FRACUNIT-fracx);
								fracx = fracy;
								fracy = tmp;
							}
							else
							{
								rely = relx;
								relx = -tmp;
								angle = ANGLE_270;

								tmp = fracx;
								fracx = (FRACUNIT-fracy);
								fracy = tmp;

							}
						}
						else if(ddeltay == 0 && sdeltay)
						{
							int tmp = relx;
							if(ddeltax < 0)
							{
								relx = rely;
								rely = -tmp;
								angle = ANGLE_90;

								tmp = fracy;
								fracy = (FRACUNIT-fracx);
								fracx = tmp;
							}
							else
							{
								relx = -rely;
								rely = tmp;
								angle = ANGLE_270;

								tmp = (FRACUNIT-fracy);
								fracy = fracx;
								fracx = tmp;
							}
						}
						activator->x = ((next->GetX() - relx)<<16)|fracx;
						activator->y = ((next->GetY() - rely)<<16)|fracy;
						activator->angle += angle;
						activator->EnterZone(map->GetSpot(activator->tilex, activator->tiley, 0)->zone);
					}
				}
				break;
			case Shaking:
				// TODO: Shake the screen
				if(--callSpeed == 0)
				{
					state = Finished;

					// Find the actual door trigger and try to open it.
					for(unsigned int i = nextDoor->triggers.Size();i-- > 0;)
					{
						MapTrigger &trig = nextDoor->triggers[i];
						if(trig.action == Door_Elevator)
						{
							map->ActivateTrigger(trig, MapTrigger::East, activator);
							break;
						}
					}
				}
				break;
		}
	}

	void Serialize(FArchive &arc)
	{
		if(GameSave::SaveVersion > 1438232816)
		{
			BYTE state = this->state;
			arc << state;
			this->state = static_cast<State>(state);

			arc << activator
				<< sndseq
				<< spot
				<< door
				<< next
				<< nextDoor
				<< elevTag
				<< callSpeed;
		}

		Super::Serialize(arc);
	}

protected:
	void StartSoundSequence()
	{
		FName seqname;
		if(spot->tile->soundSequence == NAME_None)
			seqname = NAME_Platform;
		else
			seqname = spot->tile->soundSequence;

		sndseq = new SndSeqPlayer(SoundSeq(seqname, SEQ_OpenNormal), spot);
	}

private:
	enum State { ClosingDoor, Moving, Shaking, Finished };

	TObjPtr<AActor> activator;
	SndSeqPlayer *sndseq;
	MapSpot spot;
	MapSpot door;
	MapSpot next;
	MapSpot nextDoor;
	unsigned int elevTag;
	unsigned int callSpeed;
	State state;
};
IMPLEMENT_INTERNAL_POINTY_CLASS(EVElevator)
	DECLARE_POINTER(activator)
END_POINTERS

// Takes same arugments as Door_Open, but the tag points to the elevator switch.
// Will attempt to call the elevator if not in the correct position.
FUNC(Door_Elevator)
{
	static const unsigned int DOOR_TYPE_DIRECTION = 0x1;

	// If no activator it's probably the elevator reaching its destination.
	if(activator)
	{
		if(activator->player)
		{
			if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
				return 0;
		}

		if(activator->player || (activator->flags & FL_REQUIREKEYS))
		{
			if(args[3] != 0)
			{
				if(!P_CheckKeys(activator, args[3], false))
					return 0;
			}
		}
	}

	if(spot->thinker)
	{
		if(spot->thinker->IsThinkerType<EVDoor>())
		{
			return barrier_cast<EVDoor *>(spot->thinker)->Reactivate(activator, activator && (activator->flags & FL_ISMONSTER));
		}
		return 0;
	}

	// Now locate the switch and elevator trigger so we can find out if we need
	// to call to our location.
	MapSpot swtch = map->GetSpotByTag(args[0], NULL);
	if(!swtch)
	{
		Printf("Door_Elevator: Could not find switch.\n");
		return 0;
	}
	MapTrigger *trig = NULL; 
	for(unsigned int i = 0;i < swtch->triggers.Size();++i)
	{
		if(swtch->triggers[i].action == Elevator_SwitchFloor)
		{
			trig = &swtch->triggers[i];
			break;
		}
	}
	if(!trig)
	{
		Printf("Door_Elevator: Could not find elevator trigger.\n");
		return 0;
	}

	// Call elevator
	if(map->elevatorPosition[trig->arg[0]] != swtch)
	{
		// Check if elevator is active
		if(map->elevatorPosition[trig->arg[0]] == NULL)
			return 0;

		new EVElevator(activator, swtch, spot, swtch, trig->arg[0], trig->arg[2]);
		return 1;
	}

	new EVDoor(spot, args[1], args[2], args[4]&DOOR_TYPE_DIRECTION, args[4]>>1);
	return 1;
}

FUNC(Elevator_SwitchFloor)
{ // elevTag, doorTag, callSpeed, nextTag
	MapSpot door = map->GetSpotByTag(args[1], NULL);
	MapSpot next = map->GetSpotByTag(args[3], NULL);
	if(spot->thinker || !door || !next)
		return 0;
	
	EVElevator *elevator = new EVElevator(activator, spot, door, next, args[0], args[2]);
	if(!(elevator->ObjectFlags & OF_EuthanizeMe))
	{
		spot->thinker = elevator;
		return 1;
	}
	return 0;
}

class EVPushwall : public Thinker
{
	DECLARE_CLASS(EVPushwall, Thinker)

	public:
		EVPushwall(MapSpot spot, unsigned int speed, MapTrigger::Side direction, unsigned int distance, bool nostop) :
			Thinker(ThinkerList::WORLD), spot(spot), moveTo(NULL), direction(direction), position(0),
			speed(speed), distance(distance), nostop(nostop)
		{
			if(spot->tile->soundSequence == NAME_None)
				seqname = gameinfo.PushwallSoundSequence;
			else
				seqname = spot->tile->soundSequence;
			sndseq = NULL;

			if(distance == 0) // ROTT style pushwall, move until blocked
				distance = 0xFFFF;

			spot->thinker = this;
			spot->pushDirection = MapTile::Side(direction);
		}

		static bool CheckSpotFree(MapSpot spot)
		{
			if(spot->tile)
			{
				// Hit a wall so we're done.
				return false;
			}

			// Check for any blocking actors
			AActor::Iterator iter = AActor::GetIterator();
			int movex = spot->GetX();
			int movey = spot->GetY();
			while(iter.Next())
			{
				AActor *actor = iter;
				if((actor->flags&FL_ISMONSTER) || actor->player)
				{
					if(actor->tilex+dirdeltax[actor->dir] == movex &&
						actor->tiley+dirdeltay[actor->dir] == movey)
					{
						// Blocked!
						return false;
					}
				}
			}
			return true;
		}

		void Destroy()
		{
			if(sndseq)
				delete sndseq;
			if(spot->thinker == this)
				spot->thinker = NULL;
			Super::Destroy();
		}

		void Tick()
		{
			if(position == 0)
				sndseq = new SndSeqPlayer(SoundSeq(seqname, SEQ_OpenNormal), spot);

			if(sndseq)
				sndseq->Tick();

			// Setup the next tile to get ready to accept this tile.
			if(moveTo == NULL)
			{
				moveTo = spot->GetAdjacent(MapTile::Side(direction));

				if(moveTo == NULL)
				{
					Destroy();
					return;
					// Maybe in the future this can be a flag or something
					#if 0
					FString error;
					error.Format("\"I'm free!\" -Pushwall @ (%d, %d)", spot->GetX(), spot->GetY());
					throw CRecoverableError(error);
					#endif
				}

				if(!nostop && !CheckSpotFree(moveTo))
				{
					Destroy();
					return;
				}

				moveTo->SetTile(spot->tile);
				moveTo->pushReceptor = spot;
				moveTo->pushDirection = spot->pushDirection;

				// Try to get a sound zone.
				if(spot->zone == NULL)
					spot->zone = moveTo->zone;
			}

			// Move the tile a bit.
			if((position += speed) > 1024)
			{
				position -= 1024;
				spot->pushAmount = 0;
				spot->SetTile(NULL);
				spot->thinker = NULL;
				moveTo->pushReceptor = NULL;
				moveTo->thinker = this;

				// Transfer amflags
				moveTo->amFlags |= spot->amFlags;

				spot = moveTo;
				moveTo = NULL;
			}
			else
				spot->pushAmount = position/16;

			if(!moveTo)
			{
				if(--distance == 0)
					Destroy();
			}
		}

		void Serialize(FArchive &arc)
		{
			arc << spot
				<< moveTo
				<< direction
				<< position
				<< speed
				<< distance;

			if(GameSave::SaveVersion > 1410810515)
				arc << sndseq << seqname;

			Super::Serialize(arc);
		}

	private:

		MapSpot spot, moveTo;

		SndSeqPlayer *sndseq;
		FName seqname;

		unsigned short	direction;
		unsigned int	position;
		unsigned int	speed;
		unsigned int	distance;
		bool nostop;
};
IMPLEMENT_INTERNAL_CLASS(EVPushwall)

static int DoPushwall(MapSpot spot, MapTrigger::Side direction, const int *args, bool nostop)
{
	static const unsigned int PUSHWALL_DIR_DIAGONAL = 0x1;
	static const unsigned int PUSHWALL_DIR_ABSOLUTE = 0x8;

	if(args[2] & PUSHWALL_DIR_DIAGONAL)
		throw CRecoverableError("Diagonal pushwalls not yet supported!");

	bool absolute = !!(args[2]&PUSHWALL_DIR_ABSOLUTE);
	MapTrigger::Side dir = absolute ? MapTrigger::Side((args[2]>>1)&0x3) : MapTrigger::Side((direction + 1 + (args[2]>>1))&0x3);

	if(args[0] == 0)
	{
		if(spot->thinker || !spot->tile || (spot->GetAdjacent(MapTile::Side(dir))->tile && !nostop))
		{
			return 0;
		}

		new EVPushwall(spot, args[1], dir, args[3], nostop);
	}
	else
	{
		bool activated = false;
		MapSpot pwall = NULL;
		while((pwall = map->GetSpotByTag(args[0], pwall)))
		{
			if(pwall->thinker || !pwall->tile || (pwall->GetAdjacent(MapTile::Side(dir))->tile && !nostop))
			{
				continue;
			}

			activated = true;
			new EVPushwall(pwall, args[1], dir, args[3], nostop);
		}
		return activated;
	}
	return 1;
}

FUNC(Pushwall_Move)
{
	return DoPushwall(spot, direction, args, false);
}

FUNC(Pushwall_MoveNoStop)
{
	// TODO: This really needs the pushwalls to be detached from our map grid.
	// Secondly we might want to look into crushing features? I think ROTT does
	// that.
	return DoPushwall(spot, direction, args, true);
}

FUNC(Exit_Normal)
{
	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
		control[activator->player->GetPlayerNum()].buttonheld[bt_use] = true;
	}

	playstate = ex_completed;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Secret)
{
	
	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
		control[activator->player->GetPlayerNum()].buttonheld[bt_use] = true;
	}

	playstate = ex_secretlevel;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Exit_Victory)
{
	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
		control[activator->player->GetPlayerNum()].buttonheld[bt_use] = true;
	}

	playstate = ex_victorious;
	SD_WaitSoundDone();
	return 1;
}

FUNC(Teleport_NewMap)
{
	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
		control[activator->player->GetPlayerNum()].buttonheld[bt_use] = true;
	}

	playstate = ex_newmap;
	NewMap.newmap = args[0];
	NewMap.flags = args[2];
	return 1;
}

class EVVictorySpin : public Thinker
{
	DECLARE_CLASS(EVVictorySpin, Thinker)
	HAS_OBJECT_POINTERS
	public:
		// Note that we trigger slightly a half unit before wolf3d did
		EVVictorySpin(AActor *activator, MapTrigger::Side direction) : Thinker(ThinkerList::VICTORY),
			doturn(true), dist(6*FRACUNIT + FRACUNIT/2), activator(activator)
		{
			gamestate.victoryflag = true;
			activator->player->SetPSprite(NULL, player_t::ps_weapon);

			runner = AActor::Spawn(ClassDef::FindClass("BJRun"), activator->x, activator->y, 0, SPAWN_AllowReplacement);
			runner->flags |= FL_PATHING;
			runner->angle = ((direction+2)%4)*ANGLE_90;
			runner->dir = static_cast<dirtype>(runner->angle/ANGLE_45);
			runner->SetPriority(ThinkerList::VICTORY);

			activator->SetPriority(ThinkerList::VICTORY);
		}

		void Serialize(FArchive &arc)
		{
			arc << doturn
				<< dist
				<< activator
				<< runner;

			Super::Serialize(arc);
		}

		void Tick()
		{
			if(doturn)
			{
				angle_t oldangle = activator->angle;
				A_Face(activator, runner, 3*ANGLE_1);
				if(activator->angle == oldangle)
					doturn = false;
			}

			// Keep the runner in line with the player (A_Chase will try to tile align)
			switch(runner->dir)
			{
				case north:
				case south:
					runner->x = activator->x;
					break;
				case east:
				case west:
					runner->y = activator->y;
					break;
				default: break;
			}

			if(dist > 0)
			{
				static const unsigned int speed = 4096;

				if(dist <= speed)
				{
					activator->x += FixedMul(dist, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					activator->y -= FixedMul(dist, finesine[runner->angle>>ANGLETOFINESHIFT]);
					dist = 0;
				}
				else
				{
					activator->x += FixedMul(speed, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					activator->y -= FixedMul(speed, finesine[runner->angle>>ANGLETOFINESHIFT]);
					dist -= speed;
				}
			}
			else
			{
				// This is the gross part, we must do a "collision" check.
				// If it passes we're done here, but we must first make the
				// runner a projectile and then make sure it doesn't try to
				// explode on us.
				fixed radius = runner->radius + activator->radius + runner->speed;
				if(abs(activator->x - runner->x) <= radius &&
					abs(activator->y - runner->y) <= radius)
				{
					runner->Die();
					runner->velx = FixedMul(runner->runspeed, finecosine[runner->angle>>ANGLETOFINESHIFT]);
					runner->vely = -FixedMul(runner->runspeed, finesine[runner->angle>>ANGLETOFINESHIFT]);
					runner->flags |= FL_MISSILE;
					runner->radius = 1;
					Destroy();
				}
			}
		}

		bool			doturn;
		unsigned int	dist;
		TObjPtr<AActor>	activator;
		TObjPtr<AActor>	runner;
};
IMPLEMENT_INTERNAL_POINTY_CLASS(EVVictorySpin)
	DECLARE_POINTER(activator)
	DECLARE_POINTER(runner)
END_POINTERS
FUNC(Exit_VictorySpin)
{
	if(activator->player)
	{
		if(control[activator->player->GetPlayerNum()].buttonheld[bt_use])
			return 0;
		control[activator->player->GetPlayerNum()].buttonheld[bt_use] = true;
	}

	new EVVictorySpin(activator, direction);
	return 1;
}

// Executes all triggers on a particular map spot
// Useful for making events happen in binary format maps
FUNC(Trigger_Execute)
{
	if(!map->IsValidTileCoordinate(args[0], args[1], args[2]))
		return 0;

	MapSpot target = map->GetSpot(args[0], args[1], args[2]);

	bool activated = false;
	for(unsigned int i = target->triggers.Size();i-- > 0;)
	{
		if(map->ActivateTrigger(target->triggers[i], MapTrigger::East, activator))
			activated = true;
	}

	return activated;
}

FUNC(StartConversation)
{
	if(args[0] != 0)
		I_Error("TIDs are not yet supported.");

	AActor *talker = activator;

	if(args[1] == 1) // Face talker
	{
		A_Face(activator, talker);
	}

	Dialog::StartConversation(talker);

	return 1;
}

FUNC(Teleport_Relative)
{
	enum
	{
		TELEPORT_NoStop = 1,
		TELEPORT_NoFog = 2,
		TELEPORT_Center = 4, // Center onto destination tile
		TELEPORT_AbsoluteAngle = 8, // Set absolute angle instead of relative
		TELEPORT_ActivationAngle = 0x10, // Face the activation point (typically used with AbsoluteAngle)
	};

	if(!spot)
	{
		Printf("Error: Attempted to relative teleport without a reference point.\n");
		return 0;
	}

	if(activator->player && control[activator->player->GetPlayerNum()].buttonheld[bt_use])
		return 0;

	// Collect destination points so we can randomly decide
	TArray<MapSpot> dests;
	MapSpot dest = NULL;
	while((dest = map->GetSpotByTag(args[0], dest)))
		dests.Push(dest);
	if(dests.Size() == 0)
		return 0;
	dest = dests[pr_teleport(dests.Size())];

	if(!(args[2] & TELEPORT_NoStop))
		activator->sighttime = 35; // Halt for half second

	fixed x = activator->x + ((dest->GetX() - spot->GetX())<<FRACBITS);
	fixed y = activator->y + ((dest->GetY() - spot->GetY())<<FRACBITS);
	if((args[2] & TELEPORT_Center))
	{
		x = (activator->x&0xFFFF0000)|0x8000;
		y = (activator->y&0xFFFF0000)|0x8000;
	}

	angle_t angle = (args[1]<<24) +
		(!(args[2] & TELEPORT_AbsoluteAngle) ? activator->angle : 0) +
		((args[2] & TELEPORT_ActivationAngle) ? (direction<<30)+ANGLE_180 : 0);

	// Check that teleport remains in bounds
	if(!map->IsValidTileCoordinate(x>>FRACBITS, y>>FRACBITS, 0))
	{
		Printf("Error: %s at (%d, %d) attempted to teleport out of bounds. Possible double teleport?\n", activator->GetClass()->GetName().GetChars(), activator->tilex, activator->tiley);
		return false;
	}

	activator->Teleport(x, y, angle, !!(args[2] & TELEPORT_NoFog));
	return 1;
}

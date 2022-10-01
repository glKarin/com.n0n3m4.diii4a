/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __NETWORKSYSTEM_H__
#define __NETWORKSYSTEM_H__

#ifdef _RAVEN
    //RAVEN END
    typedef enum {
        SC_NONE = -1,
        SC_FAVORITE,
        SC_LOCKED,
        SC_DEDICATED,
        SC_PB,
        SC_NAME,
        SC_PING,
        SC_REPEATER,
        SC_PLAYERS,
        SC_GAMETYPE,
        SC_MAP,
        SC_ALL,
        NUM_SC
    } sortColumn_t;
   
    typedef struct {
        sortColumn_t            column;
        idList<int>::cmp_t* compareFn;
        idList<int>::filter_t* filterFn;
        const char* description;
    } sortInfo_t;

typedef struct { 
      netadr_t    adr; 
      idDict      serverInfo; 
      int         ping; 
   
      int         clients; 
   
      int         OSMask; 
   
      //RAVEN BEGIN 
      // shouchard:  added favorite flag 
      bool        favorite;   // true if this has been marked by a user as a favorite 
      bool        dedicated; 
      // shouchard:  added performance filtered flag 
      bool        performanceFiltered;    // true if the client machine is too wimpy to have good perfo    rmance 
  //RAVEN END 
  } scannedServer_t;

#endif

/*
===============================================================================

  Network System.

===============================================================================
*/

class idNetworkSystem
{
	public:
		virtual					~idNetworkSystem(void) {}

		virtual void			ServerSendReliableMessage(int clientNum, const idBitMsg &msg);
		virtual void			ServerSendReliableMessageExcluding(int clientNum, const idBitMsg &msg);
		virtual int				ServerGetClientPing(int clientNum);
		virtual int				ServerGetClientPrediction(int clientNum);
		virtual int				ServerGetClientTimeSinceLastPacket(int clientNum);
		virtual int				ServerGetClientTimeSinceLastInput(int clientNum);
		virtual int				ServerGetClientOutgoingRate(int clientNum);
		virtual int				ServerGetClientIncomingRate(int clientNum);
		virtual float			ServerGetClientIncomingPacketLoss(int clientNum);

		virtual void			ClientSendReliableMessage(const idBitMsg &msg);
		virtual int				ClientGetPrediction(void);
		virtual int				ClientGetTimeSinceLastPacket(void);
		virtual int				ClientGetOutgoingRate(void);
		virtual int				ClientGetIncomingRate(void);
		virtual float			ClientGetIncomingPacketLoss(void);

#ifdef _RAVEN
        // for MP games 
        virtual void            SetLoadingText(const char* loadingText) { }
        virtual void            AddLoadingIcon(const char* icon) { }

		// server browser
		virtual int             GetNumScannedServers(void) { return 0; }
        virtual const scannedServer_t* GetScannedServerInfo(int serverNum) { return 0; }
        virtual void            UseSortFunction(const sortInfo_t& sortInfo, bool use = true) { }
        virtual void            AddSortFunction(const sortInfo_t& sortInfo) { }

        virtual bool            RemoveSortFunction(const sortInfo_t& sortInfo) { return 0; }

	virtual const char* GetClientGUID(int clientNum) { return 0; }

// ddynerman: added some utility functions
	// uses a static buffer, copy it before calling in game again
	virtual const char* GetServerAddress(void) { return 0; }
	virtual int				ServerGetClientNum(int clientId) { return 0; }
	virtual	int				ServerGetServerTime(void) { return 0; }

	virtual	void			AddFriend(int clientNum) { }
	virtual void			RemoveFriend(int clientNum) { }
#endif
#ifdef _RAVEN // _QUAKE4
	virtual int				ServerSetBotUserCommand(int clientNum, int frameNum, const usercmd_t& cmd);
	virtual int				AllocateClientSlotForBot(const char *botName, int maxPlayersOnServer);
#endif

};

extern idNetworkSystem 	*networkSystem;

#endif /* !__NETWORKSYSTEM_H__ */

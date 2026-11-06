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

#ifdef _SPLASHDAMAGE
struct repeaterUserOrigin_t {
    idVec3	origin;
    int		followClient;
};

#ifdef _XENON

const int MAX_ASYNC_CLIENTS			= 16;

#else

const int MAX_ASYNC_CLIENTS			= 32;

#endif

#ifdef SD_SUPPORT_REPEATER

const int REPEATER_CLIENT_INDEX		= -1;

#endif // SD_SUPPORT_REPEATER
#endif

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

#ifdef _SPLASHDAMAGE
struct clientNetworkAddress_t {
    unsigned char	ip[ 4 ];
    unsigned short	port;

    bool operator==( const clientNetworkAddress_t& rhs ) const
    {
        return	ip[ 0 ] == rhs.ip[ 0 ] &&
                ip[ 1 ] == rhs.ip[ 1 ] &&
                ip[ 2 ] == rhs.ip[ 2 ] &&
                ip[ 3 ] == rhs.ip[ 3 ] &&
                port == rhs.port;
    }
};

enum voiceMode_t {
    VO_GLOBAL,
    VO_TEAM,
    VO_FIRETEAM,
    VO_NUM_MODES,
};

struct sdNetClientId;
#endif
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
#ifdef _SPLASHDAMAGE
    	virtual void			ServerGetClientNetworkInfo( int clientNum, clientNetworkAddress_t& info );
    	virtual void			ServerGetClientNetId( int clientNum, sdNetClientId& netClientId );
    	virtual void			ServerKickClient( int clientNum, const char* reason, bool localizedReason );
    	
//mal: allow the network to access some engine side bot functions....
    	virtual int				AllocateClientSlotForBot( int maxPlayersOnServer );
    	virtual int				ServerSetBotUserCommand( int clientNum, int frameNum, const usercmd_t& cmd );
    	virtual int				ServerSetBotUserName( int clientNum, const char* playerName );
    	
    	virtual void			WriteClientUserCmds( int clientNum, idBitMsg& msg );
    	virtual void			ReadClientUserCmds( int clientNum, const idBitMsg& msg );
	    virtual bool			IsDedicated( void );
	    virtual bool			IsLANServer( void );
    	virtual bool			IsActive( void );
    	
    	virtual netadr_t		ClientGetServerAddress( void ) const;
    	virtual void			EnableVoip( voiceMode_t mode );
    	virtual void			DisableVoip( void );

    	virtual int				GetLastVoiceSentTime( void );
    	virtual int				GetLastVoiceReceivedTime( int clientIndex );
    	
    	virtual int				ClientGetFrameTime( void );


    	virtual int				GetDemoState( int& time, int& position, int& length, int& startPosition, int& endPosition, int &cutStartMarker, int &cutEndMarker );
    	virtual const char*		GetDemoName( void );
    	
    	virtual bool			CanPlayDemo( const char* fileName );
    
    	virtual const idDict&	GetUserInfo( int clientNum );

    	virtual bool			IsRankedServer( void );

    	virtual void			StartSoundTest( int duration );
    	virtual bool			IsSoundTestActive( void );
    	virtual bool			IsSoundTestPlaybackActive( void );
    	virtual float			GetSoundTestProgress( void );
    	
    	virtual voiceMode_t		GetVoiceMode( void );

		virtual void			RegisterServerInterest( const netadr_t& address );
#ifdef SD_SUPPORT_REPEATER
		virtual void			RepeaterSendReliableMessage( int clientNum, const idBitMsg& msg, bool ignoreRelays );

		virtual void			RepeaterSetInfo( const idDict& info );
		virtual const idDict&	RepeaterGetClientInfo( int clientNum );
		virtual void			SetClientRepeaterUserOrigin( const repeaterUserOrigin_t& origin );
#endif // SD_SUPPORT_REPEATER

#if !defined( SD_PUBLIC_TOOLS )
    	virtual bool			HTTPEnable( bool enable );
#endif // !SD_PUBLIC_TOOLS
#endif

#ifdef _RAVEN
        // for MP games 
        virtual void            SetLoadingText(const char* loadingText);
        virtual void            AddLoadingIcon(const char* icon);

		// server browser
		virtual int             GetNumScannedServers(void);
        virtual const scannedServer_t* GetScannedServerInfo(int serverNum);
        virtual void            UseSortFunction(const sortInfo_t& sortInfo, bool use = true);
        virtual void            AddSortFunction(const sortInfo_t& sortInfo);

        virtual bool            RemoveSortFunction(const sortInfo_t& sortInfo);

		virtual const char* GetClientGUID(int clientNum);
	
		// ddynerman: added some utility functions
		// uses a static buffer, copy it before calling in game again
		virtual const char* GetServerAddress(void);
		virtual int				ServerGetClientNum(int clientId);
		virtual	int				ServerGetServerTime(void);
	
		virtual	void			AddFriend(int clientNum);
		virtual void			RemoveFriend(int clientNum);
#endif

};

extern idNetworkSystem 	*networkSystem;

#endif /* !__NETWORKSYSTEM_H__ */

/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "Engine/StdH.h"

#include <Engine/Engine.h>
#include <Engine/CurrentVersion.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/CTString.h>
#include <Engine/Network/Server.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/SessionState.h>
#include <GameMP/SessionProperties.h>
#include <Engine/GameAgent/GameAgent.h>

#if defined(PLATFORM_WIN32)
#pragma comment(lib, "wsock32.lib")
WSADATA* _wsaData = NULL;
typedef int socklen_t;
#else
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#define closesocket close
typedef int SOCKET;
typedef struct hostent HOSTENT, *PHOSTENT;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr    SOCKADDR;
#define WSAGetLastError() (INDEX) errno
#define WSACleanup()
#endif

#define MSPORT      28900
#define BUFFSZ      8192
#define BUFFSZSTR   4096

#define PCK         "\\gamename\\%s" \
                    "\\enctype\\%d" \
                    "\\validate\\%s" \
                    "\\final\\" \
                    "\\queryid\\1.1" \
                    "\\list\\cmp" \
                    "\\gamename\\%s" \
                    "\\gamever\\1.05" \
                    "%s%s" \
                    "\\final\\"

#define PCKQUERY    "\\gamename\\%s" \
                    "\\gamever\\%s" \
                    "\\location\\%s" \
                    "\\hostname\\%s" \
                    "\\hostport\\%hu" \
                    "\\mapname\\%s" \
                    "\\gametype\\%s" \
                    "\\activemod\\%s" \
                    "\\numplayers\\%d" \
                    "\\maxplayers\\%d" \
                    "\\gamemode\\openplaying" \
                    "\\difficulty\\%s" \
                    "\\friendlyfire\\%d" \
                    "\\weaponsstay\\%d" \
                    "\\ammosstay\\%d" \
                    "\\healthandarmorstays\\%d" \
                    "\\allowhealth\\%d" \
                    "\\allowarmor\\%d" \
                    "\\infinitearmor\\%d" \
                    "\\respawninplace\\%d" \
                    "\\password\\0" \
                    "\\vipplayers\\0"

#define PCKRULES    "\\difficulty\\%s" \
                    "\\friendlyfire\\%d" \
                    "\\weaponsstay\\%d" \
                    "\\ammosstay\\%d" \
                    "\\healthandarmorstays\\%d" \
                    "\\allowhealth\\%d" \
                    "\\allowarmor\\%d" \
                    "\\infinitearmor\\%d" \
                    "\\respawninplace\\%d" \
                    "\\password\\0" \
                    "\\vipplayers\\0"

#define PCKINFO     "\\hostname\\%s" \
                    "\\hostport\\%hu" \
                    "\\mapname\\%s" \
                    "\\gametype\\%s" \
                    "\\activemod\\%s" \
                    "\\numplayers\\%d" \
                    "\\maxplayers\\%d" \
                    "\\gamemode\\openplaying" \
                    "\\final\\" \
                    "\\queryid\\334.1"

#define PCKBASIC    "\\gamename\\%s" \
                    "\\gamever\\%s" \
                    "\\location\\%s" \
                    "\\final\\" \
                    "\\queryid\\331.1"


#define CHK_BUFFSTRLEN if((iLen < 0) || (iLen > BUFFSZSTR)) { \
                        CPrintF("\n" \
                            "Error: the used buffer is smaller than how much needed (%d < %d)\n" \
                            "\n", iLen, BUFFSZSTR); \
                            if(cMsstring) free (cMsstring); \
                            closesocket(_sock); \
                            WSACleanup(); \
                        }

#define CLEANMSSRUFF1       closesocket(_sock); \
                            WSACleanup();

#define CLEANMSSRUFF2       if(cResponse) free (cResponse); \
                            closesocket(_sock); \
                            WSACleanup();

#define SERIOUSSAMKEY       "AKbna4\0"
#ifdef FIRST_ENCOUNTER  // First Encounter
#define SERIOUSSAMSTR       "serioussam"
#else
#define SERIOUSSAMSTR       "serioussamse"
#endif

#include <Engine/GameAgent/MSLegacy.h>

SOCKET _socket = INVALID_SOCKET;

sockaddr_in* _sin = NULL;
sockaddr_in* _sinLocal = NULL;
sockaddr_in _sinFrom;

CHAR* _szBuffer = NULL;
CHAR* _szIPPortBuffer = NULL;
LONG   _iIPPortBufferLen = 0;
CHAR* _szIPPortBufferLocal = NULL;
LONG   _iIPPortBufferLocalLen = 0;

BOOL _bServer = FALSE;
#ifdef STATICALLY_LINKED
static 
#endif
BOOL _bInitialized = FALSE;
BOOL _bActivated = FALSE;
BOOL _bActivatedLocal = FALSE;

TIME _tmLastHeartbeat = 0;
char _datetime[128] = {0};

CDynamicStackArray<CServerRequest> ga_asrRequests;

/* List of Master servers...
sam.ostap.eu
42amsterdam.net
master.42amsterdam.net
master.333networks.com

master.errorist.eu
master.gonespy.com
master.newbiesplayground.net
rhea.333networks.com
rhea.333networks.com

*/
//extern CTString ga_strServer = "master1.croteam.org";
__extern CTString ga_strServer = "sam.ostap.eu";
//extern CTString ga_strMSLegacy = "master1.croteam.org";
__extern CTString ga_strMSLegacy = "sam.ostap.eu";

__extern BOOL ga_bMSLegacy = TRUE;
//extern BOOL ga_bMSLegacy = FALSE;

#ifdef WIN32
void DateTime(char *datetime){

    char tmpbuf[9] = {0};
    errno_t err;
    _tzset();
    err = _strdate_s( tmpbuf, 9 );
    if (err)
    {
       CPrintF("Error getting date\n");
       tmpbuf[6] = '1';
       tmpbuf[7] = '4';
       tmpbuf[0] = '1';
       tmpbuf[1] = '2';
       tmpbuf[3] = '1';
       tmpbuf[4] = '2';
    }
    datetime[0]  = '2';
    datetime[1]  = '0';
    datetime[2]  = tmpbuf[6];
    datetime[3]  = tmpbuf[7];
    datetime[4]  = '-';
    datetime[5]  = tmpbuf[0];
    datetime[6]  = tmpbuf[1];
    datetime[7]  = '-';
    datetime[8]  = tmpbuf[3];
    datetime[9] = tmpbuf[4];
    datetime[10] = ' ';

    err = _strtime_s( tmpbuf, 9 );
    if (err)
    {
       CPrintF("Error getting time\n");
       tmpbuf[0] = '1';
       tmpbuf[1] = '1';
       tmpbuf[2] = ':';
       tmpbuf[3] = '1';
       tmpbuf[4] = '1';
       tmpbuf[5] = ':';
       tmpbuf[6] = '1';
       tmpbuf[7] = '1';
       tmpbuf[8] = 0x00;
    }

    for (unsigned int i = 0; i < 9 ; ++i){
        datetime[11 + i] = tmpbuf[i];
    }
    datetime[20] = 0x00;
    return;
}
#else
void DateTime(char *datetime){

    char dtbuf[40] = {0};
    time_t st;

    time(&st);
    strftime (dtbuf, sizeof (dtbuf), "%Y-%m-%d %H:%M:%S",localtime (&st));
    for (unsigned int i = 0; i < 20 ; ++i){
        datetime[i] = dtbuf[i];
    }
    datetime[20] = 0x00;
    return;
}
#endif

int timeout(SOCKET sock, int sec) {
    struct  timeval tout;
    fd_set  fd_read;
    int     err;

    tout.tv_sec  = sec;
    tout.tv_usec = 0;
    FD_ZERO(&fd_read);
    FD_SET(sock, &fd_read);
    err = select(sock + 1, &fd_read, NULL, NULL, &tout);
    //if(err < 0) std_err();
    if(err < 0) return(-1);
    if(!err) return(-1);
    return(0);
}


void _uninitWinsock();
void _initializeWinsock(void)
{
#ifdef PLATFORM_WIN32
  if(_wsaData != NULL && _socket != INVALID_SOCKET) {
    return;
  }

  _wsaData = new WSADATA;
  _socket = INVALID_SOCKET;

  // start WSA
  if(WSAStartup(MAKEWORD(2, 2), _wsaData) != 0) {
    DateTime(_datetime);
    CPrintF("[%s] Error initializing winsock!\n", _datetime);
    _uninitWinsock();
    return;
  }
#else
  if(_socket != INVALID_SOCKET) {
    DateTime(_datetime);
    CPrintF("[%s] GameAgent: socket already created and binding\n", _datetime);
    return;
  }
#endif

  // make the buffer that we'll use for packet reading
  if(_szBuffer != NULL) {
    delete[] _szBuffer;
  }
  _szBuffer = new char[2050];

  // get the host IP
  hostent* phe;
  #ifndef PLATFORM_FREEBSD
  if(!ga_bMSLegacy) {
    phe = gethostbyname(ga_strServer);
  } else {
    phe = gethostbyname(ga_strMSLegacy);
  }
  // if we couldn't resolve the hostname
  if(phe == NULL) {
    // report and stop
    DateTime(_datetime);
    CPrintF("[%s] Couldn't resolve GameAgent server %s.\n", _datetime, (const char *) ga_strServer);
    _uninitWinsock();
    return;
  }
  #endif
  
  // create the socket destination address
  _sin = new sockaddr_in;
  _sin->sin_family = AF_INET;
  #ifdef PLATFORM_FREEBSD
  _sin->sin_addr.s_addr = inet_addr("116.202.216.176");  // 42amsterdam.net = 116.202.216.176
  #else
  _sin->sin_addr.s_addr = *(ULONG*)phe->h_addr_list[0];
  #endif
  
  if(!ga_bMSLegacy) {
    _sin->sin_port = htons(9005);
  } else {
    _sin->sin_port = htons(27900);
  }

  // create the socket
  _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (_socket == INVALID_SOCKET) {
    DateTime(_datetime);
    CPrintF("[%s] Error creating GameAgent socket!\n", _datetime);
    _uninitWinsock();
    return;
  }
  DateTime(_datetime);
  CPrintF("[%s] Creating GameAgent socket.\n", _datetime);

  // if we're a server
  if(_bServer) {
    // create the local socket source address
    _sinLocal = new sockaddr_in;
    _sinLocal->sin_family = AF_INET;
    _sinLocal->sin_addr.s_addr = inet_addr("0.0.0.0");
    _sinLocal->sin_port = htons(_pShell->GetINDEX("net_iPort") + 1);

    // bind the socket
    bind(_socket, (sockaddr*)_sinLocal, sizeof(*_sinLocal));
    DateTime(_datetime);
    CPrintF("[%s] Server: Bind GameAgent socket.\n", _datetime);
  }

  // set the socket to be nonblocking
#ifdef PLATFORM_WIN32
  DWORD dwNonBlocking = 1;
  if(ioctlsocket(_socket, FIONBIO, &dwNonBlocking) != 0) {
    DateTime(_datetime);
    CPrintF("[%s] Error setting socket to nonblocking!\n", _datetime);
    _uninitWinsock();
    return;
  }
#else
  int flags = fcntl(_socket, F_GETFL);
  int failed = flags;
  if (failed != -1) {
      flags |= O_NONBLOCK;
      failed = fcntl(_socket, F_SETFL, flags);
  }

  if (failed == -1) {
    DateTime(_datetime);
    CPrintF("[%s] Error setting socket to nonblocking!\n", _datetime);
    _uninitWinsock();
    return;
  }
#endif
  //DateTime(_datetime);
  //CPrintF("[%s] Set socket to nonblocking - done.\n", _datetime);
}

void _uninitWinsock()
{
  if (_socket != INVALID_SOCKET) {
    closesocket(_socket);
    _socket = INVALID_SOCKET;
  }
  #ifdef PLATFORM_WIN32
  if(_wsaData != NULL) {
    delete _wsaData;
    _wsaData = NULL;
  }
  #endif
}

void _sendPacketTo(const char* szBuffer, sockaddr_in* addsin)
{
  //DateTime(_datetime);
  //CPrintF("[%s] Send sendto\n", _datetime);
  sendto(_socket, szBuffer, strlen(szBuffer), 0, (sockaddr*)addsin, sizeof(sockaddr_in));
}


void _sendPacket(const char* szBuffer) // send only
{
  _sendPacketTo(szBuffer, _sin);
}
void _sendPacketB(const char* szBuffer) // Create bind + send
{
  _initializeWinsock();
  _sendPacketTo(szBuffer, _sin);
}

int _recvPacket()
{
  socklen_t fromLength = sizeof(_sinFrom);
  return recvfrom(_socket, _szBuffer, 2048, 0, (sockaddr*)&_sinFrom, &fromLength);
}

CTString _getGameModeName(INDEX iGameMode)
{
  // get function that will provide us the info about gametype
  CShellSymbol *pss = _pShell->GetSymbol("GetGameTypeNameSS", /*bDeclaredOnly=*/ TRUE);

  if(pss == NULL) {
    return "";
  }

  CTString (*pFunc)(INDEX) = (CTString (*)(INDEX))pss->ss_pvValue;
  return pFunc(iGameMode);
}

const CSessionProperties* _getSP()
{
  return ((const CSessionProperties *)_pNetwork->GetSessionProperties());
}

void _sendHeartbeat(INDEX iChallenge)
{
  CTString strPacket;
  if (!ga_bMSLegacy) {
    strPacket.PrintF("0;challenge;%d;players;%d;maxplayers;%d;level;%s;gametype;%s;version;%s;product;%s",
        iChallenge,
        _pNetwork->ga_srvServer.GetPlayersCount(),
        _pNetwork->ga_sesSessionState.ses_ctMaxPlayers,
        (const char *) _pNetwork->ga_World.wo_strName,
        (const char *) _getGameModeName(_getSP()->sp_gmGameMode),
        _SE_VER_STRING,
        (const char *) _pShell->GetString("sam_strGameName"));
  } else {
    #ifdef FIRST_ENCOUNTER  // First Encounter
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussam", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussam\n", _datetime);
    #else
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussamse", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussamse\n", _datetime);
    #endif
  }
  _sendPacket(strPacket);
  _tmLastHeartbeat = _pTimer->GetRealTimeTick();
}

static void _setStatus(const CTString &strStatus)
{
  _pNetwork->ga_bEnumerationChange = TRUE;
  _pNetwork->ga_strEnumerationStatus = strStatus;
}

CServerRequest::CServerRequest(void)
{
  Clear();
}
CServerRequest::~CServerRequest(void) { }
void CServerRequest::Clear(void)
{
  sr_ulAddress = 0;
  sr_iPort = 0;
  sr_tmRequestTime = 0;
}

/// Initialize GameAgent.
extern void GameAgent_ServerInit(void)
{
  // join
  _bServer = TRUE;
  _bInitialized = TRUE;

  if (!ga_bMSLegacy) {
    _sendPacketB("q");
  } else {
    CTString strPacket;
    #ifdef FIRST_ENCOUNTER  // First Encounter
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussam", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussam\n", _datetime);
    #else
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussamse", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussamse\n", _datetime);
    #endif
    _sendPacketB(strPacket);
  }
}

/// Let GameAgent know that the server has stopped.
extern void GameAgent_ServerEnd(void)
{
  if (!_bInitialized) {
    return;
  }

  if (ga_bMSLegacy) {
    CTString strPacket;
    #ifdef FIRST_ENCOUNTER  // First Encounter
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussam\\statechanged", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussam\\statechanged\n", _datetime);
    #else
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussamse\\statechanged", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussamse\\statechanged\n", _datetime);
    #endif
    _sendPacket(strPacket);
  }

  _uninitWinsock();
  _bInitialized = FALSE;
}

/// GameAgent server update call which responds to enumeration pings and sends pings to masterserver.
extern void GameAgent_ServerUpdate(void)
{
  if((_socket == INVALID_SOCKET) || (!_bInitialized)) {
    return;
  }

  int iLen = _recvPacket();
  if(iLen > 0) {
    if (!ga_bMSLegacy) {
    // check the received packet ID
    switch(_szBuffer[0]) {
    case 1: // server join response
      {
        int iChallenge = *(INDEX*)(_szBuffer + 1);
        // send the challenge
        _sendHeartbeat(iChallenge);
        break;
      }

    case 2: // server status request
      {
        // send the status response
        CTString strPacket;
        strPacket.PrintF("0;players;%d;maxplayers;%d;level;%s;gametype;%s;version;%s;gamename;%s;sessionname;%s",
          _pNetwork->ga_srvServer.GetPlayersCount(),
          _pNetwork->ga_sesSessionState.ses_ctMaxPlayers,
          (const char *) _pNetwork->ga_World.wo_strName,
          (const char *) _getGameModeName(_getSP()->sp_gmGameMode),
          _SE_VER_STRING,
          (const char *) _pShell->GetString("sam_strGameName"),
          (const char *) _pShell->GetString("gam_strSessionName"));
        _sendPacketTo(strPacket, &_sinFrom);
        break;
      }

    case 3: // player status request
      {
        // send the player status response
        CTString strPacket;
        strPacket.PrintF("\x01players\x02%d\x03", _pNetwork->ga_srvServer.GetPlayersCount());
        for(INDEX i=0; i<_pNetwork->ga_srvServer.GetPlayersCount(); i++) {
          CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[i];
          CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[i];
          if(plt.plt_bActive) {
            CTString strPlayer;
            plt.plt_penPlayerEntity->GetGameAgentPlayerInfo(plb.plb_Index, strPlayer);

            // if we don't have enough space left for the next player
            if(strlen(strPacket) + strlen(strPlayer) > 2048) {
              // send the packet
              _sendPacketTo(strPacket, &_sinFrom);
              strPacket = "";
            }

            strPacket += strPlayer;
          }
        }

        strPacket += "\x04";
        _sendPacketTo(strPacket, &_sinFrom);
        break;
      }

    case 4: // ping
      {
        // just send back 1 byte and the amount of players in the server (this could be useful in some cases for external scripts)
        CTString strPacket;
        strPacket.PrintF("\x04%d", _pNetwork->ga_srvServer.GetPlayersCount());
        _sendPacketTo(strPacket, &_sinFrom);
        break;
      }
     }
   } else {

      _szBuffer[iLen] = 0;
      // for debug
      DateTime(_datetime);
      CPrintF("[%s] GameAgent query: %s\n", _datetime, _szBuffer);
      //
      char *sPch1 = NULL, *sPch2 = NULL, *sPch3 = NULL, *sPch4 = NULL, *sPch5 = NULL, *sPch6 = NULL;
      sPch1 = strstr(_szBuffer, "\\status\\");
      sPch2 = strstr(_szBuffer, "\\info\\");
      sPch3 = strstr(_szBuffer, "\\basic\\");
      sPch4 = strstr(_szBuffer, "\\players\\");
      sPch5 = strstr(_szBuffer, "\\rules\\");
      sPch6 = strstr(_szBuffer, "\\secure\\");
      if(sPch1) {
        INDEX iStartDifficulty = _pShell->GetINDEX( "gam_iStartDifficulty");
        CTString strDifficulty = "Normal";
        switch(iStartDifficulty) {
            case -1: strDifficulty = "Tourist"; break;
            case  0: strDifficulty = "Easy"; break;
            case  1: strDifficulty = "Normal"; break;
            case  2: strDifficulty = "Hard"; break;
            case  3: strDifficulty = "Serious"; break;
            case  4: strDifficulty = "Mental"; break;
        }
        CTString strPacket;
        CTString strLocation;
        CTString strGameAgentExtras;
        CTString strMod = _strModName;
        strLocation = _pShell->GetString("net_strLocalHost");
        if ( strLocation == ""){
          strLocation = "Heartland";
        }
        strPacket.PrintF( PCKQUERY,
          (const char *) _pShell->GetString("sam_strGameName"),
          _SE_VER_STRING,
          //(const char *) _pShell->GetString("net_strLocalHost"),
          (const char *) strLocation,
          (const char *) _pShell->GetString("gam_strSessionName"),
          _pShell->GetINDEX("net_iPort"),
          (const char *) _pNetwork->ga_World.wo_strName,
          (const char *) _getGameModeName(_getSP()->sp_gmGameMode),
          (const char *) strMod,
          _pNetwork->ga_srvServer.GetPlayersCount(),
          _pNetwork->ga_sesSessionState.ses_ctMaxPlayers,
          (const char *) strDifficulty,
          _pShell->GetINDEX("gam_bFriendlyFire"),
          _pShell->GetINDEX("gam_bWeaponsStay"),
          _pShell->GetINDEX("gam_bAmmoStays"),
          _pShell->GetINDEX("gam_bHealthArmorStays"),
          _pShell->GetINDEX("gam_bAllowHealth"),
          _pShell->GetINDEX("gam_bAllowArmor"),
          _pShell->GetINDEX("gam_bInfiniteAmmo"),
          _pShell->GetINDEX("gam_bRespawnInPlace"));

          // Note: deprecated! ->
          // we don't add player names to the query, only the Players Count. 
          //So we will be safe from buffer overflow of the function PrintF - buffer increase in function - not worked!! see CTString.cpp

          for(INDEX i=0; i<_pNetwork->ga_srvServer.GetPlayersCount(); i++) {
            CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[i];
            CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[i];
            if(plt.plt_bActive) {
              CTString strPlayer;
              plt.plt_penPlayerEntity->GetMSLegacyPlayerInf(plb.plb_Index, strPlayer);

              // if we don't have enough space left for the next player
              if(strlen(strPacket) + strlen(strPlayer) > 4096) { // buff 5120
                // send the packet
                _sendPacketTo(strPacket, &_sinFrom);
                strPacket = "";
              }
              strPacket += strPlayer;
            }
          }

        strGameAgentExtras = _pShell->GetString("gam_strGameAgentExtras"); // see Dedicated_startup.ini
        if ( strGameAgentExtras != ""){
           strPacket += strGameAgentExtras;
        }

        strPacket += "\\final\\\\queryid\\333.1";
        _sendPacketTo(strPacket, &_sinFrom);

      } else if (sPch2){

        CTString strPacket;
        CTString strMod = _strModName;
        strPacket.PrintF( PCKINFO,
          (const char *) _pShell->GetString("gam_strSessionName"),
          _pShell->GetINDEX("net_iPort"),
          (const char *) _pNetwork->ga_World.wo_strName,
          (const char *) _getGameModeName(_getSP()->sp_gmGameMode),
          (const char *) strMod,
          _pNetwork->ga_srvServer.GetPlayersCount(),
          _pNetwork->ga_sesSessionState.ses_ctMaxPlayers);
        _sendPacketTo(strPacket, &_sinFrom);

      } else if (sPch3){

        CTString strPacket;
        CTString strLocation;
        strLocation = _pShell->GetString("net_strLocalHost");
        if ( strLocation == ""){
          strLocation = "Heartland";
        }
        strPacket.PrintF( PCKBASIC,
          (const char *) _pShell->GetString("sam_strGameName"),
          _SE_VER_STRING,
          //(const char *) _pShell->GetString("net_strLocalHost"));
          (const char *) strLocation);
        _sendPacketTo(strPacket, &_sinFrom);

      } else if (sPch4){

        // send the player status response
        CTString strPacket;
        strPacket = "";
        for(INDEX i=0; i<_pNetwork->ga_srvServer.GetPlayersCount(); i++) {
          CPlayerBuffer &plb = _pNetwork->ga_srvServer.srv_aplbPlayers[i];
          CPlayerTarget &plt = _pNetwork->ga_sesSessionState.ses_apltPlayers[i];
          if(plt.plt_bActive) {
            CTString strPlayer;
            plt.plt_penPlayerEntity->GetMSLegacyPlayerInf(plb.plb_Index, strPlayer);

            // if we don't have enough space left for the next player
            if(strlen(strPacket) + strlen(strPlayer) > 4096) {
              // send the packet
              _sendPacketTo(strPacket, &_sinFrom);
              strPacket = "";
            }

            strPacket += strPlayer;
          }
        }

        strPacket += "\\final\\\\queryid\\332.1";
        _sendPacketTo(strPacket, &_sinFrom);

      } else if (sPch5){
        INDEX iStartDifficulty = _pShell->GetINDEX( "gam_iStartDifficulty");
        CTString strDifficulty = "Normal";
        switch(iStartDifficulty) {
            case -1: strDifficulty = "Tourist"; break;
            case  0: strDifficulty = "Easy"; break;
            case  1: strDifficulty = "Normal"; break;
            case  2: strDifficulty = "Hard"; break;
            case  3: strDifficulty = "Serious"; break;
            case  4: strDifficulty = "Mental"; break;
        }
        CTString strPacket;
        CTString strLocation;
        CTString strGameAgentExtras;
        strLocation = _pShell->GetString("net_strLocalHost");
        if ( strLocation == ""){
          strLocation = "Heartland";
        }
        strPacket.PrintF( PCKRULES,
          (const char *) strDifficulty,
          _pShell->GetINDEX("gam_bFriendlyFire"),
          _pShell->GetINDEX("gam_bWeaponsStay"),
          _pShell->GetINDEX("gam_bAmmoStays"),
          _pShell->GetINDEX("gam_bHealthArmorStays"),
          _pShell->GetINDEX("gam_bAllowHealth"),
          _pShell->GetINDEX("gam_bAllowArmor"),
          _pShell->GetINDEX("gam_bInfiniteAmmo"),
          _pShell->GetINDEX("gam_bRespawnInPlace"));

        strGameAgentExtras = _pShell->GetString("gam_strGameAgentExtras"); // see Dedicated_startup.ini
        if ( strGameAgentExtras != ""){
           strPacket += strGameAgentExtras;
        }

        strPacket += "\\final\\\\queryid\\330.1";
        _sendPacketTo(strPacket, &_sinFrom);

      } else if (sPch6){
        // we ignore ... this
        DateTime(_datetime);
        CPrintF("[%s] GameAgent: \\secure\\ query\n", _datetime);
        //
      } else {
        DateTime(_datetime);
        CPrintF("[%s] GameAgent_ServerUpdate: Unknown query server response!\n", _datetime);
        return;
      }
   }
 }

 // send a heartbeat every 150 seconds
 if(_pTimer->GetRealTimeTick() - _tmLastHeartbeat >= 150.0f) {
    _sendHeartbeat(0);
 }
}

/// Notify GameAgent that the server state has changed.
extern void GameAgent_ServerStateChanged(void)
{
  if (!_bInitialized) {
    return;
  }
  if (!ga_bMSLegacy) {
    _sendPacket("u");
  } else {
    CTString strPacket;
    #ifdef FIRST_ENCOUNTER  // First Encounter
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussam\\statechanged", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussam\\statechanged\n", _datetime);
    #else
    strPacket.PrintF("\\heartbeat\\%hu\\gamename\\serioussamse\\statechanged", (_pShell->GetINDEX("net_iPort") + 1));
    DateTime(_datetime);
    CPrintF("[%s] Send heartbeat - serioussamse\\statechanged\n", _datetime);
    #endif
    _sendPacket(strPacket);
  }
}

/// Request serverlist enumeration.
extern void GameAgent_EnumTrigger(BOOL bInternet)
{

  if ( _pNetwork->ga_bEnumerationChange ) {
    return;
  }
  
  if ( !bInternet && ga_bMSLegacy) {
    // make sure that there are no requests still stuck in buffer
    ga_asrRequests.Clear();
    // we're not a server
    _bServer = FALSE;
    _pNetwork->ga_strEnumerationStatus = ".";

	PHOSTENT _phHostinfo;
	ULONG    _uIP,*_pchIP = &_uIP;
	USHORT   _uPort,*_pchPort = &_uPort;
	LONG     _iLen;
	char     _cName[256],*_pch,_strFinal[8] = {0};

	struct in_addr addr;

    // make the buffer that we'll use for packet reading
    if(_szIPPortBufferLocal != NULL) {
       return;
    }
    _szIPPortBufferLocal = new char[1024];

    #ifdef PLATFORM_WIN32
	// start WSA
	WSADATA  wsaData;
	const WORD _wsaRequested = MAKEWORD( 2, 2 );
    if( WSAStartup(_wsaRequested, &wsaData) != 0) {
        DateTime(_datetime);
		CPrintF("[%s] Error initializing winsock!\n", _datetime);
		if(_szIPPortBufferLocal != NULL) {
			delete[] _szIPPortBufferLocal;
		}
		_szIPPortBufferLocal = NULL;
		_uninitWinsock();
		_bInitialized = FALSE;
		_pNetwork->ga_bEnumerationChange = FALSE;
		_pNetwork->ga_strEnumerationStatus = "";
		WSACleanup();
        return;
    }
    #endif

    _pch = _szIPPortBufferLocal;
	_iLen = 0;
	strcpy(_strFinal,"\\final\\");
	
    if( gethostname ( _cName, sizeof(_cName)) == 0)
	{
	    #ifdef PLATFORM_FREEBSD
	    if((_phHostinfo = gethostbyname("localhost")) != NULL)
	    #else
		if((_phHostinfo = gethostbyname(_cName)) != NULL)
		#endif
		{
			int _iCount = 0;
			while(_phHostinfo->h_addr_list[_iCount])
			{
				addr.s_addr = *(u_long *) _phHostinfo->h_addr_list[_iCount];
				_uIP = htonl(addr.s_addr);
				
				for (UINT uPort = 25601; uPort < 25622; ++uPort){
					_uPort = htons(uPort);
					memcpy(_pch,_pchIP,4);
					_pch  +=4;
					_iLen +=4;
					memcpy(_pch,_pchPort,2);
					_pch  +=2;
					_iLen +=2;
				}
				++_iCount;
			}
			memcpy(_pch,_strFinal, 7);
			_pch  +=7;
			_iLen +=7;
			_pch[_iLen] = 0x00;
		}
	}
    _iIPPortBufferLocalLen = _iLen;

    _bActivatedLocal = TRUE;
    _bInitialized = TRUE;
    _initializeWinsock();	
    return;
	
  } else {

  if (!ga_bMSLegacy) {
    // make sure that there are no requests still stuck in buffer
    ga_asrRequests.Clear();
    // we're not a server
    _bServer = FALSE;
    // Initialization
    _bInitialized = TRUE;
    // send enumeration packet to masterserver
    _sendPacketB("e");
    _setStatus(".");
  }
  else
  { /* MSLegacy */
    // make sure that there are no requests still stuck in buffer
    ga_asrRequests.Clear();
    // we're not a server
    _bServer = FALSE;
    _pNetwork->ga_strEnumerationStatus = ".";

    struct  sockaddr_in peer; 
    struct  timeval tv; 

    socklen_t lon; 
    fd_set  _fdset;
    SOCKET  _sock               = INVALID_SOCKET;
    u_int   uiMSIP;
    long    arg; 
    int     res,valopt,iErr,
            iLen,
            iDynsz,
            iEnctype             = 0;
    u_short usMSport             = MSPORT;

    u_char  ucGamekey[]          = {SERIOUSSAMKEY},
            ucGamestr[]          = {SERIOUSSAMSTR},
            *ucSec               = NULL,
            *ucKey               = NULL;

    const char *cFilter             = "";
    const char *cWhere              = "";
    char cMS[128]             = {0};
    char *cResponse           = NULL;
    char *cMsstring           = NULL;
    char *cSec                = NULL;

    strcpy(cMS,ga_strMSLegacy);

    #ifdef PLATFORM_WIN32
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error initializing winsock!\n", _datetime);
        return;
    } else {
		DateTime(_datetime);
		CPrintF("[%s] Initializing winsock - Done.\n", _datetime);
	}
    #endif

/* Open a socket and connect to the Master server */

    #ifdef PLATFORM_FREEBSD
    peer.sin_addr.s_addr = inet_addr("116.202.216.176");  // 42amsterdam.net = 116.202.216.176
    peer.sin_port        = htons(28900);
    #else
    peer.sin_addr.s_addr = uiMSIP = resolv(cMS);
    peer.sin_port        = htons(usMSport);
    #endif
    peer.sin_family      = AF_INET;

    static const struct linger  ling = {1,2};
    struct timeval timeout_tcp;      
    timeout_tcp.tv_sec = 2;
    timeout_tcp.tv_usec = 0;

    _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(_sock < 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error creating TCP socket!\n", _datetime);
        WSACleanup();
        return;
	} else {
		DateTime(_datetime);
		CPrintF("[%s]  Creating TCP socket - Done.\n", _datetime);
	}

/* Set non-blocking  */

#ifdef PLATFORM_UNIX
    if( (arg = fcntl(_sock, F_GETFL, NULL)) < 0) {
        DateTime(_datetime); 
        CPrintF("[%s] Error fcntl(..., F_GETFL) (%s)\n", _datetime, strerror(errno)); 
        CLEANMSSRUFF1;
        return;
    } 
    arg |= O_NONBLOCK; 
    if( fcntl(_sock, F_SETFL, arg) < 0) {
        DateTime(_datetime); 
        CPrintF("[%s] Error fcntl(..., F_SETFL) (%s)\n", _datetime, strerror(errno)); 
        CLEANMSSRUFF1;
        return;
    }
#else
/*
// Set nonblocked mode
	BOOL l1 = TRUE;
	if (SOCKET_ERROR == ioctlsocket(_sock, FIONBIO, (unsigned long*)&l1))
	{
		DateTime(_datetime);
		CPrintF("[%s] Error ioctlsocket(..., FIONBIO) (%s)\n", _datetime, strerror(errno));
		CLEANMSSRUFF1;
		return;
	}*/
#endif

/* Trying to connect with timeout   */

#ifndef	EINPROGRESS
#define EINPROGRESS 119
#endif

#ifndef	EINTR
#define	EINTR 4
#endif

    res = connect(_sock, (struct sockaddr *)&peer, sizeof(peer)); 
    if (res < 0) { 
       if (errno == EINPROGRESS) { 
          DateTime(_datetime);
          CPrintF("[%s] GameAgent EINPROGRESS in connect() - selecting\n", _datetime); 
          do { 
             tv.tv_sec  = 2; 
             tv.tv_usec = 0; 
             FD_ZERO(&_fdset); 
             FD_SET(_sock, &_fdset); 
             res = select(_sock+1, NULL, &_fdset, NULL, &tv); 
             if (res < 0 && errno != EINTR) {
                DateTime(_datetime);
                CPrintF("[%s] Error connecting %d - %s\n", _datetime, errno, strerror(errno)); 
                CLEANMSSRUFF1;
                return;
             } 
             else if (res > 0) { 
                // Socket selected for write 
                lon = sizeof(int); 
                if (getsockopt(_sock, SOL_SOCKET, SO_ERROR, (char*)(&valopt), &lon) < 0) { 
                   DateTime(_datetime);
                   CPrintF("[%s] Error in getsockopt() %d - %s\n", _datetime, errno, strerror(errno)); 
                   CLEANMSSRUFF1;
                   return;
                } 
                // Check the value returned... 
                if (valopt) { 
                   DateTime(_datetime);
                   CPrintF("[%s] Error in delayed connection() %d - %s\n", _datetime, valopt, strerror(valopt)); 
                   CLEANMSSRUFF1;
                   return;
                } 
                break; 
             } 
             else {
                DateTime(_datetime);
                CPrintF("[%s] Timeout in select() - Cancelling!\n", _datetime); 
                CLEANMSSRUFF1;
                return;
             } 
          } while (1); 
       } 
       else { 
          DateTime(_datetime);
          CPrintF("[%s] Error connecting %d - %s\n", _datetime, errno, strerror(errno)); 
          CLEANMSSRUFF1;
          return;
       } 
    } 
#ifdef PLATFORM_UNIX
    // Set to blocking mode again... 
    if( (arg = fcntl(_sock, F_GETFL, NULL)) < 0) {
       DateTime(_datetime); 
       CPrintF("[%s] Error fcntl(..., F_GETFL) (%s)\n", _datetime, strerror(errno)); 
       CLEANMSSRUFF1;
       return;
    } 
    arg &= (~O_NONBLOCK); 
    if( fcntl(_sock, F_SETFL, arg) < 0) {
       DateTime(_datetime); 
       CPrintF("[%s] Error fcntl(..., F_SETFL) (%s)\n", _datetime, strerror(errno)); 
       CLEANMSSRUFF1;
       return;
    }
#else
/*
// Set nonblocked mode again... 
BOOL l2 = TRUE;
if (SOCKET_ERROR == ioctlsocket(_sock, FIONBIO, (unsigned long*)&l2))
{
	DateTime(_datetime);
	CPrintF("[%s] Error ioctlsocket(..., FIONBIO) (%s)\n", _datetime, strerror(errno));
	CLEANMSSRUFF1;
	return;
}*/
#endif

/* setsockopt */

#ifdef PLATFORM_UNIX
    if (setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char *)&ling,
                sizeof (ling)) < 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error setsockopt SO_LINGER to TCP socket!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }
    if (setsockopt (_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_tcp,
                sizeof (timeout_tcp)) < 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error setsockopt SO_RCVTIMEO to TCP socket!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }
    if (setsockopt (_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout_tcp,
                sizeof (timeout_tcp)) < 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error setsockopt SO_SNDTIMEO to TCP socket!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }
#endif

/* Allocate memory for a buffer and get a pointer to it */

    cResponse = (char*) malloc(BUFFSZSTR + 1);
    if(!cResponse) {
        DateTime(_datetime);
        CPrintF("[%s] Error initializing memory buffer!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }

/* Reading response from Master Server - returns the string with the secret key */

    iLen = 0;
    iErr = recv(_sock, (char*)cResponse + iLen, BUFFSZSTR - iLen, 0);
    if(iErr < 0) {
        DateTime(_datetime);
        CPrintF("[%s] Error reading from TCP socket!\n", _datetime);
        CLEANMSSRUFF2;
        return;
    }

    iLen += iErr;
    cResponse[iLen] = 0x00;

/* Allocate memory for a buffer and get a pointer to it */

    ucSec = (u_char*) malloc(BUFFSZSTR + 1);
    if(!ucSec) {
        DateTime(_datetime);
        CPrintF("[%s] Error initializing memory buffer!\n", _datetime);
        CLEANMSSRUFF2;
        return;
    }
    memcpy ( ucSec, cResponse,  BUFFSZSTR);
    ucSec[iLen] = 0x00;

/* Geting the secret key from a string */

    cSec = strstr(cResponse, "\\secure\\");
    if(!cSec) {
        DateTime(_datetime);
        CPrintF("[%s] Not valid master server response!\n", _datetime);
        CLEANMSSRUFF2;
        return;
    } else {
        ucSec  += 15;
        //DateTime(_datetime);
        //CPrintF("[%s] Valid master server response - step 1\n", _datetime);

/* Creating a key for authentication (Validate key) */

        ucKey = gsseckey(ucSec, ucGamekey, iEnctype);
    }
    ucSec -= 15;
    if(cResponse) free (cResponse);
    if(ucSec) free (ucSec);

/* Generate a string for the response (to Master Server) with the specified (Validate ucKey) */

    cMsstring = (char*) malloc(BUFFSZSTR + 1);
    if(!cMsstring) {
        DateTime(_datetime);
        CPrintF("[%s] Not valid master server response!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }
    //DateTime(_datetime);
    //CPrintF("[%s] Valid master server response - step 2\n", _datetime);
    iLen = _snprintf(
        cMsstring,
        BUFFSZSTR,
        PCK,
        ucGamestr,
        iEnctype,
        ucKey,
        ucGamestr,
        cWhere,
        cFilter);

/* Check the buffer */

    CHK_BUFFSTRLEN;

/* The string sent to master server */
  
    //DateTime(_datetime);
    //CPrintF("[%s] The string sent to master server\n", _datetime);
    if(send(_sock,cMsstring, iLen, 0) < 0){
        DateTime(_datetime);
        CPrintF("[%s] Error reading from TCP socket!\n", _datetime);
        if(cMsstring) free (cMsstring);
        CLEANMSSRUFF1;
        return;
    }
    if(cMsstring) free (cMsstring);
    

 /* Allocate memory for a buffer and get a pointer to it */

    if(_szIPPortBuffer ) {
        CLEANMSSRUFF1;
        return;
    };
    //DateTime(_datetime); 
    //CPrintF("[%s] The string sent to master server - done.\n", _datetime);
    _szIPPortBuffer = (char*) malloc(BUFFSZ + 1);
    if(!_szIPPortBuffer) {
        DateTime(_datetime);
        CPrintF("[%s] Error reading from TCP socket!\n", _datetime);
        CLEANMSSRUFF1;
        return;
    }
    iDynsz = BUFFSZ;
    //DateTime(_datetime);
    //CPrintF("[%s] Try received encoded data after sending the string (Validate key)\n", _datetime);

/* The received encoded data after sending the string (Validate key) */

    iLen = 0;
    DateTime(_datetime);
    CPrintF("[%s] Receiving:   ", _datetime);
    while(!timeout(_sock, 1)) {
        iErr = recv(_sock, _szIPPortBuffer + iLen, iDynsz - iLen, 0);
        if(iErr <= 0) break;
        DateTime(_datetime);
        CPrintF(".");
        iLen += iErr;
        if(iLen >= iDynsz) {
            iDynsz += BUFFSZ;
            _szIPPortBuffer = (char*)realloc(_szIPPortBuffer, iDynsz);
            if(!_szIPPortBuffer) {
                DateTime(_datetime);
                CPrintF("[%s] Error reallocation memory buffer!\n", _datetime);
                if(_szIPPortBuffer) free (_szIPPortBuffer);
                CLEANMSSRUFF1;
                return;
            }
        }
    }
    CPrintF(" %u bytes\n", iLen);
#ifdef PLATFORM_WIN32
	closesocket(_sock);
#else
	close(_sock);
#endif
    //DateTime(_datetime);
    //CPrintF("[%s] Received encoded data after sending the string (Validate key) - done", _datetime);
    CLEANMSSRUFF1;
    _iIPPortBufferLen = iLen;

    _bActivated = TRUE;
    _bInitialized = TRUE;
    _initializeWinsock();
    //DateTime(_datetime);
    //CPrintF("[%s] GameAgent_EnumTrigger - done\n", _datetime);  
    }
  }
}

/// GameAgent client update for enumerations.
extern void GameAgent_EnumUpdate(void)
{
  if((_socket == INVALID_SOCKET) || (!_bInitialized)) {
    return;
  }

  if (!ga_bMSLegacy) {
   int iLen = _recvPacket();
   if(iLen != -1) {
    // null terminate the buffer
    _szBuffer[iLen] = 0;
    switch(_szBuffer[0]) {
    case 's':
      {
        // !!! FIXME: serialize this and byteswap it.  --ryan.
        #pragma pack(1)
        struct sIPPort {
          UBYTE bFirst;
          UBYTE bSecond;
          UBYTE bThird;
          UBYTE bFourth;
          USHORT iPort;
        };
        #pragma pack()

        _pNetwork->ga_strEnumerationStatus = "";

        sIPPort* pServers = (sIPPort*)(_szBuffer + 1);
        while(iLen - ((CHAR*)pServers - _szBuffer) >= static_cast<int>(sizeof(sIPPort))) {
          sIPPort ip = *pServers;

          CTString strIP;
          strIP.PrintF("%d.%d.%d.%d", ip.bFirst, ip.bSecond, ip.bThird, ip.bFourth);

          sockaddr_in sinServer;
          sinServer.sin_family = AF_INET;
          sinServer.sin_addr.s_addr = inet_addr(strIP);
          sinServer.sin_port = htons(ip.iPort + 1);

          // insert server status request into container
          CServerRequest &sreq = ga_asrRequests.Push();
          sreq.sr_ulAddress = sinServer.sin_addr.s_addr;
          sreq.sr_iPort = sinServer.sin_port;
          sreq.sr_tmRequestTime = _pTimer->GetHighPrecisionTimer().GetMilliseconds();

          // send packet to server
          _sendPacketTo("\x02", &sinServer);

          pServers++;
        }
      }
      break;

    case '0':
      {
        CTString strPlayers;
        CTString strMaxPlayers;
        CTString strLevel;
        CTString strGameType;
        CTString strVersion;
        CTString strGameName;
        CTString strSessionName;

        CHAR* pszPacket = _szBuffer + 2; // we do +2 because the first character is always ';', which we don't care about.

        BOOL bReadValue = FALSE;
        CTString strKey;
        CTString strValue;

        while(*pszPacket != 0) {
          switch(*pszPacket) {
          case ';':
            if(strKey != "sessionname") {
              if(bReadValue) {
                // we're done reading the value, check which key it was
                if(strKey == "players") {
                  strPlayers = strValue;
                } else if(strKey == "maxplayers") {
                  strMaxPlayers = strValue;
                } else if(strKey == "level") {
                  strLevel = strValue;
                } else if(strKey == "gametype") {
                  strGameType = strValue;
                } else if(strKey == "version") {
                  strVersion = strValue;
                } else if(strKey == "gamename") {
                  strGameName = strValue;
                } else {
                  DateTime(_datetime);
                  CPrintF("[%s] Unknown GameAgent parameter key '%s'!",_datetime , (const char *) strKey);
                }

                // reset temporary holders
                strKey = "";
                strValue = "";
              }
            }
            bReadValue = !bReadValue;
            break;

          default:
            // read into the value or into the key, depending where we are
            if(bReadValue) {
              strValue.InsertChar(strlen(strValue), *pszPacket);
            } else {
              strKey.InsertChar(strlen(strKey), *pszPacket);
            }
            break;
          }

          // move to next character
          pszPacket++;
        }

        // check if we still have a sessionname to back up
        if(strKey == "sessionname") {
          strSessionName = strValue;
        }

        // insert the server into the serverlist
        CNetworkSession &ns = *new CNetworkSession;
        _pNetwork->ga_lhEnumeratedSessions.AddTail(ns.ns_lnNode);

        long long tmPing = -1;
        // find the request in the request array
        for(INDEX i=0; i<ga_asrRequests.Count(); i++) {
          CServerRequest &req = ga_asrRequests[i];
          if(req.sr_ulAddress == _sinFrom.sin_addr.s_addr && req.sr_iPort == _sinFrom.sin_port) {
            tmPing = _pTimer->GetHighPrecisionTimer().GetMilliseconds() - req.sr_tmRequestTime;
            ga_asrRequests.Delete(&req);
            break;
          }
        }

        if(tmPing == -1) {
          // server status was never requested
          break;
        }

        // add the server to the serverlist
        ns.ns_strSession = strSessionName;
        ns.ns_strAddress = inet_ntoa(_sinFrom.sin_addr) + CTString(":") + CTString(0, "%d", htons(_sinFrom.sin_port) - 1);
        ns.ns_tmPing = (tmPing / 1000.0f);
        ns.ns_strWorld = strLevel;
        ns.ns_ctPlayers = atoi(strPlayers);
        ns.ns_ctMaxPlayers = atoi(strMaxPlayers);
        ns.ns_strGameType = strGameType;
        ns.ns_strMod = strGameName;
        ns.ns_strVer = strVersion;
      }
      break;

    default:
      DateTime(_datetime);
      CPrintF("[%s] Unknown enum packet ID %x!\n",_datetime, _szBuffer[0]);
      break;
    }
  }
 } else {
 #ifndef PLATFORM_WIN32
 //STUBBED("write me");
 /* MSLegacy LinuxPort */
    if(_bActivated) {
        pthread_t  	_hThread;
        int   		_iThreadId;

        _iThreadId = pthread_create(&_hThread, NULL, _MS_Thread, NULL);
        if (_iThreadId == 0) {
            pthread_detach(_hThread);
        }
        _bActivated = FALSE;		
    }
    if(_bActivatedLocal) {
        pthread_t  _hThread;
        int   	   _iThreadId;

        _iThreadId = pthread_create(&_hThread, NULL, _LocalNet_Thread, NULL);
        if (_iThreadId == 0) {
            pthread_detach(_hThread);
        }
        _bActivatedLocal = FALSE;		
    }	 
 #else
 /* MSLegacy */
    if(_bActivated) {
        HANDLE  _hThread;
        DWORD   _dwThreadId;

        _hThread = CreateThread(NULL, 0, _MS_Thread, 0, 0, &_dwThreadId);
        if (_hThread != NULL) {
            CloseHandle(_hThread);
        }
        _bActivated = FALSE;		
    }
    if(_bActivatedLocal) {
        HANDLE  _hThread;
        DWORD   _dwThreadId;

        _hThread = CreateThread(NULL, 0, _LocalNet_Thread, 0, 0, &_dwThreadId);
        if (_hThread != NULL) {
            CloseHandle(_hThread);
        }
        _bActivatedLocal = FALSE;		
    }	
 #endif
 }
}

/// Cancel the GameAgent serverlist enumeration.
extern void GameAgent_EnumCancel(void)
{
  if (_bInitialized) {
    DateTime(_datetime);
    CPrintF("[%s]...GameAgent_EnumCancel!\n", _datetime);
    ga_asrRequests.Clear();
    _uninitWinsock();
  }
}

#ifdef PLATFORM_WIN32
DWORD WINAPI _MS_Thread(LPVOID lpParam) {
#else
void*        _MS_Thread(void *arg) {
#endif

    SOCKET _sockudp = INVALID_SOCKET;
    struct _sIPPort {
        UBYTE bFirst;
        UBYTE bSecond;
        UBYTE bThird;
        UBYTE bFourth;
        USHORT iPort;
      };

    _setStatus("");
    _sockudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockudp == INVALID_SOCKET){
        WSACleanup();
        return 0;
    }

    _sIPPort* pServerIP = (_sIPPort*)(_szIPPortBuffer);
    while(_iIPPortBufferLen >= 6) {
        if(!strncmp((char *)pServerIP, "\\final\\", 7)) {
                break;
            }

        _sIPPort ip = *pServerIP;

        CTString strIP;
        strIP.PrintF("%d.%d.%d.%d", ip.bFirst, ip.bSecond, ip.bThird, ip.bFourth);

        sockaddr_in sinServer;
        sinServer.sin_family = AF_INET;
        sinServer.sin_addr.s_addr = inet_addr(strIP);
        sinServer.sin_port = ip.iPort;

        // insert server status request into container
        CServerRequest &sreq = ga_asrRequests.Push();
        sreq.sr_ulAddress = sinServer.sin_addr.s_addr;
        sreq.sr_iPort = sinServer.sin_port;
        sreq.sr_tmRequestTime = _pTimer->GetHighPrecisionTimer().GetMilliseconds();

        // send packet to server
        sendto(_sockudp,"\\status\\",8,0,
            (sockaddr *) &sinServer, sizeof(sinServer));

        sockaddr_in _sinClient;
        socklen_t _iClientLength = sizeof(_sinClient);

        fd_set readfds_udp;                         // declare a read set
        struct timeval timeout_udp;                 // declare a timeval for our timer
        int iRet = -1;

        FD_ZERO(&readfds_udp);                      // zero out the read set
        FD_SET(_sockudp, &readfds_udp);                // add socket to the read set
        timeout_udp.tv_sec = 0;                     // timeout = 0 seconds
        timeout_udp.tv_usec = 50000;               // timeout += 0.05 seconds
        int _iN = select(_sockudp + 1, &readfds_udp, NULL, NULL, &timeout_udp);
        if (_iN > 0) {
          /** do recvfrom stuff **/
          iRet =  recvfrom(_sockudp, _szBuffer, 2048, 0, (sockaddr*)&_sinClient, &_iClientLength);
          FD_CLR(_sockudp, &readfds_udp);
          if(iRet != -1 && iRet > 100 && iRet != SOCKET_ERROR) {
            // null terminate the buffer
            _szBuffer[iRet] = 0;
            char *sPch = NULL;
            #ifdef FIRST_ENCOUNTER  // First Encounter
            sPch = strstr(_szBuffer, "\\gamename\\serioussam\\");
            #else
            sPch = strstr(_szBuffer, "\\gamename\\serioussamse\\");
            #endif
            if(!sPch) {
                DateTime(_datetime);
                CPrintF("[%s] MS_Thread: Unknown query server response!\n", _datetime);
                return 0;
            } else {

                CTString strPlayers;
                CTString strMaxPlayers;
                CTString strLevel;
                CTString strGameType;
                CTString strVersion;
                CTString strGameName;
                CTString strSessionName;

                CTString strGamePort;
                CTString strServerLocation;
                CTString strGameMode;
                CTString strActiveMod;

                CHAR* pszPacket = _szBuffer + 1; // we do +1 because the first character is always '\', which we don't care about.

                BOOL bReadValue = FALSE;
                CTString strKey;
                CTString strValue;

                while(*pszPacket != 0) {
                switch(*pszPacket) {
                case '\\':
                    if(strKey != "gamemode") {
                      if(bReadValue) {
                        // we're done reading the value, check which key it was
                        if(strKey == "gamename") {
                            strGameName = strValue;
                        } else if(strKey == "gamever") {
                            strVersion = strValue;
                        } else if(strKey == "location") {
                            strServerLocation = strValue;
                        } else if(strKey == "hostname") {
                            strSessionName = strValue;
                        } else if(strKey == "hostport") {
                            strGamePort = strValue;
                        } else if(strKey == "mapname") {
                            strLevel = strValue;
                        } else if(strKey == "gametype") {
                            strGameType = strValue;
                        } else if(strKey == "activemod") {
                            strActiveMod = strValue;
                        } else if(strKey == "numplayers") {
                            strPlayers = strValue;
                        } else if(strKey == "maxplayers") {
                            strMaxPlayers = strValue;
                        } else {
                            //DateTime(_datetime);
                            //CPrintF("[%s] Unknown GameAgent parameter key '%s'!", _datetime, strKey);
                        }
                        // reset temporary holders
                        strKey = "";
                        strValue = "";
                      }
                    }
                    bReadValue = !bReadValue;
                    break;

                default:
                    // read into the value or into the key, depending where we are
                    if(bReadValue) {
                        strValue.InsertChar(strlen(strValue), *pszPacket);
                    } else {
                        strKey.InsertChar(strlen(strKey), *pszPacket);
                    }
                    break;
                  }
                  // move to next character
                  pszPacket++;
                }

                // check if we still have a maxplayers to back up
                if(strKey == "gamemode") {
                    strGameMode = strValue;
                }
                if(strActiveMod != "") {
                    strGameName = strActiveMod;
                }
				
                long long tmPing = -1;
                // find the request in the request array
                for(INDEX i=0; i<ga_asrRequests.Count(); i++) {
                    CServerRequest &req = ga_asrRequests[i];
                    if(req.sr_ulAddress == _sinClient.sin_addr.s_addr && req.sr_iPort == _sinClient.sin_port) {
                        tmPing = _pTimer->GetHighPrecisionTimer().GetMilliseconds() - req.sr_tmRequestTime;
                        ga_asrRequests.Delete(&req);
                        break;
                    }
                }

                if(tmPing > 0 && tmPing < 2500000) {
				    // insert the server into the serverlist
                    CNetworkSession &ns = *new CNetworkSession;
                    _pNetwork->ga_lhEnumeratedSessions.AddTail(ns.ns_lnNode);
					
                    // add the server to the serverlist
                    ns.ns_strSession = strSessionName;
                    ns.ns_strAddress = inet_ntoa(_sinClient.sin_addr) + CTString(":") + CTString(0, "%d", htons(_sinClient.sin_port) - 1);
                    ns.ns_tmPing = (tmPing / 1000.0f);
                    ns.ns_strWorld = strLevel;
                    ns.ns_ctPlayers = atoi(strPlayers);
                    ns.ns_ctMaxPlayers = atoi(strMaxPlayers);
                    ns.ns_strGameType = strGameType;
                    ns.ns_strMod = strGameName;
                    ns.ns_strVer = strVersion;
                }
            }
          } else {
            // find the request in the request array
            for(INDEX i=0; i<ga_asrRequests.Count(); i++) {
              CServerRequest &req = ga_asrRequests[i];
              if(req.sr_ulAddress == _sinClient.sin_addr.s_addr && req.sr_iPort == _sinClient.sin_port) {
                ga_asrRequests.Delete(&req);
                break;
              }
            }
          }
        }
        pServerIP++;
        _iIPPortBufferLen -= 6;
    }
    if(_szIPPortBuffer) free (_szIPPortBuffer);
    _szIPPortBuffer = NULL;

    closesocket(_sockudp);
    _uninitWinsock();
    _bInitialized = FALSE;
    _pNetwork->ga_bEnumerationChange = FALSE;
    WSACleanup();
    return 0;
}

#ifdef PLATFORM_WIN32
DWORD WINAPI _LocalNet_Thread(LPVOID lpParam) {
#else
void*        _LocalNet_Thread(void *arg) {
#endif

    SOCKET _sockudp = INVALID_SOCKET;
    struct _sIPPort {
        UBYTE bFirst;
        UBYTE bSecond;
        UBYTE bThird;
        UBYTE bFourth;
        USHORT iPort;
      };

    _sockudp = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockudp == INVALID_SOCKET){
        WSACleanup();
        _pNetwork->ga_strEnumerationStatus = "";
		if(_szIPPortBufferLocal != NULL) {
			delete[] _szIPPortBufferLocal;
		}
		_szIPPortBufferLocal = NULL;		
		return 0;
    }

    _sIPPort* pServerIP = (_sIPPort*)(_szIPPortBufferLocal);
    while(_iIPPortBufferLocalLen >= 6) {
        if(!strncmp((char *)pServerIP, "\\final\\", 7)) {
                break;
            }

        _sIPPort ip = *pServerIP;

        CTString strIP;
        strIP.PrintF("%d.%d.%d.%d", ip.bFourth, ip.bThird, ip.bSecond, ip.bFirst);

        sockaddr_in sinServer;
        sinServer.sin_family = AF_INET;
        sinServer.sin_addr.s_addr = inet_addr(strIP);
        sinServer.sin_port = ip.iPort;

        // insert server status request into container
        CServerRequest &sreq = ga_asrRequests.Push();
        sreq.sr_ulAddress = sinServer.sin_addr.s_addr;
        sreq.sr_iPort = sinServer.sin_port;
        sreq.sr_tmRequestTime = _pTimer->GetHighPrecisionTimer().GetMilliseconds();

        // send packet to server
        sendto(_sockudp,"\\status\\",8,0,
            (sockaddr *) &sinServer, sizeof(sinServer));

        sockaddr_in _sinClient;
        socklen_t _iClientLength = sizeof(_sinClient);

        fd_set readfds_udp;                         // declare a read set
        struct timeval timeout_udp;                 // declare a timeval for our timer
        int iRet = -1;

        FD_ZERO(&readfds_udp);                      // zero out the read set
        FD_SET(_sockudp, &readfds_udp);             // add socket to the read set
        timeout_udp.tv_sec = 0;                     // timeout = 0 seconds
        timeout_udp.tv_usec = 50000;                // timeout += 0.05 seconds
        int _iN = select(_sockudp + 1, &readfds_udp, NULL, NULL, &timeout_udp);
        if (_iN > 0) {
          /** do recvfrom stuff **/
          iRet =  recvfrom(_sockudp, _szBuffer, 2048, 0, (sockaddr*)&_sinClient, &_iClientLength);
          FD_CLR(_sockudp, &readfds_udp);
          if(iRet != -1 && iRet > 100 && iRet != SOCKET_ERROR) {
            // null terminate the buffer
            _szBuffer[iRet] = 0;
            char *sPch = NULL;
            #ifdef FIRST_ENCOUNTER  // First Encounter
            sPch = strstr(_szBuffer, "\\gamename\\serioussam\\");
            #else
            sPch = strstr(_szBuffer, "\\gamename\\serioussamse\\");
            #endif
            if(!sPch) {
                DateTime(_datetime);
                CPrintF("[%s] LocalNet_Thread: Unknown query server response!\n", _datetime);
				if(_szIPPortBufferLocal != NULL) {
					delete[] _szIPPortBufferLocal;
				}
				_szIPPortBufferLocal = NULL;               
				WSACleanup();
				return 0;
            } else {

                CTString strPlayers;
                CTString strMaxPlayers;
                CTString strLevel;
                CTString strGameType;
                CTString strVersion;
                CTString strGameName;
                CTString strSessionName;

                CTString strGamePort;
                CTString strServerLocation;
                CTString strGameMode;
                CTString strActiveMod;

                CHAR* pszPacket = _szBuffer + 1; // we do +1 because the first character is always '\', which we don't care about.

                BOOL bReadValue = FALSE;
                CTString strKey;
                CTString strValue;

                while(*pszPacket != 0) {
                switch(*pszPacket) {
                case '\\':
                    if(strKey != "gamemode") {
                      if(bReadValue) {
                        // we're done reading the value, check which key it was
                        if(strKey == "gamename") {
                            strGameName = strValue;
                        } else if(strKey == "gamever") {
                            strVersion = strValue;
                        } else if(strKey == "location") {
                            strServerLocation = strValue;
                        } else if(strKey == "hostname") {
                            strSessionName = strValue;
                        } else if(strKey == "hostport") {
                            strGamePort = strValue;
                        } else if(strKey == "mapname") {
                            strLevel = strValue;
                        } else if(strKey == "gametype") {
                            strGameType = strValue;
                        } else if(strKey == "activemod") {
                            strActiveMod = strValue;
                        } else if(strKey == "numplayers") {
                            strPlayers = strValue;
                        } else if(strKey == "maxplayers") {
                            strMaxPlayers = strValue;
                        } else {
                            //DateTime(_datetime);
							//CPrintF("[%s] Unknown GameAgent parameter key '%s'!",_datetime, strKey);
                        }
                        // reset temporary holders
                        strKey = "";
                        strValue = "";
                      }
                    }
                    bReadValue = !bReadValue;
                    break;

                default:
                    // read into the value or into the key, depending where we are
                    if(bReadValue) {
                        strValue.InsertChar(strlen(strValue), *pszPacket);
                    } else {
                        strKey.InsertChar(strlen(strKey), *pszPacket);
                    }
                    break;
                  }
                  // move to next character
                  pszPacket++;
                }

                // check if we still have a maxplayers to back up
                if(strKey == "gamemode") {
                    strGameMode = strValue;
                }
                if(strActiveMod != "") {
                    strGameName = strActiveMod;
                }

                long long tmPing = -1;
                // find the request in the request array
                for(INDEX i=0; i<ga_asrRequests.Count(); i++) {
                    CServerRequest &req = ga_asrRequests[i];
                    if(req.sr_ulAddress == _sinClient.sin_addr.s_addr && req.sr_iPort == _sinClient.sin_port) {
                        tmPing = _pTimer->GetHighPrecisionTimer().GetMilliseconds() - req.sr_tmRequestTime;
                        ga_asrRequests.Delete(&req);
                        break;
                    }
                }

                if(tmPing > 0 && tmPing < 2500000) {
				    // insert the server into the serverlist
                    _pNetwork->ga_strEnumerationStatus = "";
					CNetworkSession &ns = *new CNetworkSession;
                    _pNetwork->ga_lhEnumeratedSessions.AddTail(ns.ns_lnNode);
					
                    // add the server to the serverlist
                    ns.ns_strSession = strSessionName;
                    ns.ns_strAddress = inet_ntoa(_sinClient.sin_addr) + CTString(":") + CTString(0, "%d", htons(_sinClient.sin_port) - 1);
                    ns.ns_tmPing = (tmPing / 1000.0f);
                    ns.ns_strWorld = strLevel;
                    ns.ns_ctPlayers = atoi(strPlayers);
                    ns.ns_ctMaxPlayers = atoi(strMaxPlayers);
                    ns.ns_strGameType = strGameType;
                    ns.ns_strMod = strGameName;
                    ns.ns_strVer = strVersion;
                }
            }
          } else {
            // find the request in the request array
            for(INDEX i=0; i<ga_asrRequests.Count(); i++) {
              CServerRequest &req = ga_asrRequests[i];
              if(req.sr_ulAddress == _sinClient.sin_addr.s_addr && req.sr_iPort == _sinClient.sin_port) {
                ga_asrRequests.Delete(&req);
                break;
              }
            }
          }
        }
        pServerIP++;
        _iIPPortBufferLocalLen -= 6;
    }
	if(_szIPPortBufferLocal != NULL) {
      delete[] _szIPPortBufferLocal;
    }
	_szIPPortBufferLocal = NULL;
	
    closesocket(_sockudp);
    _uninitWinsock();
    _bInitialized = FALSE;
    _pNetwork->ga_bEnumerationChange = FALSE;
	_pNetwork->ga_strEnumerationStatus = "";
    WSACleanup();
    return 0;
}



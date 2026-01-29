#include "../plugin.h"


#define	MAX_INFO_STRING	196
#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768
#define MAX_CLIENTS 32


#define	A2C_PRINT			'n'	// print a message on client
#define M2C_MASTER_REPLY	'd'	// + \n + qw server port list


#define PORT_ANY -1

#ifdef _WIN32
#include "winsock.h"
#define EWOULDBLOCK		WSAEWOULDBLOCK
#define ECONNREFUSED	WSAECONNREFUSED
#define EADDRNOTAVAIL	WSAEADDRNOTAVAIL
#define EMSGSIZE		WSAEMSGSIZE
#define ECONNABORTED	WSAECONNABORTED
#define ECONNRESET		WSAECONNRESET

#define qerrno WSAGetLastError()
#else
#define qerrno errno

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
typedef int SOCKET;
#endif





#define SS_FTESERVER	1	//hehehe...
#define SS_QUAKE2		2	//useful (and cool). Could be blamed for swamping.
#define SS_NETQUAKE		4
#define SS_FAVORITE		8	//filter all others.
#define SS_KEEPINFO		16


//despite not supporting nq or q2, we still load them. We just filter them. This is to make sure we properly write the listing files.
#define MT_BAD			0	//this would be an error
#define MT_BCASTQW		1	//-1status
#define MT_BCASTQ2		2	//-1status
#define MT_BCASTNQ		3	//see code
#define MT_SINGLEQW		4	//-1status
#define MT_SINGLEQ2		5	//-1status
#define MT_SINGLENQ		6	//see code.
#define MT_MASTERQW		7	//c\n\0
#define MT_MASTERQ2		8	//query


//contains info about a server in greater detail. Could be too mem intensive.
typedef struct serverdetailedinfo_s {
	char info[MAX_SERVERINFO_STRING];

	int numplayers;

	struct {
		int userid;
		int frags;
		float time;
		int ping;
		char name[64];
		char skin[64];
		char topc;
		char botc;				
	} players[MAX_CLIENTS];
} serverdetailedinfo_t;

//hold minimum info.
typedef struct serverinfo_s {
	char name[64];	//hostname.
	struct sockaddr_in adr;

	short players;
	short maxplayers;

	short tl;
	short fl;
	char gamedir[8+1];
	char map[8+1];

	int refreshtime;	//msecs
	qbyte special;	//flags
	unsigned short ping;
	int sends;

	serverdetailedinfo_t *moreinfo;

	struct serverinfo_s *next;
} serverinfo_t;

typedef struct master_s{
	struct master_s *next;
	struct sockaddr_in adr;
	int type;
	char name[1];
} master_t;

struct {
	qboolean inuse;
	struct sockaddr_in adr;

	serverdetailedinfo_t *detail;

	int linenum;
} selectedserver;

typedef struct player_s {
	char name[16];
	int frags;
	int colour;
	char skin[8];
	struct sockaddr_in adr;

	struct player_s *next;
} player_t;

void SListOptionChanged(serverinfo_t *newserver);

extern serverinfo_t *firstserver;
extern master_t *master;
extern player_t *mplayers;

void CL_QueryServers(void);
int NET_CheckPollSockets(void);
serverinfo_t *Master_InfoForServer (struct sockaddr_in addr);
serverinfo_t *Master_InfoForNum (int num);
int Master_TotalCount(void);
void Master_QueryServer(serverinfo_t *server);
void MasterInfo_WriteServers(void);
void MasterInfo_Begin(void);


char *NET_AdrToString(const struct sockaddr_in *a);


#define Q_strncpyS(d, s, n) do{const char *____in=(s);char *____out=(d);int ____i; for (____i=0;*(____in); ____i++){if (____i == (n))break;*____out++ = *____in++;}if (____i < (n))*____out='\0';}while(0)	//only use this when it should be used. If undiciided, use N
#define Q_strncpyN(d, s, n) do{if (n < 0)Sys_Error("Bad length in strncpyz");Q_strncpyS((d), (s), (n));((char *)(d))[n] = '\0';}while(0)	//this'll stop me doing buffer overflows. (guarenteed to overflow if you tried the wrong size.)
#define Q_strncpyNCHECKSIZE(d, s, n) do{if (n < 1)Sys_Error("Bad length in strncpyz");Q_strncpyS((d), (s), (n));((char *)(d))[n-1] = '\0';((char *)(d))[n] = '255';}while(0)	//This forces nothing else to be within the buffer. Should be used for testing and nothing else.
#define Q_strncpyz(d, s, n) Q_strncpyN(d, s, (n)-1)

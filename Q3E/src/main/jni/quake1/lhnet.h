
// Written by Ashley Rose Hale (LadyHavoc) 2003-06-15 and placed into public domain.

#ifndef LHNET_H
#define LHNET_H

#include <stddef.h>
#include "com_list.h"

typedef enum lhnetaddresstype_e
{
	LHNETADDRESSTYPE_NONE,
	LHNETADDRESSTYPE_LOOP,
	LHNETADDRESSTYPE_INET4,
	LHNETADDRESSTYPE_INET6
}
lhnetaddresstype_t;

typedef struct lhnetaddress_s
{
	lhnetaddresstype_t addresstype;
	int port; // used by LHNETADDRESSTYPE_LOOP
	unsigned char storage[256]; // sockaddr_in or sockaddr_in6
}
lhnetaddress_t;

int LHNETADDRESS_FromPort(lhnetaddress_t *address, lhnetaddresstype_t addresstype, int port);
int LHNETADDRESS_FromString(lhnetaddress_t *address, const char *string, int defaultport);
/// Returns the number of bytes written to *string excluding the \0 terminator.
int LHNETADDRESS_ToString(const lhnetaddress_t *address, char *string, int stringbuffersize, int includeport);
static inline lhnetaddresstype_t LHNETADDRESS_GetAddressType(const lhnetaddress_t *address)
{
	if (address)
		return address->addresstype;
	else
		return LHNETADDRESSTYPE_NONE;
}
const char *LHNETADDRESS_GetInterfaceName(const lhnetaddress_t *address, char *ifname, size_t ifnamelength);
int LHNETADDRESS_GetPort(const lhnetaddress_t *address);
int LHNETADDRESS_SetPort(lhnetaddress_t *address, int port);
int LHNETADDRESS_Compare(const lhnetaddress_t *address1, const lhnetaddress_t *address2);

typedef struct lhnetsocket_s
{
	lhnetaddress_t address;
	int inetsocket;
	llist_t list;
}
lhnetsocket_t;
extern lhnetsocket_t lhnet_socketlist;

void LHNET_Init(void);
void LHNET_Shutdown(void);
int LHNET_DefaultDSCP(int dscp); // < 0: query; >= 0: set (returns previous value)
void LHNET_SleepUntilPacket_Microseconds(int microseconds);
lhnetsocket_t *LHNET_OpenSocket_Connectionless(lhnetaddress_t *address);
void LHNET_CloseSocket(lhnetsocket_t *lhnetsocket);
lhnetaddress_t *LHNET_AddressFromSocket(lhnetsocket_t *sock);
int LHNET_Read(lhnetsocket_t *lhnetsocket, void *content, int maxcontentlength, lhnetaddress_t *address);
int LHNET_Write(lhnetsocket_t *lhnetsocket, const void *content, int contentlength, const lhnetaddress_t *address);

#endif


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

#ifndef SE_INCL_CLIENTINTERFACE_H
#define SE_INCL_CLIENTINTERFACE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Network/CPacket.h>

class CClientInterface {
public:
  BOOL ci_bUsed;						// client unused
  CTString ci_strAddress;   // client address in human readable format
  CAddress ci_adrAddress;		// address this client is connected to (address, port, ID)
	BOOL ci_bClientLocal;     // set for local clients
	ULONG ci_ulSequence;			// sequence number for reliable packet confirmation
	
	CPacketBuffer ci_pbOutputBuffer;					// output buffer
	CPacketBuffer ci_pbWaitAckBuffer;					// buffer for reliable packets that need to be acknowledged
  CPacketBuffer ci_pbInputBuffer;						// input buffer
	CPacketBuffer ci_pbReliableInputBuffer;		// a buffer containing received reliable packets
	BOOL ci_bReliableComplete;								// does the reliable input buffer contain a complete reliable message?

  CClientInterface *ci_pciOther;			// other-side client - for local clients
	
	// interface:
  CClientInterface(void);
  ~CClientInterface(void);
  void Clear(void);

	// sets the client to be local and optionally connects to another local client
  void SetLocal(CClientInterface *ci_pciOther);

  // send a message through the interface
  void Send(const void *pvSend, SLONG slSize,BOOL bReliable);
  void SendTo(const void *pvSend, SLONG slSize,const CAddress adrAdress,BOOL bReliable);
  // receive a message from the interface
  BOOL Receive(void *pvReceive, SLONG &slSize,BOOL bReliable);
  BOOL ReceiveFrom(void *pvReceive, SLONG &slSize, CAddress *adrAdress,BOOL bReliable);
  BOOL Receive(CTStream &strmReceive,UBYTE bReliable);

	// exchanges packets beetween this socket and it's local partner
	// from output of this buffet to the input of the other and vice versa
	void ExchangeBuffers(void);

  // update socket buffers (transfer from input buffer to the reliable buffer...) - grouped acknowledges
  BOOL UpdateInputBuffers(void);

	// update socket buffers with per-packet acknowledge
	BOOL UpdateInputBuffersBroadcast(void);

  // update socket outgoing buffer (resends, timeouts...)
  BOOL UpdateOutputBuffers(void);

	// get a packet whose time has come from the output buffer, NULL if no such packet
	CPacket* GetPendingPacket(void);

	// reads the expected size of realiable message in the reliable input buffer
	SLONG GetExpectedReliableSize(void);
  // reads the current size of realiable message in the reliable input buffer
  SLONG GetCurrentReliableSize(void);

};

#endif

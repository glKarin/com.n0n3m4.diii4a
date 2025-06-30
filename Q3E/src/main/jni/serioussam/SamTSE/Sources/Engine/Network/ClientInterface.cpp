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

#include <Engine/Base/CTString.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/ErrorTable.h>
#include <Engine/Base/Lists.h>
#include <Engine/Base/Stream.h>
#include <Engine/Base/Translation.h>

#include <Engine/Network/ClientInterface.h>
#include <Engine/Network/CPacket.h>

#include <Engine/Base/ListIterator.inl>

// how many acknowledges can fit into one UDP packet
#define MAX_ACKS_PER_PACKET (MAX_UDP_BLOCK_SIZE/sizeof(ULONG))

extern FLOAT net_fDropPackets;
extern INDEX net_bReportPackets;

CClientInterface::CClientInterface(void)
{
 Clear();
};

CClientInterface::~CClientInterface(void)
{
 Clear();
};


void CClientInterface::Clear(void) 
{
	ci_bUsed = FALSE;

	ci_bReliableComplete = FALSE;
	ci_pbInputBuffer.Clear();
	ci_pbOutputBuffer.Clear();
	ci_pbReliableInputBuffer.Clear();
	ci_pbWaitAckBuffer.Clear();

	ci_adrAddress.Clear();
	ci_strAddress = "";
	
	ci_pciOther = NULL;
	ci_ulSequence = 0;
};

// mark the client interface as local for this computer
void CClientInterface::SetLocal(CClientInterface *pciOther)
{
  Clear();

	ci_bUsed = TRUE;

  ci_bClientLocal = TRUE;
  ci_pciOther = pciOther;
  if (pciOther!=NULL) {
    pciOther->ci_pciOther = this;
  }

	ci_adrAddress.Clear();
		
};

// send a message through this client interface - reliable messages are not limited in size
void CClientInterface::Send(const void *pvSend, SLONG slSize,BOOL bReliable)
{
	ASSERT (ci_bUsed == TRUE);
	ASSERT(pvSend != NULL && slSize>0);
	// unreliable messages must fit within one UDP packet
	ASSERT(bReliable != UDP_PACKET_UNRELIABLE || slSize < MAX_UDP_BLOCK_SIZE);

	UBYTE ubPacketReliable;
	UBYTE* pubData;
	SLONG slSizeToSend;
  SLONG slTransferSize;
	ULONG ulSequence;
	CPacket* ppaNewPacket;

	//if the message is reliable, make sure the first packet is marked as a head of the message
	if (bReliable) {
		ubPacketReliable = UDP_PACKET_RELIABLE | UDP_PACKET_RELIABLE_HEAD;
		if (slSize <= MAX_UDP_BLOCK_SIZE) {
			ubPacketReliable |= UDP_PACKET_RELIABLE_TAIL;
		}
	} else {
		ubPacketReliable = UDP_PACKET_UNRELIABLE;
	}

	pubData = (UBYTE*) pvSend;
	slSizeToSend = slSize;
  slTransferSize = slSizeToSend;
	
	
	// split large reliable messages into packets, and put them in the output buffer
	while (slSizeToSend>MAX_UDP_BLOCK_SIZE) {
		ppaNewPacket = new CPacket;

		// for each packet, increment the sequence (very important)
		ulSequence = (++ci_ulSequence);
		ppaNewPacket->WriteToPacket(pubData,MAX_UDP_BLOCK_SIZE,ubPacketReliable,ulSequence,ci_adrAddress.adr_uwID,slTransferSize);
		ppaNewPacket->pa_adrAddress.adr_ulAddress = ci_adrAddress.adr_ulAddress;
		ppaNewPacket->pa_adrAddress.adr_uwPort = ci_adrAddress.adr_uwPort;
		ppaNewPacket->pa_adrAddress.adr_uwID = ci_adrAddress.adr_uwID;
		ci_pbOutputBuffer.AppendPacket(*ppaNewPacket,TRUE);

		// turn off udp head flag, if exists (since we just put a packet in the output buffer, the next 
		// packet cannot be the head
		ubPacketReliable &= UDP_PACKET_RELIABLE;

		slSizeToSend -=  MAX_UDP_BLOCK_SIZE;
		pubData += MAX_UDP_BLOCK_SIZE;
	}

	// what remains is a tail of a reliable message, or an unreliable packet
	if (ubPacketReliable != UDP_PACKET_UNRELIABLE) {
		ubPacketReliable |= UDP_PACKET_RELIABLE_TAIL;
	}

	// so send it
	ppaNewPacket = new CPacket;

	ulSequence = (++ci_ulSequence);
	ppaNewPacket->WriteToPacket(pubData,slSizeToSend,ubPacketReliable,ulSequence,ci_adrAddress.adr_uwID,slTransferSize);
	ppaNewPacket->pa_adrAddress.adr_ulAddress = ci_adrAddress.adr_ulAddress;
	ppaNewPacket->pa_adrAddress.adr_uwPort = ci_adrAddress.adr_uwPort;
	ppaNewPacket->pa_adrAddress.adr_uwID = ci_adrAddress.adr_uwID;
	ci_pbOutputBuffer.AppendPacket(*ppaNewPacket,TRUE);

  
};

// send a message through this client interface, to the provided address
void CClientInterface::SendTo(const void *pvSend, SLONG slSize,const CAddress adrAdress,BOOL bReliable)
{
	ASSERT (ci_bUsed);
	ASSERT(pvSend != NULL && slSize>0);
	// unreliable packets must fit within one UDP packet
	ASSERT(bReliable != UDP_PACKET_UNRELIABLE || slSize < MAX_UDP_BLOCK_SIZE);

	UBYTE ubPacketReliable;
	UBYTE* pubData;
	SLONG slSizeToSend;
  SLONG slTransferSize;
	ULONG ulSequence;
	CPacket* ppaNewPacket;

	//if the message is reliable, make sure the first packet is marked as a head of the message
	if (bReliable) {
		ubPacketReliable = UDP_PACKET_RELIABLE | UDP_PACKET_RELIABLE_HEAD;
		if (slSize <= MAX_UDP_BLOCK_SIZE) {
			ubPacketReliable |= UDP_PACKET_RELIABLE_TAIL;
		}
	} else {
		ubPacketReliable = UDP_PACKET_UNRELIABLE;
	}

	pubData = (UBYTE*) pvSend;
	slSizeToSend = slSize;
  slTransferSize = slSizeToSend;
	

	// split large reliable messages into packets, and put them in the output buffer
	while (slSizeToSend>MAX_UDP_BLOCK_SIZE) {
		ppaNewPacket = new CPacket;

		// for each packet, increment the sequence (very important)
		ulSequence = (++ci_ulSequence);
		ppaNewPacket->WriteToPacket(pubData,MAX_UDP_BLOCK_SIZE,ubPacketReliable,ulSequence,adrAdress.adr_uwID,slTransferSize);
		ppaNewPacket->pa_adrAddress.adr_ulAddress = adrAdress.adr_ulAddress;
		ppaNewPacket->pa_adrAddress.adr_uwPort = adrAdress.adr_uwPort;
		ppaNewPacket->pa_adrAddress.adr_uwID = adrAdress.adr_uwID;
		ci_pbOutputBuffer.AppendPacket(*ppaNewPacket,TRUE);

		// turn off udp head flag, if exists (since we just put a packet in the output buffer, the next 
		// packet cannot be the head
		ubPacketReliable &= UDP_PACKET_RELIABLE;

		slSizeToSend -=  MAX_UDP_BLOCK_SIZE;
		pubData += MAX_UDP_BLOCK_SIZE;
	}

	// what remains is a tail of a reliable message, or an unreliable packet
	if (ubPacketReliable != UDP_PACKET_UNRELIABLE) {
		ubPacketReliable |= UDP_PACKET_RELIABLE_TAIL;
	}

	ppaNewPacket = new CPacket;

	ulSequence = (++ci_ulSequence);
	ppaNewPacket->WriteToPacket(pubData,slSizeToSend,ubPacketReliable,ulSequence,adrAdress.adr_uwID,slTransferSize);
	ppaNewPacket->pa_adrAddress.adr_ulAddress = adrAdress.adr_ulAddress;
	ppaNewPacket->pa_adrAddress.adr_uwPort = adrAdress.adr_uwPort;
	ppaNewPacket->pa_adrAddress.adr_uwID = adrAdress.adr_uwID;
	ci_pbOutputBuffer.AppendPacket(*ppaNewPacket,TRUE);
};


// receive a message through the interface, discard originating address
BOOL CClientInterface::Receive(void *pvReceive, SLONG &slSize,BOOL bReliable)
{
	ASSERT (slSize>0);
	ASSERT (pvReceive != NULL);

	// we'll use the other receive procedure, and tell it to ignore the address
	return ReceiveFrom(pvReceive,slSize,NULL,bReliable);
};

// receive a message through the interface, and fill in the originating address
BOOL CClientInterface::ReceiveFrom(void *pvReceive, SLONG &slSize, CAddress *padrAdress,BOOL bReliable)
{
	CPacket* ppaPacket;
	UBYTE* pubData = (UBYTE*) pvReceive;
	SLONG slDummySize;
	UBYTE ubReliable;

	// if a reliable message is requested
	if (bReliable) {
		// if there is no complete reliable message ready
		if (ci_pbReliableInputBuffer.CheckSequence(slDummySize) == FALSE) {
			return FALSE;
		// if the ready message is longer than the expected size
		} else if ( GetCurrentReliableSize() > slSize) {
			return FALSE;
		// if everything is ok, compose the message and kill the packets
		} else {
			// fill in the originating address (if necessary)
			if (padrAdress != NULL) {
				ppaPacket = ci_pbReliableInputBuffer.PeekFirstPacket();
				padrAdress->adr_ulAddress = ppaPacket->pa_adrAddress.adr_ulAddress;
				padrAdress->adr_uwPort = ppaPacket->pa_adrAddress.adr_uwPort;
				padrAdress->adr_uwID = ppaPacket->pa_adrAddress.adr_uwID;
			}

			slSize = 0;
			do {
				ppaPacket = ci_pbReliableInputBuffer.GetFirstPacket();
				ubReliable = ppaPacket->pa_ubReliable;
				slDummySize = ppaPacket->pa_slSize - MAX_HEADER_SIZE;
				ppaPacket->ReadFromPacket(pubData,slDummySize);
				pubData += slDummySize;
				slSize += slDummySize;
				delete ppaPacket;
			} while (!(ubReliable & UDP_PACKET_RELIABLE_TAIL));
			return TRUE;
		}
	// if an unreliable message is requested
	} else {
		// if there are no packets in the input buffer, return
		if (ci_pbInputBuffer.pb_ulNumOfPackets == 0) {
			return FALSE;
		}
		ppaPacket = ci_pbInputBuffer.PeekFirstPacket();
		// if the reliable buffer is not empty, nothing can be accepted from the input buffer
		// because it would be accepted out-of order (before earlier sequences have been read)
		if (ci_pbReliableInputBuffer.pb_ulNumOfPackets != 0) {
			return FALSE;
		// if the first packet in the input buffer is not unreliable
		} else if (ppaPacket->pa_ubReliable != UDP_PACKET_UNRELIABLE) {
			return FALSE;
		// if the ready message is longer than the expected size
		} else if ( ppaPacket->pa_slTransferSize > slSize) {
			return FALSE;
		// if everything is ok, read the packet data, and kill the packet
		} else {
			// fill in the originating address (if necessary)
			if (padrAdress != NULL) {
				padrAdress->adr_ulAddress = ppaPacket->pa_adrAddress.adr_ulAddress;
				padrAdress->adr_uwPort = ppaPacket->pa_adrAddress.adr_uwPort;
				padrAdress->adr_uwID = ppaPacket->pa_adrAddress.adr_uwID;
			}
			slSize = ppaPacket->pa_slSize - MAX_HEADER_SIZE;
			ppaPacket->ReadFromPacket(pubData,slSize);
			// remove the packet from the buffer, and delete it from memory
			ci_pbInputBuffer.RemoveFirstPacket(TRUE); 
			return TRUE;
		}
		 
	}

	return FALSE;
};


// receive a message through the interface, discard originating address
BOOL CClientInterface::Receive(CTStream &strmReceive,UBYTE bReliable)
{
	CPacket* ppaPacket;
	UBYTE ubReliable;
  SLONG slDummySize;

	// if a reliable message is requested
	if (bReliable) {
		// if there is no complete reliable message ready
		if (ci_pbReliableInputBuffer.CheckSequence(slDummySize) == FALSE) {
			return FALSE;	
		// if everything is ok, compose the message and kill the packets
		} else {
			do {
				ppaPacket = ci_pbReliableInputBuffer.GetFirstPacket();
				ubReliable = ppaPacket->pa_ubReliable;
				strmReceive.Write_t(ppaPacket->pa_pubPacketData + MAX_HEADER_SIZE,ppaPacket->pa_slSize - MAX_HEADER_SIZE);
        if (ci_pbInputBuffer.pb_ulLastSequenceOut < ppaPacket->pa_ulSequence) {
          ci_pbInputBuffer.pb_ulLastSequenceOut = ppaPacket->pa_ulSequence;
        }
				delete ppaPacket;
			} while (!(ubReliable & UDP_PACKET_RELIABLE_TAIL));
			return TRUE;
		}
	// if an unreliable message is requested
	} else {
		ppaPacket = ci_pbInputBuffer.PeekFirstPacket();
		// if the reliable buffer is not empty, nothing can be accepted from the input buffer
		// because it would be accepted out-of order (before earlier sequences have been read)
		if (ci_pbReliableInputBuffer.pb_ulNumOfPackets != 0) {
			return FALSE;
		// if the first packet in the input buffer is not unreliable
		} else if (ppaPacket->pa_ubReliable != UDP_PACKET_RELIABLE) {
			return FALSE;
		// if everything is ok, read the packet data, and kill the packet
		} else {
			strmReceive.Write_t(ppaPacket->pa_pubPacketData + MAX_HEADER_SIZE,ppaPacket->pa_slSize - MAX_HEADER_SIZE);
			// remove the packet from the buffer, and delete it from memory
      if (ci_pbInputBuffer.pb_ulLastSequenceOut < ppaPacket->pa_ulSequence) {
        ci_pbInputBuffer.pb_ulLastSequenceOut = ppaPacket->pa_ulSequence;
      }
			ci_pbInputBuffer.RemoveFirstPacket(TRUE); 			
			return TRUE;
		}
		 
	}

	return FALSE;
};


// exchanges packets beetween this socket and it's local partner
// from output of this buffet to the input of the other and vice versa
void CClientInterface::ExchangeBuffers(void)
{
	ASSERT (ci_pciOther != NULL);
	CPacket* ppaPacket;
	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

	// take the output from this interface and give it to it's partner socket
	while (ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
		ppaPacket = ci_pbOutputBuffer.PeekFirstPacket();
		if (ppaPacket->pa_tvSendWhen < tvNow) {
			ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
			if (ci_pciOther->ci_pbInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
				delete ppaPacket;
			}
		} else {
			break;
		}
	}

	// and the other way around
	while (ci_pciOther->ci_pbOutputBuffer.pb_ulNumOfPackets > 0) {
		ppaPacket = ci_pciOther->ci_pbOutputBuffer.PeekFirstPacket();
		if (ppaPacket->pa_tvSendWhen < tvNow) {
			ppaPacket = ci_pciOther->ci_pbOutputBuffer.GetFirstPacket();
			if (ci_pbInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
				delete ppaPacket;
			};

		}	else {
			break;
		}
	}

};

#define SLASHSLASH  0x2F2F   // looks like "//" in ASCII.

// update interface's input buffer (transfer from input buffer to the reliable buffer...),
// for incoming acknowledge packets, remove acknowledged packets from the output buffers,
// and generate acknowledge messages for incoming reliable packets
BOOL CClientInterface::UpdateInputBuffers(void)
{
	//BOOL bSomethingDone;
	ULONG pulGenAck[MAX_ACKS_PER_PACKET];
	ULONG ulAckCount=0;
	CTimerValue tvNow;
		
	// if there are packets in the input buffer, process them
	FORDELETELIST(CPacket,pa_lnListNode,ci_pbInputBuffer.pb_lhPacketStorage,ppaPacket) {
		//CPacket &paPacket = *ppaPacket;
		
			// if it's an acknowledge packet, remove the acknowledged packets from the wait acknowledge buffer
			if (ppaPacket->pa_ubReliable & UDP_PACKET_ACKNOWLEDGE) {
				ULONG *pulAck;
				SLONG slSize;
				ULONG ulSequence;

				slSize = ppaPacket->pa_slSize - MAX_HEADER_SIZE;
				// if slSize isn't rounded to the size of ulSequence, abort 
				ASSERT (slSize % sizeof(ULONG) == 0);

				// get the pointer to the start of acknowledged sequences
				pulAck = (ULONG*) (ppaPacket->pa_pubPacketData + MAX_HEADER_SIZE);
				// for each acknowledged sequence number
				while (slSize>0) {
					ulSequence = *pulAck;
					
					// report the packet info to the console
					if (net_bReportPackets == TRUE) {
						tvNow = _pTimer->GetHighPrecisionTimer();
						CPrintF("%lu: Received acknowledge for packet sequence %d\n",(ULONG) tvNow.GetMilliseconds(),ulSequence);
					}
					
					// remove the matching packet from the wait acknowledge buffer
					ci_pbWaitAckBuffer.RemovePacket(ulSequence,TRUE);
					// if the packet is waiting to be resent it's in the outgoing buffer, so remove it
					ci_pbOutputBuffer.RemovePacket(ulSequence,TRUE);
					pulAck++;
					slSize -= sizeof(ULONG);
				}

				// take this packet out of the input buffer and kill it
				ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
				delete ppaPacket;
				
				//bSomethingDone = TRUE;
			// if the packet is reliable
			} else if (ppaPacket->pa_ubReliable & UDP_PACKET_RELIABLE) {
				
				// generate packet acknowledge
				// if the packet is from the broadcast address, send the acknowledge for that packet only
				if (ppaPacket->pa_adrAddress.adr_uwID == SLASHSLASH || ppaPacket->pa_adrAddress.adr_uwID == 0) {
					CPacket *ppaAckPacket = new CPacket;
					ppaAckPacket->pa_adrAddress.adr_ulAddress = ppaPacket->pa_adrAddress.adr_ulAddress;
					ppaAckPacket->pa_adrAddress.adr_uwPort = ppaPacket->pa_adrAddress.adr_uwPort;
					ppaAckPacket->WriteToPacket(&(ppaPacket->pa_ulSequence),sizeof(ULONG),UDP_PACKET_ACKNOWLEDGE,++ci_ulSequence,SLASHSLASH,sizeof(ULONG));
					ci_pbOutputBuffer.AppendPacket(*ppaAckPacket,TRUE);
					if (net_bReportPackets == TRUE) {
						CPrintF("Acknowledging broadcast packet sequence %d\n",ppaPacket->pa_ulSequence);
					}
					ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
				}	else {
					// if we have filled the packet to the maximum with acknowledges (an extremely rare event)
					// finish this packet and start the next one
					if (ulAckCount == MAX_ACKS_PER_PACKET) {
						CPacket *ppaAckPacket = new CPacket;
						ppaAckPacket->pa_adrAddress.adr_ulAddress = ci_adrAddress.adr_ulAddress;
						ppaAckPacket->pa_adrAddress.adr_uwPort = ci_adrAddress.adr_uwPort;
						ppaAckPacket->WriteToPacket(pulGenAck,ulAckCount*sizeof(ULONG),UDP_PACKET_ACKNOWLEDGE,++ci_ulSequence,ci_adrAddress.adr_uwID,ulAckCount*sizeof(ULONG));
						ci_pbOutputBuffer.AppendPacket(*ppaAckPacket,TRUE);
						ulAckCount = 0;
					}	
					// add the acknowledge for this packet
					pulGenAck[ulAckCount] = ppaPacket->pa_ulSequence;

					// report the packet info to the console
					if (net_bReportPackets == TRUE) {
						tvNow = _pTimer->GetHighPrecisionTimer();
						CPrintF("%lu: Acknowledging packet sequence %d\n",(ULONG) tvNow.GetMilliseconds(),ppaPacket->pa_ulSequence);
					}

					ulAckCount++;
				}
			
				// take this packet out of the input buffer
				ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);		

        if (ppaPacket->pa_ulSequence == 8) {
          ppaPacket->pa_ulSequence = 8;
        }
				// a packet can be accepted from the broadcast ID only if it is an acknowledge packet or 
				// if it is a connection confirmation response packet and the client isn't already connected
				if (ppaPacket->pa_adrAddress.adr_uwID == SLASHSLASH || ppaPacket->pa_adrAddress.adr_uwID == 0) {
					if  (((!ci_bUsed) && (ppaPacket->pa_ubReliable & UDP_PACKET_CONNECT_RESPONSE)) ||
						 (ppaPacket->pa_ubReliable & UDP_PACKET_ACKNOWLEDGE) || ci_bClientLocal) {

           /* if (ci_pbReliableInputBuffer.pb_ulLastSequenceOut >= ppaPacket->pa_ulSequence) {
              delete ppaPacket;
            } else*/ 
            ppaPacket->pa_ulSequence = 0;
            if (ci_pbReliableInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
							delete ppaPacket;
						}
					}	else {
						delete ppaPacket;
					}
					// reject duplicates
				} else if (ppaPacket->pa_ulSequence > ci_pbReliableInputBuffer.pb_ulLastSequenceOut &&
					!(ci_pbReliableInputBuffer.IsSequenceInBuffer(ppaPacket->pa_ulSequence))) {
					if (ci_pbReliableInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
						delete ppaPacket;
					}				
				} else {
					delete ppaPacket;
				}

			// if the packet is unreliable, leave it in the input buffer
			// when it is needed, the message will be pulled from there
			} else {
	
			// reject duplicates
			ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
			if (ppaPacket->pa_ulSequence > ci_pbInputBuffer.pb_ulLastSequenceOut &&
					!(ci_pbReliableInputBuffer.IsSequenceInBuffer(ppaPacket->pa_ulSequence))) {						
				if (ci_pbInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
					delete ppaPacket;
				}				
			} else {
				delete ppaPacket;
			}

		}
	}

	// if there are any remaining unsent acknowldges, put them into a packet and send it
	if (ulAckCount >0) {
		CPacket *ppaAckPacket = new CPacket;
		ppaAckPacket->pa_adrAddress.adr_ulAddress = ci_adrAddress.adr_ulAddress;
		ppaAckPacket->pa_adrAddress.adr_uwPort = ci_adrAddress.adr_uwPort;
		ppaAckPacket->WriteToPacket(pulGenAck,ulAckCount*sizeof(ULONG),UDP_PACKET_ACKNOWLEDGE,++ci_ulSequence,ci_adrAddress.adr_uwID,ulAckCount*sizeof(ULONG));
		ci_pbOutputBuffer.AppendPacket(*ppaAckPacket,TRUE);
	}	
	return TRUE;

};


// update socket input buffer (transfer from input buffer to the reliable buffer...),
// for incoming acknowledge packets, remove acknowledged packets from the output buffers,
// and generate acknowledge messages for incoming reliable packets
// this method is different than the previous becoause it sends acknowledges for each 
// packet separately, instead of grouping them together
BOOL CClientInterface::UpdateInputBuffersBroadcast(void)
{
	//BOOL bSomethingDone;
	CTimerValue tvNow;
	
	// if there are packets in the input buffer, process them
	FORDELETELIST(CPacket,pa_lnListNode,ci_pbInputBuffer.pb_lhPacketStorage,ppaPacket) {
		//CPacket &paPacket = *ppaPacket;

			// if it's an acknowledge packet, remove the acknowledged packets from the wait acknowledge buffer
			if (ppaPacket->pa_ubReliable & UDP_PACKET_ACKNOWLEDGE) {
				ULONG *pulAck;
				SLONG slSize;
				ULONG ulSequence;

				slSize = ppaPacket->pa_slSize - MAX_HEADER_SIZE;
				// if slSize isn't rounded to the size of ulSequence, abort 
				ASSERT (slSize % sizeof(ULONG) == 0);

				// get the pointer to the start of acknowledged sequences
				pulAck = (ULONG*) (ppaPacket->pa_pubPacketData + MAX_HEADER_SIZE);
				// for each acknowledged sequence number
				while (slSize>0) {
					ulSequence = *pulAck;

					// report the packet info to the console
					if (net_bReportPackets == TRUE) {
						tvNow = _pTimer->GetHighPrecisionTimer();
						CPrintF("%lu: Received acknowledge for broadcast packet sequence %d\n",(ULONG) tvNow.GetMilliseconds(),ulSequence);
					}

					// remove the matching packet from the wait acknowledge buffer
					ci_pbWaitAckBuffer.RemovePacket(ulSequence,TRUE);
					// if the packet is waiting to be resent it's in the outgoing buffer, so remove it
					ci_pbOutputBuffer.RemovePacket(ulSequence,TRUE);
					pulAck++;
					slSize -= sizeof(ULONG);
				}

				ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
				//bSomethingDone = TRUE;
				delete ppaPacket;
			// if the packet is reliable
			} else if (ppaPacket->pa_ubReliable & UDP_PACKET_RELIABLE) {
				
				// generate packet acknowledge (each reliable broadcast packet is acknowledged separately 
				// because the broadcast interface can receive packets from any number of different addresses
				CPacket *ppaAckPacket = new CPacket;
				ppaAckPacket->pa_adrAddress.adr_ulAddress = ppaPacket->pa_adrAddress.adr_ulAddress;
				ppaAckPacket->pa_adrAddress.adr_uwPort = ppaPacket->pa_adrAddress.adr_uwPort;
				ppaAckPacket->WriteToPacket(&(ppaPacket->pa_ulSequence),sizeof(ULONG),UDP_PACKET_ACKNOWLEDGE,ci_ulSequence++,ppaPacket->pa_adrAddress.adr_uwID,sizeof(ULONG));
				ci_pbOutputBuffer.AppendPacket(*ppaAckPacket,TRUE);

				// report the packet info to the console
				if (net_bReportPackets == TRUE) {
					tvNow = _pTimer->GetHighPrecisionTimer();
					CPrintF("%lu: Acknowledging broadcast packet sequence %d\n",(ULONG) tvNow.GetMilliseconds(),ppaPacket->pa_ulSequence);
				}

				ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
				if (ci_pbReliableInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
					delete ppaPacket;
				}				
			} else {
	
			// if the packet is unreliable, leave it in the input buffer
			// when it is needed, the message will be pulled from there
			// have to check for duplicates
	
			ci_pbInputBuffer.RemovePacket(ppaPacket->pa_ulSequence,FALSE);
			if (ppaPacket->pa_ulSequence > ci_pbInputBuffer.pb_ulLastSequenceOut &&
					!(ci_pbReliableInputBuffer.IsSequenceInBuffer(ppaPacket->pa_ulSequence))) {						
				if (ci_pbInputBuffer.InsertPacket(*ppaPacket,FALSE) == FALSE) {
					delete ppaPacket;
				}
			} else {
				delete ppaPacket;
			}

		}
	}

	return TRUE;
							
};


// take a look at the wait acknowledge buffer and resend any packets that heve reached the timeout
// if there is a packet that can't be sent sucessfully (RS_NOTATALL), signal it
BOOL CClientInterface::UpdateOutputBuffers(void)
{
	CPacket* ppaPacket;
	UBYTE ubRetry;

	// handle resends
	while (ci_pbWaitAckBuffer.pb_ulNumOfPackets > 0) {
		ppaPacket = ci_pbWaitAckBuffer.PeekFirstPacket();
		
		ubRetry = ppaPacket->CanRetry();
		switch (ubRetry) {
			// if it's time to retry sending the packet
			case RS_NOW: {	ci_pbWaitAckBuffer.RemoveFirstPacket(FALSE);
											ci_pbOutputBuffer.Retry(*ppaPacket);
											break;
									 }
			// if the packet cannot be sent now, no other packets can be sent, so exit
			case RS_NOTNOW: { return TRUE; }
			// if the packet has reached the retry limit - close the client's connection
			case RS_NOTATALL: { Clear(); 
													return FALSE;
												}
			

		}
	}
	return TRUE;
};


// get the next available packet from the output buffer
CPacket* CClientInterface::GetPendingPacket(void)
{
	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

	if (ci_pbOutputBuffer.pb_ulNumOfPackets == 0) {
		return NULL;
	}

	CPacket* ppaPacket = ci_pbOutputBuffer.PeekFirstPacket();
	
	// if it's time to send the packet
	if (ppaPacket->pa_tvSendWhen <= tvNow) {
		ci_pbOutputBuffer.RemoveFirstPacket(FALSE);
		return ppaPacket;
	}

	return NULL;

};


// reads the expected size of current realiable message in the reliable input buffer
SLONG CClientInterface::GetExpectedReliableSize(void)
{
  if (ci_pbReliableInputBuffer.pb_ulNumOfPackets == 0) {
    return 0;
  }
  CPacket* ppaPacket = ci_pbReliableInputBuffer.PeekFirstPacket();
	return ppaPacket->pa_slTransferSize;
};

// reads the expected size of current realiable message in the reliable input buffer
SLONG CClientInterface::GetCurrentReliableSize(void)
{
  SLONG slSize;
  ci_pbReliableInputBuffer.CheckSequence(slSize);
	return slSize;
};

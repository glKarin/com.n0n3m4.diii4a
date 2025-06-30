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

#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Math/Functions.h>
#include <Engine/Base/Lists.h>
#include <Engine/Base/Memory.h>
#include <Engine/Network/Packet.h>

#include <Engine/Base/Listiterator.inl>

// should the packet transfers in/out of the buffer be reported to the console
extern INDEX net_bReportPackets;
extern INDEX net_iMaxSendRetries;
extern FLOAT net_fSendRetryWait;

#define MAX_RETRIES 10
#define RETRY_INTERVAL 3.0f

// make the address broadcast
void CAddress::MakeBroadcast(void)
{
  adr_ulAddress = INADDR_BROADCAST;
  extern INDEX net_iPort;
  adr_uwPort = net_iPort;
  adr_uwID = 0;
}



/*
*
*	CPacket class implementation
*
*/

// copy constructor
CPacket::CPacket(CPacket &paOriginal) 
{

	ASSERT(paOriginal.pa_pubPacketData != NULL && paOriginal.pa_slSize > 0);

	pa_slSize = paOriginal.pa_slSize;
  pa_slTransferSize = paOriginal.pa_slTransferSize;

	pa_ulSequence = paOriginal.pa_ulSequence;
	pa_ubReliable = paOriginal.pa_ubReliable;
	pa_tvSendWhen = paOriginal.pa_tvSendWhen;
	pa_ubRetryNumber = paOriginal.pa_ubRetryNumber;
	pa_adrAddress.adr_ulAddress = paOriginal.pa_adrAddress.adr_ulAddress;
	pa_adrAddress.adr_uwPort = paOriginal.pa_adrAddress.adr_uwPort;
	pa_adrAddress.adr_uwID = paOriginal.pa_adrAddress.adr_uwID;

	memcpy(pa_pubPacketData,paOriginal.pa_pubPacketData,pa_slSize);

};

// initialization of the packet - clear all data and remove the packet from any list (buffer) it is in
void CPacket::Clear() 
{

	pa_slSize = 0;
  pa_slTransferSize = 0;
	pa_ubReliable = UDP_PACKET_UNRELIABLE;
	pa_ubRetryNumber = 0;

	pa_tvSendWhen = CTimerValue(0.0f);
	if(pa_lnListNode.IsLinked()) pa_lnListNode.Remove();

};

void CPacket::operator=(const CPacket &paOriginal)
{
	ASSERT(paOriginal.pa_pubPacketData != NULL && paOriginal.pa_slSize > 0);

	pa_slSize = paOriginal.pa_slSize;
  pa_slTransferSize = paOriginal.pa_slTransferSize;

	pa_ulSequence = paOriginal.pa_ulSequence;
	pa_ubReliable = paOriginal.pa_ubReliable;
	pa_tvSendWhen = paOriginal.pa_tvSendWhen;
	pa_ubRetryNumber = paOriginal.pa_ubRetryNumber;
	pa_adrAddress.adr_ulAddress = paOriginal.pa_adrAddress.adr_ulAddress;
	pa_adrAddress.adr_uwPort = paOriginal.pa_adrAddress.adr_uwPort;
	pa_adrAddress.adr_uwID = paOriginal.pa_adrAddress.adr_uwID;

	memcpy(pa_pubPacketData,paOriginal.pa_pubPacketData,pa_slSize);

};


// Takes data from a pointer, adds the packet header and copies the data to the packet
BOOL CPacket::WriteToPacket(void* pv,SLONG slSize,UBYTE ubReliable,ULONG ulSequence,UWORD uwClientID,SLONG slTransferSize) 
{
	UBYTE* pubData;

	ASSERT(slSize <= MAX_UDP_BLOCK_SIZE && slSize > 0);
	ASSERT(pv != NULL);
  ASSERT(slTransferSize >= slSize);

	// set packet properties to values received as parameters
	pa_ubReliable = ubReliable;
	pa_adrAddress.adr_uwID = uwClientID;
	pa_ulSequence = ulSequence;
	pa_slSize = slSize + MAX_HEADER_SIZE;
  pa_slTransferSize = slTransferSize;

	// insert packet header to the beginning of the packet data
	pubData = pa_pubPacketData;
	*pubData = pa_ubReliable;
	pubData++;

	*(ULONG*)pubData = pa_ulSequence;
	pubData+=sizeof(pa_ulSequence);

	*(UWORD*)pubData = pa_adrAddress.adr_uwID;
	pubData+=sizeof(pa_adrAddress.adr_uwID);
  
  *(SLONG*)pubData = pa_slTransferSize;
	pubData+=sizeof(pa_slTransferSize);

	// copy the data the packet is to contain
	memcpy(pubData,pv,slSize);

	

	return TRUE;

};


// Takes data from a pointer, reads the packet header and copies the data to the packet
BOOL CPacket::WriteToPacketRaw(void* pv,SLONG slSize) 
{
	UBYTE* pubData;

	ASSERT(slSize <= MAX_PACKET_SIZE && slSize > 0);
	ASSERT(pv != NULL);

	// get the packet properties from the pointer, and set the values
	pubData = (UBYTE*)pv;
	pa_ubReliable = *pubData;
	pubData++;
	pa_ulSequence = *(ULONG*)pubData;
	pubData+=sizeof(pa_ulSequence);
  pa_adrAddress.adr_uwID = *(UWORD*)pubData;
	pubData+=sizeof(pa_adrAddress.adr_uwID);
  pa_slTransferSize = *(SLONG*)pubData;
	pubData+=sizeof(pa_slTransferSize);

	pa_slSize = slSize;

	// transfer the data to the packet
	memcpy(pa_pubPacketData,pv,pa_slSize);

	return TRUE;

};


// Copies the data from the packet to the location specified by the *pv. 
// packet header data is skipped
BOOL CPacket::ReadFromPacket(void* pv,SLONG &slExpectedSize) 
{
	UBYTE* pubData;

	ASSERT(slExpectedSize > 0);
	ASSERT(pv != NULL);
	ASSERT(pa_pubPacketData != NULL);

	if (slExpectedSize < (pa_slSize - MAX_HEADER_SIZE)) {
		return FALSE;
	}
	
	// how much data is actually returned
	slExpectedSize = pa_slSize - MAX_HEADER_SIZE;

	// skip the header data
	pubData = pa_pubPacketData + sizeof(pa_ubReliable) + sizeof(pa_ulSequence) + sizeof(pa_adrAddress.adr_uwID) + sizeof(pa_slTransferSize);
	
	memcpy(pv,pubData,slExpectedSize);

	return TRUE;

};

// is the packet reliable?
BOOL CPacket::IsReliable() 
{
	return pa_ubReliable;
};

// is the packet a head of a reliable stream
BOOL CPacket::IsReliableHead() 
{
	return pa_ubReliable & UDP_PACKET_RELIABLE_HEAD;
};

// is the packet a tail of a reliable stream
BOOL CPacket::IsReliableTail() 
{
	return pa_ubReliable & UDP_PACKET_RELIABLE_TAIL;
};

// return the sequence of a packet
ULONG CPacket::GetSequence()
{
	ASSERT(pa_ubReliable);
	return pa_ulSequence;
};

// return the retry status of a packet - can retry now, later or not at all 
UBYTE CPacket::CanRetry()
{

	if (pa_ubRetryNumber >= net_iMaxSendRetries) {
		return RS_NOTATALL;
	}

	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

	if (tvNow < (pa_tvSendWhen + CTimerValue(net_fSendRetryWait * (pa_ubRetryNumber + 1)))) {
		return RS_NOTNOW;
	}

	return RS_NOW;

};

//drop the packet from a list (buffer)
void CPacket::Drop()
{
	if (pa_lnListNode.IsLinked()) {
		pa_lnListNode.Remove();
	}
};

SLONG CPacket::GetTransferSize() 
{
  return pa_slTransferSize;
};

BOOL CPacket::IsBroadcast() 
{
  if (pa_adrAddress.adr_uwID == '//' || pa_adrAddress.adr_uwID == 0) {
    return TRUE;
  }

  return FALSE;

};


/*
*
*
*	CPacketBufferStats Class implementation
*
*
*/

// this class is used for MaxBPS limitation (prevets flooding the client) and for bandwidth limit and latency emulation
void CPacketBufferStats::Clear(void)
{

	pbs_fBandwidthLimit = 0;
	pbs_fLatencyLimit = 0.0f;
	pbs_fLatencyVariation = 0.0f;
	pbs_tvTimeNextPacketStart = _pTimer->GetHighPrecisionTimer();

};

// when can a certian ammount of data be sent?
CTimerValue CPacketBufferStats::GetPacketSendTime(SLONG slSize) 
{
	CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();

  // calculate how much should the packet be delayed due to latency and due to bandwidth
  CTimerValue tvBandwidth;
  if (pbs_fBandwidthLimit<=0.0f) {
    tvBandwidth = CTimerValue(0.0);
  } else {
    tvBandwidth = CTimerValue(DOUBLE((slSize*8)/pbs_fBandwidthLimit));
  }
  CTimerValue tvLatency;
  if (pbs_fLatencyLimit<=0.0f && pbs_fLatencyVariation<=0.0f) {
   tvLatency = CTimerValue(0.0);
  } else {
   tvLatency = CTimerValue(DOUBLE(pbs_fLatencyLimit+(pbs_fLatencyVariation*rand())/(float)(RAND_MAX)));
  }

  // time when the packet should be sent is max of
  CTimerValue tvStart(
    Max(
      // current time plus latency and
      (tvNow+tvLatency).tv_llValue,
      // next free point in time
      pbs_tvTimeNextPacketStart.tv_llValue));
  // remember next free time and return it
  pbs_tvTimeNextPacketStart = tvStart+tvBandwidth;
  return pbs_tvTimeNextPacketStart;

};




/*
*
*	CPacketBuffer Class implementation
*
*/


// Empty the packet buffer
void CPacketBuffer::Clear() 
{

	pb_lhPacketStorage.Clear();
	
	pb_ulNumOfPackets = 0;
	pb_ulNumOfReliablePackets = 0;
	pb_ulTotalSize = 0;
	pb_ulLastSequenceOut = 0;

	pb_pbsLimits.Clear();

};


// Is the packet buffer empty?
BOOL CPacketBuffer::IsEmpty() 
{

	if (pb_ulNumOfPackets>0) {
		return FALSE;
	}

	return TRUE;
};


// Calculate when the packet can be output from the buffer
CTimerValue CPacketBuffer::GetPacketSendTime(SLONG slSize) {
	CTimerValue tvSendTime;
	
	// if traffic emulation is in use, use the time with the lower bandwidth limit
	if (pb_ppbsStats != NULL) {
		if (pb_ppbsStats->pbs_fBandwidthLimit > 0.0f && pb_ppbsStats->pbs_fBandwidthLimit < pb_pbsLimits.pbs_fBandwidthLimit) {
			tvSendTime = pb_ppbsStats->GetPacketSendTime(slSize);
			pb_pbsLimits.pbs_tvTimeNextPacketStart = tvSendTime;
		} else {
			tvSendTime = pb_pbsLimits.GetPacketSendTime(slSize);
			pb_ppbsStats->pbs_tvTimeNextPacketStart = tvSendTime;
		}
	// else just use the MaxBPS control
	} else {
		tvSendTime = pb_pbsLimits.GetPacketSendTime(slSize);
	}

	return tvSendTime;
};

// Adds the packet to the end of the list
BOOL CPacketBuffer::AppendPacket(CPacket &paPacket,BOOL bDelay) 
{

	// bDelay regulates if the packet should be delayed because of the bandwidth limits or not
	// internal buffers (reliable, waitack and master buffers) do not pay attention to bandwidth limits
	if (bDelay) {
		paPacket.pa_tvSendWhen = GetPacketSendTime(paPacket.pa_slSize);
	} else {
		paPacket.pa_tvSendWhen = _pTimer->GetHighPrecisionTimer();
	}

	// Add the packet to the end of the list
	pb_lhPacketStorage.AddTail(paPacket.pa_lnListNode);
	pb_ulNumOfPackets++;

	// if the packet is reliable, bump up the number of reliable packets
	if (paPacket.pa_ubReliable & UDP_PACKET_RELIABLE) {
		pb_ulNumOfReliablePackets++;
	}

	// update the total size of data stored in the buffer
	pb_ulTotalSize += paPacket.pa_slSize - MAX_HEADER_SIZE;
	return TRUE;

};

// Inserts the packet in the buffer, according to it's sequence number
BOOL CPacketBuffer::InsertPacket(CPacket &paPacket,BOOL bDelay) 
{
	
	// find the right place to insert this packet (this is if this packet is out of sequence)
	FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		// if there is a packet in the buffer with greater sequence number, insert this one before it
		if (paPacket.pa_ulSequence < litPacketIter->pa_ulSequence) {

			// bDelay regulates if the packet should be delayed because of the bandwidth limits or not
			// internal buffers (reliable, waitack and master buffers) do not pay attention to bandwidth limits
			if (bDelay) {
				paPacket.pa_tvSendWhen = GetPacketSendTime(paPacket.pa_slSize);
			} else {
				paPacket.pa_tvSendWhen = _pTimer->GetHighPrecisionTimer();
			}

			litPacketIter.InsertBeforeCurrent(paPacket.pa_lnListNode);
			pb_ulNumOfPackets++;

			// if the packet is reliable, bump up the number of reliable packets
			if (paPacket.pa_ubReliable & UDP_PACKET_RELIABLE) {
				pb_ulNumOfReliablePackets++;
			}

			// update the total size of data stored in the buffer
			pb_ulTotalSize += paPacket.pa_slSize - MAX_HEADER_SIZE;

      
			return TRUE;

		// if there already is a packet in the buffer with the same sequence, do nothing
		} else if (paPacket.pa_ulSequence == litPacketIter->pa_ulSequence) {
			return FALSE;
		}
	}

	// if this packet has the greatest sequence number so far, add it to the end of the list
	pb_lhPacketStorage.AddTail(paPacket.pa_lnListNode);
	pb_ulNumOfPackets++;

  
	// if the packet is reliable, bump up the number of reliable packets
	if (paPacket.pa_ubReliable & UDP_PACKET_RELIABLE) {
		pb_ulNumOfReliablePackets++;
	}

	pb_ulTotalSize += paPacket.pa_slSize - MAX_HEADER_SIZE;

	return TRUE;
 
};


// Bumps up the retry count and time, and appends the packet to the buffer
BOOL CPacketBuffer::Retry(CPacket &paPacket) 
{
	paPacket.pa_ubRetryNumber++;

	if (net_bReportPackets == TRUE)	{
		CPrintF("Retrying sequence: %d, reliable flag: %d\n",paPacket.pa_ulSequence,paPacket.pa_ubReliable);
	}

	return AppendPacket(paPacket,TRUE);
};

// Reads the data from the first packet in the bufffer, but does not remove it
CPacket* CPacketBuffer::PeekFirstPacket()
{
	ASSERT(pb_ulNumOfPackets != 0);
	return LIST_HEAD(pb_lhPacketStorage,CPacket,pa_lnListNode);
};

// Reads the first packet in the bufffer, and removes it from the buffer
CPacket* CPacketBuffer::GetFirstPacket()
{
	ASSERT(pb_ulNumOfPackets != 0);
	
	CPacket* ppaHead = LIST_HEAD(pb_lhPacketStorage,CPacket,pa_lnListNode);

	// remove the first packet from the start of the list
	pb_lhPacketStorage.RemHead();
	pb_ulNumOfPackets--;
	if (ppaHead->pa_ubReliable & UDP_PACKET_RELIABLE) {
		pb_ulNumOfReliablePackets--;
	}

	// update the total size of data stored in the buffer
	pb_ulTotalSize -= (ppaHead->pa_slSize - MAX_HEADER_SIZE);

	// mark the last packet sequence that was output from the buffer - helps to prevent problems wit duplicated packets	
	if (pb_ulLastSequenceOut < ppaHead->pa_ulSequence) {
		pb_ulLastSequenceOut = ppaHead->pa_ulSequence;		
	}

	return ppaHead;

};

// Reads the data from the packet with the requested sequence, but does not remove it
CPacket* CPacketBuffer::PeekPacket(ULONG ulSequence)
{
	FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ulSequence == ulSequence) {
			return litPacketIter;
		}
	}
	return NULL;
};

// Returns te packet with the matching sequence from the buffer
CPacket* CPacketBuffer::GetPacket(ULONG ulSequence)
{
	
	FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ulSequence == ulSequence) {
			litPacketIter->pa_lnListNode.Remove();

			pb_ulNumOfPackets--;
			if (litPacketIter->pa_ubReliable & UDP_PACKET_RELIABLE) {
				pb_ulNumOfReliablePackets--;
			}

			// update the total size of data stored in the buffer
			pb_ulTotalSize -= (litPacketIter->pa_slSize - MAX_HEADER_SIZE);

			return litPacketIter;
		}
	}
	return NULL;
};

// Reads the first connection request packet from the buffer
CPacket* CPacketBuffer::GetConnectRequestPacket() {
		FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ubReliable & UDP_PACKET_CONNECT_REQUEST) {
			litPacketIter->pa_lnListNode.Remove();

			pb_ulNumOfPackets--;
			// connect request packets are allways reliable
			pb_ulNumOfReliablePackets--;

			// update the total size of data stored in the buffer
			pb_ulTotalSize -= (litPacketIter->pa_slSize - MAX_HEADER_SIZE);

			return litPacketIter;
		}
	}
	return NULL;
};

// Removes the first packet from the buffer
BOOL CPacketBuffer::RemoveFirstPacket(BOOL bDelete) {
	ASSERT(pb_ulNumOfPackets > 0);
	CPacket *lnHead = LIST_HEAD(pb_lhPacketStorage,CPacket,pa_lnListNode);

	pb_ulNumOfPackets--;
	if (lnHead->pa_ubReliable & UDP_PACKET_RELIABLE) {
		pb_ulNumOfReliablePackets--;
	}

	// update the total size of data stored in the buffer
	pb_ulTotalSize -= (lnHead->pa_slSize - MAX_HEADER_SIZE);

	if (pb_ulLastSequenceOut < lnHead->pa_ulSequence) {
		pb_ulLastSequenceOut = lnHead->pa_ulSequence;		
	}

	pb_lhPacketStorage.RemHead();
	if (bDelete) {
		delete lnHead;
	}
	return TRUE;
};

// Removes the packet with the requested sequence from the buffer
BOOL CPacketBuffer::RemovePacket(ULONG ulSequence,BOOL bDelete)
{
//	ASSERT(pb_ulNumOfPackets > 0);
	FORDELETELIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ulSequence == ulSequence) {
			litPacketIter->pa_lnListNode.Remove();
			
			pb_ulNumOfPackets--;
			if (litPacketIter->pa_ubReliable & UDP_PACKET_RELIABLE) {
				pb_ulNumOfReliablePackets--;
			}

			// update the total size of data stored in the buffer
			pb_ulTotalSize -= (litPacketIter->pa_slSize - MAX_HEADER_SIZE);

			if (bDelete) {
				delete litPacketIter;
			}
		}
	}
	return FALSE;
};

// Remove connect response packets from the buffer
BOOL CPacketBuffer::RemoveConnectResponsePackets() {
		FORDELETELIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ubReliable & UDP_PACKET_CONNECT_RESPONSE) {
			litPacketIter->pa_lnListNode.Remove();

			pb_ulNumOfPackets--;
			
			// connect request packets are allways reliable
			pb_ulNumOfReliablePackets--;

			// update the total size of data stored in the buffer
			pb_ulTotalSize -= (litPacketIter->pa_slSize - MAX_HEADER_SIZE);

			delete litPacketIter;
		}
	}
	return NULL;
};


// Gets the sequence number of the first packet in the buffer
ULONG CPacketBuffer::GetFirstSequence()
{
	ASSERT(pb_ulNumOfPackets > 0);

	return LIST_HEAD(pb_lhPacketStorage,CPacket,pa_lnListNode)->pa_ulSequence;

};

// Gets the sequence number of the last packet in the buffer
ULONG CPacketBuffer::GetLastSequence()
{
	ASSERT(pb_ulNumOfPackets > 0);

	return LIST_TAIL(pb_lhPacketStorage,CPacket,pa_lnListNode)->pa_ulSequence;

};

// Removes the packet with the requested sequence from the buffer
BOOL CPacketBuffer::IsSequenceInBuffer(ULONG ulSequence)
{
	FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		if (litPacketIter->pa_ulSequence == ulSequence) {
			return TRUE;
		}
	}
	return FALSE;
};


// Check if the buffer contains a complete sequence of reliable packets	at the start of the buffer
BOOL CPacketBuffer::CheckSequence(SLONG &slSize)
{

	CPacket* paPacket;
	ULONG ulSequence;

  slSize=0;

	if (pb_ulNumOfPackets == 0) {  
		return FALSE;
	}

	paPacket = LIST_HEAD(pb_lhPacketStorage,CPacket,pa_lnListNode);

	// if the first packet is not the head of the reliable packet transfer
	if (!(paPacket->pa_ubReliable & UDP_PACKET_RELIABLE_HEAD)) {
		return FALSE;
	}

	ulSequence = paPacket->pa_ulSequence;

	// for each packet in the buffer
	FOREACHINLIST(CPacket,pa_lnListNode,pb_lhPacketStorage,litPacketIter) {
		// if it's out of order (there is a gap in the reliable sequence), the message is not complete
		if (litPacketIter->pa_ulSequence != ulSequence) {
			return FALSE;
		}
		// if it's a tail of the reliable sequence the message is complete (all packets so far
		// have been in order)
		if (litPacketIter->pa_ubReliable & UDP_PACKET_RELIABLE_TAIL) {
			return TRUE;
		}
    slSize += litPacketIter->pa_slSize - MAX_HEADER_SIZE;
		ulSequence++;
	}

	// if the function hasn't exited while in the loop, the message is not complete
	// (all the packets are in sequence, but there is no tail)
	return FALSE;
};




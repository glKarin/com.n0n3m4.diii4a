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

#include "stdafx.h"
#include "EMsgBuffer.h"

#include <Engine/Math/Functions.h>
#include <Engine/Base/Memory.h>
#include <Engine/Math/Quaternion.h>


void AngleToUL(ANGLE3D &Angle,ULONG &ulResult) 
{
	Quaternion<float> qQuat;
	FLOAT3D	Axis;
	float fRotAngle;
	ANGLE3D AxisAngles;
	UBYTE ubDir;
	UWORD swAngle;

	qQuat.FromEuler(Angle);
	qQuat.ToAxisAngle(Axis,fRotAngle);
	Axis.Normalize();
	DirectionVectorToAngles(Axis,AxisAngles);


	ubDir = (UBYTE) (AxisAngles(1)/360*255);
	ulResult = ubDir;
	ubDir = (UBYTE) (AxisAngles(2)/90*127);
	ulResult = ulResult << 8;
  ulResult |= ubDir;
	swAngle = (UWORD) (SWORD) (fRotAngle * 180);	// after rounding, angle is precise up to 1/180 degrees (65536/360 ~ 180)
	ulResult = (ulResult << 16) | swAngle;

};


void ULToAngle(ULONG &ulResult,ANGLE3D &Angle) 
{
	Quaternion<float> qQuat;
	FLOAT3D	Axis;
	float fRotAngle;
	ANGLE3D AxisAngles;
	UBYTE ubDir;
	UWORD swAngle;
	Matrix<float,3,3> mRotMatrix;

	swAngle = ulResult & 0x0000FFFF;
	fRotAngle = swAngle / 180.0f;
	ulResult = ulResult >> 16;
	ubDir = ulResult & 0x000000FF;
	AxisAngles(2) = ((FLOAT) ubDir)*90/127;
	ulResult = ulResult >> 8;
	ubDir = ulResult & 0x000000FF;
	AxisAngles(1) = ((FLOAT) ubDir)*360/255;
	AxisAngles(3) = 0;

	AnglesToDirectionVector(AxisAngles,Axis);
	qQuat.FromAxisAngle(Axis,fRotAngle);
	qQuat.ToMatrix(mRotMatrix);

	DecomposeRotationMatrixNoSnap(Angle,mRotMatrix);

};



void CEntityMessage::WritePlacement(ULONG &ulEntityID,CPlacement3D &plPlacement)
{
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;


	em_ulType = EMT_SETPLACEMENT;
	em_ubSize = sizeof(FLOAT3D) + sizeof(ULONG);
	em_ulEntityID = ulEntityID;	

  swH = (SWORD) ((plPlacement.pl_OrientationAngle(1)+180)*5);
  swP = (SWORD) ((plPlacement.pl_OrientationAngle(2)+90)*5);
  swB = (SWORD) ((plPlacement.pl_OrientationAngle(3)+180)*5);

  ulShrunkAngle  = ((((ULONG) swH) & 0x000007FF) << 21);
  ulShrunkAngle |= ((((ULONG) swP) & 0x000003FF) << 11);
  ulShrunkAngle |= (((ULONG) swB) & 0x000007FF);

	pubMarker = em_aubMessage;
	memcpy(pubMarker,&(plPlacement.pl_PositionVector(1)),sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(pubMarker,&ulShrunkAngle,sizeof(ULONG));
};


void CEntityMessage::ReadPlacement(ULONG &ulEntityID,CPlacement3D &plPlacement)
{
	ASSERT (em_ulType == EMT_SETPLACEMENT);
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;


  ulEntityID = em_ulEntityID;

	pubMarker = em_aubMessage;
	memcpy(&(plPlacement.pl_PositionVector(1)),pubMarker,sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(&ulShrunkAngle,pubMarker,sizeof(ULONG));
  
  swB = (SWORD) ulShrunkAngle & 0x000007FF;
  swP = (SWORD) (ulShrunkAngle >> 11) & 0x000003FF;
  swH = (SWORD) (ulShrunkAngle >> 21) & 0x000007FF;
  
  plPlacement.pl_OrientationAngle(1) = ((float)swH) / 5 - 180;
  plPlacement.pl_OrientationAngle(2) = ((float)swP) / 5 - 90;
  plPlacement.pl_OrientationAngle(3) = ((float)swB) / 5 - 180;

};


void CEntityMessage::WriteEntityEvent(ULONG &ulEntityID,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	UBYTE *pubMarker;

	em_ulType = EMT_EVENT;
	em_ubSize = sizeof(UWORD) + uwDataSize;
	em_ulEntityID = ulEntityID;

	pubMarker = em_aubMessage;
	memcpy(pubMarker,&uwEventCode,sizeof(SLONG));
	pubMarker += sizeof(UWORD);
	memcpy(pubMarker,pvEventData,uwDataSize);

};


void CEntityMessage::ReadEntityEvent(ULONG &ulEntityID,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	ASSERT (em_ulType == EMT_EVENT);
	UBYTE *pubMarker;

	pubMarker = em_aubMessage;
	memcpy(&uwEventCode,pubMarker,sizeof(SLONG));
	pubMarker += sizeof(UWORD);
	uwDataSize =  em_ubSize - sizeof(ULONG) - sizeof(UWORD);
	memcpy(pvEventData,pubMarker,uwDataSize);

};


void CEntityMessage::WriteEntityCreate(ULONG &ulEntityID,CPlacement3D &plPlacement,UWORD &uwEntityClassID,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;

	em_ulType = EMT_CREATE;
	em_ubSize = sizeof(FLOAT3D) + sizeof(ULONG) + sizeof(UWORD) + sizeof(UWORD) + uwDataSize;
	em_ulEntityID = ulEntityID;

  swH = (SWORD) ((plPlacement.pl_OrientationAngle(1)+180)*5);
  swP = (SWORD) ((plPlacement.pl_OrientationAngle(2)+90)*5);
  swB = (SWORD) ((plPlacement.pl_OrientationAngle(3)+180)*5);

  ulShrunkAngle  = ((((ULONG) swH) & 0x000007FF) << 21);
  ulShrunkAngle |= ((((ULONG) swP) & 0x000003FF) << 11);
  ulShrunkAngle |= (((ULONG) swB) & 0x000007FF);


	pubMarker = em_aubMessage;
	memcpy(pubMarker,&(plPlacement.pl_PositionVector(1)),sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(pubMarker,&ulShrunkAngle,sizeof(ULONG));
	pubMarker += sizeof(ULONG);
	memcpy(pubMarker,&uwEntityClassID,sizeof(UWORD));
	pubMarker += sizeof(UWORD);
	memcpy(pubMarker,&uwEventCode,sizeof(UWORD));	
	pubMarker += sizeof(UWORD);
	memcpy(pubMarker,pvEventData,uwDataSize);

};

void CEntityMessage::ReadEntityCreate(ULONG &ulEntityID,CPlacement3D &plPlacement,UWORD &uwEntityClassID,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	ASSERT (em_ulType == EMT_CREATE);
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;

  ulEntityID = em_ulEntityID;

	pubMarker = em_aubMessage;
	memcpy(&(plPlacement.pl_PositionVector(1)),pubMarker,sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(&ulShrunkAngle,pubMarker,sizeof(ULONG));
  
  swB = (SWORD) ulShrunkAngle & 0x000007FF;
  swP = (SWORD) (ulShrunkAngle >> 11) & 0x000003FF;
  swH = (SWORD) (ulShrunkAngle >> 21) & 0x000007FF;
  
  plPlacement.pl_OrientationAngle(1) = ((float)swH) / 5 - 180;
  plPlacement.pl_OrientationAngle(2) = ((float)swP) / 5 - 90;
  plPlacement.pl_OrientationAngle(3) = ((float)swB) / 5 - 180;

	pubMarker += sizeof(ULONG);
	memcpy(&uwEntityClassID,pubMarker,sizeof(UWORD));
	pubMarker += sizeof(UWORD);
	memcpy(&uwEventCode,pubMarker,sizeof(UWORD));	
	pubMarker += sizeof(UWORD);
	uwDataSize = em_ubSize - sizeof(FLOAT3D) - sizeof(ULONG) - sizeof(UWORD) - sizeof(UWORD);
	memcpy(pubMarker,pvEventData,uwDataSize);

};


void CEntityMessage::WriteEntityDestroy(ULONG &ulEntityID)
{
	
	em_ulType = EMT_DESTROY;
	em_ubSize = 0;
  em_ulEntityID = ulEntityID;

};


void CEntityMessage::ReadEntityDestroy(ULONG &ulEntityID)
{
	ASSERT (em_ulType == EMT_DESTROY);

	ulEntityID = em_ulEntityID;

};



void CEntityMessage::WriteEntityCopy(ULONG &ulSourceEntityID,ULONG &ulTargetEntityID,CPlacement3D &plPlacement,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;

	em_ulType = EMT_COPY;
	em_ubSize = sizeof(ULONG) + sizeof(FLOAT3D) + sizeof(ULONG) + sizeof(UWORD) + uwDataSize;
	em_ulEntityID = ulSourceEntityID;

  swH = (SWORD) ((plPlacement.pl_OrientationAngle(1)+180)*5);
  swP = (SWORD) ((plPlacement.pl_OrientationAngle(2)+90)*5);
  swB = (SWORD) ((plPlacement.pl_OrientationAngle(3)+180)*5);

  ulShrunkAngle  = ((((ULONG) swH) & 0x000007FF) << 21);
  ulShrunkAngle |= ((((ULONG) swP) & 0x000003FF) << 11);
  ulShrunkAngle |= (((ULONG) swB) & 0x000007FF);



	pubMarker = em_aubMessage;
	memcpy(pubMarker,&ulTargetEntityID,sizeof(ULONG));
	pubMarker += sizeof(ULONG);
	memcpy(pubMarker,&(plPlacement.pl_PositionVector(1)),sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(pubMarker,&ulShrunkAngle,sizeof(ULONG));
	pubMarker += sizeof(ULONG);
	memcpy(pubMarker,&uwEventCode,sizeof(SLONG));	
	pubMarker += sizeof(UWORD);
	memcpy(pubMarker,pvEventData,uwDataSize);

};

void CEntityMessage::ReadEntityCopy(ULONG &ulSourceEntityID,ULONG &ulTargetEntityID,CPlacement3D &plPlacement,UWORD &uwEventCode,void* pvEventData,UWORD &uwDataSize)
{
	ASSERT (em_ulType == EMT_COPY);
	UBYTE *pubMarker;
  ULONG ulShrunkAngle;
  SWORD swH,swP,swB;

	ulSourceEntityID = em_ulEntityID;

	pubMarker = em_aubMessage;
	memcpy(&ulTargetEntityID,pubMarker,sizeof(ULONG));
	pubMarker += sizeof(ULONG);
	memcpy(&(plPlacement.pl_PositionVector(1)),pubMarker,sizeof(FLOAT3D));
	pubMarker += sizeof(FLOAT3D);
	memcpy(&ulShrunkAngle,pubMarker,sizeof(ULONG));
  
  swB = (SWORD) ulShrunkAngle & 0x000007FF;
  swP = (SWORD) (ulShrunkAngle >> 11) & 0x000003FF;
  swH = (SWORD) (ulShrunkAngle >> 21) & 0x000007FF;
  
  plPlacement.pl_OrientationAngle(1) = ((float)swH) / 5 - 180;
  plPlacement.pl_OrientationAngle(2) = ((float)swP) / 5 - 90;
  plPlacement.pl_OrientationAngle(3) = ((float)swB) / 5 - 180;

	pubMarker += sizeof(ULONG);
	memcpy(&uwEventCode,pubMarker,sizeof(SLONG));	
	pubMarker += sizeof(UWORD);
	uwDataSize = em_ubSize - sizeof(ULONG) - sizeof(FLOAT3D) - sizeof(ULONG) - sizeof(UWORD);
	memcpy(pubMarker,pvEventData,uwDataSize);

};



CEMsgBuffer::CEMsgBuffer()
{				  
	emb_uwNumTickMarkers    = 0;
	emb_iFirstTickMarker    = 0;
  emb_iCurrentTickMarker  = 0;
  emb_fCurrentTickTime    = -1;

	for (int i=0;i<MAX_TICKS_KEPT;i++) {
		emb_atmTickMarkers[i].tm_slTickOffset = -1;
		emb_atmTickMarkers[i].tm_fTickTime    = -1;
		emb_atmTickMarkers[i].tm_ubAcknowledgesExpected = 0;
	}

};



CEMsgBuffer::~CEMsgBuffer ()
{
};


void CEMsgBuffer::Clear(void)
{
  emb_uwNumTickMarkers    = 0;
	emb_iFirstTickMarker    = 0;
  emb_iCurrentTickMarker  = 0;
  emb_fCurrentTickTime    = -1;

	for (int i=0;i<MAX_TICKS_KEPT;i++) {
		emb_atmTickMarkers[i].tm_slTickOffset = -1;
		emb_atmTickMarkers[i].tm_fTickTime    = -1;
		emb_atmTickMarkers[i].tm_ubAcknowledgesExpected = 0;
	}

  bu_slWriteOffset = 0;
  bu_slReadOffset = 0;

  bu_slFree = bu_slSize;

};


void CEMsgBuffer::WriteMessage(CEntityMessage  &emEntityMessage) 
{
  // a tick must have started before any messages are generated
  ASSERT (emb_uwNumTickMarkers > 0);
	ULONG ulTemp;
  int iTickMarker;

	ulTemp = emEntityMessage.em_ulEntityID | (emEntityMessage.em_ulType << 24);
	WriteBytes(&(ulTemp),sizeof(ULONG));	
	WriteBytes(&(emEntityMessage.em_ubSize),sizeof(emEntityMessage.em_ubSize));
	if (emEntityMessage.em_ubSize > 0) {
		WriteBytes(emEntityMessage.em_aubMessage,emEntityMessage.em_ubSize);
	}

  iTickMarker = emb_iCurrentTickMarker-1;
  if (iTickMarker < 0) iTickMarker += MAX_TICKS_KEPT;
  emb_atmTickMarkers[iTickMarker].tm_uwNumMessages++;
};


int CEMsgBuffer::ReadMessage(CEntityMessage  &emEntityMessage)
{
	ULONG ulTemp;

  if (bu_slReadOffset == bu_slWriteOffset) {
    return EMB_ERR_BUFFER_EMPTY;
  }

	ReadBytes(&(ulTemp),sizeof(ULONG));
	emEntityMessage.em_ulType = ulTemp >> 24;
	emEntityMessage.em_ulEntityID = ulTemp & 0x007FFFFF;
	ReadBytes(&(emEntityMessage.em_ubSize),sizeof(emEntityMessage.em_ubSize));
	if (emEntityMessage.em_ubSize > 0) {
		ReadBytes(emEntityMessage.em_aubMessage,emEntityMessage.em_ubSize);
	}

  return EMB_SUCCESS_OK;
};


int CEMsgBuffer::PeekMessageAtOffset(CEntityMessage &emEntityMessage,SLONG &slTickOffset)
{
  ASSERT(slTickOffset >= 0 && slTickOffset <= bu_slSize);
  ULONG ulTemp;

  PeekBytesAtOffset(&(ulTemp),sizeof(ULONG),slTickOffset);
	emEntityMessage.em_ulType = ulTemp >> 24;
	emEntityMessage.em_ulEntityID = ulTemp & 0x007FFFFF;
	PeekBytesAtOffset(&(emEntityMessage.em_ubSize),sizeof(emEntityMessage.em_ubSize),slTickOffset);
	if (emEntityMessage.em_ubSize > 0) {
		PeekBytesAtOffset(emEntityMessage.em_aubMessage,emEntityMessage.em_ubSize,slTickOffset);
	}

  return EMB_SUCCESS_OK;
};


int CEMsgBuffer::StartNewTick(float fTickTime)
{
	
	if (emb_uwNumTickMarkers >= MAX_TICKS_KEPT) {
		return EMB_ERR_MAX_TICKS;
	}

  emb_atmTickMarkers[emb_iCurrentTickMarker].tm_fTickTime = fTickTime;
	emb_atmTickMarkers[emb_iCurrentTickMarker].tm_slTickOffset = bu_slWriteOffset;
	emb_atmTickMarkers[emb_iCurrentTickMarker].tm_ubAcknowledgesExpected = 0;
  emb_atmTickMarkers[emb_iCurrentTickMarker].tm_uwNumMessages = 0;
	
  emb_uwNumTickMarkers++;
  emb_iCurrentTickMarker++;
  emb_iCurrentTickMarker %= MAX_TICKS_KEPT;

	return EMB_SUCCESS_OK;
};


int CEMsgBuffer::SetCurrentTick(float fTickTime) 
{
  int iErr;
  INDEX iTickIndex;

  iErr = GetTickIndex(fTickTime,iTickIndex);
  if (iErr != EMB_SUCCESS_OK) {
    return iErr;
  }  

  emb_iCurrentTickMarker = iTickIndex;

  return EMB_SUCCESS_OK;
};


int CEMsgBuffer::GetTickIndex(float fTickTime,INDEX &iTickIndex)
{
  INDEX iTickMarker;

  // 0.025 should be _pTimer->TickQuantum/2
	for (int i=0;i<emb_uwNumTickMarkers;i++) {
    iTickMarker = (i + emb_iFirstTickMarker) % MAX_TICKS_KEPT;
		if (fabs(emb_atmTickMarkers[iTickMarker].tm_fTickTime - fTickTime) < 0.025) {
      iTickIndex = iTickMarker;			
			return EMB_SUCCESS_OK;
		}
	}

  iTickIndex = -1;

  return EMB_ERR_NOT_IN_BUFFER;
};


int CEMsgBuffer::GetTickOffset(float fTickTime,SLONG &slTickOffset)
{
  INDEX iTickMarker;
  // 0.025 should be _pTimer->TickQuantum/2
	for (int i=0;i<emb_uwNumTickMarkers;i++) {  
    iTickMarker = (i + emb_iFirstTickMarker) % MAX_TICKS_KEPT;
		if (fabs(emb_atmTickMarkers[iTickMarker].tm_fTickTime - fTickTime) < 0.025) {
      slTickOffset = emb_atmTickMarkers[iTickMarker].tm_slTickOffset;			
			return EMB_SUCCESS_OK;
		}
	}

  slTickOffset = -1.0f;

  return EMB_ERR_NOT_IN_BUFFER;
};



int CEMsgBuffer::GetNextTickTime(float fTickTime,float &fNextTickTime)
{
  INDEX iTickIndex;
  int   iErr;

  if (fTickTime < 0) {
    if (emb_uwNumTickMarkers > 0) {
      fNextTickTime = emb_atmTickMarkers[0].tm_fTickTime;
      return EMB_SUCCESS_OK;
    } else {
      return EMB_ERR_NOT_IN_BUFFER;
    }
  }

  if (fTickTime < emb_atmTickMarkers[emb_iFirstTickMarker].tm_fTickTime) {
    fNextTickTime = emb_atmTickMarkers[emb_iFirstTickMarker].tm_fTickTime;
    return SUCCESS_OK;
  };

  iErr = GetTickIndex(fTickTime,iTickIndex);
  if (iErr != EMB_SUCCESS_OK) {
    return iErr;
  }

  iTickIndex = (iTickIndex + 1) % MAX_TICKS_KEPT;

  fNextTickTime = emb_atmTickMarkers[iTickIndex].tm_fTickTime;

  return EMB_SUCCESS_OK;
};


int CEMsgBuffer::RequestTickAcknowledge(float fTickTime,UBYTE ubNumAcknowledges) 
{
  ASSERT (fTickTime >= 0);
  INDEX iTickIndex;
  int   iErr;

  iErr = GetTickIndex(fTickTime,iTickIndex);
  if (iErr == EMB_SUCCESS_OK) {
    emb_atmTickMarkers[iTickIndex].tm_ubAcknowledgesExpected += ubNumAcknowledges;
		return EMB_SUCCESS_OK;
  }
  return iErr;
};



int CEMsgBuffer::ReceiveTickAcknowledge(float fTickTime) 
{
  ASSERT (fTickTime >= 0);
  INDEX iTickIndex;
  INDEX iFirst;
  int   iErr;
  int   iNumMark = emb_uwNumTickMarkers;

  iErr = GetTickIndex(fTickTime,iTickIndex);
  if (iErr == EMB_SUCCESS_OK) {
    iFirst = emb_iFirstTickMarker;
    emb_atmTickMarkers[iFirst].tm_ubAcknowledgesExpected--;
    while (iFirst!=iTickIndex) {
      iFirst++;
      iFirst%=MAX_TICKS_KEPT;
      emb_atmTickMarkers[iFirst].tm_ubAcknowledgesExpected--;
    }
    

    iFirst = emb_iFirstTickMarker;
    if (emb_atmTickMarkers[iFirst].tm_ubAcknowledgesExpected == 0 && iNumMark != 0) {
      while (emb_atmTickMarkers[iFirst].tm_ubAcknowledgesExpected == 0 && iNumMark != 0) {
        if (iNumMark > 0) {     
          iNumMark--;
          iFirst++;
          iFirst %= MAX_TICKS_KEPT;
        }
      }     
      MoveToStartOfTick(emb_atmTickMarkers[iFirst].tm_fTickTime);
    }

		return EMB_SUCCESS_OK;
  }
  return iErr;
};


// does not advance the read offset - access is random, not sequential
int  CEMsgBuffer::ReadTick(float fTickTime,const void *pv, SLONG &slSize)
{
  ASSERT (slSize>0);
  ASSERT (pv != NULL);
  ASSERT (fTickTime >= 0);
  int   iErr;
  INDEX iTickIndex,iNextTickIndex;
  
  
  iErr = GetTickIndex(fTickTime,iTickIndex);

  if (iErr != EMB_SUCCESS_OK) {
    return iErr;
  }

  if (iTickIndex >= (emb_iFirstTickMarker + emb_uwNumTickMarkers - 1)) {
    return EMB_ERR_TICK_NOT_COMPLETE;
  }

  iNextTickIndex = (iTickIndex + 1) % MAX_TICKS_KEPT;
  SLONG slTickSize = emb_atmTickMarkers[iNextTickIndex].tm_slTickOffset - emb_atmTickMarkers[iTickIndex].tm_slTickOffset;
  // if not wrapping 
  if ( slTickSize > 0) {
    if (slSize < slTickSize) {
      return EMB_ERR_BUFFER_TOO_SMALL;
    }
    slSize = slTickSize;
    memcpy((UBYTE*)pv,bu_pubBuffer+emb_atmTickMarkers[iTickIndex].tm_slTickOffset,slTickSize);          
  } else {
    if (slSize < (bu_slSize - emb_atmTickMarkers[iTickIndex].tm_slTickOffset) + emb_atmTickMarkers[iNextTickIndex].tm_slTickOffset-1) {
      return EMB_ERR_BUFFER_TOO_SMALL;
    }
    slSize = (bu_slSize - emb_atmTickMarkers[iTickIndex].tm_slTickOffset) + emb_atmTickMarkers[iNextTickIndex].tm_slTickOffset;
    // copy data from the start of this ick to the end of the bufer
    memcpy((UBYTE*)pv,bu_pubBuffer+emb_atmTickMarkers[iTickIndex].tm_slTickOffset,bu_slSize - emb_atmTickMarkers[iTickIndex].tm_slTickOffset);
    // copy datafrom the start of the buffer to the start of next tick
    memcpy(((UBYTE*)pv)+bu_slSize - emb_atmTickMarkers[iTickIndex].tm_slTickOffset,bu_pubBuffer,emb_atmTickMarkers[iNextTickIndex].tm_slTickOffset);
  }

  return EMB_SUCCESS_OK;

};


void CEMsgBuffer::WriteTick(float tm_fTickTime,const void *pv, SLONG slSize)
{
  ASSERT(slSize>=0 && pv!=NULL);

  StartNewTick(tm_fTickTime);
  WriteBytes(pv,slSize);
};



int CEMsgBuffer::MoveToStartOfTick(float fTickTime) 
{
  int iErr;
  INDEX iTickIndex;

  ASSERT(fTickTime >= emb_atmTickMarkers[emb_iFirstTickMarker].tm_fTickTime);

  iErr = GetTickIndex(fTickTime,iTickIndex);
  if (iErr != EMB_SUCCESS_OK) {
    return iErr;
  }  


  // if not wrapping
  if (iTickIndex >= emb_iFirstTickMarker) { 
    emb_uwNumTickMarkers -= iTickIndex - emb_iFirstTickMarker;
  } else {
    emb_uwNumTickMarkers -= iTickIndex + (MAX_TICKS_KEPT - emb_iFirstTickMarker);
  }

  emb_iFirstTickMarker = iTickIndex;

  if (bu_slReadOffset <= emb_atmTickMarkers[iTickIndex].tm_slTickOffset) {
    bu_slFree += emb_atmTickMarkers[iTickIndex].tm_slTickOffset - bu_slReadOffset;
  } else {
    bu_slFree += (bu_slSize - bu_slReadOffset) + emb_atmTickMarkers[iTickIndex].tm_slTickOffset;
  }

  bu_slReadOffset = emb_atmTickMarkers[iTickIndex].tm_slTickOffset;
 

  return EMB_SUCCESS_OK;

};


// expand buffer to be given number of bytes in size
void CEMsgBuffer::Expand(SLONG slNewSize)
{
  ASSERT(slNewSize>0);
  ASSERT(bu_slSize>=0);
  // if not already allocated
  if (bu_slSize==0) {
    // allocate a new empty buffer
    ASSERT(bu_pubBuffer==NULL);
    bu_pubBuffer = (UBYTE*)AllocMemory(slNewSize);
    bu_slWriteOffset = 0;
    bu_slReadOffset = 0;
    bu_slFree = slNewSize;
    bu_slSize = slNewSize;

    // if already allocated
  } else {
    ASSERT(slNewSize>bu_slSize);
    SLONG slSizeDiff = slNewSize-bu_slSize;
    ASSERT(bu_pubBuffer!=NULL);
    // grow buffer
    GrowMemory((void**)&bu_pubBuffer, slNewSize);

    cout << "EXPAND!\n";
    // if buffer is currently wrapping
    if (bu_slReadOffset>bu_slWriteOffset||bu_slFree==0) {
      cout << "WRAP!\n";
      // move part at the end of buffer to the end
      memmove(bu_pubBuffer+bu_slReadOffset+slSizeDiff, bu_pubBuffer+bu_slReadOffset,bu_slSize-bu_slReadOffset);
 			for (int i=0;i<MAX_TICKS_KEPT;i++) {
				if (emb_atmTickMarkers[i].tm_slTickOffset >= bu_slReadOffset) {
					emb_atmTickMarkers[i].tm_slTickOffset += slSizeDiff;
				}
			}
      bu_slReadOffset+=slSizeDiff;
    }
    bu_slFree += slNewSize-bu_slSize;
    bu_slSize = slNewSize;

    ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
    ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);
  }
}




// write bytes to buffer
void CEMsgBuffer::WriteBytes(const void *pv, SLONG slSize)
{
	BOOL bWraping = FALSE;
	SLONG slOldReadOffset = bu_slReadOffset;

	// if buffer is currently wrapping

  ASSERT(slSize>=0 && pv!=NULL);
  // if there is nothing to write

  if (slSize==0) {
    // do nothing
    return;
  }
  // check for errors
  if (slSize<0) {
    cout << "WARNING: WriteBytes(): slSize<0\n!";
    return;
  }

  // if there is not enough free space
  if (bu_slFree<slSize) {
		SLONG slSizeDiff;
		SLONG slNewSize;
		slNewSize = bu_slSize + ((slSize-bu_slFree + bu_slAllocationStep - 1) / bu_slAllocationStep) * bu_slAllocationStep;
		slSizeDiff =  slNewSize - bu_slSize;

		// if buffer is currently wrapping
//    if (bu_slReadOffset>bu_slWriteOffset||bu_slFree==0) {
//			bWraping = TRUE;
//		}
    // expand the buffer
    Expand(slNewSize);


    ASSERT(bu_slFree>=slSize);
  }

  UBYTE *pub = (UBYTE*)pv;

  // write part of block at the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-bu_slWriteOffset, slSize);
  memcpy(bu_pubBuffer+bu_slWriteOffset, pub, slSizeEnd);
  pub+=slSizeEnd;
  memcpy(bu_pubBuffer, pub, slSize-slSizeEnd);
  // move write pointer
  bu_slWriteOffset+=slSize;
  bu_slWriteOffset%=bu_slSize;
  bu_slFree-=slSize;

  ASSERT(bu_slWriteOffset>=0 && bu_slWriteOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);
};




SLONG CEMsgBuffer::PeekBytes(const void *pv, SLONG slSize)
{
  ASSERT(slSize>0 && pv!=NULL);
  UBYTE *pub = (UBYTE*)pv;

  // clamp size to amount of bytes actually in the buffer
  SLONG slUsed = bu_slSize-bu_slFree;
  if (slUsed<slSize) {
    slSize = slUsed;
  }
  // if there is nothing to read
  if (slSize==0) {
    // do nothing
    return 0;
  }

  // read part of block after read pointer to the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-bu_slReadOffset, slSize);
  memcpy(pub, bu_pubBuffer+bu_slReadOffset, slSizeEnd);
  pub+=slSizeEnd;
  // if that is not all
  if (slSizeEnd<slSize) {
    // read rest from start of buffer
    memcpy(pub, bu_pubBuffer, slSize-slSizeEnd);
  }

  ASSERT(bu_slReadOffset>=0 && bu_slReadOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);

  return slSize;
}


SLONG CEMsgBuffer::PeekBytesAtOffset(const void *pv, SLONG slSize,SLONG &slTickOffset)
{
  ASSERT(slSize>0 && pv!=NULL);
  UBYTE *pub = (UBYTE*)pv;

  // clamp size to amount of bytes actually in the buffer
  SLONG slUsed = bu_slSize-bu_slFree;
  if (slUsed<slSize) {
    slSize = slUsed;
  }
  // if there is nothing to read
  if (slSize==0) {
    // do nothing
    return 0;
  }

  // read part of block after read pointer to the end of buffer
  SLONG slSizeEnd = __min(bu_slSize-slTickOffset, slSize);
  memcpy(pub, bu_pubBuffer+slTickOffset, slSizeEnd);
  pub+=slSizeEnd;
  slTickOffset += slSizeEnd;
  // if that is not all
  if (slSizeEnd<slSize) {
    // read rest from start of buffer
    memcpy(pub, bu_pubBuffer, slSize-slSizeEnd);
    slTickOffset = slSize - slSizeEnd;
  }   

  slTickOffset %= bu_slSize;
  ASSERT(slTickOffset>=0 && slTickOffset<bu_slSize);
  ASSERT(bu_slFree>=0 && bu_slFree<=bu_slSize);

  return slSize;
}

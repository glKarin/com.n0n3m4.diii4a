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

#ifndef SE_INCL_NETWORKPROFILE_H
#define SE_INCL_NETWORKPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#ifndef __ENGINE_BASE_PROFILING_H__
#include <Engine/Base/Profiling.h>
#endif

/* Class for holding profiling information for network. */
class CNetworkProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_MAINLOOP,                 // time spent in main game loop
    PTI_TIMERLOOP,                // time spent in timer game loop

    PTI_SERVER_LOOP,              // time server spent processing messages
    PTI_SESSIONSTATE_LOOP,        // time session state spent processing messages
    PTI_SESSIONSTATE_PROCESSGAMESTREAM, // time session state spent processing gamestream (includes physics)

    PTI_SENDMESSAGE,              // time spend sending message
    PTI_RECEIVEMESSAGE,           // time spend receiving message
    PTI_COUNT
  };
  enum ProfileCounterIndex {
    PCI_GAMESTREAMRESENDS,  // how many times gamestream block was resent from server

    PCI_GAMESTREAM_BYTES_SENT,      // bytes sent in gamestream messages
    PCI_GAMESTREAM_BYTES_RECEIVED,  // bytes received in gamestream messages
    PCI_ACTION_BYTES_SENT,          // bytes sent in action messages
    PCI_ACTION_BYTES_RECEIVED,      // bytes received in action messages

    PCI_MESSAGESSENT,       // total number of messages sent
    PCI_MESSAGESRECEIVED,   // total number of messages received
    PCI_BYTESSENT,          // total number of bytes sent
    PCI_BYTESRECEIVED,      // total number of bytes received
    PCI_COUNT
  };
  // constructor
  CNetworkProfile(void);
};

#endif  // include-once blocker.



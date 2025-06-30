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

#include <Engine/Network/NetworkProfile.h>

// profile form for profiling world editing
CNetworkProfile npNetworkProfile;
CProfileForm &_pfNetworkProfile = npNetworkProfile;

CNetworkProfile::CNetworkProfile(void)
 : CProfileForm("Network", "ticks",
    CNetworkProfile::PCI_COUNT, CNetworkProfile::PTI_COUNT)
{
  // initialize network profile form
  SETTIMERNAME(CNetworkProfile::PTI_MAINLOOP,                 "MainLoop()", "");
  SETTIMERNAME(CNetworkProfile::PTI_TIMERLOOP,                "TimerLoop()", "");
  SETTIMERNAME(CNetworkProfile::PTI_SERVER_LOOP,              "ServerLoop()", "");
  SETTIMERNAME(CNetworkProfile::PTI_SESSIONSTATE_LOOP,        "SessionStateLoop()", "");
  SETTIMERNAME(CNetworkProfile::PTI_SESSIONSTATE_PROCESSGAMESTREAM, "CSessionState::ProcessGameStream()", "");
  SETTIMERNAME(CNetworkProfile::PTI_SENDMESSAGE,              "Send()", "");
  SETTIMERNAME(CNetworkProfile::PTI_RECEIVEMESSAGE,           "Receive()", "");

  SETCOUNTERNAME(CNetworkProfile::PCI_GAMESTREAMRESENDS, "game stream resends");

  SETCOUNTERNAME(CNetworkProfile::PCI_GAMESTREAM_BYTES_SENT,     "gamestream bytes sent");
  SETCOUNTERNAME(CNetworkProfile::PCI_GAMESTREAM_BYTES_RECEIVED, "gamestream bytes received");
  SETCOUNTERNAME(CNetworkProfile::PCI_ACTION_BYTES_SENT,         "action bytes sent");
  SETCOUNTERNAME(CNetworkProfile::PCI_ACTION_BYTES_RECEIVED,     "action bytes received");

  SETCOUNTERNAME(CNetworkProfile::PCI_MESSAGESSENT,      "messages sent");
  SETCOUNTERNAME(CNetworkProfile::PCI_MESSAGESRECEIVED,  "messages received");
  SETCOUNTERNAME(CNetworkProfile::PCI_BYTESSENT,         "bytes sent");
  SETCOUNTERNAME(CNetworkProfile::PCI_BYTESRECEIVED,     "bytes received");
}

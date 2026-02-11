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
#ifndef DEBUGGERAPP_H_
#define DEBUGGERAPP_H_

#include "../../sys/win32/win_local.h"
//#include "../../framework/sync/Msg.h"

#ifndef REGISTRYOPTIONS_H_
#include "../common/RegistryOptions.h"
#endif

#ifndef DEBUGGERWINDOW_H_
#include "DebuggerWindow.h"
#endif

#ifndef DEBUGGERMESSAGES_H_
#include "DebuggerMessages.h"
#endif

#ifndef DEBUGGERCLIENT_H_
#include "DebuggerClient.h"
#endif

// These were changed to static by ID so to make it easy we just throw them
// in this header
#if 1
// we need a lot to be able to list all threads in mars_city1
const int MAX_MSGLEN = 8600;
#else
const int MAX_MSGLEN = 1400;
#endif

class rvDebuggerApp
{
	public:

		rvDebuggerApp();
        ~rvDebuggerApp();

		bool				Initialize(HINSTANCE hInstance);
		int					Run(void);

		rvRegistryOptions	&GetOptions(void);
		rvDebuggerClient	&GetClient(void);
		rvDebuggerWindow	&GetWindow(void);

		HINSTANCE			GetInstance(void);

		bool				TranslateAccelerator(LPMSG msg);

	protected:

		rvRegistryOptions	mOptions;
		rvDebuggerWindow	*mDebuggerWindow;
		HINSTANCE			mInstance;
		rvDebuggerClient	mClient;
		HACCEL				mAccelerators;

	private:

		bool	ProcessNetMessages(void);
		bool	ProcessWindowMessages(void);
};

ID_INLINE HINSTANCE rvDebuggerApp::GetInstance(void)
{
	return mInstance;
}

ID_INLINE rvDebuggerClient &rvDebuggerApp::GetClient(void)
{
	return mClient;
}

ID_INLINE rvRegistryOptions &rvDebuggerApp::GetOptions(void)
{
	return mOptions;
}

ID_INLINE rvDebuggerWindow &rvDebuggerApp::GetWindow(void)
{
	assert(mDebuggerWindow);
	return *mDebuggerWindow;
}

extern rvDebuggerApp gDebuggerApp;

// compat
typedef idBitMsg msg_t;

#define MSG_Init(msg, a, b) (msg)->Init((a), (b))
#define MSG_InitW(msg, a, b) { MSG_Init(msg, a, b); \
                           (msg)->BeginWriting(); }
#define MSG_InitR(msg, a, b) { MSG_Init(msg, a, b); \
                           (msg)->BeginReading(); }
#define MSG_ReadShort(msg) (msg)->ReadShort()
#define MSG_WriteShort(msg, a) (msg)->WriteShort((a))
#define MSG_ReadString(msg, a, b) (msg)->ReadString((a), (b))
#define MSG_WriteString(msg, a) (msg)->WriteString((a))
#define MSG_ReadLong(msg) (msg)->ReadLong()
#define MSG_WriteLong(msg, a) (msg)->WriteLong((a))
#define MSG_WriteBits(msg, a, b) (msg)->WriteBits((a), (b))
#define MSG_ReadBits(msg, a) (msg)->ReadBits((a))
#define MSG_data(msg) (msg)->GetData()
#define MSG_cursize(msg) (msg)->GetSize()
#define MSG_Size(msg, a) { (msg)->SetSize(b); \
                         (msg)->BeginReading(); }
#define MSG_Set(msg, a, b) { (msg)->Init(a, sizeof(a)); \
                           (msg)->SetSize(b); }
#define MSG_SetR(msg, a, b) { MSG_Set(msg, a, b) \
                           (msg)->BeginReading(); }
#define MSG_SetW(msg, a, b) { MSG_Set(msg, a, b) \
                           (msg)->BeginWriting(); }
#define MSG_readcount(msg) (msg)->GetReadCount()
#define MSG_readcount_(msg, a) (msg)->SetReadCount(a)
#define MSG_bit(msg) (msg)->GetReadBit()
#define MSG_bit_(msg, a) (msg)->SetReadBit(a)

#endif // DEBUGGERAPP_H_

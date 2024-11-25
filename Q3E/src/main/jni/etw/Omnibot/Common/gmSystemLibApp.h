#ifndef _GMSYSTEMLIB_H_
#define _GMSYSTEMLIB_H_

#include "gmConfig.h"

class gmMachine;

#include "gmBind.h"

class gmFile : public gmBind<File, gmFile>
{
public:
	GMBIND_DECLARE_FUNCTIONS( );
	GMBIND_DECLARE_PROPERTIES( );

	// Functions
	static int gmfOpen(gmThread *a_thread);
	static int gmfClose(gmThread *a_thread);
	static int gmfIsOpen(gmThread *a_thread);
	static int gmfSeek(gmThread *a_thread);
	static int gmfTell(gmThread *a_thread);
	static int gmfEndOfFile(gmThread *a_thread);
	static int gmfFileSize(gmThread *a_thread);
	static int gmfFlush(gmThread *a_thread);
	static int gmfGetLastError(gmThread *a_thread);

	static int gmfReadInt32(gmThread *a_thread);
	static int gmfReadInt16(gmThread *a_thread);
	static int gmfReadInt8(gmThread *a_thread);
	static int gmfReadFloat(gmThread *a_thread);
	static int gmfReadString(gmThread *a_thread);
	static int gmfReadLine(gmThread *a_thread);

	static int gmfWrite(gmThread *a_thread);
	
	static File *Constructor(gmThread *a_thread);
	static void Destructor(File *_native);
};

void gmBindSystemLib(gmMachine *a_machine);

#endif

#ifndef CL_PARSE_H
#define CL_PARSE_H

#include "qtypes.h"
#include "cvar.h"

extern cvar_t qport;

void CL_Parse_Init(void);
void CL_Parse_Shutdown(void);
void CL_ParseServerMessage(void);
void CL_Parse_DumpPacket(void);
void CL_Parse_ErrorCleanUp(void);
void QW_CL_StartUpload(unsigned char *data, int size);
void CL_KeepaliveMessage(qbool readmessages); // call this during loading of large content

#endif

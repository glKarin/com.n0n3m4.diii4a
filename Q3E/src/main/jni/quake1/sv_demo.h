#ifndef SV_DEMO_H
#define SV_DEMO_H

#include "qtypes.h"
struct sizebuf_s;
struct client_s;

void SV_StartDemoRecording(struct client_s *client, const char *filename, int forcetrack);
void SV_WriteDemoMessage(struct client_s *client, struct sizebuf_s *sendbuffer, qbool clienttoserver);
void SV_StopDemoRecording(struct client_s *client);
void SV_WriteNetnameIntoDemo(struct client_s *client);

#endif

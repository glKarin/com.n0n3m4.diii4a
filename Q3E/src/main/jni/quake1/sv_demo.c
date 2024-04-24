#include "quakedef.h"
#include "sv_demo.h"

extern cvar_t sv_autodemo_perclient_discardable;

void SV_StartDemoRecording(client_t *client, const char *filename, int forcetrack)
{
	prvm_prog_t *prog = SVVM_prog;
	char name[MAX_QPATH];

	if(client->sv_demo_file != NULL)
		return; // we already have a demo

	dp_strlcpy(name, filename, sizeof(name));
	FS_DefaultExtension(name, ".dem", sizeof(name));

	Con_Printf("Recording demo for # %d (%s) to %s\n", PRVM_NUM_FOR_EDICT(client->edict), client->netaddress, name);

	// Reset discardable flag for every new demo.
	PRVM_serveredictfloat(client->edict, discardabledemo) = 0;

	client->sv_demo_file = FS_OpenRealFile(name, "wb", false);
	if(!client->sv_demo_file)
	{
		Con_Print(CON_ERROR "ERROR: couldn't open.\n");
		return;
	}

	FS_Printf(client->sv_demo_file, "%i\n", forcetrack);
}

void SV_WriteDemoMessage(client_t *client, sizebuf_t *sendbuffer, qbool clienttoserver)
{
	prvm_prog_t *prog = SVVM_prog;
	int len, i;
	float f;
	int temp;

	if(client->sv_demo_file == NULL)
		return;
	if(sendbuffer->cursize == 0)
		return;
	
	temp = sendbuffer->cursize | (clienttoserver ? DEMOMSG_CLIENT_TO_SERVER : 0);
	len = LittleLong(temp);
	FS_Write(client->sv_demo_file, &len, 4);
	for(i = 0; i < 3; ++i)
	{
		f = LittleFloat(PRVM_serveredictvector(client->edict, v_angle)[i]);
		FS_Write(client->sv_demo_file, &f, 4);
	}
	FS_Write(client->sv_demo_file, sendbuffer->data, sendbuffer->cursize);
}

void SV_StopDemoRecording(client_t *client)
{
	prvm_prog_t *prog = SVVM_prog;
	sizebuf_t buf;
	unsigned char bufdata[64];

	if(client->sv_demo_file == NULL)
		return;
	
	buf.data = bufdata;
	buf.maxsize = sizeof(bufdata);
	SZ_Clear(&buf);
	MSG_WriteByte(&buf, svc_disconnect);
	SV_WriteDemoMessage(client, &buf, false);

	if (sv_autodemo_perclient_discardable.integer && PRVM_serveredictfloat(client->edict, discardabledemo))
	{
		FS_RemoveOnClose(client->sv_demo_file);
		Con_Printf("Stopped recording discardable demo for # %d (%s)\n", PRVM_NUM_FOR_EDICT(client->edict), client->netaddress);
	}
	else
		Con_Printf("Stopped recording demo for # %d (%s)\n", PRVM_NUM_FOR_EDICT(client->edict), client->netaddress);

	FS_Close(client->sv_demo_file);
	client->sv_demo_file = NULL;
}

void SV_WriteNetnameIntoDemo(client_t *client)
{
	// This "pseudo packet" is written so a program can easily find out whose demo this is
	sizebuf_t buf;
	unsigned char bufdata[MAX_SCOREBOARDNAME + 64];

	if(client->sv_demo_file == NULL)
		return;

	buf.data = bufdata;
	buf.maxsize = sizeof(bufdata);
	SZ_Clear(&buf);
	MSG_WriteByte(&buf, svc_stufftext);
	MSG_WriteUnterminatedString(&buf, "\n// this demo contains the point of view of: ");
	MSG_WriteUnterminatedString(&buf, client->name);
	MSG_WriteString(&buf, "\n");
	SV_WriteDemoMessage(client, &buf, false);
}

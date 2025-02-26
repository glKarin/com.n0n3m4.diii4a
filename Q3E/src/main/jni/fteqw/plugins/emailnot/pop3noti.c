/*
Copyright (C) 2005 David Walton.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

As a special exception, you may incorpotate patents and libraries regarding only hashing and security, on the conditions that it is also open source.
This means md4/5, rsa, ssl and similar.
*/



#include "../plugin.h"

//the idea is to send a UIDL request, and compare against the previous list.
//this list will be stored on disk on quit.

//be aware that we cannot stay connected. POP3 mailboxes are not refreshable without disconnecting.
//so we have a special state.




char *MD5_GetPop3APOPString(char *timestamp, char *secrit);

void IMAP_CreateConnection(char *servername, char *username, char *password);
void IMAP_Think (void);





//exported.
void POP3_CreateConnection(char *servername, char *username, char *password);
int pop3_checkfrequency=60*1000;
void POP3_Think (void);
void POP3_WriteCache (void);
//end export list.

typedef struct msglist_s {
	struct msglist_s  *next;
	char name[4];
} msglist_t;

msglist_t *msglist;
qboolean POP3_IsMessageUnique(char *name)
{
	msglist_t *msg;

	for (msg = msglist; msg; msg = msg->next)
	{
		if (!strcmp(msg->name, name))
			return false;
	}

	msg = malloc(sizeof(msglist_t) + strlen(name)+1);
	if (!msg)
		return false;
	strcpy(msg->name, name);
	msg->next = msglist;
	msglist = msg;
	
	return true;
}

#define POP3_PORT	110


typedef struct pop3_con_s {
	char server[128];
	char username[128];
	char password[128];

	unsigned int lastnoop;

	//these are used so we can fail a send.
	//or recieve only part of an input.
	//FIXME:	make dynamically sizable, as it could drop if the send is too small (That's okay.
	//			but if the read is bigger than one command we suddenly fail entirly.)
	int sendlen;
	int sendbuffersize;
	char *sendbuffer;
	int readlen;
	int readbuffersize;
	char *readbuffer;

	qboolean drop;

	qhandle_t socket;

	//we have a certain number of stages.
	enum {
		POP3_NOTCONNECTED,
		POP3_WAITINGFORINITIALRESPONCE,	//waiting for an initial response.
		POP3_AUTHING,	//wating for a response from USER
		POP3_AUTHING2,	//Set PASS, waiting to see if we passed.
		POP3_LISTING,	//Sent UIDL, waiting to see
		POP3_RETRIEVING,	//sent TOP, waiting for message headers to print info.
		POP3_HEADER,
		POP3_BODY,
		POP3_QUITTING
	} state;

	int retrlist[256];	//unrecognised uidls are added to this list.
	int numtoretrieve;

	char msgsubject[256];
	char msgfrom[256];

	struct pop3_con_s *next;
} pop3_con_t;

static pop3_con_t *pop3sv;

void POP3_CreateConnection(char *addy, char *username, char *password)
{
	pop3_con_t *con;

	for (con = pop3sv; con; con = con->next)
	{
		if (!strcmp(con->server, addy) && !strcmp(con->username, username))
		{
			if (con->state == POP3_NOTCONNECTED && !con->socket)
				break;
			Con_Printf("Already connected to that pop3 server\n");
			return;
		}
	}

	if (!con)
	{
		con = malloc(sizeof(pop3_con_t));
		if (!con)
		{
			Con_Printf ("POP3_CreateConnection: out of plugin memory\n");
			return;
		}
		memset(con, 0, sizeof(*con));
	}
	else
		con->state = POP3_WAITINGFORINITIALRESPONCE;

	con->socket = Net_TCPConnect(addy, POP3_PORT);
	if (!con->socket)
	{
		Con_Printf ("POP3_CreateConnection: connect failed\n");
		free(con);
		return;
	}

	strlcpy(con->server, addy, sizeof(con->server));
	strlcpy(con->username, username, sizeof(con->username));
	strlcpy(con->password, password, sizeof(con->password));

	if (!con->state)
	{
		con->state = POP3_WAITINGFORINITIALRESPONCE;

		con->next = pop3sv;
		pop3sv = con;

		Con_Printf("Connected to %s\n", con->server);
	}
}

static void POP3_EmitCommand(pop3_con_t *pop3, char *text)
{
	int newlen;

	newlen = pop3->sendlen + strlen(text) + 2;

	if (newlen >= pop3->sendbuffersize || !pop3->sendbuffer)	//pre-length check.
	{
		char *newbuf;
		pop3->sendbuffersize = newlen*2;
		newbuf = malloc(pop3->sendbuffersize);
		if (!newbuf)
		{
			Con_Printf("Memory is low\n");
			pop3->drop = true;	//failed.
			return;
		}
		if (pop3->sendbuffer)
		{
			memcpy(newbuf, pop3->sendbuffer, pop3->sendlen);
			free(pop3->sendbuffer);
		}
		pop3->sendbuffer = newbuf;
	}


	snprintf(pop3->sendbuffer+pop3->sendlen, newlen+1, "%s\r\n", text);
	pop3->sendlen = newlen;

//	Con_Printf("^3%s\n", text);
}

static qboolean POP3_ThinkCon(pop3_con_t *pop3)	//false means drop the connection.
{
	char *ending;
	int len;

	if (pop3->state == POP3_NOTCONNECTED && !pop3->socket)
	{
		if (pop3->lastnoop + pop3_checkfrequency < Sys_Milliseconds())
		{	//we need to recreate the connection now.
			pop3->lastnoop = Sys_Milliseconds();
			POP3_CreateConnection(pop3->server, pop3->username, pop3->password);
		}

		return true;
	}

	//get the buffer, stick it in our read holder
	if (pop3->readlen+32 >= pop3->readbuffersize || !pop3->readbuffer)
	{
		len = pop3->readbuffersize;
		if (!pop3->readbuffer)
			pop3->readbuffersize = 256;
		else
			pop3->readbuffersize*=2;

		ending = malloc(pop3->readbuffersize);
		if (!ending)
		{
			Con_Printf("Memory is low\n");
			return false;
		}
		if (pop3->readbuffer)
		{
			memcpy(ending, pop3->readbuffer, len);
			free(pop3->readbuffer);
		}
		pop3->readbuffer = ending;
	}

	len = Net_Recv(pop3->socket, pop3->readbuffer+pop3->readlen, pop3->readbuffersize-pop3->readlen-1);
	if (len>0)
	{
		pop3->readlen+=len;
		pop3->readbuffer[pop3->readlen] = '\0';
	}

	if (pop3->readlen>0)
	{
		ending = strstr(pop3->readbuffer, "\r\n");

		if (ending)	//pollable text.
		{
			*ending = '\0';
//			Con_Printf("^2%s\n", pop3->readbuffer);

			ending+=2;
			if (pop3->state == POP3_WAITINGFORINITIALRESPONCE)
			{
				if (!strncmp(pop3->readbuffer, "+OK", 3))
				{
					char *angle1;
					char *angle2 = NULL;
					angle1 = strchr(pop3->readbuffer, '<');
					if (angle1)
					{
						angle2 = strchr(angle1+1, '>');
					}
					if (angle2)
					{	//just in case
						angle2[1] = '\0';

						POP3_EmitCommand(pop3, va("APOP %s %s", pop3->username, MD5_GetPop3APOPString(angle1, pop3->password)));
						pop3->state = POP3_AUTHING2;
					}
					else
					{
						POP3_EmitCommand(pop3, va("USER %s", pop3->username));
						pop3->state = POP3_AUTHING;
					}
				}
				else
				{
					Con_Printf("Unexpected response from POP3 server\n");
					return false;	//some sort of error.
				}
			}
			else if (pop3->state == POP3_AUTHING)
			{
				if (!strncmp(pop3->readbuffer, "+OK", 3))
				{
					POP3_EmitCommand(pop3, va("PASS %s", pop3->password));
					pop3->state = POP3_AUTHING2;
				}
				else
				{
					Con_Printf("Unexpected response from POP3 server.\nCheck username/password\n");
					return false;	//some sort of error.
				}
			}
			else if (pop3->state == POP3_AUTHING2)
			{
				if (!strncmp(pop3->readbuffer, "+OK", 3))
				{
					POP3_EmitCommand(pop3, "UIDL");
					pop3->state = POP3_LISTING;
					pop3->lastnoop = Sys_Milliseconds();
				}
				else
				{
					Con_Printf("Unexpected response from POP3 server.\nCheck username/password\n");
					return false;
				}
			}
			else if (pop3->state == POP3_LISTING)
			{
				if (!strncmp(pop3->readbuffer, "-ERR", 4))
				{
					Con_Printf("Unexpected response from POP3 server.\nUIDL not supported?\n");
					return false;
				}
				else if (!strncmp(pop3->readbuffer, "+OK", 3))
				{
				}
				else if (!strncmp(pop3->readbuffer, ".", 1))	//we only ever search for recent messages. So we fetch them and get sender and subject.
				{
					if (!pop3->numtoretrieve)
					{
						pop3->state = POP3_QUITTING;
						POP3_EmitCommand(pop3, "QUIT");
					}
					else
					{
						pop3->state = POP3_RETRIEVING;
						POP3_EmitCommand(pop3, va("RETR %i", pop3->retrlist[--pop3->numtoretrieve]));
					}
				}
				else
				{
					char *s;
					s = pop3->readbuffer;
					if (*s)
					{
						s++;
						while (*s >= '0' && *s <= '9')
							s++;
						while (*s == ' ')
							s++;
					}

					if (POP3_IsMessageUnique(s))
						if (pop3->numtoretrieve < sizeof(pop3->retrlist)/sizeof(pop3->retrlist[0]))
							pop3->retrlist[pop3->numtoretrieve++] = atoi(pop3->readbuffer);
				}
			}
			else if (pop3->state == POP3_RETRIEVING)
			{
				if (!strncmp(pop3->readbuffer, "+OK", 3))
				{
					pop3->msgsubject[0] = '\0';
					pop3->msgfrom[0] = '\0';

					pop3->state = POP3_HEADER;
				}
				else
				{	//erm... go for the next?
					if (!pop3->numtoretrieve)
					{
						pop3->state = POP3_QUITTING;
						POP3_EmitCommand(pop3, "QUIT");
					}
					else
						POP3_EmitCommand(pop3, va("RETR %i", pop3->retrlist[--pop3->numtoretrieve]));
				}
			}
			else if (pop3->state == POP3_HEADER)
			{
				if (!strnicmp(pop3->readbuffer, "From: ", 6))
					strlcpy(pop3->msgfrom, pop3->readbuffer + 6, sizeof(pop3->msgfrom));
				else if (!strnicmp(pop3->readbuffer, "Subject: ", 9))
					strlcpy(pop3->msgsubject, pop3->readbuffer + 9, sizeof(pop3->msgsubject));
				else if (!strncmp(pop3->readbuffer, ".", 1))
				{
					Con_Printf("New message:\nFrom: %s\nSubject: %s\n", pop3->msgfrom, pop3->msgsubject);

					if (BUILTINISVALID(SCR_CenterPrint))
						SCR_CenterPrint(va("NEW MAIL HAS ARRIVED\n\nTo: %s@%s\nFrom: %s\nSubject: %s", pop3->username, pop3->server, pop3->msgfrom, pop3->msgsubject));

					if (!pop3->numtoretrieve)
					{
						pop3->state = POP3_QUITTING;
						POP3_EmitCommand(pop3, "QUIT");
					}
					else
					{
						pop3->state = POP3_RETRIEVING;
						POP3_EmitCommand(pop3, va("RETR %i", pop3->retrlist[--pop3->numtoretrieve]));
					}
				}
				else if (!*pop3->readbuffer)
					pop3->state = POP3_BODY;
			}
			else if (pop3->state == POP3_BODY)
			{
				if (!strncmp(pop3->readbuffer, "..", 2))
				{
					//line of text, skipping first '.'
					Con_Printf("%s\n", pop3->readbuffer+1);
				}
				else if (!strncmp(pop3->readbuffer, ".", 1))
				{
					Con_Printf("New message:\nFrom: %s\nSubject: %s\n", pop3->msgfrom, pop3->msgsubject);
					if (BUILTINISVALID(SCR_CenterPrint))
						SCR_CenterPrint(va("NEW MAIL HAS ARRIVED\n\nTo: %s@%s\nFrom: %s\nSubject: %s", pop3->username, pop3->server, pop3->msgfrom, pop3->msgsubject));

					if (!pop3->numtoretrieve)
					{
						pop3->state = POP3_QUITTING;
						POP3_EmitCommand(pop3, "QUIT");
					}
					else
					{
						pop3->state = POP3_RETRIEVING;
						POP3_EmitCommand(pop3, va("RETR %i", pop3->retrlist[--pop3->numtoretrieve]));
					}
				}
				else
				{
					//normal line of text
					Con_Printf("%s\n", pop3->readbuffer);
				}
			}
			else if (pop3->state == POP3_QUITTING)
			{
				pop3->state = POP3_NOTCONNECTED;
				Net_Close(pop3->socket);
				pop3->lastnoop = Sys_Milliseconds();
				pop3->socket = 0;
				pop3->readlen = 0;
				pop3->sendlen = 0;
				return true;
			}
			else
			{
				Con_Printf("Bad client state\n");
				return false;
			}
			pop3->readlen -= ending - pop3->readbuffer;
			memmove(pop3->readbuffer, ending, strlen(ending)+1);
		}
	}
	if (pop3->drop)
		return false;

	if (pop3->sendlen)
	{
		len = Net_Send(pop3->socket, pop3->sendbuffer, pop3->sendlen);
		if (len>0)
		{
			pop3->sendlen-=len;
			memmove(pop3->sendbuffer, pop3->sendbuffer+len, pop3->sendlen+1);
		}
	}
	return true;
}

void POP3_Think (void)
{
	pop3_con_t *prev = NULL;
	pop3_con_t *pop3;

	for (pop3 = pop3sv; pop3; pop3 = pop3->next)
	{
		if (pop3->drop || !POP3_ThinkCon(pop3))
		{
			if (!prev)
				pop3sv = pop3->next;
			else
				prev->next = pop3->next;
			if (pop3->socket)
				Net_Close(pop3->socket);
			free(pop3);
			if (!prev)
				break;
		}

		prev = pop3;
	}
}


int EmailNotification_Frame(int *args)
{
	POP3_Think();
	IMAP_Think();

	return 0;
}

void IMAP_Account(void)
{
	char arg1[64];
	char arg2[64];
	char arg3[64];

	Cmd_Argv(1, arg1, sizeof(arg1));
	Cmd_Argv(2, arg2, sizeof(arg2));
	Cmd_Argv(3, arg3, sizeof(arg3));

	if (!*arg1)
	{
		Con_Printf("imapaccount <servername> <username> <password>\n");
	}
	else
		IMAP_CreateConnection(arg1, arg2, arg3);
}
void POP3_Account(void)
{
	char arg1[64];
	char arg2[64];
	char arg3[64];

	Cmd_Argv(1, arg1, sizeof(arg1));
	Cmd_Argv(2, arg2, sizeof(arg2));
	Cmd_Argv(3, arg3, sizeof(arg3));

	if (!*arg1)
	{
		Con_Printf("pop3account <servername> <username> <password>\n");
		Con_Printf("Say you had an acount called \"foo\" at yahoo's mail servers\n");
		Con_Printf("Yahoo's pop3 servers are named \"pop.mail.yahoo.co.uk\"\n");
		Con_Printf("Then if your password was bar, this is the command you would use\n");
		Con_Printf("pop3account pop.mail.yahoo.co.uk foo bar\n");
		Con_Printf("Of course, different pop3 servers have different naming conventions\n");
		Con_Printf("So read your provider's documentation\n");
	}
	else
		POP3_CreateConnection(arg1, arg2, arg3);
}

int EmailNotification_ExecuteCommand(int *args)
{
	char cmd[64];
	Cmd_Argv(0, cmd, sizeof(cmd));
	if (!strcmp(cmd, "imapaccount"))
	{
		IMAP_Account();
		return true;
	}
	if (!strcmp(cmd, "pop3account"))
	{
		POP3_Account();
		return true;
	}
	return false;
}

int Plug_Init(int *args)
{
	if (!Plug_Export("Tick", EmailNotification_Frame) || !Plug_Export("ExecuteCommand", EmailNotification_ExecuteCommand))
	{
		Con_Print("email notification plugin failed\n");
		return false;
	}

	Cmd_AddCommand("imapaccount");
	Cmd_AddCommand("pop3account");

	Con_Print("email notification plugin loaded\n");

	return true;
}
